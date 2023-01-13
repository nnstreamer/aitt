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
#include "MosquittoMQ.h"

#include <mqtt_protocol.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <stdexcept>
#include <thread>

#include "AittException.h"
#include "AittTypes.h"
#include "aitt_internal.h"

namespace aitt {

const std::string MosquittoMQ::REPLY_SEQUENCE_NUM_KEY = "sequenceNum";
const std::string MosquittoMQ::REPLY_IS_END_SEQUENCE_KEY = "isEndSequence";

MosquittoMQ::MosquittoMQ(const std::string &id, bool clean_session)
      : handle(nullptr),
        keep_alive(60),
        subscribers_iterating(false),
        subscriber_iterator_updated(false),
        connect_cb(nullptr)
{
    do {
        int ret = mosquitto_lib_init();
        if (ret != MOSQ_ERR_SUCCESS) {
            ERR("mosquitto_lib_init() Fail(%s)", mosquitto_strerror(ret));
            break;
        }

        handle = mosquitto_new(id.c_str(), clean_session, this);
        if (handle == nullptr) {
            ERR("mosquitto_new(%s, %d) Fail", id.c_str(), clean_session);
            break;
        }

        ret = mosquitto_int_option(handle, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        if (ret != MOSQ_ERR_SUCCESS) {
            ERR("mosquitto_int_option() Fail(%s)", mosquitto_strerror(ret));
            break;
        }

        mosquitto_message_v5_callback_set(handle, MessageCallback);
        mosquitto_connect_v5_callback_set(handle, ConnectCallback);
        mosquitto_disconnect_v5_callback_set(handle, DisconnectCallback);

        return;
    } while (0);

    mosquitto_destroy(handle);
    mosquitto_lib_cleanup();
    throw AittException(AittException::MQTT_ERR, std::string("MosquittoMQ Constructor Error"));
}

MosquittoMQ::~MosquittoMQ(void)
{
    int ret;
    INFO("Destructor");

    callback_lock.lock();
    connect_cb = nullptr;
    subscribers.clear();
    callback_lock.unlock();

    mosquitto_destroy(handle);

    ret = mosquitto_lib_cleanup();
    if (ret != MOSQ_ERR_SUCCESS)
        ERR("mosquitto_lib_cleanup() Fail(%s)", mosquitto_strerror(ret));
}

void MosquittoMQ::SetConnectionCallback(const MQConnectionCallback &cb)
{
    std::lock_guard<std::recursive_mutex> lock_from_here(callback_lock);
    connect_cb = cb;
}

void MosquittoMQ::ConnectCallback(struct mosquitto *mosq, void *obj, int rc, int flag,
      const mosquitto_property *props)
{
    RET_IF(obj == nullptr);
    MosquittoMQ *mq = static_cast<MosquittoMQ *>(obj);

    INFO("Connected : rc(%d), flag(%d)", rc, flag);

    std::lock_guard<std::recursive_mutex> lock_from_here(mq->callback_lock);
    if (mq->connect_cb)
        mq->connect_cb((rc == CONNACK_ACCEPTED) ? AITT_CONNECTED : AITT_CONNECT_FAILED);
}

void MosquittoMQ::DisconnectCallback(struct mosquitto *mosq, void *obj, int rc,
      const mosquitto_property *props)
{
    RET_IF(obj == nullptr);
    MosquittoMQ *mq = static_cast<MosquittoMQ *>(obj);

    INFO("Disconnected : rc(%d)", rc);

    std::lock_guard<std::recursive_mutex> lock_from_here(mq->callback_lock);
    if (mq->connect_cb)
        mq->connect_cb(AITT_DISCONNECTED);
}

void MosquittoMQ::Connect(const std::string &host, int port, const std::string &username,
      const std::string &password)
{
    int ret;

    if (username.empty() == false) {
        ret = mosquitto_username_pw_set(handle, username.c_str(), password.c_str());
        if (ret != MOSQ_ERR_SUCCESS) {
            ERR("mosquitto_username_pw_set(%s, %s) Fail(%s)", username.c_str(), password.c_str(),
                  mosquitto_strerror(ret));
            throw AittException(AittException::MQTT_ERR);
        }
    }

    ret = mosquitto_loop_start(handle);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_loop_start() Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_connect(handle, host.c_str(), port, keep_alive);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_connect(%s, %d) Fail(%s)", host.c_str(), port, mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
}

void MosquittoMQ::SetWillInfo(const std::string &topic, const void *msg, int szmsg, int qos,
      bool retain)
{
    int ret = mosquitto_will_set(handle, topic.c_str(), szmsg, msg, qos, retain);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_will_set(%s) Fail(%s)", topic.c_str(), mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
}

void MosquittoMQ::Disconnect(void)
{
    int ret = mosquitto_disconnect(handle);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_disconnect() Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_loop_stop(handle, false);
    if (ret != MOSQ_ERR_SUCCESS)
        ERR("mosquitto_loop_stop() Fail(%s)", mosquitto_strerror(ret));

    mosquitto_will_clear(handle);
}

void MosquittoMQ::MessageCallback(mosquitto *handle, void *obj, const mosquitto_message *msg,
      const mosquitto_property *props)
{
    RET_IF(obj == nullptr);
    MosquittoMQ *mq = static_cast<MosquittoMQ *>(obj);

    mq->MessageCB(msg, props);
}

void MosquittoMQ::MessageCB(const mosquitto_message *msg, const mosquitto_property *props)
{
    std::lock_guard<std::recursive_mutex> auto_lock(callback_lock);
    subscribers_iterating = true;
    subscriber_iterator = subscribers.begin();
    while (subscriber_iterator != subscribers.end()) {
        auto subscribe_data = *(subscriber_iterator);
        if (nullptr == subscribe_data) {
            ERR("Invalid subscribe data");
            ++subscriber_iterator;
            continue;
        }

        if (CompareTopic(subscribe_data->topic.c_str(), msg->topic))
            InvokeCallback(*subscriber_iterator, msg, props);

        if (!subscriber_iterator_updated)
            ++subscriber_iterator;
        else
            subscriber_iterator_updated = false;
    }
    subscribers_iterating = false;
    subscribers.insert(subscribers.end(), new_subscribers.begin(), new_subscribers.end());
    new_subscribers.clear();
}

void MosquittoMQ::InvokeCallback(SubscribeData *subscriber, const mosquitto_message *msg,
      const mosquitto_property *props)
{
    RET_IF(nullptr == subscriber);

    AittMsg mq_msg;
    mq_msg.SetTopic(msg->topic);
    if (props) {
        const mosquitto_property *prop;

        char *response_topic = nullptr;
        prop = mosquitto_property_read_string(props, MQTT_PROP_RESPONSE_TOPIC, &response_topic,
              false);
        if (prop) {
            mq_msg.SetResponseTopic(response_topic);
            free(response_topic);
        }

        void *correlation = nullptr;
        uint16_t correlation_size = 0;
        prop = mosquitto_property_read_binary(props, MQTT_PROP_CORRELATION_DATA, &correlation,
              &correlation_size, false);
        if (prop == nullptr || correlation == nullptr)
            ERR("No Correlation Data");

        mq_msg.SetCorrelation(std::string((char *)correlation, correlation_size));
        if (correlation)
            free(correlation);

        char *name = nullptr;
        char *value = nullptr;
        prop = mosquitto_property_read_string_pair(props, MQTT_PROP_USER_PROPERTY, &name, &value,
              false);
        while (prop) {
            if (REPLY_SEQUENCE_NUM_KEY == name) {
                mq_msg.SetSequence(std::stoi(value));
            } else if (REPLY_IS_END_SEQUENCE_KEY == name) {
                mq_msg.SetEndSequence(std::stoi(value) == 1);
            } else {
                ERR("Unsupported property(%s, %s)", name, value);
            }
            free(name);
            free(value);

            prop = mosquitto_property_read_string_pair(prop, MQTT_PROP_USER_PROPERTY, &name, &value,
                  true);
        }
    }

    subscriber->cb(&mq_msg, msg->topic, msg->payload, msg->payloadlen, subscriber->user_data);
}

void MosquittoMQ::Publish(const std::string &topic, const void *data, const int datalen, int qos,
      bool retain)
{
    int mid = -1;
    int ret = mosquitto_publish(handle, &mid, topic.c_str(), datalen, data, qos, retain);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_publish(%s) Fail(%s)", topic.c_str(), mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
}

void MosquittoMQ::PublishWithReply(const std::string &topic, const void *data, const int datalen,
      int qos, bool retain, const std::string &reply_topic, const std::string &correlation)
{
    int ret;
    int mid = -1;
    mosquitto_property *props = nullptr;

    ret = mosquitto_property_add_string(&props, MQTT_PROP_RESPONSE_TOPIC, reply_topic.c_str());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_property_add_string(response-topic) Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_property_add_binary(&props, MQTT_PROP_CORRELATION_DATA, correlation.c_str(),
          correlation.size());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_property_add_binary(correlation) Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
    ret = mosquitto_publish_v5(handle, &mid, topic.c_str(), datalen, data, qos, retain, props);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_publish_v5(%s) Fail(%s)", topic.c_str(), mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
}

void MosquittoMQ::SendReply(AittMsg *msg, const void *data, const int datalen, int qos, bool retain)
{
    RET_IF(msg == nullptr);

    int ret;
    int mId = -1;
    mosquitto_property *props = nullptr;

    ret = mosquitto_property_add_binary(&props, MQTT_PROP_CORRELATION_DATA,
          msg->GetCorrelation().c_str(), msg->GetCorrelation().size());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_property_add_binary(correlation) Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_property_add_string_pair(&props, MQTT_PROP_USER_PROPERTY,
          REPLY_SEQUENCE_NUM_KEY.c_str(), std::to_string(msg->GetSequence()).c_str());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_property_add_string_pair(squenceNum) Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_property_add_string_pair(&props, MQTT_PROP_USER_PROPERTY,
          REPLY_IS_END_SEQUENCE_KEY.c_str(), std::to_string(msg->IsEndSequence()).c_str());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_property_add_string_pair(IsEndSequence) Fail(%s)", mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    ret = mosquitto_publish_v5(handle, &mId, msg->GetResponseTopic().c_str(), datalen, data, qos,
          retain, props);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_publish_v5(%s) Fail(%s)", msg->GetResponseTopic().c_str(),
              mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }
}

