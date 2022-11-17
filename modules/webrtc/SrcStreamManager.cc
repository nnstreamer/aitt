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
      : StreamManager(topic + "/SRC", topic + "/SINK", aitt_id, thread_id)
{
}

SrcStreamManager::~SrcStreamManager()
{
}

void SrcStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    stream.GetEventHandler().SetOnStateChangedCb(
          std::bind(&SrcStreamManager::OnStreamStateChanged, this, std::placeholders::_1));

    stream.GetEventHandler().SetOnSignalingStateNotifyCb(
          std::bind(&SrcStreamManager::OnSignalingStateNotify, this, std::placeholders::_1,
                std::ref(stream)));

    stream.GetEventHandler().SetOnIceCandidateCb(
          std::bind(&SrcStreamManager::OnIceCandidate, this));
}

void SrcStreamManager::OnStreamStateChanged(WebRtcState::Stream state)
{
    DBG("OnSrcStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
}

void SrcStreamManager::OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream)
{
    DBG("OnSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
    if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
        stream.CreateAnswerAsync(std::bind(&SrcStreamManager::OnAnswerCreated, this,
              std::placeholders::_1, std::ref(stream)));
    }
}

void SrcStreamManager::OnAnswerCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);

    stream.SetLocalDescription(sdp);
}

void SrcStreamManager::OnIceCandidate(void)
{
    if (ice_candidate_added_cb_)
        ice_candidate_added_cb_();
}

void SrcStreamManager::HandleStreamState(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    DBG("%s", __func__);
    auto sink_state = flexbuffers::GetRoot(message).ToString();

    if (sink_state.compare("STOP") == 0)
        HandleRemovedClient(discovery_id);
    else
        ERR("Invalid message %s", sink_state);
}

void SrcStreamManager::HandleStreamInfo(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    DBG("%s", __func__);
    if (!WebRtcMessage::IsValidStreamInfo(message)) {
        ERR("Invalid streams info");
        return;
    }

    // have a stream at Current status
    auto stream = flexbuffers::GetRoot(message).AsMap();
    auto id = stream["id"].AsString().str();
    auto peer_id = stream["peer_id"].AsString().str();
    auto sdp = stream["sdp"].AsString().str();
    std::vector<std::string> ice_candidates;
    auto ice_info = stream["ice_candidates"].AsVector();
    for (size_t ice_idx = 0; ice_idx < ice_info.size(); ++ice_idx)
        ice_candidates.push_back(ice_info[ice_idx].AsString().str());
    UpdateStreamInfo(discovery_id, id, peer_id, sdp, ice_candidates);
}

void SrcStreamManager::UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
      const std::string &peer_id, const std::string &sdp,
      const std::vector<std::string> &ice_candidates)
{
    // There's only one stream for a aitt ID
    if (peer_aitt_id_.empty())
        AddStream(discovery_id, id, sdp, ice_candidates);
    else if (peer_aitt_id_ == discovery_id && peer_id.compare(stream_.GetStreamId()) != 0)
        stream_.UpdatePeerInformation(ice_candidates);
    else
        ERR("Invalid peer ID");
}

void SrcStreamManager::AddStream(const std::string &discovery_id, const std::string &id,
      const std::string &sdp, const std::vector<std::string> &ice_candidates)
{
    SetWebRtcStreamCallbacks(stream_);
    stream_.Create(true, false);
    stream_.AttachCameraSource();
    stream_.Start();

    std::stringstream s_stream;
    s_stream << static_cast<void *>(&stream_);

    stream_.SetStreamId(std::string(thread_id_ + s_stream.str()));
    stream_.SetPeerId(id);
    peer_aitt_id_ = discovery_id;
    stream_.AddPeerInformation(sdp, ice_candidates);

    return;
}

std::vector<uint8_t> SrcStreamManager::GetDiscoveryMessage(void)
{
    std::vector<uint8_t> message;

    flexbuffers::Builder fbb;
    fbb.Map([&] {
        fbb.String("id", stream_.GetStreamId());
        fbb.String("peer_id", stream_.GetPeerId());
        fbb.String("sdp", stream_.GetLocalDescription());
        fbb.Vector("ice_candidates", [&]() {
            for (const auto &candidate : stream_.GetIceCandidates()) {
                fbb.String(candidate);
            }
        });
    });
    fbb.Finish();

    message = fbb.GetBuffer();
    return message;
}

}  // namespace AittWebRTCNamespace
