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
#include "AITTEx.h"

using namespace aitt;

AITTEx::AITTEx(ErrCode code) : err_code(code)
{
    err_msg = getErrString();
}

AITTEx::AITTEx(ErrCode code, const std::string& msg) : err_code(code)
{
    err_msg = getErrString() + " : " + msg;
}

AITTEx::ErrCode AITTEx::getErrCode()
{
    return err_code;
}

std::string AITTEx::getErrString() const
{
    switch (err_code) {
    case INVALID_ARG:
        return "Invalid Argument";
    case NO_MEMORY:
        return "Memory allocation failure";
    case OPERATION_FAILED:
        return "Operation failure";
    case SYSTEM_ERR:
        return "System failure";
    case MQTT_ERR:
        return "MQTT failure";
    case NO_DATA:
        return "No data found";
    default:
        return "Unknown Error";
    }
}

const char* AITTEx::what() const throw()
{
    return err_msg.c_str();
}

