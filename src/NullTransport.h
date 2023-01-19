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

#include <AittTransport.h>

using AittTransport = aitt::AittTransport;
using AittDiscovery = aitt::AittDiscovery;

class NullTransport : public AittTransport {
  public:
    explicit NullTransport(AittDiscovery &discovery, const std::string &ip);
    virtual ~NullTransport(void) = default;

    void Publish(const std::string &topic, const void *data, const int datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false) override;

    void *Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata = nullptr,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE) override;

    void *Unsubscribe(void *handle) override;

    void PublishWithReply(const std::string &topic, const void *data, const int datalen,
          AittQoS qos, bool retain, const std::string &reply_topic,
          const std::string &correlation) override;

    void SendReply(AittMsg *msg, const void *data, const int datalen, AittQoS qos,
          bool retain) override;
};
