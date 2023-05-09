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

#include <algorithm>
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

    std::lock_guard<std::mutex> auto_lock_publish(publishTableLock);
    for (PublishMap::iterator it = publishTable.begin(); it != publishTable.end(); ++it) {
        // NOTE: Find entries that have matched with the given topic
        if (!discovery.CompareTopic(it->first, is_reply ? msg.GetResponseTopic() : msg.GetTopic()))
            continue;

        for (HostMap::iterator hostIt = it->second.begin(); hostIt != it->second.end(); ++hostIt) {
            PortInfo &port_info = hostIt->second;
            if (!port_info.second) {
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

                std::unique_ptr<TCP> client(new TCP(host, port_info.first));
                port_info.second = std::move(client);
            }

            if (!port_info.second) {
                ERR("Failed to create a new client instance");
                continue;
            }

            try {
                flexbuffers::Builder fbb;
                PackMsgInfo(fbb, msg, is_reply);
                auto buffer = fbb.GetBuffer();
                port_info.second->SendSizedData(buffer.data(), buffer.size());

                port_info.second->SendSizedData(data, datalen);
            } catch (std::exception &e) {
                ERR("An exception(%s) occurs during Send().", e.what());
            }
        }
    }  // publishTable
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
    TCPServerData *listen_info;
    std::unique_ptr<Subscribe_CB_Info> cb_info(new Subscribe_CB_Info(cb, cbdata));
    Subscribe_CB_Info *info_ptr = cb_info.get();

    std::lock_guard<std::mutex> lock_from_here(subscribeTableLock);
    auto it = std::find_if(subscribeTable.begin(), subscribeTable.end(),
          [&](const SubscribeMap::value_type &entry) { return entry.first->topic == topic; });
    if (it != subscribeTable.end()) {
        listen_info = it->first;
        listen_info->cb_list.push_back(std::move(cb_info));
    } else {
        unsigned short port = 0;
        std::unique_ptr<TCP::Server> tcpServer(new TCP::Server("0.0.0.0", port, secure));

        listen_info = new TCPServerData;
        listen_info->impl = this;
        listen_info->cb_list.push_back(std::move(cb_info));
        listen_info->topic = topic;

        main_loop.AddWatch(tcpServer->GetHandle(), AcceptConnection, listen_info);

        auto ret =
              subscribeTable.insert(SubscribeMap::value_type(listen_info, std::move(tcpServer)));
        if (false == ret.second) {
            ERR("insert(%s) Fail", topic.c_str());
            throw std::runtime_error("insert() Fail: " + listen_info->topic);
        }
    }

    UpdateDiscoveryMsg();

    subscribe_handles.insert(SubscribeHandles::value_type(info_ptr, listen_info));

    return info_ptr;
}

