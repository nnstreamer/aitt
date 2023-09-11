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

class AittTcpTest : public testing::Test, public AittTests {
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
            ready = false;
            aitt.SetConnectionCallback([&](AITT &handle, int status, void *user_data) {
                if (status != AITT_CONNECTED)
                    return;
                aitt.Subscribe(
                      sub_topic,
                      [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                          AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
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

            auto timeout = mainLoop->AddTimeout(
                  CHECK_INTERVAL,
                  [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                        -> int { return ReadyCheck(static_cast<AittTests *>(this)); },
                  nullptr);
            IterateEventLoop();
            mainLoop->RemoveTimeout(timeout);

            ASSERT_TRUE(ready);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
    void TCP_SubscribeSameTopicTwiceTemplate(AittProtocol protocol)
    {
        try {
            ready = false;
            ready2 = false;

            AITT aitt(clientId, LOCAL_IP);
            aitt.Connect();

            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
                      test->ToggleReady();
                  },
                  static_cast<void *>(this), protocol);
            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
                      test->ToggleReady2();
                  },
                  static_cast<void *>(this), protocol);

            while (aitt.CountSubscriber(testTopic, protocol) != 2) {
                usleep(SLEEP_10MS);
            }

            aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), protocol);

            auto timeout = mainLoop->AddTimeout(
                  CHECK_INTERVAL,
                  [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                        -> int { return ReadyAllCheck(static_cast<AittTests *>(this)); },
                  nullptr);
            IterateEventLoop();
            mainLoop->RemoveTimeout(timeout);

            ASSERT_TRUE(ready);
            ASSERT_TRUE(ready2);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
    void PublishSubscribeTCPTwiceTemplate(AittProtocol protocol)
    {
        try {
            AITT aitt(clientId, LOCAL_IP);
            aitt.Connect();

            int cnt = 0;
            ready = false;
            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
                      ++cnt;
                      std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                      DBG("Subscribe callback called: %d, szmsg = %d, msg = [%s]", cnt, szmsg,
                            receivedMsg.c_str());
                      if (cnt == 1) {
                          ASSERT_EQ(STR_EQ, strcmp(receivedMsg.c_str(), TEST_MSG));
                      } else if (cnt == 2) {
                          ASSERT_EQ(STR_EQ, strcmp(receivedMsg.c_str(), TEST_MSG2));
                          test->ToggleReady();
                      }
                  },
                  static_cast<void *>(this), protocol);

            // Wait a few seconds to the AITT client gets server list (discover devices)
            while (aitt.CountSubscriber(testTopic, protocol) == 0) {
                usleep(SLEEP_10MS);
            }

            // NOTE:
            // Select target peers and send the data through the specified protocol - TCP
            aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), protocol);

            // NOTE:
            // Publish message through the specified protocol - TCP
            aitt.Publish(testTopic, TEST_MSG2, sizeof(TEST_MSG2), protocol);

            mainLoop->AddTimeout(
                  CHECK_INTERVAL,
                  [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                        -> int { return ReadyCheck(static_cast<AittTests *>(this)); },
                  nullptr);

            IterateEventLoop();

            ASSERT_TRUE(ready);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }

