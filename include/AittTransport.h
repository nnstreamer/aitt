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

#include <AittDiscovery.h>
#include <AittTypes.h>

#include <functional>
#include <string>

namespace aitt {

class AittTransport {
  public:
    typedef void *(*ModuleEntry)(const char *ip, AittDiscovery &discovery);
    using SubscribeCallback = std::function<void(const std::string &topic, const void *msg,
          const size_t szmsg, void *cbdata, const std::string &correlation)>;

    static constexpr const char *const MODULE_ENTRY_NAME = "aitt_module_entry";

    explicit AittTransport(AittDiscovery &discovery) : discovery(discovery) {}
    virtual ~AittTransport(void) = default;

    virtual void Publish(const std::string &topic, const void *data, const size_t datalen,
          const std::string &correlation, AittQoS qos = AITT_QOS_AT_MOST_ONCE,
          bool retain = false) = 0;

    virtual void Publish(const std::string &topic, const void *data, const size_t datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false) = 0;

    virtual void *Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AittQoS qos = AITT_QOS_AT_MOST_ONCE) = 0;
    virtual void *Subscribe(const std::string &topic, const SubscribeCallback &cb, const void *data,
          const size_t datalen, void *cbdata = nullptr, AittQoS qos = AITT_QOS_AT_MOST_ONCE) = 0;

    virtual void *Unsubscribe(void *handle) = 0;

  protected:
    aitt::AittDiscovery &discovery;
};

}  // namespace aitt
