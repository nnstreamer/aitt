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

#include "TransportModuleLoader.h"

namespace aitt {

class TransportModuleLoader {
  public:
    explicit TransportModuleLoader(const std::string &ip);
    virtual ~TransportModuleLoader() = default;

    void Init(AittDiscovery &discovery);
    std::shared_ptr<AittTransport> GetInstance(AittProtocol protocol);

  private:
    using Handler = std::unique_ptr<void, void (*)(const void *)>;
    using ModuleMap = std::map<AittProtocol, std::pair<Handler, std::shared_ptr<AittTransport>>>;

    std::string GetModuleFilename(AittProtocol protocol);
    int LoadModule(AittProtocol protocol, AittDiscovery &discovery);

    ModuleMap module_table;
    std::mutex module_lock;
    std::string ip;
};

}  // namespace aitt
