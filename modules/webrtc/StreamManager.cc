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

#include "StreamManager.h"

#include "AittException.h"
#include "aitt_internal.h"

namespace AittWebRTCNamespace {

StreamManager::StreamManager(const std::string &topic, const std::string &watching_topic,
      const std::string &aitt_id, const std::string &thread_id)
      : is_started_(false),
        width_(0),
        height_(0),
        frame_rate_(0),
        // Need to sync with source_type_ on stream
        source_type_("NULL"),
        decode_codec_("VP8"),
        topic_(topic),
        watching_topic_(watching_topic),
        aitt_id_(aitt_id),
        thread_id_(thread_id)
{
    stream_.AddDataChannel();
}

bool StreamManager::IsStarted(void) const
{
    return is_started_;
}

std::string StreamManager::GetFormat(void)
{
    return format_;
}

int StreamManager::GetWidth(void)
{
    return width_;
}

int StreamManager::GetHeight(void)
{
    return height_;
}

void StreamManager::SetWidth(int width)
{
    if (width_ == width)
        return;

    if (source_type_ == "MEDIA_PACKET" && height_ && frame_rate_ && format_.size())
        stream_.SetMediaFormat(width_, height_, frame_rate_, format_);
    else if (source_type_ == "CAMERA" && height_)
        stream_.SetVideoResolution(width, height_);
    width_ = width;
}

void StreamManager::SetHeight(int height)
{
    if (height_ == height)
        return;

    if (source_type_ == "MEDIA_PACKET" && width_ && frame_rate_ && format_.size())
        stream_.SetMediaFormat(width_, height, frame_rate_, format_);
    else if (source_type_ == "CAMERA" && width_)
        stream_.SetVideoResolution(width_, height);
    height_ = height;
}

void StreamManager::SetFrameRate(int frame_rate)
{
    if (frame_rate_ == frame_rate)
        return;

    if (source_type_ == "MEDIA_PACKET" && width_ && height_ && format_.size())
        stream_.SetMediaFormat(width_, height_, frame_rate, format_);
    else if (source_type_ == "CAMERA")
        stream_.SetVideoFrameRate(frame_rate);
    frame_rate_ = frame_rate;
}

void StreamManager::SetSourceType(const std::string &source_type)
{
    if (source_type_ == source_type)
        return;

    stream_.DeactivateSource();
    if (source_type == "MEDIA_PACKET") {
        stream_.SetSourceType(WEBRTC_MEDIA_SOURCE_TYPE_MEDIA_PACKET);
        stream_.ActivateSource();
        if (width_ && height_ && frame_rate_ && format_.size())
            stream_.SetMediaFormat(width_, height_, frame_rate_, format_);
    } else if (source_type == "CAMERA") {
        stream_.SetSourceType(WEBRTC_MEDIA_SOURCE_TYPE_CAMERA);
        stream_.ActivateSource();
        if (width_ && height_)
            stream_.SetVideoResolution(width_, height_);
        if (frame_rate_)
            stream_.SetVideoFrameRate(frame_rate_);
    } else if (source_type == "NULL") {
        stream_.SetSourceType(WEBRTC_MEDIA_SOURCE_TYPE_NULL);
        stream_.ActivateSource();
    } else
        DBG("%s is not available source type", source_type.c_str());

    source_type_ = source_type;
}

void StreamManager::SetMediaFormat(const std::string &format)
{
    if (format_ == format)
        return;

    IsSupportedFormat(format);

    if (source_type_ == "MEDIA_PACKET" && width_ && height_ && frame_rate_)
        stream_.SetMediaFormat(width_, height_, frame_rate_, format);
    format_ = format;
}

void StreamManager::SetDecodeCodec(const std::string &codec)
{
    if (decode_codec_ == codec)
        return;

    IsSupportedFormat(codec);

    if (source_type_ == "NULL")
        stream_.SetDecodeCodec(codec);
    decode_codec_ = codec;
}

void StreamManager::SetDisplay(void *display_object)
{
    stream_.SetDisplay(display_object);
}

void StreamManager::Start(void)
{
    DBG("%s %s", __func__, GetTopic().c_str());
    if (stream_start_cb_)
        stream_start_cb_();
    is_started_ = true;
}

void StreamManager::Stop(void)
{
    DBG("%s %s", __func__, GetTopic().c_str());
    peer_aitt_id_.clear();
    stream_.Destroy();

    if (stream_stop_cb_)
        stream_stop_cb_();
    is_started_ = false;
}

void StreamManager::HandleRemovedClient(const std::string &discovery_id)
{
    if (peer_aitt_id_ != discovery_id) {
        ERR("There's no stream %s", discovery_id.c_str());
        return;
    }

    stream_.Destroy();

    return;
}

void StreamManager::HandleMsg(const std::string &discovery_id, const std::vector<uint8_t> &message)
{
    if (flexbuffers::GetRoot(message).IsString())
        HandleStreamState(discovery_id, message);
    else if (flexbuffers::GetRoot(message).IsVector())
        HandleStreamInfo(discovery_id, message);
}

std::string StreamManager::GetTopic(void) const
{
    return topic_;
}

std::string StreamManager::GetWatchingTopic(void) const
{
    return watching_topic_;
}

void StreamManager::SetIceCandidateAddedCallback(IceCandidateAddedCallback cb)
{
    ice_candidate_added_cb_ = cb;
}

void StreamManager::SetStreamStartCallback(StreamStartCallback cb)
{
    stream_start_cb_ = cb;
}

void StreamManager::SetStreamStopCallback(StreamStopCallback cb)
{
    stream_stop_cb_ = cb;
}

void StreamManager::SetStreamStateCallback(StreamStateCallback cb)
{
    stream_state_cb_ = cb;
}

void StreamManager::SetOnFrameCallback(OnFrameCallback cb)
{
    on_frame_cb_ = cb;
}

void StreamManager::IsSupportedFormat(const std::string &format)
{
    if (format != "VP8" && format != "H264")
        throw aitt::AittException(aitt::AittException::INVALID_ARG);
}

}  // namespace AittWebRTCNamespace
