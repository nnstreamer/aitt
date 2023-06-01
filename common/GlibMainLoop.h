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
#pragma once

#include <glib.h>

#include <map>
#include <mutex>

#include "MainLoopIface.h"

namespace aitt {

class GlibMainLoop : public MainLoopIface {
  public:
    GlibMainLoop();
    ~GlibMainLoop();

    void Run() override;
    bool Quit() override;
    void AddIdle(const mainLoopCB &cb, MainLoopData *user_data) override;
    void AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data) override;
    MainLoopData *RemoveWatch(int fd) override;
    unsigned int AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *user_data) override;
    void RemoveTimeout(unsigned int id) override;

  private:
    struct MainLoopCbData {
        MainLoopCbData();
        mainLoopCB cb;
        MainLoopData *data;
        Event result;
        int fd;
        GMainContext *ctx;
    };
    using CallbackMap = std::map<int, std::pair<GSource *, MainLoopCbData *>>;

    static gboolean CallbackHandler(gpointer user_data);
    static gboolean EventHandler(GIOChannel *src, GIOCondition cond, gpointer user_data);
    static void DestroyNotify(gpointer data);

    GMainLoop *loop;
    CallbackMap callback_table;
    std::mutex callback_table_lock;
};
}  // namespace aitt
