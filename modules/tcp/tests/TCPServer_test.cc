/*
 * Copyright 2021-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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
#include "../TCPServer.h"

#include <gtest/gtest.h>

#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

#define TEST_SERVER_ADDRESS "127.0.0.1"
#define TEST_SERVER_INVALID_ADDRESS "287.0.0.1"
#define TEST_SERVER_PORT 8123
#define TEST_SERVER_AVAILABLE_PORT 0

using namespace AittTCPNamespace;

TEST(TCPServer, Create_N_Anytime)
{
    try {
        unsigned short port = TEST_SERVER_PORT;

        std::unique_ptr<TCP::Server> tcp(new TCP::Server(TEST_SERVER_INVALID_ADDRESS, port));
        ASSERT_EQ(tcp, nullptr);
    } catch (std::exception &e) {
        ASSERT_STREQ(e.what(), strerror(EINVAL));
    }
}

TEST(TCPServer, Create_AutoPort_P_Anytime)
{
    unsigned short port = TEST_SERVER_AVAILABLE_PORT;
    std::unique_ptr<TCP::Server> tcp(new TCP::Server(TEST_SERVER_ADDRESS, port));
    ASSERT_NE(tcp, nullptr);
    ASSERT_NE(port, 0);
    ASSERT_EQ(tcp->GetPort(), port);
    ASSERT_GE(tcp->GetHandle(), 0);
}

TEST(TCPServer, Create_Port_P_Anytime)
{
    unsigned short port = TEST_SERVER_PORT;
    std::unique_ptr<TCP::Server> tcp(new TCP::Server(TEST_SERVER_ADDRESS, port));
    ASSERT_NE(tcp, nullptr);
    ASSERT_EQ(tcp->GetPort(), TEST_SERVER_PORT);
    ASSERT_GE(tcp->GetHandle(), 0);
}

TEST(TCPServer, AcceptPeer_P_Anytime)
{
    std::mutex m;
    std::condition_variable ready_cv;
    std::condition_variable connected_cv;
    bool ready = false;
    bool connected = false;

    unsigned short serverPort = TEST_SERVER_PORT;
    std::thread serverThread(
          [serverPort, &m, &ready, &connected, &ready_cv, &connected_cv](void) mutable -> void {
              std::unique_ptr<TCP::Server> tcp(new TCP::Server(TEST_SERVER_ADDRESS, serverPort));
              ASSERT_NE(tcp, nullptr);
              {
                  std::lock_guard<std::mutex> lk(m);
                  ready = true;
              }
              ready_cv.notify_one();

              std::unique_ptr<TCP> peer = tcp->AcceptPeer();
              {
                  std::lock_guard<std::mutex> lk(m);
                  connected = !!peer;
              }
              connected_cv.notify_one();
          });

    {
        std::unique_lock<std::mutex> lk(m);
        ready_cv.wait(lk, [&ready] { return ready; });
        TCP::ConnectInfo info;
        info.port = serverPort;
        std::unique_ptr<TCP> tcp(new TCP(TEST_SERVER_ADDRESS, info));
        ASSERT_NE(tcp, nullptr);
        connected_cv.wait(lk, [&connected] { return connected; });
    }

    serverThread.join();

    ASSERT_EQ(ready, true);
    ASSERT_EQ(connected, true);
}
