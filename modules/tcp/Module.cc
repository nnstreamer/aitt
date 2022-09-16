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

#include <AittUtil.h>
#include <flatbuffers/flexbuffers.h>
#include <unistd.h>

#include "aitt_internal.h"

Module::Module(AittProtocol protocol, const std::string &ip, AittDiscovery &discovery)
      : AittTransport(discovery), protocol(protocol), ip(ip)
{
    aittThread = std::thread(&Module::ThreadMain, this);

    discovery_cb = discovery.AddDiscoveryCB(AITT_TYPE_TCP,
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    DBG("Discovery Callback : %p, %d", this, discovery_cb);
}

Module::~Module(void)
{
    discovery.RemoveDiscoveryCB(discovery_cb);

    while (main_loop.Quit() == false) {
        // wait when called before the thread has completely created
        usleep(1000);
    }

    if (aittThread.joinable())
        aittThread.join();
}

void Module::ThreadMain(void)
{
    pthread_setname_np(pthread_self(), "TCPWorkerLoop");
    main_loop.Run();
}

void Module::Publish(
      const std::string &topic, const void *data, const size_t datalen, AittQoS qos, bool retain)
{
    Publish(topic, data, datalen, std::string(), qos, retain);
}

void Module::Publish(const std::string &topic, const void *data, const size_t datalen,
      const std::string &correlation, AittQoS qos, bool retain)
{
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
        if (!aitt::AittUtil::CompareTopic(it->first, topic))
            continue;

        INFO("[Topic] it->first (%s)", it->first.c_str());
        for (HostMap::iterator hostIt = it->second.begin(); hostIt != it->second.end(); ++hostIt) {
            INFO("[ClientID] hostIt->first (%s)", hostIt->first.c_str());
            // Iterate all ports,
            // the current implementation only be able to have the ZERO or a SINGLE entry
            for (PortMap::iterator portIt = hostIt->second.begin(); portIt != hostIt->second.end();
                  ++portIt) {
                // portIt->second // handle
                INFO("[Port] portIt->first = (%d)", portIt->first);
                if (!portIt->second) {  // AITT_TYPE_TCP
                    std::string host = FindHost(hostIt);
                    if (host.empty() == true) {
                        ERR("clientTable or subscribeTable is broken.");
                        continue;
                    }
                    std::unique_ptr<TCP> client(new TCP(host, portIt->first));

                    // TODO:
                    // If the client gets disconnected, this channel entry must be cleared
                    // In order to do that, there should be an observer to monitor
                    // each connections and manipulate the discovered service table
                    INFO("A new TCP client for topic(%s) is created!!", topic.c_str());
                    std::unique_ptr<TCPPublishInfo> clientInfo(new TCPPublishInfo());
                    clientInfo->client_handle = std::move(client);
                    portIt->second = std::move(clientInfo);
                }

                if (protocol == AITT_TYPE_SECURE_TCP && !portIt->second->client_handle) {
                    std::string host = FindHost(hostIt);
                    if (host.empty() == true) {
                        ERR("clientTable or subscribeTable is broken.");
                        continue;
                    }
                    std::unique_ptr<TCP> client(new TCP(host, portIt->first));

                    INFO("[SECURE_TCP] A new TCP client for topic(%s) is created!!",
                            topic.c_str());
                    portIt->second->client_handle = std::move(client);
                }

                if (!portIt->second->client_handle) {
                    ERR("Failed to create a new client instance");
                    continue;
                }

                if (protocol == AITT_TYPE_SECURE_TCP) {
                    if (SendEncryptedTopic(topic, portIt) == true)
                        SendEncryptedPayload(datalen, portIt, data);
                } else {
                    if (SendTopic(topic, portIt) == true)
                        SendPayload(datalen, portIt, data);
                }
            }
        }
    }
}

