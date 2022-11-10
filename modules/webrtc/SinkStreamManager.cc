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
      : StreamManager(topic + "/SINK", topic + "/SRC", aitt_id, thread_id)
{
}

SinkStreamManager::~SinkStreamManager()
{
    for (auto itr = streams_.begin(); itr != streams_.end(); ++itr)
        itr->second->Destroy();
    streams_.clear();
}

void SinkStreamManager::SetOnEncodedFrameCallback(EncodedFrameCallabck cb)
{
    encoded_frame_cb_ = cb;
}

void SinkStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    stream.GetEventHandler().SetOnStateChangedCb(std::bind(&SinkStreamManager::OnStreamStateChanged,
          this, std::placeholders::_1, std::ref(stream)));

    stream.GetEventHandler().SetOnIceCandidateCb(
          std::bind(&SinkStreamManager::OnIceCandidate, this));

    stream.GetEventHandler().SetOnEncodedFrameCb(std::bind(&SinkStreamManager::OnEncodedFrame, this));
}

void SinkStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    if (state == WebRtcState::Stream::NEGOTIATING) {
        stream.CreateOfferAsync(std::bind(&SinkStreamManager::OnOfferCreated, this,
              std::placeholders::_1, std::ref(stream)));
    }
}

void SinkStreamManager::OnOfferCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SinkStreamManager::OnIceCandidate(void)
{
    if (ice_candidate_added_cb_)
        ice_candidate_added_cb_();
}

void SinkStreamManager::OnEncodedFrame(void)
{
    if (encoded_frame_cb_)
        encoded_frame_cb_();
}

void SinkStreamManager::HandleStreamState(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    DBG("%s", __func__);
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
    DBG("%s", __func__);
    auto src_stream = streams_.find(discovery_id);
    if (src_stream != streams_.end()) {
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
    streams_[discovery_id] = stream;
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
    auto src_stream = streams_.find(discovery_id);
    if (src_stream == streams_.end()) {
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
        for (auto itr = streams_.begin(); itr != streams_.end(); ++itr) {
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

}  // namespace AittWebRTCNamespace
