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

#include "TransportModuleLoader.h"

#include <dlfcn.h>

#include "log.h"

namespace aitt {

TransportModuleLoader::TransportModuleLoader(const std::string &ip) : ip(ip)
{
}

TransportModuleLoader::~TransportModuleLoader(void)
{
}

std::string TransportModuleLoader::GetModuleFilename(AittProtocol protocol)
{
    // TODO:
    // We are able to generate the module name by a particular syntax,
    // It could be introduced later when we have several modules.
    if (protocol == AITT_TYPE_TCP)
        return "libaitt-transport-tcp.so";

    return std::string();
}

std::shared_ptr<TransportModule> TransportModuleLoader::GetInstance(AittProtocol protocol)
{
    auto item = module_table.find(protocol);
    if (item != module_table.end())
        return item->second.second;

    std::string filename = GetModuleFilename(protocol);

    // NOTE:
    // Prevent from reference symbols of the loaded module from the subsequently loaded other
    // modules
    Handler handle(dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL),
          [](const void *handle) -> void {
              if (dlclose(const_cast<void *>(handle)))
                  ERR("dlclose: %s", dlerror());
          });
    if (handle == nullptr) {
        ERR("dlopen: %s", dlerror());
        return nullptr;
    }

    TransportModule::ModuleEntry get_instance_fn = reinterpret_cast<TransportModule::ModuleEntry>(
          dlsym(handle.get(), TransportModule::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        return nullptr;
    }

    std::shared_ptr<TransportModule> instance(
          static_cast<TransportModule *>(get_instance_fn(ip.c_str())),
          [](const TransportModule *instance) -> void { delete instance; });
    if (instance == nullptr) {
        ERR("Failed to create a new instance");
        return nullptr;
    }

    // NOTE:
    // Keep the created module instance to manage it as a singleton
    module_table.insert(
          ModuleMap::value_type(protocol, std::make_pair(std::move(handle), instance)));
    return instance;
}

}  // namespace aitt
