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

#include <stdio.h>

class AESEncryptor {
  public:
    constexpr static int AES_KEY_BYTE_SIZE = 16;

  public:
    AESEncryptor(void);
    explicit AESEncryptor(const unsigned char key[AES_KEY_BYTE_SIZE]);
    ~AESEncryptor(void);

    unsigned char *GetEncryptedData(
          const void *data, size_t data_length, size_t &encrypted_data_length);
    void Encrypt(const unsigned char *target_data, unsigned char *encrypted_data);
    void GetDecryptedData(unsigned char *padding_buffer, size_t padding_buffer_size,
          size_t data_length, void *data);
    void Decrypt(const unsigned char *target_data, unsigned char *decrypted_data);
    size_t GetPaddingBufferSize(size_t data_length);
    const unsigned char *GetCipherKey(void);

  private:
    void GenerateCipherKey(void);

    unsigned char cipher_key[AES_KEY_BYTE_SIZE];

    constexpr static int AES_KEY_BIT_SIZE = AES_KEY_BYTE_SIZE << 3;
};
