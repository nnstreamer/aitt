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
#include "SubscribeStream.h"

#include "WebRtcEventHandler.h"
#include "aitt_internal.h"

SubscribeStream::~SubscribeStream()
{
    // TODO Clear resources
}

void SubscribeStream::Start(bool need_display, void *display_object)
{
    display_object_ = display_object;
    is_track_added_ = need_display;
    SetSignalingServerCallbacks();
    SetRoomCallbacks();
    try {
        server_->Connect();
    } catch (const std::exception &e) {
        ERR("Failed to start Subscribe stream %s", e.what());
    }
}

void SubscribeStream::SetSignalingServerCallbacks(void)
{
    auto on_signaling_server_connection_state_changed =
          std::bind(OnSignalingServerConnectionStateChanged, std::placeholders::_1,
                std::ref(*room_), std::ref(*server_));

    server_->SetConnectionStateChangedCb(on_signaling_server_connection_state_changed);

    auto on_room_message_arrived =
          std::bind(OnRoomMessageArrived, std::placeholders::_1, std::ref(*room_));

    server_->SetRoomMessageArrivedCb(on_room_message_arrived);
}

void SubscribeStream::OnSignalingServerConnectionStateChanged(IfaceServer::ConnectionState state,
      WebRtcRoom &room, MqttServer &server)
{
    // TODO VD doesn't show DBG level log
    ERR("current state [%s]", MqttServer::GetConnectionStateStr(state).c_str());

    if (state == IfaceServer::ConnectionState::Disconnected) {
        ;  // TODO: what to do when server is disconnected?
    } else if (state == IfaceServer::ConnectionState::Registered) {
        if (server.GetSourceId().size() != 0)
            room.SetSourceId(server.GetSourceId());
        server.JoinRoom(room.getId());
    }
}

void SubscribeStream::OnRoomMessageArrived(const std::string &message, WebRtcRoom &room)
{
    room.handleMessage(message);
}

void SubscribeStream::SetRoomCallbacks(void)
{
    auto on_room_joined = std::bind(OnRoomJoined, std::ref(*this));

    room_->SetRoomJoinedCb(on_room_joined);

    auto on_peer_joined = std::bind(OnPeerJoined, std::placeholders::_1, std::ref(*this));
    room_->SetPeerJoinedCb(on_peer_joined);

    auto on_peer_left = std::bind(OnPeerLeft, std::placeholders::_1, std::ref(*room_));
    room_->SetPeerLeftCb(on_peer_left);
}

void SubscribeStream::OnRoomJoined(SubscribeStream &subscribe_stream)
{
    // TODO: Notify Room Joined?
    ERR("%s on %p", __func__, &subscribe_stream);
}

void SubscribeStream::OnPeerJoined(const std::string &peer_id, SubscribeStream &subscribe_stream)
{
    ERR("%s [%s]", __func__, peer_id.c_str());

    if (peer_id.compare(subscribe_stream.room_->GetSourceId()) != 0) {
        ERR("is not matched to source ID, ignored");
        return;
    }

    if (!subscribe_stream.room_->AddPeer(peer_id)) {
        ERR("Failed to add peer");
        return;
    }

    try {
        WebRtcPeer &peer = subscribe_stream.room_->GetPeer(peer_id);

        auto webrtc_subscribe_stream = peer.GetWebRtcStream();
        webrtc_subscribe_stream->Create(false, subscribe_stream.is_track_added_);
        webrtc_subscribe_stream->Start();
        subscribe_stream.SetWebRtcStreamCallbacks(peer);
    } catch (std::out_of_range &e) {
        ERR("Wired %s", e.what());
    }
}

void SubscribeStream::SetWebRtcStreamCallbacks(WebRtcPeer &peer)
{
    WebRtcEventHandler event_handlers;

    auto on_signaling_state_notify = std::bind(OnSignalingStateNotify, std::placeholders::_1,
          std::ref(peer), std::ref(*server_));
    event_handlers.SetOnSignalingStateNotifyCb(on_signaling_state_notify);

    auto on_ice_connection_state_notify = std::bind(OnIceConnectionStateNotify,
          std::placeholders::_1, std::ref(peer), std::ref(*server_));
    event_handlers.SetOnIceConnectionStateNotifyCb(on_ice_connection_state_notify);

    auto on_encoded_frame = std::bind(OnEncodedFrame, std::ref(peer));
    event_handlers.SetOnEncodedFrameCb(on_encoded_frame);

    auto on_track_added =
          std::bind(OnTrackAdded, std::placeholders::_1, display_object_, std::ref(peer));
    event_handlers.SetOnTrakAddedCb(on_track_added);

    peer.GetWebRtcStream()->SetEventHandler(event_handlers);
}

void SubscribeStream::OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcPeer &peer,
      MqttServer &server)
{
    ERR("Singaling State: %s", WebRtcState::SignalingToStr(state).c_str());
    if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
        auto on_answer_created_cb =
              std::bind(OnAnswerCreated, std::placeholders::_1, std::ref(peer), std::ref(server));
        peer.GetWebRtcStream()->CreateAnswerAsync(on_answer_created_cb);
    }
}

void SubscribeStream::OnIceConnectionStateNotify(WebRtcState::IceConnection state, WebRtcPeer &peer,
      MqttServer &server)
{
    ERR("IceConnection State: %s", WebRtcState::IceConnectionToStr(state).c_str());
    if (state == WebRtcState::IceConnection::CHECKING) {
        auto ice_candidates = peer.GetWebRtcStream()->GetIceCandidates();
        for (const auto &candidate : ice_candidates)
            server.SendMessage(peer.getId(), candidate);
    }
}

void SubscribeStream::OnAnswerCreated(std::string sdp, WebRtcPeer &peer, MqttServer &server)
{
    server.SendMessage(peer.getId(), sdp);
    peer.GetWebRtcStream()->SetLocalDescription(sdp);
}

void SubscribeStream::OnEncodedFrame(WebRtcPeer &peer)
{
    // TODO
}

void SubscribeStream::OnTrackAdded(unsigned int id, void *display_object, WebRtcPeer &peer)
{
    peer.GetWebRtcStream()->SetDisplayObject(id, display_object);
}

void SubscribeStream::OnPeerLeft(const std::string &peer_id, WebRtcRoom &room)
{
    /*TODO
    ERR("%s [%s]", __func__, peer_id.c_str());
    if (peer_id.compare(room.getSourceId()) != 0) {
        ERR("is not matched to source ID, ignored");
        return;
    }
    */
    if (!room.RemovePeer(peer_id))
        ERR("Failed to remove peer");
}

void *SubscribeStream::Stop(void)
{
    try {
        server_->Disconnect();
    } catch (const std::exception &e) {
        ERR("Failed to disconnect server %s", e.what());
    }

    room_->ClearPeers();

    return display_object_;
}
