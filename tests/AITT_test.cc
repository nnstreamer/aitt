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

TEST(AITT_Test, Create_P_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, Create_Custom_Mqtt_N_Anytime)
{
    EXPECT_THROW({ AITT aitt("clientId", LOCAL_IP, AittOption(true, true)); }, std::exception);
}

TEST(AITT_Test, Connect_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
    });
}

TEST(AITT_Test, Connect_Invalid_User_N_Anytime)
{
    char malformed_user[2] = {(char)-1, (char)-2};
    std::string user(malformed_user, 2);

    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP);
              aitt.Connect(AITT_LOCALHOST, AITT_PORT, user, "passwd");
              FAIL() << "Should not be called";
          },
          aitt::AittException);
}

TEST(AITT_Test, Connect_Invalid_Port_N_Anytime)
{
    EXPECT_THROW(
          {
              int invalid_port = -1;
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Connect(AITT_LOCALHOST, invalid_port);
          },
          aitt::AittException);
}

TEST(AITT_Test, Disconnect_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Disconnect();
    });
}

TEST(AITT_Test, Disconnect_N_Anytime)
{
    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Disconnect();
          },
          std::exception);
}

TEST(AITT_Test, Connect_twice_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Disconnect();
        aitt.Connect();
    });
}

TEST(AITT_Test, Connect_twice_N_)
{
    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Connect();
              aitt.Connect();
          },
          std::exception);
}

TEST(AITT_Test, Ready_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt(AITT_MUST_CALL_READY);
        aitt.Ready("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();

        AITT aitt1("Must Call Ready() First");
        aitt1.Ready("", LOCAL_IP, AittOption(true, false));
        aitt1.Connect();
    });
}

TEST(AITT_Test, Ready_N_Anytime)
{
    EXPECT_THROW({ AITT aitt("must call ready() first"); }, aitt::AittException);
    EXPECT_THROW({ AITT aitt("not ready"); }, aitt::AittException);
    EXPECT_THROW({ AITT aitt("unknown notice"); }, aitt::AittException);

    EXPECT_THROW(
          {
              AITT aitt(AITT_MUST_CALL_READY);
              aitt.Ready("clientId", LOCAL_IP);
              aitt.Ready("clientId", LOCAL_IP, AittOption(true, false));
          },
          aitt::AittException);

    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Ready("clientId", LOCAL_IP);
          },
          aitt::AittException);
}

TEST(AITT_Test, Not_READY_STATUS_N_Anytime)
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

TEST(AITT_Test, Publish_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG));
        aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP);
        aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP_SECURE);
    });
}

TEST(AITT_Test, Publish_max_size_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, AITT_PAYLOAD_MAX + 1),
              aitt::AittException);
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, AITT_PAYLOAD_MAX + 1, AITT_TYPE_TCP),
              aitt::AittException);
        EXPECT_THROW(
              aitt.Publish("testTopic", TEST_MSG, AITT_PAYLOAD_MAX + 1, AITT_TYPE_TCP_SECURE),
              aitt::AittException);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, Publish_Without_Connection_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG)), aitt::AittException);
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP),
              aitt::AittException);
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG), AITT_TYPE_TCP_SECURE),
              aitt::AittException);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, Publish_minus_size_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, -1, AITT_TYPE_MQTT), aitt::AittException);
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, -1, AITT_TYPE_TCP), aitt::AittException);
        EXPECT_THROW(aitt.Publish("testTopic", TEST_MSG, -1, AITT_TYPE_TCP_SECURE),
              aitt::AittException);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, Publish_Multiple_Protocols_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG),
              (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP | AITT_TYPE_TCP_SECURE));
    });
}

TEST(AITT_Test, Publish_Invalid_Protocols_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();
        aitt.Publish("testTopic", TEST_MSG, sizeof(TEST_MSG), (AittProtocol)0x100);
    } catch (aitt::AittException &e) {
        EXPECT_EQ(e.getErrCode(), aitt::AittException::INVALID_ARG);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, Unsubscribe_P_Anytime)
{
    EXPECT_NO_THROW({
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        aitt.Connect();

        auto subscribeHandle = aitt.Subscribe(
              "testTopic",
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_MQTT);
        aitt.Unsubscribe(subscribeHandle);

        subscribeHandle = aitt.Subscribe(
              "testTopic",
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP);
        aitt.Unsubscribe(subscribeHandle);

        subscribeHandle = aitt.Subscribe(
              "testTopic",
              [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) -> void {},
              nullptr, AITT_TYPE_TCP_SECURE);
        aitt.Unsubscribe(subscribeHandle);
    });
}

