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
      : is_source_(IsSource(role)), topic_(topic), discovery_(discovery)
{
    discovery_cb_ = discovery_.AddDiscoveryCB(topic,
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    std::stringstream s_stream;
    s_stream << std::this_thread::get_id();

    if (is_source_) {
        stream_manager_ = new SrcStreamManager(topic_, discovery_.GetId(), s_stream.str());
    } else {
        stream_manager_ = new SinkStreamManager(topic_, discovery_.GetId(), s_stream.str());
    }
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
    auto on_stream_ready = std::bind(&Module::OnStreamReady, this, std::placeholders::_1);
    stream_manager_->SetStreamReadyCallback(on_stream_ready);
    stream_manager_->Start();
}

void Module::Stop(void)
{
}

void Module::OnStreamReady(WebRtcStream &stream)
{
    DBG("OnStreamReady");

    auto discovery_message = WebRtcMessage::GenerateDiscoveryMessage(topic_, is_source_,
          stream.GetLocalDescription(), stream.GetIceCandidates());
    discovery_.UpdateDiscoveryMsg(topic_, discovery_message.data(), discovery_message.size());
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
    if (!clientId.compare(discovery_.GetId()))
        return;

    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        stream_manager_->HandleRemovedClient(clientId);
        return;
    }

    stream_manager_->HandleDiscoveredStream(clientId,
          std::vector<uint8_t>(static_cast<const uint8_t *>(msg),
                static_cast<const uint8_t *>(msg) + szmsg));
}

}  // namespace AittWebRTCNamespace
