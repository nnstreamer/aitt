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

#include "WebRtcState.h"

namespace AittWebRTCNamespace {

WebRtcState::Stream WebRtcState::ToStreamState(webrtc_state_e state)
{
    switch (state) {
    case WEBRTC_STATE_IDLE: {
        return Stream::IDLE;
    }
    case WEBRTC_STATE_NEGOTIATING: {
        return Stream::NEGOTIATING;
    }
    case WEBRTC_STATE_PLAYING: {
        return Stream::PLAYING;
    }
    }
    return Stream::IDLE;
}

std::string WebRtcState::StreamToStr(WebRtcState::Stream state)
{
    switch (state) {
    case Stream::IDLE: {
        return std::string("IDLE");
    }
    case Stream::NEGOTIATING: {
        return std::string("NEGOTIATING");
    }
    case Stream::PLAYING: {
        return std::string("PLAYING");
    }
    }
    return std::string("");
}

WebRtcState::Signaling WebRtcState::ToSignalingState(webrtc_signaling_state_e state)
{
    switch (state) {
    case WEBRTC_SIGNALING_STATE_STABLE: {
        return Signaling::STABLE;
    }
    case WEBRTC_SIGNALING_STATE_HAVE_LOCAL_OFFER: {
        return Signaling::HAVE_LOCAL_OFFER;
    }
    case WEBRTC_SIGNALING_STATE_HAVE_REMOTE_OFFER: {
        return Signaling::HAVE_REMOTE_OFFER;
    }
    case WEBRTC_SIGNALING_STATE_HAVE_LOCAL_PRANSWER: {
        return Signaling::HAVE_LOCAL_PRANSWER;
    }
    case WEBRTC_SIGNALING_STATE_HAVE_REMOTE_PRANSWER: {
        return Signaling::HAVE_REMOTE_PRANSWER;
    }
    case WEBRTC_SIGNALING_STATE_CLOSED: {
        return Signaling::CLOSED;
    }
    }
    return Signaling::STABLE;
}

std::string WebRtcState::SignalingToStr(WebRtcState::Signaling state)
{
    switch (state) {
    case (WebRtcState::Signaling::STABLE): {
        return std::string("STABLE");
    }
    case (WebRtcState::Signaling::CLOSED): {
        return std::string("CLOSED");
    }
    case (WebRtcState::Signaling::HAVE_LOCAL_OFFER): {
        return std::string("HAVE_LOCAL_OFFER");
    }
    case (WebRtcState::Signaling::HAVE_REMOTE_OFFER): {
        return std::string("HAVE_REMOTE_OFFER");
    }
    case (WebRtcState::Signaling::HAVE_LOCAL_PRANSWER): {
        return std::string("HAVE_LOCAL_PRANSWER");
    }
    case (WebRtcState::Signaling::HAVE_REMOTE_PRANSWER): {
        return std::string("HAVE_REMOTE_PRANSWER");
    }
    }
    return std::string("");
}

WebRtcState::IceGathering WebRtcState::ToIceGatheringState(webrtc_ice_gathering_state_e state)
{
    switch (state) {
    case WEBRTC_ICE_GATHERING_STATE_COMPLETE: {
        return IceGathering::COMPLETE;
    }
    case WEBRTC_ICE_GATHERING_STATE_GATHERING: {
        return IceGathering::GATHERING;
    }
    case WEBRTC_ICE_GATHERING_STATE_NEW: {
        return IceGathering::NEW;
    }
    }
    return IceGathering::NEW;
}

std::string WebRtcState::IceGatheringToStr(WebRtcState::IceGathering state)
{
    switch (state) {
    case (WebRtcState::IceGathering::NEW): {
        return std::string("NEW");
    }
    case (WebRtcState::IceGathering::GATHERING): {
        return std::string("GATHERING");
    }
    case (WebRtcState::IceGathering::COMPLETE): {
        return std::string("COMPLETE");
    }
    }
    return std::string("");
}

WebRtcState::IceConnection WebRtcState::ToIceConnectionState(webrtc_ice_connection_state_e state)
{
    switch (state) {
    case WEBRTC_ICE_CONNECTION_STATE_CHECKING: {
        return IceConnection::CHECKING;
    }
    case WEBRTC_ICE_CONNECTION_STATE_CLOSED: {
        return IceConnection::CLOSED;
    }
    case WEBRTC_ICE_CONNECTION_STATE_COMPLETED: {
        return IceConnection::COMPLETED;
    }
    case WEBRTC_ICE_CONNECTION_STATE_CONNECTED: {
        return IceConnection::CONNECTED;
    }
    case WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED: {
        return IceConnection::DISCONNECTED;
    }
    case WEBRTC_ICE_CONNECTION_STATE_FAILED: {
        return IceConnection::FAILED;
    }
    case WEBRTC_ICE_CONNECTION_STATE_NEW: {
        return IceConnection::NEW;
    }
    }
    return IceConnection::NEW;
}

std::string WebRtcState::IceConnectionToStr(WebRtcState::IceConnection state)
{
    switch (state) {
    case (WebRtcState::IceConnection::NEW): {
        return std::string("NEW");
    }
    case (WebRtcState::IceConnection::CHECKING): {
        return std::string("CHECKING");
    }
    case (WebRtcState::IceConnection::CONNECTED): {
        return std::string("CONNECTED");
    }
    case (WebRtcState::IceConnection::COMPLETED): {
        return std::string("COMPLETED");
    }
    case (WebRtcState::IceConnection::FAILED): {
        return std::string("FAILED");
    }
    case (WebRtcState::IceConnection::DISCONNECTED): {
        return std::string("DISCONNECTED");
    }
    case (WebRtcState::IceConnection::CLOSED): {
        return std::string("CLOSED");
    }
    }
    return std::string("");
}

WebRtcState::SourceBufferState WebRtcState::ToSourceBufferState(webrtc_media_packet_source_buffer_state_e state)
{
    switch (state) {
    case WEBRTC_MEDIA_PACKET_SOURCE_BUFFER_STATE_UNDERFLOW: {
        return SourceBufferState::UNDERFLOW;
    }
    case WEBRTC_MEDIA_PACKET_SOURCE_BUFFER_STATE_OVERFLOW: {
        return SourceBufferState::OVERFLOW;
    }
    }
    return SourceBufferState::OVERFLOW;
}

std::string WebRtcState::SourceBufferStateToStr(WebRtcState::SourceBufferState state)
{
    switch (state) {
    case (WebRtcState::SourceBufferState::UNDERFLOW): {
        return std::string("UNDERFLOW");
    }
    case (WebRtcState::SourceBufferState::OVERFLOW): {
        return std::string("OVERFLOW");
    }
    }
    return std::string("");
}

}  // namespace AittWebRTCNamespace
