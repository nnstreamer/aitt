/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include <limits.h>

#include <memory>
#include <random>
#include <vector>

#include "../AESEncryptorMbedTLS.h"
#include "../AESEncryptorOpenSSL.h"
#include "aitt_internal.h"

using namespace AittTCPNamespace;

class AESCompatibilityTest : public testing::Test {
  protected:
    unsigned char TEST_CIPHER_KEY[AITT_TCP_ENCRYPTOR_KEY_LEN] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
          0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c, 0x2b, 0x7e, 0x15, 0x16, 0x28,
          0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    unsigned char TEST_CIPHER_IV[AITT_TCP_ENCRYPTOR_IV_LEN] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
          0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

    AESEncryptorOpenSSL opensslcryptor;
    AESEncryptorMbedTLS mbedtlscryptor;
    std::vector<unsigned char> plaintext;
    unsigned char ciphertext[1024];
    unsigned char decrypted[1024];
};

TEST_F(AESCompatibilityTest, opensslEncrypt_mbedtlsDecrypt_Anytime)
{
    opensslcryptor.Init(TEST_CIPHER_KEY, TEST_CIPHER_IV);
    mbedtlscryptor.Init(TEST_CIPHER_KEY, TEST_CIPHER_IV);

    plaintext.resize(20);
    std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> random_engine;
    std::generate(begin(plaintext), end(plaintext), std::ref(random_engine));

    for (int i = 1; i < static_cast<int>(plaintext.size()); i++) {
        int ciphertext_len = opensslcryptor.Encrypt(plaintext.data(), i, ciphertext);
        int decrypted_len = mbedtlscryptor.Decrypt(ciphertext, ciphertext_len, decrypted);

        ASSERT_EQ(i, decrypted_len);
        ASSERT_EQ(0, memcmp(plaintext.data(), decrypted, decrypted_len));
    }

    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    std::generate(begin(plaintext), end(plaintext), std::ref(random_engine));

    for (int i = 1; i < static_cast<int>(plaintext.size()); i++) {
        int ciphertext_len = mbedtlscryptor.Encrypt(plaintext.data(), i, ciphertext);
        int decrypted_len = opensslcryptor.Decrypt(ciphertext, ciphertext_len, decrypted);

        ASSERT_EQ(i, decrypted_len);
        ASSERT_EQ(0, memcmp(plaintext.data(), decrypted, decrypted_len));
    }

    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    std::generate(begin(plaintext), end(plaintext), std::ref(random_engine));

    for (int i = 1; i < static_cast<int>(plaintext.size()); i++) {
        int ciphertext_len = opensslcryptor.Encrypt(plaintext.data(), i, ciphertext);
        int decrypted_len = opensslcryptor.Decrypt(ciphertext, ciphertext_len, decrypted);

        ASSERT_EQ(i, decrypted_len);
        ASSERT_EQ(0, memcmp(plaintext.data(), decrypted, decrypted_len));
    }

    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    std::generate(begin(plaintext), end(plaintext), std::ref(random_engine));

    for (int i = 1; i < static_cast<int>(plaintext.size()); i++) {
        int ciphertext_len = mbedtlscryptor.Encrypt(plaintext.data(), i, ciphertext);
        int decrypted_len = mbedtlscryptor.Decrypt(ciphertext, ciphertext_len, decrypted);

        ASSERT_EQ(i, decrypted_len);
        ASSERT_EQ(0, memcmp(plaintext.data(), decrypted, decrypted_len));
    }
}
