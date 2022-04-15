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
#include <glib.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>

#include "AITT.h"
#include "log.h"

#define MY_IP "127.0.0.1"
using AITT = aitt::AITT;

class AITTRRTest : public testing::Test {
  public:
    void PublishSyncInCallback(aitt::AITT *aitt, bool *reply1_ok, bool *reply2_ok, aitt::MSG *msg,
          const void *data, const size_t datalen, void *cbdata)
    {
        aitt->PublishWithReplySync(
              rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT,
              aitt::AITT::AT_MOST_ONCE, false,
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckReply(msg, data, datalen);
                  *reply1_ok = true;
              },
              nullptr, correlation);

        CheckReply(msg, data, datalen);
        *reply2_ok = true;

        ToggleReady();
    }

    void CheckReplyCallback(bool toggle, bool *reply_ok, aitt::MSG *msg, const void *data,
          const size_t datalen, void *cbdata)
    {
        CheckReply(msg, data, datalen);
        *reply_ok = true;
        if (toggle)
            ToggleReady();
    }

  protected:
    void SetUp() override
    {
        ready = false;
        mainLoop = g_main_loop_new(nullptr, FALSE);
        timeval tv;
        char buffer[256];
        gettimeofday(&tv, nullptr);
        snprintf(buffer, sizeof(buffer), "UniqueID.%lX%lX", tv.tv_sec, tv.tv_usec);
        clientId = buffer;
    }

    void TearDown() override { g_main_loop_unref(mainLoop); }

    void ToggleReady() { ready = true; }

    void IterateEventLoop(void) { g_main_loop_run(mainLoop); }
    void CheckReply(aitt::MSG *msg, const void *data, const size_t datalen)
    {
        std::string received_data((const char *)data, datalen);
        EXPECT_EQ(msg->GetCorrelation(), correlation);
        EXPECT_EQ(received_data, reply);
        EXPECT_EQ(msg->IsEndSequence(), true);
    }

    void CheckSubscribe(aitt::MSG *msg, const void *data, const size_t datalen)
    {
        std::string received_data((const char *)data, datalen);
        EXPECT_TRUE(msg->GetTopic() == rr_topic);
        EXPECT_TRUE(msg->GetCorrelation() == correlation);
        EXPECT_FALSE(msg->GetResponseTopic().empty());
        EXPECT_EQ(received_data, message);
    }

    void Call2Times(bool first_sync, bool second_sync)
    {
        bool sub_ok;
        bool reply_ok[2];
        sub_ok = reply_ok[0] = reply_ok[1] = false;

        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckSubscribe(msg, data, datalen);
                  aitt.SendReply(msg, reply.c_str(), reply.size());
                  sub_ok = true;
              });

        using namespace std::placeholders;
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && first_sync) || (i == 1 && second_sync)) {
                INFO("PublishWithReplySync() Call");
                aitt.PublishWithReplySync(rr_topic.c_str(), message.c_str(), message.size(),
                      AITT_TYPE_MQTT, AITT::AT_MOST_ONCE, false,
                      std::bind(&AITTRRTest::CheckReplyCallback, this, (i == 1), &reply_ok[i],
                            _1, _2, _3, _4),
                      nullptr, correlation);
            } else {
                INFO("PublishWithReply() Call");
                aitt.PublishWithReply(rr_topic.c_str(), message.c_str(), message.size(),
                      AITT_TYPE_MQTT, AITT::AT_MOST_ONCE, false,
                      std::bind(&AITTRRTest::CheckReplyCallback, this, (i == 1), &reply_ok[i],
                            _1, _2, _3, _4),
                      nullptr, correlation);
            }
        }

        g_timeout_add(10, AITTRRTest::ReadyCheck, static_cast<void *>(this));
        IterateEventLoop();

        EXPECT_TRUE(sub_ok);
        EXPECT_TRUE(reply_ok[0]);
        EXPECT_TRUE(reply_ok[1]);
    }

    void SyncCallInCallback(bool sync)
    {
        bool sub_ok, reply1_ok, reply2_ok;
        sub_ok = reply1_ok = reply2_ok = false;

        AITT sub_aitt(clientId + "sub", MY_IP, true);
        INFO("Constructor Success");

        sub_aitt.Connect();
        INFO("Connected");

        sub_aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckSubscribe(msg, data, datalen);
                  sub_aitt.SendReply(msg, reply.c_str(), reply.size());
                  sub_ok = true;
              });

        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        using namespace std::placeholders;
        auto replyCB = std::bind(&AITTRRTest::PublishSyncInCallback, GetHandle(), &aitt, &reply1_ok,
              &reply2_ok, _1, _2, _3, _4);

        if (sync) {
            aitt.PublishWithReplySync(rr_topic.c_str(), message.c_str(), message.size(),
                  AITT_TYPE_MQTT, AITT::AT_MOST_ONCE, false, replyCB, nullptr, correlation);
        } else {
            aitt.PublishWithReply(rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT,
                  AITT::AT_MOST_ONCE, false, replyCB, nullptr, correlation);
        }

        g_timeout_add(10, AITTRRTest::ReadyCheck, static_cast<void *>(this));
        IterateEventLoop();

        EXPECT_TRUE(sub_ok);
        EXPECT_TRUE(reply1_ok);
        EXPECT_TRUE(reply2_ok);
    }

    static gboolean ReadyCheck(gpointer data)
    {
        AITTRRTest *test = static_cast<AITTRRTest *>(data);

        if (test->ready) {
            g_main_loop_quit(test->mainLoop);
            return FALSE;
        }

        return TRUE;
    }

    AITTRRTest *GetHandle() { return this; }

    bool ready;
    GMainLoop *mainLoop;
    std::string clientId;
    const std::string rr_topic = "test/rr_topic";
    const std::string message = "Hello world";
    const std::string correlation = "0001";
    const std::string reply = "Nice to meet you, RequestResponse";
};

