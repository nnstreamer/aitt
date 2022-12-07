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
#include "AESEncryptor.h"

#ifdef WITH_MBEDTLS

#include <mbedtls/aes.h>

#include "aitt_internal.h"

namespace AittTCPNamespace {

AESEncryptor::AESEncryptor()
{
}

AESEncryptor::~AESEncryptor(void)
{
}

void AESEncryptor::Init(const unsigned char *key, const unsigned char *iv)
{
}

int AESEncryptor::GetCryptogramSize(int plain_size)
{
    return 0;
}

void AESEncryptor::GenerateKey(unsigned char (&key)[AITT_TCP_ENCRYPTOR_KEY_LEN],
      unsigned char (&iv)[AITT_TCP_ENCRYPTOR_IV_LEN])
{
}

int AESEncryptor::Encrypt(const unsigned char *plaintext, int plaintext_len,
      unsigned char *ciphertext)
{
    return 0;
}

int AESEncryptor::Decrypt(const unsigned char *ciphertext, int ciphertext_len,
      unsigned char *plaintext)
{
    return 0;
}

}  // namespace AittTCPNamespace

#endif  // WITH_MBEDTLS
