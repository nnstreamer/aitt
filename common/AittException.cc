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
#include "AittException.h"

using namespace aitt;

AittException::AittException(ErrCode code) : err_code(code)
{
    err_msg = getErrString();
}

AittException::AittException(ErrCode code, const std::string& msg) : err_code(code)
{
    err_msg = getErrString() + " : " + msg;
}

AittException::ErrCode AittException::getErrCode()
{
    return err_code;
}

std::string AittException::getErrString() const
{
    switch (err_code) {
    case INVALID_ARG:
        return "Invalid Argument";
    case NO_MEMORY_ERR:
        return "Memory allocation failure";
    case OPERATION_FAILED:
        return "Operation failure";
    case SYSTEM_ERR:
        return "System failure";
    case MQTT_ERR:
        return "MQTT failure";
    case NO_DATA_ERR:
        return "No data found";
    case RESOURCE_BUSY_ERR:
        return "Resource busy";
    default:
        return "Unknown Error";
    }
}

const char* AittException::what() const throw()
{
    return err_msg.c_str();
}
