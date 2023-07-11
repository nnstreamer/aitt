/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include <AITT.h>
#include <gtest/gtest.h>

#include "AittTests.h"
#include "AittTransport.h"
#include "ModuleManager.h"
#include "NullTransport.h"
#include "aitt_internal.h"

using ModuleManager = aitt::ModuleManager;

class ModuleManagerTest : public testing::Test {
  public:
    ModuleManagerTest(void) : discovery("test_id"), modules(LOCAL_IP, discovery) {}

  protected:
    void SetUp() override {}
    void TearDown() override {}

    aitt::AittDiscovery discovery;
    aitt::ModuleManager modules;
};

TEST_F(ModuleManagerTest, Get_P_Anytime)
{
    aitt::AittTransport &tcp = modules.Get(AITT_TYPE_TCP);
    EXPECT_TRUE(tcp.GetProtocol() == AITT_TYPE_TCP);
    aitt::AittTransport &tcp_secure = modules.Get(AITT_TYPE_TCP_SECURE);
    EXPECT_TRUE(tcp_secure.GetProtocol() == AITT_TYPE_TCP_SECURE);
}

TEST_F(ModuleManagerTest, Get_N_Anytime)
{
    EXPECT_THROW(
          {
              aitt::AittTransport &module = modules.Get(AITT_TYPE_MQTT);
              FAIL() << "Should not be called" << module.GetProtocol();
          },
          aitt::AittException);
}

TEST_F(ModuleManagerTest, NewCustomMQ_P)
{
    EXPECT_NO_THROW({
        std::unique_ptr<aitt::MQ> mq = modules.NewCustomMQ("test", AittOption(false, true));
        mq->SetConnectionCallback([](int status) {});
    });
}

TEST_F(ModuleManagerTest, NewCustomMQ_N_Anytime)
{
    EXPECT_THROW(
          {
              modules.NewCustomMQ("test", AittOption(false, false));
              FAIL() << "Should not be called";
          },
          aitt::AittException);
}

TEST_F(ModuleManagerTest, CreateStream_N_Anytime)
{
    EXPECT_EQ(nullptr, modules.CreateStream(AITT_STREAM_TYPE_MAX, "topic/Invalid/Stream_Type",
                             AITT_STREAM_ROLE_PUBLISHER));
    EXPECT_EQ(nullptr, modules.CreateStream(AITT_STREAM_TYPE_MAX, "topic/Invalid/Stream_Type",
                             AITT_STREAM_ROLE_SUBSCRIBER));
}

TEST_F(ModuleManagerTest, DestroyStream_N_Anytime)
{
    EXPECT_NO_THROW(modules.DestroyStream(nullptr));
}

TEST_F(ModuleManagerTest, NullTranport_N_Anytime)
{
    NullTransport null_transport(discovery, LOCAL_IP);

    EXPECT_NO_THROW({
        null_transport.Publish("AnyTopic", "AnyData", 0);
        null_transport.PublishWithReply("AnyTopic", "AnyData", 0, AITT_QOS_AT_MOST_ONCE, false,
              "AnyReplyTopic", "AnyCorrelation");
        null_transport.SendReply(nullptr, "AnyData", 0, AITT_QOS_AT_MOST_ONCE, false);

        EXPECT_EQ(nullptr,
              null_transport.Subscribe(
                    "AnyTopic",
                    [](AittMsg *handle, const void *data, const int datalen, void *cbdata) {},
                    nullptr));
        EXPECT_EQ(nullptr, null_transport.Unsubscribe(nullptr));
        char any_pointer;
        EXPECT_EQ(nullptr, null_transport.Unsubscribe(&any_pointer));
        EXPECT_EQ(0, null_transport.CountSubscriber("AnyTopic"));
    });
}
