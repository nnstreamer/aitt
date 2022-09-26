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

#include "aitt_internal.h"

#define LOCAL_IP "127.0.0.1"
#define TEST_STREAM_TOPIC "test/stream_topic"
#define TEST_CONFIG_KEY "test_config_key"
#define TEST_CONFIG_VALUE "test_config_value"

class AittStreamTests {
  public:
    void Init()
    {
        timeval tv;
        char buffer[256];
        gettimeofday(&tv, nullptr);
        snprintf(buffer, sizeof(buffer), "UniqueStreamSrcID.%lX%lX", tv.tv_sec, tv.tv_usec);
        stream_src_id_ = buffer;
        snprintf(buffer, sizeof(buffer), "UniqueStreamSinkID.%lX%lX", tv.tv_sec, tv.tv_usec);
        stream_sink_id_ = buffer;
        stream_topic_ = "StreamTopic";
        mainLoop_ = g_main_loop_new(nullptr, FALSE);
    }

    void Deinit() { g_main_loop_unref(mainLoop_); }

    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop_);
        DBG("Go forward");
    }

    AittStreamID publish_stream_handle_;
    AittStreamID subscribe_stream_handle_;

    std::string stream_src_id_;
    std::string stream_sink_id_;
    std::string stream_topic_;
    GMainLoop *mainLoop_;
};
