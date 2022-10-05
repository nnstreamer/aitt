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

#include "Module.h"

#include <flatbuffers/flexbuffers.h>

#include "aitt_internal.h"

namespace AittRTSPNamespace {

Module::Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role)
      : discovery_(discovery), topic_(topic), role_(role), info(nullptr), client(nullptr), current_state(0)
{
    DBG("RTSP Module constructor : %s, role : %d", topic_.c_str(), role_);

    discovery_cb_ = discovery_.AddDiscoveryCB(topic,
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

Module::~Module(void)
{
    DBG("RTSP Module destroyer : %s, role : %d", topic_.c_str(), role_);
}

void Module::SetConfig(const std::string &key, const std::string &value)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) {
        /* Add rtsp server table */
    }
}

void Module::SetConfig(const std::string &key, void *obj)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER) {
        /* Set evas object */
    }
}

void Module::Start(void)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) {
        /* Update Rtsp Server table with retained message of MQTT broker */
    }
    else {
        /* Check if the topic exists in server_table to find */
        /* if exists, then create pipeline using that information */
        /* if not, wait until discovery message from Publisher */
    }
}

void Module::SetStateCallback(StateCallback cb, void *user_data)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        return;

}

void Module::SetReceiveCallback(ReceiveCallback cb, void *user_data)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        return;
}

}  // namespace AittRTSPNamespace
