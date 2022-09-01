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
#include "MQProxy.h"

#include "ModuleLoader.h"
#include "MosquittoMQ.h"
#include "aitt_internal.h"

namespace aitt {

MQProxy::MQProxy(const std::string &id, const AittOption &option) : handle(nullptr, nullptr)
{
    if (option.GetUseCustomMqttBroker()) {
        ModuleLoader loader;
        handle = loader.OpenModule(ModuleLoader::TYPE_CUSTOM_MQTT);

        mq = loader.LoadMqttClient(handle.get(), id, option);
        INFO("Custom MQ(%p)", mq.get());
    } else {
        mq = std::unique_ptr<MQ>(new MosquittoMQ(id, option.GetClearSession()));
        INFO("Mosquitto MQ");
    }
}

void MQProxy::SetConnectionCallback(const MQConnectionCallback &cb)
{
    mq->SetConnectionCallback(cb);
}

void MQProxy::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    mq->Connect(host, port, username, password);
}

void MQProxy::SetWillInfo(const std::string &topic, const void *msg, size_t szmsg, int qos,
      bool retain)
{
    mq->SetWillInfo(topic, msg, szmsg, qos, retain);
}

void MQProxy::Disconnect(void)
{
    mq->Disconnect();
}

void MQProxy::Publish(const std::string &topic, const void *data, const size_t datalen, int qos,
      bool retain)
{
    mq->Publish(topic, data, datalen, qos, retain);
}

void MQProxy::PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
      int qos, bool retain, const std::string &reply_topic, const std::string &correlation)
{
    mq->PublishWithReply(topic, data, datalen, qos, retain, reply_topic, correlation);
}

void MQProxy::SendReply(MSG *msg, const void *data, const size_t datalen, int qos, bool retain)
{
    mq->SendReply(msg, data, datalen, qos, retain);
}

void *MQProxy::Subscribe(const std::string &topic, const SubscribeCallback &cb, void *user_data,
      int qos)
{
    return mq->Subscribe(topic, cb, user_data, qos);
}

void *MQProxy::Unsubscribe(void *handle)
{
    return mq->Unsubscribe(handle);
}

}  // namespace aitt
