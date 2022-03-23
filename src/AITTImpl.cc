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
#include "AITTImpl.h"

#include <flatbuffers/flexbuffers.h>
#include <sys/random.h>

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

#include "log.h"

#define DISCOVERY_TOPIC_BASE std::string("/aitt/discovery/")
#define RESPONSE_POSTFIX "_AittRe_"

namespace aitt {

AITT::Impl::Impl(AITT *parent, const std::string &id, const std::string &ipAddr, bool clearSession)
      : id_(id),
        mq(std::make_unique<MQ>(id, clearSession)),
        parent(parent),
        reply_id(0),
        discoveryCallbackHandle(0),
        modules(ipAddr)
{
    // TODO:
    // Validate ipAddr

    if (id_.empty()) {
        char buf[16];
        int rc = getrandom(buf, sizeof(buf), 0);
        id_ = "aitt-" + std::string(buf, rc);
    }

    mq->Start();

    aittThread = std::thread(&AITT::Impl::ThreadMain, this);
}

AITT::Impl::~Impl(void)
{
    if (discoveryCallbackHandle)
        Disconnect();

    while (main_loop.Quit() == false) {
        // wait when called before the thread has completely created.
        usleep(1000);
    }

    if (aittThread.joinable())
        aittThread.join();

    // NOTE:
    // If we forcibly stop the MQ network loop (threaded),
    // The disconnect message could not be sent to the broker sometimes.
    // The broker will send WILL message if it detect client disconnection without proper DISCONNECT
    // message
    mq->Stop(true);
}

void AITT::Impl::ThreadMain(void)
{
    pthread_setname_np(pthread_self(), "AITTWorkerLoop");
    main_loop.Run();
}

void AITT::Impl::Connect(const std::string &host, int port)
{
    // Connect with the last will message
    mq->Connect(host, port, DISCOVERY_TOPIC_BASE + id_, nullptr, 0, MQ::QoS::EXACTLY_ONCE, true);

    if (discoveryCallbackHandle)
        mq->Unsubscribe(discoveryCallbackHandle);

    discoveryCallbackHandle = mq->Subscribe(DISCOVERY_TOPIC_BASE + "+",
          AITT::Impl::DiscoveryMessageCallback, static_cast<void *>(this));
}

void AITT::Impl::Disconnect(void)
{
    {
        std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

        for (auto subscribe_info : subscribed_list) {
            switch (subscribe_info->first) {
            case AITT_TYPE_MQTT:
                mq->Unsubscribe(subscribe_info->second);
                break;
            case AITT_TYPE_TCP: {
                auto tcpModule = modules.GetInstance(AITT_TYPE_TCP);
                tcpModule->Unsubscribe(subscribe_info->second);
                break;
            }
            default:
                ERR("Unknown AittProtocol(%d)", subscribe_info->first);
                break;
            }

            delete subscribe_info;
        }
        subscribed_list.clear();
    }

    flexbuffers::Builder fbb;
    fbb.Map([&fbb]() { fbb.String("status", AITT::WILL_LEAVE_NETWORK); });
    fbb.Finish();
    auto buf = fbb.GetBuffer();

    // NOTE:
    // Publish the last will if the client gets disconnected intentionally, otherwise the broker is
    // going to publish it for this client
    mq->Publish(DISCOVERY_TOPIC_BASE + id_, buf.data(), buf.size(), MQ::QoS::EXACTLY_ONCE, true);
    mq->Unsubscribe(discoveryCallbackHandle);
    mq->Disconnect();
    discoveryCallbackHandle = nullptr;
}

void AITT::Impl::Publish(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocols, AITT::QoS qos, bool retain)
{
    if ((protocols & AITT_TYPE_MQTT) == AITT_TYPE_MQTT)
        mq->Publish(topic, data, datalen, static_cast<MQ::QoS>(qos), retain);

    // NOTE:
    // Invoke the publish method of the specified transport module
    if ((protocols & AITT_TYPE_TCP) == AITT_TYPE_TCP) {
        auto tcpModule = modules.GetInstance(AITT_TYPE_TCP);
        tcpModule->Publish(topic, data, datalen, qos, retain);
    }
}

AittSubscribeID AITT::Impl::Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
      void *cbdata, AittProtocol protocol, AITT::QoS qos)
{
    SubscribeInfo *info = new SubscribeInfo();
    info->first = protocol;

    void *subscribe_handle;
    switch (protocol) {
    case AITT_TYPE_MQTT:
        subscribe_handle = MQSubscribe(info, &main_loop, topic, cb, cbdata, qos);
        break;
    case AITT_TYPE_TCP:
        subscribe_handle = SubscribeTCP(info, topic, cb, cbdata, qos);
        PublishSubscribeTable();
        break;
    default:
        ERR("Unknown AittProtocol(%d)", protocol);
        delete info;
        throw std::runtime_error("Unknown AittProtocol");
    }
    info->second = subscribe_handle;
    {
        std::unique_lock<std::mutex> lock(subscribed_list_mutex_);
        subscribed_list.push_back(info);
    }

    return reinterpret_cast<AittSubscribeID>(info);
}

