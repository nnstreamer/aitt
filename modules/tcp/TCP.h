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

#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */

#include <string>

class TCP {
  public:
    class Server;

    TCP(const std::string &host, unsigned short port);
    virtual ~TCP(void);

    void Send(const void *data, size_t &szData);
    void Recv(void *data, size_t &szData);
    int GetHandle(void);
    unsigned short GetPort(void);
    void GetPeerInfo(std::string &host, unsigned short &port);

  private:
    TCP(int handle, sockaddr *addr, socklen_t addrlen);
    void SetupOptions(void);

    int handle;
    socklen_t addrlen;
    sockaddr *addr;
};
