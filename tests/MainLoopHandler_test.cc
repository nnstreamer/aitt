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
#include "MainLoopHandler.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstdlib>
#include <thread>

#include "MainLoopIface.h"
#include "aitt_internal.h"

using aitt::MainLoopIface;

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
        remove(addr.sun_path);
    }

    int server_fd;
    struct sockaddr_un addr;
    std::thread my_thread;

  protected:
    int CheckCount(MainLoopIface *handler, int &count)
    {
        switch (count) {
        case 0:
            count++;
            return AITT_LOOP_EVENT_CONTINUE;
        case 1:
            std::thread([&, handler]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                handler->Quit();
            }).detach();
            return AITT_LOOP_EVENT_REMOVE;
        default:
            ADD_FAILURE() << "Should not be called";
            return AITT_LOOP_EVENT_REMOVE;
        }
    }

  private:
    void eventWriter()
    {
        int ret;
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_NE(fd, -1);

        ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
        ASSERT_NE(ret, -1);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ret = write(fd, "1", 1);
        ASSERT_NE(ret, -1);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        close(fd);
    }
};

TEST_F(MainLoopTest, Normal_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    bool ret = false;

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler->AddWatch(
                    client_fd,
                    [&](MainLoopIface::Event result, int fd,
                          MainLoopIface::MainLoopData *data) -> int {
                        EXPECT_EQ(result, MainLoopIface::Event::OKAY);
                        char buf[2] = {0};
                        EXPECT_EQ(read(fd, buf, 1), 1);
                        EXPECT_STREQ(buf, "1");
                        handler->Quit();
                        ret = true;
                        return AITT_LOOP_EVENT_REMOVE;
                    },
                    nullptr);
              return AITT_LOOP_EVENT_REMOVE;
          },
          nullptr);
    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, HANGUP_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    bool ret = false;

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler->AddWatch(
                    client_fd,
                    [&](MainLoopIface::Event result, int fd,
                          MainLoopIface::MainLoopData *data) -> int {
                        if (result == MainLoopIface::Event::OKAY) {
                            char buf[2] = {0};
                            EXPECT_EQ(read(fd, buf, 1), 1);
                            EXPECT_STREQ(buf, "1");
                            return AITT_LOOP_EVENT_CONTINUE;
                        }

                        EXPECT_EQ(result, MainLoopIface::Event::HANGUP);
                        handler->Quit();
                        ret = true;
                        return AITT_LOOP_EVENT_REMOVE;
                    },
                    nullptr);
              return AITT_LOOP_EVENT_REMOVE;
          },
          nullptr);

    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, removeWatch_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    MainLoopIface::MainLoopData test_data;

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              ADD_FAILURE() << "It's removed";
              return AITT_LOOP_EVENT_REMOVE;
          },
          &test_data);
    MainLoopIface::MainLoopData *check_data = handler->RemoveWatch(server_fd);
    delete handler;

    EXPECT_TRUE(&test_data == check_data);
}

TEST_F(MainLoopTest, UserData_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    bool ret = false;

    MainLoopIface::MainLoopData test_data;

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              EXPECT_EQ(data, &test_data);
              handler->Quit();
              ret = true;
              return AITT_LOOP_EVENT_REMOVE;
          },
          &test_data);

    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, AddIdle_Anytime)
{
    bool ret = false;
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    MainLoopIface::MainLoopData test_data;

    handler->AddIdle(
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              EXPECT_EQ(data, &test_data);
              handler->Quit();
              ret = true;
              return AITT_LOOP_EVENT_REMOVE;
          },
          &test_data);

    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, AddTimeout_Anytime)
{
    bool ret = false;
    int interval = 100;
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    struct timespec ts_start, ts_end;
    MainLoopIface::MainLoopData test_data;

    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    handler->AddTimeout(
          interval,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              EXPECT_EQ(data, &test_data);
              clock_gettime(CLOCK_MONOTONIC, &ts_end);
              double diff = 1000.0 * ts_end.tv_sec + 1e-6 * ts_end.tv_nsec
                            - (1000.0 * ts_start.tv_sec + 1e-6 * ts_start.tv_nsec);
              EXPECT_GE(diff, interval);
              handler->Quit();
              ret = true;
              return AITT_LOOP_EVENT_REMOVE;
          },
          &test_data);

    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, Quit_Without_RemoveTimeout_Anytime)
{
    bool ret = false;
    int interval = 1;
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());
    MainLoopIface::MainLoopData test_data;

    handler->AddTimeout(
          interval,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              handler->Quit();
              EXPECT_FALSE(ret);
              ret = true;
              return AITT_LOOP_EVENT_CONTINUE;
          },
          &test_data);

    handler->Run();

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, Watch_Event_CB_Return_P_Anytime)
{
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler->AddWatch(
                    client_fd,
                    [&](MainLoopIface::Event result, int fd,
                          MainLoopIface::MainLoopData *data) -> int {
                        static int count = 0;
                        EXPECT_EQ(result, MainLoopIface::Event::OKAY);
                        return CheckCount(handler.get(), count);
                    },
                    nullptr);
              return AITT_LOOP_EVENT_REMOVE;
          },
          nullptr);
    handler->Run();
}

TEST_F(MainLoopTest, IDLE_CB_Return_P_Anytime)
{
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());

    handler->AddIdle(
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              static int count = 0;
              EXPECT_EQ(result, MainLoopIface::Event::OKAY);
              return CheckCount(handler.get(), count);
          },
          nullptr);

    handler->Run();
}

TEST_F(MainLoopTest, Timeout_CB_Return_P_Anytime)
{
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());

    handler->AddTimeout(
          200,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              static int count = 0;
              EXPECT_EQ(result, MainLoopIface::Event::OKAY);
              return CheckCount(handler.get(), count);
          },
          nullptr);

    handler->Run();
}
