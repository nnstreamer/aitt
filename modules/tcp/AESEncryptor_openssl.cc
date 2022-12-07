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

#ifndef WITH_MBEDTLS

#include <openssl/err.h>
#include <openssl/evp.h>

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
    int len;
    int ciphertext_len;

    if (key_.size() == 0)
        return 0;

    std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx(EVP_CIPHER_CTX_new(),
          [](EVP_CIPHER_CTX *c) { EVP_CIPHER_CTX_free(c); });
    if (ctx.get() == nullptr) {
        ERR("EVP_CIPHER_CTX_new() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_cbc(), NULL, key_.data(), iv_.data())) {
        ERR("EVP_EncryptInit_ex() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_EncryptUpdate(ctx.get(), ciphertext, &ciphertext_len, plaintext, plaintext_len)) {
        ERR("EVP_EncryptUpdate() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_EncryptFinal_ex(ctx.get(), ciphertext + ciphertext_len, &len)) {
        ERR("EVP_EncryptFinal_ex() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    return ciphertext_len + len;
}

int AESEncryptor::Decrypt(const unsigned char *ciphertext, int ciphertext_len,
      unsigned char *plaintext)
{
    int len;
    int plaintext_len;

    if (key_.size() == 0)
        return 0;

    std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx(EVP_CIPHER_CTX_new(),
          [](EVP_CIPHER_CTX *c) { EVP_CIPHER_CTX_free(c); });
    if (ctx.get() == nullptr) {
        ERR("EVP_CIPHER_CTX_new() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_cbc(), NULL, key_.data(), iv_.data())) {
        ERR("EVP_DecryptInit_ex() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_DecryptUpdate(ctx.get(), plaintext, &plaintext_len, ciphertext, ciphertext_len)) {
        ERR("EVP_DecryptUpdate() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }

    if (1 != EVP_DecryptFinal_ex(ctx.get(), plaintext + plaintext_len, &len)) {
        ERR("EVP_DecryptFinal_ex() Fail(%s)", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }
    plaintext_len += len;

    return plaintext_len;
}

}  // namespace AittTCPNamespace

#endif  // WITH_MBEDTLS
