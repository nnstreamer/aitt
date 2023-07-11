/*
 * Copyright 2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include "AittOption.h"

#include <gtest/gtest.h>

TEST(Option, CustomBroker_SetServiceID_N_Anytime)
{
    AittOption option;

    option.SetUseCustomMqttBroker(true);

    option.SetServiceID("test_service_id");
    EXPECT_STREQ(option.GetServiceID(), "test_service_id");

    option.SetUseCustomMqttBroker(false);

    option.SetServiceID("");
    EXPECT_STRNE(option.GetServiceID(), "");
}

TEST(Option, CustomBroker_SetLocationID_N_Anytime)
{
    AittOption option;

    option.SetUseCustomMqttBroker(true);

    option.SetLocationID("test_location_id");
    EXPECT_STREQ(option.GetLocationID(), "test_location_id");

    option.SetUseCustomMqttBroker(false);

    option.SetLocationID("");
    EXPECT_STRNE(option.GetLocationID(), "");
}

TEST(Option, CustomBroker_SetRootCA_N_Anytime)
{
    AittOption option;

    option.SetUseCustomMqttBroker(true);

    option.SetRootCA("test_root_ca");
    EXPECT_STREQ(option.GetRootCA(), "test_root_ca");

    option.SetUseCustomMqttBroker(false);

    option.SetRootCA("");
    EXPECT_STRNE(option.GetRootCA(), "");
}

TEST(Option, CustomBroker_SetCustomRWFile_N_Anytime)
{
    AittOption option;

    option.SetUseCustomMqttBroker(true);

    option.SetCustomRWFile("test_custom_rw_file");
    EXPECT_STREQ(option.GetCustomRWFile(), "test_custom_rw_file");

    option.SetUseCustomMqttBroker(false);

    option.SetCustomRWFile("");
    EXPECT_STRNE(option.GetCustomRWFile(), "");
}
