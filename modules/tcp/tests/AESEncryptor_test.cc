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

static constexpr unsigned char TEST_CIPHER_KEY[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
static const std::string TEST_MESSAGE("TCP encryptions.");

class AESEncryptorTest : public testing::Test {
  public:
    static void PrintKey(const unsigned char *key)
    {
        for (int i = 0; i < AESEncryptor::AES_KEY_BYTE_SIZE / 8; i++) {
            DBG("%u %u %u %u %u %u %u %u",
                key[8 * i + 0], key[8 * i + 1], key[8 * i + 2], key[8 * i + 3], key[8 * i + 4], key[8 * i + 5], key[8 * i + 6], key[8 * i + 7]);
        }
    }
};

TEST(AESEncryptor, Positive_Create_Anytime)
{
    std::unique_ptr<AESEncryptor> aes_encryptor(new AESEncryptor());
    ASSERT_NE(aes_encryptor, nullptr);
}

TEST(AESEncryptor, Positive_CreateWithArgument_Anytime)
{
    std::unique_ptr<AESEncryptor> aes_encryptor(new AESEncryptor(TEST_CIPHER_KEY));
    std::string aes_encryptor_key(reinterpret_cast<const char *>(aes_encryptor->GetCipherKey()));
    std::string test_key(reinterpret_cast<const char *>(TEST_CIPHER_KEY), AESEncryptor::AES_KEY_BYTE_SIZE);
    ASSERT_STREQ(aes_encryptor_key.c_str(), test_key.c_str());
}

TEST(AESEncryptor, Positive_GenerateRandomKeys_Anytime)
{
    std::unique_ptr<AESEncryptor> aes_encryptor_first(new AESEncryptor());
    std::unique_ptr<AESEncryptor> aes_encryptor_second(new AESEncryptor());
    std::string first_key(reinterpret_cast<const char *>(aes_encryptor_first->GetCipherKey()), AESEncryptor::AES_KEY_BYTE_SIZE);
    std::string second_key(reinterpret_cast<const char *>(aes_encryptor_second->GetCipherKey()), AESEncryptor::AES_KEY_BYTE_SIZE);
    ASSERT_STRNE(first_key.c_str(), second_key.c_str());
}

TEST(AESEncryptor, Positive_Encrypt_Anytime)
{
    std::unique_ptr<AESEncryptor> aes_encryptor(new AESEncryptor());
    AESEncryptorTest::PrintKey(aes_encryptor->GetCipherKey());

    try {
        unsigned char encryption_buffer[AESEncryptor::AES_KEY_BYTE_SIZE];
        aes_encryptor->Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()), encryption_buffer);
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}

TEST(AESEncryptor, Positive_EncryptDecryped_Anytime)
{
    std::unique_ptr<AESEncryptor> aes_encryptor(new AESEncryptor());
    AESEncryptorTest::PrintKey(aes_encryptor->GetCipherKey());

    try {
        unsigned char encryption_buffer[AESEncryptor::AES_KEY_BYTE_SIZE];
        unsigned char decryption_buffer[AESEncryptor::AES_KEY_BYTE_SIZE];
        aes_encryptor->Encrypt(reinterpret_cast<const unsigned char *>(TEST_MESSAGE.c_str()), encryption_buffer);
        aes_encryptor->Decrypt(encryption_buffer, decryption_buffer);
        std::string decrypted_message(reinterpret_cast<const char *>(decryption_buffer), AESEncryptor::AES_KEY_BYTE_SIZE);
        DBG("TEST_MESSAGE = (%s), decrypted_message = (%s)", TEST_MESSAGE.c_str(), decrypted_message.c_str());
        ASSERT_STREQ(decrypted_message.c_str(), TEST_MESSAGE.c_str());
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}
