/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include "aitt_internal.h"

using namespace aitt;

struct aitt_handle {
    aitt_handle() : aitt(nullptr), connected(false), custom_broker(false) {}
    AITT *aitt;
    bool connected;
    bool custom_broker;
};

struct aitt_option {
    aitt_option() : my_ip(nullptr) {}
    const char *my_ip;
    AittOption option;
};

API aitt_h aitt_new(const char *id, aitt_option_h option)
{
    aitt_h handle = nullptr;
    try {
        std::string valid_id;
        std::string valid_ip;

        if (id)
            valid_id = id;

        handle = new aitt_handle();
        if (option) {
            if (option->my_ip)
                valid_ip = option->my_ip;
            handle->custom_broker = option->option.GetUseCustomMqttBroker();
            handle->aitt = new AITT(valid_id, valid_ip, option->option);
        } else {
            handle->aitt = new AITT(valid_id, valid_ip);
        }
    } catch (std::exception &e) {
        ERR("new() Fail(%s)", e.what());
        return nullptr;
    }

    return handle;
}

API void aitt_destroy(aitt_h handle)
{
    RET_IF(handle == nullptr);

    try {
        delete handle->aitt;
        delete handle;
    } catch (std::exception &e) {
        ERR("delete() Fail(%s)", e.what());
    }
}

API aitt_option_h aitt_option_new()
{
    aitt_option_h handle = nullptr;

    try {
        handle = new aitt_option();
    } catch (std::exception &e) {
        ERR("new() Fail(%s)", e.what());
    }

    return handle;
}

API void aitt_option_destroy(aitt_option_h handle)
{
    RET_IF(handle == nullptr);

    try {
        delete handle;
    } catch (std::exception &e) {
        ERR("delete() Fail(%s)", e.what());
    }
}

static int _to_boolean(const char *value, bool &dest)
{
    if (value) {
        dest = (STR_EQ == strcasecmp(value, "true"));
        if (false == dest && STR_EQ != strcasecmp(value, "false")) {
            ERR("Unknown value(%s)", value);
            return AITT_ERROR_INVALID_PARAMETER;
        }
    } else {
        dest = false;
    }
    return AITT_ERROR_NONE;
}

API int aitt_option_set(aitt_option_h handle, aitt_option_e option, const char *value)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    int ret;
    bool bool_val = false;

    switch (option) {
    case AITT_OPT_MY_IP:
        handle->my_ip = value;
        break;

    case AITT_OPT_CLEAN_SESSION:
        ret = _to_boolean(value, bool_val);
        if (ret == AITT_ERROR_NONE)
            handle->option.SetCleanSession(bool_val);
        return ret;

    case AITT_OPT_CUSTOM_BROKER:
        ret = _to_boolean(value, bool_val);
        if (ret == AITT_ERROR_NONE)
            handle->option.SetUseCustomMqttBroker(bool_val);
        return ret;

    case AITT_OPT_SERVICE_ID:
        return handle->option.SetServiceID(value);

    case AITT_OPT_LOCATION_ID:
        return handle->option.SetLocationID(value);

    case AITT_OPT_ROOT_CA:
        return handle->option.SetRootCA(value);

    case AITT_OPT_CUSTOM_RW_FILE:
        return handle->option.SetCustomRWFile(value);

    default:
        ERR("Unknown option(%d)", option);
        return AITT_ERROR_INVALID_PARAMETER;
    }

    return AITT_ERROR_NONE;
}

API const char *aitt_option_get(aitt_option_h handle, aitt_option_e option)
{
    RETV_IF(handle == nullptr, nullptr);

    switch (option) {
    case AITT_OPT_MY_IP:
        return handle->my_ip;
    case AITT_OPT_CLEAN_SESSION:
        return (handle->option.GetCleanSession()) ? "true" : "false";
    case AITT_OPT_CUSTOM_BROKER:
        return (handle->option.GetUseCustomMqttBroker()) ? "true" : "false";
    case AITT_OPT_SERVICE_ID:
        return handle->option.GetServiceID();
    case AITT_OPT_LOCATION_ID:
        return handle->option.GetLocationID();
    case AITT_OPT_ROOT_CA:
        return handle->option.GetRootCA();
    case AITT_OPT_CUSTOM_RW_FILE:
        return handle->option.GetCustomRWFile();
    default:
        ERR("Unknown option(%d)", option);
    }

    return nullptr;
}

API int aitt_will_set(aitt_h handle, const char *topic, const void *msg, const int msg_len,
      aitt_qos_e qos, bool retain)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        handle->aitt->SetWillInfo(topic, msg, msg_len, qos, retain);
    } catch (std::exception &e) {
        ERR("SetWillInfo(%s, %d) Fail(%s)", topic, msg_len, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

static bool is_valid_ip(const char *ip)
{
    RETV_IF(ip == nullptr, false);

    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) <= 0)
        return false;

    return true;
}

