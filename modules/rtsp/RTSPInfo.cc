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

#include "RTSPInfo.h"

#include "aitt_internal.h"

RTSPInfo::RTSPInfo() : uri_(""), id_(""), password_("")
{
}

RTSPInfo::~RTSPInfo()
{
}

void RTSPInfo::SetURI(const std::string &uri)
{
    uri_ = uri;
}

std::string RTSPInfo::GetURI()
{
    return uri_;
}

void RTSPInfo::SetID(const std::string &id)
{
    id_ = id;
}

std::string RTSPInfo::GetID()
{
    return id_;
}

void RTSPInfo::SetPassword(const std::string &password)
{
    password_ = password;
}

std::string RTSPInfo::GetPassword()
{
    return password_;
}

std::string RTSPInfo::GetCompleteURI()
{
    std::string complete_uri = uri_;

    if (id_.empty() != true && password_.empty() != true) {
        complete_uri.insert(7, id_ + ":" + password_ + "@");
    }

    return complete_uri;
}
