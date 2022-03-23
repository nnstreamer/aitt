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
#include "Module.h"

#include <unistd.h>

#include "Definitions.h"
#include "log.h"

/*
 * P2P Data Packet Definition
 * TopicLength: 4 bytes
 * TopicString: $TopicLength
 */

Module::Module(const std::string &ip) : discoveryMessage(nullptr), ip(ip)
{
    aittThread = std::thread(&Module::ThreadMain, this);
}

Module::~Module(void)
{
    while (main_loop.Quit() == false) {
        // wait when called before the thread has completely created.
        usleep(1000);
    }

    if (aittThread.joinable())
        aittThread.join();

    free(discoveryMessage);
}

void Module::ThreadMain(void)
{
    pthread_setname_np(pthread_self(), "TCPWorkerLoop");
    main_loop.Run();
}

void Module::Publish(const std::string &topic, const void *data, const size_t datalen,
      const std::string &correlation, AITT::QoS qos, bool retain)
{
    // NOTE:
    // Iterate discovered service table
    // PublishMap
    // map {
    //    "/customTopic/faceRecog": map {
    //       "$clientId": map {
    //          11234: $handle,
    //
    //          ...
    //
    //          21234: nullptr,
    //       },
    //    },
    // }
    std::lock_guard<std::mutex> auto_lock_publish(publishTableLock);
    for (PublishMap::iterator it = publishTable.begin(); it != publishTable.end(); ++it) {
        // NOTE:
        // Find entries that have matched with the given topic
        if (!AITT::CompareTopic(it->first, topic))
            continue;

        // NOTE:
        // Iterate all hosts
        for (HostMap::iterator hostIt = it->second.begin(); hostIt != it->second.end(); ++hostIt) {
            // Iterate all ports,
            // the current implementation only be able to have the ZERO or a SINGLE entry
            // hostIt->first // clientId
            for (PortMap::iterator portIt = hostIt->second.begin(); portIt != hostIt->second.end();
                  ++portIt) {
                // portIt->first // port
                // portIt->second // handle
                if (!portIt->second) {
                    std::string host;
                    {
                        ClientMap::iterator clientIt;
                        std::lock_guard<std::mutex> auto_lock_client(clientTableLock);

                        clientIt = clientTable.find(hostIt->first);
                        if (clientIt != clientTable.end())
                            host = clientIt->second;

                        // NOTE:
                        // otherwise, it is a critical error
                        // The broken clientTable or subscribeTable
                    }

                    std::unique_ptr<TCP> client(std::make_unique<TCP>(host, portIt->first));

                    // TODO:
                    // If the client gets disconnected,
                    // This channel entry must be cleared
                    // In order to do that,
                    // There should be an observer to monitor
                    // each connections and manipulate
                    // the discovered service table

                    // NOTE:
                    // At the first time, the connection is established,
                    // The publisher must send the metadata for the connection

                    // Send the topic string
                    uint32_t topicLen = topic.length() + 1;
                    size_t szData = sizeof(topicLen);
                    client->Send(static_cast<void *>(&topicLen), szData);
                    szData = topicLen;
                    client->Send(static_cast<const void *>(topic.c_str()), szData);
                    portIt->second = std::move(client);
                }

                // NOTE:
                // If the protocol is not supported,
                // the channel would be nullptr then skip
                if (!portIt->second) {
                    ERR("Failed to create a new client instance");
                    continue;
                }

                // NOTE:
                // Send a message
                //
                // If possible, all protocol should
                // be inherited from a common interface
                // which defines Send and Recv methods
                uint32_t sendsize = datalen;
                size_t szsize = sizeof(sendsize);

                try {
                    portIt->second->Send(static_cast<void *>(&sendsize), szsize);

                    int msgSize = sendsize;
                    while (msgSize > 0) {
                        size_t sentSize = msgSize;
                        char *dataIdx = (char *)data + (sendsize - msgSize);
                        portIt->second->Send(dataIdx, sentSize);
                        if (sentSize > 0) {
                            msgSize -= sentSize;
                        }
                    }
                } catch (std::exception &e) {
                    ERR("An exception(%s) occurs during Send().", e.what());
                }
            }
        }  // connectionEntries
    }      // publishTable
}

void Module::Publish(const std::string &topic, const void *data, const size_t datalen,
      AITT::QoS qos, bool retain)
{
    Publish(topic, data, datalen, std::string(), qos, retain);
}

