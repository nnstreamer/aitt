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

#include <AittMsg.h>
#include <AittOption.h>

#include <functional>
#include <string>

#define AITT_MQ_NEW aitt_mq_new
#define TO_STR(s) #s
#define DEFINE_TO_STR(x) TO_STR(x)

namespace aitt {

class MQ {
  public:
    typedef void *(*ModuleEntry)(const char *id, const AittOption &option);

    using SubscribeCallback = std::function<void(AittMsg *msg, const std::string &topic,
          const void *data, const int datalen, void *user_data)>;
    using MQConnectionCallback = std::function<void(int)>;

    static constexpr const char *const MODULE_ENTRY_NAME = DEFINE_TO_STR(AITT_MQ_NEW);

    MQ() = default;
    virtual ~MQ() = default;

    virtual void SetConnectionCallback(const MQConnectionCallback &cb) = 0;
    virtual void Connect(const std::string &host, int port, const std::string &username,
          const std::string &password) = 0;
    virtual void SetWillInfo(const std::string &topic, const void *msg, int szmsg, int qos,
          bool retain) = 0;
    virtual void Disconnect(void) = 0;
    virtual void Publish(const std::string &topic, const void *data, const int datalen, int qos = 0,
          bool retain = false) = 0;
    virtual void PublishWithReply(const std::string &topic, const void *data, const int datalen,
          int qos, bool retain, const std::string &reply_topic, const std::string &correlation) = 0;
    virtual void SendReply(AittMsg *msg, const void *data, const int datalen, int qos,
          bool retain) = 0;
    virtual void *Subscribe(const std::string &topic, const SubscribeCallback &cb,
          void *user_data = nullptr, int qos = 0) = 0;
    virtual void *Unsubscribe(void *handle) = 0;
    virtual bool CompareTopic(const std::string &left, const std::string &right) = 0;
};

}  // namespace aitt

#undef TO_STR
#undef DEFINE_TO_STR
