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

#include <memory>
#include <random>
#include <stdexcept>

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
    key_.insert(key_.begin(), key, key + AITT_TCP_ENCRYPTOR_KEY_LEN);
    iv_.insert(iv_.begin(), iv, iv + AITT_TCP_ENCRYPTOR_IV_LEN);

    DBG_HEX_DUMP(key_.data(), key_.size());
    DBG_HEX_DUMP(iv_.data(), iv_.size());
}

int AESEncryptor::GetCryptogramSize(int plain_size)
{
    const int BLOCKSIZE = 16;
    return (plain_size / BLOCKSIZE + 1) * BLOCKSIZE;
}

void AESEncryptor::GenerateKey(unsigned char (&key)[AITT_TCP_ENCRYPTOR_KEY_LEN],
      unsigned char (&iv)[AITT_TCP_ENCRYPTOR_IV_LEN])
{
    std::mt19937 random_gen{std::random_device{}()};
    std::uniform_int_distribution<> gen(0, 255);

    size_t i;
    for (i = 0; i < sizeof(iv); i++) {
        key[i] = gen(random_gen);
        iv[i] = gen(random_gen);
    }
    for (size_t j = i; j < sizeof(key); j++) {
        key[j] = gen(random_gen);
    }
}

int AESEncryptor::Encrypt(const unsigned char *plaintext, int plaintext_len,
      unsigned char *ciphertext)
{
    if (key_.size() == 0)
        return 0;
    std::vector<unsigned char> iv(iv_);
    int padding_size = 16 - (plaintext_len % 16);
    unsigned char padding_buffer[plaintext_len + padding_size] = {0};
    memcpy(padding_buffer, plaintext, plaintext_len);
    for (int i = 0; i < padding_size; i++) {
        padding_buffer[plaintext_len + i] = padding_size;
    }

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key_.data(), 256);
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, plaintext_len + padding_size, iv.data(),
          padding_buffer, ciphertext);

    return plaintext_len + padding_size;
}

int AESEncryptor::Decrypt(const unsigned char *ciphertext, int ciphertext_len,
      unsigned char *plaintext)
{
    if (key_.size() == 0)
        return 0;
    std::vector<unsigned char> iv(iv_);
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    mbedtls_aes_setkey_dec(&ctx, key_.data(), 256);
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, ciphertext_len, iv.data(), ciphertext,
          plaintext);

    int padding_size = plaintext[ciphertext_len - 1];
    for (int i = 0; i < padding_size; i++) {
        if (plaintext[ciphertext_len - 1 - i] != padding_size)
            return -1;
    }

    return ciphertext_len - padding_size;
}

}  // namespace AittTCPNamespace

#endif  // WITH_MBEDTLS