std::string Module::FindHost(HostMap::iterator &host_iterator)
{
    std::lock_guard<std::mutex> auto_lock_client(clientTableLock);
    ClientMap::iterator client_iterator = clientTable.find(host_iterator->first);
    if (client_iterator != clientTable.end())
        return client_iterator->second;

    return std::string();
}

bool Module::SendEncryptedTopic(const std::string &topic, Module::PortMap::iterator &portIt)
{
    size_t topic_length = topic.length();
    unsigned char *encrypted_data = nullptr;

    try {
        SendEncryptedData(portIt, static_cast<const void *>(&topic_length), sizeof(topic_length));

        SendEncryptedData(portIt, static_cast<const void *>(topic.c_str()), topic_length);
    } catch (std::exception &e) {
        ERR("An exception(%s) occurs during SendExactSize().", e.what());
        free(encrypted_data);
        return false;
    }

    return true;
}

void Module::SendEncryptedData(
      Module::PortMap::iterator &port_iterator, const void *data, size_t data_length)
{
    size_t encrypted_data_size = 0;
    unsigned char *encrypted_data = port_iterator->second->aes_encryptor->GetEncryptedData(
          data, data_length, encrypted_data_size);
    if (encrypted_data != nullptr && encrypted_data_size > 0)
        SendExactSize(port_iterator, encrypted_data, encrypted_data_size);

    free(encrypted_data);
}

void Module::SendExactSize(
      Module::PortMap::iterator &port_iterator, const void *data, size_t data_length)
{
    size_t remaining_size = data_length;
    while (0 < remaining_size) {
        const char *data_index = static_cast<const char *>(data) + (data_length - remaining_size);
        size_t size_sent = remaining_size;
        port_iterator->second->client_handle->Send(data_index, size_sent);
        if (size_sent > 0) {
            remaining_size -= size_sent;
        } else if (size_sent == 0) {
            DBG("size_sent == 0");
            remaining_size = 0;
        }
    }
}

void Module::SendEncryptedPayload(
      const size_t &datalen, Module::PortMap::iterator &portIt, const void *data)
{
    size_t payload_size = datalen;
    if (0 == datalen) {
        // Distinguish between connection problems and zero-size messages
        INFO("Send a zero-size message.");
        payload_size = UINT32_MAX;
    }

    try {
        SendEncryptedData(portIt, static_cast<void *>(&payload_size), sizeof(payload_size));
        if (payload_size == UINT32_MAX) {
            INFO("An actual data size is 0. Skip this payload transmission.");
            return;
        }

        SendEncryptedData(portIt, data, datalen);
    } catch (std::exception &e) {
        ERR("An exception(%s) occurs during SendExactSize().", e.what());
    }
}

bool Module::SendTopic(const std::string &topic, Module::PortMap::iterator &portIt)
{
    size_t topic_length = topic.length();

    try {
        SendExactSize(portIt, static_cast<const void *>(&topic_length), sizeof(topic_length));

        SendExactSize(portIt, static_cast<const void *>(topic.c_str()), topic_length);
    } catch (std::exception &e) {
        ERR("An exception(%s) occurs during SendExactSize().", e.what());
        return false;
    }

    return true;
}

