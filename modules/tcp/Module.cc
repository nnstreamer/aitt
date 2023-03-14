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

#include <flatbuffers/flexbuffers.h>
#include <unistd.h>

#include <random>

#include "aitt_internal.h"

namespace AittTCPNamespace {

Module::Module(AittProtocol type, AittDiscovery &discovery, const std::string &my_ip)
      : AittTransport(type, discovery), ip(my_ip), secure(type == AITT_TYPE_TCP_SECURE)
{
    aittThread = std::thread(&Module::ThreadMain, this);

    discovery_cb = discovery.AddDiscoveryCB(NAME[secure],
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    DBG("Discovery Callback : %p, %d", this, discovery_cb);
}

Module::~Module(void)
{
    try {
        discovery.RemoveDiscoveryCB(discovery_cb);
    } catch (std::exception &e) {
        ERR("RemoveDiscoveryCB() Fail(%s)", e.what());
    }

    while (main_loop.Quit() == false) {
        // wait when called before the thread has completely created.
        usleep(1000);
    }

    if (aittThread.joinable())
        aittThread.join();
}

void Module::ThreadMain(void)
{
    if (secure)
        pthread_setname_np(pthread_self(), "SecureTCPLoop");
    else
        pthread_setname_np(pthread_self(), "NormalTCPLoop");
    main_loop.Run();
}

void Module::PublishFull(const AittMsg &msg, const void *data, const int datalen, AittQoS qos,
      bool retain, bool is_reply)
{
    RET_IF(datalen < 0);

    // NOTE:
    // Iterate discovered service table
    // PublishMap
    // map {
    //    "/customTopic/faceRecog": map {
    //       "$clientId": map {
    //          11234: $handle,
    //          ...
    //          21234: nullptr,
    //       },
    //    },
    // }
    std::lock_guard<std::mutex> auto_lock_publish(publishTableLock);
    for (PublishMap::iterator it = publishTable.begin(); it != publishTable.end(); ++it) {
        // NOTE: Find entries that have matched with the given topic
        if (!discovery.CompareTopic(it->first, is_reply ? msg.GetResponseTopic() : msg.GetTopic()))
            continue;

        for (HostMap::iterator hostIt = it->second.begin(); hostIt != it->second.end(); ++hostIt) {
            // Iterate all ports,
            // the current implementation only be able to have the ZERO or a SINGLE entry
            for (PortMap::iterator portIt = hostIt->second.begin(); portIt != hostIt->second.end();
                  ++portIt) {
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

                    std::unique_ptr<TCP> client(new TCP(host, portIt->first));

                    // TODO:
                    // If the client gets disconnected,
                    // This channel entry must be cleared
                    // In order to do that,
                    // There should be an observer to monitor
                    // each connections and manipulate
                    // the discovered service table
                    portIt->second = std::move(client);
                }

                if (!portIt->second) {
                    ERR("Failed to create a new client instance");
                    continue;
                }

                try {
                    flexbuffers::Builder fbb;
                    PackMsgInfo(fbb, msg, is_reply);
                    auto buffer = fbb.GetBuffer();
                    portIt->second->SendSizedData(buffer.data(), buffer.size());

                    portIt->second->SendSizedData(data, datalen);
                } catch (std::exception &e) {
                    ERR("An exception(%s) occurs during Send().", e.what());
                }
            }
        }  // connectionEntries
    }      // publishTable
}

void Module::PackMsgInfo(flexbuffers::Builder &fbb, const AittMsg &msg, bool is_reply)
{
    fbb.Map([&]() {
        if (is_reply) {
            if (!msg.GetResponseTopic().empty())
                fbb.String("topic", msg.GetResponseTopic().c_str());
        } else {
            if (!msg.GetTopic().empty())
                fbb.String("topic", msg.GetTopic().c_str());
            if (!msg.GetResponseTopic().empty())
                fbb.String("reply_topic", msg.GetResponseTopic().c_str());
        }
        if (!msg.GetCorrelation().empty())
            fbb.String("correlation", msg.GetCorrelation().c_str());
        if (msg.GetSequence() != 0)
            fbb.UInt("sequence", msg.GetSequence());
        if (msg.IsEndSequence())
            fbb.Bool("end_sequence", msg.IsEndSequence());
    });

    fbb.Finish();
}

void Module::Publish(const std::string &topic, const void *data, const int datalen, AittQoS qos,
      bool retain)
{
    AittMsg msg;
    msg.SetTopic(topic);
    PublishFull(msg, data, datalen, qos, retain);
}

void *Module::Subscribe(const std::string &topic, const AittTransport::SubscribeCallback &cb,
      void *cbdata, AittQoS qos)
{
    std::unique_ptr<TCP::Server> tcpServer;

    unsigned short port = 0;
    tcpServer = std::unique_ptr<TCP::Server>(new TCP::Server("0.0.0.0", port, secure));
    TCPServerData *listen_info = new TCPServerData;
    listen_info->impl = this;
    listen_info->cb = cb;
    listen_info->cbdata = cbdata;
    listen_info->topic = topic;
    auto handle = tcpServer->GetHandle();

    main_loop.AddWatch(handle, AcceptConnection, listen_info);

    {
        std::lock_guard<std::mutex> autoLock(subscribeTableLock);
        subscribeTable.insert(SubscribeMap::value_type(topic, std::move(tcpServer)));
        UpdateDiscoveryMsg();
    }

    return reinterpret_cast<void *>(handle);
}

void *Module::Unsubscribe(void *handlePtr)
{
    std::lock_guard<std::mutex> autoLock(subscribeTableLock);

    int handle = static_cast<int>(reinterpret_cast<intptr_t>(handlePtr));
    TCPServerData *listen_info = dynamic_cast<TCPServerData *>(main_loop.RemoveWatch(handle));
    if (!listen_info)
        return nullptr;

    auto it = subscribeTable.find(listen_info->topic);
    if (it == subscribeTable.end())
        throw std::runtime_error("Service is not registered: " + listen_info->topic);

    subscribeTable.erase(it);

    UpdateDiscoveryMsg();

    void *cbdata = listen_info->cbdata;
    for (auto fd : listen_info->client_list) {
        TCPData *tcp_data = dynamic_cast<TCPData *>(main_loop.RemoveWatch(fd));
        delete tcp_data;
    }
    listen_info->client_list.clear();
    delete listen_info;

    return cbdata;
}

void Module::PublishWithReply(const std::string &topic, const void *data, const int datalen,
      AittQoS qos, bool retain, const std::string &reply_topic, const std::string &correlation)
{
    AittMsg msg;
    msg.SetTopic(topic);
    msg.SetResponseTopic(reply_topic);
    msg.SetCorrelation(correlation);
    PublishFull(msg, data, datalen, qos, retain);
}

void Module::SendReply(AittMsg *msg, const void *data, const int datalen, AittQoS qos, bool retain)
{
    if (msg == nullptr) {
        ERR("Invalid message(msg is nullptr)");
        throw std::runtime_error("Invalid message");
    }

    PublishFull(*msg, data, datalen, qos, retain, true);
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    // NOTE: Iterate discovered service table
    // PublishMap
    // map { topic : map { clientId : map { port : pair { "protocol": 1, "handle": nullptr } } } }
    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        {
            std::lock_guard<std::mutex> autoLock(clientTableLock);
            // Delete from the { clientId : Host } mapping table
            clientTable.erase(clientId);
        }

        {
            // NOTE: Iterate all topics in the publishTable holds discovered client information
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            for (auto it = publishTable.begin(); it != publishTable.end(); ++it)
                it->second.erase(clientId);
        }
        return;
    }

