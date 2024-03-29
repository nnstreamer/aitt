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
#include "AittMsg.h"

AittMsg::AittMsg() : sequence(0), end_sequence(true), id_(nullptr), protocol_(AITT_TYPE_MQTT)
{
}

void AittMsg::SetID(AittSubscribeID id)
{
    id_ = id;
}

AittSubscribeID AittMsg::GetID() const
{
    return id_;
}

void AittMsg::SetTopic(const std::string& topic)
{
    topic_ = topic;
}

const std::string& AittMsg::GetTopic() const
{
    return topic_;
}

void AittMsg::SetCorrelation(const std::string& correlation)
{
    correlation_ = correlation;
}

const std::string& AittMsg::GetCorrelation() const
{
    return correlation_;
}

void AittMsg::SetResponseTopic(const std::string& replyTopic)
{
    reply_topic_ = replyTopic;
}

const std::string& AittMsg::GetResponseTopic() const
{
    return reply_topic_;
}

void AittMsg::SetSequence(int num)
{
    sequence = num;
}

void AittMsg::IncreaseSequence()
{
    sequence++;
}

int AittMsg::GetSequence() const
{
    return sequence;
}

void AittMsg::SetEndSequence(bool end)
{
    end_sequence = end;
}

bool AittMsg::IsEndSequence() const
{
    return end_sequence;
}

void AittMsg::SetProtocol(AittProtocol protocol)
{
    protocol_ = protocol;
}

AittProtocol AittMsg::GetProtocol() const
{
    return protocol_;
}
