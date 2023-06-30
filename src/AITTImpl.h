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

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include "AITT.h"
#include "AittDiscovery.h"
#include "AittStream.h"
#include "MQ.h"
#include "MQDiscoveryHandler.h"
#include "MainLoopIface.h"
#include "ModuleManager.h"

namespace aitt {
class AITT::Impl {
  public:
    explicit Impl(AITT &parent, const std::string &id, const std::string &my_ip,
          const AittOption &option);
    virtual ~Impl(void);

    void SetWillInfo(const std::string &topic, const void *data, const int datalen, AittQoS qos,
          bool retain);
    void SetConnectionCallback(ConnectionCallback cb, void *user_data);
    void Connect(const std::string &host, int port, const std::string &username,
          const std::string &password);
    void Disconnect(void);

    void ConfigureTransportModule(const std::string &key, const std::string &value,
          AittProtocol protocols);

    void Publish(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocols, AittQoS qos, bool retain);
    int PublishWithReply(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const AITT::SubscribeCallback &cb,
          void *cbdata, const std::string &correlation);
    int PublishWithReplySync(const std::string &topic, const void *data, const int datalen,
          AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb,
          void *cbdata, const std::string &correlation, int timeout_ms);

    AittSubscribeID Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
          void *cbdata, AittProtocol protocols, AittQoS qos);
    void *Unsubscribe(AittSubscribeID handle);

    void SendReply(AittMsg *msg, const void *data, const int datalen, bool end);

    AittStream *CreateStream(AittStreamProtocol type, const std::string &topic,
          AittStreamRole role);
    void DestroyStream(AittStream *aitt_stream);

    int CountSubscriber(const std::string &topic, AittProtocol protocols);

  private:
    using SubscribeInfo = std::pair<AittProtocol, void *>;

    int ConnectionCB(ConnectionCallback cb, void *user_data, int status,
          MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *loop_data);
    AittSubscribeID SubscribeMQ(SubscribeInfo *info, MainLoopIface *loop_handle,
          const std::string &topic, const SubscribeCallback &cb, void *cbdata, AittQoS qos);
    int DetachedCB(SubscribeCallback cb, AittMsg mq_msg, void *data, const int datalen,
          void *cbdata, MainLoopIface::Event result, int fd,
          MainLoopIface::MainLoopData *loop_data);
    void *SubscribeTCP(SubscribeInfo *, const std::string &topic, const SubscribeCallback &cb,
          void *cbdata, AittQoS qos);

    void HandleTimeout(int timeout_ms, unsigned int &timeout_id, MainLoopIface *sync_loop,
          bool &is_timeout);
    void UnsubscribeAll();
    void ThreadMain(void);

    AITT &public_api;
    AittDiscovery discovery;
    std::unique_ptr<MainLoopIface> main_loop;
    std::thread aittThread;
    ModuleManager modules;
    MQDiscoveryHandler mq_discovery_handler;
    std::unique_ptr<MQ> mq;

    std::vector<SubscribeInfo *> subscribed_list;
    std::mutex subscribed_list_mutex_;

    std::string id_;
    std::string mqtt_broker_ip_;
    int mqtt_broker_port_;
    unsigned short reply_id;

#ifdef ANDROID
    friend class AittDiscoveryHelper;
#endif
};

}  // namespace aitt
