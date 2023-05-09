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
#pragma once

#include <MainLoopHandler.h>

#include <map>
#include <string>
#include <thread>
#include <vector>

#include "AITT.h"
#include "AittDiscovery.h"

namespace aitt {
class MQDiscoveryHandler {
  public:
    explicit MQDiscoveryHandler(AittDiscovery &discovery, const std::string &id);
    virtual ~MQDiscoveryHandler(void);
    void Subscribe(AittSubscribeID handle, const std::string &topic);
    void Unsubscribe(AittSubscribeID handle);
    int CountSubscriber(const std::string &topic);

  private:
    /* My Subscribe Table - Handle, Topic */
    using MySubscribeTable = std::map<AittSubscribeID, std::string>;

    /* Remote Subscribe Table - ID, Topics */
    using RemoteSubscribeTable = std::map<std::string, std::vector<std::string>>;

    void DiscoveryMessageCallback(const std::string &id, const std::string &status, const void *msg,
          const int szmsg);
    void UpdateDiscoveryMsg(void);

    AittDiscovery &discovery_;
    int discovery_cb;
    std::string id_;

    MySubscribeTable my_subscribe_table;
    std::mutex my_subscribe_table_lock;
    RemoteSubscribeTable remote_subscribe_table;
    std::mutex remote_subscribe_table_lock;
};
}  // namespace aitt
