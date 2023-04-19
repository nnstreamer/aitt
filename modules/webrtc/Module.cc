/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "Module.h"

#include <SinkStreamManager.h>
#include <SrcStreamManager.h>
#include <flatbuffers/flexbuffers.h>

#include <sstream>
#include <thread>

#include "AittException.h"
#include "aitt_internal.h"

namespace AittWebRTCNamespace {

Module::Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role)
      : is_source_(role == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER),
        discovery_(discovery),
        state_cb_user_data_(nullptr),
        receive_cb_user_data_(nullptr)
{
    std::stringstream s_stream;
    s_stream << std::this_thread::get_id();

    if (is_source_)
        stream_manager_ = new SrcStreamManager(topic, discovery_.GetId(), s_stream.str());
    else
        stream_manager_ = new SinkStreamManager(topic, discovery_.GetId(), s_stream.str());

    discovery_cb_ = discovery_.AddDiscoveryCB(stream_manager_->GetWatchingTopic(),
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    stream_manager_->SetIceCandidateAddedCallback(std::bind(&Module::OnIceCandidateAdded, this));
    stream_manager_->SetStreamStartCallback(std::bind(&Module::OnStreamStarted, this));
    stream_manager_->SetStreamStopCallback(std::bind(&Module::OnStreamStopped, this));
    stream_manager_->SetStreamStateCallback(
          std::bind(&Module::OnStreamState, this, std::placeholders::_1));
}

Module::~Module(void)
{
    discovery_.RemoveDiscoveryCB(discovery_cb_);
    if (stream_manager_)
        delete stream_manager_;
}

AittStream *Module::SetConfig(const std::string &key, const std::string &value)
{
    if (stream_manager_->IsStarted())
        throw aitt::AittException(aitt::AittException::RESOURCE_BUSY_ERR);

    try {
        if (key == "WIDTH" && std::stoi(value) > 0)
            stream_manager_->SetWidth(std::stoi(value));
        else if (key == "HEIGHT" && std::stoi(value) > 0)
            stream_manager_->SetHeight(std::stoi(value));
        else if (key == "FRAME_RATE")
            stream_manager_->SetFrameRate(std::stoi(value));
        else if (key == "MEDIA_FORMAT")
            stream_manager_->SetMediaFormat(value);
        else if (key == "SOURCE_TYPE")
            stream_manager_->SetSourceType(value);
        else if (key == "DECODE_CODEC")
            stream_manager_->SetDecodeCodec(value);
        else
            throw aitt::AittException(aitt::AittException::INVALID_ARG);
    } catch (std::exception &e) {
        DBG("exception while setting");
        throw aitt::AittException(aitt::AittException::INVALID_ARG);
    }
    return this;
}

AittStream *Module::SetConfig(const std::string &key, void *obj)
{
    return this;
}

std::string Module::GetFormat(void)
{
    return stream_manager_->GetFormat();
}

int Module::GetWidth(void)
{
    return stream_manager_->GetWidth();
}

int Module::GetHeight(void)
{
    return stream_manager_->GetHeight();
}

void Module::Start(void)
{
    stream_manager_->Start();
}

void Module::Stop(void)
{
}

int Module::Push(void *obj)
{
    // TODO: We need to classify error codes
    return stream_manager_->Push(obj);
}

void Module::OnIceCandidateAdded(void)
{
    DBG("OnIceCandidateAdded");
    auto msg = stream_manager_->GetDiscoveryMessage();
    discovery_.UpdateDiscoveryMsg(stream_manager_->GetTopic(), msg.data(), msg.size());
}

void Module::OnStreamStarted(void)
{
    std::vector<uint8_t> msg;

    flexbuffers::Builder fbb;
    fbb.String("START");
    fbb.Finish();

    msg = fbb.GetBuffer();
    discovery_.UpdateDiscoveryMsg(stream_manager_->GetTopic(), msg.data(), msg.size());
}

void Module::OnStreamStopped(void)
{
    std::vector<uint8_t> msg;

    flexbuffers::Builder fbb;
    fbb.String("STOP");
    fbb.Finish();

    msg = fbb.GetBuffer();
    discovery_.UpdateDiscoveryMsg(stream_manager_->GetTopic(), msg.data(), msg.size());
}

void Module::OnStreamState(const std::string &state)
{
    if (!state_callback_)
        return;

    DBG("%s", state.c_str());
    if (state == "IDLE")
        state_callback_(this, AITT_STREAM_STATE_INIT, state_cb_user_data_);
    else if (state == "PLAYING")
        state_callback_(this, AITT_STREAM_STATE_PLAYING, state_cb_user_data_);
    else if (state == "UNDERFLOW")
        state_callback_(this, AITT_STREAM_STATE_UNDERFLOW, state_cb_user_data_);
    else if (state == "OVERFLOW")
        state_callback_(this, AITT_STREAM_STATE_OVERFLOW, state_cb_user_data_);
}

void Module::SetStateCallback(StateCallback cb, void *user_data)
{
    state_callback_ = cb;
    state_cb_user_data_ = user_data;
}

void Module::SetReceiveCallback(ReceiveCallback cb, void *user_data)
{
    receive_callback_ = cb;
    receive_cb_user_data_ = user_data;
    stream_manager_->SetOnFrameCallback(std::bind(receive_callback_,
          static_cast<aitt::AittStream *>(this), std::placeholders::_1, receive_cb_user_data_));
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        stream_manager_->HandleRemovedClient(clientId);
        return;
    }

    stream_manager_->HandleMsg(clientId, std::vector<uint8_t>(static_cast<const uint8_t *>(msg),
                                               static_cast<const uint8_t *>(msg) + szmsg));
}

}  // namespace AittWebRTCNamespace
