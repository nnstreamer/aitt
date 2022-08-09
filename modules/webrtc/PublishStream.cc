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
#include "PublishStream.h"

#include <sys/time.h>

#include "WebRtcEventHandler.h"
#include "aitt_internal.h"

PublishStream::~PublishStream()
{
    // TODO: clear resources
}

void PublishStream::Start(void)
{
    PrepareStream();
    SetSignalingServerCallbacks();
    SetRoomCallbacks();
}

void PublishStream::PrepareStream(void)
{
    std::lock_guard<std::mutex> prepared_stream_lock(prepared_stream_lock_);
    prepared_stream_ = std::make_shared<WebRtcStream>();
    prepared_stream_->Create(true, false);
    prepared_stream_->AttachCameraSource();
    auto on_stream_state_changed_prepared_cb =
          std::bind(OnStreamStateChangedPrepared, std::placeholders::_1, std::ref(*this));
    prepared_stream_->GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_prepared_cb);
    prepared_stream_->Start();
}

void PublishStream::OnStreamStateChangedPrepared(WebRtcState::Stream state, PublishStream &stream)
{
    ERR("%s", __func__);
    if (state == WebRtcState::Stream::NEGOTIATING) {
        auto on_offer_created_prepared_cb =
              std::bind(OnOfferCreatedPrepared, std::placeholders::_1, std::ref(stream));
        stream.prepared_stream_->CreateOfferAsync(on_offer_created_prepared_cb);
    }
}

void PublishStream::OnOfferCreatedPrepared(std::string sdp, PublishStream &stream)
{
    ERR("%s", __func__);

    stream.prepared_stream_->SetPreparedLocalDescription(sdp);
    stream.prepared_stream_->SetLocalDescription(sdp);
    try {
        stream.server_->Connect();
    } catch (const std::exception &e) {
        ERR("Failed to start Publish stream %s", e.what());
    }
}

void PublishStream::SetSignalingServerCallbacks(void)
{
    auto on_signaling_server_connection_state_changed =
          std::bind(OnSignalingServerConnectionStateChanged, std::placeholders::_1,
                std::ref(*room_), std::ref(*server_));

    server_->SetConnectionStateChangedCb(on_signaling_server_connection_state_changed);

    auto on_room_message_arrived =
          std::bind(OnRoomMessageArrived, std::placeholders::_1, std::ref(*room_));

    server_->SetRoomMessageArrivedCb(on_room_message_arrived);
}

void PublishStream::OnSignalingServerConnectionStateChanged(IfaceServer::ConnectionState state,
      WebRtcRoom &room, MqttServer &server)
{
    DBG("current state [%s]", MqttServer::GetConnectionStateStr(state).c_str());

    if (state == IfaceServer::ConnectionState::Disconnected) {
        ;  // TODO: what to do when server is disconnected?
    } else if (state == IfaceServer::ConnectionState::Registered) {
        server.JoinRoom(room.getId());
    }
}

void PublishStream::OnRoomMessageArrived(const std::string &message, WebRtcRoom &room)
{
    room.handleMessage(message);
}

void PublishStream::SetRoomCallbacks()
{
    auto on_room_joined = std::bind(OnRoomJoined, std::ref(*this));

    room_->SetRoomJoinedCb(on_room_joined);

    auto on_peer_joined = std::bind(OnPeerJoined, std::placeholders::_1, std::ref(*this));
    room_->SetPeerJoinedCb(on_peer_joined);

    auto on_peer_left = std::bind(OnPeerLeft, std::placeholders::_1, std::ref(*room_));
    room_->SetPeerLeftCb(on_peer_left);
}

void PublishStream::OnRoomJoined(PublishStream &publish_stream)
{
    // TODO: Notify Room Joined?
    DBG("%s on %p", __func__, &publish_stream);
}

void PublishStream::OnPeerJoined(const std::string &peer_id, PublishStream &publish_stream)
{
    DBG("%s [%s]", __func__, peer_id.c_str());
    if (!publish_stream.room_->AddPeer(peer_id)) {
        ERR("Failed to add peer");
        return;
    }

    try {
        WebRtcPeer &peer = publish_stream.room_->GetPeer(peer_id);

        std::unique_lock<std::mutex> prepared_stream_lock(publish_stream.prepared_stream_lock_);
        auto prepared_stream = publish_stream.prepared_stream_;
        publish_stream.prepared_stream_ = nullptr;
        prepared_stream_lock.unlock();

        try {
            peer.SetWebRtcStream(prepared_stream);
            publish_stream.SetWebRtcStreamCallbacks(peer);
            publish_stream.server_->SendMessage(peer.getId(),
                  peer.GetWebRtcStream()->GetPreparedLocalDescription());
            prepared_stream->SetPreparedLocalDescription("");
        } catch (std::exception &e) {
            ERR("Failed to start stream for peer %s", e.what());
        }
        // TODO why we can't prepare more sources?

    } catch (std::exception &e) {
        ERR("Wired %s", e.what());
    }
}

void PublishStream::SetWebRtcStreamCallbacks(WebRtcPeer &peer)
{
    // TODO: set more webrtc callbacks
    WebRtcEventHandler event_handlers;
    auto on_stream_state_changed_cb = std::bind(OnStreamStateChanged, std::placeholders::_1,
          std::ref(peer), std::ref(*server_));
    event_handlers.SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_signaling_state_notify_cb = std::bind(OnSignalingStateNotify, std::placeholders::_1,
          std::ref(peer), std::ref(*server_));
    event_handlers.SetOnSignalingStateNotifyCb(on_signaling_state_notify_cb);

    auto on_ice_connection_state_notify = std::bind(OnIceConnectionStateNotify,
          std::placeholders::_1, std::ref(peer), std::ref(*server_));
    event_handlers.SetOnIceConnectionStateNotifyCb(on_ice_connection_state_notify);

    peer.GetWebRtcStream()->SetEventHandler(event_handlers);
}

void PublishStream::OnStreamStateChanged(WebRtcState::Stream state, WebRtcPeer &peer,
      MqttServer &server)
{
    ERR("%s for %s", __func__, peer.getId().c_str());
}

void PublishStream::OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcPeer &peer,
      MqttServer &server)
{
    ERR("Singaling State: %s", WebRtcState::SignalingToStr(state).c_str());
    if (state == WebRtcState::Signaling::STABLE) {
        auto ice_candidates = peer.GetWebRtcStream()->GetIceCandidates();
        for (const auto &candidate : ice_candidates)
            server.SendMessage(peer.getId(), candidate);
    }
}

void PublishStream::OnIceConnectionStateNotify(WebRtcState::IceConnection state, WebRtcPeer &peer,
      MqttServer &server)
{
    ERR("IceConnection State: %s", WebRtcState::IceConnectionToStr(state).c_str());
}

void PublishStream::OnPeerLeft(const std::string &peer_id, WebRtcRoom &room)
{
    DBG("%s [%s]", __func__, peer_id.c_str());
    if (!room.RemovePeer(peer_id))
        ERR("Failed to remove peer");
}

void PublishStream::Stop(void)
{
    try {
        server_->Disconnect();
    } catch (const std::exception &e) {
        ERR("Failed to disconnect server %s", e.what());
    }

    room_->ClearPeers();
}
