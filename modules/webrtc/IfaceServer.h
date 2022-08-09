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

#include <functional>
#include <vector>

class IfaceServer {
  public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Registering,
        Registered,
    };

    virtual ~IfaceServer(){};
    virtual void SetConnectionStateChangedCb(
          std::function<void(ConnectionState)> connection_state_changed_cb) = 0;
    virtual void UnsetConnectionStateChangedCb(void) = 0;
    virtual int Connect(void) = 0;
    virtual int Disconnect(void) = 0;
    virtual bool IsConnected(void) = 0;
    virtual int SendMessage(const std::string &peer_id, const std::string &message) = 0;
};
