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

#include <memory>
#include <string>

#include "AESEncryptor.h"
#include "TCP.h"

class TCP::Server {
  public:
    Server(const std::string &host, unsigned short &port);
    Server(const Server &) = default;
    Server &operator=(const Server &) = default;
    virtual ~Server(void);

    std::unique_ptr<TCP> AcceptPeer(void);

    int GetHandle(void);
    unsigned short GetPort(void);
    void CreateAESEncryptor(void);
    AESEncryptor *GetAESEncryptor(void);
    const unsigned char *GetKey(void);

  private:
    int handle;
    sockaddr *addr;
    socklen_t addrlen;
    AESEncryptor *aes_encryptor;
};
