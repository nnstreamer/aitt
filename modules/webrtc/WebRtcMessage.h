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
        DiscoveryInfo(const std::string &topic, bool is_src, const std::string &sdp,
              const std::vector<std::string> &ice_candidates)
              : topic_(topic), is_src_(is_src), sdp_(sdp), ice_candidates_(ice_candidates)
        {
        }
        std::string topic_;
        bool is_src_;
        std::string sdp_;
        std::vector<std::string> ice_candidates_;
    };
    static WebRtcMessage::Type getMessageType(const std::string &message);
    static std::vector<uint8_t> GenerateDiscoveryMessage(const std::string &topic, bool is_src,
          const std::string &sdp, const std::vector<std::string> &ice_candidates);
    static bool IsValidDiscoveryMessage(const std::vector<uint8_t> &discovery_message);
    static DiscoveryInfo ParseDiscoveryMessage(const std::vector<uint8_t> &discovery_message);
    static constexpr int DISCOVERY_MESSAGE_KEY_SIZE = 4;
};

}  // namespace AittWebRTCNamespace
