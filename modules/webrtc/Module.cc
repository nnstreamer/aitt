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
      : is_source_(IsSource(role)), discovery_(discovery), state_cb_user_data_(nullptr), receive_cb_user_data_(nullptr)
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
}

Module::~Module(void)
{
    discovery_.RemoveDiscoveryCB(discovery_cb_);
    if (stream_manager_)
        delete stream_manager_;
}

void Module::SetConfig(const std::string &key, const std::string &value)
{
}

void Module::SetConfig(const std::string &key, void *obj)
{
}


void Module::Start(void)
{
    auto on_ice_candidate_added =
          std::bind(&Module::OnIceCandidateAdded, this, std::placeholders::_1);
    stream_manager_->SetIceCandidateAddedCallback(on_ice_candidate_added);

    auto on_stream_ready = std::bind(&Module::OnStreamReady, this, std::placeholders::_1);
    stream_manager_->SetStreamReadyCallback(on_stream_ready);

    auto on_stream_started = std::bind(&Module::OnStreamStarted, this);
    stream_manager_->SetStreamStartCallback(on_stream_started);

    auto on_stream_stopped = std::bind(&Module::OnStreamStopped, this);
    stream_manager_->SetStreamStopCallback(on_stream_stopped);

    stream_manager_->Start();
}

void Module::Stop(void)
{
}

void Module::OnIceCandidateAdded(WebRtcStream &stream)
{
    DBG("OnIceCandidateAdded");
    auto msg = stream_manager_->GetDiscoveryMessage();
    discovery_.UpdateDiscoveryMsg(stream_manager_->GetTopic(), msg.data(), msg.size());
}

void Module::OnStreamReady(WebRtcStream &stream)
{
    DBG("OnStreamReady");

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

void Module::SetStateCallback(StateCallback cb, void *user_data)
{
    state_callback_ = cb;
    state_cb_user_data_ = user_data;
}

void Module::SetReceiveCallback(ReceiveCallback cb, void *user_data)
{
    receive_callback_ = cb;
    receive_cb_user_data_ = user_data;
}
bool Module::IsSource(AittStreamRole role)
{
    return role == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER;
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        stream_manager_->HandleRemovedClient(clientId);
        return;
    }

    stream_manager_->HandleMsg(clientId,
          std::vector<uint8_t>(static_cast<const uint8_t *>(msg),
                static_cast<const uint8_t *>(msg) + szmsg));
}

}  // namespace AittWebRTCNamespace
