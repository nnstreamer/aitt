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

#include <AittTypes.h>

#include <functional>
#include <string>

class API AittMsg {
  public:
    AittMsg();

    void SetID(AittSubscribeID id);
    AittSubscribeID GetID() const;
    void SetTopic(const std::string &topic);
    const std::string &GetTopic() const;
    void SetCorrelation(const std::string &correlation);
    const std::string &GetCorrelation() const;
    void SetResponseTopic(const std::string &reply_topic);
    const std::string &GetResponseTopic() const;
    void SetSequence(int num);
    void IncreaseSequence();
    int GetSequence() const;
    void SetEndSequence(bool end);
    bool IsEndSequence() const;
    void SetProtocol(AittProtocol protocol);
    AittProtocol GetProtocol() const;

  private:
    std::string topic_;
    std::string correlation_;
    std::string reply_topic_;
    int sequence;
    bool end_sequence;
    AittSubscribeID id_;
    AittProtocol protocol_;
};

using AittMsgCB =
      std::function<void(AittMsg *msg, const void *data, const int data_len, void *user_data)>;
