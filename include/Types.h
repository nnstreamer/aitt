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

typedef void* AittSubscribeID;

enum AittProtocol {
    AITT_TYPE_MQTT = (0x1 << 0),    // Publish message through the MQTT
    AITT_TYPE_TCP = (0x1 << 1),     // Publish message to peers using the TCP
    AITT_TYPE_UDP = (0x1 << 2),     // Publish message to peers using the UDP
    AITT_TYPE_SRTP = (0x1 << 3),    // Publish message to peers using the SRTP
    AITT_TYPE_WEBRTC = (0x1 << 4),  // Publish message to peers using the WEBRTC
};

#ifdef TIZEN
#include <tizen.h>
#define TIZEN_ERROR_AITT -0x04020000
#else
#include <errno.h>

#define TIZEN_ERROR_NONE 0
#define TIZEN_ERROR_INVALID_PARAMETER -EINVAL
#define TIZEN_ERROR_PERMISSION_DENIED -EACCES
#define TIZEN_ERROR_OUT_OF_MEMORY -ENOMEM
#define TIZEN_ERROR_TIMED_OUT (-1073741824LL + 1)
#define TIZEN_ERROR_NOT_SUPPORTED (-1073741824LL + 2)
#define TIZEN_ERROR_AITT -0x04020000
#endif

enum AittError {
    AITT_ERROR_NONE = TIZEN_ERROR_NONE,                           /**< On Success */
    AITT_ERROR_INVALID_PARAMETER = TIZEN_ERROR_INVALID_PARAMETER, /**< Invalid parameter */
    AITT_ERROR_PERMISSION_DENIED = TIZEN_ERROR_PERMISSION_DENIED, /**< Permission denied */
    AITT_ERROR_OUT_OF_MEMORY = TIZEN_ERROR_OUT_OF_MEMORY,         /**< Out of memory */
    AITT_ERROR_TIMED_OUT = TIZEN_ERROR_TIMED_OUT,                 /**< Time out */
    AITT_ERROR_NOT_SUPPORTED = TIZEN_ERROR_NOT_SUPPORTED,         /**< Not supported */
    AITT_ERROR_UNKNOWN = TIZEN_ERROR_AITT | 0x01,                 /**< Unknown Error */
    AITT_ERROR_SYSTEM = TIZEN_ERROR_AITT | 0x02,                  /**< System errors */
};
