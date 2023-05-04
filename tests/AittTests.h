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

#include <sys/time.h>

#include "AITT.h"
#include "MainLoopHandler.h"
#include "aitt_internal.h"

#define LOCAL_IP "127.0.0.1"
#define TEST_C_TOPIC "test/topic_c"
#define TEST_C_MSG "test123456789"
#define TEST_STRESS_TOPIC "test/stress1"

#define TEST_MSG "This is aitt test message"
#define TEST_MSG2 "This message is going to be delivered through a specified AittProtocol"
#define SLEEP_MS 1000

#define CHECK_INTERVAL 10

using aitt::MainLoopHandler;

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
    }

    void Deinit() {}

    void ToggleReady() { ready = true; }
    void ToggleReady2() { ready2 = true; }
    int ReadyCheck(void *data)
    {
        AittTests *test = static_cast<AittTests *>(data);

        if (test->ready) {
            test->StopEventLoop();
            return AITT_LOOP_EVENT_REMOVE;
        }

        return AITT_LOOP_EVENT_CONTINUE;
    }

    int ReadyAllCheck(void *data)
    {
        AittTests *test = static_cast<AittTests *>(data);

        if (test->ready && test->ready2) {
            test->StopEventLoop();
            return AITT_LOOP_EVENT_REMOVE;
        }

        return AITT_LOOP_EVENT_CONTINUE;
    }

    void StopEventLoop(void) { mainLoop.Quit(); }

    void IterateEventLoop(void)
    {
        mainLoop.Run();
        DBG("Go forward");
    }

    AittSubscribeID subscribeHandle;
    bool ready;
    bool ready2;

    MainLoopHandler mainLoop;
    std::string clientId;
    std::string testTopic;
};
