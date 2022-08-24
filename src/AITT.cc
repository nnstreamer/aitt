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
#include <random>

#include "AITTImpl.h"
#include "aitt_internal.h"

namespace aitt {

AITT::AITT(const std::string &id, const std::string &ip_addr, AittOption option)
{
    std::string valid_id = id;
    std::string valid_ip = ip_addr;

    if (id.empty()) {
        const char character_set[] =
              "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
        std::mt19937 random_gen{std::random_device{}()};
        std::uniform_int_distribution<std::string::size_type> gen(0, 61);
        char name[16];
        for (size_t i = 0; i < sizeof(name); i++) {
            name[i] = character_set[gen(random_gen)];
        }
        valid_id = "aitt-" + std::string(name, sizeof(name) - 1);
        DBG("Generated name = %s", valid_id.c_str());
    }

    if (ip_addr.empty())
        valid_ip = "127.0.0.1";

    pImpl = std::unique_ptr<AITT::Impl>(
          new AITT::Impl(*this, valid_id, valid_ip, option.GetClearSession()));
}

AITT::~AITT(void)
{
}

void AITT::SetWillInfo(const std::string &topic, const void *data, const size_t datalen,
      AittQoS qos, bool retain)
{
    return pImpl->SetWillInfo(topic, data, datalen, qos, retain);
}

void AITT::SetConnectionCallback(ConnectionCallback cb, void *user_data)
{
    return pImpl->SetConnectionCallback(cb, user_data);
}

void AITT::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    return pImpl->Connect(host, port, username, password);
}

void AITT::Disconnect(void)
{
    return pImpl->Disconnect();
}

void AITT::Publish(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocols, AittQoS qos, bool retain)
{
    if (AITT_PAYLOAD_MAX < datalen) {
        ERR("Invalid Size(%zu)", datalen);
        throw std::runtime_error("Invalid Size");
    }

    return pImpl->Publish(topic, data, datalen, protocols, qos, retain);
}

int AITT::PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb, void *cbdata,
      const std::string &correlation)
{
    if (AITT_PAYLOAD_MAX < datalen) {
        ERR("Invalid Size(%zu)", datalen);
        throw std::runtime_error("Invalid Size");
    }

    return pImpl->PublishWithReply(topic, data, datalen, protocol, qos, retain, cb, cbdata,
          correlation);
}

int AITT::PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb, void *cbdata,
      const std::string &correlation, int timeout_ms)
{
    if (AITT_PAYLOAD_MAX < datalen) {
        ERR("Invalid Size(%zu)", datalen);
        throw std::runtime_error("Invalid Size");
    }

    return pImpl->PublishWithReplySync(topic, data, datalen, protocol, qos, retain, cb, cbdata,
          correlation, timeout_ms);
}

AittSubscribeID AITT::Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata,
      AittProtocol protocols, AittQoS qos)
{
    return pImpl->Subscribe(topic, cb, cbdata, protocols, qos);
}

void *AITT::Unsubscribe(AittSubscribeID handle)
{
    return pImpl->Unsubscribe(handle);
}

void AITT::SendReply(MSG *msg, const void *data, size_t datalen, bool end)
{
    if (AITT_PAYLOAD_MAX < datalen) {
        ERR("Invalid Size(%zu)", datalen);
        throw std::runtime_error("Invalid Size");
    }

    return pImpl->SendReply(msg, data, datalen, end);
}

}  // namespace aitt
