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
#pragma once

#ifdef API
#undef API
#endif
#define API __attribute__((visibility("default")))

#define STR_EQ 0

#define AITT_MANAGED_TOPIC_PREFIX "/v1/custom/f5c7b34e48c1918f/"
#define DISCOVERY_TOPIC_BASE std::string(AITT_MANAGED_TOPIC_PREFIX "discovery/")
#define RESPONSE_POSTFIX "_AittRe_"

// Specification MQTT-4.7.3-3
#define AITT_TOPIC_NAME_MAX 65535

// Specification MQTT-1.5.5
#define AITT_PAYLOAD_MAX 268435455