void Module::SendPayload(const size_t &datalen, Module::PortMap::iterator &portIt, const void *data)
{
    size_t payload_size = datalen;
    if (0 == datalen) {
        // Distinguish between connection problems and zero-size messages
        INFO("Send a zero-size message.");
        payload_size = UINT32_MAX;
    }

    try {
        DBG("sizeof(payload_size) = %zu", sizeof(payload_size));
        SendExactSize(portIt, static_cast<void *>(&payload_size), sizeof(payload_size));

        if (payload_size == UINT32_MAX) {
            INFO("An actual data size is 0. Skip this payload transmission.");
            return;
        }

        DBG("datalen = %zu", datalen);
        SendExactSize(portIt, data, datalen);
    } catch (std::exception &e) {
        ERR("An exception(%s) occurs during SendExactSize().", e.what());
    }
}
void *Module::Subscribe(const std::string &topic, const AittTransport::SubscribeCallback &cb,
      void *cbdata, AittQoS qos)
{
    std::unique_ptr<TCP::Server> tcpServer;

    unsigned short port = 0;
    tcpServer = std::unique_ptr<TCP::Server>(new TCP::Server("0.0.0.0", port));
    TCPServerData *listen_info = new TCPServerData;
    listen_info->impl = this;
    listen_info->cb = cb;
    listen_info->cbdata = cbdata;
    listen_info->topic = topic;
    listen_info->is_secure = (protocol == AITT_TYPE_SECURE_TCP ? true : false);
    if (listen_info->is_secure == true) {
        tcpServer->CreateAESEncryptor();
        listen_info->aes_encryptor = tcpServer->GetAESEncryptor();
    }

    auto handle = tcpServer->GetHandle();
    main_loop.AddWatch(handle, AcceptConnection, listen_info);

    {
        std::lock_guard<std::mutex> autoLock(subscribeTableLock);
        subscribeTable.insert(SubscribeMap::value_type(topic, std::move(tcpServer)));
        UpdateDiscoveryMsg();
    }

    return reinterpret_cast<void *>(handle);
}

void *Module::Subscribe(const std::string &topic, const AittTransport::SubscribeCallback &cb,
      const void *data, const size_t datalen, void *cbdata, AittQoS qos)
{
    return nullptr;
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

        UpdateDiscoveryMsg();
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
    //   "$topic": port,
    //   "$topic/port" : protocol
    //   // if protocol == AES_TYPE_SECURE_TCP, the below exists.
    //   "$topic/port/protocol" : cipher_key
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

        auto port = map[topic].AsUInt16();
        std::string protocol_topic = std::string(topic).append("/").append(std::to_string(port));
        auto protocol = map[protocol_topic].AsUInt16();
        const unsigned char *key = nullptr;
        if (protocol == AITT_TYPE_SECURE_TCP) {
            std::string key_topic =
                  std::string(protocol_topic).append("/").append(std::to_string(protocol));
            const char *transmitted_key = map[key_topic].AsString().c_str();
            key = reinterpret_cast<const unsigned char *>(transmitted_key);
            {
                std::lock_guard<std::mutex> autoLock(publishTableLock);
                UpdatePublishTable(topic, clientId, port, key);
            }
        }
        {
            std::lock_guard<std::mutex> autoLock(publishTableLock);
            UpdatePublishTable(topic, clientId, port, key);
        }
    }
}

void Module::UpdateDiscoveryMsg()
{
    flexbuffers::Builder fbb;
    // flexbuffers
    // {
    //   "host": "127.0.0.1",
    //   "$topic": $port,
    //   ...
    //   "$topic/port" : protocol
    //   // if protocol == AITT_TYPE_SECURE_TCP, then the below exists.
    //   "$topic/port/protocol" : key
    // }
    fbb.Map([this, &fbb]() {
        fbb.String("host", ip);

        // SubscribeTable
        // map {
        //    "/customTopic/mytopic": $serverHandle,
        //    ...
        // }
        for (auto it = subscribeTable.begin(); it != subscribeTable.end(); ++it) {
            if (it->second) {
                auto port = it->second->GetPort();
                fbb.UInt(it->first.c_str(), port);
                if (protocol == AITT_TYPE_SECURE_TCP) {
                    std::string protocol_topic =
                          std::string(it->first.c_str()).append("/").append(std::to_string(port));
                    fbb.UInt(protocol_topic.c_str(), static_cast<int>(protocol));
                    const unsigned char *key = it->second->GetKey();
                    std::string key_topic =
                          protocol_topic.append("/").append(std::to_string(protocol));
                    fbb.String(key_topic.c_str(), std::string(reinterpret_cast<const char *>(key)));
                }
            } else {
                fbb.UInt(it->first.c_str(), 0);  // this is an error case
            }
        }
    });
    fbb.Finish();

    auto buf = fbb.GetBuffer();
    discovery.UpdateDiscoveryMsg(AITT_TYPE_TCP, buf.data(), buf.size());
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
        ERR("The main loop hung up. Disconnect the client.");
        return impl->HandleClientDisconnect(handle);
    }

    size_t szmsg = 0;
    char *msg = nullptr;
    std::string topic;

    try {
        if (connect_info->parent->is_secure == true) {
            topic = impl->ReceiveDecryptedTopic(connect_info);
            if (topic.empty()) {
                ERR("A topic is empty.");
                return impl->HandleClientDisconnect(handle);
            }

            if (impl->ReceiveDecryptedPayload(connect_info, szmsg, &msg) == false) {
                free(msg);
                return impl->HandleClientDisconnect(handle);
            }
        } else {
            topic = impl->ReceiveTopic(connect_info);
            if (topic.empty()) {
                ERR("A topic is empty.");
                return impl->HandleClientDisconnect(handle);
            }

            if (impl->ReceivePayload(connect_info, szmsg, &msg) == false) {
                free(msg);
                return impl->HandleClientDisconnect(handle);
            }
        }
    } catch (std::exception &e) {
        ERR("An exception(%s) occurs", e.what());
        free(msg);
        return;
    }

    std::string correlation;
    // TODO: Correlation data (string) should be filled

    parent_info->cb(topic, msg, szmsg, parent_info->cbdata, correlation);
    free(msg);
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