void *Module::Subscribe(const std::string &topic, const TransportModule::SubscribeCallback &cb,
      void *cbdata, AITT::QoS qos)
{
    std::unique_ptr<TCP::Server> tcpServer;

    unsigned short port = 0;
    tcpServer = std::make_unique<TCP::Server>("0.0.0.0", port);
    TCPServerData *listen_info = new TCPServerData;
    listen_info->impl = this;
    listen_info->cb = cb;
    listen_info->cbdata = cbdata;
    listen_info->topic = topic;
    auto handle = tcpServer->GetHandle();

    main_loop.AddWatch(handle, AcceptConnection, listen_info);

    // 서비스 테이블에 토픽을 키워드로 프로토콜을 등록한다.
    {
        std::lock_guard<std::mutex> autoLock(subscribeTableLock);
        subscribeTable.insert(SubscribeMap::value_type(topic, std::move(tcpServer)));
    }

    return reinterpret_cast<void *>(handle);
}

void *Module::Unsubscribe(void *handlePtr)
{
    int handle = static_cast<int>(reinterpret_cast<intptr_t>(handlePtr));
    TCPServerData *listen_info = dynamic_cast<TCPServerData *>(main_loop.RemoveWatch(handle));
    if (!listen_info)
        return nullptr;

    {
        std::lock_guard<std::mutex> autoLock(subscribeTableLock);
        auto it = subscribeTable.find(listen_info->topic);
        if (it == subscribeTable.end())
            throw std::runtime_error("Service is not registered: " + listen_info->topic);

        subscribeTable.erase(it);
    }

    void *cbdata = listen_info->cbdata;
    listen_info->client_lock.lock();
    for (auto fd : listen_info->client_list) {
        TCPData *connect_info = dynamic_cast<TCPData *>(main_loop.RemoveWatch(fd));
        delete connect_info;
    }
    listen_info->client_list.clear();
    listen_info->client_lock.unlock();
    delete listen_info;

    return cbdata;
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    // NOTE:
    // Iterate discovered service table
    // PublishMap
    // map {
    //    "/customTopic/faceRecog": map {
    //       "clientId.uniq.abcd.123": map {
    //          11234: pair {
    //             "protocol": 1,
    //             "handle": nullptr,
    //          },
    //
    //          ...
    //
    //          21234: pair {
    //             "protocol": 2,
    //             "handle": nullptr,
    //          }
    //       },
    //    },
    // }
    if (!status.compare(AITT::WILL_LEAVE_NETWORK)) {
        {
            std::lock_guard<std::mutex> autoLock(clientTableLock);
            // Delete from the { clientId : Host } mapping table
            clientTable.erase(clientId);
        }

        {
            // NOTE:
            // Iterate all topics in the publishTable holds discovered client information
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            for (auto it = publishTable.begin(); it != publishTable.end(); ++it)
                it->second.erase(clientId);
        }
        return;
    }

    // serviceMessage (flexbuffers)
    // map {
    //   "host": "192.168.1.11",
    //   "$topic": port,
    // }
    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();
    std::string host = map["host"].AsString().c_str();

    // NOTE:
    // Update the clientTable
    {
        std::lock_guard<std::mutex> autoLock(clientTableLock);
        auto clientIt = clientTable.find(clientId);
        if (clientIt == clientTable.end())
            clientTable.insert(ClientMap::value_type(clientId, host));
        else if (clientIt->second.compare(host))
            clientIt->second = host;
    }

    auto topics = map.Keys();
    for (size_t idx = 0; idx < topics.size(); ++idx) {
        std::string topic = topics[idx].AsString().c_str();

        if (!topic.compare("host"))
            continue;

        auto port = map[topic].AsUInt16();

        {
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            UpdatePublishTable(topic, clientId, port);
        }
    }
}

void Module::GetDiscoveryMessage(const void *&msg, int &szmsg)
{
    flexbuffers::Builder fbb;
    // flexbuffers
    // {
    //   "host": "127.0.0.1",
    //   "/customTopic/aitt/faceRecog": $port,
    //   "/customTopic/aitt/ASR": 102020,
    //
    //   ...
    //
    //   "/customTopic/aitt/+": 20123,
    // }
    fbb.Map([this, &fbb]() {
        fbb.String("host", ip);

        // SubscribeTable
        // map {
        //    "/customTopic/mytopic": $serverHandle,
        //    ...
        // }
        for (auto it = subscribeTable.begin(); it != subscribeTable.end(); ++it) {
            if (it->second)
                fbb.UInt(it->first.c_str(), it->second->GetPort());
            else
                fbb.UInt(it->first.c_str(), 0);  // this is an error case
        }
    });
    fbb.Finish();

    auto buf = fbb.GetBuffer();

    free(discoveryMessage);

    szmsg = buf.size();
    discoveryMessage = malloc(szmsg);
    memcpy(discoveryMessage, buf.data(), szmsg);

    msg = discoveryMessage;
}

AittProtocol Module::GetProtocol(void)
{
    return AITT_TYPE_TCP;
}

