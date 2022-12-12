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

#include <random>
#include <stdexcept>

#include "aitt_internal.h"

namespace AittTCPNamespace {

void AESEncryptor::Init(const unsigned char *key, const unsigned char *iv)
{
    key_.insert(key_.begin(), key, key + AITT_TCP_ENCRYPTOR_KEY_LEN);
    iv_.insert(iv_.begin(), iv, iv + AITT_TCP_ENCRYPTOR_IV_LEN);

    // DBG_HEX_DUMP(key_.data(), key_.size());
    // DBG_HEX_DUMP(iv_.data(), iv_.size());
}

int AESEncryptor::GetCryptogramSize(int plain_size)
{
    const int BLOCKSIZE = AITT_TCP_ENCRYPTOR_BLOCK_SIZE;
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

    for (size_t j = i; j < sizeof(key); j++)
        key[j] = gen(random_gen);
}

}  // namespace AittTCPNamespace
