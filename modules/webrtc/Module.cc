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

#include "Config.h"
#include "aitt_internal.h"

Module::Module(const std::string &ip, AittDiscovery &discovery) : AittTransport(discovery)
{
}

Module::~Module(void)
{
}

void Module::Publish(const std::string &topic, const void *data, const size_t datalen,
      const std::string &correlation, AittQoS qos, bool retain)
{
    // TODO
}

void Module::Publish(const std::string &topic, const void *data, const size_t datalen, AittQoS qos,
      bool retain)
{
    std::lock_guard<std::mutex> publish_table_lock(publish_table_lock_);

    auto config = BuildConfigFromFb(data, datalen);
    if (config.GetUserDataLength()) {
        publish_table_[topic] =
              std::make_shared<PublishStream>(topic, BuildConfigFromFb(data, datalen));

        publish_table_[topic]->Start();
    } else {
        auto publish_table_itr = publish_table_.find(topic);
        if (publish_table_itr == publish_table_.end()) {
            ERR("%s not found", topic.c_str());
            return;
        }
        auto publish_stream = publish_table_itr->second;
        publish_stream->Stop();
        publish_table_.erase(publish_table_itr);
    }
}

void *Module::Subscribe(const std::string &topic, const AittTransport::SubscribeCallback &cb,
      void *cbdata, AittQoS qos)
{
    return nullptr;
}

void *Module::Subscribe(const std::string &topic, const AittTransport::SubscribeCallback &cb,
      const void *data, const size_t datalen, void *cbdata, AittQoS qos)
{
    std::lock_guard<std::mutex> subscribe_table_lock(subscribe_table_lock_);

    subscribe_table_[topic] =
          std::make_shared<SubscribeStream>(topic, BuildConfigFromFb(data, datalen));

    subscribe_table_[topic]->Start(qos == AITT_QOS_EXACTLY_ONCE, cbdata);

    return subscribe_table_[topic].get();
}

Config Module::BuildConfigFromFb(const void *data, const size_t data_size)
{
    Config config;
    auto webrtc_configs =
          flexbuffers::GetRoot(static_cast<const uint8_t *>(data), data_size).AsMap();
    auto webrtc_config_keys = webrtc_configs.Keys();
    for (size_t idx = 0; idx < webrtc_config_keys.size(); ++idx) {
        std::string key = webrtc_config_keys[idx].AsString().c_str();

        if (key.compare("Id") == 0)
            config.SetLocalId(webrtc_configs[key].AsString().c_str());
        else if (key.compare("RoomId") == 0)
            config.SetRoomId(webrtc_configs[key].AsString().c_str());
        else if (key.compare("SourceId") == 0)
            config.SetSourceId(webrtc_configs[key].AsString().c_str());
        else if (key.compare("BrokerIp") == 0)
            config.SetBrokerIp(webrtc_configs[key].AsString().c_str());
        else if (key.compare("BrokerPort") == 0)
            config.SetBrokerPort(webrtc_configs[key].AsInt32());
        else if (key.compare("UserDataLength") == 0)
            config.SetUserDataLength(webrtc_configs[key].AsUInt32());
        else {
            printf("Not supported key name: %s\n", key.c_str());
        }
    }

    return config;
}

void *Module::Unsubscribe(void *handlePtr)
{
    void *ret = nullptr;
    std::string topic;
    std::lock_guard<std::mutex> subscribe_table_lock(subscribe_table_lock_);
    for (auto itr = subscribe_table_.begin(); itr != subscribe_table_.end(); ++itr) {
        if (itr->second.get() == handlePtr) {
            auto topic = itr->first;
            break;
        }
    }

    if (topic.size() != 0) {
        ret = subscribe_table_[topic]->Stop();
        subscribe_table_.erase(topic);
    }

    return ret;
}
