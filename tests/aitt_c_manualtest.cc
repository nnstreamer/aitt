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

#include "aitt_tests.h"

TEST(AITT_C_MANUAL, will_set_P)
{
    int ret;
    aitt_h handle = aitt_new("test14", LOCAL_IP);
    ASSERT_NE(handle, nullptr);

    ret = aitt_connect(handle, LOCAL_IP, 1883);
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    static bool sub_called = false;
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    aitt_sub_h sub_handle = nullptr;
    ret = aitt_subscribe(
          handle, "test/topic_will",
          [](aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data) {
              std::string received_data((const char *)msg, msg_len);
              EXPECT_STREQ(received_data.c_str(), TEST_C_MSG);
              EXPECT_STREQ(aitt_msg_get_topic(msg_handle), TEST_C_TOPIC);
              sub_called = true;
          },
          loop, &sub_handle);
    ASSERT_EQ(ret, AITT_ERROR_NONE);
    EXPECT_TRUE(sub_handle != nullptr);

    int pid = fork();
    if (pid == 0) {
        aitt_h handle_will = aitt_new("test_will", LOCAL_IP);
        ASSERT_NE(handle_will, nullptr);

        ret = aitt_will_set(handle_will, "test/topic_will", TEST_C_MSG, strlen(TEST_C_MSG),
              AITT_QOS_AT_LEAST_ONCE, false);
        ASSERT_EQ(ret, AITT_ERROR_NONE);

        ret = aitt_connect(handle_will, LOCAL_IP, 1883);
        ASSERT_EQ(ret, AITT_ERROR_NONE);

        // Do not call below
        // aitt_disconnect(handle_will)
        // aitt _destroy(handle_will);
    } else {
        sleep(1);
        kill(pid, SIGKILL);

        g_timeout_add(
              10,
              [](gpointer data) -> gboolean {
                  if (sub_called) {
                      GMainLoop *loop = static_cast<GMainLoop *>(data);
                      g_main_loop_quit(loop);
                      return FALSE;
                  }
                  return TRUE;
              },
              loop);

        g_main_loop_run(loop);
        g_main_loop_unref(loop);

        ret = aitt_disconnect(handle);
        EXPECT_EQ(ret, AITT_ERROR_NONE);

        aitt_destroy(handle);
    }
}

// Set user/passwd in mosquitto.conf before testing
TEST(AITT_C_MANUAL, connect_id_passwd_P)
{
    aitt_h handle = aitt_new("test15", LOCAL_IP);
    ASSERT_NE(handle, nullptr);

    int ret = aitt_connect_full(handle, LOCAL_IP, 1883, "testID", "testPasswd");
    ASSERT_EQ(ret, AITT_ERROR_NONE);

    ret = aitt_disconnect(handle);
    EXPECT_EQ(ret, AITT_ERROR_NONE);

    aitt_destroy(handle);
}
