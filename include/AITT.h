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

#include <functional>
#include <memory>
#include <string>

#include <MSG.h>
#include <AittTypes.h>

#define AITT_LOCALHOST "127.0.0.1"
#define AITT_PORT 1883

namespace aitt {

class API AITT {
  public:
    // NOTE:
    // This QoS only works with the AITT_TYPE_MQTT
    enum QoS : int {
        AT_MOST_ONCE = 0,   // Fire and forget
        AT_LEAST_ONCE = 1,  // Receiver is able to receive multiple times
        EXACTLY_ONCE = 2,   // Receiver only receives exactly once
    };

    static constexpr const char *WILL_LEAVE_NETWORK = "disconnected";
    static constexpr const char *JOIN_NETWORK = "connected";

    using SubscribeCallback = std::function<void(MSG *, const void *, const size_t, void *)>;

  public:
    explicit AITT(const std::string &id, const std::string &ip_addr, bool clear_session = false);
    virtual ~AITT(void);

    void Connect(const std::string &host = AITT_LOCALHOST, int port = AITT_PORT);
    void Disconnect(void);

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocols = AITT_TYPE_MQTT, AITT::QoS qos = QoS::AT_MOST_ONCE,
          bool retain = false);
    int PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AITT::QoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);

    int PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AITT::QoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms = 0);

    AittSubscribeID Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *cbdata = nullptr, AittProtocol protocol = AITT_TYPE_MQTT,
          AITT::QoS qos = QoS::AT_MOST_ONCE);
    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(MSG *msg, const void *data, const int datalen, bool end = true);

    // NOTE:
    // Provide utility functions to developers who only be able to access the AITT class
  public:
    static bool CompareTopic(const std::string &left, const std::string &right);

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace aitt
