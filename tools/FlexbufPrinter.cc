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
#include "FlexbufPrinter.h"

#include <flatbuffers/flexbuffers.h>

#include <iostream>

#include "../modules/tcp/TCP.h"
#include "aitt_internal.h"

using namespace AittTCPNamespace;

FlexbufPrinter::FlexbufPrinter() : tab(0)
{
}

void FlexbufPrinter::SetTopic(const std::string &topic)
{
    topic_ = topic;
}

void FlexbufPrinter::PrettyPrint(const std::string &name, const uint8_t *data, int datalen)
{
    auto map = flexbuffers::GetRoot(data, datalen).AsMap();

    auto keys = map.Keys();
    for (size_t idx = 0; idx < keys.size(); ++idx) {
        std::string key = keys[idx].AsString().str();

        if (STR_EQ == key.compare("status")) {
            continue;
        }

        auto ref = flexbuffers::GetRoot(map[key].AsBlob().data(), map[key].AsBlob().size());

        if (STR_EQ == key.compare("TCP") || STR_EQ == key.compare("SECURE_TCP")) {
            TCPPrettyParsing(name, ref, key);
        } else if (STR_EQ == key.compare("MQTT")) {
            MQTTPrettyParsing(name, ref);
        } else {
            std::cout << name << std::endl;
            PrettyParsing(ref, false);
        }
    }
}

std::string FlexbufPrinter::PrettyTab(bool ignore)
{
    if (ignore)
        return std::string();

    std::stringstream ss;
    for (int i = 0; i < tab; i++)
        ss << '\t';

    return ss.str();
}

void FlexbufPrinter::PrettyMap(const flexbuffers::Reference &data, bool inline_value)
{
    std::cout << PrettyTab(inline_value) << "{" << std::endl;
    tab++;

    auto map = data.AsMap();
    auto keys = map.Keys();
    for (size_t i = 0; i < keys.size(); i++) {
        std::cout << PrettyTab(false) << keys[i].AsKey() << " : ";
        PrettyParsing(map[keys[i].AsKey()], true);
    }

    tab--;
    std::cout << PrettyTab(false) << "}" << std::endl;
}

void FlexbufPrinter::PrettyVector(const flexbuffers::Reference &data, bool inline_value)
{
    auto vec = data.AsVector();
    std::cout << PrettyTab(inline_value) << "[" << std::endl;
    tab++;

    for (size_t i = 0; i < vec.size(); i++)
        PrettyParsing(vec[i], false);

    tab--;
    std::cout << PrettyTab(false) << "]" << std::endl;
}

void FlexbufPrinter::PrettyBlob(const flexbuffers::Reference &data)
{
    auto blob = data.AsBlob();

    DBG_HEX_DUMP(blob.data(), blob.size());
}

void FlexbufPrinter::PrettyParsing(const flexbuffers::Reference &data, bool inline_value)
{
    using namespace flexbuffers;
    switch (data.GetType()) {
    case FBT_NULL:
        ERR("Unknown Type : case FBT_NULL");
        break;
    case FBT_KEY:
        ERR("Unknown Type : case FBT_KEY");
        break;
    case FBT_MAP:
        PrettyMap(data, inline_value);
        break;
    case FBT_BLOB:
        PrettyBlob(data);
        break;
    case FBT_VECTOR:
        PrettyVector(data, inline_value);
        break;
    case FBT_INT:
    case FBT_INDIRECT_INT:
    case FBT_UINT:
    case FBT_INDIRECT_UINT:
    case FBT_FLOAT:
    case FBT_INDIRECT_FLOAT:
    case FBT_STRING:
    case FBT_VECTOR_INT:
    case FBT_VECTOR_UINT:
    case FBT_VECTOR_FLOAT:
    case FBT_VECTOR_KEY:
    case FBT_VECTOR_STRING_DEPRECATED:
    case FBT_VECTOR_INT2:
    case FBT_VECTOR_UINT2:
    case FBT_VECTOR_FLOAT2:
    case FBT_VECTOR_INT3:
    case FBT_VECTOR_UINT3:
    case FBT_VECTOR_FLOAT3:
    case FBT_VECTOR_INT4:
    case FBT_VECTOR_UINT4:
    case FBT_VECTOR_FLOAT4:
    case FBT_VECTOR_BOOL:
    case FBT_BOOL:
        std::cout << PrettyTab(inline_value) << data.ToString() << std::endl;
        break;
    default:
        ERR("Unknown Type(%d)", data.GetType());
        break;
    }
}

