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
#include "ModuleManager.h"

#include <dlfcn.h>

#include "AittException.h"
#include "NullTransport.h"
#include "aitt_internal.h"

namespace aitt {

ModuleManager::ModuleManager(const std::string &my_ip, AittDiscovery &d)
      : ip(my_ip), discovery(d), custom_mqtt_handle(nullptr, nullptr), null_transport(discovery, ip)
{
    for (int i = TYPE_TCP; i < TYPE_TRANSPORT_MAX; ++i) {
        transport_handles.push_back(ModuleHandle(nullptr, nullptr));
        LoadTransport(static_cast<TransportType>(i));
    }
}

AittTransport &ModuleManager::Get(AittProtocol protocol)
{
    TransportType type = Convert(protocol);
    AittTransport *module = transports[type].get();
    if (nullptr == module)
        module = &null_transport;

    return *module;
}

ModuleManager::TransportType ModuleManager::Convert(AittProtocol type)
{
    switch (type) {
    case AITT_TYPE_TCP:
        return TYPE_TCP;
    case AITT_TYPE_TCP_SECURE:
        return TYPE_TCP_SECURE;
    case AITT_TYPE_WEBRTC:
        return TYPE_WEBRTC;

    case AITT_TYPE_MQTT:
    default:
        ERR("Unknown Transport Type(%d)", type);
        throw AittException(AittException::NO_DATA_ERR);
    }
    return TYPE_TRANSPORT_MAX;
}

std::string ModuleManager::GetTransportFileName(TransportType type)
{
    switch (type) {
    case TYPE_TCP:
    case TYPE_TCP_SECURE:
        return "libaitt-transport-tcp.so";
    case TYPE_WEBRTC:
        return "libaitt-transport-webrtc.so";
    default:
        ERR("Unknown Type(%d)", type);
        break;
    }

    return std::string("Unknown");
}

ModuleManager::ModuleHandle ModuleManager::OpenModule(const char *file)
{
    ModuleHandle handle(dlopen(file, RTLD_LAZY | RTLD_LOCAL), [](const void *handle) -> void {
        if (dlclose(const_cast<void *>(handle)))
            ERR("dlclose: %s", dlerror());
    });
    if (handle == nullptr)
        ERR("dlopen(%s): %s", file, dlerror());

    return handle;
}

ModuleManager::ModuleHandle ModuleManager::OpenTransport(TransportType type)
{
    if (TYPE_TCP_SECURE == type)
        type = TYPE_TCP;

    std::string filename = GetTransportFileName(type);
    ModuleHandle handle = OpenModule(filename.c_str());

    return handle;
}

void ModuleManager::LoadTransport(TransportType type)
{
    transport_handles[type] = OpenTransport(type);
    if (transport_handles[type] == nullptr) {
        ERR("OpenTransport(%d) Fail", type);
        return;
    }

    AittTransport::ModuleEntry get_instance_fn = reinterpret_cast<AittTransport::ModuleEntry>(
          dlsym(transport_handles[type].get(), AittTransport::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        return;
    }

    AittProtocol protocol = static_cast<AittProtocol>(0x1 << (type + 1));
    transports[type] = std::unique_ptr<AittTransport>(
          static_cast<AittTransport *>(get_instance_fn(protocol, discovery, ip.c_str())));
    if (transports[type] == nullptr) {
        ERR("get_instance_fn(%d) Fail", protocol);
    }
}

std::unique_ptr<MQ> ModuleManager::NewCustomMQ(const std::string &id, const AittOption &option)
{
    ModuleHandle handle = OpenModule("libaitt-st-broker.so");

    MQ::ModuleEntry get_instance_fn =
          reinterpret_cast<MQ::ModuleEntry>(dlsym(handle.get(), MQ::MODULE_ENTRY_NAME));
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
