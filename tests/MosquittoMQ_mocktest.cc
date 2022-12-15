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
#include "MosquittoMQ.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

#include "AittTypes.h"
#include "MosquittoMock.h"

using ::testing::Return;

#define TEST_TOPIC "Test/Topic"
#define TEST_PAYLOAD "The last will is ..."
#define TEST_CLIENT_ID "testClient"
#define TEST_PORT 8123
#define TEST_HOST "localhost"
#define TEST_HANDLE reinterpret_cast<mosquitto *>(0xbeefbeef)

class MQMockTest : public ::testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}

    MosquittoMock mqttMock;
};

TEST_F(MQMockTest, Create_lib_init_N_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_NOT_SUPPORTED));
    EXPECT_CALL(mqttMock, mosquitto_destroy(nullptr)).WillOnce(Return());
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        FAIL() << "lib_init must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), "MQTT failure : MosquittoMQ Constructor Error");
    }
}

TEST_F(MQMockTest, Create_new_N_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(nullptr));
    EXPECT_CALL(mqttMock, mosquitto_destroy(nullptr)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        FAIL() << "lib_init must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), "MQTT failure : MosquittoMQ Constructor Error");
    }
}

TEST_F(MQMockTest, Publish_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock,
          mosquitto_int_option(TEST_HANDLE, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_publish(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC),
                                sizeof(TEST_PAYLOAD), TEST_PAYLOAD, AITT_QOS_AT_MOST_ONCE, false))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, "", "");
        mq.Publish(TEST_TOPIC, TEST_PAYLOAD, sizeof(TEST_PAYLOAD));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQMockTest, Subscribe_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_subscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC),
                                AITT_QOS_AT_MOST_ONCE))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, "", "");
        mq.Subscribe(
              TEST_TOPIC,
              [](aitt::MSG *info, const std::string &topic, const void *msg, const int szmsg,
                    const void *cbdata) -> void {},
              nullptr, AITT_QOS_AT_MOST_ONCE);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQMockTest, Unsubscribe_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock,
          mosquitto_subscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC), 0))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock,
          mosquitto_unsubscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC)))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, "", "");
        void *handle = mq.Subscribe(
              TEST_TOPIC,
              [](aitt::MSG *info, const std::string &topic, const void *msg, const int szmsg,
                    const void *cbdata) -> void {},
              nullptr, AITT_QOS_AT_MOST_ONCE);
        mq.Unsubscribe(handle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQMockTest, Create_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock,
          mosquitto_int_option(TEST_HANDLE, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5))
          .Times(1);
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception occurred";
    }
}

TEST_F(MQMockTest, Connect_will_set_N_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_will_set(TEST_HANDLE, testing::StrEq("lastWill"),
                                sizeof(TEST_PAYLOAD), TEST_PAYLOAD, AITT_QOS_AT_MOST_ONCE, true))
          .WillOnce(Return(MOSQ_ERR_NOMEM));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.SetWillInfo("lastWill", TEST_PAYLOAD, sizeof(TEST_PAYLOAD), AITT_QOS_AT_MOST_ONCE, true);
        mq.Connect(TEST_HOST, TEST_PORT, "", "");
        FAIL() << "Connect() must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), "MQTT failure");
    }
}

TEST_F(MQMockTest, Connect_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, "", "");
    } catch (std::exception &e) {
        FAIL() << "Unepxected exception: " << e.what();
    }
}

TEST_F(MQMockTest, Connect_User_P_Anytime)
{
    std::string username = "test";
    std::string password = "test";
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock,
          mosquitto_username_pw_set(TEST_HANDLE, username.c_str(), password.c_str()))
          .Times(1);
    EXPECT_CALL(mqttMock, mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, username, password);
    } catch (std::exception &e) {
        FAIL() << "Unepxected exception: " << e.what();
    }
}

TEST_F(MQMockTest, Disconnect_P_Anytime)
{
    EXPECT_CALL(mqttMock, mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(mqttMock, mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_disconnect(testing::_)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_will_clear(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(mqttMock, mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(mqttMock, mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MosquittoMQ mq(TEST_CLIENT_ID, true);
        mq.Disconnect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
