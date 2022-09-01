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

#include <flatbuffers/flexbuffers.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "AITT.h"
#include "AittDiscovery.h"
#include "MQ.h"
#include "MainLoopHandler.h"
#include "ModuleLoader.h"

namespace aitt {

class AITT::Impl {
  public:
    explicit Impl(AITT &parent, const std::string &id, const std::string &my_ip,
          const AittOption &option);
    virtual ~Impl(void);

    void SetWillInfo(const std::string &topic, const void *data, const size_t datalen, AittQoS qos,
          bool retain);
    void SetConnectionCallback(ConnectionCallback cb, void *user_data);
    void Connect(const std::string &host, int port, const std::string &username,
          const std::string &password);
    void Disconnect(void);

    void ConfigureTransportModule(const std::string &key, const std::string &value,
          AittProtocol protocols);

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocols, AittQoS qos, bool retain);
    int PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const AITT::SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);
    int PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms);

    AittSubscribeID Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
          void *cbdata, AittProtocol protocols, AittQoS qos);
    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(MSG *msg, const void *data, const int datalen, bool end);

  private:
    using Blob = std::pair<const void *, int>;
    using SubscribeInfo = std::pair<AittProtocol, void *>;

    void ConnectionCB(ConnectionCallback cb, void *user_data, int status);
    AittSubscribeID SubscribeMQ(SubscribeInfo *info, MainLoopHandler *loop_handle,
          const std::string &topic, const SubscribeCallback &cb, void *cbdata, AittQoS qos);
    void DetachedCB(SubscribeCallback cb, MSG mq_msg, void *data, const size_t datalen,
          void *cbdata, MainLoopHandler::MainLoopResult result, int fd,
          MainLoopHandler::MainLoopData *loop_data);
    void *SubscribeTCP(SubscribeInfo *, const std::string &topic, const SubscribeCallback &cb,
          void *cbdata, AittQoS qos);
    void *SubscribeWebRtc(SubscribeInfo *, const std::string &topic, const SubscribeCallback &cb,
          void *cbdata, AittQoS qos);
    void HandleTimeout(int timeout_ms, unsigned int &timeout_id, aitt::MainLoopHandler &sync_loop,
          bool &is_timeout);
    void PublishWebRtc(const std::string &topic, const void *data, const size_t datalen,
          AittQoS qos, bool retain);
    void UnsubscribeAll();

    AITT &public_api;
    std::string id_;
    std::string mqtt_broker_ip_;
    int mqtt_broker_port_;
    std::unique_ptr<MQ> mq;
    AittDiscovery discovery;
    unsigned short reply_id;
    std::vector<ModuleLoader::ModuleHandle> module_handles;
    std::unique_ptr<AittTransport> transports[ModuleLoader::TYPE_TRANSPORT_MAX];
    MainLoopHandler main_loop;
    void ThreadMain(void);
    std::thread aittThread;
    std::vector<SubscribeInfo *> subscribed_list;
    std::mutex subscribed_list_mutex_;
};

}  // namespace aitt
