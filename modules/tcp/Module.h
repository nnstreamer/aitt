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

#include <AITT.h>
#include <MainLoopHandler.h>
#include <TransportModule.h>
#include <flatbuffers/flexbuffers.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "TCPServer.h"

using AITT = aitt::AITT;
using TransportModule = aitt::TransportModule;
using MainLoopHandler = aitt::MainLoopHandler;

class Module : public TransportModule {
  public:
    explicit Module(const std::string &ip);
    virtual ~Module(void);

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          const std::string &correlation, AITT::QoS qos = AITT::QoS::AT_MOST_ONCE,
          bool retain = false) override;

    void Publish(const std::string &topic, const void *data, const size_t datalen,
          AITT::QoS qos = AITT::QoS::AT_MOST_ONCE, bool retain = false) override;

    void *Subscribe(const std::string &topic, const TransportModule::SubscribeCallback &cb,
          void *cbdata = nullptr, AITT::QoS qos = AITT::QoS::AT_MOST_ONCE) override;

    void *Unsubscribe(void *handle) override;

    // NOTE:
    // The following callback is going to be called when there is a message of the discovery
    // information The callback will be called by the AITT implementation
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg) override;

    // NOTE:
    // AITT implementation could call this method to get the discovery message to broadcast it
    // through the MQTT broker
    void GetDiscoveryMessage(const void *&msg, int &szmsg) override;

    // NOTE:
    // If we are able to use a string for the protocol,
    // the module can be developed more freely.
    // even if modules based on the same protocol, implementations can be different.
    AittProtocol GetProtocol(void) override;

  private:
    struct TCPServerData : public MainLoopHandler::MainLoopData {
        Module *impl;
        TransportModule::SubscribeCallback cb;
        void *cbdata;
        std::string topic;
        std::vector<int> client_list;
        std::mutex client_lock;
    };

    struct TCPData : public MainLoopHandler::MainLoopData {
        TCPServerData *parent;
        std::string topic;
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
    //
    //          ...
    //
    //          21234: $clientHandle,
    //       },
    //    },
    // }
    //
    // NOTE:
    // TCP handle should be the unique_ptr, so if we delete the entry from the map,
    // the handle must be released automatically
    // in order to make the handle "unique_ptr", it should be a class object not the "void *"
    using PortMap = std::map<unsigned short /* port */, std::unique_ptr<TCP>>;
    using HostMap = std::map<std::string /* clientId */, PortMap>;
    using PublishMap = std::map<std::string /* topic */, HostMap>;

    static void AcceptConnection(MainLoopHandler::MainLoopResult result, int handle,
          MainLoopHandler::MainLoopData *watchData);
    static void ReceiveData(MainLoopHandler::MainLoopResult result, int handle,
          MainLoopHandler::MainLoopData *watchData);
    void HandleClientDisconnect(int handle);
    void EstablishConnection(TCPData *watchData, int handle);
    void ThreadMain(void);

    void UpdatePublishTable(const std::string &topic, const std::string &host, unsigned short port);

    MainLoopHandler main_loop;
    std::thread aittThread;
    void *discoveryMessage;
    std::string ip;

    PublishMap publishTable;
    std::mutex publishTableLock;
    SubscribeMap subscribeTable;
    std::mutex subscribeTableLock;
    ClientMap clientTable;
    std::mutex clientTableLock;
};