std::string Module::ReceiveDecryptedTopic(Module::TCPData *connect_info)
{
    size_t topic_length = 0;
    ReceiveDecryptedData(connect_info, static_cast<void *>(&topic_length), sizeof(topic_length));

    if (AITT_TOPIC_NAME_MAX < topic_length) {
        ERR("Invalid topic name length(%zu)", topic_length);
        return std::string();
    }

    char topic_buffer[topic_length];
    ReceiveDecryptedData(connect_info, topic_buffer, topic_length);

    std::string topic = std::string(topic_buffer, topic_length);
    INFO("Complete topic = [%s], topic_len = %zu", topic.c_str(), topic_length);

    return topic;
}

bool Module::ReceiveDecryptedPayload(Module::TCPData *connect_info, size_t &szmsg, char **msg)
{
    ReceiveDecryptedData(connect_info, static_cast<void *>(&szmsg), sizeof(szmsg));
    if (szmsg == 0) {
        ERR("Got a disconnection message.");
        return false;
    }

    if (UINT32_MAX == szmsg) {
        // Distinguish between connection problems and zero-size messages
        INFO("Got a zero-size message. Skip this payload transmission.");
        szmsg = 0;
    } else {
        *msg = static_cast<char *>(malloc(szmsg));
        ReceiveDecryptedData(connect_info, static_cast<void *>(*msg), szmsg);
    }

    return true;
}

void Module::ReceiveDecryptedData(Module::TCPData *connect_info, void *data, size_t data_length)
{
    size_t padding_buffer_size =
          connect_info->parent->aes_encryptor->GetPaddingBufferSize(data_length);
    DBG("data_length = %zu, padding_buffer_size = %zu", data_length, padding_buffer_size);

    unsigned char padding_buffer[padding_buffer_size];
    ReceiveExactSize(connect_info, static_cast<void *>(padding_buffer), padding_buffer_size);

    connect_info->parent->aes_encryptor->GetDecryptedData(
          padding_buffer, padding_buffer_size, data_length, data);
}

void Module::ReceiveExactSize(Module::TCPData *connect_info, void *data, size_t data_length)
{
    if (data_length == 0) {
        DBG("data_length is zero.");
        return;
    }

    size_t remaining_size = data_length;
    while (0 < remaining_size) {
        char *data_index = (char *)data + (data_length - remaining_size);
        size_t size_received = remaining_size;
        connect_info->client->Recv(data_index, size_received);
        if (size_received > 0) {
            remaining_size -= size_received;
        } else if (size_received == 0) {
            DBG("size_received == 0");
            remaining_size = 0;
        }
    }
}

