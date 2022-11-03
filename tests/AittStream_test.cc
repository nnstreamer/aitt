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

TEST(AittStreamTest, Webrtc_Subscriber_Create_P)
{
    try {
        AITT aitt("streamClientId", LOCAL_IP, AittOption(true, false));

        aitt.Connect();

        AittStream *subscriber =
              aitt.CreateStream(AITT_STREAM_TYPE_WEBRTC, "topic", AITT_STREAM_ROLE_SUBSCRIBER);
        ASSERT_TRUE(subscriber) << "CreateStream() Fail";

    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

class AITTRTSPTest : public testing::Test {
  protected:
    void SetUp() override
    {
        aitt = new AITT("streamClientId", LOCAL_IP, AittOption(true, false));
        aitt->Connect();

        publisher = aitt->CreateStream(AITT_STREAM_TYPE_RTSP, "topic", AITT_STREAM_ROLE_PUBLISHER);
        ASSERT_TRUE(publisher) << "CreateStream() Fail";

        subscriber =
              aitt->CreateStream(AITT_STREAM_TYPE_RTSP, "topic", AITT_STREAM_ROLE_SUBSCRIBER);
        ASSERT_TRUE(subscriber) << "CreateStream() Fail";
    }
    void TearDown() override
    {
        aitt->DestroyStream(publisher);
        aitt->DestroyStream(subscriber);
        aitt->Disconnect();
        delete aitt;
    }

    AITT *aitt;
    AittStream *publisher;
    AittStream *subscriber;
    MainLoopHandler main_loop;
};

TEST_F(AITTRTSPTest, Publisher_First_P)
{
    try {
        publisher->SetConfig("url",
              "rtsp://192.168.1.52:554/cam/realmonitor?channel=1&subtype=0&authbasic=64");
        publisher->SetConfig("id", "admin");
        publisher->SetConfig("password", "admin");
        publisher->Start();

        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  DBG("ReceiveCallback Called");
                  main_loop.Quit();
              },
              nullptr);

        main_loop.AddTimeout(
              3000,
              [&](MainLoopHandler::MainLoopResult result, int fd,
                    MainLoopHandler::MainLoopData *data) { subscriber->Start(); },
              nullptr);
        main_loop.Run();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTRTSPTest, Subscriber_First_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  DBG("ReceiveCallback Called");
                  main_loop.Quit();
              },
              nullptr);
        subscriber->Start();

        publisher->SetConfig("url",
              "rtsp://192.168.1.52:554/cam/realmonitor?channel=1&subtype=0&authbasic=64");
        publisher->SetConfig("id", "admin");
        publisher->SetConfig("password", "admin");
        publisher->Start();

        main_loop.Run();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
