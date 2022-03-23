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

#include <string>

#include "Types.h"

namespace aitt {
class MSG {
  public:
    MSG();

    void SetID(AittSubscribeID id);
    AittSubscribeID GetID();
    void SetTopic(const std::string &topic);
    const std::string &GetTopic();
    void SetCorrelation(const std::string &correlation);
    const std::string &GetCorrelation();
    void SetResponseTopic(const std::string &reply_topic);
    const std::string &GetResponseTopic();
    void SetSequence(int num);
    void IncreaseSequence();
    int GetSequence();
    void SetEndSequence(bool end);
    bool IsEndSequence();
    void SetProtocols(AittProtocol protocols);
    AittProtocol GetProtocols();

  protected:
    std::string topic_;
    std::string correlation_;
    std::string reply_topic_;
    int sequence;
    bool end_sequence;
    AittSubscribeID id_;
    AittProtocol protocols_;
};
}  // namespace aitt
