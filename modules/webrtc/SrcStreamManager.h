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
    std::vector<uint8_t> GetDiscoveryMessage(void) override;

  private:
    void SetWebRtcStreamCallbacks(WebRtcStream &stream) override;
    void OnStreamStateChanged(WebRtcState::Stream state);
    void OnAnswerCreated(std::string sdp, WebRtcStream &stream);
    void OnIceCandidate(void);
    void OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream);
    void HandleStreamState(const std::string &discovery_id,
          const std::vector<uint8_t> &message) override;
    void HandleStreamInfo(const std::string &discovery_id,
          const std::vector<uint8_t> &message) override;
    void AddStream(const std::string &discovery_id, const std::string &id, const std::string &sdp,
          const std::vector<std::string> &ice_candidates);
    void UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
          const std::string &peer_id, const std::string &sdp,
          const std::vector<std::string> &ice_candidates);
};
}  // namespace AittWebRTCNamespace
