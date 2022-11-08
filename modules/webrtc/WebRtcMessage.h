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

#include <flatbuffers/flexbuffers.h>

#include <string>
#include <vector>

namespace AittWebRTCNamespace {

class WebRtcMessage {
  public:
    enum class Type {
        SDP,
        ICE,
        UNKNOWN,
    };
    class DiscoveryInfo {
      public:
        DiscoveryInfo() = default;
        DiscoveryInfo(const std::string &id, const std::string &peer_id, const std::string &sdp,
              const std::vector<std::string> &ice_candidates)
              : id_(id), peer_id_(peer_id), sdp_(sdp), ice_candidates_(ice_candidates)
        {
        }
        std::string id_;
        std::string peer_id_;
        std::string sdp_;
        std::vector<std::string> ice_candidates_;
    };
    static WebRtcMessage::Type getMessageType(const std::string &message);
    static bool IsValidStreamInfo(const std::vector<uint8_t> &message);
    static bool IsValidStream(const flexbuffers::Map &info);
    static constexpr int DISCOVERY_MESSAGE_KEY_SIZE = 4;
};

}  // namespace AittWebRTCNamespace