TEST(AITT_Test, Unsubscribe_Invalid_ID_N_Anytime)
{
    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Unsubscribe((AittSubscribeID)0x100);
          },
          aitt::AittException);
}

TEST(AITT_Test, Unsubscribe_Twice_N_Anytime)
{
    EXPECT_THROW(
          {
              AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
              aitt.Connect();

              auto subscribeHandle = aitt.Subscribe(
                    "testTopic",
                    [](AittMsg *handle, const void *msg, const int szmsg, void *cbdata) {}, nullptr,
                    AITT_TYPE_MQTT);

              aitt.Unsubscribe(subscribeHandle);
              aitt.Unsubscribe(subscribeHandle);
          },
          aitt::AittException);
}

TEST(AITT_Test, WillSet_invalid_topic_N_Anytime)
{
    EXPECT_THROW(
          {
              const char *topic_wildcard = "topic/+";
              AITT aitt_will("", LOCAL_IP, AittOption(true, false));
              aitt_will.SetWillInfo(topic_wildcard, "will msg", 8, AITT_QOS_AT_MOST_ONCE, false);
          },
          aitt::AittException);
}

TEST(AITT_Test, WillSet_size_N_Anytime)
{
    EXPECT_THROW(
          {
              AITT aitt_will("", LOCAL_IP, AittOption(true, false));
              aitt_will.SetWillInfo("testTopic", "will msg", -1, AITT_QOS_AT_MOST_ONCE, false);
          },
          aitt::AittException);
    EXPECT_THROW(
          {
              AITT aitt_will("", LOCAL_IP, AittOption(true, false));
              aitt_will.SetWillInfo("testTopic", "will msg", AITT_PAYLOAD_MAX + 1,
                    AITT_QOS_AT_MOST_ONCE, false);
          },
          aitt::AittException);
}

TEST(AITT_Test, PublishWithReply_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        EXPECT_THROW(
              aitt.PublishWithReply(
                    "testTopic", TEST_MSG, 0, AITT_TYPE_MQTT, AITT_QOS_AT_MOST_ONCE, false,
                    [](AittMsg *msg, const void *data, const int data_len, void *user_data) {},
                    nullptr, "correlation"),
              aitt::AittException);
        EXPECT_THROW(
              aitt.PublishWithReply(
                    "testTopic", TEST_MSG, AITT_PAYLOAD_MAX + 1, AITT_TYPE_MQTT,
                    AITT_QOS_AT_MOST_ONCE, false,
                    [](AittMsg *msg, const void *data, const int data_len, void *user_data) {},
                    nullptr, "correlation"),
              aitt::AittException);
        ;
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, PublishWithReplySync_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP, AittOption(true, false));
        EXPECT_THROW(
              aitt.PublishWithReplySync(
                    "testTopic", TEST_MSG, 0, AITT_TYPE_MQTT, AITT_QOS_AT_MOST_ONCE, false,
                    [](AittMsg *msg, const void *data, const int data_len, void *user_data) {},
                    nullptr, "correlation"),
              aitt::AittException);
        EXPECT_THROW(
              aitt.PublishWithReplySync(
                    "testTopic", TEST_MSG, AITT_PAYLOAD_MAX + 1, AITT_TYPE_MQTT,
                    AITT_QOS_AT_MOST_ONCE, false,
                    [](AittMsg *msg, const void *data, const int data_len, void *user_data) {},
                    nullptr, "correlation"),
              aitt::AittException);
        ;
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, CreateStream_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP);
        EXPECT_EQ(nullptr, aitt.CreateStream(AITT_STREAM_TYPE_MAX, "topic/Invalid/Stream_Type",
                                 AITT_STREAM_ROLE_PUBLISHER));
        EXPECT_EQ(nullptr, aitt.CreateStream(AITT_STREAM_TYPE_MAX, "topic/Invalid/Stream_Type",
                                 AITT_STREAM_ROLE_SUBSCRIBER));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AITT_Test, DestroyStream_N_Anytime)
{
    try {
        AITT aitt("clientId", LOCAL_IP);
        EXPECT_NO_THROW(aitt.DestroyStream(nullptr));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
