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

#include <flatbuffers/flexbuffers.h>

#include <sstream>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {

SrcStreamManager::SrcStreamManager(const std::string &topic, const std::string &aitt_id,
      const std::string &thread_id)
      : StreamManager(topic + "/SRC", aitt_id, thread_id), watching_topic_(topic + "/SINK")
{
}

SrcStreamManager::~SrcStreamManager()
{
    // TODO: You should take care about stream resource
    for (auto itr = sink_streams_.begin(); itr != sink_streams_.end(); ++itr)
        itr->second->Destroy();
    sink_streams_.clear();
}

void SrcStreamManager::Start(void)
{
    DBG("%s %s", __func__, GetTopic().c_str());
    if (stream_start_cb_)
        stream_start_cb_();
}

void SrcStreamManager::Stop(void)
{
    DBG("%s %s", __func__, GetTopic().c_str());
    // TODO: You should take care about stream resource
    for (auto itr = sink_streams_.begin(); itr != sink_streams_.end(); ++itr)
        itr->second->Destroy();
    sink_streams_.clear();

    if (stream_stop_cb_)
        stream_stop_cb_();
}

void SrcStreamManager::SetIceCandidateAddedCallback(IceCandidateAddedCallback cb)
{
    ice_candidate_added_cb_ = cb;
}

void SrcStreamManager::SetStreamReadyCallback(StreamReadyCallback cb)
{
    stream_ready_cb_ = cb;
}

void SrcStreamManager::SetStreamStartCallback(StreamStartCallback cb)
{
    stream_start_cb_ = cb;
}

void SrcStreamManager::SetStreamStopCallback(StreamStopCallback cb)
{
    stream_stop_cb_ = cb;
}

void SrcStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    auto on_stream_state_changed_cb = std::bind(&SrcStreamManager::OnStreamStateChanged, this,
          std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_ice_candidate_added_cb = std::bind(&SrcStreamManager::OnIceCandidate, this,
          std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnIceCandidateCb(on_ice_candidate_added_cb);

    auto on_signaling_state_notify_cb = std::bind(&SrcStreamManager::OnSignalingStateNotify, this,
          std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnSignalingStateNotifyCb(on_signaling_state_notify_cb);

    auto on_ice_gathering_state_changed_cb = std::bind(&SrcStreamManager::OnIceGatheringStateNotify,
          this, std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);
}

void SrcStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSrcStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
}

void SrcStreamManager::OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream)
{
    DBG("OnSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
    if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
        auto on_answer_created = std::bind(&SrcStreamManager::OnAnswerCreated, this,
              std::placeholders::_1, std::ref(stream));
        stream.CreateAnswerAsync(on_answer_created);
    }
}

void SrcStreamManager::OnAnswerCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SrcStreamManager::OnIceCandidate(const std::string &candidate, WebRtcStream &stream)
{
    if (ice_candidate_added_cb_)
        ice_candidate_added_cb_(stream);
}

void SrcStreamManager::OnIceGatheringStateNotify(WebRtcState::IceGathering state,
      WebRtcStream &stream)
{
    DBG("Src IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
    if (state == WebRtcState::IceGathering::COMPLETE) {
        if (stream_ready_cb_)
            stream_ready_cb_(stream);
    }
}

void SrcStreamManager::HandleRemovedClient(const std::string &discovery_id)
{
    auto sink_stream_itr = sink_streams_.find(discovery_id);
    if (sink_stream_itr == sink_streams_.end()) {
        DBG("There's no sink stream %s", discovery_id.c_str());
        return;
    }

    // TODO: You should take care about stream resource
    sink_stream_itr->second->Destroy();
    sink_streams_.erase(sink_stream_itr);

    return;
}

void SrcStreamManager::HandleMsg(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    if (flexbuffers::GetRoot(message).IsString())
        HandleStreamState(discovery_id, message);
    else if (flexbuffers::GetRoot(message).IsVector())
        HandleStreamInfo(discovery_id, message);
}

void SrcStreamManager::HandleStreamState(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    auto sink_state = flexbuffers::GetRoot(message).ToString();

    if (sink_state.compare("STOP") == 0)
        HandleRemovedClient(discovery_id);
    else
        DBG("Invalid message %s", sink_state);
}

void SrcStreamManager::HandleStreamInfo(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    if (!WebRtcMessage::IsValidStreamInfo(message)) {
        DBG("Invalid streams info");
        return;
    }

    // sink_streams have a stream at normal situation
    auto sink_streams = flexbuffers::GetRoot(message).AsVector();
    for (size_t stream_idx = 0; stream_idx < sink_streams.size(); ++stream_idx) {
        auto stream = sink_streams[stream_idx].AsMap();
        auto id = stream["id"].AsString().str();
        auto peer_id = stream["peer_id"].AsString().str();
        auto sdp = stream["sdp"].AsString().str();
        std::vector<std::string> ice_candidates;
        auto ice_info = stream["ice_candidates"].AsVector();
        for (size_t ice_idx = 0; ice_idx < ice_info.size(); ++ice_idx)
            ice_candidates.push_back(ice_info[ice_idx].AsString().str());
        UpdateStreamInfo(discovery_id, id, peer_id, sdp, ice_candidates);
    }
}

void SrcStreamManager::UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
      const std::string &peer_id, const std::string &sdp,
      const std::vector<std::string> &ice_candidates)
{
    auto sink_stream = sink_streams_.find(discovery_id);
    if (sink_stream == sink_streams_.end())
        AddStream(discovery_id, id, sdp, ice_candidates);
    else
        sink_stream->second->UpdatePeerInformation(ice_candidates);
}

void SrcStreamManager::AddStream(const std::string &discovery_id, const std::string &id,
      const std::string &sdp, const std::vector<std::string> &ice_candidates)
{
    auto stream = new WebRtcStream();
    SetWebRtcStreamCallbacks(*stream);
    stream->Create(true, false);
    stream->AttachCameraSource();
    stream->Start();

    std::stringstream s_stream;
    s_stream << static_cast<void *>(stream);

    stream->SetStreamId(std::string(thread_id_ + s_stream.str()));
    stream->SetPeerId(id);
    stream->AddPeerInformation(sdp, ice_candidates);

    sink_streams_[discovery_id] = stream;

    return;
}

std::vector<uint8_t> SrcStreamManager::GetDiscoveryMessage(void)
{
    std::vector<uint8_t> message;

    flexbuffers::Builder fbb;
    fbb.Vector([&] {
        for (auto itr = sink_streams_.begin(); itr != sink_streams_.end(); ++itr) {
            fbb.Map([&] {
                fbb.String("id", itr->second->GetStreamId());
                fbb.String("peer_id", itr->second->GetPeerId());
                fbb.String("sdp", itr->second->GetLocalDescription());
                fbb.Vector("ice_candidates", [&]() {
                    for (const auto &candidate : itr->second->GetIceCandidates()) {
                        fbb.String(candidate);
                    }
                });
            });
        }
    });
    fbb.Finish();

    message = fbb.GetBuffer();
    return message;
}

std::string SrcStreamManager::GetWatchingTopic(void)
{
    return watching_topic_;
}

}  // namespace AittWebRTCNamespace
