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

#include <vector>

// AES-256
#define AITT_TCP_ENCRYPTOR_KEY_LEN 32
#define AITT_TCP_ENCRYPTOR_BLOCK_SIZE 16
#define AITT_TCP_ENCRYPTOR_IV_LEN AITT_TCP_ENCRYPTOR_BLOCK_SIZE

namespace AittTCPNamespace {

class AESEncryptor {
  public:
    AESEncryptor() = default;
    virtual ~AESEncryptor(void) = default;

    static void GenerateKey(unsigned char (&key)[AITT_TCP_ENCRYPTOR_KEY_LEN],
          unsigned char (&iv)[AITT_TCP_ENCRYPTOR_IV_LEN]);
    void Init(const unsigned char *key, const unsigned char *iv);
    int GetCryptogramSize(int plain_size);
    virtual int Encrypt(const unsigned char *plaintext, int plaintext_len,
          unsigned char *ciphertext) = 0;
    virtual int Decrypt(const unsigned char *ciphertext, int ciphertext_len,
          unsigned char *plaintext) = 0;

  protected:
    std::vector<unsigned char> key_;
    std::vector<unsigned char> iv_;
};

}  // namespace AittTCPNamespace
