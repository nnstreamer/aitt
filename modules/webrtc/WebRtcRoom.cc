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

#include "WebRtcRoom.h"

#include <sstream>
#include <stdexcept>

#include "aitt_internal.h"

WebRtcRoom::~WebRtcRoom()
{
    //TODO How about removing handling webrtc_stream part from Room?
    for (auto pair : peers_) {
        auto peer = pair.second;
        auto webrtc_stream = peer->GetWebRtcStream();
        webrtc_stream->Stop();
        webrtc_stream->Destroy();
    }
}

static std::vector<std::string> split(const std::string &line, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream input(line);

    std::string buffer;
    while (getline(input, buffer, delimiter)) {
        result.push_back(buffer);
    }

    return result;
}

void WebRtcRoom::handleMessage(const std::string &msg)
{
    if (msg.compare("ROOM_OK ") == 0)
        CallRoomJoinedCb();
    else if (msg.compare(0, 8, "ROOM_OK ") == 0) {
        CallRoomJoinedCb();
        HandleRoomJoinedWithPeerList(msg.substr(8, std::string::npos));
    } else if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        CallPeerJoinedCb(msg.substr(17, std::string::npos));
    } else if (msg.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        CallPeerLeftCb(msg.substr(15, std::string::npos));
    } else if (msg.compare(0, 13, "ROOM_PEER_MSG") == 0) {
        HandlePeerMessage(msg.substr(14, std::string::npos));
    } else {
        DBG("Not defined");
    }

    return;
}

void WebRtcRoom::HandleRoomJoinedWithPeerList(const std::string &peer_list)
{
    auto peer_ids = split(peer_list, ' ');
    for (const auto &peer_id : peer_ids) {
        CallPeerJoinedCb(peer_id);
    }
}

void WebRtcRoom::HandlePeerMessage(const std::string &msg)
{
    std::size_t pos = msg.find(' ');
    if (pos == std::string::npos) {
        DBG("%s can't handle %s", __func__, msg.c_str());
        return;
    }

    auto peer_id = msg.substr(0, pos);
    auto itr = peers_.find(peer_id);
    if (itr == peers_.end()) {
        ERR("%s is not in peer list", peer_id.c_str());
        //Opening backdoor here for source. What'll be crisis for this?
        CallPeerJoinedCb(peer_id);
        itr = peers_.find(peer_id);
        RET_IF(itr == peers_.end());
    }

    itr->second->HandleMessage(msg.substr(pos + 1, std::string::npos));
}

bool WebRtcRoom::AddPeer(const std::string &peer_id)
{
    auto peer = std::make_shared<WebRtcPeer>(peer_id);
    auto ret = peers_.insert(std::make_pair(peer_id, peer));

    return ret.second;
}

bool WebRtcRoom::RemovePeer(const std::string &peer_id)
{
    auto itr = peers_.find(peer_id);
    if (itr == peers_.end()) {
        DBG("There's no such peer");
        return false;
    }
    auto peer = itr->second;

    //TODO How about removing handling webrtc_stream part from Room?
    auto webrtc_stream = peer->GetWebRtcStream();
    webrtc_stream->Stop();
    webrtc_stream->Destroy();

    return peers_.erase(peer_id) == 1;
}

WebRtcPeer &WebRtcRoom::GetPeer(const std::string &peer_id)
{
    auto itr = peers_.find(peer_id);
    if (itr == peers_.end())
        throw std::out_of_range("There's no such peer");

    return *itr->second;
}

void WebRtcRoom::ClearPeers(void)
{
    //TODO How about removing handling webrtc_stream part from Room?
    for (auto pair : peers_) {
        auto peer = pair.second;
        auto webrtc_stream = peer->GetWebRtcStream();
        webrtc_stream->Stop();
        webrtc_stream->Destroy();
    }
    peers_.clear();
}
