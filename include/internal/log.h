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

#include <libgen.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include "aitt_platform.h"

#if !defined(REVISION)
#define REVISION "not defined"
#endif

#if defined(SYS_gettid)
#define GETTID() syscall(SYS_gettid)
#else  // SYS_gettid
#define GETTID() 0lu
#endif  // SYS_gettid

#if !defined(PLATFORM_LOGD)
#define PLATFORM_LOGD(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#endif
#if !defined(PLATFORM_LOGI)
#define PLATFORM_LOGI(fmt, ...) fprintf(stdout, fmt, ##__VA_ARGS__)
#endif
#if !defined(PLATFORM_LOGE)
#define PLATFORM_LOGE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#define AITT_ERRMSG_LEN 80

// Increase this if you need more space for profiling
#define AITT_PROFILE_ID_MAX 8

struct __aitt__tls__ {
    struct timeval last_timestamp;
    struct timeval profile_timestamp[AITT_PROFILE_ID_MAX];
    char profile_idx;  // Max to 255
    char initialized;  // Max to 255, but we only use 0 or 1
};

extern __thread struct __aitt__tls__ __aitt;

#if (_POSIX_C_SOURCE >= 200112L) && !_GNU_SOURCE
#define AITT_STRERROR_R(errno, buf, buflen)          \
    do {                                             \
        int ret = strerror_r(errno, buf, buflen);    \
        if (ret != 0) {                              \
            assert(ret == 0 && "strerror_r failed"); \
        }                                            \
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

#define InitAITT()                                                \
    do {                                                          \
        if (__aitt.initialized == 0) {                            \
            __aitt.initialized = 1;                               \
            if (gettimeofday(&__aitt.last_timestamp, NULL) < 0) { \
                assert(!"gettimeofday failed");                   \
            }                                                     \
        }                                                         \
    } while (0)

#define PROFILE_MARK()                                                                   \
    do {                                                                                 \
        if (__aitt.profile_idx < AITT_PROFILE_ID_MAX) {                                  \
            if (gettimeofday(__aitt.profile_timestamp + __aitt.profile_idx, NULL) < 0) { \
                assert(!"gettimeofday failed");                                          \
            }                                                                            \
            ++__aitt.profile_idx;                                                        \
        } else {                                                                         \
            ERR("Unable to mark a profile point: %d\n", __aitt.profile_idx);             \
        }                                                                                \
    } while (0)

#define PROFILE_ESTIMATE(tres)                                                                    \
    do {                                                                                          \
        struct timeval tv;                                                                        \
        struct timeval res;                                                                       \
        if (gettimeofday(&tv, NULL) < 0) {                                                        \
            assert(!"gettimeofday failed");                                                       \
        }                                                                                         \
        --__aitt.profile_idx;                                                                     \
        timersub(&tv, __aitt.profile_timestamp + __aitt.profile_idx, &res);                       \
        (tres) = static_cast<double>(res.tv_sec) + static_cast<double>(res.tv_usec) / 1000000.0f; \
    } while (0)

#define PROFILE_RESET(count)           \
    do {                               \
        __aitt.profile_idx -= (count); \
    } while (0)

#if defined(_LOG_WITH_TIMESTAMP)
#if defined(NDEBUG)
#define DBG(fmt, ...)
#else
#define DBG(fmt, ...)                                                                     \
    do {                                                                                  \
        struct timeval tv;                                                                \
        struct timeval res;                                                               \
        InitAITT();                                                                       \
        if (gettimeofday(&tv, NULL) < 0) {                                                \
            assert(!"gettimeofday failed");                                               \
        }                                                                                 \
        timersub(&tv, &__aitt_last_timestamp, &res);                                      \
        PLATFORM_LOGD("[%lu] %lu.%.6lu(%lu.%.6lu) " fmt, GETTID(), tv.tv_sec, tv.tv_usec, \
              res.tv_sec, res.tv_usec, ##__VA_ARGS__);                                    \
        __aitt_last_timestamp.tv_sec = tv.tv_sec;                                         \
        __aitt_last_timestamp.tv_usec = tv.tv_usec;                                       \
    } while (0)
#endif

#define INFO(fmt, ...)                                                                    \
    do {                                                                                  \
        struct timeval tv;                                                                \
        struct timeval res;                                                               \
        InitAITT();                                                                       \
        if (gettimeofday(&tv, NULL) < 0) {                                                \
            assert(!"gettimeofday failed");                                               \
        }                                                                                 \
        timersub(&tv, &__aitt_last_timestamp, &res);                                      \
        PLATFORM_LOGI("[%lu] %lu.%.6lu(%lu.%.6lu) " fmt, GETTID(), tv.tv_sec, tv.tv_usec, \
              res.tv_sec, res.tv_usec, ##__VA_ARGS__);                                    \
        __aitt_last_timestamp.tv_sec = tv.tv_sec;                                         \
        __aitt_last_timestamp.tv_usec = tv.tv_usec;                                       \
    } while (0)

#define ERR(fmt, ...)                                                                           \
    do {                                                                                        \
        struct timeval tv;                                                                      \
        struct timeval res;                                                                     \
        InitAITT();                                                                             \
        if (gettimeofday(&tv, NULL) < 0) {                                                      \
            assert(!"gettimeofday failed");                                                     \
        }                                                                                       \
        timersub(&tv, &__aitt_last_timestamp, &res);                                            \
        PLATFORM_LOGE("[%lu] %lu.%.6lu(%lu.%.6lu) \033[31m" fmt "\033[0m", GETTID(), tv.tv_sec, \
              tv.tv_usec, res.tv_sec, res.tv_usec, ##__VA_ARGS__);                              \
        __aitt_last_timestamp.tv_sec = tv.tv_sec;                                               \
        __aitt_last_timestamp.tv_usec = tv.tv_usec;                                             \
    } while (0)

#define ERR_CODE(_aitt_errno, fmt, ...)                                                       \
    do {                                                                                      \
        struct timeval tv;                                                                    \
        struct timeval res;                                                                   \
        char errMsg[AITT_ERRMSG_LEN] = {'\0'};                                                \
        int _errno = (_aitt_errno);                                                           \
                                                                                              \
        AITT_STRERROR_R(_errno, errMsg, sizeof(errMsg));                                      \
                                                                                              \
        InitAITT();                                                                           \
        if (gettimeofday(&tv, NULL) < 0) {                                                    \
            assert(!"gettimeofday failed");                                                   \
        }                                                                                     \
        timersub(&tv, &__aitt_last_timestamp, &res);                                          \
        PLATFORM_LOGE("[%lu] %lu.%.6lu(%lu.%.6lu) (%d:%s) \033[31m" fmt "\033[0m", GETTID(),  \
              tv.tv_sec, tv.tv_usec, res.tv_sec, res.tv_usec, _errno, errMsg, ##__VA_ARGS__); \
        __aitt_last_timestamp.tv_sec = tv.tv_sec;                                             \
        __aitt_last_timestamp.tv_usec = tv.tv_usec;                                           \
    } while (0)

#else  // _LOG_WITH_TIMESTAMP
#if defined(NDEBUG)
#define DBG(fmt, ...)
#else
#define DBG(fmt, ...) PLATFORM_LOGD("[%lu] " fmt, GETTID(), ##__VA_ARGS__)
#endif

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

#endif  // _LOG_WITH_TIMESTAMP
