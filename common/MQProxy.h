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

#include <memory>

#include "MQ.h"

namespace aitt {

class MQProxy : public MQ {
  public:
    explicit MQProxy(const std::string &id, bool clear_session, bool custom_broker);
    virtual ~MQProxy() = default;

    void SetConnectionCallback(const MQConnectionCallback &cb);
    void Connect(const std::string &host, int port, const std::string &username,
          const std::string &password);
    void SetWillInfo(const std::string &topic, const void *msg, size_t szmsg, int qos, bool retain);
    void Disconnect(void);
    void Publish(const std::string &topic, const void *data, const size_t datalen, int qos = 0,
          bool retain = false);
    void PublishWithReply(const std::string &topic, const void *data, const size_t datalen, int qos,
          bool retain, const std::string &reply_topic, const std::string &correlation);
    void SendReply(MSG *msg, const void *data, const size_t datalen, int qos, bool retain);
    void *Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *user_data = nullptr, int qos = 0);
    void *Unsubscribe(void *handle);

  private:
    std::unique_ptr<void, void (*)(const void *)> handle;
    std::shared_ptr<MQ> mq;
};

}  // namespace aitt
