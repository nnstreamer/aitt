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
#pragma once

#include <sys/syscall.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "aitt_internal_definitions.h"
#include "aitt_platform.h"

#if defined(SYS_gettid)
#define GETTID() syscall(SYS_gettid)
#else   // SYS_gettid
#define GETTID() 0lu
#endif  // SYS_gettid

#define AITT_ERRMSG_LEN 80

#ifdef TIZEN_RT
#define _POSIX_C_SOURCE 200809L
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 0
#endif
#pragma GCC system_header
#endif

#if (_POSIX_C_SOURCE >= 200112L) && !_GNU_SOURCE
#define AITT_STRERROR_R(errno, buf, buflen)                   \
    do {                                                      \
        int strerror_ret = strerror_r(errno, buf, buflen);    \
        if (strerror_ret != 0) {                              \
            assert(strerror_ret == 0 && "strerror_r failed"); \
        }                                                     \
    } while (0)
#else
#define AITT_STRERROR_R(errno, buf, buflen)                   \
    do {                                                      \
        const char *errstr = strerror_r(errno, buf, buflen);  \
        if (errstr == nullptr) {                              \
            assert(errstr != nullptr && "strerror_r failed"); \
        }                                                     \
    } while (0)
#endif

#ifdef _LOG_WITH_TIMESTAMP
#include "aitt_internal_profiling.h"
#else
#define DBG(fmt, ...) PLATFORM_LOGD("[%lu] " fmt, GETTID(), ##__VA_ARGS__)

#define INFO(fmt, ...) PLATFORM_LOGI("[%lu] " fmt, GETTID(), ##__VA_ARGS__)
#define ERR(fmt, ...) PLATFORM_LOGE("[%lu] \033[31m" fmt "\033[0m", GETTID(), ##__VA_ARGS__)
#define ERR_CODE(_aitt_errno, fmt, ...)                                                 \
    do {                                                                                \
        char errMsg[AITT_ERRMSG_LEN] = {'\0'};                                          \
        int _errno = (_aitt_errno);                                                     \
        AITT_STRERROR_R(_errno, errMsg, sizeof(errMsg));                                \
        PLATFORM_LOGE("[%lu] (%d:%s) \033[31m" fmt "\033[0m", GETTID(), _errno, errMsg, \
              ##__VA_ARGS__);                                                           \
    } while (0)

#define DBG_HEX_DUMP(data, len)                                            \
    do {                                                                   \
        size_t i;                                                          \
        char dump[len * 3];                                                \
        for (i = 0; i < (size_t)len; i++) {                                \
            snprintf(dump + i * 3, (len * 3) - (i * 3), "%02X ", data[i]); \
        }                                                                  \
        DBG("%s", dump);                                                   \
    } while (0)
#endif

#define RET_IF(expr)            \
    do {                        \
        if (expr) {             \
            ERR("(%s)", #expr); \
            return;             \
        }                       \
    } while (0)

#define RETV_IF(expr, val)      \
    do {                        \
        if (expr) {             \
            ERR("(%s)", #expr); \
            return (val);       \
        }                       \
    } while (0)

#define RETVM_IF(expr, val, fmt, ...) \
    do {                              \
        if (expr) {                   \
            ERR(fmt, ##__VA_ARGS__);  \
            return (val);             \
        }                             \
    } while (0)
