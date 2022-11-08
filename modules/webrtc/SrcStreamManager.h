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

#pragma once

#include <map>

#include "StreamManager.h"

namespace AittWebRTCNamespace {

class SrcStreamManager : public StreamManager {
  public:
    explicit SrcStreamManager(const std::string &topic, const std::string &aitt_id,
          const std::string &thread_id);
    virtual ~SrcStreamManager();
    void Start(void) override;
    // TODO: What's the best way to shutdown all?
    void Stop(void) override;
    void SetIceCandidateAddedCallback(IceCandidateAddedCallback cb) override;
    void SetStreamReadyCallback(StreamReadyCallback cb) override;
    void SetStreamStartCallback(StreamStartCallback cb) override;
    void SetStreamStopCallback(StreamStopCallback cb) override;
    std::vector<uint8_t> GetDiscoveryMessage(void) override;
    std::string GetWatchingTopic(void) override;
    void HandleRemovedClient(const std::string &discovery_id) override;
    void HandleMsg(const std::string &discovery_id, const std::vector<uint8_t> &message) override;

  private:
    void SetWebRtcStreamCallbacks(WebRtcStream &stream) override;
    void OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream);
    void OnAnswerCreated(std::string sdp, WebRtcStream &stream);
    void OnIceCandidate(const std::string &candidate, WebRtcStream &stream);
    void OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream);
    void OnIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream);
    void HandleStreamState(const std::string &discovery_id, const std::vector<uint8_t> &message);
    void HandleStreamInfo(const std::string &discovery_id, const std::vector<uint8_t> &message);
    void AddStream(const std::string &discovery_id, const std::string &id, const std::string &sdp,
          const std::vector<std::string> &ice_candidates);
    void UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
          const std::string &peer_id, const std::string &sdp,
          const std::vector<std::string> &ice_candidates);

    std::string watching_topic_;
    // TODO: What if user copies the module?
    // Think about that case with destructor
    std::map<std::string /* Peer Aitt Discovery ID */, WebRtcStream *> sink_streams_;
    IceCandidateAddedCallback ice_candidate_added_cb_;
    StreamReadyCallback stream_ready_cb_;
    StreamStartCallback stream_start_cb_;
    StreamStopCallback stream_stop_cb_;
};
}  // namespace AittWebRTCNamespace
