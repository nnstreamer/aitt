/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#pragma once

#include <AittTypes.h>

#include <string>

class API AittOption {
  public:
    AittOption();
    explicit AittOption(bool clean_session, bool use_custom_broker);
    ~AittOption() = default;

    void SetCleanSession(bool val);
    bool GetCleanSession() const;
    void SetUseCustomMqttBroker(bool val);
    bool GetUseCustomMqttBroker() const;
    void SetServiceID(const std::string &id);
    const char *GetServiceID() const;
    void SetLocationID(const std::string &id);
    const char *GetLocationID() const;
    void SetRootCA(const std::string &ca);
    const char *GetRootCA() const;
    void SetCustomRWFile(const std::string &file);
    const char *GetCustomRWFile() const;

  private:
    bool clean_session_;
    bool use_custom_broker;
    std::string service_id;
    std::string location_id;
    std::string root_ca;
    std::string custom_rw_file;
};
