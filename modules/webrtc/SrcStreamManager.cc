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
#include "SrcStreamManager.h"

#include <sstream>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {

SrcStreamManager::SrcStreamManager(const std::string &topic, const std::string &aitt_id,
      const std::string &thread_id)
      : StreamManager(topic, aitt_id, thread_id), stream_(nullptr), stream_ready_cb_(nullptr)
{
}

SrcStreamManager::~SrcStreamManager()
{
    Stop();
}

void SrcStreamManager::SetStreamReadyCallback(StreamReadyCallback cb)
{
    stream_ready_cb_ = cb;
}

void SrcStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    auto on_stream_state_changed_cb =
          std::bind(OnStreamStateChanged, std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_signaling_state_notify_cb =
          std::bind(OnSignalingStateNotify, std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnSignalingStateNotifyCb(on_signaling_state_notify_cb);

    auto on_ice_gathering_state_changed_cb =
          std::bind(OnIceGatheringStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);
}

void SrcStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSrcStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
}

void SrcStreamManager::OnAnswerCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SrcStreamManager::OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream)
{
    DBG("OnSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
    if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
        auto on_answer_created =
              std::bind(OnAnswerCreated, std::placeholders::_1, std::ref(stream));
        stream.CreateAnswerAsync(on_answer_created);
    }
}

void SrcStreamManager::OnIceGatheringStateNotify(WebRtcState::IceGathering state,
      WebRtcStream &stream, SrcStreamManager *manager)
{
    DBG("Src IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
    if (state == WebRtcState::IceGathering::COMPLETE) {
        if (manager && manager->stream_ready_cb_)
            manager->stream_ready_cb_(stream);
    }
}

void SrcStreamManager::Start(void)
{
    // TODO: What'll be done in start Src Stream Manager?
}

void SrcStreamManager::AddStream(const std::string &id, const std::vector<uint8_t> &message)
{
    // TODO Add more streams on same topic
    if (stream_)
        return;

    stream_ = new WebRtcStream();
    SetWebRtcStreamCallbacks(*stream_);
    stream_->Create(true, false);
    stream_->AttachCameraSource();
    stream_->Start();

    std::stringstream s_stream;
    s_stream << static_cast<void *>(&stream_);

    stream_->SetStreamId(std::string(aitt_id_ + thread_id_ + s_stream.str()));
    stream_->SetPeerId(id);
    stream_->AddDiscoveryInformation(message);

    return;
}

void SrcStreamManager::Stop(void)
{
    // TODO: Ad-hoc method
    if (stream_) {
        delete stream_;
        stream_ = nullptr;
    }
}

void SrcStreamManager::HandleRemovedClient(const std::string &id)
{
    if (stream_ && stream_->GetPeerId().compare(id) == 0) {
        delete stream_;
        stream_ = nullptr;
    }
}

void SrcStreamManager::HandleDiscoveredStream(const std::string &id,
      const std::vector<uint8_t> &message)
{
    auto info = WebRtcMessage::ParseDiscoveryMessage(message);
    if (info.is_src_ || info.topic_.compare(topic_)) {
        DBG("Is src or topic not matched");
        return;
    }

    if (stream_) {
        DBG("Supports one stream at once currently");
        return;
    }

    AddStream(id, message);
}

}  // namespace AittWebRTCNamespace
