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

#include <json-glib/json-glib.h>

#include "aitt_internal.h"

#include "WebRtcMessage.h"

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
