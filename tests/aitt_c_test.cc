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
#include "aitt_c.h"

#include <glib.h>
#include <gtest/gtest.h>

#define LOCAL_IP "127.0.0.1"

TEST(AITT_C_INTERFACE, new_P_Anytime)
{
    aitt_h handle = aitt_new("test1");
    EXPECT_TRUE(handle != nullptr);

    aitt_destroy(handle);

    handle = aitt_new(nullptr);
    EXPECT_TRUE(handle != nullptr);

    aitt_destroy(handle);

    handle = aitt_new("");
    EXPECT_TRUE(handle != nullptr);
    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, destroy_P_Anytime)
{
    aitt_h handle = aitt_new("test2");
    ASSERT_NE(handle, nullptr);

    aitt_destroy(handle);
    aitt_destroy(nullptr);
}

TEST(AITT_C_INTERFACE, option_P_Anytime)
{
    aitt_h handle = aitt_new("test3");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_set_option(handle, AITT_OPT_MY_IP, LOCAL_IP);
    EXPECT_EQ(ret, AITT_ERROR_NONE);
    EXPECT_STREQ(LOCAL_IP, aitt_get_option(handle, AITT_OPT_MY_IP));

    ret = aitt_set_option(handle, AITT_OPT_MY_IP, NULL);
    EXPECT_EQ(ret, AITT_ERROR_NONE);
    EXPECT_EQ(NULL, aitt_get_option(handle, AITT_OPT_MY_IP));

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, option_N_Anytime)
{
    aitt_h handle = aitt_new("test4");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_set_option(handle, AITT_OPT_UNKNOWN, LOCAL_IP);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_set_option(nullptr, AITT_OPT_MY_IP, LOCAL_IP);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, connect_disconnect_P_Anytime)
{
    aitt_h handle = aitt_new("test5");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_NONE);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, connect_N_Anytime)
{
    aitt_h handle = aitt_new("test6");
    ASSERT_NE(handle, nullptr);

    aitt_h invalid_handle = nullptr;
    int ret = aitt_connect(invalid_handle, LOCAL_IP, 1883);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    const char *invalid_ip = "1.2.3";
    ret = aitt_connect(handle, invalid_ip, 1883);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    invalid_ip = "";
    ret = aitt_connect(handle, invalid_ip, 1883);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    int invalid_port = -1;
    ret = aitt_connect(handle, LOCAL_IP, invalid_port);
    EXPECT_EQ(ret, AITT_ERROR_SYSTEM);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, disconnect_N_Anytime)
{
    int ret = aitt_disconnect(nullptr);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_h handle = aitt_new("test7");
    ASSERT_NE(handle, nullptr);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_destroy(handle);
}

#define test_msg "test123456789"
#define test_topic "test/topic_1"
TEST(AITT_C_INTERFACE, pub_sub_P_Anytime)
{
    aitt_h handle = aitt_new("test8");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    aitt_sub_h sub_handle = nullptr;
    ret = aitt_subscribe(
          handle, test_topic,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {
              GMainLoop *loop = static_cast<GMainLoop *>(user_data);
              std::string received_data((const char *)msg, msg_len);
              EXPECT_STREQ(received_data.c_str(), test_msg);
              EXPECT_STREQ(aitt_msg_get_topic(msg_handle), test_topic);
              g_main_loop_quit(loop);
          },
          loop, &sub_handle);
    ASSERT_EQ(ret, AITT_ERROR_NONE);
    EXPECT_TRUE(sub_handle != nullptr);

    ret = aitt_publish(handle, test_topic, test_msg, strlen(test_msg));
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_NONE);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, pub_N_Anytime)
{
    aitt_h handle = aitt_new("test9");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, nullptr, 1883);
    EXPECT_NE(ret, AITT_ERROR_NONE);

    ret = aitt_publish(handle, test_topic, test_msg, strlen(test_msg));
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish(nullptr, test_topic, test_msg, strlen(test_msg));
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish(handle, nullptr, test_msg, strlen(test_msg));
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish(handle, test_topic, nullptr, strlen(test_msg));
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, sub_N_Anytime)
{
    aitt_h handle = aitt_new("test10");
    aitt_sub_h sub_handle = nullptr;
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, nullptr, 1883);
    EXPECT_NE(ret, AITT_ERROR_NONE);

    ret = aitt_subscribe(
          handle, test_topic, [](aitt_msg_h, const void *, size_t, void *) {}, nullptr,
          &sub_handle);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_subscribe(
          nullptr, test_topic, [](aitt_msg_h, const void *, size_t, void *) {}, nullptr,
          &sub_handle);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_subscribe(
          handle, nullptr, [](aitt_msg_h, const void *, size_t, void *) {}, nullptr, &sub_handle);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_subscribe(handle, test_topic, nullptr, nullptr, &sub_handle);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_destroy(handle);
}

