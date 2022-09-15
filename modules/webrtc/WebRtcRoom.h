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
#include <map>
#include <memory>
#include <string>

#include "WebRtcPeer.h"

class WebRtcRoom {
  public:
    enum class State {
        JOINNING,
        JOINED,
    };
    WebRtcRoom() = delete;
    explicit WebRtcRoom(const std::string &room_id) : id_(room_id), state_(State::JOINNING){};
    ~WebRtcRoom();
    void setRoomState(State current) { state_ = current; }
    State getRoomState(void) const { return state_; };
    void handleMessage(const std::string &msg);
    bool AddPeer(const std::string &peer_id);
    bool RemovePeer(const std::string &peer_id);
    void ClearPeers(void);
    // You need to handle out_of_range exception if there's no matching peer;
    WebRtcPeer &GetPeer(const std::string &peer_id);
    std::string getId(void) const { return id_; };
    void SetSourceId(const std::string &source_id) { source_id_ = source_id; };
    std::string GetSourceId(void) const { return source_id_; };

    void SetRoomJoinedCb(std::function<void(void)> on_room_joined_cb)
    {
        on_room_joined_cb_ = on_room_joined_cb;
    };
    void CallRoomJoinedCb(void) const
    {
        if (on_room_joined_cb_)
            on_room_joined_cb_();
    };
    void UnsetRoomJoinedCb(void) { on_room_joined_cb_ = nullptr; };
    void SetPeerJoinedCb(std::function<void(const std::string &peer_id)> on_peer_joined_cb)
    {
        on_peer_joined_cb_ = on_peer_joined_cb;
    };
    void CallPeerJoinedCb(const std::string &peer_id) const
    {
        if (on_peer_joined_cb_)
            on_peer_joined_cb_(peer_id);
    };
    void UnsetPeerJoinedCb(void) { on_peer_joined_cb_ = nullptr; };
    void SetPeerLeftCb(std::function<void(const std::string &peer_id)> on_peer_left_cb)
    {
        on_peer_left_cb_ = on_peer_left_cb;
    };
    void CallPeerLeftCb(const std::string &peer_id) const
    {
        if (on_peer_left_cb_)
            on_peer_left_cb_(peer_id);
    };
    void UnsetPeerLeftCb(void) { on_peer_left_cb_ = nullptr; };

  private:
    void HandleRoomJoinedWithPeerList(const std::string &peer_list);
    void HandlePeerMessage(const std::string &msg);

  private:
    std::string id_;
    std::string source_id_;
    std::map<std::string, std::shared_ptr<WebRtcPeer>> peers_;
    State state_;
    std::function<void(void)> on_room_joined_cb_;
    std::function<void(const std::string &peer_id)> on_peer_joined_cb_;
    std::function<void(const std::string &peer_id)> on_peer_left_cb_;
};
