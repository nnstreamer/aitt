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

#include <AittTransport.h>
#include <MainLoopIface.h>
#include <flatbuffers/flexbuffers.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "TCPServer.h"

using AittTransport = aitt::AittTransport;
using MainLoopIface = aitt::MainLoopIface;
using AittDiscovery = aitt::AittDiscovery;

#define MODULE_NAMESPACE AittTCPNamespace
namespace AittTCPNamespace {

class Module : public AittTransport {
  public:
    explicit Module(AittProtocol type, AittDiscovery &manager, const std::string &ip);
    virtual ~Module(void);

    void Publish(const std::string &topic, const void *data, const int datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false) override;

    void *Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata = nullptr,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE) override;
    void *Unsubscribe(void *handle) override;
    void PublishWithReply(const std::string &topic, const void *data, const int datalen,
          AittQoS qos, bool retain, const std::string &reply_topic, const std::string &correlation);
    void SendReply(AittMsg *msg, const void *data, const int datalen, AittQoS qos, bool retain);
    int CountSubscriber(const std::string &topic);

  private:
    using Subscribe_CB_Info = std::pair<SubscribeCallback, void *>;

    struct TCPServerData : public MainLoopIface::MainLoopData {
        Module *impl;
        std::vector<std::unique_ptr<Subscribe_CB_Info>> cb_list;
        std::string topic;
        std::vector<int> client_list;
    };

    struct TCPData : public MainLoopIface::MainLoopData {
        TCPServerData *parent;
        std::unique_ptr<TCP> client;
    };

    // SubscribeTable
    // map {
    //    "/customTopic/mytopic": $serverHandle,
    //    ...
    // }
    using SubscribeMap = std::map<TCPServerData *, std::unique_ptr<TCP::Server>>;
    using SubscribeHandles = std::map<Subscribe_CB_Info *, TCPServerData *>;

    // ClientTable
    // map {
    //   $clientId: $host,
    //   "client.uniqId.123": "192.168.1.11"
    //   ...
    // }
    using ClientMap = std::map<std::string /* id */, std::string /* host */>;

    // PublishTable
    // map {
    //    "/customTopic/faceRecog": map {
    //       $clientId: pair { 11234: $clientHandle } //one topic has one port for each client.
    //       ...
    //       },
    //    },
    // }
    using PortInfo = std::pair<TCP::ConnectInfo /* port */, std::unique_ptr<TCP>>;
    using HostMap = std::map<std::string /* clientId */, PortInfo>;
    using PublishMap = std::map<std::string /* topic */, HostMap>;

    static int AcceptConnection(MainLoopIface::Event result, int handle,
          MainLoopIface::MainLoopData *watchData);
    void PublishFull(const AittMsg &msg, const void *data, const int datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false, bool is_reply = false);
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg);
    void UpdateDiscoveryMsg();
    static int ReceiveData(MainLoopIface::Event result, int handle,
          MainLoopIface::MainLoopData *watchData);
    int HandleClientDisconnect(int handle);
    void GetMsgInfo(AittMsg &msg, TCPData *connect_info);
    void ThreadMain(void);
    void UpdatePublishTable(const std::string &topic, const std::string &host,
          const TCP::ConnectInfo &info);
    void PackMsgInfo(flexbuffers::Builder &fbb, const AittMsg &msg, bool is_reply = false);
    void UnpackMsgInfo(AittMsg &msg, const void *data, const size_t datalen);

    const char *const NAME[2] = {"TCP", "SECURE_TCP"};
    std::unique_ptr<MainLoopIface> main_loop;
    std::thread aittThread;
    int discovery_cb;

    PublishMap publishTable;
    std::mutex publishTableLock;
    SubscribeMap subscribeTable;
    SubscribeHandles subscribe_handles;
    std::mutex subscribeTableLock;
    ClientMap clientTable;
    std::mutex clientTableLock;
    std::string ip;
    bool secure;
};

}  // namespace AittTCPNamespace
