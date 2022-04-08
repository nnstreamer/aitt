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

#include "Module.h"

extern "C" {

// NOTE:
// name of this function is defined in aitt::TransportModule::MODULE_ENTRY_NAME
API void *aitt_module_entry(const char *ip)
{
    // NOTE:
    // The assert() is used to break the build when the MODULE_ENTRY_NAME is changed.
    // This is only for the debugging purpose
    assert(!strcmp(__func__, aitt::TransportModule::MODULE_ENTRY_NAME)
           && "Entry point name is not matched");

    std::string ip_address(ip);
    Module *module = new Module(ip_address);

    TransportModule *tModule = dynamic_cast<TransportModule *>(module);
    // NOTE:
    // validate that the module creates valid object (which inherits TransportModule)
    assert(tModule && "Transport Module is not created");

    return tModule;
}

}  // extern "C"
