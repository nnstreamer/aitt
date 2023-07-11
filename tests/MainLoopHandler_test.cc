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

#define SLEEP_10MS 10000
#define MAINLOOP_MESSAGE "1"
using aitt::MainLoopIface;

class MainLoopTest : public testing::Test {
  public:
    static int CheckCount(MainLoopIface *handler, int &count)
    {
        switch (count) {
        case 0:
            count++;
            return AITT_LOOP_EVENT_CONTINUE;
        case 1:
            std::thread([&, handler]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                handler->Quit();
            }).detach();
            return AITT_LOOP_EVENT_REMOVE;
        default:
            ADD_FAILURE() << "Should not be called";
            return AITT_LOOP_EVENT_REMOVE;
        }
    }

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

    void ReadAndCheck(int fd)
    {
        char buf[2] = {0};
        EXPECT_EQ(read(fd, buf, 1), 1);
        EXPECT_STREQ(buf, MAINLOOP_MESSAGE);
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

        ret = write(fd, MAINLOOP_MESSAGE, 1);
        ASSERT_NE(ret, -1);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        close(fd);
    }
};

TEST(MainLoop_Test, Quit_N_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    EXPECT_EQ(handler->Quit(), false);
    delete handler;
}

TEST(MainLoop_Test, Destructor_WO_quit_N_Anytime)
{
    EXPECT_NO_THROW({
        MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
        handler->AddIdle(
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  return AITT_LOOP_EVENT_CONTINUE;
              },
              nullptr);
        delete handler;
    });
}

TEST(MainLoop_Test, AddIdle_Anytime)
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

TEST(MainLoop_Test, IDLE_CB_Return_P_Anytime)
{
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());

    handler->AddIdle(
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              static int count = 0;
              EXPECT_EQ(result, MainLoopIface::Event::OKAY);
              return MainLoopTest::CheckCount(handler.get(), count);
          },
          nullptr);

    handler->Run();
}

TEST(MainLoop_Test, AddTimeout_P_Anytime)
{
    std::vector<int> interval_list = {1, 10};

    for (auto interval : interval_list) {
        bool ret = false;
        struct timespec ts_start, ts_end;
        MainLoopIface::MainLoopData test_data;
        MainLoopIface *handler = aitt::MainLoopHandler::new_loop();

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
                  DBG("diff: %f", diff);
                  ret = true;
                  return AITT_LOOP_EVENT_REMOVE;
              },
              &test_data);

        handler->Run();
        delete handler;

        EXPECT_TRUE(ret);
    }
}

TEST(MainLoop_Test, AddTimeout_N_Anytime)
{
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();

    std::vector<int> interval_list = {-1, 0};
    for (auto interval : interval_list) {
        auto id = handler->AddTimeout(
              interval,
              [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
                  ADD_FAILURE() << "Should not be called";
                  return AITT_LOOP_EVENT_REMOVE;
              },
              nullptr);
        EXPECT_EQ(id, 0);
    }
    delete handler;
}

TEST(MainLoop_Test, Timeout_CB_Return_P_Anytime)
{
    int interval = 10;
    std::unique_ptr<MainLoopIface> handler(aitt::MainLoopHandler::new_loop());

    handler->AddTimeout(
          interval,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              static int count = 0;
              EXPECT_EQ(result, MainLoopIface::Event::OKAY);
              return MainLoopTest::CheckCount(handler.get(), count);
          },
          nullptr);

    handler->Run();
}

TEST(MainLoop_Test, Quit_Without_RemoveTimeout_Anytime)
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
              ret = !ret;
              return AITT_LOOP_EVENT_CONTINUE;
          },
          &test_data);

    handler->Run();
    usleep(SLEEP_10MS);

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, AddWatch_Normal_P_Anytime)
{
    bool ret = false;
    MainLoopIface::MainLoopData test_data;
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();

    handler->AddWatch(
          server_fd,
          [&](MainLoopIface::Event result, int fd, MainLoopIface::MainLoopData *data) -> int {
              EXPECT_EQ(data, &test_data);
              int client_fd = accept(server_fd, 0, 0);
              EXPECT_NE(client_fd, -1);
              handler->AddWatch(
                    client_fd,
                    [&](MainLoopIface::Event result, int fd,
                          MainLoopIface::MainLoopData *data) -> int {
                        EXPECT_EQ(data, &test_data);
                        EXPECT_EQ(result, MainLoopIface::Event::OKAY);
                        ReadAndCheck(fd);
                        handler->Quit();
                        ret = true;
                        return AITT_LOOP_EVENT_REMOVE;
                    },
                    &test_data);
              return AITT_LOOP_EVENT_REMOVE;
          },
          &test_data);
    handler->Run();
    delete handler;

    EXPECT_TRUE(ret);
}

TEST_F(MainLoopTest, HANGUP_N_Anytime)
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
                            ReadAndCheck(fd);
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

TEST_F(MainLoopTest, removeWatch_P_Anytime)
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

TEST(MainLoop_Test, removeWatch_N_Anytime)
{
    int unknown_fd = 777;
    MainLoopIface *handler = aitt::MainLoopHandler::new_loop();
    MainLoopIface::MainLoopData test_data;

    MainLoopIface::MainLoopData *check_data = handler->RemoveWatch(unknown_fd);
    delete handler;

    EXPECT_EQ(check_data, nullptr);
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
