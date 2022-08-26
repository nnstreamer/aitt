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

#include "ModuleLoader.h"

#include <dlfcn.h>

#include "AittException.h"
#include "MQ.h"
#include "NullTransport.h"
#include "aitt_internal.h"

namespace aitt {

std::string ModuleLoader::GetModuleFilename(Type type)
{
    if (type == TYPE_TCP)
        return "libaitt-transport-tcp.so";
    if (type == TYPE_WEBRTC)
        return "libaitt-transport-webrtc.so";
    if (type == TYPE_CUSTOM_MQTT)
        return "libaitt-st-broker.so";

    return std::string("Unknown");
}

ModuleLoader::ModuleHandle ModuleLoader::OpenModule(Type type)
{
    std::string filename = GetModuleFilename(type);

    ModuleHandle handle(dlopen(filename.c_str(), RTLD_LAZY | RTLD_LOCAL),
          [](const void *handle) -> void {
              if (dlclose(const_cast<void *>(handle)))
                  ERR("dlclose: %s", dlerror());
          });
    if (handle == nullptr)
        ERR("dlopen: %s", dlerror());

    return handle;
}

std::unique_ptr<AittTransport> ModuleLoader::LoadTransport(void *handle, const std::string &ip,
      AittDiscovery &discovery)
{
    if (handle == nullptr) {
        ERR("handle is NULL");
        return std::unique_ptr<AittTransport>(new NullTransport(ip.c_str(), discovery));
    }

    AittTransport::ModuleEntry get_instance_fn = reinterpret_cast<AittTransport::ModuleEntry>(
          dlsym(handle, AittTransport::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        return std::unique_ptr<AittTransport>(new NullTransport(ip.c_str(), discovery));
    }

    std::unique_ptr<AittTransport> instance(
          static_cast<AittTransport *>(get_instance_fn(ip.c_str(), discovery)));
    if (instance == nullptr) {
        ERR("get_instance_fn(AittTransport) Fail");
        return std::unique_ptr<AittTransport>(new NullTransport(ip.c_str(), discovery));
    }

    return instance;
}

std::unique_ptr<MQ> ModuleLoader::LoadMqttClient(void *handle, const std::string &id,
      const AittOption &option)
{
    MQ::ModuleEntry get_instance_fn =
          reinterpret_cast<MQ::ModuleEntry>(dlsym(handle, MQ::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        throw AittException(AittException::SYSTEM_ERR);
    }

    std::unique_ptr<MQ> instance(static_cast<MQ *>(get_instance_fn(id.c_str(), option)));
    if (instance == nullptr) {
        ERR("get_instance_fn(MQ) Fail");
        throw AittException(AittException::SYSTEM_ERR);
    }

    return instance;
}

}  // namespace aitt
