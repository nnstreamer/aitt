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

#include "WebRtcPeer.h"

#include "WebRtcMessage.h"
#include "aitt_internal.h"

WebRtcPeer::WebRtcPeer(const std::string &peer_id)
      : local_id_(peer_id), webrtc_stream_(std::make_shared<WebRtcStream>())
{
    DBG("%s", __func__);
}

WebRtcPeer::~WebRtcPeer()
{
    webrtc_stream_ = nullptr;
    DBG("%s removed", local_id_.c_str());
}

std::shared_ptr<WebRtcStream> WebRtcPeer::GetWebRtcStream(void) const
{
    return webrtc_stream_;
}

void WebRtcPeer::SetWebRtcStream(std::shared_ptr<WebRtcStream> webrtc_stream)
{
    webrtc_stream_ = webrtc_stream;
}

std::string WebRtcPeer::getId(void) const
{
    return local_id_;
}

void WebRtcPeer::HandleMessage(const std::string &message)
{
    WebRtcMessage::Type type = WebRtcMessage::getMessageType(message);
    if (type == WebRtcMessage::Type::SDP)
        webrtc_stream_->SetRemoteDescription(message);
    else if (type == WebRtcMessage::Type::ICE)
        webrtc_stream_->AddIceCandidateFromMessage(message);
    else
        DBG("%s can't handle %s", __func__, message.c_str());
}
