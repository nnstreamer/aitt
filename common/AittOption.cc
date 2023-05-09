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
#include "AittOption.h"

#include "aitt_internal.h"

AittOption::AittOption() : clean_session_(false), use_custom_broker(false)
{
}

AittOption::AittOption(bool clean_session, bool use_custom_mqtt_broker)
      : clean_session_(clean_session), use_custom_broker(use_custom_mqtt_broker)
{
}

void AittOption::SetCleanSession(bool val)
{
    clean_session_ = val;
}

bool AittOption::GetCleanSession() const
{
    return clean_session_;
}

void AittOption::SetUseCustomMqttBroker(bool val)
{
    use_custom_broker = val;
}

bool AittOption::GetUseCustomMqttBroker() const
{
    return use_custom_broker;
}

void AittOption::SetServiceID(const std::string& id)
{
    RET_IF(false == use_custom_broker);
    service_id = id;
}

const char* AittOption::GetServiceID() const
{
    return service_id.c_str();
}

void AittOption::SetLocationID(const std::string& id)
{
    RET_IF(false == use_custom_broker);
    location_id = id;
}

const char* AittOption::GetLocationID() const
{
    return location_id.c_str();
}

void AittOption::SetRootCA(const std::string& ca)
{
    RET_IF(false == use_custom_broker);
    root_ca = ca;
}

const char* AittOption::GetRootCA() const
{
    return root_ca.c_str();
}

void AittOption::SetCustomRWFile(const std::string& file)
{
    RET_IF(false == use_custom_broker);
    custom_rw_file = file;
}

const char* AittOption::GetCustomRWFile() const
{
    return custom_rw_file.c_str();
}
