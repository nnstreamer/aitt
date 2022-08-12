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

#include <AITT.h>
#include <AittTransport.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "ModuleLoader.h"

namespace aitt {

class ModuleLoader {
  public:
    enum Type {
        TYPE_MQTT = 0,
        TYPE_TCP,
        TYPE_WEBRTC,
        TYPE_RTSP,
        TYPE_MAX,
    };

    using ModuleHandle = std::unique_ptr<void, void (*)(const void *)>;

    explicit ModuleLoader(const std::string &ip);
    virtual ~ModuleLoader() = default;

    ModuleHandle OpenModule(Type type);
    std::shared_ptr<AittTransport> LoadTransport(void *handle, AittDiscovery &discovery);

  private:
    std::string GetModuleFilename(Type type);

    std::string ip;
};

}  // namespace aitt
