/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
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
#include "MainLoopHandler.h"

namespace aitt {

static void TimerHandler(int sig, siginfo_t *si, void *uc)
{
    struct MainLoopHandler::TimeoutData *data =
          (MainLoopHandler::TimeoutData *)si->si_value.sival_ptr;
    if (data == NULL)
        ERR("sival_ptr is null");
    MainLoopHandler::WriteToPipe(data->pipe_fd, data->timeout_id);
    delete data;
}

MainLoopHandler::MainLoopHandler() : timeout_pipe{}, idle_pipe{}, is_running(false), is_idle(false)
{
    if (pipe(idle_pipe) == -1 || pipe(timeout_pipe) == -1) {
        ERR("failed to create pipe");
    }
    if (fcntl(idle_pipe[0], F_SETFL, O_NONBLOCK) == -1
          || fcntl(timeout_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        ERR("failed to set NonBlock");
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = TimerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        ERR("failed to invoke sigaction..");
}

MainLoopHandler::~MainLoopHandler()
{
    if (is_running)
        Quit();

    std::lock_guard<std::mutex> lock(table_lock);
    for (auto iter = timeout_table.begin(); iter != timeout_table.end(); iter++) {
        delete iter->second;
    }
    for (auto iter = watch_table.begin(); iter != watch_table.end(); iter++) {
        delete iter->second;
    }
    for (auto iter = idle_table.begin(); iter != idle_table.end(); iter++) {
        delete *iter;
    }
    timeout_table.clear();
    watch_table.clear();
    idle_table.clear();

    for (int iter = 0; iter < 2; iter++) {
        close(timeout_pipe[iter]);
        close(idle_pipe[iter]);
    }
}

void MainLoopHandler::Run()
{
    is_running = true;

    while (is_running) {
        table_lock.lock();
        is_idle = true;
        struct pollfd pfds[watch_table.size() + 2];
        nfds_t nfds = 0;

        // for Watch
        for (auto iter = watch_table.begin(); iter != watch_table.end(); iter++) {
            pfds[nfds++] = {iter->second->fd, POLLIN | POLLHUP | POLLERR, 0};
        }
        // for idle
        pfds[nfds++] = {idle_pipe[0], POLLIN, 0};
        // for timeout
        pfds[nfds++] = {timeout_pipe[0], POLLIN, 0};
        table_lock.unlock();

        if (0 < poll(pfds, nfds, -1)) {
            CheckTimeout(pfds[nfds - 1], POLLIN);
            CheckWatch(pfds, nfds - 2, POLLIN | POLLHUP | POLLERR);
            if (is_idle)
                CheckIdle(pfds[nfds - 2], POLLIN);
        }
    }
}

bool MainLoopHandler::Quit()
{
    if (is_running == false) {
        ERR("main loop is not running");
        return false;
    }

    is_running = false;
    WriteToPipe(timeout_pipe[1], INVALID);
    return true;
}

void MainLoopHandler::AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = user_data;
    cb_data->fd = fd;

    WatchTableInsert(cb_data->fd, cb_data);
}

MainLoopHandler::MainLoopData *MainLoopHandler::RemoveWatch(int fd)
{
    MainLoopData *user_data = nullptr;

    std::lock_guard<std::mutex> lock(table_lock);
    WatchMap::iterator iter = watch_table.find(fd);
    if (iter == watch_table.end())
        return user_data;
    user_data = iter->second->data;
    watch_table.erase(iter);

    return user_data;
}

unsigned int MainLoopHandler::AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *data)
{
    if (interval < 0)
        ERR("interval must be greater then or equal to zero");
    static unsigned int identifier = 1;

    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = data;
    cb_data->fd = PipeValue::TIMEOUT;
    cb_data->timeout_id = identifier++;

    TimeoutTableInsert(cb_data->timeout_id, cb_data);
    SetTimer(interval, cb_data->timeout_id);
    return cb_data->timeout_id;
}

void MainLoopHandler::RemoveTimeout(unsigned int id)
{
    std::lock_guard<std::mutex> lock(table_lock);
    TimeoutMap::iterator iter = timeout_table.find(id);
    if (iter != timeout_table.end()) {
        delete iter->second;
        timeout_table.erase(iter);
    }
}

void MainLoopHandler::AddIdle(MainLoopHandler *handle, const mainLoopCB &cb,
      MainLoopData *user_data)
{
    RET_IF(handle == nullptr);

    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = user_data;

    handle->IdleTableInsert(cb_data);
}

void MainLoopHandler::WriteToPipe(int pipe_fd, int identifier)
{
    if (write(pipe_fd, (const void *)(&identifier), sizeof(int)) != sizeof(int)) {
        ERR("write fail");
    }
}

void MainLoopHandler::WatchTableInsert(int identifier, MainLoopCbData *cb_data)
{
    std::lock_guard<std::mutex> lock(table_lock);
    watch_table.insert(WatchMap::value_type(identifier, cb_data));
    WriteToPipe(idle_pipe[1], INVALID);
}

void MainLoopHandler::TimeoutTableInsert(unsigned int identifier, MainLoopCbData *cb_data)
{
    std::lock_guard<std::mutex> lock(table_lock);
    timeout_table.insert(TimeoutMap::value_type(identifier, cb_data));
}

void MainLoopHandler::IdleTableInsert(MainLoopCbData *cb_data)
{
    std::lock_guard<std::mutex> lock(table_lock);
    idle_table.push_back(cb_data);
    WriteToPipe(idle_pipe[1], IDLE);
}

void MainLoopHandler::CheckWatch(pollfd *pfds, nfds_t nfds, short int event)
{
    for (unsigned long int idx = 0; idx < nfds; idx++) {
        if (pfds[idx].revents & event) {
            table_lock.lock();
            WatchMap::iterator wt_map = watch_table.find(pfds[idx].fd);
            MainLoopCbData *cb_data = wt_map == watch_table.end() ? NULL : wt_map->second;
            table_lock.unlock();

            if (cb_data) {
                if (pfds[idx].revents & (POLLHUP | POLLERR)) {
                    cb_data->result = (POLLHUP & pfds[idx].revents) ? HANGUP : ERROR;
                }
                cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);
                is_idle = false;
            }
        }
    }
}