    // serviceMessage (flexbuffers)
    // map {
    //   "host": "192.168.1.11",
    //   "$topic": {port, key, iv}
    // }
    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();
    std::string host = map["host"].AsString().c_str();

    // NOTE: Update the clientTable
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

        TCP::ConnectInfo info;
        auto connectInfo = map[topic].AsVector();
        size_t vec_size = connectInfo.size();
        info.port = connectInfo[0].AsUInt16();
        if (secure) {
            if (vec_size != 3) {
                ERR("Unknown Message");
                return;
            }
            info.secure = true;
            auto key_blob = connectInfo[1].AsBlob();
            if (key_blob.size() == sizeof(info.key))
                memcpy(info.key, key_blob.data(), key_blob.size());
            else
                ERR("Invalid key blob(%zu) != %zu", key_blob.size(), sizeof(info.key));

            auto iv_blob = connectInfo[2].AsBlob();
            if (iv_blob.size() == sizeof(info.iv))
                memcpy(info.iv, iv_blob.data(), iv_blob.size());
            else
                ERR("Invalid iv blob(%zu) != %zu", iv_blob.size(), sizeof(info.iv));
        }
        {
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            UpdatePublishTable(topic, clientId, info);
        }
    }
}

void Module::UpdateDiscoveryMsg()
{
    flexbuffers::Builder fbb;
    fbb.Map([this, &fbb]() {
        fbb.String("host", ip);

        // SubscribeTable
        // map {
        //    "/customTopic/mytopic": $serverHandle,
        //    ...
        // }
        for (auto it = subscribeTable.begin(); it != subscribeTable.end(); ++it) {
            if (it->second) {
                fbb.Vector(it->first.c_str(), [&]() {
                    fbb.UInt(it->second->GetPort());
                    if (secure) {
                        fbb.Blob(it->second->GetCryptoKey(), AITT_TCP_ENCRYPTOR_KEY_LEN);
                        fbb.Blob(it->second->GetCryptoIv(), AITT_TCP_ENCRYPTOR_IV_LEN);
                    }
                });
            } else {
                // this is an error case
                TCP::ConnectInfo info;
                fbb.Vector(it->first.c_str(), [&]() { fbb.UInt(it->second->GetPort()); });
            }
        }
    });
    fbb.Finish();

    auto buf = fbb.GetBuffer();
    discovery.UpdateDiscoveryMsg(NAME[secure], buf.data(), buf.size());
}

