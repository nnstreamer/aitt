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

#include "WebRtcStream.h"

#include "WebRtcMessage.h"
#include "aitt_internal.h"

WebRtcStream::WebRtcStream() : webrtc_handle_(nullptr), channel_(nullptr), source_id_(0)
{
}

WebRtcStream::~WebRtcStream()
{
    Destroy();
    DBG("%s", __func__);
}

bool WebRtcStream::Create(bool is_source, bool need_display)
{
    if (webrtc_handle_) {
        ERR("Already created %p", webrtc_handle_);
        return false;
    }

    auto ret = webrtc_create(&webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE) {
        ERR("Failed to create webrtc handle");
        return false;
    }

    if (!is_source) {
        auto add_source_ret =
              webrtc_add_media_source(webrtc_handle_, WEBRTC_MEDIA_SOURCE_TYPE_NULL, &source_id_);
        if (add_source_ret != WEBRTC_ERROR_NONE)
            ERR("Failed to add media source");
        auto set_transceiver_codec_ret = webrtc_media_source_set_transceiver_codec(webrtc_handle_,
              source_id_, WEBRTC_MEDIA_TYPE_VIDEO, WEBRTC_TRANSCEIVER_CODEC_VP8);
        if (set_transceiver_codec_ret != WEBRTC_ERROR_NONE)
            ERR("Failed to set transceiver codec");
    }

    webrtc_create_data_channel(webrtc_handle_, "label", nullptr, &channel_);
    AttachSignals(is_source, need_display);

    return true;
}

void WebRtcStream::Destroy(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return;
    }
    auto stop_ret = webrtc_stop(webrtc_handle_);
    if (stop_ret != WEBRTC_ERROR_NONE)
        ERR("Failed to stop webrtc handle");

    DetachSignals();
    auto destroy_channel_ret = webrtc_destroy_data_channel(channel_);
    if (destroy_channel_ret != WEBRTC_ERROR_NONE)
        ERR("Failed to destroy webrtc channel");
    auto ret = webrtc_destroy(webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to destroy webrtc handle");
    webrtc_handle_ = nullptr;
}

