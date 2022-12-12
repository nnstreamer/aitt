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
#include "AESEncryptorOpenSSL.h"

#include <openssl/err.h>
#include <openssl/evp.h>

#include <memory>
#include <stdexcept>

#include "aitt_internal.h"

namespace AittTCPNamespace {

int AESEncryptorOpenSSL::Encrypt(const unsigned char *plaintext, int plaintext_len,
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

int AESEncryptorOpenSSL::Decrypt(const unsigned char *ciphertext, int ciphertext_len,
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
