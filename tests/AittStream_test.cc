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
#include "AittStream.h"

#include <gtest/gtest.h>

#include "AITT.h"
#include "AittTests.h"

using namespace aitt;
#ifdef WITH_WEBRTC
#include <glib.h>
class AITTWEBRTCTest : public testing::Test {
  protected:
    void SetUp() override
    {
        main_loop = g_main_loop_new(NULL, FALSE);
        aitt = new AITT("streamClientId", LOCAL_IP, AittOption(true, false));
        aitt->Connect();

        publisher =
              aitt->CreateStream(AITT_STREAM_TYPE_WEBRTC, "topic", AITT_STREAM_ROLE_PUBLISHER);
        ASSERT_TRUE(publisher) << "CreateStream() Fail";

        subscriber =
              aitt->CreateStream(AITT_STREAM_TYPE_WEBRTC, "topic", AITT_STREAM_ROLE_SUBSCRIBER);
        ASSERT_TRUE(subscriber) << "CreateStream() Fail";
    }
    void TearDown() override
    {
        aitt->DestroyStream(publisher);
        aitt->DestroyStream(subscriber);
        aitt->Disconnect();
        delete aitt;
        g_main_loop_unref(main_loop);
    }

    AITT *aitt;
    AittStream *publisher;
    AittStream *subscriber;
    GMainLoop *main_loop;
};

TEST_F(AITTWEBRTCTest, Default_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  if (stream == nullptr) {
                      printf("Invalid stream\n");
                      return;
                  }

                  DBG("ReceiveCallback Called");
                  if (g_main_loop_is_running(main_loop))
                      g_main_loop_quit(main_loop);
              },
              nullptr);
        subscriber->Start();
        publisher->Start();

        g_main_loop_run(main_loop);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
#define SD_WIDTH 640
#define SD_HEIGHT 480

TEST_F(AITTWEBRTCTest, Set_Resolution_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  if (stream == nullptr) {
                      printf("Invalid stream\n");
                      return;
                  }

                  DBG("ReceiveCallback Called");
                  EXPECT_EQ(stream->GetWidth(), SD_WIDTH);
                  EXPECT_EQ(stream->GetHeight(), SD_HEIGHT);
                  if (g_main_loop_is_running(main_loop))
                      g_main_loop_quit(main_loop);
              },
              nullptr);
        subscriber->Start();

        publisher->SetConfig("WIDTH", std::to_string(SD_WIDTH))
              ->SetConfig("HEIGHT", std::to_string(SD_HEIGHT))
              ->Start();

        g_main_loop_run(main_loop);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
#define HD_WIDTH 1280
#define HD_HEIGHT 720
#define FRAME_RATE_10 10

TEST_F(AITTWEBRTCTest, Set_Resolution_Frame_Rate_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  if (stream == nullptr) {
                      printf("Invalid stream\n");
                      return;
                  }

                  DBG("ReceiveCallback Called");
                  EXPECT_EQ(stream->GetWidth(), HD_WIDTH);
                  EXPECT_EQ(stream->GetHeight(), HD_HEIGHT);
                  if (g_main_loop_is_running(main_loop))
                      g_main_loop_quit(main_loop);
              },
              nullptr);
        subscriber->Start();

        publisher->SetConfig("WIDTH", std::to_string(HD_WIDTH))
              ->SetConfig("HEIGHT", std::to_string(HD_HEIGHT))
              ->SetConfig("FRAME_RATE", std::to_string(FRAME_RATE_10))
              ->Start();

        g_main_loop_run(main_loop);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

#define MEDIA_FORMAT_I420 "I420"
TEST_F(AITTWEBRTCTest, Set_Source_Type_Media_Packet_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  if (stream == nullptr) {
                      printf("Invalid stream\n");
                      return;
                  }

                  DBG("ReceiveCallback Called");
                  if (g_main_loop_is_running(main_loop))
                      g_main_loop_quit(main_loop);
              },
              nullptr);
        subscriber->Start();

        publisher->SetConfig("SOURCE_TYPE", "MEDIA_PACKET")
              ->SetConfig("WIDTH", std::to_string(HD_WIDTH))
              ->SetConfig("HEIGHT", std::to_string(HD_HEIGHT))
              ->SetConfig("FRAME_RATE", std::to_string(FRAME_RATE_10))
              ->SetConfig("MEDIA_FORMAT", MEDIA_FORMAT_I420)
              ->Start();

        g_main_loop_run(main_loop);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTWEBRTCTest, SetConfig_param_N_)
{
    EXPECT_THROW({ publisher->SetConfig("UNKNOWN", ""); }, aitt::AittException);
    EXPECT_THROW({ subscriber->SetConfig("UNKNOWN", ""); }, aitt::AittException);
}

TEST_F(AITTWEBRTCTest, SetConfig_after_start_N_)
{
    EXPECT_THROW(
          {
              publisher->Start();
              publisher->SetConfig("WIDTH", std::to_string(HD_WIDTH));
          },
          aitt::AittException);
}

#endif  // WITH_WEBRTC

#ifdef WITH_RTSP
class AITTRTSPTest : public testing::Test {
  protected:
    void SetUp() override
    {
        main_loop = aitt::MainLoopHandler::new_loop();

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
        delete main_loop;
    }

    AITT *aitt;
    AittStream *publisher;
    AittStream *subscriber;
    MainLoopIface *main_loop;
};

TEST_F(AITTRTSPTest, Publisher_SetConfig_N_)
{
    EXPECT_THROW(
          {
              publisher->SetConfig("URI",
                    "192.168.1.52:554/cam/realmonitor?channel=1&subtype=0&authbasic=64");
          },
          aitt::AittException);
}

TEST_F(AITTRTSPTest, Publisher_First_P)
{
    try {
        publisher
              ->SetConfig("URI",
                    "rtsp://192.168.1.52:554/cam/realmonitor?channel=1&subtype=0&authbasic=64")
              ->SetConfig("ID", "admin")
              ->SetConfig("Password", "admin")
              ->Start();

        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  DBG("ReceiveCallback Called");
                  main_loop->Quit();
              },
              nullptr);

        main_loop->AddTimeout(
              3000,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  subscriber->Start();
                  return AITT_LOOP_EVENT_REMOVE;
              },
              nullptr);
        main_loop->Run();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTRTSPTest, Subscriber_SetConfig_N_)
{
    EXPECT_THROW({ subscriber->SetConfig("FPS", "NOT_NUMBER"); }, aitt::AittException);
}

TEST_F(AITTRTSPTest, Subscriber_First_P)
{
    try {
        subscriber->SetReceiveCallback(
              [&](AittStream *stream, void *obj, void *user_data) {
                  DBG("ReceiveCallback Called");
                  main_loop->Quit();
              },
              nullptr);
        subscriber->SetConfig("FPS", "1")->Start();

        publisher
              ->SetConfig("URI",
                    "rtsp://192.168.1.52:554/cam/realmonitor?channel=1&subtype=0&authbasic=64")
              ->SetConfig("ID", "admin")
              ->SetConfig("Password", "admin")
              ->Start();

        main_loop->Run();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
#endif  // WITH_RTSP
