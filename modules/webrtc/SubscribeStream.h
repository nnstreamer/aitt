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
#include <mutex>

#include "Config.h"
#include "MqttServer.h"
#include "WebRtcRoom.h"
#include "WebRtcStream.h"

class SubscribeStream {
  public:
    SubscribeStream() = delete;
    SubscribeStream(const std::string &topic, const Config &config)
          : topic_(topic),
            config_(config),
            server_(std::make_shared<MqttServer>(config)),
            room_(std::make_shared<WebRtcRoom>(config.GetRoomId())),
            is_track_added_(false),
            display_object_(nullptr){};
    ~SubscribeStream();

    // TODO what will be final form of callback
    void Start(bool need_display, void *display_object);
    void *Stop(void);
    void SetSignalingServerCallbacks(void);
    void SetRoomCallbacks(void);
    void SetWebRtcStreamCallbacks(WebRtcPeer &peer);

  private:
    static void OnSignalingServerConnectionStateChanged(IfaceServer::ConnectionState state,
          WebRtcRoom &room, MqttServer &server);
    static void OnRoomMessageArrived(const std::string &message, WebRtcRoom &room);
    static void OnRoomJoined(SubscribeStream &subscribe_stream);
    static void OnPeerJoined(const std::string &peer_id, SubscribeStream &subscribe_stream);
    static void OnPeerLeft(const std::string &peer_id, WebRtcRoom &room);
    static void OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcPeer &peer,
          MqttServer &server);
    static void OnIceConnectionStateNotify(WebRtcState::IceConnection state, WebRtcPeer &peer,
          MqttServer &server);
    static void OnAnswerCreated(std::string sdp, WebRtcPeer &peer, MqttServer &server);
    static void OnEncodedFrame(WebRtcPeer &peer);
    static void OnTrackAdded(unsigned int id, void *dispaly_object, WebRtcPeer &peer);

  private:
    std::string topic_;
    Config config_;
    std::shared_ptr<MqttServer> server_;
    std::shared_ptr<WebRtcRoom> room_;
    bool is_track_added_;
    void *display_object_;
};