int Module::ReceiveData(MainLoopHandler::MainLoopResult result, int handle,
      MainLoopHandler::MainLoopData *user_data)
{
    TCPData *tcp_data = dynamic_cast<TCPData *>(user_data);
    RETV_IF(tcp_data == nullptr, AITT_LOOP_EVENT_REMOVE);
    TCPServerData *parent_info = tcp_data->parent;
    RETV_IF(parent_info == nullptr, AITT_LOOP_EVENT_REMOVE);
    Module *impl = parent_info->impl;
    RETV_IF(impl == nullptr, AITT_LOOP_EVENT_REMOVE);

    if (result == MainLoopHandler::HANGUP) {
        ERR("The main loop hung up. Disconnect the client.");
        return impl->HandleClientDisconnect(handle);
    }

    int32_t szmsg = 0;
    char *msg = nullptr;
    AittMsg msg_info;

    impl->GetMsgInfo(msg_info, tcp_data);
    if (msg_info.GetTopic().empty()) {
        ERR("A topic is empty.");
        return AITT_LOOP_EVENT_CONTINUE;
    }

    szmsg = tcp_data->client->RecvSizedData((void **)&msg);
    if (-ENOTCONN == szmsg) {
        ERR("Got a disconnection message(%d)", szmsg);
        return impl->HandleClientDisconnect(handle);
    } else if (szmsg < 0) {
        ERR("Failed to receive data(%d)", szmsg);
        return AITT_LOOP_EVENT_CONTINUE;
    }

    auto callback = parent_info->cb;
    callback(&msg_info, msg, szmsg, parent_info->cbdata);
    free(msg);

    return AITT_LOOP_EVENT_CONTINUE;
}

int Module::HandleClientDisconnect(int handle)
{
    std::lock_guard<std::mutex> autoLock(subscribeTableLock);
    TCPData *tcp_data = dynamic_cast<TCPData *>(main_loop.RemoveWatch(handle));
    if (tcp_data == nullptr) {
        ERR("No watch data");
        return AITT_LOOP_EVENT_REMOVE;
    }
    auto it = std::find(tcp_data->parent->client_list.begin(), tcp_data->parent->client_list.end(),
          handle);
    tcp_data->parent->client_list.erase(it);

    delete tcp_data;
    return AITT_LOOP_EVENT_REMOVE;
}

