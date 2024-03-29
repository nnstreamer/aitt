/*
 * Copyright 2021-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "AESEncryptorMbedTLS.h"
#include "AESEncryptorOpenSSL.h"

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif

namespace AittTCPNamespace {
class TCP {
  public:
    class Server;
    struct ConnectInfo {
        ConnectInfo();

        unsigned short port;
        int num_of_cb;
        bool secure;
        unsigned char key[AITT_TCP_ENCRYPTOR_KEY_LEN];
        unsigned char iv[AITT_TCP_ENCRYPTOR_IV_LEN];
    };

    enum ConnectInfoMember {
        CONN_INFO_PORT,
        CONN_INFO_NUM_OF_CB,
        CONN_INFO_KEY,
        CONN_INFO_IV,
        CONN_INFO_MAX
    };

    TCP(const std::string &host, const ConnectInfo &ConnectInfo);
    virtual ~TCP(void);

    void SendSizedData(const void *data, int32_t data_size);
    int RecvSizedData(void **data);
    int GetHandle(void);
    unsigned short GetPort(void);
    void GetPeerInfo(std::string &host, unsigned short &port);

    // For unittest, it's public
    int32_t Send(const void *data, int32_t data_size);
    int32_t Recv(void *data, int32_t szData);

  private:
    TCP(int handle, sockaddr *addr, socklen_t addrlen, const ConnectInfo &connect_info);
    void SetupOptions(const ConnectInfo &connect_info);
    int HandleZeroMsg(void **data);
    void SendSizedDataNormal(const void *data, int32_t data_size);
    int RecvSizedDataNormal(void **data);
    void SendSizedDataSecure(const void *data, int32_t data_size);
    int RecvSizedDataSecure(void **data);

    int handle_;
    socklen_t addrlen_;
    sockaddr *addr_;
    bool secure;
#ifdef WITH_MBEDTLS
    AESEncryptorMbedTLS crypto;
#else
    AESEncryptorOpenSSL crypto;
#endif
};

}  // namespace AittTCPNamespace
