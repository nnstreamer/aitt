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
#include <mutex>
#include <string>
#include <vector>

#include "MQ.h"
#include "MSG.h"

#define MQTT_LOCALHOST "127.0.0.1"
#define MQTT_PORT 1883

namespace aitt {

class MosquittoMQ : public MQ {
  public:
    explicit MosquittoMQ(const std::string &id, bool clear_session = false);
    virtual ~MosquittoMQ(void);

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
    struct SubscribeData {
        SubscribeData(const std::string &topic, const SubscribeCallback &cb, void *user_data);
        std::string topic;
        SubscribeCallback cb;
        void *user_data;
    };

    static void ConnectCallback(struct mosquitto *mosq, void *obj, int rc, int flag,
          const mosquitto_property *props);
    static void DisconnectCallback(struct mosquitto *mosq, void *obj, int rc,
          const mosquitto_property *props);
    static void MessageCallback(mosquitto *, void *, const mosquitto_message *,
          const mosquitto_property *);
    void InvokeCallback(SubscribeData *subscriber, const mosquitto_message *msg,
          const mosquitto_property *props);

    static const std::string REPLY_SEQUENCE_NUM_KEY;
    static const std::string REPLY_IS_END_SEQUENCE_KEY;

    mosquitto *handle;
    const int keep_alive;
    std::vector<SubscribeData *> subscribers;
    bool subscribers_iterating;
    std::vector<SubscribeData *> new_subscribers;
    std::vector<SubscribeData *>::iterator subscriber_iterator;
    bool subscriber_iterator_updated;
    std::recursive_mutex callback_lock;
    MQConnectionCallback connect_cb;
};

}  // namespace aitt
