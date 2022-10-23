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
#include "SinkStreamManager.h"

#include <sstream>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {
SinkStreamManager::SinkStreamManager(const std::string &topic, const std::string &aitt_id,
      const std::string &thread_id)
      : StreamManager(topic, aitt_id, thread_id)
{
    std::stringstream s_stream;
    s_stream << static_cast<void *>(&stream_);
    stream_.SetStreamId(aitt_id + thread_id + s_stream.str());
}
SinkStreamManager::~SinkStreamManager()
{
    Stop();
}

void SinkStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    auto on_stream_state_changed_cb =
          std::bind(OnStreamStateChanged, std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_ice_gathering_state_changed_cb =
          std::bind(OnIceGatheringStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);

    auto on_encoded_frame_cb = std::bind(OnEncodedFrame, std::ref(stream), this);
    stream.GetEventHandler().SetOnEncodedFrameCb(on_encoded_frame_cb);
}

void SinkStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    if (state == WebRtcState::Stream::NEGOTIATING) {
        auto on_offer_created = std::bind(OnOfferCreated, std::placeholders::_1, std::ref(stream));
        stream.CreateOfferAsync(on_offer_created);
    }
}

void SinkStreamManager::OnOfferCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SinkStreamManager::OnIceGatheringStateNotify(WebRtcState::IceGathering state,
      WebRtcStream &stream, SinkStreamManager *manager)
{
    DBG("Sink IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
    if (state == WebRtcState::IceGathering::COMPLETE) {
        if (manager && manager->stream_ready_cb_)
            manager->stream_ready_cb_(stream);
    }
}

void SinkStreamManager::OnEncodedFrame(WebRtcStream &stream, SinkStreamManager *manager)
{
    if (manager && manager->encoded_frame_cb_)
        manager->encoded_frame_cb_(stream);
}

void SinkStreamManager::SetStreamReadyCallback(StreamReadyCallback cb)
{
    stream_ready_cb_ = cb;
}

void SinkStreamManager::SetOnEncodedFrameCallback(EncodedFrameCallabck cb)
{
    encoded_frame_cb_ = cb;
}

void SinkStreamManager::Start()
{
    // TODO: Handle failures on create and start
    SetWebRtcStreamCallbacks(stream_);
    stream_.Create(false, false);
    stream_.Start();
}

void SinkStreamManager::Stop()
{
    stream_.Destroy();
}

void SinkStreamManager::HandleRemovedClient(const std::string &id)
{
    if (id.compare(stream_.GetPeerId()) == 0) {
        stream_.Destroy();
        stream_.Create(false, false);
        stream_.Start();
    }

    return;
}

void SinkStreamManager::HandleDiscoveredStream(const std::string &id,
      const std::vector<uint8_t> &message)
{
    auto info = WebRtcMessage::ParseDiscoveryMessage(message);
    if (!info.is_src_ || info.topic_.compare(topic_)) {
        DBG("Is sink or topic not matched");
        return;
    }

    // Sink'll update offer if WebRTC state is ice candidate compelete
    // Src create answer after it receives the offer.
    // So, We don't need to worry about state.
    stream_.AddDiscoveryInformation(message);
}

}  // namespace AittWebRTCNamespace
