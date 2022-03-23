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
#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <memory>
#include <string>
#include <vector>

namespace aitt {

class UDP {
  public:
    UDP(const std::string &host, unsigned short &port);
    UDP(void);
    virtual ~UDP(void);

    void Send(const void *data, size_t &szData, const std::string &host, unsigned short port);
    void Recv(void *data, size_t &szData, std::string *host = nullptr,
          unsigned short *port = nullptr);
    void JoinMulticastGroup(const std::string &peer, const std::string *iface = nullptr,
          std::string *source = nullptr);
    void LeaveMulticastGroup(const std::string &peer, const std::string *iface = nullptr,
          std::string *source = nullptr);
    void SetMulticastInterface(const std::string &iface);

    int GetHandle(void);

  private:
    int GetMulticastRequest(const std::string &peer, ip_mreqn &mreq, const std::string *iface);
    int GetMulticastSourceRequest(const std::string &peer, const std::string &source,
          ip_mreq_source &mreq, const std::string *iface);

    int handle;
    sockaddr *addr;
    socklen_t addrlen;
};

}  // namespace aitt
