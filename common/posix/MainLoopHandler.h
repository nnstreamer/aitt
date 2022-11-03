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

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>

#include <deque>
#include <functional>
#include <map>
#include <mutex>

#include "aitt_internal.h"

#define BUF_SIZE 10

namespace aitt {

class MainLoopHandler {
  public:
    enum MainLoopResult {
        OK,
        ERROR,
        REMOVED,
        HANGUP,
    };

    enum PipeValue {
        INVALID = -3,
        TIMEOUT = -2,
        IDLE = -1,
    };

    struct TimeoutData {
        unsigned int timeout_id;
        int pipe_fd;
    };

    struct MainLoopData {
        virtual ~MainLoopData() = default;
    };
    using mainLoopCB = std::function<void(MainLoopResult result, int fd, MainLoopData *data)>;

    MainLoopHandler();
    ~MainLoopHandler();

    static void AddIdle(MainLoopHandler *handle, const mainLoopCB &cb, MainLoopData *user_data);
    static void WriteToPipe(int pipe_fd, int identifier);

    void Run();
    bool Quit();
    void AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data);
    MainLoopData *RemoveWatch(int fd);
    unsigned int AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *user_data);
    void RemoveTimeout(unsigned int id);

  private:
    struct MainLoopCbData {
        MainLoopCbData();
        mainLoopCB cb;
        MainLoopData *data;
        MainLoopResult result;
        int fd;
        unsigned int timeout_id;
    };

    using WatchMap = std::multimap<int, MainLoopCbData *>;
    using TimeoutMap = std::map<unsigned int, MainLoopCbData *>;
    using IdleQueue = std::deque<MainLoopCbData *>;

    void WatchTableInsert(int identifier, MainLoopCbData *cb_data);
    void TimeoutTableInsert(unsigned int identifier, MainLoopCbData *cb_data);
    void IdleTableInsert(MainLoopCbData *cb_data);
    void CheckWatch(pollfd *pfds, nfds_t nfds, short int event);
    void CheckTimeout(pollfd pfd, short int event);
    void CheckIdle(pollfd pfd, short int event);
    void SetTimer(int interval, unsigned int timeout_id);

    WatchMap watch_table;
    TimeoutMap timeout_table;
    IdleQueue idle_table;
    std::mutex table_lock;
    int timeout_pipe[2];
    int idle_pipe[2];

    bool is_running;
    bool is_idle;
};
}  // namespace aitt