void Module::ReceiveData(MainLoopHandler::MainLoopResult result, int handle,
      MainLoopHandler::MainLoopData *user_data)
{
    TCPData *connect_info = dynamic_cast<TCPData *>(user_data);
    RET_IF(connect_info == nullptr);
    TCPServerData *parent_info = connect_info->parent;
    RET_IF(parent_info == nullptr);
    Module *impl = parent_info->impl;
    RET_IF(impl == nullptr);

    if (result == MainLoopHandler::HANGUP) {
        ERR("Disconnected");
        return impl->HandleClientDisconnect(handle);
    }

    if (connect_info->topic.empty()) {
        impl->EstablishConnection(connect_info, handle);
    } else {
        // ReceiveData
        // TODO:
        // Need to build the TCP data protocol
        uint32_t szmsg = 0;
        size_t szdata = sizeof(szmsg);
        char *msg = nullptr;

        try {
            connect_info->client->Recv(static_cast<void *>(&szmsg), szdata);

            if (szmsg == 0) {
                ERR("Disconnected");
                return impl->HandleClientDisconnect(handle);
            }

            msg = static_cast<char *>(malloc(szmsg));
            int msgSize = szmsg;
            while (0 < msgSize) {
                size_t receivedSize = msgSize;
                connect_info->client->Recv(static_cast<void *>(msg + (szmsg - msgSize)),
                      receivedSize);
                if (receivedSize > 0) {
                    msgSize -= receivedSize;
                }
            }
        } catch (std::exception &e) {
            ERR("An exception(%s) occurs during Recv()", e.what());
        }

        std::string correlation;
        // TODO:
        // Correlation data (string) should be filled

        parent_info->cb(connect_info->topic, msg, szmsg, parent_info->cbdata, correlation);
        free(msg);
    }
}

void Module::HandleClientDisconnect(int handle)
{
    TCPData *connect_info = dynamic_cast<TCPData *>(main_loop.RemoveWatch(handle));
    if (connect_info == nullptr) {
        ERR("No watch data");
        return;
    }
    connect_info->parent->client_lock.lock();
    auto it = std::find(connect_info->parent->client_list.begin(),
          connect_info->parent->client_list.end(), handle);
    connect_info->parent->client_list.erase(it);
    connect_info->parent->client_lock.unlock();

    delete connect_info;
}

void Module::EstablishConnection(Module::TCPData *connect_info, int handle)
{
    // First, the client should send a topic string.
    uint32_t topic_len = 0;
    size_t data_size = sizeof(topic_len);
    connect_info->client->Recv(static_cast<void *>(&topic_len), data_size);

    if (AITT_TOPIC_NAME_MAX < topic_len) {
        ERR("Invalid topic name length(%d)", topic_len);
        return;
    }

    char data[topic_len];
    data_size = topic_len;
    connect_info->client->Recv(data, data_size);
    if (data_size != topic_len)
        ERR("Recv() Fail");

    connect_info->topic = std::string(data, data_size);
}

void Module::AcceptConnection(MainLoopHandler::MainLoopResult result, int handle,
      MainLoopHandler::MainLoopData *user_data)
{
    // TODO:
    // Update the discovery map
    std::unique_ptr<TCP> client;

    TCPServerData *listen_info = dynamic_cast<TCPServerData *>(user_data);
    Module *impl = listen_info->impl;
    {
        std::lock_guard<std::mutex> autoLock(impl->subscribeTableLock);

        auto clientIt = impl->subscribeTable.find(listen_info->topic);
        if (clientIt == impl->subscribeTable.end())
            return;

        client = clientIt->second->AcceptPeer();
    }

    if (client == nullptr) {
        ERR("Unable to accept a peer");  // NOTE: FATAL ERROR
        return;
    }

    int cHandle = client->GetHandle();
    listen_info->client_list.push_back(cHandle);

    TCPData *ecd = new TCPData;
    ecd->parent = listen_info;
    ecd->client = std::move(client);

    impl->main_loop.AddWatch(cHandle, ReceiveData, ecd);
}

void Module::UpdatePublishTable(const std::string &topic, const std::string &clientId,
      unsigned short port)
{
    auto topicIt = publishTable.find(topic);
    if (topicIt == publishTable.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(port, nullptr));
        HostMap hostMap;
        hostMap.insert(HostMap::value_type(clientId, std::move(portMap)));
        publishTable.insert(PublishMap::value_type(topic, std::move(hostMap)));
        return;
    }

    auto hostIt = topicIt->second.find(clientId);
    if (hostIt == topicIt->second.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(port, nullptr));
        topicIt->second.insert(HostMap::value_type(clientId, std::move(portMap)));
        return;
    }

    // NOTE:
    // The current implementation only has a single port entry
    // therefore, if the hostIt is not empty, there is the previous connection
    if (!hostIt->second.empty()) {
        auto portIt = hostIt->second.begin();

        if (portIt->first == port)
            return;  // nothing changed. keep the current handle

        // otherwise, delete the connection handle
        // to make a new connection with the new port
        hostIt->second.clear();
    }

    hostIt->second.insert(PortMap::value_type(port, nullptr));
}
