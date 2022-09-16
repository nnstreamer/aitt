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
#ifndef ANDROID
#include <openssl/aes.h>
#endif
#include <algorithm>
#include <climits>
#include <functional>
#include <random>
#include <stdexcept>

#include "aitt_internal.h"

using random_bytes_generator =
      std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;

AESEncryptor::AESEncryptor(void)
{
    GenerateCipherKey();
}

AESEncryptor::AESEncryptor(const unsigned char key[AES_KEY_BYTE_SIZE])
{
    memcpy(cipher_key, key, AES_KEY_BYTE_SIZE);
}

AESEncryptor::~AESEncryptor(void)
{
}

void AESEncryptor::GenerateCipherKey(void)
{
    std::random_device rd;
    random_bytes_generator rbg(rd());
    std::vector<unsigned char> key_vector(AES_KEY_BYTE_SIZE);
    std::generate(begin(key_vector), end(key_vector), std::ref(rbg));
    std::copy(key_vector.begin(), key_vector.end(), cipher_key);
}

unsigned char *AESEncryptor::GetEncryptedData(
      const void *data, size_t data_length, size_t &encrypted_data_length)
{
    size_t padding_buffer_size = GetPaddingBufferSize(data_length);
    DBG("data_length = %zu, padding_buffer_size = %zu", data_length, padding_buffer_size);

    unsigned char padding_buffer[padding_buffer_size];
    memcpy(padding_buffer, data, data_length);

    unsigned char *encrypted_data = (unsigned char *)malloc(padding_buffer_size);
    for (int i = 0; i < static_cast<int>(padding_buffer_size) / AESEncryptor::AES_KEY_BYTE_SIZE;
          i++) {
        Encrypt(padding_buffer + AESEncryptor::AES_KEY_BYTE_SIZE * i,
              encrypted_data + AESEncryptor::AES_KEY_BYTE_SIZE * i);
    }
    encrypted_data_length = padding_buffer_size;

    return encrypted_data;
}

void AESEncryptor::Encrypt(const unsigned char *target_data, unsigned char *encrypted_data)
{
#ifndef ANDROID
    AES_KEY encryption_key;
    if (AES_set_encrypt_key(cipher_key, AES_KEY_BIT_SIZE, &encryption_key) < 0) {
        ERR("Fail to AES_set_encrypt_key()");
        throw std::runtime_error(strerror(errno));
    }

    AES_ecb_encrypt(target_data, encrypted_data, &encryption_key, AES_ENCRYPT);
#endif
}

void AESEncryptor::GetDecryptedData(
      unsigned char *padding_buffer, size_t padding_buffer_size, size_t data_length, void *data)
{
    unsigned char decrypted_data[padding_buffer_size];
    for (int i = 0; i < (int)padding_buffer_size / AESEncryptor::AES_KEY_BYTE_SIZE; i++) {
        Decrypt(padding_buffer + AESEncryptor::AES_KEY_BYTE_SIZE * i,
              decrypted_data + AESEncryptor::AES_KEY_BYTE_SIZE * i);
    }
    memcpy(data, decrypted_data, data_length);
}

void AESEncryptor::Decrypt(const unsigned char *target_data, unsigned char *decrypted_data)
{
#ifndef ANDROID
    AES_KEY decryption_key;
    if (AES_set_decrypt_key(cipher_key, AES_KEY_BIT_SIZE, &decryption_key) < 0) {
        ERR("Fail to AES_set_decrypt_key()");
        throw std::runtime_error(strerror(errno));
    }

    AES_ecb_encrypt(target_data, decrypted_data, &decryption_key, AES_DECRYPT);
#endif
}

size_t AESEncryptor::GetPaddingBufferSize(size_t data_length)
{
    size_t padding_buffer_size = (data_length + AESEncryptor::AES_KEY_BYTE_SIZE)
                                 / AESEncryptor::AES_KEY_BYTE_SIZE * AESEncryptor::AES_KEY_BYTE_SIZE;
    if (padding_buffer_size % AESEncryptor::AES_KEY_BYTE_SIZE != 0) {
        ERR("data_length is not a multiple of AES_KEY_BYTE_SIZE.");
        return 0;
    }

    return padding_buffer_size;
}

const unsigned char *AESEncryptor::GetCipherKey(void)
{
    return cipher_key;
}
