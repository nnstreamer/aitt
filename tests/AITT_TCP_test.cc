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
#include <glib.h>
#include <gtest/gtest.h>

#include <thread>

#include "AITT.h"
#include "AittTests.h"
#include "aitt_internal.h"

using AITT = aitt::AITT;

class AITTTCPTest : public testing::Test, public AittTests {
  protected:
    void SetUp() override { Init(); }
    void TearDown() override { Deinit(); }

    void TCPWildcardsTopicTemplate(AittProtocol protocol)
    {
        try {
            char dump_msg[204800];

            AITT aitt(clientId, LOCAL_IP);
            aitt.Connect();

            int cnt = 0;
            aitt.Subscribe(
                  "test/+",
                  [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);
                      INFO("Got Message(Topic:%s, size:%d)", handle->GetTopic().c_str(), szmsg);
                      ++cnt;

                      std::stringstream ss;
                      ss << "test/value" << cnt;
                      EXPECT_EQ(ss.str(), handle->GetTopic());

                      if (cnt == 3)
                          test->ToggleReady();
                  },
                  static_cast<void *>(this), protocol);

            // Wait a few seconds until the AITT client gets a server list (discover devices)
            DBG("Sleep %d secs", SLEEP_MS);
            sleep(SLEEP_MS);

            aitt.Publish("test/value1", dump_msg, 12, protocol);
            aitt.Publish("test/value2", dump_msg, 1600, protocol);
            aitt.Publish("test/value3", dump_msg, 1600, protocol);

            g_timeout_add(10, AittTests::ReadyCheck, static_cast<AittTests *>(this));

            IterateEventLoop();

            ASSERT_TRUE(ready);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
};

TEST_F(AITTTCPTest, TCP_Wildcards1_Anytime)
{
    try {
        char dump_msg[204800];

        AITT aitt(clientId, LOCAL_IP);
        aitt.Connect();

        aitt.Subscribe(
              "test/#",
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);
                  INFO("Got Message(Topic:%s, size:%d)", handle->GetTopic().c_str(), szmsg);
                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 3)
                      test->ToggleReady();
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        // Wait a few seconds until the AITT client gets a server list (discover devices)
        DBG("Sleep %d secs", SLEEP_MS);
        sleep(SLEEP_MS);

        aitt.Publish("test/step1/value1", dump_msg, 12, AITT_TYPE_TCP);
        aitt.Publish("test/step2/value1", dump_msg, 1600, AITT_TYPE_TCP);
        aitt.Publish("test/step2/value1", dump_msg, 1600, AITT_TYPE_TCP);

        g_timeout_add(10, AittTests::ReadyCheck, static_cast<AittTests *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTCPTest, TCP_Wildcards2_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP);
}

TEST_F(AITTTCPTest, SECURE_TCP_Wildcards_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP_SECURE);
}
