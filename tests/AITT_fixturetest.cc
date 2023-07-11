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

TEST_F(AITTTest, PublishSubscribe_P_Anytime)
{
    PubSubFull(TEST_MSG, AITT_TYPE_MQTT);
    PubSubFull(TEST_MSG, AITT_TYPE_TCP);
    PubSubFull(TEST_MSG, AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, Publish_0_P_Anytime)
{
    PubSubFull("", AITT_TYPE_MQTT);
    PubSubFull("", AITT_TYPE_TCP);
    PubSubFull("", AITT_TYPE_TCP_SECURE);
}

TEST_F(AITTTest, Unsubscribe_in_Subscribe_MQTT_P_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        AittSubscribeID subscribeHandle = aitt.Subscribe(
              testTopic,
              [&](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  std::string receivedMsg(static_cast<const char *>(msg), szmsg);
                  DBG("Subscribe invoked: %s %d", receivedMsg.c_str(), szmsg);

                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 2)
                      FAIL() << "Should not be called";

                  aitt.Unsubscribe(subscribeHandle);
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
        aitt.Subscribe(
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

TEST_F(AITTTest, CountSubscriber_With_Wildcard_N_Anytime)
{
    try {
        AITT aitt(clientId, LOCAL_IP, AittOption(true, false));

        aitt.Connect();
        aitt.Subscribe("topic1/1", TempSubCallback, nullptr);
        aitt.Subscribe("topic1/2", TempSubCallback, nullptr);

        EXPECT_EQ(aitt.CountSubscriber("topic1/+"), AITT_ERROR_NOT_SUPPORTED);
        EXPECT_EQ(aitt.CountSubscriber("topic1/#"), AITT_ERROR_NOT_SUPPORTED);
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
