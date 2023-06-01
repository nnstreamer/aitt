/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "PosixMainLoop.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "aitt_internal.h"

namespace aitt {

PosixMainLoop::PosixMainLoop() : timeout_pipe{}, idle_pipe{}, is_running(false)
{
    if (pipe(idle_pipe) == -1 || pipe(timeout_pipe) == -1) {
        ERR("pipe() Fail(%d)", errno);
        throw std::runtime_error("PosixMainLoop() Fail");
    }
    if (fcntl(idle_pipe[0], F_SETFL, O_NONBLOCK) == -1
          || fcntl(timeout_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        ERR("fcntl(O_NONBLOCK) Fail(%d)", errno);
        throw std::runtime_error("PosixMainLoop() Fail");
    }
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = TimerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        ERR("sigaction() Fail(%d)", errno);
        throw std::runtime_error("PosixMainLoop() Fail");
    }
}

PosixMainLoop::~PosixMainLoop()
{
    if (is_running)
        Quit();

    std::lock_guard<std::mutex> lock(table_lock);
    for (const auto &pair : timeout_table)
        delete pair.second;

    for (const auto &data : idle_table)
        delete data;

    timeout_table.clear();
    watch_table.clear();
    idle_table.clear();

    for (int iter = 0; iter < 2; iter++) {
        close(timeout_pipe[iter]);
        close(idle_pipe[iter]);
    }
}

void PosixMainLoop::Run()
{
    is_running = true;

    while (is_running) {
        table_lock.lock();
        struct pollfd pfd = {0};
        std::vector<struct pollfd> pfds;

        for (auto iter = watch_table.begin(); iter != watch_table.end(); iter++) {
            pfd.fd = iter->second->fd;
            pfd.events = POLLIN | POLLHUP | POLLERR;
            pfd.revents = 0;
            pfds.push_back(pfd);
        }
        table_lock.unlock();

        pfd.fd = idle_pipe[0];
        pfd.events = POLLIN;
        pfd.revents = 0;
        pfds.push_back(pfd);

        pfd.fd = timeout_pipe[0];
        pfd.events = POLLIN;
        pfd.revents = 0;
        pfds.push_back(pfd);

        if (0 < poll(pfds.data(), pfds.size(), -1)) {
            bool handled = false;
            int ret = CheckTimeout(pfds[pfds.size() - 1], POLLIN);
            if (ret < 0)
                break;
            handled |= !!ret;
            handled |= CheckWatch(pfds, pfds.size() - 2, POLLIN | POLLHUP | POLLERR);
            if (!handled)
                CheckIdle(pfds[pfds.size() - 2], POLLIN);
        }
        if (false == idle_table.empty())
            WriteToPipe(idle_pipe[1], IDLE);
    }
}

bool PosixMainLoop::Quit()
{
    if (is_running == false) {
        ERR("main loop is not running");
        return false;
    }

    is_running = false;
    WriteToPipe(timeout_pipe[1], QUIT);
    return true;
}

void PosixMainLoop::AddIdle(const mainLoopCB &cb, MainLoopData *user_data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = user_data;

    std::lock_guard<std::mutex> lock(table_lock);
    idle_table.push_back(cb_data);
    WriteToPipe(idle_pipe[1], IDLE);
}

void PosixMainLoop::AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = user_data;
    cb_data->fd = fd;

    std::lock_guard<std::mutex> lock(table_lock);
    watch_table.insert(WatchMap::value_type(cb_data->fd, cb_data));
    WriteToPipe(idle_pipe[1], PING);
}

PosixMainLoop::MainLoopData *PosixMainLoop::RemoveWatch(int fd)
{
    std::lock_guard<std::mutex> lock(table_lock);
    WatchMap::iterator iter = watch_table.find(fd);
    if (iter == watch_table.end()) {
        ERR("Unknown fd(%d)", fd);
        return nullptr;
    }
    MainLoopData *user_data = iter->second->data;
    watch_table.erase(iter);
    WriteToPipe(idle_pipe[1], PING);

    return user_data;
}

unsigned int PosixMainLoop::AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *data)
{
    static int identifier = TIMEOUT_START;

    if (interval < 0) {
        ERR("Invalid : interval(%d) < 0", interval);
        return 0;
    }

    TimeoutData *timeout_data = SetTimer(interval, identifier);
    if (nullptr != timeout_data) {
        MainLoopCbData *cb_data = new MainLoopCbData();
        cb_data->cb = cb;
        cb_data->data = data;
        cb_data->timeout_interval = interval;
        cb_data->timeout_data = timeout_data;
        TimeoutTableInsert(identifier, cb_data);
    } else {
        ERR("SetTimer() Fail");
        return 0;
    }

    return identifier++;
}

void PosixMainLoop::RemoveTimeout(unsigned int id)
{
    std::lock_guard<std::mutex> lock(table_lock);
    auto iter = timeout_table.find(id);
    if (iter != timeout_table.end()) {
        delete iter->second;
        timeout_table.erase(iter);
    }
}

void PosixMainLoop::WriteToPipe(int pipe_fd, unsigned int identifier)
{
    ssize_t ret = write(pipe_fd, (void *)&identifier, sizeof(identifier));
    if (ret != sizeof(identifier)) {
        ERR("write() Fail(%zd)", ret);
    }
}

