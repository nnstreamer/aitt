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
#include "MosquittoMQ.h"

#include <gtest/gtest.h>

#include "AittTests.h"
#include "aitt_internal.h"

using MosquittoMQ = aitt::MosquittoMQ;

class MQTest : public testing::Test, public AittTests {
  protected:
    void SetUp() override { Init(); }
    void TearDown() override { Deinit(); }
};

TEST_F(MQTest, Subscribe_in_Subscribe_MQTT_P_Anytime)
{
    try {
        MosquittoMQ mq("MQ_TEST_ID");
        mq.Connect(LOCAL_IP, 1883, "", "");
        mq.Subscribe(
              "MQ_TEST_TOPIC1",
              [&](AittMsg *handle, const std::string &topic, const void *data, const int datalen,
                    void *user_data) {
                  DBG("Subscribe invoked: %s %d", static_cast<const char *>(data), datalen);

                  mq.Subscribe(
                        "topic1InCallback",
                        [](AittMsg *handle, const std::string &topic, const void *msg,
                              const int szmsg, void *cbdata) {},
                        user_data);

                  mq.Subscribe(
                        "topic2InCallback",
                        [](AittMsg *handle, const std::string &topic, const void *msg,
                              const int szmsg, void *cbdata) {},
                        user_data);

                  MQTest *test = static_cast<MQTest *>(user_data);
                  test->ToggleReady();
              },
              static_cast<void *>(this));

        DBG("Publish message to %s (%s)", "MQ_TEST_TOPIC1", TEST_MSG);
        mq.Publish("MQ_TEST_TOPIC1", TEST_MSG, sizeof(TEST_MSG));

        mainLoop.AddTimeout(
              CHECK_INTERVAL,
              [&](MainLoopHandler::MainLoopResult result, int fd,
                    MainLoopHandler::MainLoopData *data) -> int {
                  return ReadyCheck(static_cast<AittTests *>(this));
              },
              nullptr);

        IterateEventLoop();

        ASSERT_TRUE(ready);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}