void MainLoopHandler::CheckTimeout(pollfd pfd, short int event)
{
    if (pfd.revents & event) {
        int identifier = INVALID;
        while (read(pfd.fd, &identifier, sizeof(int)) == sizeof(int)) {
            if (identifier == INVALID)
                continue;

            table_lock.lock();
            TimeoutMap::iterator tm_map = timeout_table.find((unsigned int)(identifier));
            MainLoopCbData *cb_data = tm_map == timeout_table.end() ? NULL : tm_map->second;
            table_lock.unlock();

            if (cb_data) {
                cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);
                is_idle = false;

                table_lock.lock();
                tm_map = timeout_table.find((unsigned int)(identifier));
                if (tm_map != timeout_table.end()) {
                    delete tm_map->second;
                    timeout_table.erase(tm_map);
                }
                table_lock.unlock();
            }
        }
    }
}

void MainLoopHandler::CheckIdle(pollfd pfd, short int event)
{
    if (pfd.revents & event) {
        int identifier = INVALID;
        if (read(pfd.fd, &identifier, sizeof(int)) == sizeof(int)) {
            if (identifier == IDLE) {
                table_lock.lock();
                MainLoopCbData *cb_data = idle_table.empty() ? NULL : idle_table.front();
                table_lock.unlock();

                if (cb_data) {
                    cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);

                    table_lock.lock();
                    idle_table.pop_front();
                    delete cb_data;
                    table_lock.unlock();
                }
            }
        }
    }
}

void MainLoopHandler::SetTimer(int interval, unsigned int timeout_id)
{
    struct sigevent se;
    timer_t timer;
    struct itimerspec its;

    struct TimeoutData *data = new TimeoutData();
    data->pipe_fd = timeout_pipe[1];
    data->timeout_id = timeout_id;

    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIGUSR1;
    se.sigev_value.sival_ptr = (void *)(data);

    long sec = interval / 1000;
    long msec = interval % 1000;

    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = msec * 1000 * 1000;

    if (timer_create(CLOCK_MONOTONIC, &se, &timer) == -1)
        ERR("failed to create timer");
    if (timer_settime(timer, 0, &its, NULL) == -1)
        ERR("failed to set timer");
}

MainLoopHandler::MainLoopCbData::MainLoopCbData()
      : data(nullptr), result(OK), fd(IDLE), timeout_id(0)
{
}

}  // namespace aitt
