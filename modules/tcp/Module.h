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

#include <AittTransport.h>
#include <MainLoopHandler.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "TCPServer.h"

using AittTransport = aitt::AittTransport;
using MainLoopHandler = aitt::MainLoopHandler;
using AittDiscovery = aitt::AittDiscovery;

#define MODULE_NAMESPACE AittTCPNamespace
namespace AittTCPNamespace {

class Module : public AittTransport {
  public:
    explicit Module(AittProtocol type, AittDiscovery &discovery, const std::string &ip);
    virtual ~Module(void);

    void Publish(const std::string &topic, const void *data, const int datalen,
          const std::string &correlation, AittQoS qos = AITT_QOS_AT_MOST_ONCE,
          bool retain = false) override;
    void Publish(const std::string &topic, const void *data, const int datalen,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE, bool retain = false) override;

    void *Subscribe(const std::string &topic, const SubscribeCallback &cb, void *cbdata = nullptr,
          AittQoS qos = AITT_QOS_AT_MOST_ONCE) override;
    void *Unsubscribe(void *handle) override;

  private:
    struct TCPServerData : public MainLoopHandler::MainLoopData {
        Module *impl;
        SubscribeCallback cb;
        void *cbdata;
        std::string topic;
        std::vector<int> client_list;
        std::mutex client_lock;
    };

    struct TCPData : public MainLoopHandler::MainLoopData {
        TCPServerData *parent;
        std::unique_ptr<TCP> client;
    };

    // SubscribeTable
    // map {
    //    "/customTopic/mytopic": $serverHandle,
    //    ...
    // }
    using SubscribeMap = std::map<std::string, std::unique_ptr<TCP::Server>>;

    // ClientTable
    // map {
    //   $clientId: $host,
    //   "client.uniqId.123": "192.168.1.11"
    //   ...
    // }
    using ClientMap = std::map<std::string /* id */, std::string /* host */>;

    // NOTE:
    // There could be multiple clientIds for the single host
    // If several applications are run on the same device, each applicaion will get unique client
    // Ids therefore we have to keep in mind that the clientId is not 1:1 matched for the IPAddress.

    // PublishTable
    // map {
    //    "/customTopic/faceRecog": map {
    //       $clientId: map {
    //          11234: $clientHandle,
    //          ...
    //          21234: $clientHandle,
    //       },
    //    },
    // }
    //
    // NOTE:
    // TCP handle should be the unique_ptr, so if we delete the entry from the map,
    // the handle must be released automatically
    // in order to make the handle "unique_ptr", it should be a class object not the "void *"
    using PortMap =
          std::map<TCP::ConnectInfo /* port */, std::unique_ptr<TCP>, TCP::ConnectInfo::Compare>;
    using HostMap = std::map<std::string /* clientId */, PortMap>;
    using PublishMap = std::map<std::string /* topic */, HostMap>;

    static int AcceptConnection(MainLoopHandler::MainLoopResult result, int handle,
          MainLoopHandler::MainLoopData *watchData);
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg);
    void UpdateDiscoveryMsg();
    static int ReceiveData(MainLoopHandler::MainLoopResult result, int handle,
          MainLoopHandler::MainLoopData *watchData);
    int HandleClientDisconnect(int handle);
    std::string GetTopicName(TCPData *connect_info);
    void ThreadMain(void);
    void UpdatePublishTable(const std::string &topic, const std::string &host,
          const TCP::ConnectInfo &info);

    const char *const NAME[2] = {"TCP", "SECURE_TCP"};
    MainLoopHandler main_loop;
    std::thread aittThread;
    int discovery_cb;

    PublishMap publishTable;
    std::mutex publishTableLock;
    SubscribeMap subscribeTable;
    std::mutex subscribeTableLock;
    ClientMap clientTable;
    std::mutex clientTableLock;
    std::string ip;
    bool secure;
};

}  // namespace AittTCPNamespace