API int aitt_connect(aitt_h handle, const char *host, int port)
{
    return aitt_connect_full(handle, host, port, NULL, NULL);
}

API int aitt_set_connect_callback(aitt_h handle, aitt_connect_cb cb, void *user_data)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        if (cb) {
            handle->aitt->SetConnectionCallback(
                  [handle, cb, user_data](AITT &aitt, int status, void *data) {
                      cb(handle, status, user_data);
                  });
        } else {
            handle->aitt->SetConnectionCallback(nullptr);
        }
    } catch (std::exception &e) {
        ERR("SetConnectionCallback() Fail(%s)", e.what());
        return AITT_ERROR_SYSTEM;
    }

    return AITT_ERROR_NONE;
}

API int aitt_connect_full(aitt_h handle, const char *host, int port, const char *username,
      const char *password)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    if (handle->custom_broker)
        RETV_IF(host == nullptr, AITT_ERROR_INVALID_PARAMETER);
    else
        RETVM_IF(!is_valid_ip(host), AITT_ERROR_INVALID_PARAMETER, "Invalid IP(%s)", host);

    try {
        handle->aitt->Connect(host, port, username ? username : std::string(),
              password ? password : std::string());
    } catch (std::exception &e) {
        ERR("Connect(%s, %d) Fail(%s)", host, port, e.what());
        return AITT_ERROR_SYSTEM;
    }

    handle->connected = true;
    return AITT_ERROR_NONE;
}

API int aitt_disconnect(aitt_h handle)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->connected == false, AITT_ERROR_NOT_READY);

    try {
        handle->aitt->Disconnect();
    } catch (std::exception &e) {
        ERR("Disconnect() Fail(%s)", e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_publish(aitt_h handle, const char *topic, const void *msg, const int msg_len)
{
    return aitt_publish_full(handle, topic, msg, msg_len, AITT_TYPE_MQTT, AITT_QOS_AT_MOST_ONCE);
}

API int aitt_publish_full(aitt_h handle, const char *topic, const void *msg, const int msg_len,
      int protocols, aitt_qos_e qos)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->connected == false, AITT_ERROR_NOT_READY);
    RETV_IF(topic == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(msg == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        handle->aitt->Publish(topic, msg, msg_len, AITT_TYPE_MQTT);
    } catch (std::exception &e) {
        ERR("Publish(topic:%s, msg_len:%d) Fail(%s)", topic, msg_len, e.what());
        return AITT_ERROR_SYSTEM;
    }

    return AITT_ERROR_NONE;
}

API int aitt_publish_with_reply(aitt_h handle, const char *topic, const void *msg,
      const int msg_len, aitt_protocol_e protocols, aitt_qos_e qos, const char *correlation,
      aitt_sub_fn cb, void *user_data)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->connected == false, AITT_ERROR_NOT_READY);
    RETV_IF(topic == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(msg == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(cb == nullptr, AITT_ERROR_INVALID_PARAMETER);

    try {
        // TODO: handle protocols, qos
        handle->aitt->PublishWithReply(topic, msg, msg_len, protocols, AITT_QOS_AT_MOST_ONCE, false,
              cb, user_data, std::string(correlation));
    } catch (std::exception &e) {
        ERR("PublishWithReply(%s) Fail(%s)", topic, e.what());
        return AITT_ERROR_SYSTEM;
    }
    return AITT_ERROR_NONE;
}

API int aitt_send_reply(aitt_h handle, aitt_msg_h msg_handle, const void *reply,
      const int reply_len, bool end)
{
    try {
        AittMsg *msg = static_cast<AittMsg *>(msg_handle);

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
    return aitt_subscribe_full(handle, topic, cb, user_data, AITT_TYPE_MQTT, AITT_QOS_AT_MOST_ONCE,
          sub_handle);
}

API int aitt_subscribe_full(aitt_h handle, const char *topic, aitt_sub_fn cb, void *user_data,
      aitt_protocol_e protocol, aitt_qos_e qos, aitt_sub_h *sub_handle)
{
    RETV_IF(handle == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->aitt == nullptr, AITT_ERROR_INVALID_PARAMETER);
    RETV_IF(handle->connected == false, AITT_ERROR_NOT_READY);
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
    RETV_IF(handle->connected == false, AITT_ERROR_NOT_READY);
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

    AittMsg *msg = reinterpret_cast<AittMsg *>(handle);
    return msg->GetTopic().c_str();
}
