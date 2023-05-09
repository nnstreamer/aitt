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
#include "aitt_internal.h"

using ModuleManager = aitt::ModuleManager;

class ModuleLoaderTest : public testing::Test {
  public:
    ModuleLoaderTest(void) : discovery("test_id"), modules(LOCAL_IP, discovery) {}

  protected:
    void SetUp() override {}
    void TearDown() override {}

    aitt::AittDiscovery discovery;
    aitt::ModuleManager modules;
};

TEST_F(ModuleLoaderTest, Get_P_Anytime)
{
    aitt::AittTransport &tcp = modules.Get(AITT_TYPE_TCP);
    EXPECT_TRUE(tcp.GetProtocol() == AITT_TYPE_TCP);
    aitt::AittTransport &tcp_secure = modules.Get(AITT_TYPE_TCP_SECURE);
    EXPECT_TRUE(tcp_secure.GetProtocol() == AITT_TYPE_TCP_SECURE);
}
TEST_F(ModuleLoaderTest, Get_N_Anytime)
{
    EXPECT_THROW(
          {
              aitt::AittTransport &module = modules.Get(AITT_TYPE_MQTT);
              FAIL() << "Should not be called" << module.GetProtocol();
          },
          aitt::AittException);
}

TEST_F(ModuleLoaderTest, NewCustomMQ_P)
{
    EXPECT_NO_THROW({
        std::unique_ptr<aitt::MQ> mq = modules.NewCustomMQ("test", AittOption(false, true));
        mq->SetConnectionCallback([](int status) {});
    });
}

TEST_F(ModuleLoaderTest, NewCustomMQ_N_Anytime)
{
    EXPECT_THROW(
          {
              modules.NewCustomMQ("test", AittOption(false, false));
              FAIL() << "Should not be called";
          },
          aitt::AittException);
}
