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

#include <stdint.h>
#include <sys/socket.h>

#include <string>

#include "AESEncryptor.h"

namespace AittTCPNamespace {

class TCP {
  public:
    class Server;
    struct ConnectInfo {
        struct Compare {
            bool operator()(const ConnectInfo &lhs, const ConnectInfo &rhs) const
            {
                return lhs.port < rhs.port;
            }
        };

        ConnectInfo();
        unsigned short port;
        bool secure;
        unsigned char key[AITT_TCP_ENCRYPTOR_KEY_LEN];
        unsigned char iv[AITT_TCP_ENCRYPTOR_IV_LEN];
    };

    TCP(const std::string &host, const ConnectInfo &ConnectInfo);
    virtual ~TCP(void);

    void Send(const void *data, int32_t &szData);
    void SendSizedData(const void *data, int32_t &szData);
    int Recv(void *data, int32_t &szData);
    int RecvSizedData(void **data, int32_t &szData);
    int GetHandle(void);
    unsigned short GetPort(void);
    void GetPeerInfo(std::string &host, unsigned short &port);

  private:
    TCP(int handle, sockaddr *addr, socklen_t addrlen, const ConnectInfo &connect_info);
    void SetupOptions(const ConnectInfo &connect_info);
    int HandleZeroMsg(void **data, int32_t &data_size);
    void SendSizedDataNormal(const void *data, int32_t &data_size);
    int RecvSizedDataNormal(void **data, int32_t &data_size);
    void SendSizedDataSecure(const void *data, int32_t &data_size);
    int RecvSizedDataSecure(void **data, int32_t &data_size);

    int handle;
    socklen_t addrlen;
    sockaddr *addr;
    bool secure;
    AESEncryptor crypto;
};

}  // namespace AittTCPNamespace
