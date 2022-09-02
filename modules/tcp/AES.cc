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
#include "AES.h"
#ifndef ANDROID
#include <openssl/aes.h>
#endif
#include <stdexcept>

#include "aitt_internal.h"

#define AES_KEY_BIT_SIZE 128

static const unsigned char cipher_key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

void AES::Encrypt(const unsigned char *target_data, unsigned char *encrypted_data)
{
#ifndef ANDROID
    static int first_run = 1;
    static AES_KEY encryption_key;

    if (first_run == 1) {
        if (AES_set_encrypt_key(cipher_key, AES_KEY_BIT_SIZE, &encryption_key) < 0) {
            ERR("Fail to AES_set_encrypt_key()");
            throw std::runtime_error(strerror(errno));
        }
        first_run = 0;
    }

    AES_ecb_encrypt(target_data, encrypted_data, &encryption_key, AES_ENCRYPT);
#endif
}

void AES::Decrypt(const unsigned char *target_data, unsigned char *decrypted_data)
{
#ifndef ANDROID
    static int first_run = 1;
    static AES_KEY decryption_key;

    if (first_run == 1) {
        if (AES_set_decrypt_key(cipher_key, AES_KEY_BIT_SIZE, &decryption_key) < 0) {
            ERR("Fail to AES_set_decrypt_key()");
            throw std::runtime_error(strerror(errno));
        }
        first_run = 0;
    }

    AES_ecb_encrypt(target_data, decrypted_data, &decryption_key, AES_DECRYPT);
#endif
}
