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
#include <sys/time.h>

#include "AITT.h"
#include "aitt_internal.h"

#define LOCAL_IP "127.0.0.1"
#define TEST_C_TOPIC "test/topic_c"
#define TEST_C_MSG "test123456789"
#define TEST_STRESS_TOPIC "test/stress1"

#define TEST_MSG "This is aitt test message"
#define TEST_MSG2 "This message is going to be delivered through a specified AittProtocol"
#define SLEEP_MS 1000

class AittTests {
  public:
    void Init()
    {
        ready = false;
        ready2 = false;

        timeval tv;
        char buffer[256];
        gettimeofday(&tv, nullptr);
        snprintf(buffer, sizeof(buffer), "UniqueID.%lX%lX", tv.tv_sec, tv.tv_usec);
        clientId = buffer;
        snprintf(buffer, sizeof(buffer), "TestTopic.%lX%lX", tv.tv_sec, tv.tv_usec);
        testTopic = buffer;
        mainLoop = g_main_loop_new(nullptr, FALSE);
    }

    void Deinit() { g_main_loop_unref(mainLoop); }

    void ToggleReady() { ready = true; }
    void ToggleReady2() { ready2 = true; }
    static gboolean ReadyCheck(gpointer data)
    {
        AittTests *test = static_cast<AittTests *>(data);

        if (test->ready) {
            g_main_loop_quit(test->mainLoop);
            return G_SOURCE_REMOVE;
        }

        return G_SOURCE_CONTINUE;
    }

    void StopEventLoop(void) { g_main_loop_quit(mainLoop); }

    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop);
        DBG("Go forward");
    }

    AittSubscribeID subscribeHandle;
    bool ready;
    bool ready2;

    GMainLoop *mainLoop;
    std::string clientId;
    std::string testTopic;
};
