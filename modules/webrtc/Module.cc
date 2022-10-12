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

#include "AittException.h"
#include "aitt_internal.h"

namespace AittWebRTCNamespace {

Module::Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role)
      : discovery_(discovery), topic_(topic), role_(role)
{
    discovery_cb_ = discovery_.AddDiscoveryCB(topic,
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

Module::~Module(void)
{
    discovery_.RemoveDiscoveryCB(discovery_cb_);
}

void Module::SetConfig(const std::string &key, const std::string &value)
{
}

void Module::SetConfig(const std::string &key, void *obj)
{
}

void Module::Start(void)
{
}

void Module::SetStateCallback(StateCallback cb, void *user_data)
{
}

void Module::SetReceiveCallback(ReceiveCallback cb, void *user_data)
{
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
}

}  // namespace AittWebRTCNamespace
