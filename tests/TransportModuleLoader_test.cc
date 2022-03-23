/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved
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
#include "TransportModuleLoader.h"

#include <AITT.h>
#include <gtest/gtest.h>

#include "TransportModule.h"
#include "log.h"

class TransportModuleLoaderTest : public testing::Test {
  public:
    TransportModuleLoaderTest(void) : loader("127.0.0.1") {}

  protected:
    void SetUp() override {}

    void TearDown() override {}

  protected:
    aitt::TransportModuleLoader loader;
};

TEST_F(TransportModuleLoaderTest, Positive_GetInstance_Anytime)
{
    std::shared_ptr<aitt::TransportModule> module = loader.GetInstance(AITT_TYPE_TCP);
    ASSERT_NE(module, nullptr);
}

TEST_F(TransportModuleLoaderTest, Negative_GetInstance_Anytime)
{
    std::shared_ptr<aitt::TransportModule> module = loader.GetInstance(AITT_TYPE_MQTT);
    ASSERT_EQ(module, nullptr);
}
