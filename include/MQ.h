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

#include <mosquitto.h>

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "MSG.h"

#define MQTT_LOCALHOST "127.0.0.1"
#define MQTT_PORT 1883
#define EMPTY_WILL ""

namespace aitt {

class MQ {
  public:
    enum QoS : int {
        AT_MOST_ONCE = 0,   // Fire and forget
        AT_LEAST_ONCE = 1,  // Receiver is able to receive multiple times
        EXACTLY_ONCE = 2,   // Receiver only receives exactly once
    };
    using SubscribeCallback =
          std::function<void(MSG *, const std::string &topic, const void *, const int, void *)>;

    explicit MQ(const std::string &id, bool clear_session = false);
    virtual ~MQ(void);

    static bool CompareTopic(const std::string &left, const std::string &right);

    void Connect(const std::string &host = MQTT_LOCALHOST, int port = MQTT_PORT,
          const std::string &willtopic = EMPTY_WILL, const void *willmsg = nullptr,
          size_t szmsg = 0, MQ::QoS qos = QoS::AT_MOST_ONCE, bool retain = false);
    void Disconnect(void);
    void Publish(const std::string &topic, const void *data, const size_t datalen,
          MQ::QoS qos = QoS::AT_MOST_ONCE, bool retain = false);
    void PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
          MQ::QoS qos, bool retain, const std::string &reply_topic, const std::string &correlation);
    void SendReply(MSG *msg, const void *data, const size_t datalen, MQ::QoS qos, bool retain);
    void *Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata = nullptr,
          MQ::QoS qos = QoS::AT_MOST_ONCE);
    void *Unsubscribe(void *handle);
    void Start(void);
    void Stop(bool force = false);

  private:
    struct SubscribeData {
        SubscribeData(const std::string topic, const SubscribeCallback &cb, void *cbdata);
        virtual ~SubscribeData(void) = default;
        std::string topic;
        SubscribeCallback cb;
        void *cbdata;
    };

    static void MQTTMessageCallback(mosquitto *, void *, const mosquitto_message *,
          const mosquitto_property *);
    void InvokeCallback(const mosquitto_message *msg, const mosquitto_property *props);

    static const std::string REPLY_SEQUENCE_NUM_KEY;
    static const std::string REPLY_IS_END_SEQUENCE_KEY;

    bool clear_session_;
    std::string mq_id;
    mosquitto *handle;
    const int keep_alive;
    std::vector<SubscribeData *> subscribers;
    std::vector<SubscribeData *>::iterator subscriber_iterator;
    bool subscriber_iterator_updated;
    std::recursive_mutex subscribers_lock;
};

}  // namespace aitt
