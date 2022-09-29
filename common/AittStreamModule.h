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

#include <AittDiscovery.h>
#include <AittStream.h>
#include <AittTypes.h>

#include <functional>
#include <string>

#define AITT_STREAM_NEW aitt_stream_new
#define TO_STR(s) #s
#define DEFINE_TO_STR(x) TO_STR(x)

namespace aitt {

class AittStreamModule : public AittStream {
  public:
    typedef void *(
          *ModuleEntry)(AittDiscovery &discovery, const std::string &topic, AittStreamRole role);

    static constexpr const char *const MODULE_ENTRY_NAME = DEFINE_TO_STR(AITT_STREAM_NEW);

    AittStreamModule() = default;
    virtual ~AittStreamModule(void) = default;
};

}  // namespace aitt

#undef TO_STR
#undef DEFINE_TO_STR
