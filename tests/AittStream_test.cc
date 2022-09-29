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
#include "AittStream.h"

#include <gtest/gtest.h>

#include "AITT.h"
#include "AittTests.h"

using namespace aitt;

TEST(AittStreamTest, Full_P)
{
    try {
        AITT aitt("streamClientId", LOCAL_IP, AittOption(true, false));

        aitt.Connect();

        AittStream *publisher =
              aitt.CreateStream(AITT_STREAM_TYPE_WEBRTC, "topic", AITT_STREAM_ROLE_PUBLISHER);
        ASSERT_TRUE(publisher) << "CreateStream() Fail";

        AittStream *subscriber =
              aitt.CreateStream(AITT_STREAM_TYPE_WEBRTC, "topic", AITT_STREAM_ROLE_SUBSCRIBER);
        ASSERT_TRUE(subscriber) << "CreateStream() Fail";

        publisher->SetConfig("key", "value");
        publisher->Start();

        subscriber->SetConfig("key", "value");
        subscriber->SetStateCallback([](AittStream *stream, int state, void *user_data) {},
              (void *)"user_data");
        subscriber->SetReceiveCallback([](AittStream *stream, void *obj, void *user_data) {},
              (void *)"user-data");
        subscriber->Start();

        aitt.DestroyStream(publisher);
        aitt.DestroyStream(subscriber);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
