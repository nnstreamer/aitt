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
#include <gtest/gtest.h>

#include <cstring>

#include "../AESEncryptorMbedTLS.h"
#include "../AESEncryptorOpenSSL.h"
#include "aitt_internal.h"

using namespace AittTCPNamespace;

class AESEncryptorTest : public testing::Test {
  protected:
    void SetUp() override { crypto.Init(TEST_CIPHER_KEY, TEST_CIPHER_IV); }

    unsigned char TEST_CIPHER_KEY[AITT_TCP_ENCRYPTOR_KEY_LEN] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
          0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c, 0x2b, 0x7e, 0x15, 0x16, 0x28,
          0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    unsigned char TEST_CIPHER_IV[AITT_TCP_ENCRYPTOR_IV_LEN] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
          0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

    const std::string TEST_MESSAGE = "TCP encryptions.";

#ifdef WITH_MBEDTLS
    AESEncryptorMbedTLS crypto;
#else
    AESEncryptorOpenSSL crypto;
#endif
};

TEST_F(AESEncryptorTest, Encrypt_P_Anytime)
{
    try {
        unsigned char encryption_buffer[crypto.GetCryptogramSize(TEST_MESSAGE.size())];
        crypto.Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()),
              TEST_MESSAGE.size(), encryption_buffer);
    } catch (std::exception &e) {
        FAIL() << "Unexpected exception: " << e.what();
    }
}

TEST_F(AESEncryptorTest, EncryptDecryped_P_Anytime)
{
    try {
        unsigned char ciphertext[crypto.GetCryptogramSize(TEST_MESSAGE.size())];
        unsigned char plaintext[crypto.GetCryptogramSize(TEST_MESSAGE.size())];
        int len = crypto.Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()),
              TEST_MESSAGE.size(), ciphertext);
        len = crypto.Decrypt(ciphertext, len, plaintext);
        plaintext[len] = 0;
        ASSERT_STREQ(TEST_MESSAGE.c_str(), reinterpret_cast<char *>(plaintext));
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}
