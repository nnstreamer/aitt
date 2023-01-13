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

#include "MQ.h"

namespace aitt {

class AittDiscovery {
  public:
    static constexpr const char *WILL_LEAVE_NETWORK = "disconnected";
    static constexpr const char *JOIN_NETWORK = "connected";

    using DiscoveryCallback = std::function<void(const std::string &clientId,
          const std::string &status, const void *msg, const int szmsg)>;

    // AittDiscovery() = default;
    explicit AittDiscovery(const std::string &id);
    void SetMQ(std::unique_ptr<MQ> mq);
    bool HasValidMQ(void);
    void Start(const std::string &host, int port, const std::string &username,
          const std::string &password);
    void Restart();
    void Stop();
    void UpdateDiscoveryMsg(const std::string &protocol, const void *msg, int length);
    int AddDiscoveryCB(const std::string &protocol, const DiscoveryCallback &cb);
    void RemoveDiscoveryCB(int callback_id);
    bool CompareTopic(const std::string &left, const std::string &right);
    std::string GetId(void) const { return id_; };

  private:
    struct DiscoveryBlob {
        explicit DiscoveryBlob(const void *msg, int length);
        ~DiscoveryBlob();
        DiscoveryBlob(const DiscoveryBlob &src);
        DiscoveryBlob &operator=(const DiscoveryBlob &src);

        int len;
        std::shared_ptr<char> data;
    };

    static void DiscoveryMessageCallback(AittMsg *mq, const std::string &topic, const void *msg,
          const int szmsg, void *user_data);
    void PublishDiscoveryMsg();

    std::string id_;
    std::unique_ptr<MQ> discovery_mq;
    void *callback_handle;
    std::map<std::string, DiscoveryBlob> discovery_map;
    std::map<int, std::pair<std::string, DiscoveryCallback>> callbacks;
};

// Discovery Message (flexbuffers)
// map {
//   "status": "connected",
//   "tcp": Blob Data for tcp Module,
//   "webrtc": Blob Data for tcp Module,
// }

}  // namespace aitt
