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

#include "WebRtcMessage.h"

#include <flatbuffers/flexbuffers.h>
#include <json-glib/json-glib.h>

#include "aitt_internal.h"

WebRtcMessage::Type WebRtcMessage::getMessageType(const std::string &message)
{
    WebRtcMessage::Type type = WebRtcMessage::Type::UNKNOWN;
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, message.c_str(), -1, NULL)) {
        DBG("Unknown message '%s', ignoring", message.c_str());
        g_object_unref(parser);
        return type;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        DBG("Unknown json message '%s', ignoring", message.c_str());
        g_object_unref(parser);
        return type;
    }

    JsonObject *object = json_node_get_object(root);
    /* Check type of JSON message */

    if (json_object_has_member(object, "sdp")) {
        type = WebRtcMessage::Type::SDP;
    } else if (json_object_has_member(object, "ice")) {
        type = WebRtcMessage::Type::ICE;
    } else {
        DBG("%s:UNKNOWN", __func__);
    }

    g_object_unref(parser);
    return type;
}

std::vector<uint8_t> WebRtcMessage::GenerateDiscoveryMessage(const std::string &topic, bool is_src,
      const std::string &sdp, const std::vector<std::string> &ice_candidates)
{
    std::vector<uint8_t> message;

    flexbuffers::Builder fbb;
    fbb.Map([=, &fbb]() {
        fbb.String("topic", topic);
        fbb.Bool("is_src", is_src);
        fbb.String("sdp", sdp);
        fbb.Vector("ice_candidates", [&]() {
            for (const auto &candidate : ice_candidates) {
                fbb.String(candidate);
            }
        });
    });
    fbb.Finish();

    message = fbb.GetBuffer();

    return message;
}

bool WebRtcMessage::IsValidDiscoveryMessage(const std::vector<uint8_t> &discovery_message)
{
    if (!flexbuffers::GetRoot(discovery_message).IsMap())
        return false;

    auto discovery_info = flexbuffers::GetRoot(discovery_message).AsMap();
    bool topic_exists{discovery_info["topic"].IsString()};
    bool is_source_exists{discovery_info["is_src"].IsBool()};
    bool sdp_exists{discovery_info["sdp"].IsString()};
    bool ice_candidates_exists{discovery_info["ice_candidates"].IsVector()};

    return topic_exists && is_source_exists && sdp_exists && ice_candidates_exists;
}

WebRtcMessage::DiscoveryInfo WebRtcMessage::ParseDiscoveryMessage(const std::vector<uint8_t> &discovery_message)
{
    WebRtcMessage::DiscoveryInfo info;
    if (!IsValidDiscoveryMessage(discovery_message))
        return info;

    auto discovery_info_map = flexbuffers::GetRoot(discovery_message).AsMap();
    info.topic = discovery_info_map["topic"].AsString().str();
    info.is_src = discovery_info_map["is_src"].AsBool();
    info.sdp = discovery_info_map["sdp"].AsString().str();
    auto ice_candidates_info = discovery_info_map["ice_candidates"].AsVector();
    for (size_t idx = 0; idx < ice_candidates_info.size(); ++idx) {
        info.ice_candidates.push_back(ice_candidates_info[idx].AsString().str());
    }

    return info;
}
