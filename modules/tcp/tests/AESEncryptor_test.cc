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
#include "../AESEncryptor.h"

#include <gtest/gtest.h>

#include <cstring>

#include "aitt_internal.h"

static constexpr unsigned char TEST_CIPHER_KEY[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
      0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c, 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2,
      0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static constexpr unsigned char TEST_CIPHER_IV[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
      0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

static const std::string TEST_MESSAGE("TCP encryptions.");

using namespace AittTCPNamespace;

TEST(AESEncryptor, Encrypt_P_Anytime)
{
    try {
        AESEncryptor encryptor;
        encryptor.Init(TEST_CIPHER_KEY, TEST_CIPHER_IV);

        unsigned char encryption_buffer[encryptor.GetCryptogramSize(TEST_MESSAGE.size())];
        encryptor.Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()),
              TEST_MESSAGE.size(), encryption_buffer);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST(AESEncryptor, EncryptDecryped_P_Anytime)
{
    try {
        AESEncryptor encryptor;
        encryptor.Init(TEST_CIPHER_KEY, TEST_CIPHER_IV);

        unsigned char ciphertext[encryptor.GetCryptogramSize(TEST_MESSAGE.size())];
        unsigned char plaintext[encryptor.GetCryptogramSize(TEST_MESSAGE.size())];
        size_t len =
              encryptor.Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()),
                    TEST_MESSAGE.size(), ciphertext);
        len = encryptor.Decrypt(ciphertext, len, plaintext);
        plaintext[len] = 0;
        ASSERT_STREQ(TEST_MESSAGE.c_str(), reinterpret_cast<char *>(plaintext));
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}
