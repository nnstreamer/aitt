/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
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
#include "aitt_c.h"

#include <arpa/inet.h>
#include <stdlib.h>

#include "AITT.h"
#include "log.h"

using namespace aitt;

struct aitt_handle {
    aitt_handle() : aitt(nullptr) {}
    AITT *aitt;
    std::string id;
    std::string ip;
};

API aitt_h aitt_new(const char *id)
{
    aitt_h handle = nullptr;
    try {
        handle = new aitt_handle();
        if (id)
            handle->id = id;
    } catch (std::exception &e) {
        ERR("new() Fail(%s)", e.what());
        return nullptr;
    }

    return handle;
}

API int aitt_set_option(aitt_h handle, aitt_option_e option, const char *value)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);

    switch (option) {
    case AITT_OPT_MY_IP:
        try {
            if (value)
                handle->ip = value;
            else
                handle->ip.clear();
        } catch (std::exception &e) {
            ERR("string() Fail(%s)", e.what());
            return AITT_ERROR_SYSTEM;
        }
        break;
    default:
        ERR("Unknown option(%d)", option);
        return AITT_ERROR_INVALID_PARAMETER;
    }

    return AITT_ERROR_NONE;
}

API const char *aitt_get_option(aitt_h handle, aitt_option_e option)
{
    RETV_IF(handle == nullptr, nullptr);

    switch (option) {
    case AITT_OPT_MY_IP:
        if (handle->ip.empty())
            return nullptr;
        else
            return handle->ip.c_str();
    default:
        ERR("Unknown option(%d)", option);
    }

    return nullptr;
}

API void aitt_destroy(aitt_h handle)
{
    if (handle == nullptr) {
        ERR("handle is NULL");
        return;
    }

    try {
        delete handle->aitt;
        delete handle;
    } catch (std::exception &e) {
        ERR("delete() Fail(%s)", e.what());
    }
}

static bool is_valid_ip(const char *ip)
{
    RETV_IF(ip == nullptr, false);

    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) <= 0)
        return false;

    return true;
}

API int aitt_connect(aitt_h handle, const char *broker_ip, int port)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETVM_IF(is_valid_ip(broker_ip) == false, AITT_ERROR_INVALID_PARAMETER, "Invalid IP(%s)",
          broker_ip);

    try {
        handle->aitt = new AITT(handle->id, handle->ip, true);
        handle->aitt->Connect(broker_ip, port);
    } catch (std::exception &e) {
        ERR("Connect(%s, %d) Fail(%s)", broker_ip, port, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_disconnect(aitt_h handle)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        handle->aitt->Disconnect();
    } catch (std::exception &e) {
        ERR("Disconnect() Fail(%s)", e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_publish(aitt_h handle, const char *topic, const void *msg, const size_t msg_len)
{
    return aitt_publish_full(handle, topic, msg, msg_len, AITT_TYPE_MQTT, 0);
}

API int aitt_publish_full(aitt_h handle, const char *topic, const void *msg, const size_t msg_len,
      int protocols, int qos)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(topic == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(msg == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        handle->aitt->Publish(topic, msg, msg_len, AITT_TYPE_MQTT);
    } catch (std::exception &e) {
        ERR("Publish(topic:%s, msg_len:%zu) Fail(%s)", topic, msg_len, e.what());
        return AITT_ERROR_SYSTEM;
    }

    return AITT_ERROR_NONE;
}

API int aitt_publish_with_reply(aitt_h handle, const char *topic, const void *msg,
      const size_t msg_len, aitt_protocol_e protocols, int qos, const char *correlation,
      aitt_sub_fn cb, void *user_data)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(topic == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(msg == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(cb == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        // TODO: handle protocols, qos
        handle->aitt->PublishWithReply(topic, msg, msg_len, protocols, AITT::QoS::AT_LEAST_ONCE,
              false, cb, user_data, std::string(correlation));
    } catch (std::exception &e) {
        ERR("PublishWithReply(%s) Fail(%s)", topic, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_send_reply(aitt_h handle, aitt_msg_h msg_handle, const void *reply,
      const size_t reply_len, bool end)
{
    try {
        aitt::MSG *msg = static_cast<aitt::MSG *>(msg_handle);

        handle->aitt->SendReply(msg, reply, reply_len, end);
    } catch (std::exception &e) {
        ERR("SendReply Fail(%s)", e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_subscribe(aitt_h handle, const char *topic, aitt_sub_fn cb, void *user_data,
      aitt_sub_h *sub_handle)
{
    return aitt_subscribe_full(handle, topic, cb, user_data, AITT_TYPE_MQTT, 0, sub_handle);
}

API int aitt_subscribe_full(aitt_h handle, const char *topic, aitt_sub_fn cb, void *user_data,
      aitt_protocol_e protocol, int qos, aitt_sub_h *sub_handle)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(topic == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(cb == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(sub_handle == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        // TODO: handle protocols, qos
        *sub_handle =
              handle->aitt->Subscribe(topic, cb, user_data, static_cast<AittProtocol>(protocol));
    } catch (std::exception &e) {
        ERR("Subscribe(%s) Fail(%s)", topic, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_unsubscribe(aitt_h handle, aitt_sub_h sub_handle)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(sub_handle == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        handle->aitt->Unsubscribe(static_cast<AittSubscribeID>(sub_handle));
    } catch (std::exception &e) {
        ERR("Unsubscribe(%p) Fail(%s)", sub_handle, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API const char *aitt_msg_get_topic(aitt_msg_h handle)
{
    RETV_IF(handle == nullptr, nullptr);

    MSG *msg = reinterpret_cast<MSG *>(handle);
    return msg->GetTopic().c_str();
}
