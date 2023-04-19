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

#include <AittDiscovery.h>
#include <AittStreamModule.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "StreamManager.h"

using AittDiscovery = aitt::AittDiscovery;
using AittStreamModule = aitt::AittStreamModule;
using AittStream = aitt::AittStream;

#define MODULE_NAMESPACE AittWebRTCNamespace
namespace AittWebRTCNamespace {

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
    // TODO: Update Ice Candidates when stream add candidates.
    void OnIceCandidateAdded(void);
    void OnStreamStarted(void);
    void OnStreamStopped(void);
    void OnStreamState(const std::string &state);
    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg);

    bool is_source_;
    AittDiscovery &discovery_;
    int discovery_cb_;

    // TODO: What if user copies the module?
    // Think about that case with destructor
    StreamManager *stream_manager_;
    StateCallback state_callback_;
    void *state_cb_user_data_;
    ReceiveCallback receive_callback_;
    void *receive_cb_user_data_;
};
}  // namespace AittWebRTCNamespace
