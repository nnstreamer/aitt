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
#include <memory>

#include "AITTImpl.h"
#include "log.h"

namespace aitt {

AITT::AITT(const std::string &id, const std::string &ip_addr, bool clear_session)
      : pImpl(std::make_unique<AITT::Impl>(this, id, ip_addr, clear_session))
{
}

AITT::~AITT(void)
{
}

void AITT::Connect(const std::string &host, int port)
{
    return pImpl->Connect(host, port);
}

void AITT::Disconnect(void)
{
    return pImpl->Disconnect();
}

void AITT::Publish(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocols, AITT::QoS qos, bool retain)
{
    return pImpl->Publish(topic, data, datalen, protocols, qos, retain);
}

int AITT::PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, AITT::QoS qos, bool retain, const SubscribeCallback &cb, void *cbdata,
      const std::string &correlation)
{
    return pImpl->PublishWithReply(topic, data, datalen, protocol, qos, retain, cb, cbdata,
          correlation);
}

int AITT::PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, AITT::QoS qos, bool retain, const SubscribeCallback &cb, void *cbdata,
      const std::string &correlation, int timeout_ms)
{
    return pImpl->PublishWithReplySync(topic, data, datalen, protocol, qos, retain, cb, cbdata,
          correlation, timeout_ms);
}

AittSubscribeID AITT::Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata,
      AittProtocol protocols, AITT::QoS qos)
{
    return pImpl->Subscribe(topic, cb, cbdata, protocols, qos);
}

void *AITT::Unsubscribe(AittSubscribeID handle)
{
    return pImpl->Unsubscribe(handle);
}

void AITT::SendReply(MSG *msg, const void *data, const int datalen, bool end)
{
    return pImpl->SendReply(msg, data, datalen, end);
}

bool AITT::CompareTopic(const std::string &left, const std::string &right)
{
    return MQ::CompareTopic(left, right);
}

}  // namespace aitt
