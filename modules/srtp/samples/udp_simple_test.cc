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
#include <UDP.h>
#include <getopt.h>
#include <glib.h>
#include <log.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

#define HELLO_STRING "hello"
#define BYE_STRING "bye"
#define SEND_INTERVAL 1000

__thread __aitt__tls__ __aitt;

int main(int argc, char *argv[])
{
    const option opts[] = {
          {
                .name = "first",
                .has_arg = 0,
                .flag = nullptr,
                .val = 'f',
          },
          {
                .name = "host",
                .has_arg = 1,
                .flag = nullptr,
                .val = 'h',
          },
          {
                .name = "port",
                .has_arg = 1,
                .flag = nullptr,
                .val = 'p',
          },
    };
    int c;
    int idx;
    bool isFirst = false;
    std::string host = "127.0.0.1";
    unsigned short sendPort = 0;

    while ((c = getopt_long(argc, argv, "fh:p:", opts, &idx)) != -1) {
        switch (c) {
        case 'f':
            isFirst = true;
            break;
        case 'h':
            host = optarg;
            break;
        case 'p':
            sendPort = std::stoi(optarg);
            break;
        default:
            break;
        }
    }

    INFO("Host[%s] send[%u]", host.c_str(), sendPort);

    GMainLoop *mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        ERR("Failed to create a main loop");
        return 1;
    }

    const int testCount = 3;

    if (isFirst) {
        char buffer[30];
        std::string msg;

        // NOTE:
        // In the case of using the UDP() constructor,
        // the sender does not want to care what port will be assigned to receive the
        // datagram from the peer. Just send message and simply get the response back from
        // the peer.

        std::unique_ptr<aitt::UDP> peer(std::make_unique<aitt::UDP>());

        for (int i = 0; i < testCount; ++i) {
            size_t szBuffer;
            msg = "My ID is " + std::to_string(getpid());
            INFO("Send[%s]", msg.c_str());
            szBuffer = msg.size() + 1;
            peer->Send(msg.c_str(), szBuffer, host, sendPort);

            void *ptr = static_cast<void *>(buffer);
            szBuffer = sizeof(buffer);
            peer->Recv(ptr, szBuffer);
            INFO("Replied with[%s]", buffer);
        }
    } else {
        char buffer[100];
        std::string msg;

        std::unique_ptr<aitt::UDP> peer(std::make_unique<aitt::UDP>(host, sendPort));

        for (int i = 0; i < testCount; ++i) {
            size_t szBuffer;
            void *ptr = static_cast<void *>(buffer);
            szBuffer = sizeof(buffer);
            std::string sendBackTo;
            unsigned short sendBackPort;
            peer->Recv(ptr, szBuffer, &sendBackTo, &sendBackPort);
            INFO("Gots[%s] from %s:%u", buffer, sendBackTo.c_str(), sendBackPort);

            msg = "My ID is " + std::to_string(getpid());
            szBuffer = msg.size() + 1;
            peer->Send(msg.c_str(), szBuffer, sendBackTo, sendBackPort);
            INFO("Reply to client[%s]", msg.c_str());
        }
    }

    g_main_loop_run(mainLoop);
    g_main_loop_unref(mainLoop);
    return 0;
}
