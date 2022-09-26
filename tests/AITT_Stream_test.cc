/*
 * Copyright (c) 2021-2022 Samsung Electronics Co., Ltd All Rights Reserved
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
#include <gtest/gtest.h>
#include <sys/random.h>

#include <thread>

#include "AITT.h"
#include "AITT_Stream_tests.h"

using AITT = aitt::AITT;

class AITTStreamTest : public testing::Test, public AittStreamTests {
  protected:
    void SetUp() override { Init(); }
    void TearDown() override { Deinit(); }
};

TEST_F(AITTStreamTest, Positive_Create_Pubilsh_Stream_WebRTC_Anytime)
{
    try {
        AITT aitt(stream_src_id_, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        publish_stream_handle_ =
              aitt.CreatePublishStream(TEST_STREAM_TOPIC, AITT_TYPE_WEBRTC);
        EXPECT_NE(publish_stream_handle_, nullptr);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTStreamTest, Positive_Create_Subscribe_Stream_WebRTC_Anytime)
{
    try {
        AITT aitt(stream_src_id_, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribe_stream_handle_ =
              aitt.CreateSubscribeStream(TEST_STREAM_TOPIC, AITT_TYPE_WEBRTC);
        EXPECT_NE(subscribe_stream_handle_, nullptr);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTStreamTest, Positive_Destroy_Pubilsh_Stream_WebRTC_Anytime)
{
    try {
        AITT aitt(stream_src_id_, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        publish_stream_handle_ =
              aitt.CreatePublishStream(TEST_STREAM_TOPIC, AITT_TYPE_WEBRTC);
        aitt.DestroyStream(publish_stream_handle_);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTStreamTest, Positive_Destroy_Subscribe_Stream_WebRTC_Anytime)
{
    try {
        AITT aitt(stream_src_id_, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribe_stream_handle_ =
              aitt.CreateSubscribeStream(TEST_STREAM_TOPIC, AITT_TYPE_WEBRTC);
        aitt.DestroyStream(subscribe_stream_handle_);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTStreamTest, Negative_Destroy_Stream_Anytime)
{
    EXPECT_THROW(
          {
              try {
                  AITT aitt(stream_src_id_, LOCAL_IP, AittOption(true, false));
                  aitt.Connect();
                  aitt.DestroyStream(nullptr);
              } catch (const std::runtime_error &e) {
                  // and this tests that it has the correct message
                  throw;
              }
          },
          std::runtime_error);
}
