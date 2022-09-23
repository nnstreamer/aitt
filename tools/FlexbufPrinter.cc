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
#include "FlexbufPrinter.h"

#include <flatbuffers/flexbuffers.h>

#include <iostream>

#include "aitt_internal.h"

FlexbufPrinter::FlexbufPrinter() : tab(0)
{
}

void FlexbufPrinter::PrettyPrint(const std::string &name, const uint8_t *data, int datalen)
{
    std::cout << name << std::endl;

    auto root = flexbuffers::GetRoot(data, datalen);
    PrettyParsing(root, false);
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

void FlexbufPrinter::PrettyBlob(const flexbuffers::Reference &data, bool inline_value)
{
    auto blob = data.AsBlob();
    DBG_HEX_DUMP(blob.data(), blob.size());
    // auto root = flexbuffers::GetRoot(static_cast<const uint8_t *>(blob.data()), blob.size());

    // PrettyParsing(root, true);
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
        PrettyBlob(data, inline_value);
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
