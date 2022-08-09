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
#include "MainLoopHandler.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstdlib>
#include <thread>

#include "aitt_internal.h"

class MainLoopTest : public testing::Test {
  protected:
    void SetUp() override
    {
        bzero(&addr, sizeof(addr));

        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_NE(server_fd, -1);

        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, std::to_string(std::rand() / 1e10).c_str(),
              sizeof(addr.sun_path) - 1);

        int ret = bind(server_fd, (struct sockaddr *)&addr, SUN_LEN(&addr));
        ASSERT_NE(ret, -1);

        listen(server_fd, 1);
        my_thread = std::thread(&MainLoopTest::eventWriter, this);
    }

    void TearDown() override
    {
        my_thread.join();
        close(server_fd);
    }

    int server_fd;
    struct sockaddr_un addr;
    std::thread my_thread;

  private:
    void eventWriter()
    {
        int ret;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_NE(fd, -1);

        ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
        ASSERT_NE(ret, -1);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ret = write(fd, "1", 1);
        ASSERT_NE(ret, -1);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        close(fd);
    }
};

using aitt::MainLoopHandler;

TEST_F(MainLoopTest, Normal_Anytime)
{
    MainLoopHandler handler;
    bool ret = false;

    handler.AddWatch(
          server_fd,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler.AddWatch(
                    client_fd,
                    [&](MainLoopHandler::MainLoopResult result, int fd,
                          MainLoopHandler::MainLoopData *data) {
                        EXPECT_EQ(result, MainLoopHandler::OK);
                        char buf[2] = {0};
                        EXPECT_EQ(read(fd, buf, 1), 1);
                        EXPECT_STREQ(buf, "1");
                        handler.Quit();
                        ret = true;
                    },
                    nullptr);
          },
          nullptr);
    handler.Run();

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, HANGUP_Anytime)
{
    MainLoopHandler handler;
    bool ret = false;

    handler.AddWatch(
          server_fd,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler.AddWatch(
                    client_fd,
                    [&](MainLoopHandler::MainLoopResult result, int fd,
                          MainLoopHandler::MainLoopData *data) {
                        if (result == MainLoopHandler::OK) {
                            char buf[2] = {0};
                            EXPECT_EQ(read(fd, buf, 1), 1);
                            EXPECT_STREQ(buf, "1");
                            return;
                        }

                        EXPECT_EQ(result, MainLoopHandler::HANGUP);
                        handler.Quit();
                        ret = true;
                    },
                    nullptr);
          },
          nullptr);

    handler.Run();

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, removeWatch_Anytime)
{
    MainLoopHandler handler;
    MainLoopHandler::MainLoopData test_data;

    handler.AddWatch(
          server_fd,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              FAIL() << "It's removed";
          },
          &test_data);
    MainLoopHandler::MainLoopData *check_data = handler.RemoveWatch(server_fd);

    EXPECT_TRUE(&test_data == check_data);
}

TEST_F(MainLoopTest, UserData_Anytime)
{
    MainLoopHandler handler;
    bool ret = false;

    MainLoopHandler::MainLoopData test_data;

    handler.AddWatch(
          server_fd,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              EXPECT_EQ(data, &test_data);
              handler.Quit();
              ret = true;
          },
          &test_data);

    handler.Run();

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, AddIdle_Anytime)
{
    bool ret = false;
    MainLoopHandler handler;
    MainLoopHandler::MainLoopData test_data;

    handler.AddIdle(
          &handler,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              EXPECT_EQ(data, &test_data);
              handler.Quit();
              ret = true;
          },
          &test_data);

    handler.Run();

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, AddTimeout_Anytime)
{
    bool ret = false;
    int interval = 1000;
    MainLoopHandler handler;
    struct timespec ts_start, ts_end;
    MainLoopHandler::MainLoopData test_data;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    handler.AddTimeout(
          interval,
          [&](MainLoopHandler::MainLoopResult result, int fd, MainLoopHandler::MainLoopData *data) {
              EXPECT_EQ(data, &test_data);
              clock_gettime(CLOCK_MONOTONIC, &ts_end);
              double diff = 1000.0 * ts_end.tv_sec + 1e-6 * ts_end.tv_nsec
                            - (1000.0 * ts_start.tv_sec + 1e-6 * ts_start.tv_nsec);
              EXPECT_GE(diff, interval);
              handler.Quit();
              ret = true;
          },
          &test_data);

    handler.Run();

    EXPECT_TRUE(ret);
}