#define reply_msg "hello reply message"
#define test_correlation "0001"

TEST(AITT_C_INTERFACE, pub_with_reply_send_reply_P_Anytime)
{
    aitt_h handle = aitt_new("test11");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    aitt_sub_h sub_handle = nullptr;
    ret = aitt_subscribe(
          handle, test_topic,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {
              aitt_h handle = static_cast<aitt_h>(user_data);
              std::string received_data((const char *)msg, msg_len);
              EXPECT_STREQ(received_data.c_str(), test_msg);
              EXPECT_STREQ(aitt_msg_get_topic(msg_handle), test_topic);
              aitt_send_reply(handle, msg_handle, reply_msg, sizeof(reply_msg), true);
          },
          handle, &sub_handle);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    ret = aitt_publish_with_reply(
          handle, test_topic, test_msg, strlen(test_msg), AITT_TYPE_MQTT, 0, test_correlation,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {
              GMainLoop *loop = static_cast<GMainLoop *>(user_data);
              std::string received_data((const char *)msg, msg_len);
              EXPECT_STREQ(received_data.c_str(), reply_msg);
              g_main_loop_quit(loop);
          },
          loop);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_NONE);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, pub_with_reply_N_Anytime)
{
    aitt_h handle = aitt_new("test12");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    ret = aitt_publish_with_reply(
          nullptr, test_topic, test_msg, strlen(test_msg), AITT_TYPE_MQTT, 0, test_correlation,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {}, nullptr);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish_with_reply(
          handle, nullptr, test_msg, strlen(test_msg), AITT_TYPE_MQTT, 0, test_correlation,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {}, nullptr);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish_with_reply(
          handle, test_topic, nullptr, 0, AITT_TYPE_MQTT, 0, test_correlation,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {}, nullptr);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    ret = aitt_publish_with_reply(handle, test_topic, test_msg, strlen(test_msg), AITT_TYPE_MQTT, 0,
          test_correlation, nullptr, nullptr);
    EXPECT_EQ(ret, AITT_ERROR_INVALID_PARAMETER);

    aitt_destroy(handle);
}

TEST(AITT_C_INTERFACE, sub_unsub_P_Anytime)
{
    aitt_h handle = aitt_new("test13");
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    static unsigned int sub_call_count = 0;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    static aitt_sub_h sub_handle = nullptr;
    ret = aitt_subscribe(
          handle, test_topic,
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {
              sub_call_count++;
          },
          nullptr, &sub_handle);
    ASSERT_EQ(ret, AITT_ERROR_NONE);
    EXPECT_TRUE(sub_handle != nullptr);

    ret = aitt_publish(handle, test_topic, test_msg, strlen(test_msg));
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    g_timeout_add(
          1000,
          [](gpointer data) -> gboolean {
              aitt_h handle = static_cast<aitt_h>(data);
              int ret = aitt_unsubscribe(handle, sub_handle);
              EXPECT_EQ(ret, AITT_ERROR_NONE);
              sub_handle = nullptr;

              ret = aitt_publish(handle, test_topic, test_msg, strlen(test_msg));
              EXPECT_EQ(ret, AITT_ERROR_NONE);
              return FALSE;
          },
          handle);

    g_timeout_add(
          2000,
          [](gpointer data) -> gboolean {
              GMainLoop *loop = static_cast<GMainLoop *>(data);
              EXPECT_EQ(sub_call_count, 1);

              if (sub_call_count == 1) {
                  g_main_loop_quit(loop);
                  return FALSE;
              }

              return FALSE;
          },
          loop);

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_NONE);

    aitt_destroy(handle);
}
