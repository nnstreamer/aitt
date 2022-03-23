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
#include <tuple>
#include <utility>

#include "AITT.h"
#include "MQ.h"
#include "MainLoopHandler.h"
#include "TransportModuleLoader.h"

namespace aitt {

class AITT::Impl {
  public:
    Impl(AITT *parent, const std::string &id, const std::string &ipAddr, bool clearSession);
    virtual ~Impl(void);

    void Connect(const std::string &host, int port);
    void Disconnect(void);

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocols, AITT::QoS qos, bool retain);

    int PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AITT::QoS qos, bool retain, const AITT::SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);

    int PublishWithReplySync(const std::string &topic, const void *data, const size_t datalen,
          AittProtocol protocol, AITT::QoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms);

    AittSubscribeID Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
          void *cbdata, AittProtocol protocols, AITT::QoS qos);

    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(MSG *msg, const void *data, const int datalen, bool end);

  private:
    using Blob = std::pair<const void *, int>;
    using SubscribeInfo = std::pair<AittProtocol, void *>;

    static void DiscoveryMessageCallback(MSG *mq, const std::string &topic, const void *msg,
          const int szmsg, void *cbdata);
    AittSubscribeID MQSubscribe(SubscribeInfo *info, MainLoopHandler *loop_handle,
          const std::string &topic, const SubscribeCallback &cb, void *cbdata, AITT::QoS qos);
    void DetachedCB(SubscribeCallback cb, MSG mq_msg, void *data, const size_t datalen,
          void *cbdata, MainLoopHandler::MainLoopResult result, int fd,
          MainLoopHandler::MainLoopData *loop_data);
    void PublishSubscribeTable(void);
    void *SubscribeTCP(SubscribeInfo *, const std::string &topic, const SubscribeCallback &cb,
          void *cbdata, QoS qos);
    void HandleTimeout(int timeout_ms, unsigned int &timeout_id, aitt::MainLoopHandler &sync_loop,
          bool &is_timeout);

    std::string id_;
    std::unique_ptr<MQ> mq;
    AITT *parent;
    unsigned short reply_id;
    void *discoveryCallbackHandle;
    TransportModuleLoader modules;
    MainLoopHandler main_loop;
    void ThreadMain(void);
    std::thread aittThread;
    std::vector<SubscribeInfo *> subscribed_list;
    std::mutex subscribed_list_mutex_;
};

}  // namespace aitt