void Module::GetMsgInfo(AittMsg &msg, Module::TCPData *tcp_data)
{
    int32_t info_length = 0;
    void *msg_info = nullptr;
    info_length = tcp_data->client->RecvSizedData(&msg_info);
    if (-ENOTCONN == info_length) {
        ERR("Got a disconnection message.");
        HandleClientDisconnect(tcp_data->client->GetHandle());
        return;
    }
    if (nullptr == msg_info) {
        ERR("Unknown topic");
        return;
    }

    UnpackMsgInfo(msg, msg_info, info_length);

    free(msg_info);
}

void Module::UnpackMsgInfo(AittMsg &msg, const void *data, const size_t datalen)
{
    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(data), datalen).AsMap();

    if (map["topic"].IsString())
        msg.SetTopic(map["topic"].AsString().str());
    if (map["reply_topic"].IsString())
        msg.SetResponseTopic(map["reply_topic"].AsString().str());
    if (map["correlation"].IsString())
        msg.SetCorrelation(map["correlation"].AsString().str());
    if (map["sequence"].IsUInt())
        msg.SetSequence(map["sequence"].AsUInt64());
    if (map["end_sequence"].IsBool())
        msg.SetEndSequence(map["end_sequence"].AsBool());
}

int Module::AcceptConnection(MainLoopHandler::MainLoopResult result, int handle,
      MainLoopHandler::MainLoopData *user_data)
{
    TCPServerData *listen_info = dynamic_cast<TCPServerData *>(user_data);
    RETV_IF(listen_info == nullptr, AITT_LOOP_EVENT_REMOVE);
    Module *impl = listen_info->impl;
    RETV_IF(impl == nullptr, AITT_LOOP_EVENT_REMOVE);

    std::unique_ptr<TCP> client;
    {
        std::lock_guard<std::mutex> autoLock(impl->subscribeTableLock);

        auto clientIt = impl->subscribeTable.find(listen_info->topic);
        if (clientIt == impl->subscribeTable.end())
            return AITT_LOOP_EVENT_REMOVE;

        client = clientIt->second->AcceptPeer();
    }

    if (client == nullptr) {
        ERR("Unable to accept a peer");  // NOTE: FATAL ERROR
        return AITT_LOOP_EVENT_CONTINUE;
    }

    int client_handle = client->GetHandle();
    listen_info->client_list.push_back(client_handle);

    TCPData *ecd = new TCPData;
    ecd->parent = listen_info;
    ecd->client = std::move(client);

    impl->main_loop.AddWatch(client_handle, ReceiveData, ecd);
    return AITT_LOOP_EVENT_CONTINUE;
}

void Module::UpdatePublishTable(const std::string &topic, const std::string &clientId,
      const TCP::ConnectInfo &info)
{
    auto topicIt = publishTable.find(topic);
    if (topicIt == publishTable.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(info, nullptr));
        HostMap hostMap;
        hostMap.insert(HostMap::value_type(clientId, std::move(portMap)));
        publishTable.insert(PublishMap::value_type(topic, std::move(hostMap)));
        return;
    }

    auto hostIt = topicIt->second.find(clientId);
    if (hostIt == topicIt->second.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(info, nullptr));
        topicIt->second.insert(HostMap::value_type(clientId, std::move(portMap)));
        return;
    }

    if (!hostIt->second.empty()) {
        ERR("there is the previous connection(The current implementation only has a single port "
            "entry)");
        auto portIt = hostIt->second.begin();

        if (portIt->first.port == info.port) {
            DBG("nothing changed. keep the current handle");
            return;
        }

        DBG("delete the connection handle to make a new connection with the new port");
        hostIt->second.clear();
    }

    hostIt->second.insert(PortMap::value_type(info, nullptr));
}

}  // namespace AittTCPNamespace