    void PublishDisconnectTemplate(AittProtocol protocol)
    {
        ready = false;
        const char character_set[] =
              "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
        std::mt19937 random_gen{std::random_device{}()};
        std::uniform_int_distribution<std::string::size_type> gen(0, 61);

        char dump_msg[204800] = {};
        for (size_t i = 0; i < sizeof(dump_msg); i++) {
            dump_msg[i] = character_set[gen(random_gen)];
        }

        try {
            AITT aitt(clientId, LOCAL_IP);
            AITT aitt_retry("retry_test", LOCAL_IP);
            aitt.Connect();
            aitt_retry.Connect();

            int cnt = 0;
            aitt.Subscribe(
                  TEST_STRESS_TOPIC,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
                      ++cnt;
                      if (szmsg == 0 && cnt != 12) {
                          FAIL() << "Unexpected value" << cnt;
                      }

                      if (msg) {
                          ASSERT_EQ(STR_EQ, strncmp(static_cast<const char *>(msg), dump_msg,
                                                  sizeof(dump_msg)));
                      }

                      if (cnt == 10)
                          test->ToggleReady();
                      if (cnt == 11)
                          test->ToggleReady();
                  },
                  static_cast<void *>(this), protocol);

            {
                AITT aitt1("stress_test1", LOCAL_IP);
                aitt1.Connect();

                // Wait a few seconds to the AITT client gets server list (discover devices)
                while (aitt1.CountSubscriber(TEST_STRESS_TOPIC, protocol) == 0) {
                    usleep(SLEEP_10MS);
                }

                for (int i = 0; i < 10; i++) {
                    INFO("size = %zu", sizeof(dump_msg));
                    aitt1.Publish(TEST_STRESS_TOPIC, dump_msg, sizeof(dump_msg), protocol,
                          AITT_QOS_AT_MOST_ONCE);
                }
                mainLoop->AddTimeout(
                      CHECK_INTERVAL,
                      [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                            -> int { return ReadyCheck(static_cast<AittTests *>(this)); },
                      nullptr);

                IterateEventLoop();
            }
            DBG("Client aitt1 is finished");

            // Here, an unexpected callback(szmsg = 0) is received
            // when the publisher is disconnected.

            ASSERT_TRUE(ready);
            ready = false;

            aitt_retry.Publish(TEST_STRESS_TOPIC, dump_msg, sizeof(dump_msg), protocol,
                  AITT_QOS_AT_MOST_ONCE);

            mainLoop->AddTimeout(
                  CHECK_INTERVAL,
                  [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                        -> int { return ReadyCheck(static_cast<AittTests *>(this)); },
                  nullptr);

            IterateEventLoop();

            ASSERT_TRUE(ready);

            aitt_retry.Publish(TEST_STRESS_TOPIC, nullptr, 0, protocol, AITT_QOS_AT_LEAST_ONCE);
            // Check auto release of aitt. There should be no segmentation faults.
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }

    void SubscribeRetainedTCPTemplate(AittProtocol protocol)
    {
        try {
            ready = false;
            AITT aitt(clientId, LOCAL_IP);

            aitt.SetConnectionCallback([&](AITT &handle, int status, void *user_data) {
                if (status != AITT_CONNECTED)
                    return;

                // Subscribers who subscribe after publishing will not receive this message
                aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_MQTT,
                      AITT_QOS_AT_MOST_ONCE, true);

                aitt.Subscribe(
                      testTopic,
                      [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                          AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);
                          std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                          EXPECT_STREQ(receivedMsg.c_str(), TEST_MSG2);
                          test->ToggleReady();
                      },
                      static_cast<void *>(this), protocol);

                // Wait a few seconds to the AITT client gets server list (discover devices)
                while (aitt.CountSubscriber(testTopic, protocol) == 0) {
                    usleep(SLEEP_10MS);
                }

                aitt.Publish(testTopic, TEST_MSG2, sizeof(TEST_MSG2), protocol);
            });
            aitt.Connect();

            mainLoop->AddTimeout(
                  CHECK_INTERVAL,
                  [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data)
                        -> int { return ReadyCheck(static_cast<AittTests *>(this)); },
                  nullptr);

            IterateEventLoop();
            ASSERT_TRUE(ready);
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }
};

TEST_F(AittTcpTest, TCP_Wildcard_single_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP, true);
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP_SECURE, true);
}

TEST_F(AittTcpTest, TCP_Wildcard_multi_Anytime)
{
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP, false);
    TCPWildcardsTopicTemplate(AITT_TYPE_TCP_SECURE, false);
}

TEST_F(AittTcpTest, Subscribe_Same_Topic_twice_Anytime)
{
    TCP_SubscribeSameTopicTwiceTemplate(AITT_TYPE_TCP);
    TCP_SubscribeSameTopicTwiceTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AittTcpTest, PublishSubscribe_twice_P_Anytime)
{
    PublishSubscribeTCPTwiceTemplate(AITT_TYPE_TCP);
    PublishSubscribeTCPTwiceTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AittTcpTest, Subscribe_Retained_P_Anytime)
{
    SubscribeRetainedTCPTemplate(AITT_TYPE_TCP);
    SubscribeRetainedTCPTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AittTcpTest, Publish_Disconnect_P_Anytime)
{
    PublishDisconnectTemplate(AITT_TYPE_TCP);
    PublishDisconnectTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AittTcpTest, SECURE_TCP_various_msg_Anytime)
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
                            AittTcpTest *test = static_cast<AittTcpTest *>(cbdata);

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
