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

#include "AITTEx.h"
#include "aitt_internal.h"

namespace aitt {

TransportModuleLoader::TransportModuleLoader(const std::string &ip) : ip(ip)
{
}

std::string TransportModuleLoader::GetModuleFilename(AittProtocol protocol)
{
    // TODO:
    // We are able to generate the module name by a particular syntax,
    // It could be introduced later when we have several modules.
    if (protocol == AITT_TYPE_TCP)
        return "libaitt-transport-tcp.so";
    if (protocol == AITT_TYPE_WEBRTC)
        return "libaitt-transport-webrtc.so";

    return std::string();
}

int TransportModuleLoader::LoadModule(AittProtocol protocol, AittDiscovery &discovery)
{
    std::string filename = GetModuleFilename(protocol);

    Handler handle(dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL),
          [](const void *handle) -> void {
              if (dlclose(const_cast<void *>(handle)))
                  ERR("dlclose: %s", dlerror());
          });
    if (handle == nullptr) {
        ERR("dlopen: %s", dlerror());
        return -1;
    }

    AittTransport::ModuleEntry get_instance_fn = reinterpret_cast<AittTransport::ModuleEntry>(
          dlsym(handle.get(), AittTransport::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        return -1;
    }

    std::shared_ptr<AittTransport> instance(
          static_cast<AittTransport *>(get_instance_fn(ip.c_str(), discovery)),
          [](const AittTransport *instance) -> void { delete instance; });
    if (instance == nullptr) {
        ERR("Failed to create a new instance");
        return -1;
    }

    module_table.emplace(protocol, std::make_pair(std::move(handle), instance));

    return 0;
}

void TransportModuleLoader::Init(AittDiscovery &discovery)
{
    std::lock_guard<std::mutex> lock_from_here(module_lock);
    if (LoadModule(AITT_TYPE_TCP, discovery) < 0) {
        ERR("LoadModule(AITT_TYPE_TCP) Fail");
    }

#ifdef WITH_WEBRTC
    if (LoadModule(AITT_TYPE_WEBRTC, discovery) < 0) {
        ERR("LoadModule(AITT_TYPE_WEBRTC) Fail");
    }
#endif  // WITH_WEBRTC
}

std::shared_ptr<AittTransport> TransportModuleLoader::GetInstance(AittProtocol protocol)
{
    std::lock_guard<std::mutex> lock_from_here(module_lock);

    auto item = module_table.find(protocol);
    if (item == module_table.end()) {
        ERR("Not Initialized");
        // throw AITTEx(AITTEx::NO_DATA, "Not Initialized");
        return nullptr;
    }

    return item->second.second;
}

}  // namespace aitt
