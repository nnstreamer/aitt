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
#include "WebRtcStream.h"

#include <inttypes.h>
#include <media_format.h>
#include <webrtc_internal.h>

#include <stdexcept>

#include "WebRtcMessage.h"
#include "aitt_internal.h"

namespace AittWebRTCNamespace {

WebRtcStream::WebRtcStream()
      : webrtc_handle_(nullptr),
        source_type_(WEBRTC_MEDIA_SOURCE_TYPE_NULL),
        channel_(nullptr),
        source_id_(0),
        display_object_(nullptr)
{
    // Notice for Tizen webrtc handle
    // This API includes file read operation, launching thread,
    // gstreamer library initialization, and so on.
    auto ret = webrtc_create(&webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        throw std::runtime_error("WebRtc Handler Creation Failed");
}

WebRtcStream::~WebRtcStream()
{
    Destroy();
    DBG("%s", __func__);
}

void WebRtcStream::Destroy(void)
{
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

    // TODO what should be initialized?
}

bool WebRtcStream::Start(void)
{
    auto ret = webrtc_start(webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to start webrtc handle");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::Stop(void)
{
    auto ret = webrtc_stop(webrtc_handle_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to stop webrtc handle");

    // TODO what should be initialized?
    return ret == WEBRTC_ERROR_NONE;
}

int WebRtcStream::Push(void *obj)
{
    auto ret = webrtc_media_packet_source_push_packet(webrtc_handle_, source_id_,
          static_cast<media_packet_h>(obj));
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to push packet");

    // TODO what should be initialized?
    return ret;
}

void WebRtcStream::SetSourceType(webrtc_media_source_type_e source_type)
{
    source_type_ = source_type;
}

bool WebRtcStream::ActivateSource(void)
{
    if (source_id_) {
        ERR("source already attached");
        return false;
    }

    auto ret = webrtc_add_media_source(webrtc_handle_, source_type_, &source_id_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to add media source");

    if (source_type_ == WEBRTC_MEDIA_SOURCE_TYPE_MEDIA_PACKET) {
        int set_buffer_cb_ret = webrtc_media_packet_source_set_buffer_state_changed_cb(
              webrtc_handle_, source_id_, OnBufferStateChanged, this);
        DBG("webrtc_media_packet_source_set_buffer_state_changed_cb %s",
              set_buffer_cb_ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }
    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::DeactivateSource(void)
{
    if (!source_id_) {
        ERR("Media source is not attached");
        return false;
    }

    auto ret = webrtc_remove_media_source(webrtc_handle_, source_id_);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to remove media source");
    else
        source_id_ = 0;

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::SetVideoResolution(int width, int height)
{
    if (!source_id_) {
        ERR("Media source is not attached");
        return false;
    }

    auto ret = webrtc_media_source_set_video_resolution(webrtc_handle_, source_id_, width, height);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set video resolution");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::SetVideoFrameRate(int frame_rate)
{
    if (!source_id_) {
        ERR("Media source is not attached");
        return false;
    }

    auto ret = webrtc_media_source_set_video_framerate(webrtc_handle_, source_id_, frame_rate);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set video frame rate");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::SetMediaFormat(int width, int height, int frame_rate, const std::string &format)
{
    media_format_h media_format =
          static_cast<media_format_h>(GetMediaFormatHandler(width, height, frame_rate, format));
    if (!media_format)
        return false;

    auto ret = webrtc_media_packet_source_set_format(webrtc_handle_, source_id_, media_format);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set media format");
    media_format_unref(media_format);

    if (source_type_ == WEBRTC_MEDIA_SOURCE_TYPE_MEDIA_PACKET && format == "H264") {
        int set_payload_ret = webrtc_media_source_set_payload_type(webrtc_handle_, source_id_,
              WEBRTC_MEDIA_TYPE_VIDEO, 127);
        DBG("webrtc_media_source_set_payload_type %s",
              set_payload_ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }

    return ret == WEBRTC_ERROR_NONE;
}

void WebRtcStream::SetDecodeCodec(const std::string &codec)
{
    webrtc_transceiver_codec_e transceiver_codec = WEBRTC_TRANSCEIVER_CODEC_VP8;
    if (codec == "VP9")
        transceiver_codec = WEBRTC_TRANSCEIVER_CODEC_VP9;
    else if (codec == "H264")
        transceiver_codec = WEBRTC_TRANSCEIVER_CODEC_H264;
    auto ret = webrtc_media_source_set_transceiver_codec(webrtc_handle_, source_id_,
          WEBRTC_MEDIA_TYPE_VIDEO, transceiver_codec);
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set transceiver codec");

    if (transceiver_codec == WEBRTC_TRANSCEIVER_CODEC_H264) {
        int set_payload_ret = webrtc_media_source_set_payload_type(webrtc_handle_, source_id_,
              WEBRTC_MEDIA_TYPE_VIDEO, 127);
        DBG("webrtc_media_source_set_payload_type %s",
              set_payload_ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }
    return;
}


void WebRtcStream::SetDisplay(void *display_object)
{
    display_object_ = display_object;
    if (display_object_)
        webrtc_unset_encoded_video_frame_cb(webrtc_handle_);
    else
        webrtc_set_encoded_video_frame_cb(webrtc_handle_, OnEncodedFrame, this);
}

bool WebRtcStream::IsDisplaySink(void)
{
    return display_object_ != nullptr;
}

bool WebRtcStream::CreateOfferAsync(std::function<void(std::string)> on_created_cb)
{
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
    auto ret = webrtc_set_local_description(webrtc_handle_, description.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set local description");
    else
        local_description_ = description;

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::SetRemoteDescription(const std::string &description)
{
    auto ret = webrtc_set_remote_description(webrtc_handle_, description.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set remote description");

    return ret == WEBRTC_ERROR_NONE;
}

void WebRtcStream::SetStreamId(const std::string &id)
{
    id_ = id;
}

std::string WebRtcStream::GetStreamId(void) const
{
    return id_;
}

void WebRtcStream::SetPeerId(const std::string &id)
{
    peer_id_ = id;
}

std::string &WebRtcStream::GetPeerId(void)
{
    return peer_id_;
}

bool WebRtcStream::AddIceCandidateFromMessage(const std::string &ice_message)
{
    auto ret = webrtc_add_ice_candidate(webrtc_handle_, ice_message.c_str());
    if (ret != WEBRTC_ERROR_NONE)
        ERR("Failed to set add ice candidate");

    return ret == WEBRTC_ERROR_NONE;
}

bool WebRtcStream::AddPeerInformation(const std::string &sdp,
      const std::vector<std::string> &ice_candidates)
{
    remote_description_ = sdp;
    peer_ice_candidates_ = ice_candidates;
    if (!IsNegotiatingState()) {
        DBG("Not negotiable");
        return false;
    }
    return SetPeerInformation();
}

void WebRtcStream::UpdatePeerInformation(const std::vector<std::string> &ice_candidates)
{
    if (IsPlayingState()) {
        remote_description_.clear();
        peer_ice_candidates_.clear();
        stored_peer_ice_candidates_.clear();
        DBG("Now Playing");
        return;
    }

    peer_ice_candidates_ = ice_candidates;
    if (!IsNegotiatingState()) {
        DBG("Not negotiable");
        return;
    }

    SetPeerInformation();
    return;
}

bool WebRtcStream::IsNegotiatingState(void)
{
    webrtc_state_e state;
    auto get_state_ret = webrtc_get_state(webrtc_handle_, &state);
    if (get_state_ret != WEBRTC_ERROR_NONE) {
        ERR("Failed to get state");
        return false;
    }

    return state == WEBRTC_STATE_NEGOTIATING;
}

bool WebRtcStream::IsPlayingState(void)
{
    webrtc_state_e state;
    auto get_state_ret = webrtc_get_state(webrtc_handle_, &state);
    if (get_state_ret != WEBRTC_ERROR_NONE) {
        ERR("Failed to get state");
        return false;
    }

    return state == WEBRTC_STATE_PLAYING;
}

bool WebRtcStream::SetPeerInformation(void)
{
    bool res = true;

    res = SetPeerSDP() && SetPeerIceCandidates();
    DBG("Peer information is now %s", res ? "Valid" : "Invalid");

    return res;
}
bool WebRtcStream::SetPeerSDP(void)
{
    ERR("%s", __func__);
    bool res = true;
    if (remote_description_.size() == 0) {
        DBG("Peer SDP empty");
        return res;
    }

    if (SetRemoteDescription(remote_description_))
        remote_description_.clear();
    else
        res = false;

    return res;
}

bool WebRtcStream::SetPeerIceCandidates(void)
{
    ERR("%s", __func__);
    bool res{true};
    for (auto itr = peer_ice_candidates_.begin(); itr != peer_ice_candidates_.end(); ++itr) {
        if (IsRedundantCandidate(*itr))
            continue;

        if (AddIceCandidateFromMessage(*itr))
            stored_peer_ice_candidates_.push_back(*itr);
        else
            res = false;
    }

    return res;
}

bool WebRtcStream::IsRedundantCandidate(const std::string &candidate)
{
    for (const auto &stored_ice_candidate : stored_peer_ice_candidates_) {
        if (stored_ice_candidate.compare(candidate) == 0)
            return true;
    }
    return false;
}

static bool __stats_cb(webrtc_stats_type_e type, const webrtc_stats_prop_info_s *prop_info,
      void *user_data)
{
    switch (prop_info->type) {
    case WEBRTC_STATS_PROP_TYPE_BOOL:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%d]", type, prop_info->name, prop_info->prop,
              prop_info->v_bool);
        break;
    case WEBRTC_STATS_PROP_TYPE_INT:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%d]", type, prop_info->name, prop_info->prop,
              prop_info->v_int);
        break;
    case WEBRTC_STATS_PROP_TYPE_INT64:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%" PRId64 "]", type, prop_info->name,
              prop_info->prop, prop_info->v_int64);
        break;
    case WEBRTC_STATS_PROP_TYPE_UINT:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%u]", type, prop_info->name, prop_info->prop,
              prop_info->v_uint);
        break;
    case WEBRTC_STATS_PROP_TYPE_UINT64:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%" PRIu64 "]", type, prop_info->name,
              prop_info->prop, prop_info->v_uint64);
        break;
    case WEBRTC_STATS_PROP_TYPE_FLOAT:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%f]", type, prop_info->name, prop_info->prop,
              prop_info->v_float);
        break;
    case WEBRTC_STATS_PROP_TYPE_DOUBLE:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%lf]", type, prop_info->name, prop_info->prop,
              prop_info->v_double);
        break;
    case WEBRTC_STATS_PROP_TYPE_STRING:
        DBG("type[0x%x] prop[%s, 0x%08x, value:%s]", type, prop_info->name, prop_info->prop,
              prop_info->v_string);
        break;
    }
    return true;
}

void WebRtcStream::PrintStats(void)
{
    if (webrtc_foreach_stats(webrtc_handle_, WEBRTC_STATS_TYPE_INBOUND_RTP, __stats_cb, nullptr)
          != WEBRTC_ERROR_NONE)
        DBG("webrtc_foreach_stats failed");
}

void WebRtcStream::AddDataChannel(void)
{
    webrtc_create_data_channel(webrtc_handle_, "label", nullptr, &channel_);
}

void WebRtcStream::AttachSignals(bool is_source)
{
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

    if (!is_source) {
        ret = webrtc_set_encoded_video_frame_cb(webrtc_handle_, OnEncodedFrame, this);
        ERR("webrtc_set_encoded_video_frame_cb %s",
              ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
        ret = webrtc_set_track_added_cb(webrtc_handle_, OnTrackAdded, this);
        ERR("webrtc_set_track_added_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");
    }

    ret = webrtc_data_channel_set_open_cb(channel_, OnDataChannelOpen, this);
    DBG("webrtc_data_channel_set_open_cb %s", ret == WEBRTC_ERROR_NONE ? "Succeeded" : "failed");

    return;
}

void WebRtcStream::DetachSignals(void)
{
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

WebRtcEventHandler &WebRtcStream::GetEventHandler(void)
{
    return event_handler_;
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

    if (current == WEBRTC_STATE_NEGOTIATING)
        webrtc_stream->SetPeerSDP();

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
    webrtc_stream->GetEventHandler().CallOnIceCandidateCb(candidate);
}

void WebRtcStream::OnEncodedFrame(webrtc_h webrtc, webrtc_media_type_e type, unsigned int track_id,
      media_packet_h packet, void *user_data)
{
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);

    if (type == WEBRTC_MEDIA_TYPE_VIDEO)
        webrtc_stream->GetEventHandler().CallOnEncodedFrameCb(packet);
}

void WebRtcStream::OnTrackAdded(webrtc_h webrtc, webrtc_media_type_e type, unsigned int id,
      void *user_data)
{
    // type AUDIO(0), VIDEO(1)
    INFO("Added Track : id(%d), type(%s)", id, type ? "Video" : "Audio");
    RET_IF(!user_data || type != WEBRTC_MEDIA_TYPE_VIDEO);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);

    if (webrtc_stream->display_object_)
        webrtc_set_display(webrtc, id, WEBRTC_DISPLAY_TYPE_EVAS, webrtc_stream->display_object_);
    else
        webrtc_stream->GetEventHandler().CallOnTrakAddedCb(id);
}

void WebRtcStream::OnDataChannelOpen(webrtc_data_channel_h channel, void *user_data)
{
    ERR("%s", __func__);
}

void WebRtcStream::OnBufferStateChanged(unsigned int id,
      webrtc_media_packet_source_buffer_state_e state, void *user_data)
{
    ERR("%s", __func__);
    auto webrtc_stream = static_cast<WebRtcStream *>(user_data);
    RET_IF(webrtc_stream == nullptr);
    webrtc_stream->GetEventHandler().CallOnSourceBufferStateCb(
          WebRtcState::ToSourceBufferState(state));
}

static media_format_mimetype_e __get_video_format_enum(const std::string &format)
{
    if (format == "I420")
        return MEDIA_FORMAT_I420;
    else if (format == "NV12")
        return MEDIA_FORMAT_NV12;
    else if (format == "VP8")
        return MEDIA_FORMAT_VP8;
    else if (format == "VP9")
        return MEDIA_FORMAT_VP9;
    else if (format == "H264")
        return MEDIA_FORMAT_H264_HP;
    else if (format == "JPEG")
        return MEDIA_FORMAT_MJPEG;
    else
        return MEDIA_FORMAT_MAX;
}

void *WebRtcStream::GetMediaFormatHandler(int width, int height, int frame_rate,
      const std::string &format)
{
    media_format_h media_format = nullptr;
    auto ret = media_format_create(&media_format);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
        DBG("failed to media_format_create()");
        return nullptr;
    }

    ret = media_format_set_video_mime(media_format, __get_video_format_enum(format));
    ret |= media_format_set_video_width(media_format, width);
    ret |= media_format_set_video_height(media_format, height);
    ret |= media_format_set_video_frame_rate(media_format, frame_rate);
    if (ret != MEDIA_FORMAT_ERROR_NONE) {
        DBG("failed to set video format");
        media_format_unref(media_format);
        return nullptr;
    }

    return media_format;
}
}  // namespace AittWebRTCNamespace
