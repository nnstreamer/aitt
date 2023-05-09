/*
 * Copyright 2021-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

bool AittDiscovery::HasValidMQ(void)
{
    return discovery_mq != nullptr;
}

void AittDiscovery::Start(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    RET_IF(callback_handle);

    discovery_mq->SetWillInfo(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_EXACTLY_ONCE, true);
    discovery_mq->SetConnectionCallback([&](int status) {
        if (status != AITT_CONNECTED)
            return;

        DBG("Discovery Connected");
        callback_handle = discovery_mq->Subscribe(DISCOVERY_TOPIC_BASE + "+",
              DiscoveryMessageCallback, static_cast<void *>(this), AITT_QOS_EXACTLY_ONCE);
        discovery_mq->SetConnectionCallback(nullptr);
    });
    discovery_mq->Connect(host, port, username, password);
}

void AittDiscovery::Restart()
{
    RET_IF(callback_handle == nullptr);
    discovery_mq->Unsubscribe(callback_handle);
    callback_handle = discovery_mq->Subscribe(DISCOVERY_TOPIC_BASE + "+", DiscoveryMessageCallback,
          static_cast<void *>(this), AITT_QOS_EXACTLY_ONCE);
}

void AittDiscovery::Stop()
{
    discovery_mq->SetConnectionCallback(nullptr);
    discovery_mq->Unsubscribe(callback_handle);
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_EXACTLY_ONCE, true);
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, nullptr, 0, AITT_QOS_AT_MOST_ONCE, true);
    callback_handle = nullptr;
    discovery_mq->Disconnect();
}

void AittDiscovery::UpdateDiscoveryMsg(const std::string &protocol, const void *msg, int length)
{
    auto it = discovery_map.find(protocol);
    if (it == discovery_map.end())
        discovery_map.emplace(protocol, DiscoveryBlob(msg, length));
    else
        it->second = DiscoveryBlob(msg, length);

    PublishDiscoveryMsg();
}

int AittDiscovery::AddDiscoveryCB(const std::string &protocol, const DiscoveryCallback &cb)
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
        return;
    }
    callbacks.erase(it);
}

bool AittDiscovery::CompareTopic(const std::string &left, const std::string &right)
{
    return discovery_mq->CompareTopic(left, right);
}

void AittDiscovery::DiscoveryMessageCallback(AittMsg *info, const void *msg, const int szmsg,
      void *user_data)
{
    RET_IF(user_data == nullptr);
    RET_IF(info == nullptr);

    AittDiscovery *discovery = static_cast<AittDiscovery *>(user_data);

    size_t end = info->GetTopic().find("/", DISCOVERY_TOPIC_BASE.length());
    std::string clientId = info->GetTopic().substr(DISCOVERY_TOPIC_BASE.length(), end);
    if (clientId.empty()) {
        ERR("ClientId is empty");
        return;
    }

    if (msg == nullptr) {
        for (const auto &node : discovery->callbacks) {
            std::pair<std::string, DiscoveryCallback> cb_info = node.second;
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
            std::pair<std::string, DiscoveryCallback> cb_info = node.second;
            if (cb_info.first == key) {
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

        for (const std::pair<const std::string &, const DiscoveryBlob &> node : discovery_map) {
            fbb.Key(node.first);
            fbb.Blob(node.second.data.get(), node.second.len);
        }
    });

    fbb.Finish();

    auto buf = fbb.GetBuffer();
    discovery_mq->Publish(DISCOVERY_TOPIC_BASE + id_, buf.data(), buf.size(), AITT_QOS_EXACTLY_ONCE,
          true);
}

AittDiscovery::DiscoveryBlob::DiscoveryBlob(const void *msg, int length)
      : len(length), data(new char[len], [](char *p) { delete[] p; })
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
