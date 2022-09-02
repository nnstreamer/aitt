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

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

#include "MQProxy.h"
#include "aitt_internal.h"

#define WEBRTC_ROOM_ID_PREFIX std::string(AITT_MANAGED_TOPIC_PREFIX "webrtc/room/Room.webrtc")
#define WEBRTC_ID_POSTFIX std::string("_for_webrtc")

namespace aitt {

AITT::Impl::Impl(AITT &parent, const std::string &id, const std::string &my_ip,
      const AittOption &option)
      : public_api(parent),
        id_(id),
        mq(new MQProxy(id, option)),
        discovery(id, option),
        reply_id(0),
        transports{0}
{
    // TODO: Validate my_ip
    ModuleLoader loader;
    for (ModuleLoader::Type i = ModuleLoader::TYPE_TCP; i < ModuleLoader::TYPE_TRANSPORT_MAX;
          i = ModuleLoader::Type(i + 1)) {
        module_handles.push_back(loader.OpenModule(i));
        const ModuleLoader::ModuleHandle &handle = module_handles.back();
        if (handle == nullptr)
            ERR("OpenModule() Fail");

        transports[i] = loader.LoadTransport(handle.get(), loader.GetProtocol(i), my_ip, discovery);
    }
    aittThread = std::thread(&AITT::Impl::ThreadMain, this);
}

AITT::Impl::~Impl(void)
{
    if (false == mqtt_broker_ip_.empty())
        Disconnect();

    while (main_loop.Quit() == false) {
        // wait when called before the thread has completely created.
        usleep(1000);  // 1millisecond
    }

    if (aittThread.joinable())
        aittThread.join();
}

void AITT::Impl::ThreadMain(void)
{
    pthread_setname_np(pthread_self(), "AITTWorkerLoop");
    main_loop.Run();
}

void AITT::Impl::SetWillInfo(const std::string &topic, const void *data, const size_t datalen,
      AittQoS qos, bool retain)
{
    mq->SetWillInfo(topic, data, datalen, qos, retain);
}

void AITT::Impl::SetConnectionCallback(ConnectionCallback cb, void *user_data)
{
    if (cb)
        mq->SetConnectionCallback(
              std::bind(&Impl::ConnectionCB, this, cb, user_data, std::placeholders::_1));
    else
        mq->SetConnectionCallback(nullptr);
}

void AITT::Impl::ConnectionCB(ConnectionCallback cb, void *user_data, int status)
{
    RET_IF(cb == nullptr);

    cb(public_api, status, user_data);
}

void AITT::Impl::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    discovery.Start(host, port, username, password);
    mq->Connect(host, port, username, password);

    mqtt_broker_ip_ = host;
    mqtt_broker_port_ = port;
}

void AITT::Impl::Disconnect(void)
{
    UnsubscribeAll();

    mqtt_broker_ip_.clear();
    mqtt_broker_port_ = -1;

    mq->Disconnect();
    discovery.Stop();
}

void AITT::Impl::UnsubscribeAll()
{
    std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

    for (auto subscribe_info : subscribed_list) {
        switch (subscribe_info->first) {
        case AITT_TYPE_MQTT:
            mq->Unsubscribe(subscribe_info->second);
            break;
        case AITT_TYPE_TCP:
            transports[ModuleLoader::TYPE_TCP]->Unsubscribe(subscribe_info->second);
            break;
        case AITT_TYPE_SECURE_TCP:
            transports[ModuleLoader::TYPE_SECURE_TCP]->Unsubscribe(subscribe_info->second);
            break;
        case AITT_TYPE_WEBRTC:
            transports[ModuleLoader::TYPE_WEBRTC]->Unsubscribe(subscribe_info->second);
            break;

        default:
            ERR("Unknown AittProtocol(%d)", subscribe_info->first);
            break;
        }

        delete subscribe_info;
    }
    subscribed_list.clear();
}

void AITT::Impl::ConfigureTransportModule(const std::string &key, const std::string &value,
      AittProtocol protocols)
{
}

void AITT::Impl::Publish(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocols, AittQoS qos, bool retain)
{
    if ((protocols & AITT_TYPE_MQTT) == AITT_TYPE_MQTT)
        mq->Publish(topic, data, datalen, qos, retain);

    if ((protocols & AITT_TYPE_TCP) == AITT_TYPE_TCP)
        transports[ModuleLoader::TYPE_TCP]->Publish(topic, data, datalen, qos, retain);

    if ((protocols & AITT_TYPE_SECURE_TCP) == AITT_TYPE_SECURE_TCP)
        transports[ModuleLoader::TYPE_SECURE_TCP]->Publish(topic, data, datalen, qos, retain);

    if ((protocols & AITT_TYPE_WEBRTC) == AITT_TYPE_WEBRTC)
        PublishWebRtc(topic, data, datalen, qos, retain);
}

