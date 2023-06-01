/*
 * Copyright 2021-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <algorithm>
#include <climits>
#include <random>

#include "AITT.h"
#include "AittTests.h"
#include "aitt_internal.h"

using AITT = aitt::AITT;

class AITTTCPTest : public testing::Test, public AittTests {
  protected:
    void SetUp() override { Init(); }
    void TearDown() override { Deinit(); }

    void TCPWildcardsTopicTemplate(AittProtocol protocol, bool single_level)
    {
        try {
            std::mt19937 random_gen{std::random_device{}()};
            std::uniform_int_distribution<std::string::size_type> gen(0, 255);

            char dump_msg[204800] = {};
            for (size_t i = 0; i < sizeof(dump_msg); i++) {
                dump_msg[i] = gen(random_gen);
            }

            std::string sub_topic = "test/" + std::string(single_level ? "+" : "#");

            AITT aitt(clientId, LOCAL_IP);

            int cnt = 0;
            aitt.SetConnectionCallback([&](AITT &handle, int status, void *user_data) {
                if (status != AITT_CONNECTED)
                    return;
                aitt.Subscribe(
                      sub_topic,
                      [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                          AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);
                          INFO("Got Message(Topic:%s, size:%d)", handle->GetTopic().c_str(), szmsg);
                          ++cnt;

                          if (cnt == 3)
                              test->ToggleReady();
                      },
                      static_cast<void *>(this), protocol);

                // Wait a few seconds until the AITT client gets a server list (discover devices)
                while (aitt.CountSubscriber("test/value1", protocol) == 0) {
                    usleep(SLEEP_10MS);
                }

                aitt.Publish("test/value1", dump_msg, 12, protocol);
                if (single_level) {
                    aitt.Publish("test/value2", dump_msg, 1600, protocol);
                    aitt.Publish("test/value3", dump_msg, 1600, protocol);
                } else {
                    aitt.Publish("test/step1/value1", dump_msg, 1600, protocol);
                    aitt.Publish("test/step2/value1", dump_msg, 1600, protocol);
                }
            });
            aitt.Connect();

            mainLoop.AddTimeout(CHECK_INTERVAL,
                  [&](MainLoopHandler::Event result, int fd,
                        MainLoopHandler::MainLoopData *data) -> int {
                      return ReadyCheck(static_cast<AittTests *>(this));
                  });
            IterateEventLoop();

            ASSERT_TRUE(ready);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
    void TCP_SubscribeSameTopicTwiceTemplate(AittProtocol protocol)
    {
        try {
            AITT aitt(clientId, LOCAL_IP);
            aitt.Connect();

            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);
                      test->ToggleReady();
                  },
                  static_cast<void *>(this), protocol);
            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);
                      test->ToggleReady2();
                  },
                  static_cast<void *>(this), protocol);

            while (aitt.CountSubscriber(testTopic, protocol) != 2) {
                usleep(SLEEP_10MS);
            }

            aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), protocol);

            mainLoop.AddTimeout(CHECK_INTERVAL,
                  [&](MainLoopHandler::Event result, int fd,
                        MainLoopHandler::MainLoopData *data) -> int {
                      return ReadyAllCheck(static_cast<AittTests *>(this));
                  });
            IterateEventLoop();

            ASSERT_TRUE(ready);
            ASSERT_TRUE(ready2);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
};

TEST_F(AITTTCPTest, TCP_Wildcard_single_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP, true);
}

TEST_F(AITTTCPTest, SECURE_TCP_Wildcard_single_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP_SECURE, true);
}

TEST_F(AITTTCPTest, TCP_Wildcard_multi_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP, false);
}

TEST_F(AITTTCPTest, SECURE_TCP_Wildcard_multi_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP_SECURE, false);
}

TEST_F(AITTTCPTest, SECURE_TCP_various_msg_Anytime)
{
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> random_engine;
    std::vector<unsigned char> data(20000);
    std::generate(begin(data), end(data), std::ref(random_engine));

    unsigned char *dump_msg = data.data();

    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.SetConnectionCallback(
              [&, dump_msg](AITT &handle, int status, void *user_data) {
                  if (status != AITT_CONNECTED) {
                      ERR("Invalid Status(%d)", status);
                      return;
                  }
                  aitt.Subscribe(
                        testTopic,
                        [&](AittMsg *handle, const void *msg, const int szmsg,
                              void *cbdata) -> void {
                            AITTTCPTest *test = static_cast<AITTTCPTest *>(cbdata);

                            DBG("Subscribe() invoked : size(%d)", szmsg);

                            if (static_cast<int>(data.size()) <= szmsg) {
                                test->StopEventLoop();
                                return;
                            }

                            static int i = 1;
                            if (i == szmsg) {
                                EXPECT_EQ(memcmp(msg, data.data(), szmsg), 0);
                                i *= 3;
                            } else {
                                FAIL() << "Unexpected size" << szmsg;
                            }
                        },
                        user_data, AITT_TYPE_TCP_SECURE);

                  // Wait a few seconds until the AITT client gets a server list (discover devices)
                  while (aitt.CountSubscriber(testTopic, AITT_TYPE_TCP_SECURE) == 0) {
                      usleep(SLEEP_10MS);
                  }

                  for (int i = 1; i < static_cast<int>(data.size()); i *= 3) {
                      DBG("Publish(%s) : size(%d)", testTopic.c_str(), i);
                      aitt.Publish(testTopic, dump_msg, i, AITT_TYPE_TCP_SECURE);
                  }
                  aitt.Publish(testTopic, dump_msg, data.size(), AITT_TYPE_TCP_SECURE);
              },
              this);

        aitt.Connect();

        IterateEventLoop();
        aitt.Disconnect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTCPTest, TCP_Subscribe_Same_Topic_twice_Anytime)
{
    TCP_SubscribeSameTopicTwiceTemplate(AITT_TYPE_TCP);
}

TEST_F(AITTTCPTest, Secure_TCP_Subscribe_Same_Topic_twice_Anytime)
{
    TCP_SubscribeSameTopicTwiceTemplate(AITT_TYPE_TCP_SECURE);
}
