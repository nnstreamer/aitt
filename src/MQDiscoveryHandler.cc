/*
 * Copyright 2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "MQDiscoveryHandler.h"

#include <flatbuffers/flexbuffers.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

#include "AITTImpl.h"
#include "aitt_internal.h"

namespace aitt {

MQDiscoveryHandler::MQDiscoveryHandler(AittDiscovery &discovery, const std::string &id)
      : discovery_(discovery), id_(id)
{
    discovery_cb = discovery_.AddDiscoveryCB("MQTT",
          std::bind(&MQDiscoveryHandler::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    DBG("ID : %s, Discovery Callback : %p, %d", id_.c_str(), this, discovery_cb);
}

MQDiscoveryHandler::~MQDiscoveryHandler(void)
{
    try {
        discovery_.RemoveDiscoveryCB(discovery_cb);
    } catch (std::exception &e) {
        ERR("RemoveDiscoveryCB() Fail(%s)", e.what());
    }
}

void MQDiscoveryHandler::UpdateDiscoveryMsg()
{
    flexbuffers::Builder fbb;

    fbb.Vector([&]() {
        for (const auto &subscribe : my_subscribe_table) {
            fbb.String(subscribe.second);
        }
    });

    fbb.Finish();
    auto buf = fbb.GetBuffer();
    discovery_.UpdateDiscoveryMsg("MQTT", buf.data(), buf.size());
}

void MQDiscoveryHandler::DiscoveryMessageCallback(const std::string &id, const std::string &status,
      const void *msg, const int szmsg)
{
    if (id == id_)
        return;

    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        std::lock_guard<std::mutex> auto_lock(remote_subscribe_table_lock);
        remote_subscribe_table.erase(id);

        return;
    }

    auto vec = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsVector();

    std::vector<std::string> topics;
    for (size_t vec_idx = 0; vec_idx < vec.size(); ++vec_idx) {
        topics.push_back(vec[vec_idx].AsString().str());
    }

    {
        std::lock_guard<std::mutex> auto_lock(remote_subscribe_table_lock);
        remote_subscribe_table[id] = topics;
    }
}

void MQDiscoveryHandler::Subscribe(AittSubscribeID handle, const std::string &topic)
{
    std::lock_guard<std::mutex> auto_lock(my_subscribe_table_lock);

    DBG("Subscribe : %p, %s", handle, topic.c_str());
    auto result = my_subscribe_table.insert(std::make_pair(handle, topic)).second;
    if (result == false) {
        throw AittException(AittException::ALREADY);
    }

    UpdateDiscoveryMsg();
}

void MQDiscoveryHandler::Unsubscribe(AittSubscribeID handle)
{
    std::lock_guard<std::mutex> auto_lock(my_subscribe_table_lock);

    DBG("Unsubscribe : %p", handle);
    if (my_subscribe_table.erase(handle) == 0) {
        throw AittException(AittException::NO_DATA_ERR);
    }

    UpdateDiscoveryMsg();
}

int MQDiscoveryHandler::CountSubscriber(const std::string &topic)
{
    int count = 0;

    std::lock_guard<std::mutex> my_auto_lock(my_subscribe_table_lock);
    for (const auto &subscribe_handle : my_subscribe_table) {
        if (discovery_.CompareTopic(subscribe_handle.second, topic)) {
            count++;
        }
    }

    std::lock_guard<std::mutex> remote_auto_lock(remote_subscribe_table_lock);
    for (const auto &remote : remote_subscribe_table) {
        for (const auto &remote_topic : remote.second) {
            if (discovery_.CompareTopic(remote_topic, topic)) {
                count++;
            }
        }
    }
    return count;
}

}  // namespace aitt
