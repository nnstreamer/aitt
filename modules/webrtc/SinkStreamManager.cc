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

#include <flatbuffers/flexbuffers.h>

#include <sstream>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {
SinkStreamManager::SinkStreamManager(const std::string &topic, const std::string &aitt_id,
      const std::string &thread_id)
      : StreamManager(topic + "/SINK", aitt_id, thread_id), watching_topic_(topic + "/SRC")
{
}

SinkStreamManager::~SinkStreamManager()
{
    for (auto itr = src_stream_.begin(); itr != src_stream_.end(); ++itr)
        itr->second->Destroy();
    src_stream_.clear();
}

void SinkStreamManager::Start()
{
    DBG("%s %s", __func__, GetTopic().c_str());
    if (stream_start_cb_)
        stream_start_cb_();
}

void SinkStreamManager::Stop()
{
    DBG("%s %s", __func__, GetTopic().c_str());
    // TODO: You should take care about stream resource
    for (auto itr = src_stream_.begin(); itr != src_stream_.end(); ++itr)
        itr->second->Destroy();
    src_stream_.clear();

    if (stream_stop_cb_)
        stream_stop_cb_();
}

void SinkStreamManager::OnEncodedFrame(WebRtcStream &stream)
{
    if (encoded_frame_cb_)
        encoded_frame_cb_(stream);
}

void SinkStreamManager::SetIceCandidateAddedCallback(IceCandidateAddedCallback cb)
{
    ice_candidate_added_cb_ = cb;
}

void SinkStreamManager::SetStreamReadyCallback(StreamReadyCallback cb)
{
    stream_ready_cb_ = cb;
}

void SinkStreamManager::SetStreamStartCallback(StreamStartCallback cb)
{
    stream_start_cb_ = cb;
}

void SinkStreamManager::SetStreamStopCallback(StreamStopCallback cb)
{
    stream_stop_cb_ = cb;
}

void SinkStreamManager::SetOnEncodedFrameCallback(EncodedFrameCallabck cb)
{
    encoded_frame_cb_ = cb;
}

void SinkStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    auto on_stream_state_changed_cb = std::bind(&SinkStreamManager::OnStreamStateChanged, this,
          std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_ice_candidate_added_cb = std::bind(&SinkStreamManager::OnIceCandidate, this,
          std::placeholders::_1, std::ref(stream));
    stream.GetEventHandler().SetOnIceCandidateCb(on_ice_candidate_added_cb);

    auto on_ice_gathering_state_changed_cb =
          std::bind(&SinkStreamManager::OnIceGatheringStateNotify, this, std::placeholders::_1,
                std::ref(stream));
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);

    auto on_encoded_frame_cb =
          std::bind(&SinkStreamManager::OnEncodedFrame, this, std::ref(stream));
    stream.GetEventHandler().SetOnEncodedFrameCb(on_encoded_frame_cb);
}

void SinkStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    if (state == WebRtcState::Stream::NEGOTIATING) {
        auto on_offer_created = std::bind(&SinkStreamManager::OnOfferCreated, this,
              std::placeholders::_1, std::ref(stream));
        stream.CreateOfferAsync(on_offer_created);
    }
}

void SinkStreamManager::OnOfferCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SinkStreamManager::OnIceCandidate(const std::string &candidate, WebRtcStream &stream)
{
    if (ice_candidate_added_cb_)
        ice_candidate_added_cb_(stream);
}

void SinkStreamManager::OnIceGatheringStateNotify(WebRtcState::IceGathering state,
      WebRtcStream &stream)
{
    DBG("Sink IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
    if (state == WebRtcState::IceGathering::COMPLETE) {
        if (stream_ready_cb_)
            stream_ready_cb_(stream);
    }
}

void SinkStreamManager::HandleRemovedClient(const std::string &discovery_id)
{
    auto src_stream_itr = src_stream_.find(discovery_id);
    if (src_stream_itr == src_stream_.end()) {
        DBG("There's no source stream %s", discovery_id.c_str());
        return;
    }

    // TODO: You should take care about stream resource
    src_stream_itr->second->Destroy();
    src_stream_.erase(src_stream_itr);

    return;
}

void SinkStreamManager::HandleMsg(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    if (flexbuffers::GetRoot(message).IsString())
        HandleStreamState(discovery_id, message);

    else if (flexbuffers::GetRoot(message).IsVector())
        HandleStreamInfo(discovery_id, message);
}

void SinkStreamManager::HandleStreamState(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    auto src_state = flexbuffers::GetRoot(message).ToString();

    if (src_state.compare("START") == 0)
        HandleStartStream(discovery_id);
    else if (src_state.compare("STOP") == 0)
        HandleRemovedClient(discovery_id);
    else
        DBG("Invalid message %s", src_state);
}

void SinkStreamManager::HandleStartStream(const std::string &discovery_id)
{
    auto src_stream = src_stream_.find(discovery_id);
    if (src_stream != src_stream_.end()) {
        DBG("There's stream already");
        return;
    }

    DBG("Src Stream Started");
    AddStream(discovery_id);
}

void SinkStreamManager::AddStream(const std::string &discovery_id)
{
    auto stream = new WebRtcStream();
    SetWebRtcStreamCallbacks(*stream);
    stream->Create(false, false);
    stream->Start();

    std::stringstream s_stream;
    s_stream << static_cast<void *>(stream);

    stream->SetStreamId(std::string(thread_id_ + s_stream.str()));
    src_stream_[discovery_id] = stream;
}

void SinkStreamManager::HandleStreamInfo(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    if (!WebRtcMessage::IsValidStreamInfo(message)) {
        DBG("Invalid streams info");
        return;
    }

    // sink_streams have a stream at normal situation
    auto src_streams = flexbuffers::GetRoot(message).AsVector();
    for (size_t stream_idx = 0; stream_idx < src_streams.size(); ++stream_idx) {
        auto stream = src_streams[stream_idx].AsMap();
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

void SinkStreamManager::UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
      const std::string &peer_id, const std::string &sdp,
      const std::vector<std::string> &ice_candidates)
{
    auto src_stream = src_stream_.find(discovery_id);
    if (src_stream == src_stream_.end()) {
        DBG("No matching stream");
        return;
    }

    if (peer_id.compare(src_stream->second->GetStreamId()) != 0) {
        DBG("Different ID");
        return;
    }

    if (src_stream->second->GetPeerId().size() == 0) {
        DBG("first update");
        src_stream->second->SetPeerId(id);
        src_stream->second->AddPeerInformation(sdp, ice_candidates);
    } else
        src_stream->second->UpdatePeerInformation(ice_candidates);

}

std::vector<uint8_t> SinkStreamManager::GetDiscoveryMessage(void)
{
    std::vector<uint8_t> message;

    flexbuffers::Builder fbb;
    fbb.Vector([&] {
        for (auto itr = src_stream_.begin(); itr != src_stream_.end(); ++itr) {
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

std::string SinkStreamManager::GetWatchingTopic(void)
{
    return watching_topic_;
}

}  // namespace AittWebRTCNamespace
