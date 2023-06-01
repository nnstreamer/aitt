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
#include "CustomMQ.h"

#ifdef TIZEN_RT

namespace aitt {

CustomMQ::CustomMQ(const std::string &id, const AittOption &option)
{
}

void CustomMQ::SetConnectionCallback(const MQConnectionCallback &cb)
{
}

void CustomMQ::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
}

void CustomMQ::SetWillInfo(const std::string &topic, const void *msg, int szmsg, int qos,
      bool retain)
{
}

void CustomMQ::Disconnect(void)
{
}

void CustomMQ::Publish(const std::string &topic, const void *data, const int datalen, int qos,
      bool retain)
{
}

void CustomMQ::PublishWithReply(const std::string &topic, const void *data, const int datalen,
      int qos, bool retain, const std::string &reply_topic, const std::string &correlation)
{
}

void CustomMQ::SendReply(AittMsg *msg, const void *data, const int datalen, int qos, bool retain)
{
}

void *CustomMQ::Subscribe(const std::string &topic, const SubscribeCallback &cb, void *user_data,
      int qos)
{
    return nullptr;
}

void *CustomMQ::Unsubscribe(void *handle)
{
    return nullptr;
}

bool CustomMQ::CompareTopic(const std::string &left, const std::string &right)
{
    return false;
}

}  // namespace aitt

#endif  // TIZEN_RT
