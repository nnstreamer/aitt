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
#pragma once

#include <AittException.h>
#include <AittMsg.h>
#include <AittOption.h>
#include <AittStream.h>
#include <AittTypes.h>

#include <functional>
#include <memory>
#include <string>

#define AITT_LOCALHOST "127.0.0.1"
#define AITT_PORT 1883
#define AITT_MUST_CALL_READY "Must Call Ready() First"

namespace aitt {

class API AITT {
  public:
    using SubscribeCallback = AittMsgCB;
    using ConnectionCallback = std::function<void(AITT &, int, void *user_data)>;

    explicit AITT(const std::string notice);
    explicit AITT(const std::string &id, const std::string &ip_addr,
          const AittOption &option = AittOption(false, false));
    virtual ~AITT(void);

    void Ready(const std::string &id, const std::string &ip_addr,
          const AittOption &option = AittOption(false, false));
    void SetWillInfo(const std::string &topic, const void *data, const int datalen, AittQoS qos,
          bool retain);
    void SetConnectionCallback(ConnectionCallback cb, void *user_data = nullptr);
    void Connect(const std::string &host = AITT_LOCALHOST, int port = AITT_PORT,
          const std::string &username = std::string(), const std::string &password = std::string());
    void Disconnect(void);

    void Publish(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocols = AITT_TYPE_MQTT, AittQoS qos = AITT_QOS_AT_MOST_ONCE,
          bool retain = false);
    void PublishWithReply(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);

    int PublishWithReplySync(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms = 0);

    AittSubscribeID Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AittProtocol protocol = AITT_TYPE_MQTT,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE);
    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(AittMsg *msg, const void *data, const int datalen, bool end = true);

    AittStream *CreateStream(AittStreamProtocol type, const std::string &topic,
          AittStreamRole role);
    void DestroyStream(AittStream *aitt_stream);
    int CountSubscriber(const std::string &topic,
          AittProtocol protocols = (AittProtocol)(AITT_TYPE_MQTT | AITT_TYPE_TCP
                                                  | AITT_TYPE_TCP_SECURE));

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

#ifdef ANDROID
    friend class AittDiscoveryHelper;
#endif
};

}  // namespace aitt
