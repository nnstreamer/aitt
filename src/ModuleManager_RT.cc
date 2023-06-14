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
#ifdef TIZEN_RT

#include "ModuleManager_RT.h"

#include "../modules/tcp/Module.h"
#include "AittException.h"
#include "CustomMQ.h"
#include "NullTransport.h"
#include "aitt_internal.h"

namespace aitt {

ModuleManager::ModuleManager(const std::string &my_ip, AittDiscovery &d)
      : ip(my_ip), discovery(d), null_transport(discovery, ip)
{
    transports[TYPE_TCP] = std::unique_ptr<AittTransport>(static_cast<AittTransport *>(
          new AittTCPNamespace::Module(AITT_TYPE_TCP, discovery, my_ip)));
    transports[TYPE_TCP_SECURE] = std::unique_ptr<AittTransport>(static_cast<AittTransport *>(
          new AittTCPNamespace::Module(AITT_TYPE_TCP_SECURE, discovery, my_ip)));
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

std::unique_ptr<MQ> ModuleManager::NewCustomMQ(const std::string &id, const AittOption &option)
{
    std::unique_ptr<MQ> instance(new CustomMQ(id, option));
    if (instance == nullptr) {
        ERR("STBrokerMQ() Fail");
        throw AittException(AittException::SYSTEM_ERR);
    }

    return instance;
}

AittStream *ModuleManager::CreateStream(AittStreamProtocol type, const std::string &topic,
      AittStreamRole role)
{
    throw AittException(AittException::NOT_SUPPORTED);
}
void ModuleManager::DestroyStream(AittStream *aitt_stream)
{
}
void ModuleManager::DestroyStreamAll(void)
{
}

}  // namespace aitt

#endif  // TIZEN_RT
