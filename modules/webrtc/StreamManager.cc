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

#include "StreamManager.h"

#include "aitt_internal.h"

namespace AittWebRTCNamespace {

StreamManager::StreamManager(const std::string &topic, const std::string &watching_topic,
      const std::string &aitt_id, const std::string &thread_id)
      : need_display_(false),
        is_started_(false),
        width_(0),
        height_(0),
        frame_rate_(0),
        source_type_("CAMERA"),
        topic_(topic),
        watching_topic_(watching_topic),
        aitt_id_(aitt_id),
        thread_id_(thread_id)
{
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

void StreamManager::SetFormat(const std::string &format, int width, int height)
{
    format_ = format;
    width_ = width;
    height_ = height;
}

void StreamManager::SetWidth(int width)
{
    width_ = width;
}

void StreamManager::SetHeight(int height)
{
    height_ = height;
}

void StreamManager::SetFrameRate(int frame_rate)
{
    frame_rate_ = frame_rate;
}

void StreamManager::SetSourceType(const std::string &source_type)
{
    source_type_ = source_type;
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

void StreamManager::SetOnFrameCallback(OnFrameCallback cb)
{
    on_frame_cb_ = cb;
}

}  // namespace AittWebRTCNamespace
