/*
 * Copyright (c) 2021-2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <AittTypes.h>

#include <string>
namespace aitt {

class StreamTag {
  public:
    explicit StreamTag(const std::string &topic, AittProtocol protocol, AittStreamRole role,
          void *handle)
          : topic_(topic), protocol_(protocol), role_(role), handle_(handle){};
    std::string topic_;
    AittProtocol protocol_;
    AittStreamRole role_;
    void *handle_;
};

}  // namespace aitt
