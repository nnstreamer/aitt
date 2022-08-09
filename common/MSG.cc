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
#include "MSG.h"

namespace aitt {
MSG::MSG() : sequence(0), end_sequence(true), id_(nullptr), protocols_(AITT_TYPE_MQTT)
{
}

void MSG::SetID(AittSubscribeID id)
{
    id_ = id;
}

AittSubscribeID MSG::GetID()
{
    return id_;
}

void MSG::SetTopic(const std::string& topic)
{
    topic_ = topic;
}

const std::string& MSG::GetTopic()
{
    return topic_;
}

void MSG::SetCorrelation(const std::string& correlation)
{
    correlation_ = correlation;
}

const std::string& MSG::GetCorrelation()
{
    return correlation_;
}

void MSG::SetResponseTopic(const std::string& replyTopic)
{
    reply_topic_ = replyTopic;
}

const std::string& MSG::GetResponseTopic()
{
    return reply_topic_;
}

void MSG::SetSequence(int num)
{
    sequence = num;
}

void MSG::IncreaseSequence()
{
    sequence++;
}

int MSG::GetSequence()
{
    return sequence;
}

void MSG::SetEndSequence(bool end)
{
    end_sequence = end;
}

bool MSG::IsEndSequence()
{
    return end_sequence;
}

void MSG::SetProtocols(AittProtocol protocols)
{
    protocols_ = protocols;
}

AittProtocol MSG::GetProtocols()
{
    return protocols_;
}

}  // namespace aitt
