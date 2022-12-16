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
#pragma once

#define API __attribute__((visibility("default")))

typedef void* AittSubscribeID;

enum AittProtocol {
    AITT_TYPE_UNKNOWN = 0,
    AITT_TYPE_MQTT = (0x1 << 0),        // Publish message through the MQTT
    AITT_TYPE_TCP = (0x1 << 1),         // Publish message to peers using the TCP
    AITT_TYPE_TCP_SECURE = (0x1 << 2),  // Publish message to peers using the Secure TCP
};

enum AittStreamProtocol {
    AITT_STREAM_TYPE_WEBRTC,  // Publish message to peers using the WEBRTC
    AITT_STREAM_TYPE_RTSP,    // Publish message to peers using the RTSP
    AITT_STREAM_TYPE_MAX
};

enum AittStreamState {
    AITT_STREAM_STATE_INIT = 0,
    AITT_STREAM_STATE_READY = 1,
    AITT_STREAM_STATE_PLAYING = 2,
};

enum AittStreamRole {
    AITT_STREAM_ROLE_PUBLISHER = 0,   // Role of source media
    AITT_STREAM_ROLE_SUBSCRIBER = 1,  // Role of destination(receiver)
};

// AittQoS only works with the AITT_TYPE_MQTT
enum AittQoS {
    AITT_QOS_AT_MOST_ONCE = 0,   // Fire and forget
    AITT_QOS_AT_LEAST_ONCE = 1,  // Receiver is able to receive multiple times
    AITT_QOS_EXACTLY_ONCE = 2,   // Receiver only receives exactly once
};

enum AittConnectionState {
    AITT_DISCONNECTED = 0,    // The connection is disconnected.
    AITT_CONNECTED = 1,       // A connection was successfully established to the mqtt broker.
    AITT_CONNECT_FAILED = 2,  // Failed to connect to the mqtt broker.
};

// The maximum size in bytes of a message. It follows MQTT
#define AITT_MESSAGE_MAX 268435455

#ifdef TIZEN
#include <tizen.h>
#define TIZEN_ERROR_AITT -0x04020000
#else
#include <errno.h>

#define TIZEN_ERROR_NONE 0
#define TIZEN_ERROR_OUT_OF_MEMORY -ENOMEM
#define TIZEN_ERROR_PERMISSION_DENIED -EACCES
#define TIZEN_ERROR_RESOURCE_BUSY -EBUSY
#define TIZEN_ERROR_INVALID_PARAMETER -EINVAL
#define TIZEN_ERROR_ALREADY_IN_PROGRESS -EALREADY
#define TIZEN_ERROR_TIMED_OUT (-1073741824LL + 1)
#define TIZEN_ERROR_NOT_SUPPORTED (-1073741824LL + 2)
#define TIZEN_ERROR_AITT -0x04020000
#endif

enum AittError {
    AITT_ERROR_NONE = TIZEN_ERROR_NONE,                           /**< On Success */
    AITT_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY,         /**< Out of memory */
    AITT_ERROR_PERMISSION_DENIED = TIZEN_ERROR_PERMISSION_DENIED, /**< Permission denied */
    AITT_ERROR_RESOURCE_BUSY = TIZEN_ERROR_RESOURCE_BUSY,         /**< Device or resource busy */
    AITT_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
    AITT_ERROR_ALREADY = TIZEN_ERROR_ALREADY_IN_PROGRESS, /**< Operation already in progress */
    AITT_ERROR_TIMED_OUT = TIZEN_ERROR_TIMED_OUT,         /**< Time out */
    AITT_ERROR_NOT_SUPPORTED = TIZEN_ERROR_NOT_SUPPORTED, /**< Not supported */
    AITT_ERROR_UNKNOWN = TIZEN_ERROR_AITT | 0x01,         /**< Unknown Error */
    AITT_ERROR_SYSTEM = TIZEN_ERROR_AITT | 0x02,          /**< System errors */
    AITT_ERROR_NOT_READY = TIZEN_ERROR_AITT | 0x03,       /**< Not available */
};
