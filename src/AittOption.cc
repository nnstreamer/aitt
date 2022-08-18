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
#include "AittOption.h"

AittOption::AittOption() : clear_session_(false), use_custom_broker_(false)
{
}

AittOption::AittOption(bool clear_session, bool use_custom_mqtt_broker)
      : clear_session_(clear_session), use_custom_broker_(use_custom_mqtt_broker)
{
}

void AittOption::SetClearSession(bool val)
{
    clear_session_ = val;
}

bool AittOption::GetClearSession()
{
    return clear_session_;
}

void AittOption::SetUseCustomMqttBroker(bool val)
{
    use_custom_broker_ = val;
}

bool AittOption::GetUseCustomMqttBroker()
{
    return use_custom_broker_;
}
