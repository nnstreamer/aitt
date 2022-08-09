/*
 * Copyright (c) 2021-2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <condition_variable>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "TCPServer.h"

#define TEST_SERVER_ADDRESS "127.0.0.1"
#define TEST_SERVER_INVALID_ADDRESS "287.0.0.1"
#define TEST_SERVER_PORT 8123
#define TEST_SERVER_AVAILABLE_PORT 0
#define TEST_BUFFER_SIZE 256
#define TEST_BUFFER_HELLO "Hello World"
#define TEST_BUFFER_BYE "Good Bye"

class TCPTest : public testing::Test {
  protected:
    void SetUp() override
    {
        ready = false;
        serverPort = TEST_SERVER_PORT;
        customTest = [](void) {};

        clientThread = std::thread([this](void) mutable -> void {
            std::unique_lock<std::mutex> lk(m);
            ready_cv.wait(lk, [this] { return ready; });
            client = std::make_unique<TCP>(TEST_SERVER_ADDRESS, serverPort);

            customTest();
        });
    }

    void RunServer(void)
    {
        tcp = std::make_unique<TCP::Server>(TEST_SERVER_ADDRESS, serverPort);
        {
            std::lock_guard<std::mutex> lk(m);
            ready = true;
        }
        ready_cv.notify_one();

        peer = tcp->AcceptPeer();
    }

    void TearDown() override { clientThread.join(); }

  protected:
    std::mutex m;
    std::condition_variable ready_cv;
    bool ready;
    unsigned short serverPort;
    std::thread clientThread;
    std::unique_ptr<TCP::Server> tcp;
    std::unique_ptr<TCP> peer;
    std::unique_ptr<TCP> client;
    std::function<void(void)> customTest;
};

TEST(TCP, Negative_Create_InvalidPort_Anytime)
{
    try {
        std::unique_ptr<TCP> tcp(
              std::make_unique<TCP>(TEST_SERVER_ADDRESS, TEST_SERVER_AVAILABLE_PORT));
        ASSERT_EQ(tcp, nullptr);
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}

TEST(TCP, Negative_Create_InvalidAddress_Anytime)
{
    try {
        std::unique_ptr<TCP> tcp(
              std::make_unique<TCP>(TEST_SERVER_INVALID_ADDRESS, TEST_SERVER_PORT));
        ASSERT_EQ(tcp, nullptr);
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}

TEST_F(TCPTest, Positive_GetPeerInfo_Anytime)
{
    std::string peerHost;
    unsigned short peerPort = 0;

    RunServer();

    peer->GetPeerInfo(peerHost, peerPort);
    ASSERT_STREQ(peerHost.c_str(), TEST_SERVER_ADDRESS);
    ASSERT_GT(peerPort, 0);
}

TEST_F(TCPTest, Positive_GetHandle_Anytime)
{
    RunServer();
    int handle = peer->GetHandle();
    ASSERT_GE(handle, 0);
}

TEST_F(TCPTest, Positive_GetPort_Anytime)
{
    RunServer();
    unsigned short port = peer->GetPort();
    ASSERT_GT(port, 0);
}

TEST_F(TCPTest, Positive_SendRecv_Anytime)
{
    char helloBuffer[TEST_BUFFER_SIZE];
    char byeBuffer[TEST_BUFFER_SIZE];

    customTest = [this, &helloBuffer](void) mutable -> void {
        size_t szData = sizeof(helloBuffer);
        client->Recv(static_cast<void *>(helloBuffer), szData);

        szData = sizeof(TEST_BUFFER_BYE);
        client->Send(TEST_BUFFER_BYE, szData);
    };

    RunServer();

    size_t szMsg = sizeof(TEST_BUFFER_HELLO);
    peer->Send(TEST_BUFFER_HELLO, szMsg);

    szMsg = sizeof(byeBuffer);
    peer->Recv(static_cast<void *>(byeBuffer), szMsg);

    ASSERT_STREQ(helloBuffer, TEST_BUFFER_HELLO);
    ASSERT_STREQ(byeBuffer, TEST_BUFFER_BYE);
}
