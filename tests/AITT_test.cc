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
#include "AITT.h"

#include <gtest/gtest.h>

#include <random>

#include "AittTests.h"
#include "aitt_internal.h"

using AITT = aitt::AITT;

class AITTTest : public testing::Test, public AittTests {
  protected:
    void SetUp() override { Init(); }
    void TearDown() override { Deinit(); }

    void PubSub(AITT &aitt, const char *test_msg, AittProtocol protocol, AITTTest *aitt_test)
    {
        aitt.Subscribe(
              testTopic,
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  if (msg)
                      DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);
                  else
                      DBG("Subscribe invoked: zero size msg(%d)", szmsg);
                  test->StopEventLoop();
              },
              aitt_test, protocol);

        // Wait a few seconds until the AITT client gets a server list (discover devices)
        while (aitt.CountSubscriber(testTopic, protocol) == 0) {
            usleep(SLEEP_10MS);
        }

        DBG("Publish(%s) : %s(%zu)", testTopic.c_str(), test_msg, strlen(test_msg));
        aitt.Publish(testTopic, test_msg, strlen(test_msg), protocol);
    }

    void PubSubFull(const char *test_msg, AittProtocol protocol)
    {
        try {
            AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
            aitt.SetConnectionCallback(
                  [&, test_msg, protocol](AITT &handle, int status, void *user_data) {
                      if (status == AITT_CONNECTED)
                          PubSub(aitt, test_msg, protocol, this);
                  },
                  this);

            aitt.Connect();

            IterateEventLoop();
            aitt.Disconnect();
        } catch (std::exception &e) {
            FAIL() << "Unexpected exception: " << e.what();
        }
    }

    void PublishDisconnectTemplate(AittProtocol protocol)
    {
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
                      AITTTest *test = static_cast<AITTTest *>(cbdata);
                      ++cnt;
                      if (szmsg == 0 && cnt != 12) {
                          FAIL() << "Unexpected value" << cnt;
                      }

                      if (msg) {
                          ASSERT_TRUE(!strncmp(static_cast<const char *>(msg), dump_msg,
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

    void PublishSubscribeTCPTwiceTemplate(AittProtocol protocol)
    {
        try {
            AITT aitt(clientId, LOCAL_IP);
            aitt.Connect();

            int cnt = 0;
            aitt.Subscribe(
                  testTopic,
                  [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                      AITTTest *test = static_cast<AITTTest *>(cbdata);
                      // NOTE:
                      // Subscribe callback will be invoked 2 times
                      ++cnt;
                      std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                      DBG("Subscribe callback called: %d, szmsg = %d, msg = [%s]", cnt, szmsg,
                            receivedMsg.c_str());
                      if (cnt == 1) {
                          ASSERT_TRUE(!strcmp(receivedMsg.c_str(), TEST_MSG));
                      } else if (cnt == 2) {
                          ASSERT_TRUE(!strcmp(receivedMsg.c_str(), TEST_MSG2));
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

    void SubscribeRetainedTCPTemplate(AittProtocol protocol)
    {
        try {
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
                          AITTTest *test = static_cast<AITTTest *>(cbdata);
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

    bool WaitDiscovery(AITT &aitt, std::string topic, int count, int max_ms = 2000)
    {
        int i = 0;
        int max_loop = max_ms / 10;
        int result = false;

        for (i = 0; i < max_loop; ++i) {
            if (aitt.CountSubscriber(topic) == count) {
                result = true;
                break;
            }

            usleep(SLEEP_10MS);
        }

        DBG("WAIT %d ms", i * SLEEP_10MS);
        return result;
    }

    static void TempSubCallback(AittMsg *msg, const void *data, const int data_len, void *user_data)
    {
        DBG("TEMP CALLBACK");
    }
};

TEST_F(AITTTest, Create_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, SetConnectionCallback_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.SetConnectionCallback(
              [&](AITT &handle, int status, void *user_data) {
                  AITTTest *test = static_cast<AITTTest *>(user_data);

                  if (test->ready2) {
                      EXPECT_EQ(status, AITT_DISCONNECTED);
                      test->ToggleReady();
                  } else {
                      EXPECT_EQ(status, AITT_CONNECTED);
                      test->ToggleReady2();
                      handle.Disconnect();
                  }
              },
              this);
        aitt.Connect();

        mainLoop->AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();
        ASSERT_TRUE(ready);
        ASSERT_TRUE(ready2);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, PubSubInConnectionCB_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.SetConnectionCallback(
              [&](AITT &handle, int status, void *user_data) {
                  if (status == AITT_CONNECTED)
                      PubSub(aitt, TEST_MSG, AITT_TYPE_MQTT, this);
              },
              this);
        aitt.Connect();

        IterateEventLoop();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, UnsetConnectionCallback_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.SetConnectionCallback(
              [&](AITT &handle, int status, void *user_data) {
                  AITTTest *test = static_cast<AITTTest *>(user_data);

                  if (test->ready) {
                      FAIL() << "Should not be called";
                  } else {
                      EXPECT_EQ(status, AITT_CONNECTED);
                      test->ToggleReady();
                      handle.SetConnectionCallback(nullptr, nullptr);
                      handle.Disconnect();
                  }
              },
              this);
        aitt.Connect();

        mainLoop->AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();
        ASSERT_FALSE(ready2);
        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Connect_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Disconnect_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Disconnect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Connect_twice_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Disconnect();
        aitt.Connect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Ready_P_Anytime)
{
    try {
        AITT aitt(AITT_MUST_CALL_READY);
        aitt.Ready(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();

        AITT aitt1("Must Call Ready() First");
        aitt1.Ready("", LOCAL_IP, AittOption(true, false));
        aitt1.Connect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Ready_N_Anytime)
{
    EXPECT_THROW({ AITT aitt("must call ready() first"); }, aitt::AittException);
    EXPECT_THROW({ AITT aitt("not ready"); }, aitt::AittException);
    EXPECT_THROW({ AITT aitt("unknown notice"); }, aitt::AittException);

    EXPECT_THROW(
          {
              AITT aitt(AITT_MUST_CALL_READY);
              aitt.Ready(clientId, LOCAL_IP);
              aitt.Ready(clientId, LOCAL_IP, AittOption(true, false));
          },
          aitt::AittException);

    EXPECT_THROW(
          {
              AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
              aitt.Ready(clientId, LOCAL_IP);
          },
          aitt::AittException);
}

TEST_F(AITTTest, Not_READY_STATUS_N_Anytime)
{
    EXPECT_DEATH(
          {
              AITT aitt(AITT_MUST_CALL_READY);
              // If it is called whithout aitt.Ready(), it crashes.
              aitt.Connect();
              FAIL() << "MUST NOT use the aitt before calling Ready()";
          },
          "");
}

TEST_F(AITTTest, Publish_MQTT_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Publish_TCP_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Publish_SECURE_TCP_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP_SECURE);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Publish_minus_size_N_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        EXPECT_THROW(aitt.Publish(testTopic, TEST_MSG, -1, AITT_TYPE_TCP), aitt::AittException);
        EXPECT_THROW(aitt.Publish(testTopic, TEST_MSG, -1, AITT_TYPE_MQTT), aitt::AittException);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Publish_Multiple_Protocols_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG),
              (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Unsubscribe_MQTT_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_MQTT);
        DBG(">>> Handle: %p", reinterpret_cast<void *>(subscribeHandle));
        aitt.Unsubscribe(subscribeHandle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Unsubscribe_TCP_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP);
        DBG("Subscribe handle: %p", reinterpret_cast<void *>(subscribeHandle));
        aitt.Unsubscribe(subscribeHandle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Unsubscribe_SECURE_TCP_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP_SECURE);
        DBG("Subscribe handle: %p", reinterpret_cast<void *>(subscribeHandle));
        aitt.Unsubscribe(subscribeHandle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, PublishSubscribe_MQTT_P_Anytime)
{
    PubSubFull(TEST_MSG, AITT_TYPE_MQTT);
}

TEST_F(AITTTest, Publish_0_MQTT_P_Anytime)
{
    PubSubFull("", AITT_TYPE_MQTT);
}

TEST_F(AITTTest, Unsubscribe_in_Subscribe_MQTT_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);

                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 2)
                      FAIL() << "Should not be called";

                  aitt.Unsubscribe(test->subscribeHandle);
                  DBG("Ready flag is toggled");
                  test->ToggleReady();
              },
              static_cast<void *>(this));

        DBG("Publish message to %s (%s)", testTopic.c_str(), TEST_MSG);
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG));

        mainLoop->AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Subscribe_in_Subscribe_MQTT_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);

                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 2)
                      FAIL() << "Should not be called";

                  aitt.Subscribe(
                        "topic1InCallback",
                        [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) {},
                        cbdata);

                  aitt.Subscribe(
                        "topic2InCallback",
                        [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) {},
                        cbdata);
                  DBG("Ready flag is toggled");

                  mainLoop->AddTimeout(
                        CHECK_INTERVAL,
                        [&](MainLoopIface::Event result, int fd,
                              MainLoopIface::MainLoopData *data) -> int {
                            AITTTest *test = static_cast<AITTTest *>(this);
                            test->ToggleReady();
                            return AITT_LOOP_EVENT_REMOVE;
                        },
                        nullptr);
              },
              static_cast<void *>(this));

        DBG("Publish message to %s (%s)", testTopic.c_str(), TEST_MSG);
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG));

        mainLoop->AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, PublishSubscribe_TCP_P_Anytime)
{
    PubSubFull(TEST_MSG, AITT_TYPE_TCP);
}

