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
#include "MQ.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

#include "MQTTMock.h"
#include "MQTTTest.h"

using ::testing::Return;

#define TEST_TOPIC "Test/Topic"
#define TEST_PAYLOAD "The last will is ..."
#define TEST_CLIENT_ID "testClient"
#define TEST_PORT 8123
#define TEST_HOST "localhost"
#define TEST_HANDLE reinterpret_cast<mosquitto *>(0xbeefbeef)

TEST_F(MQTTTest, Negative_Create_lib_init_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_NOT_SUPPORTED));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        FAIL() << "lib_init must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), mosquitto_strerror(MOSQ_ERR_NOT_SUPPORTED));
    }
}

TEST_F(MQTTTest, Negative_Create_new_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(nullptr));
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        FAIL() << "lib_init must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), "Failed to mosquitto_new");
    }
}

TEST_F(MQTTTest, Positive_Publish_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_publish(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC),
                                 sizeof(TEST_PAYLOAD), TEST_PAYLOAD, 0, false))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Start();
        mq.Connect(TEST_HOST, TEST_PORT);
        mq.Publish(TEST_TOPIC, TEST_PAYLOAD, sizeof(TEST_PAYLOAD));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQTTTest, Positive_Subscribe_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(),
          mosquitto_subscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC), 0))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Start();
        mq.Connect(TEST_HOST, TEST_PORT);
        mq.Subscribe(
              TEST_TOPIC,
              [](aitt::MSG *info, const std::string &topic, const void *msg, const int szmsg,
                    const void *cbdata) -> void {},
              nullptr, static_cast<aitt::MQ::QoS>(0));
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQTTTest, Positive_Unsubscribe_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(),
          mosquitto_subscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC), 0))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(),
          mosquitto_unsubscribe(TEST_HANDLE, testing::_, testing::StrEq(TEST_TOPIC)))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Start();
        mq.Connect(TEST_HOST, TEST_PORT);
        void *handle = mq.Subscribe(
              TEST_TOPIC,
              [](aitt::MSG *info, const std::string &topic, const void *msg, const int szmsg,
                    const void *cbdata) -> void {},
              nullptr, static_cast<aitt::MQ::QoS>(0));
        mq.Unsubscribe(handle);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQTTTest, Positive_Create_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception occurred";
    }
}

TEST_F(MQTTTest, Negative_Connect_will_set_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_will_set(TEST_HANDLE, testing::StrEq("lastWill"),
                                 sizeof(TEST_PAYLOAD), TEST_PAYLOAD, 0, true))
          .WillOnce(Return(MOSQ_ERR_NOMEM));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT, "lastWill", TEST_PAYLOAD, sizeof(TEST_PAYLOAD),
              aitt::MQ::QoS::AT_MOST_ONCE, true);
        FAIL() << "Connect() must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), mosquitto_strerror(MOSQ_ERR_NOMEM));
    }
}

TEST_F(MQTTTest, Positive_Connect_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_connect(TEST_HANDLE, testing::StrEq(TEST_HOST), TEST_PORT, 60))
          .WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Connect(TEST_HOST, TEST_PORT);
    } catch (std::exception &e) {
        FAIL() << "Unepxected exception: " << e.what();
    }
}

TEST_F(MQTTTest, Positive_Disconnect_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_disconnect(testing::_)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_will_clear(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Disconnect();
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(MQTTTest, Negative_Start_invalid_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_loop_start(TEST_HANDLE)).WillOnce(Return(MOSQ_ERR_INVAL));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Start();
        FAIL() << "Start() must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), mosquitto_strerror(MOSQ_ERR_INVAL));
    }
}

TEST_F(MQTTTest, Negative_Stop_invalid_Anytime)
{
    EXPECT_CALL(GetMock(), mosquitto_lib_init()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(GetMock(), mosquitto_new(testing::StrEq(TEST_CLIENT_ID), true, testing::_))
          .WillOnce(Return(TEST_HANDLE));
    EXPECT_CALL(GetMock(), mosquitto_message_v5_callback_set(TEST_HANDLE, testing::_)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_loop_stop(TEST_HANDLE, false))
          .WillOnce(Return(MOSQ_ERR_INVAL));
    EXPECT_CALL(GetMock(), mosquitto_destroy(TEST_HANDLE)).Times(1);
    EXPECT_CALL(GetMock(), mosquitto_lib_cleanup()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    try {
        aitt::MQ mq(TEST_CLIENT_ID, true);
        mq.Stop();
        FAIL() << "Stop() must be failed";
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), mosquitto_strerror(MOSQ_ERR_INVAL));
    }
}