TEST_F(AITTRRTest, RequestResponse_P_Anytime)
{
    bool sub_ok, reply_ok;
    sub_ok = reply_ok = false;

    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckSubscribe(msg, data, datalen);
                  aitt.SendReply(msg, reply.c_str(), reply.size());
                  sub_ok = true;
              });

        aitt.PublishWithReply(rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT,
              AITT::AT_MOST_ONCE, false,
              std::bind(&AITTRRTest::CheckReplyCallback, GetHandle(), true, &reply_ok,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                    std::placeholders::_4),
              nullptr, correlation);

        g_timeout_add(10, AITTRRTest::ReadyCheck, static_cast<void *>(this));
        IterateEventLoop();

        EXPECT_TRUE(sub_ok);
        EXPECT_TRUE(reply_ok);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_asymmetry_Anytime)
{
    std::string reply1 = "1st data";
    std::string reply2 = "2nd data";
    std::string reply3 = "final data";

    bool sub_ok, reply_ok;
    sub_ok = reply_ok = false;

    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckSubscribe(msg, data, datalen);

                  aitt.SendReply(msg, reply1.c_str(), reply1.size(), false);
                  aitt.SendReply(msg, reply2.c_str(), reply2.size(), false);
                  aitt.SendReply(msg, reply3.c_str(), reply3.size(), true);

                  sub_ok = true;
              });

        aitt.PublishWithReply(
              rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT, AITT::AT_MOST_ONCE,
              false,
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  std::string reply((const char *)data, datalen);

                  EXPECT_EQ(msg->GetCorrelation(), correlation);
                  switch (msg->GetSequence()) {
                  case 1:
                      EXPECT_EQ(reply, reply1);
                      break;
                  case 2:
                      EXPECT_EQ(reply, reply2);
                      break;
                  case 3:
                      EXPECT_EQ(reply, reply3);
                      EXPECT_TRUE(msg->IsEndSequence());
                      reply_ok = true;
                      ToggleReady();
                      break;
                  default:
                      FAIL() << "Unknown sequence" << msg->GetSequence();
                  }
              },
              nullptr, correlation);

        g_timeout_add(10, AITTRRTest::ReadyCheck, static_cast<void *>(this));
        IterateEventLoop();

        EXPECT_TRUE(sub_ok);
        EXPECT_TRUE(reply_ok);

    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_2times_Anytime)
{
    try {
        bool first_sync = false;
        bool second_sync = false;
        Call2Times(first_sync, second_sync);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_sync_P_Anytime)
{
    bool sub_ok, reply1_ok;
    sub_ok = reply1_ok = false;

    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  CheckSubscribe(msg, data, datalen);
                  aitt.SendReply(msg, reply.c_str(), reply.size());
                  sub_ok = true;
              });

        aitt.PublishWithReplySync(rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT,
              AITT::AT_MOST_ONCE, false,
              std::bind(&AITTRRTest::CheckReplyCallback, GetHandle(), false, &reply1_ok,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                    std::placeholders::_4),
              nullptr, correlation);

        EXPECT_TRUE(sub_ok);
        EXPECT_TRUE(reply1_ok);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_sync_async_P_Anytime)
{
    try {
        bool first_sync = true;
        bool second_sync = false;
        Call2Times(first_sync, second_sync);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_sync_in_async_P_Anytime)
{
    try {
        bool sync_callback = false;
        SyncCallInCallback(sync_callback);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_sync_in_sync_P_Anytime)
{
    try {
        bool sync_callback = true;
        SyncCallInCallback(sync_callback);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_timeout_P_Anytime)
{
    try {
        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        int ret = aitt.PublishWithReplySync(
              rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT, AITT::AT_MOST_ONCE,
              false,
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  FAIL() << "Should not be called";
              },
              nullptr, correlation, 1);

        EXPECT_EQ(ret, AITT_ERROR_TIMED_OUT);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}

TEST_F(AITTRRTest, RequestResponse_timeout_restart_P_Anytime)
{
    bool sub_ok, reply_ok;
    sub_ok = reply_ok = false;

    try {
        AITT sub_aitt(clientId + "sub", MY_IP, true);
        sub_aitt.Connect();
        sub_aitt.Subscribe(rr_topic.c_str(),
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  INFO("Subscribe Callback is called");
                  CheckSubscribe(msg, data, datalen);
                  sub_aitt.SendReply(msg, reply.c_str(), reply.size(), false);
                  sub_ok = true;
              });

        AITT aitt(clientId, MY_IP, true);
        aitt.Connect();

        int ret = aitt.PublishWithReplySync(
              rr_topic.c_str(), message.c_str(), message.size(), AITT_TYPE_MQTT, AITT::AT_MOST_ONCE,
              false,
              [&](aitt::MSG *msg, const void *data, const size_t datalen, void *cbdata) {
                  INFO("Reply Callback is called");
                  static int invalid = 0;
                  if (invalid)
                      FAIL() << "Should not be called";
                  else
                      reply_ok = true;
                  invalid++;
              },
              nullptr, correlation, 500);

        EXPECT_TRUE(sub_ok == reply_ok);
        EXPECT_EQ(ret, AITT_ERROR_TIMED_OUT);
    } catch (std::exception &e) {
        FAIL() << e.what();
    }
}