std::string Module::ReceiveTopic(Module::TCPData *connect_info)
{
    size_t topic_length = 0;
    ReceiveExactSize(connect_info, static_cast<void *>(&topic_length), sizeof(topic_length));

    if (AITT_TOPIC_NAME_MAX < topic_length) {
        ERR("Invalid topic name length(%zu)", topic_length);
        return std::string();
    }

    char topic_buffer[topic_length];
    ReceiveExactSize(connect_info, topic_buffer, topic_length);
    std::string topic = std::string(topic_buffer, topic_length);
    INFO("Complete topic = [%s], topic_len = %zu", topic.c_str(), topic_length);

    return topic;
}

bool Module::ReceivePayload(Module::TCPData *connect_info, size_t &szmsg, char **msg)
{
    ReceiveExactSize(connect_info, static_cast<void *>(&szmsg), sizeof(szmsg));
    if (szmsg == 0) {
        ERR("Got a disconnection message.");
        return false;
    }
    ERR("szmsg = [%zu]", szmsg);
    if (UINT32_MAX == szmsg) {
        // Distinguish between connection problems and zero-size messages
        INFO("Got a zero-size message. Skip this payload transmission.");
        szmsg = 0;
    } else {
        *msg = static_cast<char *>(malloc(szmsg));
        ReceiveExactSize(connect_info, static_cast<void *>(*msg), szmsg);
    }

    return true;
}

void Module::AcceptConnection(MainLoopHandler::MainLoopResult result, int handle,
      MainLoopHandler::MainLoopData *user_data)
{
    // TODO: Update the discovery map
    std::unique_ptr<TCP> client;
    TCPServerData *listen_info = dynamic_cast<TCPServerData *>(user_data);
    Module *impl = listen_info->impl;
    {
        std::lock_guard<std::mutex> autoLock(impl->subscribeTableLock);

        auto clientIt = impl->subscribeTable.find(listen_info->topic);
        if (clientIt == impl->subscribeTable.end())
            return;

        client = clientIt->second->AcceptPeer();
        INFO("A TCP connection (client handle=%d) is created.", client->GetHandle());
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
      unsigned short port, const unsigned char *key)
{
    auto topicIt = publishTable.find(topic);
    std::unique_ptr<TCPPublishInfo> keyInfo(new TCPPublishInfo());
    if (key == nullptr) {
        keyInfo = nullptr;
    } else {
        keyInfo->client_handle = nullptr;
        keyInfo->aes_encryptor = new AESEncryptor(key);
    }

    if (topicIt == publishTable.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(port, std::move(keyInfo)));
        HostMap hostMap;
        hostMap.insert(HostMap::value_type(clientId, std::move(portMap)));
        publishTable.insert(PublishMap::value_type(topic, std::move(hostMap)));
        INFO("A topic(%s) is inserted to the publish table.", topic.c_str());
        return;
    }

    auto hostIt = topicIt->second.find(clientId);
    if (hostIt == topicIt->second.end()) {
        PortMap portMap;
        portMap.insert(PortMap::value_type(port, std::move(keyInfo)));
        topicIt->second.insert(HostMap::value_type(clientId, std::move(portMap)));
        INFO("A HostMap element is added, clientId(%s).", clientId.c_str());
        return;
    }

    // NOTE:
    // The current implementation only has a single port entry
    // Therefore, if the hostIt is not empty, there is the previous connection
    if (!hostIt->second.empty()) {
        auto portIt = hostIt->second.begin();
        INFO("A client handle already exists. port = %d", port);
        if (portIt->first == port)
            return;  // Nothing is changed, keep the current handle

        // Otherwise, delete the connection handle
        // to make a new connection with the new port
        hostIt->second.clear();
    }

    INFO("A PortMap element is inserted. clientId(%s), port = %d", clientId.c_str(), port);
    hostIt->second.insert(PortMap::value_type(port, std::move(keyInfo)));
}
