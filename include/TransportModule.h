/*
 * Copyright (c) 2021-2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <AITT.h>

#include <functional>
#include <string>

namespace aitt {

class TransportModule {
  public:
    typedef void *(*ModuleEntry)(const char *ip);
    using SubscribeCallback = std::function<void(const std::string &topic, const void *msg,
          const size_t szmsg, void *cbdata, const std::string &correlation)>;

    static constexpr const char *const MODULE_ENTRY_NAME = "aitt_module_entry";

    TransportModule(void) = default;
    virtual ~TransportModule(void) = default;

    virtual void Publish(const std::string &topic, const void *data, const size_t datalen,
          const std::string &correlation, AITT::QoS qos = AITT::QoS::AT_MOST_ONCE,
          bool retain = false) = 0;

    virtual void Publish(const std::string &topic, const void *data, const size_t datalen,
          AITT::QoS qos = AITT::QoS::AT_MOST_ONCE, bool retain = false) = 0;

    virtual void *Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AITT::QoS qos = AITT::QoS::AT_MOST_ONCE) = 0;

    virtual void *Unsubscribe(void *handle) = 0;

  public:
    // NOTE:
    // The following callback is going to be called when there is a message of the discovery
    // information The callback will be called by the AITT implementation
    virtual void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg) = 0;

    // AITT implementation could call this method to get the discovery message to broadcast it
    // through the MQTT broker
    virtual void GetDiscoveryMessage(const void *&msg, int &szmsg) = 0;

    // NOTE:
    // If we are able to use a string for the protocol,
    // the module can be developed more freely.
    // even if modules based on the same protocol, implementations can be different.
    virtual AittProtocol GetProtocol(void) = 0;
};

}  // namespace aitt
