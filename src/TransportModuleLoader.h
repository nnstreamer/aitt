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
#include <TransportModule.h>

#include <map>
#include <memory>
#include <string>

#include "TransportModuleLoader.h"

namespace aitt {

class TransportModuleLoader {
  public:
    // TODO:
    // The current version of TransportModuleLoader does not consider how to configure the module in
    // a common way. Later, the module loader should provide a way to configure a module if a new
    // type of module is added which requires additional information. Now, we only provide a
    // device's "ip" address information for configurating a module.
    explicit TransportModuleLoader(const std::string &ip);
    virtual ~TransportModuleLoader(void);

    std::shared_ptr<TransportModule> GetInstance(AittProtocol protocol);

  private:
    std::string GetModuleFilename(AittProtocol protocol);

  private:
    using Handler = std::unique_ptr<void, void (*)(const void *)>;
    using ModuleMap = std::map<AittProtocol, std::pair<Handler, std::shared_ptr<TransportModule>>>;
    ModuleMap module_table;
    std::string ip;
};

}  // namespace aitt
