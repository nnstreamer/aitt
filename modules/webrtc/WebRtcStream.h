/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <atomic>
#include <functional>
#include <string>
#include <vector>

// TODO: webrtc.h is very heavy header file.
// I think we need to decide whether to include this or not
#include <webrtc.h>

#include "WebRtcEventHandler.h"
#include "WebRtcMessage.h"

namespace AittWebRTCNamespace {

class WebRtcStream {
  public:
    WebRtcStream();
    ~WebRtcStream();
    void Destroy(void);
    bool Start(void);
    bool Stop(void);
    int Push(void *obj);
    void SetSourceType(webrtc_media_source_type_e source_type);
    bool ActivateSource(void);
    bool DeactivateSource(void);
    bool SetVideoResolution(int width, int height);
    bool SetVideoFrameRate(int frame_rate);
    bool SetMediaFormat(int width, int height, int frame_rate, const std::string &format);
    void SetDecodeCodec(const std::string &codec);
    void SetDisplay(void *display_object);
    bool IsDisplaySink(void);
    void AddDataChannel(void);
    void AttachSignals(bool is_source);
    void DetachSignals(void);
    // Cautions : Event handler is not a pointer. So, change event_handle after Set Event handler
    // doesn't affect event handler which is included in WebRtcStream
    WebRtcEventHandler &GetEventHandler(void);

    bool CreateOfferAsync(std::function<void(std::string)> on_created_cb);
    bool CreateAnswerAsync(std::function<void(std::string)> on_created_cb);

    bool SetLocalDescription(const std::string &description);
    bool SetRemoteDescription(const std::string &description);
    void SetStreamId(const std::string &id);
    std::string GetStreamId(void) const;
    void SetPeerId(const std::string &id);
    std::string &GetPeerId(void);

    bool AddIceCandidateFromMessage(const std::string &ice_message);
    bool AddPeerInformation(const std::string &sdp, const std::vector<std::string> &ice_candidates);
    void UpdatePeerInformation(const std::vector<std::string> &ice_candidates);
    bool SetPeerInformation(void);
    bool SetPeerSDP(void);
    bool SetPeerIceCandidates(void);
    const std::vector<std::string> &GetIceCandidates() const { return ice_candidates_; };

    std::string GetLocalDescription(void) const { return local_description_; };

    void PrintStats(void);
    bool IsPlayingState(void);

  private:
    static void OnOfferCreated(webrtc_h webrtc, const char *description, void *user_data);
    static void OnAnswerCreated(webrtc_h webrtc, const char *description, void *user_data);
    static void OnError(webrtc_h webrtc, webrtc_error_e error, webrtc_state_e state,
          void *user_data);
    static void OnStateChanged(webrtc_h webrtc, webrtc_state_e previous, webrtc_state_e current,
          void *user_data);
    static void OnSignalingStateChanged(webrtc_h webrtc, webrtc_signaling_state_e state,
          void *user_data);
    static void OnIceGatheringStateChanged(webrtc_h webrtc, webrtc_ice_gathering_state_e state,
          void *user_data);
    static void OnIceConnectionStateChanged(webrtc_h webrtc, webrtc_ice_connection_state_e state,
          void *user_data);
    static void OnIceCandiate(webrtc_h webrtc, const char *candidate, void *user_data);
    static void OnEncodedFrame(webrtc_h webrtc, webrtc_media_type_e type, unsigned int track_id,
          media_packet_h packet, void *user_data);
    static void OnTrackAdded(webrtc_h webrtc, webrtc_media_type_e type, unsigned int id,
          void *user_data);
    static void OnDataChannelOpen(webrtc_data_channel_h channel, void *user_data);
    static void OnBufferStateChanged(unsigned int id,
          webrtc_media_packet_source_buffer_state_e state, void *user_data);
    bool IsNegotiatingState(void);
    bool IsRedundantCandidate(const std::string &candidate);
    static void *GetMediaFormatHandler(int width, int height, int frame_rate,
          const std::string &format);

  private:
    webrtc_h webrtc_handle_;
    webrtc_media_source_type_e source_type_;
    webrtc_data_channel_h channel_;
    unsigned int source_id_;
    void *display_object_;
    std::string local_description_;
    std::string remote_description_;
    std::string id_;
    std::string peer_id_;
    std::vector<std::string> ice_candidates_;
    std::vector<std::string> peer_ice_candidates_;
    std::vector<std::string> stored_peer_ice_candidates_;
    std::function<void(std::string)> on_offer_created_cb_;
    std::function<void(std::string)> on_answer_created_cb_;
    WebRtcEventHandler event_handler_;
};
}  // namespace AittWebRTCNamespace