void *MosquittoMQ::Subscribe(const std::string &topic, const SubscribeCallback &cb, void *user_data,
      int qos)
{
    int mid = -1;
    int ret = mosquitto_subscribe(handle, &mid, topic.c_str(), qos);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_subscribe(%s) Fail(%s)", topic.c_str(), mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    std::lock_guard<std::recursive_mutex> lock_from_here(callback_lock);
    SubscribeData *data = new SubscribeData(topic, cb, user_data);
    if (subscribers_iterating)
        new_subscribers.push_back(data);
    else
        subscribers.push_back(data);

    return static_cast<void *>(data);
}

void *MosquittoMQ::Unsubscribe(void *sub_handle)
{
    RETV_IF(nullptr == sub_handle, nullptr);

    std::lock_guard<std::recursive_mutex> auto_lock(callback_lock);
    auto it = std::find(subscribers.begin(), subscribers.end(),
          static_cast<SubscribeData *>(sub_handle));

    if (it == subscribers.end()) {
        ERR("No Subscription(%p)", sub_handle);
        throw AittException(AittException::NO_DATA_ERR);
    }

    SubscribeData *data = static_cast<SubscribeData *>(sub_handle);

    if (subscriber_iterator == it) {
        subscriber_iterator = subscribers.erase(it);
        subscriber_iterator_updated = true;
    } else {
        subscribers.erase(it);
    }

    void *user_data = data->user_data;
    std::string topic = data->topic;
    delete data;

    int mid = -1;
    int ret = mosquitto_unsubscribe(handle, &mid, topic.c_str());
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_unsubscribe(%s) Fail(%s)", topic.c_str(), mosquitto_strerror(ret));
        throw AittException(AittException::MQTT_ERR);
    }

    return user_data;
}

bool MosquittoMQ::CompareTopic(const std::string &left, const std::string &right)
{
    bool result = false;
    int ret = mosquitto_topic_matches_sub(left.c_str(), right.c_str(), &result);
    if (ret != MOSQ_ERR_SUCCESS) {
        ERR("mosquitto_topic_matches_sub(%s, %s) Fail(%s)", left.c_str(), right.c_str(),
              mosquitto_strerror(ret));
        return false;
    }
    return result;
}

MosquittoMQ::SubscribeData::SubscribeData(const std::string &in_topic,
      const SubscribeCallback &in_cb, void *in_user_data)
      : topic(in_topic), cb(in_cb), user_data(in_user_data)
{
}

}  // namespace aitt
