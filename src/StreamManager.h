/*
 * Copyright 2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <vector>

#include "AittStreamModule.h"
#include "ModuleManager.h"

namespace aitt {

class StreamManager {
  public:
#ifndef TIZEN_RT
    explicit StreamManager(ModuleManager &module_man) : module_manager(module_man) {}
#else
    explicit StreamManager(ModuleManager &module_man) {}
#endif

    virtual ~StreamManager() = default;

    AittStream *CreateStream(AittStreamProtocol type, const std::string &topic,
          AittStreamRole role);
    void DestroyStream(AittStream *aitt_stream);
    void DestroyStreamAll(void);

#ifndef TIZEN_RT
  private:
    std::vector<AittStreamModule *> streams_in_use;
    ModuleManager &module_manager;
#endif
};

}  // namespace aitt
