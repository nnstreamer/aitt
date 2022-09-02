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
#pragma once

#include <AittException.h>
#include <AittOption.h>
#include <AittTypes.h>
#include <MSG.h>

#include <functional>
#include <memory>
#include <string>

#define AITT_LOCALHOST "127.0.0.1"
#define AITT_PORT 1883

namespace aitt {

class API AITT {
  public:
    using SubscribeCallback =
          std::function<void(MSG *msg, const void *data, const size_t datalen, void *user_data)>;
    using ConnectionCallback = std::function<void(AITT &, int, void *user_data)>;

    explicit AITT(const std::string &id, const std::string &ip_addr,
          AittOption option = AittOption(false, false));
    virtual ~AITT(void);

    void SetWillInfo(const std::string &topic, const void *data, const size_t datalen, AittQoS qos,
          bool retain);
    void SetConnectionCallback(ConnectionCallback cb, void *user_data = nullptr);
    void Connect(const std::string &host = AITT_LOCALHOST, int port = AITT_PORT,
          const std::string &username = std::string(), const std::string &password = std::string());
    void Disconnect(void);

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocols = AITT_TYPE_MQTT, AittQoS qos = AITT_QOS_AT_MOST_ONCE,
          bool retain = false);

    int PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);

    int PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms = 0);

    AittSubscribeID Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AittProtocol protocol = AITT_TYPE_MQTT,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE);

    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(MSG *msg, const void *data, const size_t datalen, bool end = true);

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace aitt
