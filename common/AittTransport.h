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
#include <AittMsg.h>
#include <AittTypes.h>

#include <string>

#define AITT_TRANSPORT_NEW aitt_transport_new
#define TO_STR(s) #s
#define DEFINE_TO_STR(x) TO_STR(x)

namespace aitt {

class AittTransport {
  public:
    typedef void *(
          *ModuleEntry)(AittProtocol type, AittDiscovery &discovery, const std::string &my_ip);
    using SubscribeCallback = AittMsgCB;

    static constexpr const char *const MODULE_ENTRY_NAME = DEFINE_TO_STR(AITT_TRANSPORT_NEW);

    explicit AittTransport(AittProtocol type, AittDiscovery &discovery)
          : protocol(type), discovery(discovery)
    {
    }
    virtual ~AittTransport(void) = default;

    virtual void Publish(const std::string &topic, const void *data, const int datalen,
          const std::string &correlation, AittQoS qos = AITT_QOS_AT_MOST_ONCE,
          bool retain = false) = 0;
    virtual void Publish(const std::string &topic, const void *data, const int datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false) = 0;
    virtual void *Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AittQoS qos = AITT_QOS_AT_MOST_ONCE) = 0;
    virtual void *Unsubscribe(void *handle) = 0;

    AittProtocol GetProtocol() { return protocol; }

  protected:
    AittProtocol protocol;
    AittDiscovery &discovery;
};

}  // namespace aitt

#undef TO_STR
#undef DEFINE_TO_STR
