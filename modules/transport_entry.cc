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
#include <assert.h>

#include <string>

#include "Module.h"
#include "aitt_internal_definitions.h"

extern "C" {

API void *AITT_TRANSPORT_NEW(const char *ip, AittDiscovery &discovery)
{
    assert(STR_EQ == strcmp(__func__, aitt::AittTransport::MODULE_ENTRY_NAME)
           && "Entry point name is not matched");

    std::string ip_address(ip);
    Module *module = new Module(ip_address, discovery);

    // validate that the module creates valid object (which inherits AittTransport)
    AittTransport *transport_module = dynamic_cast<AittTransport *>(module);
    assert(transport_module && "Transport Module is not created");

    return transport_module;
}

}  // extern "C"