void *Module::Unsubscribe(void *handlePtr)
{
    std::lock_guard<std::mutex> autoLock(subscribeTableLock);

    auto handle_it = subscribe_handles.find(static_cast<Subscribe_CB_Info *>(handlePtr));
    if (handle_it == subscribe_handles.end()) {
        ERR("Unknown handle(%p)", handlePtr);
        return nullptr;
    }

    TCPServerData *listen_info = handle_it->second;
    void *cbdata = handle_it->first->second;
    subscribe_handles.erase(handle_it);

    if (1 < listen_info->cb_list.size()) {
        auto cb_it = std::find_if(listen_info->cb_list.begin(), listen_info->cb_list.end(),
              [&](const std::unique_ptr<Subscribe_CB_Info> &cb_info) {
                  return cb_info.get() == handlePtr;
              });
        if (cb_it != listen_info->cb_list.end())
            listen_info->cb_list.erase(cb_it);
        else
            throw std::runtime_error("Invalid Callback Info");
        return cbdata;
    }

    for (auto fd : listen_info->client_list) {
        TCPData *tcp_data = dynamic_cast<TCPData *>(main_loop.RemoveWatch(fd));
        delete tcp_data;
    }
    listen_info->client_list.clear();

    auto it = subscribeTable.find(listen_info);
    if (it == subscribeTable.end())
        throw std::runtime_error("Not subscribed topic: " + listen_info->topic);
    main_loop.RemoveWatch(it->second->GetHandle());
    subscribeTable.erase(it);
    delete listen_info;

    UpdateDiscoveryMsg();

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

// Discovery Message (flexbuffers)
// map {
//   "host": "192.168.1.11",
//   "$topic": {port, cb_list_size, key, iv}
// }
void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        {
            std::lock_guard<std::mutex> autoLock(clientTableLock);
            clientTable.erase(clientId);
        }

        {
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            for (auto it = publishTable.begin(); it != publishTable.end(); ++it)
                it->second.erase(clientId);
        }
        return;
    }

    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();
    std::string host = map["host"].AsString().c_str();

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
        info.num_of_cb = connectInfo[1].AsUInt16();
        if (secure) {
            if (vec_size != 4) {
                ERR("Unknown Message");
                return;
            }
            info.secure = true;
            auto key_blob = connectInfo[2].AsBlob();
            if (key_blob.size() == sizeof(info.key))
                memcpy(info.key, key_blob.data(), key_blob.size());
            else
                ERR("Invalid key blob(%zu) != %zu", key_blob.size(), sizeof(info.key));

            auto iv_blob = connectInfo[3].AsBlob();
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

        for (auto it = subscribeTable.begin(); it != subscribeTable.end(); ++it) {
            if (it->second) {
                fbb.Vector(it->first->topic.c_str(), [&]() {
                    fbb.UInt(it->second->GetPort());
                    fbb.UInt(it->first->cb_list.size());
                    if (secure) {
                        fbb.Blob(it->second->GetCryptoKey(), AITT_TCP_ENCRYPTOR_KEY_LEN);
                        fbb.Blob(it->second->GetCryptoIv(), AITT_TCP_ENCRYPTOR_IV_LEN);
                    }
                });
            } else {
                // this is an error case
                TCP::ConnectInfo info;
                fbb.Vector(it->first->topic.c_str(), [&]() { fbb.UInt(it->second->GetPort()); });
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

    std::vector<Subscribe_CB_Info> cb_list;
    {
        std::lock_guard<std::mutex> autoLock(impl->subscribeTableLock);
        std::transform(parent_info->cb_list.begin(), parent_info->cb_list.end(),
              std::back_inserter(cb_list),
              [](std::unique_ptr<Subscribe_CB_Info> const &it) { return *it; });
    }
    for (auto const &it : cb_list)
        it.first(&msg_info, msg, szmsg, it.second);
    free(msg);

    return AITT_LOOP_EVENT_CONTINUE;
}

int Module::HandleClientDisconnect(int handle)
{
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
    std::lock_guard<std::mutex> autoLock(subscribeTableLock);
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

        auto clientIt = impl->subscribeTable.find(listen_info);
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

    TCPData *tcp_data = new TCPData;
    tcp_data->parent = listen_info;
    tcp_data->client = std::move(client);

    impl->main_loop.AddWatch(client_handle, ReceiveData, tcp_data);
    return AITT_LOOP_EVENT_CONTINUE;
}

void Module::UpdatePublishTable(const std::string &topic, const std::string &clientId,
      const TCP::ConnectInfo &info)
{
    auto topicIt = publishTable.find(topic);
    if (topicIt == publishTable.end()) {
        HostMap hostMap;
        hostMap.insert(HostMap::value_type(clientId, std::make_pair(info, nullptr)));
        publishTable.insert(PublishMap::value_type(topic, std::move(hostMap)));
        return;
    }

    auto hostIt = topicIt->second.find(clientId);
    if (hostIt == topicIt->second.end()) {
        topicIt->second.insert(HostMap::value_type(clientId, std::make_pair(info, nullptr)));
    } else {
        PortInfo &port_info = hostIt->second;
        if (port_info.first.port == info.port) {
            port_info.first.num_of_cb = info.num_of_cb;
        } else {
            port_info = std::make_pair(info, nullptr);
        }
    }
}

int Module::CountSubscriber(const std::string &topic)
{
    int count = 0;

    std::lock_guard<std::mutex> auto_lock(publishTableLock);

    for (auto topicIt = publishTable.begin(); topicIt != publishTable.end(); ++topicIt) {
        if (discovery.CompareTopic(topicIt->first, topic)) {
            for (auto hostIt = topicIt->second.begin(); hostIt != topicIt->second.end(); ++hostIt) {
                TCP::ConnectInfo info = hostIt->second.first;
                count += info.num_of_cb;
            }
        }
    }

    return count;
}

}  // namespace AittTCPNamespace