AittSubscribeID AITT::Impl::MQSubscribe(SubscribeInfo *handle, MainLoopHandler *loop_handle,
      const std::string &topic, const SubscribeCallback &cb, void *cbdata, AITT::QoS qos)
{
    return mq->Subscribe(
          topic,
          [this, handle, loop_handle, cb](MSG *msg, const std::string &topic, const void *data,
                const size_t datalen, void *cbdata) {
              void *delivery = malloc(datalen);
              if (delivery)
                  memcpy(delivery, data, datalen);

              msg->SetID(handle);
              auto idler_cb = std::bind(&Impl::DetachedCB, this, cb, *msg, delivery, datalen,
                    cbdata, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
              MainLoopHandler::AddIdle(loop_handle, idler_cb, nullptr);
          },
          cbdata, static_cast<MQ::QoS>(qos));
}

void AITT::Impl::DetachedCB(SubscribeCallback cb, MSG msg, void *data, const size_t datalen,
      void *cbdata, MainLoopHandler::MainLoopResult result, int fd,
      MainLoopHandler::MainLoopData *loop_data)
{
    RET_IF(cb == nullptr);

    cb(&msg, data, datalen, cbdata);

    free(data);
}

void *AITT::Impl::Unsubscribe(AittSubscribeID subscribe_id)
{
    SubscribeInfo *info = reinterpret_cast<SubscribeInfo *>(subscribe_id);

    std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

    auto it = std::find(subscribed_list.begin(), subscribed_list.end(), info);
    if (it == subscribed_list.end()) {
        ERR("Unknown subscribe_id(%p)", subscribe_id);
        throw std::runtime_error("subscribe_id");
    }

    void *cbdata = nullptr;
    SubscribeInfo *found_info = *it;
    switch (found_info->first) {
    case AITT_TYPE_MQTT:
        cbdata = mq->Unsubscribe(found_info->second);
        break;
    case AITT_TYPE_TCP: {
        auto tcpModule = modules.GetInstance(AITT_TYPE_TCP);
        cbdata = tcpModule->Unsubscribe(found_info->second);
        break;
    }
    default:
        ERR("Unknown AittProtocol(%d)", found_info->first);
        break;
    }

    subscribed_list.erase(it);
    delete info;

    PublishSubscribeTable();
    return cbdata;
}

void AITT::Impl::DiscoveryMessageCallback(MSG *mq, const std::string &topic, const void *msg,
      const int szmsg, void *cbdata)
{
    AITT::Impl *impl = static_cast<AITT::Impl *>(cbdata);

    size_t end = topic.find("/", DISCOVERY_TOPIC_BASE.length());

    std::string clientId = topic.substr(DISCOVERY_TOPIC_BASE.length(), end);
    if (clientId.empty()) {
        ERR("ClientId is empty");
        return;
    }

    if (msg == nullptr) {
        auto tcpModule = impl->modules.GetInstance(AITT_TYPE_TCP);
        if (tcpModule)
            tcpModule->DiscoveryMessageCallback(clientId, AITT::WILL_LEAVE_NETWORK, nullptr, 0);

        // TODO:
        // Invoke more modules for handling the disconnection

        return;
    }

    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();

    // serviceMessage (flexbuffers)
    // map {
    //   "status": "connected",
    //   "tcp": binaryModuleData,
    //   "webrtc": binaryModuleData,
    // }
    std::string status = map["status"].AsString().c_str();

    auto keys = map.Keys();
    for (size_t idx = 0; idx < keys.size(); ++idx) {
        std::string key = keys[idx].AsString().c_str();

        if (!key.compare("status"))
            continue;

        auto blob = map[key].AsBlob();
        if (!key.compare("tcp")) {
            auto tcpModule = impl->modules.GetInstance(AITT_TYPE_TCP);
            if (tcpModule)
                tcpModule->DiscoveryMessageCallback(clientId, status, blob.data(), blob.size());
        }

        // TODO:
        // Invoke more modules for handling the each protocol information
    }
}

int AITT::Impl::PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, QoS qos, bool retain, const SubscribeCallback &cb, void *cbdata,
      const std::string &correlation)
{
    std::string replyTopic = topic + RESPONSE_POSTFIX + std::to_string(reply_id++);

    if (protocol != AITT_TYPE_MQTT)
        return -1;  // not yet support

    Subscribe(
          replyTopic,
          [this, cb](MSG *sub_msg, const void *sub_data, const size_t sub_datalen,
                void *sub_cbdata) {
              cb(sub_msg, sub_data, sub_datalen, sub_cbdata);

              if (sub_msg->IsEndSequence()) {
                  Unsubscribe(sub_msg->GetID());
              }
          },
          cbdata, protocol, qos);

    mq->PublishWithReply(topic, data, datalen, static_cast<MQ::QoS>(qos), false, replyTopic,
          correlation);
    return 0;
}

