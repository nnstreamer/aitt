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
#include "AittDiscovery.h"

#include <flatbuffers/flexbuffers.h>

#include <atomic>

#include "AittException.h"
#include "aitt_internal.h"

namespace aitt {

AittDiscovery::AittDiscovery(const std::string &id) : id_(id), callback_handle(nullptr)
{
}

void AittDiscovery::SetMQ(std::unique_ptr<MQ> mq)
{
    discovery_mq = std::move(mq);
}

void AittDiscovery::Start(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    RET_IF(callback_handle);

    discovery_mq->SetWillInfo(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_EXACTLY_ONCE, true);
    discovery_mq->Connect(host, port, username, password);

    callback_handle = discovery_mq->Subscribe(DISCOVERY_TOPIC_BASE + "+", DiscoveryMessageCallback,
          static_cast<void *>(this), AITT_QOS_EXACTLY_ONCE);
}

void AittDiscovery::Stop()
{
    discovery_mq->Unsubscribe(callback_handle);
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_EXACTLY_ONCE, true);
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_AT_MOST_ONCE, true);
    callback_handle = nullptr;
    discovery_mq->Disconnect();
}

void AittDiscovery::UpdateDiscoveryMsg(AittProtocol protocol, const void *msg, size_t length)
{
    auto it = discovery_map.find(protocol);
    if (it == discovery_map.end())
        discovery_map.emplace(protocol, DiscoveryBlob(msg, length));
    else
        it->second = DiscoveryBlob(msg, length);

    PublishDiscoveryMsg();
}

int AittDiscovery::AddDiscoveryCB(AittProtocol protocol, const DiscoveryCallback &cb)
{
    static std::atomic_int id(0);
    id++;
    callbacks.emplace(id, std::make_pair(protocol, cb));

    return id;
}

void AittDiscovery::RemoveDiscoveryCB(int callback_id)
{
    auto it = callbacks.find(callback_id);
    if (it == callbacks.end()) {
        ERR("Unknown callback_id(%d)", callback_id);
        throw AittException(AittException::INVALID_ARG);
    }
    callbacks.erase(it);
}

bool AittDiscovery::CompareTopic(const std::string &left, const std::string &right)
{
    return discovery_mq->CompareTopic(left, right);
}

void AittDiscovery::DiscoveryMessageCallback(MSG *mq, const std::string &topic, const void *msg,
      const int szmsg, void *user_data)
{
    RET_IF(user_data == nullptr);

    AittDiscovery *discovery = static_cast<AittDiscovery *>(user_data);

    size_t end = topic.find("/", DISCOVERY_TOPIC_BASE.length());
    std::string clientId = topic.substr(DISCOVERY_TOPIC_BASE.length(), end);
    if (clientId.empty()) {
        ERR("ClientId is empty");
        return;
    }

    if (msg == nullptr) {
        for (const auto &node : discovery->callbacks) {
            std::pair<AittProtocol, DiscoveryCallback> cb_info = node.second;
            cb_info.second(clientId, WILL_LEAVE_NETWORK, nullptr, 0);
        }
        return;
    }

    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();
    std::string status = map["status"].AsString().c_str();

    auto keys = map.Keys();
    for (size_t idx = 0; idx < keys.size(); ++idx) {
        std::string key = keys[idx].AsString().str();

        if (!key.compare("status"))
            continue;

        auto blob = map[key].AsBlob();
        for (const auto &node : discovery->callbacks) {
            std::pair<AittProtocol, DiscoveryCallback> cb_info = node.second;
            if (cb_info.first == discovery->GetProtocol(key)) {
                cb_info.second(clientId, status, blob.data(), blob.size());
            }
        }
    }
}

void AittDiscovery::PublishDiscoveryMsg()
{
    flexbuffers::Builder fbb;

    fbb.Map([this, &fbb]() {
        fbb.String("status", JOIN_NETWORK);

        for (const std::pair<const AittProtocol, const DiscoveryBlob &> &node : discovery_map) {
            fbb.Key(GetProtocolStr(node.first));
            fbb.Blob(node.second.data.get(), node.second.len);
        }
    });

    fbb.Finish();

    auto buf = fbb.GetBuffer();
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, buf.data(), buf.size(), AITT_QOS_EXACTLY_ONCE,
          true);
}

const char *AittDiscovery::GetProtocolStr(AittProtocol protocol)
{
    switch (protocol) {
    case AITT_TYPE_MQTT:
        return "mqtt";
    case AITT_TYPE_TCP:
        return "tcp";
    case AITT_TYPE_TCP_SECURE:
        return "tcp_secure";
    case AITT_TYPE_WEBRTC:
        return "webrtc";
    default:
        ERR("Unknown protocol(%d)", protocol);
    }

    return nullptr;
}

AittProtocol AittDiscovery::GetProtocol(const std::string &protocol_str)
{
    if (STR_EQ == protocol_str.compare(GetProtocolStr(AITT_TYPE_MQTT)))
        return AITT_TYPE_MQTT;

    if (STR_EQ == protocol_str.compare(GetProtocolStr(AITT_TYPE_TCP)))
        return AITT_TYPE_TCP;

    if (STR_EQ == protocol_str.compare(GetProtocolStr(AITT_TYPE_TCP_SECURE)))
        return AITT_TYPE_TCP_SECURE;

    if (STR_EQ == protocol_str.compare(GetProtocolStr(AITT_TYPE_WEBRTC)))
        return AITT_TYPE_WEBRTC;

    return AITT_TYPE_UNKNOWN;
}

AittDiscovery::DiscoveryBlob::DiscoveryBlob(const void *msg, size_t length)
      : len(length), data(new char[len])
{
    memcpy(data.get(), msg, length);
}

AittDiscovery::DiscoveryBlob::~DiscoveryBlob()
{
}

AittDiscovery::DiscoveryBlob::DiscoveryBlob(const DiscoveryBlob &a) : len(a.len), data(a.data)
{
}

AittDiscovery::DiscoveryBlob &AittDiscovery::DiscoveryBlob::operator=(const DiscoveryBlob &src)
{
    len = src.len;
    data = src.data;
    return *this;
}

}  // namespace aitt
