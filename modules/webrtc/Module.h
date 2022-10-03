/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <AittStreamModule.h>

using AittDiscovery = aitt::AittDiscovery;
using AittStreamModule = aitt::AittStreamModule;

#define MODULE_NAMESPACE AittWebRTCNamespace
namespace AittWebRTCNamespace {

class Module : public AittStreamModule {
  public:
    explicit Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role);
    virtual ~Module(void);

    void SetConfig(const std::string &key, const std::string &value) override;
    void SetConfig(const std::string &key, void *obj) override;
    void Start(void) override;

    void SetStateCallback(StateCallback cb, void *user_data) override;
    void SetReceiveCallback(ReceiveCallback cb, void *user_data) override;

  private:
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg);
      AittDiscovery &discovery_;
    int discovery_cb_;
    std::string topic_;
    AittStreamRole role_;
};
}  // namespace AittWebRTCNamespace