void PosixMainLoop::TimerHandler(int sig, siginfo_t *si, void *uc)
{
    TimeoutData *timeout_data = static_cast<TimeoutData *>(si->si_value.sival_ptr);
    if (timeout_data == NULL) {
        ERR("timeout_data is NULL");
        return;
    }
    WriteToPipe(timeout_data->pipe_fd, timeout_data->timeout_id);
}

void PosixMainLoop::TimeoutTableInsert(unsigned int identifier, MainLoopCbData *cb_data)
{
    std::lock_guard<std::mutex> lock(table_lock);
    timeout_table.insert(TimeoutMap::value_type(identifier, cb_data));
}

bool PosixMainLoop::CheckWatch(const std::vector<struct pollfd> &pfds, nfds_t nfds, short int event)
{
    bool handled = false;
    for (nfds_t idx = 0; idx < nfds; idx++) {
        if (false == (pfds[idx].revents & event))
            continue;

        table_lock.lock();
        auto iter = watch_table.find(pfds[idx].fd);
        auto cb_data = (watch_table.end() == iter) ? nullptr : iter->second;
        table_lock.unlock();

        if (cb_data) {
            if (pfds[idx].revents & (POLLHUP | POLLERR))
                cb_data->result = (POLLHUP & pfds[idx].revents) ? Event::HANGUP : Event::ERROR;

            int ret = cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);
            handled = true;

            if (AITT_LOOP_EVENT_REMOVE == ret)
                RemoveWatch(pfds[idx].fd);
        }
    }
    return handled;
}

int PosixMainLoop::CheckTimeout(pollfd pfd, short int event)
{
    if (false == (pfd.revents & event))
        return false;

    bool handled = false;
    int identifier = PING;
    while (read(pfd.fd, &identifier, sizeof(identifier)) == sizeof(identifier)) {
        if (identifier == QUIT) {
            INFO("Terminating");
            return -1;
        }

        table_lock.lock();
        TimeoutMap::iterator iter = timeout_table.find(identifier);
        MainLoopCbData *cb_data = iter == timeout_table.end() ? NULL : iter->second;
        table_lock.unlock();

        if (cb_data) {
            int ret = cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);
            handled = true;

            if (AITT_LOOP_EVENT_REMOVE == ret)
                RemoveTimeout(identifier);
            else {
                if (nullptr != cb_data->timeout_data) {
                    delete cb_data->timeout_data;
                }
                cb_data->timeout_data = SetTimer(cb_data->timeout_interval, identifier);
                if (nullptr == cb_data->timeout_data) {
                    ERR("SetTimer() Fail");
                    return -1;
                }
            }
        }
    }
    return handled;
}

void PosixMainLoop::CheckIdle(pollfd pfd, short int event)
{
    if (false == (pfd.revents & event))
        return;

    int identifier = PING;
    if (read(pfd.fd, &identifier, sizeof(identifier)) == sizeof(identifier)) {
        if (identifier == IDLE) {
            table_lock.lock();
            MainLoopCbData *cb_data = idle_table.empty() ? NULL : idle_table.front();
            table_lock.unlock();

            if (cb_data) {
                int ret = cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);

                if (AITT_LOOP_EVENT_REMOVE == ret) {
                    table_lock.lock();
                    idle_table.pop_front();
                    delete cb_data;
                    table_lock.unlock();
                }
            }
        }
    }
}

PosixMainLoop::TimeoutData *PosixMainLoop::SetTimer(int interval, unsigned int identifier)
{
    TimeoutData *data = new TimeoutData;
    data->pipe_fd = timeout_pipe[1];
    data->timeout_id = identifier;

    struct sigevent se;
    memset(&se, 0, sizeof(se));
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGUSR1;
    se.sigev_value.sival_ptr = static_cast<void *>(data);

    timer_t timerid;
    if (timer_create(CLOCK_MONOTONIC, &se, &timerid) == -1) {
        ERR("timer_create() Fail(%d)", errno);
        delete data;
        return nullptr;
    }
    data->timerid = timerid;

    struct itimerspec its;
    memset(&its, 0, sizeof(its));

    long sec = interval / 1000;
    long msec = interval % 1000;

    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = msec * 1000 * 1000;
    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        ERR("timer_settime() Fail(%d)", errno);
        delete data;
        return nullptr;
    }
    return data;
}

PosixMainLoop::TimeoutData::TimeoutData() : timerid(nullptr), timeout_id(0), pipe_fd(-1)
{
}
PosixMainLoop::TimeoutData::~TimeoutData()
{
    if (nullptr != timerid) {
        if (timer_delete(timerid) == -1) {
            ERR("timer_delete() Fail(%d)", errno);
        }
    }
}

PosixMainLoop::MainLoopCbData::MainLoopCbData()
      : data(nullptr), result(Event::OKAY), fd(IDLE), timeout_interval(0), timeout_data(nullptr)
{
}

PosixMainLoop::MainLoopCbData::~MainLoopCbData()
{
    if (nullptr != timeout_data) {
        delete timeout_data;
    }
}

}  // namespace aitt
