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
#pragma once

#include <poll.h>
#include <signal.h>
#include <time.h>

#include <deque>
#include <map>
#include <memory>
#include <mutex>

#include "MainLoopIface.h"

namespace aitt {

class PosixMainLoop : public MainLoopIface {
  public:
    PosixMainLoop();
    virtual ~PosixMainLoop();

    void Run() override;
    bool Quit() override;
    void AddIdle(const mainLoopCB &cb, MainLoopData *user_data) override;
    void AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data) override;
    MainLoopData *RemoveWatch(int fd) override;
    unsigned int AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *user_data) override;
    void RemoveTimeout(unsigned int id) override;

  private:
    enum PipeValue {
        QUIT = -1,
        PING = 0,
        IDLE = 1,
        TIMEOUT_START = 2,
    };

    struct TimeoutData {
        unsigned int timeout_id;
        int pipe_fd;
    };

    struct MainLoopCbData {
        MainLoopCbData();
        ~MainLoopCbData();
        mainLoopCB cb;
        MainLoopData *data;
        MainLoopResult result;
        int fd;
        int timeout_interval;
        timer_t timerid;
    };

    using WatchMap = std::map<int, std::shared_ptr<MainLoopCbData>>;
    using TimeoutMap = std::map<unsigned int, MainLoopCbData *>;
    using IdleQueue = std::deque<MainLoopCbData *>;

    static void WriteToPipe(int pipe_fd, unsigned int identifier);
    static void TimerHandler(int sig, siginfo_t *si, void *uc);

    void TimeoutTableInsert(unsigned int identifier, MainLoopCbData *cb_data);
    bool CheckWatch(pollfd *pfds, nfds_t nfds, short int event);
    int CheckTimeout(pollfd pfd, short int event);
    void CheckIdle(pollfd pfd, short int event);
    timer_t SetTimer(int interval, unsigned int identifier);

    WatchMap watch_table;
    TimeoutMap timeout_table;
    IdleQueue idle_table;
    std::mutex table_lock;
    int timeout_pipe[2];
    int idle_pipe[2];
    bool is_running;
};

}  // namespace aitt
