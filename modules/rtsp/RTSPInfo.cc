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

RTSPInfo::RTSPInfo(const std::string &_url, const std::string &_id, const std::string &_password)
      : url(_url), id(_id), password(_password)
{
    // encoding secure id and password
}

RTSPInfo::~RTSPInfo()
{
}

std::string RTSPInfo::GetUrl()
{
    return url;
}

std::string RTSPInfo::GetID()
{
    // decoding secure id
    return id;
}

std::string RTSPInfo::GetPassword()
{
    // decoding secure password
    return password;
}
