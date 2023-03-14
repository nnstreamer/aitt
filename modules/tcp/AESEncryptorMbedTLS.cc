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
#include "AESEncryptorMbedTLS.h"

#include <mbedtls/aes.h>

#include "aitt_internal.h"

namespace AittTCPNamespace {

int AESEncryptorMbedTLS::Encrypt(const unsigned char *plaintext, int plaintext_len,
      unsigned char *ciphertext)
{
    if (key_.size() == 0)
        return -1;

    const int BLOCKSIZE = AITT_TCP_ENCRYPTOR_BLOCK_SIZE;
    int padding_len = BLOCKSIZE - (plaintext_len % BLOCKSIZE);
    unsigned char padding_buffer[plaintext_len + padding_len];

    memcpy(padding_buffer, plaintext, plaintext_len);
    for (int i = 0; i < padding_len; i++)
        padding_buffer[plaintext_len + i] = padding_len;

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key_.data(), AITT_TCP_ENCRYPTOR_KEY_LEN * 8);
    unsigned char iv[AITT_TCP_ENCRYPTOR_IV_LEN];
    std::copy(iv_.begin(), iv_.end(), iv);
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, plaintext_len + padding_len, iv,
          padding_buffer, ciphertext);

    return plaintext_len + padding_len;
}

int AESEncryptorMbedTLS::Decrypt(const unsigned char *ciphertext, int ciphertext_len,
      unsigned char *plaintext)
{
    if (key_.size() == 0)
        return -1;

    unsigned char iv[AITT_TCP_ENCRYPTOR_IV_LEN];
    std::copy(iv_.begin(), iv_.end(), iv);
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    mbedtls_aes_setkey_dec(&ctx, key_.data(), AITT_TCP_ENCRYPTOR_KEY_LEN * 8);
    mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, ciphertext_len, iv, ciphertext, plaintext);

    int padding_size = plaintext[ciphertext_len - 1];
    for (int i = 0; i < padding_size; i++) {
        int loc = ciphertext_len - 1 - i;
        if (plaintext[loc] != padding_size) {
            ERR("Not Match padding(%d != %d) at %d", plaintext[loc], padding_size, loc);
            return -1;
        }
    }

    return ciphertext_len - padding_size;
}

}  // namespace AittTCPNamespace
