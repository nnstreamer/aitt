/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "AESEncryptor.h"

namespace AittTCPNamespace {

class AESEncryptorOpenSSL : public AESEncryptor {
  public:
    AESEncryptorOpenSSL() = default;
    ~AESEncryptorOpenSSL(void) = default;

    int Encrypt(const unsigned char *plaintext, int plaintext_len,
          unsigned char *ciphertext) override;
    int Decrypt(const unsigned char *ciphertext, int ciphertext_len,
          unsigned char *plaintext) override;
};

}  // namespace AittTCPNamespace
