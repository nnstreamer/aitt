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

#include <functional>
#include <map>
#include <mutex>

#define AITT_LOOP_EVENT_REMOVE 0
#define AITT_LOOP_EVENT_CONTINUE 1

namespace aitt {

class MainLoopIface {
  public:
    enum MainLoopResult {
        OK,
        ERROR,
        REMOVED,
        HANGUP,
    };
    struct MainLoopData {
        virtual ~MainLoopData() = default;
    };
    using mainLoopCB = std::function<int(MainLoopResult result, int fd, MainLoopData *data)>;

    MainLoopIface() = default;
    virtual ~MainLoopIface() = default;

    virtual void Run() = 0;
    virtual bool Quit() = 0;
    virtual void AddIdle(const mainLoopCB &cb, MainLoopData *user_data) = 0;
    virtual void AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data) = 0;
    virtual MainLoopData *RemoveWatch(int fd) = 0;
    virtual unsigned int AddTimeout(int interval, const mainLoopCB &cb,
          MainLoopData *user_data) = 0;
    virtual void RemoveTimeout(unsigned int id) = 0;
};

}  // namespace aitt
