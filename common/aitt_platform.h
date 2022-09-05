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

#ifndef LOG_TAG
#define LOG_TAG "AITT"
#endif

#define LOG_RED "\033[31m"
#define LOG_GREEN "\033[32m"
#define LOG_BROWN "\033[33m"
#define LOG_BLUE "\033[34m"
#define LOG_END "\033[0m"

#if defined(PLATFORM) && !defined(LOG_STDOUT)

#define HEADER PLATFORM/aitt_platform.h
#define TO_STR(x) #x
#define DEFINE_TO_STR(x) TO_STR(x)
#include DEFINE_TO_STR(HEADER)
#undef TO_STR
#undef DEFINE_TO_STR

#else  // PLATFORM

#include <libgen.h>
#include <stdio.h>

#define __FILENAME__ basename((char *)(__FILE__))
#define PLATFORM_LOGD(fmt, ...)                                                                  \
    fprintf(stdout, LOG_BROWN "[%s]%s(%s:%d)" LOG_END fmt "\n", LOG_TAG, __func__, __FILENAME__, \
          __LINE__, ##__VA_ARGS__)
#define PLATFORM_LOGI(fmt, ...)                                                                  \
    fprintf(stdout, LOG_GREEN "[%s]%s(%s:%d)" LOG_END fmt "\n", LOG_TAG, __func__, __FILENAME__, \
          __LINE__, ##__VA_ARGS__)
#define PLATFORM_LOGE(fmt, ...)                                                                \
    fprintf(stderr, LOG_RED "[%s]%s(%s:%d)" LOG_END fmt "\n", LOG_TAG, __func__, __FILENAME__, \
          __LINE__, ##__VA_ARGS__)

#endif  // PLATFORM
