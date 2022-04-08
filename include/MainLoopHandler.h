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

#include <glib.h>

#include <functional>
#include <map>
#include <mutex>

#include <AittTypes.h>

namespace aitt {

class API MainLoopHandler {
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
    using mainLoopCB = std::function<void(MainLoopResult result, int fd, MainLoopData *data)>;

    MainLoopHandler();
    ~MainLoopHandler();

    static void AddIdle(MainLoopHandler *handle, const mainLoopCB &cb, MainLoopData *user_data);

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
        GMainContext *ctx;
    };
    using CallbackMap = std::map<int, std::pair<GSource *, MainLoopCbData *>>;

    static void AddIdle(MainLoopCbData *, GDestroyNotify);
    static gboolean IdlerHandler(gpointer user_data);
    static gboolean EventHandler(GIOChannel *src, GIOCondition cond, gpointer user_data);
    static void DestroyNotify(gpointer data);

    GMainLoop *loop;
    CallbackMap callback_table;
    std::mutex callback_table_lock;
};
}  // namespace aitt
