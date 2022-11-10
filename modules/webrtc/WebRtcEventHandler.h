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

#include <functional>

#include "WebRtcState.h"

namespace AittWebRTCNamespace {

class WebRtcEventHandler {
  public:
    // TODO Add error and state callbacks
    void SetOnStateChangedCb(std::function<void(WebRtcState::Stream)> on_state_changed_cb)
    {
        on_state_changed_cb_ = on_state_changed_cb;
    };
    void CallOnStateChangedCb(WebRtcState::Stream state) const
    {
        if (on_state_changed_cb_)
            on_state_changed_cb_(state);
    };
    void UnsetOnStateChangedCb(void) { on_state_changed_cb_ = nullptr; };

    void SetOnSignalingStateNotifyCb(
          std::function<void(WebRtcState::Signaling)> on_signaling_state_notify_cb)
    {
        on_signaling_state_notify_cb_ = on_signaling_state_notify_cb;
    };
    void CallOnSignalingStateNotifyCb(WebRtcState::Signaling state) const
    {
        if (on_signaling_state_notify_cb_)
            on_signaling_state_notify_cb_(state);
    };
    void UnsetOnSignalingStateNotifyCb(void) { on_signaling_state_notify_cb_ = nullptr; };

    void SetOnIceCandidateCb(std::function<void(std::string)> on_ice_candidate_cb)
    {
        on_ice_candidate_cb_ = on_ice_candidate_cb;
    };
    void CallOnIceCandidateCb(std::string candidate) const
    {
        if (on_ice_candidate_cb_)
            on_ice_candidate_cb_(candidate);
    };
    void UnsetOnIceCandidateCb(void) { on_ice_candidate_cb_ = nullptr; };

    void SetOnIceGatheringStateNotifyCb(
          std::function<void(WebRtcState::IceGathering)> on_ice_gathering_state_notify_cb)
    {
        on_ice_gathering_state_notify_cb_ = on_ice_gathering_state_notify_cb;
    };
    void CallOnIceGatheringStateNotifyCb(WebRtcState::IceGathering state) const
    {
        if (on_ice_gathering_state_notify_cb_)
            on_ice_gathering_state_notify_cb_(state);
    };
    void UnsetOnIceGatheringStateNotifyeCb(void) { on_ice_gathering_state_notify_cb_ = nullptr; };

    void SetOnIceConnectionStateNotifyCb(
          std::function<void(WebRtcState::IceConnection)> on_ice_connection_state_notify_cb)
    {
        on_ice_connection_state_notify_cb_ = on_ice_connection_state_notify_cb;
    };
    void CallOnIceConnectionStateNotifyCb(WebRtcState::IceConnection state) const
    {
        if (on_ice_connection_state_notify_cb_)
            on_ice_connection_state_notify_cb_(state);
    };
    void UnsetOnIceConnectionStateNotifyeCb(void) { on_ice_connection_state_notify_cb_ = nullptr; };

    void SetOnEncodedFrameCb(std::function<void(void)> on_encoded_frame_cb)
    {
        on_encoded_frame_cb_ = on_encoded_frame_cb;
    };
    void CallOnEncodedFrameCb(void) const
    {
        if (on_encoded_frame_cb_)
            on_encoded_frame_cb_();
    };
    void UnsetEncodedFrameCb(void) { on_encoded_frame_cb_ = nullptr; };

    void SetOnTrakAddedCb(std::function<void(unsigned int id)> on_track_added_cb)
    {
        on_track_added_cb_ = on_track_added_cb;
    };
    void CallOnTrakAddedCb(unsigned int id) const
    {
        if (on_track_added_cb_)
            on_track_added_cb_(id);
    };
    void UnsetTrackAddedCb(void) { on_track_added_cb_ = nullptr; };

  private:
    std::function<void(void)> on_negotiation_needed_cb_;
    std::function<void(WebRtcState::Stream)> on_state_changed_cb_;
    std::function<void(WebRtcState::Signaling)> on_signaling_state_notify_cb_;
    std::function<void(std::string)> on_ice_candidate_cb_;
    std::function<void(WebRtcState::IceGathering)> on_ice_gathering_state_notify_cb_;
    std::function<void(WebRtcState::IceConnection)> on_ice_connection_state_notify_cb_;
    std::function<void(void)> on_encoded_frame_cb_;
    std::function<void(unsigned int id)> on_track_added_cb_;
};

}  // namespace AittWebRTCNamespace
