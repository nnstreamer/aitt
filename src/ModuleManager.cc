/*
 * Copyright 2021-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

    for (int i = AITT_STREAM_TYPE_WEBRTC; i < AITT_STREAM_TYPE_MAX; ++i) {
        stream_handles.push_back(ModuleHandle(nullptr, nullptr));
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

    case AITT_TYPE_MQTT:
    default:
        ERR("Unknown Transport Type(%d)", type);
        throw AittException(AittException::NO_DATA_ERR);
    }
    return TYPE_TRANSPORT_MAX;
}

const char *ModuleManager::GetTransportFileName(TransportType type)
{
    switch (type) {
    case TYPE_TCP:
    case TYPE_TCP_SECURE:
        return "libaitt-transport-tcp.so";
    default:
        ERR("Unknown Type(%d)", type);
        break;
    }

    return "Unknown";
}

const char *ModuleManager::GetStreamFileName(AittStreamProtocol type)
{
    switch (type) {
    case AITT_STREAM_TYPE_WEBRTC:
        return "libaitt-stream-webrtc.so";
    case AITT_STREAM_TYPE_RTSP:
        return "libaitt-stream-rtsp.so";
    default:
        ERR("Unknown Type(%d)", type);
        break;
    }

    return "Unknown";
}

ModuleManager::ModuleHandle ModuleManager::OpenModule(const char *file)
{
    ModuleHandle handle(dlopen(file, RTLD_LAZY | RTLD_LOCAL), [](void *handle) {
        if (dlclose(handle))
            ERR("dlclose: %s", dlerror());
    });
    if (handle == nullptr) {
        ERR("dlopen(%s): %s", file, dlerror());
        throw AittException(AittException::SYSTEM_ERR);
    }

    return handle;
}

ModuleManager::ModuleHandle ModuleManager::OpenTransport(TransportType type)
{
    if (TYPE_TCP_SECURE == type)
        type = TYPE_TCP;

    ModuleHandle handle = OpenModule(GetTransportFileName(type));

    return handle;
}

void ModuleManager::LoadTransport(TransportType type)
{
    try {
        transport_handles[type] = OpenTransport(type);
    } catch (AittException &e) {
        ERR("OpenTransport(%d) Fail(%s)", type, e.what());
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

AittStreamModule *ModuleManager::NewStreamModule(AittStreamProtocol type, const std::string &topic,
      AittStreamRole role)
{
    if (nullptr == stream_handles[type])
        stream_handles[type] = OpenModule(GetStreamFileName(type));

    AittStreamModule::ModuleEntry get_instance_fn = reinterpret_cast<AittStreamModule::ModuleEntry>(
          dlsym(stream_handles[type].get(), AittStreamModule::MODULE_ENTRY_NAME));
    if (get_instance_fn == nullptr) {
        ERR("dlsym: %s", dlerror());
        throw AittException(AittException::SYSTEM_ERR);
    }

    AittStreamModule *instance(
          static_cast<AittStreamModule *>(get_instance_fn(discovery, topic, role)));
    if (instance == nullptr) {
        ERR("get_instance_fn(AittStreamModule) Fail");
        throw AittException(AittException::SYSTEM_ERR);
    }

    return instance;
}

std::unique_ptr<MQ> ModuleManager::NewCustomMQ(const std::string &id, const AittOption &option)
{
    if (nullptr == custom_mqtt_handle)
        custom_mqtt_handle = OpenModule("libaitt-st-broker.so");

    MQ::ModuleEntry get_instance_fn =
          reinterpret_cast<MQ::ModuleEntry>(dlsym(custom_mqtt_handle.get(), MQ::MODULE_ENTRY_NAME));
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