void FlexbufPrinter::TCPTopicPrint(const flexbuffers::Map &map,
      const flexbuffers::TypedVector &topics, const std::string &protocol, int idx)
{
    std::string topic = topics[idx].AsString().str();

    if (STR_EQ == topic.compare("host")) {
        return;
    }

    tab++;
    std::cout << PrettyTab(false) << "topic : " << topic << std::endl;
    auto connectInfo = map[topic].AsVector();
    size_t vec_size = connectInfo.size();
    int port = connectInfo[TCP::CONN_INFO_PORT].AsUInt16();
    int num_of_cb = connectInfo[TCP::CONN_INFO_NUM_OF_CB].AsUInt16();

    std::cout << PrettyTab(false) << "[ port : " << port;
    std::cout << ", num_of_cb : " << num_of_cb << " ]" << std::endl;

    if (STR_EQ == protocol.compare("SECURE_TCP")) {
        if (vec_size != TCP::CONN_INFO_MAX) {
            ERR("Unknown Message");
            tab--;
            return;
        }

        DBG_HEX_DUMP(connectInfo[TCP::CONN_INFO_KEY].AsBlob().data(),
              connectInfo[TCP::CONN_INFO_KEY].AsBlob().size());
        DBG_HEX_DUMP(connectInfo[TCP::CONN_INFO_IV].AsBlob().data(),
              connectInfo[TCP::CONN_INFO_IV].AsBlob().size());
    }
    tab--;
}

void FlexbufPrinter::TCPPrettyParsing(const std::string &name, const flexbuffers::Reference &data,
      const std::string &protocol)
{
    auto map = data.AsMap();
    auto topics = map.Keys();
    std::vector<int> matched_topic_idx;

    if (!topic_.empty()) {
        for (size_t idx = 0; idx < topics.size(); ++idx) {
            std::string topic = topics[idx].AsString().str();
            if (STR_EQ == topic.compare(0, topic_.size(), topic_)) {
                matched_topic_idx.push_back(idx);
            }
        }
        if (matched_topic_idx.empty()) {
            return;
        }
    }

    std::cout << name << std::endl;
    std::string host = map["host"].AsString().str();
    std::cout << PrettyTab(false) << "protocol : " << protocol << std::endl;
    std::cout << PrettyTab(false) << "[" << std::endl;
    tab++;

    std::cout << PrettyTab(false) << "host : " << host << std::endl;
    std::cout << PrettyTab(false) << "topic list : {" << std::endl;
    if (!matched_topic_idx.empty()) {
        for (const auto &idx : matched_topic_idx) {
            TCPTopicPrint(map, topics, protocol, idx);
        }
    } else {
        for (size_t idx = 0; idx < topics.size(); ++idx) {
            TCPTopicPrint(map, topics, protocol, idx);
        }
    }
    std::cout << PrettyTab(false) << "}" << std::endl;
    tab--;
    std::cout << PrettyTab(false) << "]" << std::endl << std::endl;
}

void FlexbufPrinter::MQTTTopicPrint(const flexbuffers::Vector &topics, int idx)
{
    tab++;
    std::cout << PrettyTab(false) << "topic : " << topics[idx].AsString().str() << std::endl;
    tab--;
}

void FlexbufPrinter::MQTTPrettyParsing(const std::string &name, const flexbuffers::Reference &data)
{
    auto topics = data.AsVector();
    std::vector<int> matched_topic_idx;

    if (!topic_.empty()) {
        for (size_t idx = 0; idx < topics.size(); ++idx) {
            std::string topic = topics[idx].AsString().str();
            if (STR_EQ == topic.compare(0, topic_.size(), topic_)) {
                matched_topic_idx.push_back(idx);
            }
        }
        if (matched_topic_idx.empty()) {
            return;
        }
    }

    std::cout << name << std::endl;
    std::cout << PrettyTab(false) << "protocol : MQTT" << std::endl;
    std::cout << PrettyTab(false) << "[" << std::endl;
    tab++;
    std::cout << PrettyTab(false) << "topic list : {" << std::endl;

    if (!matched_topic_idx.empty()) {
        for (const auto &idx : matched_topic_idx) {
            MQTTTopicPrint(topics, idx);
        }
    } else {
        for (size_t idx = 0; idx < topics.size(); ++idx) {
            MQTTTopicPrint(topics, idx);
        }
    }
    std::cout << PrettyTab(false) << "}" << std::endl;
    tab--;
    std::cout << PrettyTab(false) << "]" << std::endl << std::endl;
}
