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

#include <memory>
#include <string>

#include "TCP.h"

namespace AittTCPNamespace {

class TCP::Server {
  public:
    Server(const std::string &host, unsigned short &port, bool secure = false);
    virtual ~Server(void);

    std::unique_ptr<TCP> AcceptPeer(void);

    int GetHandle(void);
    unsigned short GetPort(void);
    const unsigned char *GetCryptoKey(void);
    const unsigned char *GetCryptoIv(void);

  private:
    int handle;
    sockaddr *addr_;
    socklen_t addrlen_;
    bool secure;
    unsigned char key[AITT_TCP_ENCRYPTOR_KEY_LEN];
    unsigned char iv[AITT_TCP_ENCRYPTOR_IV_LEN];
};

}  // namespace AittTCPNamespace