void AITT::Impl::PublishWebRtc(const std::string &topic, const void *data, const size_t datalen,
      AittQoS qos, bool retain)
{
    flexbuffers::Builder fbb;
    fbb.Map([=, &fbb]() {
        fbb.String("Id", id_ + WEBRTC_ID_POSTFIX);
        fbb.String("BrokerIp", mqtt_broker_ip_);
        fbb.Int("BrokerPort", mqtt_broker_port_);
        fbb.String("RoomId", WEBRTC_ROOM_ID_PREFIX + topic);
        fbb.String("SourceId", id_ + WEBRTC_ID_POSTFIX);
        // TODO pass user data to WEBRTC module
        fbb.UInt("UserDataLength", datalen);
    });
    fbb.Finish();
    auto buf = fbb.GetBuffer();
    transports[ModuleLoader::TYPE_WEBRTC]->Publish(topic, buf.data(), buf.size(), qos, retain);
}

AittSubscribeID AITT::Impl::Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
      void *user_data, AittProtocol protocol, AittQoS qos)
{
    SubscribeInfo *info = new SubscribeInfo();
    info->first = protocol;
    void *subscribe_handle;
    switch (protocol) {
    case AITT_TYPE_MQTT:
        subscribe_handle = SubscribeMQ(info, &main_loop, topic, cb, user_data, qos);
        break;
    case AITT_TYPE_TCP:
        subscribe_handle = SubscribeTCP(info, topic, cb, user_data, qos);
        break;
    case AITT_TYPE_SECURE_TCP:
        subscribe_handle = SubscribeSecureTCP(info, topic, cb, user_data, qos);
        break;
    case AITT_TYPE_WEBRTC:
        subscribe_handle = SubscribeWebRtc(info, topic, cb, user_data, qos);
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

    INFO("Subscribe topic(%s) : %p", topic.c_str(), info);
    return reinterpret_cast<AittSubscribeID>(info);
}

AittSubscribeID AITT::Impl::SubscribeMQ(SubscribeInfo *handle, MainLoopHandler *loop_handle,
      const std::string &topic, const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    return mq->Subscribe(
          topic,
          [this, handle, loop_handle, cb](MSG *msg, const std::string &topic, const void *data,
                const size_t datalen, void *mq_user_data) {
              void *delivery = malloc(datalen);
              if (delivery)
                  memcpy(delivery, data, datalen);

              msg->SetID(handle);
              auto idler_cb =
                    std::bind(&Impl::DetachedCB, this, cb, *msg, delivery, datalen, mq_user_data,
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
              MainLoopHandler::AddIdle(loop_handle, idler_cb, nullptr);
          },
          user_data, qos);
}

void AITT::Impl::DetachedCB(SubscribeCallback cb, MSG msg, void *data, const size_t datalen,
      void *user_data, MainLoopHandler::MainLoopResult result, int fd,
      MainLoopHandler::MainLoopData *loop_data)
{
    RET_IF(cb == nullptr);

    cb(&msg, data, datalen, user_data);

    free(data);
}

void *AITT::Impl::Unsubscribe(AittSubscribeID subscribe_id)
{
    INFO("subscribe_id : %p", subscribe_id);
    SubscribeInfo *info = reinterpret_cast<SubscribeInfo *>(subscribe_id);

    std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

    auto it = std::find(subscribed_list.begin(), subscribed_list.end(), info);
    if (it == subscribed_list.end()) {
        ERR("Unknown subscribe_id(%p)", subscribe_id);
        throw std::runtime_error("subscribe_id");
    }

    void *user_data = nullptr;
    SubscribeInfo *found_info = *it;
    switch (found_info->first) {
    case AITT_TYPE_MQTT:
        user_data = mq->Unsubscribe(found_info->second);
        break;
    case AITT_TYPE_TCP: {
        user_data = transports[ModuleLoader::TYPE_TCP]->Unsubscribe(found_info->second);
        break;
    }
    case AITT_TYPE_WEBRTC: {
        user_data = transports[ModuleLoader::TYPE_WEBRTC]->Unsubscribe(found_info->second);
        break;
    }
    default:
        ERR("Unknown AittProtocol(%d)", found_info->first);
        break;
    }

    subscribed_list.erase(it);
    delete info;

    return user_data;
}

int AITT::Impl::PublishWithReply(const std::string &topic, const void *data, const size_t datalen,
      AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb, void *user_data,
      const std::string &correlation)
{
    std::string replyTopic = topic + RESPONSE_POSTFIX + std::to_string(reply_id++);

    if (protocol != AITT_TYPE_MQTT)
        return -1;  // not yet support

    Subscribe(
          replyTopic,
          [this, cb](
                MSG *sub_msg, const void *sub_data, const size_t sub_datalen, void *sub_cbdata) {
              if (sub_msg->IsEndSequence()) {
                  try {
                      Unsubscribe(sub_msg->GetID());
                  } catch (std::runtime_error &e) {
                      ERR("Unsubscribe() Fail(%s)", e.what());
                  }
              }
              cb(sub_msg, sub_data, sub_datalen, sub_cbdata);
          },
          user_data, protocol, qos);

    mq->PublishWithReply(topic, data, datalen, qos, false, replyTopic, correlation);
    return 0;
}

int AITT::Impl::PublishWithReplySync(const std::string &topic, const void *data,
      const size_t datalen, AittProtocol protocol, AittQoS qos, bool retain,
      const SubscribeCallback &cb, void *user_data, const std::string &correlation, int timeout_ms)
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

    subscribe_handle = SubscribeMQ(
          info, &sync_loop, replyTopic,
          [&](MSG *sub_msg, const void *sub_data, const size_t sub_datalen, void *sub_cbdata) {
              if (sub_msg->IsEndSequence()) {
                  try {
                      Unsubscribe(sub_msg->GetID());
                  } catch (std::runtime_error &e) {
                      ERR("Unsubscribe() Fail(%s)", e.what());
                  }
                  sync_loop.Quit();
              } else {
                  if (timeout_id) {
                      sync_loop.RemoveTimeout(timeout_id);
                      HandleTimeout(timeout_ms, timeout_id, sync_loop, is_timeout);
                  }
              }
              cb(sub_msg, sub_data, sub_datalen, sub_cbdata);
          },
          user_data, qos);
    info->second = subscribe_handle;
    {
        std::unique_lock<std::mutex> lock(subscribed_list_mutex_);
        subscribed_list.push_back(info);
    }

    mq->PublishWithReply(topic, data, datalen, qos, false, replyTopic, correlation);
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

    mq->SendReply(msg, data, datalen, AITT_QOS_AT_MOST_ONCE, false);
}

void *AITT::Impl::SubscribeTCP(SubscribeInfo *handle, const std::string &topic,
      const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    ERR("[ENTER] SubscribeTCP");
    return transports[ModuleLoader::TYPE_TCP]->Subscribe(
          topic,
          [handle, cb](const std::string &topic, const void *data, const size_t datalen,
                void *user_data, const std::string &correlation) -> void {
              MSG msg;
              msg.SetID(handle);
              msg.SetTopic(topic);
              msg.SetCorrelation(correlation);
              msg.SetProtocols(AITT_TYPE_TCP);

              return cb(&msg, data, datalen, user_data);
          },
          user_data, qos);
}

void *AITT::Impl::SubscribeSecureTCP(SubscribeInfo *handle, const std::string &topic,
      const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    ERR("[ENTER] SubscribeSecureTCP");
    return transports[ModuleLoader::TYPE_SECURE_TCP]->Subscribe(
          topic,
          [handle, cb](const std::string &topic, const void *data, const size_t datalen,
                void *user_data, const std::string &correlation) -> void {
              MSG msg;
              msg.SetID(handle);
              msg.SetTopic(topic);
              msg.SetCorrelation(correlation);
              msg.SetProtocols(AITT_TYPE_SECURE_TCP);

              return cb(&msg, data, datalen, user_data);
          },
          user_data, qos);
}

void *AITT::Impl::SubscribeWebRtc(SubscribeInfo *handle, const std::string &topic,
      const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    flexbuffers::Builder fbb;
    fbb.Map([=, &fbb]() {
        fbb.String("Id", id_ + WEBRTC_ID_POSTFIX);
        fbb.String("BrokerIp", mqtt_broker_ip_);
        fbb.String("RoomId", WEBRTC_ROOM_ID_PREFIX + topic);
        fbb.Int("BrokerPort", mqtt_broker_port_);
    });
    fbb.Finish();
    auto buf = fbb.GetBuffer();

    return transports[ModuleLoader::TYPE_WEBRTC]->Subscribe(
          topic,
          [handle, cb](const std::string &topic, const void *data, const size_t datalen,
                void *user_data, const std::string &correlation) -> void {
              MSG msg;
              msg.SetID(handle);
              msg.SetTopic(topic);
              msg.SetCorrelation(correlation);
              msg.SetProtocols(AITT_TYPE_WEBRTC);

              return cb(&msg, data, datalen, user_data);
          },
          buf.data(), buf.size(), user_data, qos);
}
}  // namespace aitt
