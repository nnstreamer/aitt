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
#include "AITT.h"

#include <glib.h>
#include <gtest/gtest.h>
#include <sys/random.h>
#include <unistd.h>

#include "log.h"

#define TEST_MSG "This is my test message"
#define TEST_MSG2 "This message is going to be delivered through a specified AittProtocol"
#define MY_IP "127.0.0.1"
#define SLEEP_MS 1

class AITTTest : public testing::Test {
  public:
    void ToggleReady() { ready = true; }
    void ToggleReady2() { ready2 = true; }

  protected:
    void SetUp() override
    {
        ready = false;
        ready2 = false;
        mainLoop = g_main_loop_new(nullptr, FALSE);
        timeval tv;
        char buffer[256];
        gettimeofday(&tv, nullptr);
        snprintf(buffer, sizeof(buffer), "UniqueID.%lX%lX", tv.tv_sec, tv.tv_usec);
        clientId = buffer;
        snprintf(buffer, sizeof(buffer), "TestTopic.%lX%lX", tv.tv_sec, tv.tv_usec);
        testTopic = buffer;
    }

    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop);
        DBG("Go forward");
    }

    void TearDown() override { g_main_loop_unref(mainLoop); }

    static gboolean ReadyCheck(gpointer data)
    {
        AITTTest *test = static_cast<AITTTest *>(data);

        if (test->ready) {
            g_main_loop_quit(test->mainLoop);
            return FALSE;
        }

        return TRUE;
    }

  protected:
    bool ready;
    bool ready2;
    GMainLoop *mainLoop;
    std::string clientId;
    std::string testTopic;

  public:
    void *subscribeHandle;
};

using AITT = aitt::AITT;

TEST_F(AITTTest, Positive_Create_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Connect_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Disconnect_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Disconnect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Connect_twice_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Disconnect();
        aitt.Connect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Publish_MQTT_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Publish_TCP_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Publish_Multiple_Protocols_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG),
              (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Subscribe_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Unsubscribe_MQTT_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_MQTT);
        DBG(">>> Handle: %p", reinterpret_cast<void *>(subscribeHandle));
        aitt.Unsubscribe(subscribeHandle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Unsubscribe_TCP_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP);
        DBG("Subscribe handle: %p", reinterpret_cast<void *>(subscribeHandle));
        aitt.Unsubscribe(subscribeHandle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positve_PublishSubscribe_MQTT_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  test->ToggleReady();
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(msg), szmsg);
              },
              static_cast<void *>(this));

        DBG("Publish message to %s (%s)", testTopic.c_str(), TEST_MSG);
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG));

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positve_Unsubscribe_in_Subscribe_MQTT_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        subscribeHandle = aitt.Subscribe(
              testTopic,
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(msg), szmsg);

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

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positve_PublishSubscribe_TCP_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  test->ToggleReady();
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(msg), szmsg);
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        // Wait a few seconds until the AITT client gets a server list (discover devices)
        DBG("Sleep %d secs", SLEEP_MS);
        sleep(SLEEP_MS);

        DBG("Publish message to %s (%s) / %zu", testTopic.c_str(), TEST_MSG, sizeof(TEST_MSG));
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP);

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positve_PublishSubscribe_Multiple_Protocols_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(msg), szmsg);
                  test->ToggleReady();
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        aitt.Subscribe(
              testTopic,
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(msg), szmsg);
                  test->ToggleReady2();
              },
              static_cast<void *>(this), AITT_TYPE_MQTT);

        // Wait a few seconds to the AITT client gets server list (discover devices)
        DBG("Sleep %d secs", SLEEP_MS);
        sleep(SLEEP_MS);

        DBG("Publish message to %s (%s) / %zu", testTopic.c_str(), TEST_MSG, sizeof(TEST_MSG));
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG),
              (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP));

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
        ASSERT_TRUE(ready2);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positve_PublishSubscribe_twice_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  // NOTE:
                  // Subscribe callback will be invoked 2 times
                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 2)
                      test->ToggleReady();
                  DBG("Subscribe callback called: %d", cnt);
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        // Wait a few seconds to the AITT client gets server list (discover devices)
        sleep(SLEEP_MS);

        // NOTE:
        // Select target peers and send the data through the specified protocol - TCP
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP);

        // NOTE:
        // Publish message through the specified protocol - TCP
        aitt.Publish(testTopic, TEST_MSG2, sizeof(TEST_MSG2), AITT_TYPE_TCP);

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, Positive_Subscribe_Retained_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP);
        aitt.Connect();
        aitt.Subscribe(
              testTopic,
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  static int cnt = 0;
                  ++cnt;
                  if (cnt == 1)
                      test->ToggleReady();
                  DBG("Subscribe callback called: %d", cnt);
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        // Wait a few seconds to the AITT client gets server list (discover devices)
        sleep(SLEEP_MS);

        // NOTE:
        // Publish a message with the retained flag
        // This message will not be delivered, subscriber subscribes TCP protocol
        aitt.Publish(testTopic, TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_MQTT, AITT::QoS::AT_MOST_ONCE,
              true);

        aitt.Publish(testTopic, TEST_MSG2, sizeof(TEST_MSG2), AITT_TYPE_TCP);

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        aitt.Publish(testTopic, nullptr, 0, AITT_TYPE_MQTT, AITT::QoS::AT_LEAST_ONCE, true);

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AITTTest, TCP_Publish_Disconnect_Anytime)
{
    try {
        char dump_msg[204800];
        int dump_msg_size = getrandom(dump_msg, sizeof(dump_msg), 0);

        AITT aitt(clientId, MY_IP);
        AITT aitt_retry("retry_test", MY_IP);
        aitt.Connect();
        aitt_retry.Connect();

        aitt.Subscribe(
              "test/stress1",
              [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                  AITTTest *test = static_cast<AITTTest *>(cbdata);
                  static int cnt = 0;
                  ++cnt;
                  if (szmsg == 0) {
                      FAIL() << "Unexpected value";
                  }
                  if (cnt == 10)
                      test->ToggleReady();
                  if (cnt == 11)
                      test->ToggleReady();
              },
              static_cast<void *>(this), AITT_TYPE_TCP);

        {
            AITT aitt1("stress_test1", MY_IP);
            aitt1.Connect();

            // Wait a few seconds to the AITT client gets server list (discover devices)
            sleep(SLEEP_MS);

            for (int i = 0; i < 10; i++) {
                aitt1.Publish("test/stress1", dump_msg, dump_msg_size, AITT_TYPE_TCP,
                      AITT::QoS::AT_MOST_ONCE, true);
            }

            g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

            IterateEventLoop();
        }
        DBG("aitt1 client Finish");

        // Here, It's automatically checked Unexpected callback(szmsg = 0)
        // when publisher is disconnected.

        ASSERT_TRUE(ready);
        ready = false;

        aitt_retry.Publish("test/stress1", dump_msg, dump_msg_size, AITT_TYPE_TCP,
              AITT::QoS::AT_MOST_ONCE, true);

        g_timeout_add(10, AITTTest::ReadyCheck, static_cast<void *>(this));

        IterateEventLoop();

        ASSERT_TRUE(ready);

        aitt_retry.Publish("test/stress1", nullptr, 0, AITT_TYPE_TCP, AITT::QoS::AT_LEAST_ONCE);
        // Check auto release of aitt. It sould be no Segmentation fault
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