int AITT::Impl::PublishWithReplySync(const std::string &topic, const void *data,
      const size_t datalen, AittProtocol protocol, AITT::QoS qos, bool retain,
      const SubscribeCallback &cb, void *cbdata, const std::string &correlation, int timeout_ms)
{
    std::string replyTopic = topic + RESPONSE_POSTFIX + std::to_string(reply_id++);

    if (protocol != AITT_TYPE_MQTT)
        return -1;  // not yet support

    SubscribeInfo *info = new SubscribeInfo();
    info->first = protocol;

    void *subscribe_handle;
    MainLoopHandler sync_loop;
    unsigned int timeout_id = 0;
    bool is_timeout = false;

    subscribe_handle = MQSubscribe(
          info, &sync_loop, replyTopic,
          [&](MSG *sub_msg, const void *sub_data, const size_t sub_datalen, void *sub_cbdata) {
              cb(sub_msg, sub_data, sub_datalen, sub_cbdata);

              if (sub_msg->IsEndSequence()) {
                  Unsubscribe(sub_msg->GetID());
                  sync_loop.Quit();
              } else {
                  if (timeout_id) {
                      sync_loop.RemoveTimeout(timeout_id);
                      HandleTimeout(timeout_ms, timeout_id, sync_loop, is_timeout);
                  }
              }
          },
          cbdata, qos);
    info->second = subscribe_handle;
    {
        std::unique_lock<std::mutex> lock(subscribed_list_mutex_);
        subscribed_list.push_back(info);
    }

    mq->PublishWithReply(topic, data, datalen, static_cast<MQ::QoS>(qos), false, replyTopic,
          correlation);
    if (timeout_ms)
        HandleTimeout(timeout_ms, timeout_id, sync_loop, is_timeout);

    sync_loop.Run();

    if (is_timeout)
        return AITT_ERROR_TIMED_OUT;
    return 0;
}

void AITT::Impl::HandleTimeout(int timeout_ms, unsigned int &timeout_id,
      aitt::MainLoopHandler &sync_loop, bool &is_timeout)
{
    timeout_id = sync_loop.AddTimeout(
          timeout_ms,
          [&, timeout_ms](MainLoopHandler::MainLoopResult result, int fd,
                MainLoopHandler::MainLoopData *data) {
              ERR("PublishWithReplySync() timeout(%d)", timeout_ms);
              sync_loop.Quit();
              is_timeout = true;
          },
          nullptr);
}

void AITT::Impl::SendReply(MSG *msg, const void *data, const int datalen, bool end)
{
    RET_IF(msg == nullptr);

    if ((msg->GetProtocols() & AITT_TYPE_MQTT) != AITT_TYPE_MQTT)
        return;  // not yet support

    if (end == false || msg->GetSequence())
        msg->IncreaseSequence();
    msg->SetEndSequence(end);

    mq->SendReply(msg, data, datalen, MQ::AT_MOST_ONCE, false);
}

void AITT::Impl::PublishSubscribeTable(void)
{
    flexbuffers::Builder fbb;

    // serviceMessage (flexbuffers)
    // map {
    //   "status": "connected",
    //   "tcp": binaryModuleData,
    //   "webrtc": binaryModuleData,
    // }
    fbb.Map([this, &fbb]() {
        fbb.String("status", AITT::JOIN_NETWORK);

        auto tcpModule = modules.GetInstance(AITT_TYPE_TCP);
        if (tcpModule) {
            const void *msg = nullptr;
            int szmsg = 0;
            tcpModule->GetDiscoveryMessage(msg, szmsg);
            fbb.Key("tcp");
            fbb.Blob(msg, szmsg);
        }

        // TODO:
        // Extract more DiscoveryMessage from modules
    });

    fbb.Finish();

    auto buf = fbb.GetBuffer();
    mq->Publish(DISCOVERY_TOPIC_BASE + id_, buf.data(), buf.size(), MQ::QoS::EXACTLY_ONCE, true);
}

void *AITT::Impl::SubscribeTCP(SubscribeInfo *handle, const std::string &topic,
      const SubscribeCallback &cb, void *cbdata, QoS qos)
{
    auto tcpModule = modules.GetInstance(AITT_TYPE_TCP);
    return tcpModule->Subscribe(
          topic,
          [handle, cb](const std::string &topic, const void *data, const size_t datalen,
                void *cbdata, const std::string &correlation) -> void {
              MSG msg;
              msg.SetID(handle);
              msg.SetTopic(topic);
              msg.SetCorrelation(correlation);
              msg.SetProtocols(AITT_TYPE_TCP);

              return cb(&msg, data, datalen, cbdata);
          },
          cbdata, qos);
}

}  // namespace aitt