TEST_F(AITTTest, PublishSubscribe_SECURE_TCP_P_Anytime)
{
    PubSubFull(TEST_MSG, AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, Publish_0_TCP_P_Anytime)
{
    PubSubFull("", AITT_TYPE_TCP);
}

TEST_F(AITTTest, Publish_0_SECURE_TCP_P_Anytime)
{
    PubSubFull("", AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, PublishSubscribe_Multiple_Protocols_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);
                  test->ToggleReady();
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        aitt.Subscribe(
              testTopic,
              [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);
                  test->ToggleReady2();
              },
              static_cast<void *>(this), AITT_TYPE_MQTT);

        // Wait a few seconds to the AITT client gets server list (discover devices)
        while (
              aitt.CountSubscriber(testTopic, (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP)) < 2) {
            usleep(SLEEP_10MS);
        }

        DBG("Publish message to %s (%s) / %zu", testTopic.c_str(), TEST_MSG, sizeof(TEST_MSG));
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG),
              (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP));

        mainLoop->AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();

        ASSERT_TRUE(ready);
        ASSERT_TRUE(ready2);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, CountSubscriber_P_Anytime)
{
    try {
        AITT aitt_client1(clientId + std::string(".1"), LOCAL_IP, AittOption(true, false));
        AITT aitt_client2(clientId + std::string(".2"), LOCAL_IP, AittOption(true, false));
        AITT aitt_server(clientId + std::string(".server"), LOCAL_IP, AittOption(true, false));

        aitt_client1.Connect();
        aitt_client2.Connect();
        aitt_server.Connect();

        aitt_client1.Subscribe("topic1", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client1.Subscribe("topic1", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client1.Subscribe("topic2", TempSubCallback, nullptr, AITT_TYPE_TCP);
        aitt_client1.Subscribe("topic1", TempSubCallback, nullptr, AITT_TYPE_TCP_SECURE);
        aitt_client1.Subscribe("topic1", TempSubCallback, nullptr, AITT_TYPE_TCP_SECURE);

        aitt_client2.Subscribe("topic2", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client1.Subscribe("topic2", TempSubCallback, nullptr, AITT_TYPE_TCP_SECURE);
        aitt_client2.Subscribe("topic2", TempSubCallback, nullptr, AITT_TYPE_TCP);
        aitt_client2.Subscribe("topic2", TempSubCallback, nullptr, AITT_TYPE_TCP);

        ASSERT_TRUE(WaitDiscovery(aitt_server, std::string("topic1"), 4));
        ASSERT_TRUE(WaitDiscovery(aitt_server, std::string("topic2"), 5));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, CountSubscriber_Wildcard_Singlelevel_P_Anytime)
{
    try {
        AITT aitt_client1(clientId + std::string(".1"), LOCAL_IP, AittOption(true, false));
        AITT aitt_client2(clientId + std::string(".2"), LOCAL_IP, AittOption(true, false));
        AITT aitt_server(clientId + std::string(".server"), LOCAL_IP, AittOption(true, false));

        aitt_client1.Connect();
        aitt_client2.Connect();
        aitt_server.Connect();

        aitt_client1.Subscribe("topic/+/single/level", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client1.Subscribe("topic/client/+/level", TempSubCallback, nullptr, AITT_TYPE_TCP);
        aitt_client2.Subscribe("topic/client/single/+", TempSubCallback, nullptr,
              AITT_TYPE_TCP_SECURE);

        aitt_client2.Subscribe("topic2/client/+/+", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client2.Subscribe("topic2/+/single/+", TempSubCallback, nullptr, AITT_TYPE_TCP);
        aitt_client1.Subscribe("topic2/+/+/level", TempSubCallback, nullptr, AITT_TYPE_TCP_SECURE);

        aitt_client1.Subscribe("topic3/+/+/+", TempSubCallback, nullptr, AITT_TYPE_MQTT);

        ASSERT_TRUE(WaitDiscovery(aitt_server, "topic/client/single/level", 3));
        ASSERT_TRUE(WaitDiscovery(aitt_server, "topic2/client/single/level", 3));
        ASSERT_TRUE(WaitDiscovery(aitt_server, "topic3/client/single/level", 1));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, CountSubscriber_Wildcard_Multilevel_P_Anytime)
{
    try {
        AITT aitt_client1(clientId + std::string(".1"), LOCAL_IP, AittOption(true, false));
        AITT aitt_client2(clientId + std::string(".2"), LOCAL_IP, AittOption(true, false));
        AITT aitt_server(clientId + std::string(".server"), LOCAL_IP, AittOption(true, false));

        aitt_client1.Connect();
        aitt_client2.Connect();
        aitt_server.Connect();

        aitt_client2.Subscribe("#", TempSubCallback, nullptr, AITT_TYPE_MQTT);
        aitt_client1.Subscribe("topic/#", TempSubCallback, nullptr, AITT_TYPE_TCP);
        aitt_client1.Subscribe("topic/client/#", TempSubCallback, nullptr, AITT_TYPE_TCP_SECURE);
        aitt_client2.Subscribe("topic/client/multi/#", TempSubCallback, nullptr,
              AITT_TYPE_TCP_SECURE);

        ASSERT_TRUE(WaitDiscovery(aitt_server, "topic/client/multi/level", 4));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, PublishSubscribe_TCP_twice_P_Anytime)
{
    PublishSubscribeTCPTwiceTemplate(AITT_TYPE_TCP);
}

TEST_F(AITTTest, PublishSubscribe_SECURE_TCP_twice_P_Anytime)
{
    PublishSubscribeTCPTwiceTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, Subscribe_Retained_TCP_P_Anytime)
{
    SubscribeRetainedTCPTemplate(AITT_TYPE_TCP);
}

TEST_F(AITTTest, Subscribe_Retained_SECURE_TCP_P_Anytime)
{
    SubscribeRetainedTCPTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, Publish_Disconnect_TCP_P_Anytime)
{
    PublishDisconnectTemplate(AITT_TYPE_TCP);
}

TEST_F(AITTTest, Publish_Disconnect_SECURE_TCP_P_Anytime)
{
    PublishDisconnectTemplate(AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, WillSet_N_Anytime)
{
    EXPECT_THROW(
          {
              AITT aitt_will("", LOCAL_IP, AittOption(true, false));
              aitt_will.SetWillInfo("+", "will msg", 8, AITT_QOS_AT_MOST_ONCE, false);
              aitt_will.Connect();
              aitt_will.Disconnect();
          },
          std::exception);
}
