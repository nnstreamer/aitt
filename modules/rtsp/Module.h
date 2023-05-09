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

#include <AittStreamModule.h>

#include <map>
#include <string>

#include "RTSPClient.h"
#include "RTSPInfo.h"

using AittDiscovery = aitt::AittDiscovery;
using AittStreamModule = aitt::AittStreamModule;
using AittStream = aitt::AittStream;

#define MODULE_NAMESPACE AittRTSPNamespace
namespace AittRTSPNamespace {
class Module : public AittStreamModule {
  public:
    explicit Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role);
    virtual ~Module(void);

    AittStream *SetConfig(const std::string &key, const std::string &value) override;
    AittStream *SetConfig(const std::string &key, void *obj) override;
    std::string GetFormat(void) override;
    int GetWidth(void) override;
    int GetHeight(void) override;
    void Start(void) override;
    void Stop(void) override;
    int Push(void *obj) override;
    void SetStateCallback(StateCallback cb, void *user_data) override;
    void SetReceiveCallback(ReceiveCallback cb, void *user_data) override;

  private:
    void UpdateState(AittStreamRole role, AittStreamState state);
    void UpdateDiscoveryMsg();
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg);

    AittDiscovery &discovery_;
    std::string topic_;
    AittStreamRole role_;

    int discovery_cb_;
    RTSPInfo info;
    RTSPClient client;

    AittStreamState server_state;
    AittStreamState client_state;

    std::pair<StateCallback, void *> state_cb;
    std::pair<ReceiveCallback, void *> receive_cb;
};
}  // namespace AittRTSPNamespace
