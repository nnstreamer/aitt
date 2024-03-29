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
#include "AITTImpl.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>

#include "MQDiscoveryHandler.h"
#include "MainLoopHandler.h"
#include "MosquittoMQ.h"
#include "aitt_internal.h"

namespace aitt {

AITT::Impl::Impl(AITT &parent, const std::string &id, const std::string &my_ip,
      const AittOption &option)
      : public_api(parent),
        discovery(id),
        main_loop(MainLoopHandler::new_loop()),
        modules(my_ip, discovery),
        mq_discovery_handler(discovery, id),
        id_(id),
        mqtt_broker_port_(0),
        reply_id(0)
{
    if (option.GetUseCustomMqttBroker()) {
        mq = modules.NewCustomMQ(id, option);
        AittOption discovery_option = option;
        discovery_option.SetCleanSession(false);
        discovery.SetMQ(modules.NewCustomMQ(id + 'd', option));
    } else {
        mq = std::unique_ptr<MQ>(new MosquittoMQ(id, option.GetCleanSession()));
        discovery.SetMQ(std::unique_ptr<MQ>(new MosquittoMQ(id + 'd', false)));
    }
    aittThread = std::thread(&AITT::Impl::ThreadMain, this);
}

AITT::Impl::~Impl(void)
{
    if (mqtt_broker_ip_.empty() == false) {
        try {
            Disconnect();
        } catch (std::exception &e) {
            ERR("Disconnect() Fail(%s)", e.what());
        }
    }
    while (main_loop->Quit() == false) {
        // wait when called before the thread has completely created.
        usleep(1000);  // 1millisecond
    }

    if (aittThread.joinable())
        aittThread.join();

    discovery.SetMQ(nullptr);
    mq = nullptr;
}

void AITT::Impl::ThreadMain(void)
{
    pthread_setname_np(pthread_self(), "AITTWorkerLoop");
    main_loop->Run();
}

void AITT::Impl::SetWillInfo(const std::string &topic, const void *data, const int datalen,
      AittQoS qos, bool retain)
{
    mq->SetWillInfo(topic, data, datalen, qos, retain);
}

void AITT::Impl::SetConnectionCallback(ConnectionCallback cb, void *user_data)
{
    if (cb) {
        mq->SetConnectionCallback([&, cb, user_data](int status) {
            auto idler_cb = std::bind(&Impl::ConnectionCB, this, cb, user_data, status,
                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            main_loop->AddIdle(idler_cb, nullptr);
        });
    } else {
        mq->SetConnectionCallback(nullptr);
    }
}

int AITT::Impl::ConnectionCB(ConnectionCallback cb, void *user_data, int status,
      MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *loop_data)
{
    RETV_IF(cb == nullptr, AITT_LOOP_EVENT_REMOVE);

    cb(public_api, status, user_data);

    return AITT_LOOP_EVENT_REMOVE;
}

void AITT::Impl::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    try {
        discovery.Start(host, port, username, password);
        mq->Connect(host, port, username, password);
    } catch (std::exception &e) {
        ERR("Connect() Fail(%s)", e.what());
        mq->Disconnect();
        discovery.Stop();
        throw;
    }
    mqtt_broker_ip_ = host;
    mqtt_broker_port_ = port;
}

void AITT::Impl::Disconnect(void)
{
    UnsubscribeAll();
    modules.DestroyStreamAll();

    mqtt_broker_ip_.clear();
    mqtt_broker_port_ = -1;

    discovery.Stop();
    mq->Disconnect();
}

void AITT::Impl::UnsubscribeAll()
{
    std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

    DBG("Subscribed list %zu", subscribed_list.size());

    for (auto subscribe_info : subscribed_list) {
        switch (subscribe_info->first) {
        case AITT_TYPE_MQTT:
            mq->Unsubscribe(subscribe_info->second);
            mq_discovery_handler.Unsubscribe(subscribe_info->second);
            break;
        case AITT_TYPE_TCP:
        case AITT_TYPE_TCP_SECURE:
            modules.Get(subscribe_info->first).Unsubscribe(subscribe_info->second);
            break;

        default:
            ERR("Unknown AittProtocol(%d)", subscribe_info->first);
            break;
        }

        delete subscribe_info;
    }
    subscribed_list.clear();
}

void AITT::Impl::Publish(const std::string &topic, const void *data, const int datalen,
      AittProtocol protocols, AittQoS qos, bool retain)
{
    if (discovery.IsRunning() == false) {
        ERR("Not connected");
        throw AittException(AittException::INVALID_STATE);
    }
    if ((protocols & ~(AITT_TYPE_MQTT | AITT_TYPE_TCP | AITT_TYPE_TCP_SECURE)) != 0) {
        ERR("Unknown Protocol(%d)", protocols);
        throw AittException(AittException::INVALID_ARG);
    }

    if ((protocols & AITT_TYPE_MQTT) == AITT_TYPE_MQTT)
        mq->Publish(topic, data, datalen, qos, retain);

    if ((protocols & AITT_TYPE_TCP) == AITT_TYPE_TCP)
        modules.Get(AITT_TYPE_TCP).Publish(topic, data, datalen, qos, retain);

    if ((protocols & AITT_TYPE_TCP_SECURE) == AITT_TYPE_TCP_SECURE)
        modules.Get(AITT_TYPE_TCP_SECURE).Publish(topic, data, datalen, qos, retain);
}

AittSubscribeID AITT::Impl::Subscribe(const std::string &topic, const AITT::SubscribeCallback &cb,
      void *user_data, AittProtocol protocol, AittQoS qos)
{
    SubscribeInfo *info = new SubscribeInfo();
    info->first = protocol;

    void *subscribe_handle;
    switch (protocol) {
    case AITT_TYPE_MQTT:
        subscribe_handle = SubscribeMQ(info, main_loop.get(), topic, cb, user_data, qos);
        break;
    case AITT_TYPE_TCP:
    case AITT_TYPE_TCP_SECURE:
        subscribe_handle = SubscribeTCP(info, topic, cb, user_data, qos);
        break;
    default:
        ERR("Unknown AittProtocol(%d)", protocol);
        delete info;
        throw AittException(AittException::INVALID_ARG);
    }
    info->second = subscribe_handle;
    {
        std::unique_lock<std::mutex> lock(subscribed_list_mutex_);
        subscribed_list.push_back(info);
    }

    INFO("Subscribe topic(%s) : %p", topic.c_str(), info);
    return reinterpret_cast<AittSubscribeID>(info);
}

AittSubscribeID AITT::Impl::SubscribeMQ(SubscribeInfo *handle, MainLoopIface *loop_handle,
      const std::string &topic, const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    RETV_IF(nullptr == loop_handle, nullptr);

    AittSubscribeID subscribe_handle = mq->Subscribe(
          topic,
          [this, handle, loop_handle, cb](AittMsg *msg, const void *data, const int datalen,
                void *mq_user_data) {
              void *delivery = malloc(datalen);
              if (delivery)
                  memcpy(delivery, data, datalen);

              msg->SetID(handle);
              auto idler_cb =
                    std::bind(&Impl::DetachedCB, this, cb, *msg, delivery, datalen, mq_user_data,
                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
              loop_handle->AddIdle(idler_cb, nullptr);
          },
          user_data, qos);

    mq_discovery_handler.Subscribe(subscribe_handle, topic);

    return subscribe_handle;
}

int AITT::Impl::DetachedCB(SubscribeCallback cb, AittMsg msg, void *data, const int datalen,
      void *user_data, MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *loop_data)
{
    RETV_IF(cb == nullptr, AITT_LOOP_EVENT_REMOVE);

    cb(&msg, data, datalen, user_data);

    free(data);
    return AITT_LOOP_EVENT_REMOVE;
}

void *AITT::Impl::Unsubscribe(AittSubscribeID subscribe_id)
{
    INFO("subscribe_id : %p", subscribe_id);
    SubscribeInfo *info = reinterpret_cast<SubscribeInfo *>(subscribe_id);

    std::unique_lock<std::mutex> lock(subscribed_list_mutex_);

    auto it = std::find(subscribed_list.begin(), subscribed_list.end(), info);
    if (it == subscribed_list.end()) {
        ERR("Unknown subscribe_id(%p)", subscribe_id);
        throw AittException(AittException::NO_DATA_ERR);
    }

    void *user_data = nullptr;
    SubscribeInfo *found_info = *it;
    switch (found_info->first) {
    case AITT_TYPE_MQTT:
        user_data = mq->Unsubscribe(found_info->second);
        mq_discovery_handler.Unsubscribe(found_info->second);
        break;
    case AITT_TYPE_TCP:
    case AITT_TYPE_TCP_SECURE:
        user_data = modules.Get(found_info->first).Unsubscribe(found_info->second);
        break;

    default:
        ERR("Unknown AittProtocol(%d)", found_info->first);
        break;
    }

    subscribed_list.erase(it);
    delete info;

    return user_data;
}

// It's not supported with multiple protocols like subscribe.
void AITT::Impl::PublishWithReply(const std::string &topic, const void *data, const int datalen,
      AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb, void *user_data,
      const std::string &correlation)
{
    std::string replyTopic = topic + RESPONSE_POSTFIX + std::to_string(reply_id++);

    auto id = Subscribe(
          replyTopic,
          [this, cb](AittMsg *sub_msg, const void *sub_data, const int sub_datalen,
                void *sub_cbdata) {
              if (sub_msg->IsEndSequence()) {
                  try {
                      Unsubscribe(sub_msg->GetID());
                  } catch (AittException &e) {
                      ERR("Unsubscribe() Fail(%s)", e.what());
                  }
              }
              cb(sub_msg, sub_data, sub_datalen, sub_cbdata);
          },
          user_data, protocol, qos);

    switch (protocol) {
    case AITT_TYPE_MQTT:
        mq->PublishWithReply(topic, data, datalen, qos, retain, replyTopic, correlation);
        break;
    case AITT_TYPE_TCP:
    case AITT_TYPE_TCP_SECURE:
        modules.Get(protocol).PublishWithReply(topic, data, datalen, qos, retain, replyTopic,
              correlation);
        break;
    default:
        ERR("Unknown AittProtocol(%d)", protocol);
        Unsubscribe(id);
        throw AittException(AittException::INVALID_ARG);
    }
}

int AITT::Impl::PublishWithReplySync(const std::string &topic, const void *data, const int datalen,
      AittProtocol protocol, AittQoS qos, bool retain, const SubscribeCallback &cb, void *user_data,
      const std::string &correlation, int timeout_ms)
{
    if (protocol != AITT_TYPE_MQTT)
        return -1;  // not yet support

    std::string replyTopic = topic + RESPONSE_POSTFIX + std::to_string(reply_id++);
    MainLoopIface *sync_loop = MainLoopHandler::new_loop();
    SubscribeInfo *info = new SubscribeInfo();
    void *subscribe_handle;
    unsigned int timeout_id = 0;
    bool is_timeout = false;

    info->first = protocol;

    subscribe_handle = SubscribeMQ(
          info, sync_loop, replyTopic,
          [&](AittMsg *sub_msg, const void *sub_data, const int sub_datalen, void *sub_cbdata) {
              if (sub_msg->IsEndSequence()) {
                  try {
                      Unsubscribe(sub_msg->GetID());
                  } catch (AittException &e) {
                      ERR("Unsubscribe() Fail(%s)", e.what());
                  }
                  sync_loop->Quit();
              } else {
                  if (timeout_id) {
                      sync_loop->RemoveTimeout(timeout_id);
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

    sync_loop->Run();
    delete sync_loop;

    if (is_timeout)
        return AITT_ERROR_TIMED_OUT;
    return 0;
}

void AITT::Impl::HandleTimeout(int timeout_ms, unsigned int &timeout_id, MainLoopIface *sync_loop,
      bool &is_timeout)
{
    timeout_id = sync_loop->AddTimeout(
          timeout_ms,
          [&, sync_loop, timeout_ms](MainLoopIface::Event result, int fd,
                MainLoopIface::MainLoopData *data) -> int {
              ERR("PublishWithReplySync() timeout(%d)", timeout_ms);
              sync_loop->Quit();
              is_timeout = true;
              return AITT_LOOP_EVENT_REMOVE;
          },
          nullptr);
}

void AITT::Impl::SendReply(AittMsg *msg, const void *data, const int datalen, bool end)
{
    RET_IF(msg == nullptr);

    if (end == false || msg->GetSequence())
        msg->IncreaseSequence();
    msg->SetEndSequence(end);

    switch (msg->GetProtocol()) {
    case AITT_TYPE_MQTT:
        mq->SendReply(msg, data, datalen, AITT_QOS_AT_MOST_ONCE, false);
        break;
    case AITT_TYPE_TCP:
    case AITT_TYPE_TCP_SECURE:
        modules.Get(msg->GetProtocol()).SendReply(msg, data, datalen, AITT_QOS_AT_MOST_ONCE, false);
        break;
    default:
        ERR("Unknown AittProtocol(%d)", msg->GetProtocol());
        break;
    }
}

void *AITT::Impl::SubscribeTCP(SubscribeInfo *handle, const std::string &topic,
      const SubscribeCallback &cb, void *user_data, AittQoS qos)
{
    auto protocol = handle->first;
    return modules.Get(protocol).Subscribe(
          topic,
          [handle, cb, protocol](AittMsg *msg, const void *data, const int datalen,
                void *userdata) {
              msg->SetID(handle);
              msg->SetProtocol(protocol);

              return cb(msg, data, datalen, userdata);
          },
          user_data, qos);
}

AittStream *AITT::Impl::CreateStream(AittStreamProtocol type, const std::string &topic,
      AittStreamRole role)
{
    AittStream *stream = modules.CreateStream(type, topic, role);
    discovery.Restart();

    return stream;
}

void AITT::Impl::DestroyStream(AittStream *aitt_stream)
{
    modules.DestroyStream(aitt_stream);
}

int AITT::Impl::CountSubscriber(const std::string &topic, AittProtocol protocols)
{
    if (topic.find("+") != std::string::npos || topic.find("#") != std::string::npos) {
        ERR("Not Support Wildcard in CountSubscriber");
        return AITT_ERROR_NOT_SUPPORTED;
    }

    int total = 0;
    if (protocols & AITT_TYPE_MQTT)
        total += mq_discovery_handler.CountSubscriber(topic);

    if (protocols & AITT_TYPE_TCP)
        total += modules.Get(AITT_TYPE_TCP).CountSubscriber(topic);

    if (protocols & AITT_TYPE_TCP_SECURE)
        total += modules.Get(AITT_TYPE_TCP_SECURE).CountSubscriber(topic);

    return total;
}

}  // namespace aitt
