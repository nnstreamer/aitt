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
#include "ModuleLoader.h"

#include <AITT.h>
#include <gtest/gtest.h>

#include "AittTransport.h"
#include "aitt_internal.h"
#include "aitt_tests.h"

using ModuleLoader = aitt::ModuleLoader;

class ModuleLoaderTest : public testing::Test {
  public:
    ModuleLoaderTest(void) : discovery("test", false) {}

  protected:
    void SetUp() override {}
    void TearDown() override {}

    aitt::AittDiscovery discovery;
    aitt::ModuleLoader loader;
};

TEST_F(ModuleLoaderTest, LoadTransport_P_Anytime)
{
    ModuleLoader::ModuleHandle handle = loader.OpenModule(ModuleLoader::TYPE_TCP);
    ASSERT_NE(handle, nullptr);

    std::shared_ptr<aitt::AittTransport> module =
          loader.LoadTransport(handle.get(), LOCAL_IP, discovery);
    ASSERT_NE(module, nullptr);
}

TEST_F(ModuleLoaderTest, LoadTransport_N_Anytime)
{
    ModuleLoader::ModuleHandle handle = loader.OpenModule(ModuleLoader::TYPE_TRANSPORT_MAX);
    ASSERT_EQ(handle.get(), nullptr);

    auto module = loader.LoadTransport(handle.get(), LOCAL_IP, discovery);
    ASSERT_NE(module, nullptr);
}