bool WebRtcStream::Start(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }

    auto ret = webrtc_start(webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to start webrtc handle");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::Stop(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }

    auto ret = webrtc_stop(webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to stop webrtc handle");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::AttachCameraSource(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }

    if (source_id_) {
        ERR("source already attached");
        return false;
    }

    auto ret =
          webrtc_add_media_source(webrtc_handle_, WEBRTC_MEDIA_SOURCE_TYPE_CAMERA, &source_id_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to add media source");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::DetachCameraSource(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }

    if (!source_id_) {
        ERR("Camera source is not attached");
        return false;
    }

    auto ret = webrtc_remove_media_source(webrtc_handle_, source_id_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to remove media source");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::CreateOfferAsync(std::function<void(std::string)> on_created_cb)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }
    on_offer_created_cb_ = on_created_cb;
    auto ret = webrtc_create_offer_async(webrtc_handle_, NULL, OnOfferCreated, this);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to create offer async");

    return ret == WEBRTC_ERROR_NONE;
}

void WebRtcStream::OnOfferCreated(webrtc_h webrtc, const char *description, void *user_data)
{
    RET_IF(!user_data);

    WebRtcStream *webrtc_stream = static_cast<WebRtcStream *>(user_data);

    if (webrtc_stream->on_offer_created_cb_)
        webrtc_stream->on_offer_created_cb_(std::string(description));
}

bool WebRtcStream::CreateAnswerAsync(std::function<void(std::string)> on_created_cb)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }
    on_answer_created_cb_ = on_created_cb;
    auto ret = webrtc_create_answer_async(webrtc_handle_, NULL, OnAnswerCreated, this);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to create answer async");

    return ret == WEBRTC_ERROR_NONE;
}

void WebRtcStream::OnAnswerCreated(webrtc_h webrtc, const char *description, void *user_data)
{
    if (!user_data)
        return;

    WebRtcStream *webrtc_stream = static_cast<WebRtcStream *>(user_data);
    if (webrtc_stream->on_answer_created_cb_)
        webrtc_stream->on_answer_created_cb_(std::string(description));
}

bool WebRtcStream::SetLocalDescription(const std::string &description)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }
    auto ret = webrtc_set_local_description(webrtc_handle_, description.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set local description");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::SetRemoteDescription(const std::string &description)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }

    webrtc_state_e state;
    auto get_state_ret = webrtc_get_state(webrtc_handle_, &state);
    if (get_state_ret != WEBRTC_ERROR_NONE) {
        ERR("Failed to get state");
        return false;
    }

    if (state != WEBRTC_STATE_NEGOTIATING) {
        remote_description_ = description;
        ERR("Invalid state, will be registred at NEGOTIATING state");
        return true;
    }

    auto ret = webrtc_set_remote_description(webrtc_handle_, description.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set remote description");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::AddIceCandidateFromMessage(const std::string &ice_message)
{
    ERR("%s", __func__);
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return false;
    }
    auto ret = webrtc_add_ice_candidate(webrtc_handle_, ice_message.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set add ice candidate");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::AddDiscoveryInformation(const std::vector<uint8_t> &discovery_message)
{
    ERR("%s", __func__);

    auto info = WebRtcMessage::ParseDiscoveryMessage(discovery_message);
    if (!SetRemoteDescription(info.sdp))
        return false;

    for (const auto &ice_candidate : info.ice_candidates)
        AddIceCandidateFromMessage(ice_candidate);

    return true;
}

void WebRtcStream::AttachSignals(bool is_source, bool need_display)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return;
    }

    int ret = WEBRTC_ERROR_NONE;
    // TODO: ADHOC TV profile doesn't show DBG level log
    ret = webrtc_set_error_cb(webrtc_handle_, OnError, this);
    DBG("webrtc_set_error_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    ret = webrtc_set_state_changed_cb(webrtc_handle_, OnStateChanged, this);
    DBG("webrtc_set_state_changed_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    ret = webrtc_set_signaling_state_change_cb(webrtc_handle_, OnSignalingStateChanged, this);
    DBG("webrtc_set_signaling_state_change_cb %s",
          ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    ret = webrtc_set_ice_gathering_state_change_cb(webrtc_handle_, OnIceGatheringStateChanged,
          this);
    DBG("webrtc_set_ice_gathering_state_change_cb %s",
          ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    ret = webrtc_set_ice_connection_state_change_cb(webrtc_handle_, OnIceConnectionStateChanged,
          this);
    DBG("webrtc_set_ice_connection_state_change_cb %s",
          ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    ret = webrtc_set_ice_candidate_cb(webrtc_handle_, OnIceCandiate, this);
    DBG("webrtc_set_ice_candidate_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");

    if (!is_source && !need_display) {
        ret = webrtc_set_encoded_video_frame_cb(webrtc_handle_, OnEncodedFrame, this);
        ERR("webrtc_set_encoded_video_frame_cb %s",
              ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }

    if (!is_source && need_display) {
        ret = webrtc_set_track_added_cb(webrtc_handle_, OnTrackAdded, this);
        ERR("webrtc_set_track_added_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }
    ret = webrtc_data_channel_set_open_cb(channel_, OnDataChannelOpen, this);
    DBG("webrtc_data_channel_set_open_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");

    return;
}

void WebRtcStream::DetachSignals(void)
{
    if (!webrtc_handle_) {
        ERR("WebRTC handle is not created");
        return;
    }

    webrtc_unset_error_cb(webrtc_handle_);
    webrtc_unset_state_changed_cb(webrtc_handle_);
    webrtc_unset_signaling_state_change_cb(webrtc_handle_);
    webrtc_unset_ice_gathering_state_change_cb(webrtc_handle_);
    webrtc_unset_ice_connection_state_change_cb(webrtc_handle_);
    webrtc_unset_ice_candidate_cb(webrtc_handle_);
    webrtc_unset_encoded_video_frame_cb(webrtc_handle_);
    webrtc_unset_track_added_cb(webrtc_handle_);
    webrtc_data_channel_unset_open_cb(channel_);
}

void WebRtcStream::OnError(webrtc_h webrtc, webrtc_error_e error, webrtc_state_e state,
      void *user_data)
{
    // TODO
    ERR("%s", __func__);
}

void WebRtcStream::OnStateChanged(webrtc_h webrtc, webrtc_state_e previous, webrtc_state_e current,
      void *user_data)
{
    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    if (current == WEBRTC_STATE_NEGOTIATING && webrtc_stream->remote_description_.size() != 0) {
        ERR("received remote description exists");
        auto ret = webrtc_set_remote_description(webrtc_stream->webrtc_handle_,
              webrtc_stream->remote_description_.c_str());
        if (ret != WEBRTC_ERROR_NONE)
            ERR("Failed to set remote description");
        webrtc_stream->remote_description_ = std::string();
    }
    webrtc_stream->GetEventHandler().CallOnStateChangedCb(WebRtcState::ToStreamState(current));
}

void WebRtcStream::OnSignalingStateChanged(webrtc_h webrtc, webrtc_signaling_state_e state,
      void *user_data)
{
    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);
    webrtc_stream->GetEventHandler().CallOnSignalingStateNotifyCb(
          WebRtcState::ToSignalingState(state));
}

void WebRtcStream::OnIceGatheringStateChanged(webrtc_h webrtc, webrtc_ice_gathering_state_e state,
      void *user_data)
{
    ERR("%s %d", __func__, state);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    webrtc_stream->GetEventHandler().CallOnIceGatheringStateNotifyCb(
          WebRtcState::ToIceGatheringState(state));
}

void WebRtcStream::OnIceConnectionStateChanged(webrtc_h webrtc, webrtc_ice_connection_state_e state,
      void *user_data)
{
    ERR("%s %d", __func__, state);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    webrtc_stream->GetEventHandler().CallOnIceConnectionStateNotifyCb(
          WebRtcState::ToIceConnectionState(state));
}

void WebRtcStream::OnIceCandiate(webrtc_h webrtc, const char *candidate, void *user_data)
{
    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    webrtc_stream->ice_candidates_.push_back(candidate);
}

void WebRtcStream::OnEncodedFrame(webrtc_h webrtc, webrtc_media_type_e type, unsigned int track_id,
      media_packet_h packet, void *user_data)
{
    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    if (type == WEBRTC_MEDIA_TYPE_VIDEO)
        webrtc_stream->GetEventHandler().CallOnEncodedFrameCb();
}

void WebRtcStream::OnTrackAdded(webrtc_h webrtc, webrtc_media_type_e type, unsigned int id,
      void *user_data)
{
    // type AUDIO(0), VIDEO(1)
    INFO("Added Track : id(%d), type(%s)", id, type ? "Video" : "Audio");

    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    if (type == WEBRTC_MEDIA_TYPE_VIDEO)
        webrtc_stream->GetEventHandler().CallOnTrakAddedCb(id);
}

void WebRtcStream::OnDataChannelOpen(webrtc_data_channel_h channel, void *user_data)
{
    ERR("%s", __func__);
}
