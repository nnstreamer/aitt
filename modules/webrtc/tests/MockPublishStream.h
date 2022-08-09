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

#include <memory>
#include <string>

#include "Config.h"
#include "MqttServer.h"
#include "WebRtcRoom.h"

class MockPublishStream {
    // TODO: Notify & get status
  public:
    MockPublishStream() = delete;
    MockPublishStream(const std::string &topic, const Config &config)
          : topic_(topic), config_(config), server_(std::make_shared<MqttServer>(config))
    {
        config.SetSourceId(config.GetLocalId());
        config.SetRoomId(config::MQTT_ROOM_PREFIX() + topic);
        room_ = std::make_shared<WebRtcRoom>(config.GetRoomId());
    };
    ~MockPublishStream();

    void Start(void)
    {
        SetSignalingServerCallbacks();
        SetRoomCallbacks();
        server_->Connect();
    }
    void Stop(void){
          // TODO
    };

  private:
    void SetSignalingServerCallbacks(void)
    {
        auto on_signaling_server_connection_state_changed =
              std::bind(OnSignalingServerConnectionStateChanged, std::placeholders::_1,
                    std::ref(*room_), std::ref(*server_));

        server_->SetConnectionStateChangedCb(on_signaling_server_connection_state_changed);

        auto on_room_message_arrived =
              std::bind(OnRoomMessageArrived, std::placeholders::_1, std::ref(*room_));

        server_->SetRoomMessageArrivedCb(on_room_message_arrived);
    };

    void SetRoomCallbacks(void)
    {
        auto on_peer_joined =
              std::bind(OnPeerJoined, std::placeholders::_1, std::ref(*server_), std::ref(*room_));
        room_->SetPeerJoinedCb(on_peer_joined);

        auto on_peer_left = std::bind(OnPeerLeft, std::placeholders::_1, std::ref(*room_));
        room_->SetPeerLeftCb(on_peer_left);
    };

    static void OnSignalingServerConnectionStateChanged(IfaceServer::ConnectionState state,
          WebRtcRoom &room, MqttServer &server)
    {
        DBG("current state [%s]", SignalingServer::GetConnectionStateStr(state).c_str());

        if (state == IfaceServer::ConnectionState::Registered)
            server.JoinRoom(room.getId());
    };

    static void OnRoomMessageArrived(const std::string &message, WebRtcRoom &room)
    {
        room.handleMessage(message);
    };

    static void OnPeerJoined(const std::string &peer_id, MqttServer &server, WebRtcRoom &room)
    {
        DBG("%s [%s]", __func__, peer_id.c_str());
    };

    static void OnPeerLeft(const std::string &peer_id, WebRtcRoom &room)
    {
        DBG("%s [%s]", __func__, peer_id.c_str());
    };

  private:
    std::string topic_;
    config config_;
    std::shared_ptr<MqttServer> server_;
    std::shared_ptr<WebRtcRoom> room_;
};
