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

#include "StreamManager.h"

namespace AittWebRTCNamespace {

class SrcStreamManager : public StreamManager {
  public:
    explicit SrcStreamManager(const std::string &topic, const std::string &aitt_id,
          const std::string &thread_id);
    ~SrcStreamManager();
    void Start(void) override;
    void SetStreamReadyCallback(StreamReadyCallback cb) override;
    //TODO: What's the best way to shutdown all?
    void Stop(void) override;
    void HandleRemovedClient(const std::string &id) override;
    void HandleDiscoveredStream(const std::string &id, const std::vector<uint8_t> &message) override;

  private:
    void SetWebRtcStreamCallbacks(WebRtcStream &stream) override;
    void AddStream(const std::string &id, const std::vector<uint8_t> &message);
    static void OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream);
    static void OnAnswerCreated(std::string sdp, WebRtcStream &stream);
    static void OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream);
    static void OnIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          SrcStreamManager *manager);
    // TODO: What if user copies the module?
    // Think about that case with destructor
    WebRtcStream *stream_;
    StreamReadyCallback stream_ready_cb_;
};
}  // namespace AittWebRTCNamespace
