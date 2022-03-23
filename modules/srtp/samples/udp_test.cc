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
#include <NetUtil.h>
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
                .name = "sender",
                .has_arg = 1,
                .flag = nullptr,
                .val = 's',
          },
          {
                .name = "receiver",
                .has_arg = 1,
                .flag = nullptr,
                .val = 'r',
          },
    };
    int c;
    int idx;
    bool isFirst = false;
    std::string host = "127.0.0.1";
    unsigned short sendPort = 0;
    unsigned short receivePort = 0;

    while ((c = getopt_long(argc, argv, "fh:s:r:", opts, &idx)) != -1) {
        switch (c) {
        case 'f':
            isFirst = true;
            break;
        case 'h':
            host = optarg;
            break;
        case 's':
            sendPort = std::stoi(optarg);
            break;
        case 'r':
            receivePort = std::stoi(optarg);
            break;
        default:
            break;
        }
    }

    INFO("Host[%s] send[%u] recv[%u]", host.c_str(), sendPort, receivePort);

    GMainLoop *mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        ERR("Failed to create a main loop");
        return 1;
    }

    const int testCount = 3;

    std::unique_ptr<aitt::UDP> peer(std::make_unique<aitt::UDP>("0.0.0.0", receivePort));
    INFO("assigned port: %u", receivePort);

    if (isFirst) {
        char buffer[30];
        size_t szBuffer;
        std::string msg;
        aitt::NetUtil netUtil;

        std::vector<aitt::NetUtil::Interface> ifaceList;

        netUtil.GetInterfaceList(ifaceList);

        INFO("%zd interfaces are found", ifaceList.size());
        int validIfIdx = -1;
        for (size_t i = 0; i < ifaceList.size(); ++i) {
            INFO("Interface[%zd] Name[%s], IPv4[%s], BROADCAST[%s], HWADDR[%s]", i,
                  ifaceList[i].name.c_str(), ifaceList[i].address.c_str(),
                  ifaceList[i].broadcast.c_str(), ifaceList[i].MAC.c_str());

            if (!ifaceList[i].is_up)
                INFO("interface down");
            else if (!ifaceList[i].is_loopback)
                INFO("loopback device");
            else if (!ifaceList[i].multicast_enabled)
                INFO("multicast is not supported");
            else if (validIfIdx < 0 && !ifaceList[i].MAC.empty())
                validIfIdx = i;
        }

        for (int i = 0; i < testCount; ++i) {
            msg = "My ID is " + std::to_string(getpid());
            INFO("Send[%s]", msg.c_str());
            szBuffer = msg.size() + 1;
            peer->Send(msg.c_str(), szBuffer, host, sendPort);

            void *ptr = static_cast<void *>(buffer);
            szBuffer = sizeof(buffer);
            peer->Recv(ptr, szBuffer);
            INFO("Replied with[%s]", buffer);
        }

        // NOTE:
        // Take a time for the server ready
        INFO("Pause 2 seconds for the server ready");
        sleep(2);
        INFO("Let's send the MCAST!");

        if (validIfIdx < 0) {
            INFO("There is no valid interface for the multicast");
        } else {
            // TC: Using the interface name
            INFO("Join MCAST address(239.0.0.1): interface - %s (%s)",
                  ifaceList[validIfIdx].address.c_str(), ifaceList[validIfIdx].name.c_str());
            peer->JoinMulticastGroup("239.0.0.1", &ifaceList[validIfIdx].name);

            msg = "I sent MCAST(" + std::to_string(getpid()) + ") through the "
                  + ifaceList[validIfIdx].name;
            szBuffer = msg.size() + 1;
            peer->Send(msg.c_str(), szBuffer, "239.0.0.1", sendPort);
            INFO("MCAST message sent! [%s] through %d", msg.c_str(), sendPort);
        }
    } else {
        char buffer[100];
        size_t szBuffer;
        std::string msg;
        aitt::NetUtil netUtil;

        std::vector<aitt::NetUtil::Interface> ifaceList;

        netUtil.GetInterfaceList(ifaceList);

        INFO("%zd interfaces are found", ifaceList.size());
        int validIfIdx = -1;
        for (size_t i = 0; i < ifaceList.size(); ++i) {
            INFO("Interface[%zd] Name[%s], IPv4[%s], BROADCAST[%s], HWADDR[%s]", i,
                  ifaceList[i].name.c_str(), ifaceList[i].address.c_str(),
                  ifaceList[i].broadcast.c_str(), ifaceList[i].MAC.c_str());

            if (!ifaceList[i].is_up)
                INFO("interface down");
            else if (!ifaceList[i].is_loopback)
                INFO("loopback device");
            else if (!ifaceList[i].multicast_enabled)
                INFO("multicast is not supported");
            else if (validIfIdx < 0 && !ifaceList[i].MAC.empty())
                validIfIdx = i;
        }

        for (int i = 0; i < testCount; ++i) {
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

        // NOTE:
        // In the local machine, the mcast message will be retrieved
        // In order to test this properly. the client should run on the other deivce,
        // or turn on the MULTICAST_LOOP option for the socket handle
        if (validIfIdx < 0) {
            INFO("There is no valid interface for the multicast");
        } else {
            unsigned short port;
            std::string mcastAddr;
            // TC: Using the interface address
            INFO("Join MCAST address(239.0.0.1): interface - %s (%s)",
                  ifaceList[validIfIdx].address.c_str(), ifaceList[validIfIdx].name.c_str());
            peer->JoinMulticastGroup("239.0.0.1", &ifaceList[validIfIdx].address);
            szBuffer = sizeof(buffer);
            peer->Recv(buffer, szBuffer, &mcastAddr, &port);
            INFO("MCAST [%s] from %s:%d", buffer, mcastAddr.c_str(), port);
        }
    }

    g_main_loop_run(mainLoop);
    g_main_loop_unref(mainLoop);
    return 0;
}
