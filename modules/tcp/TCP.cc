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
#include "TCP.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "log.h"

TCP::TCP(const std::string &host, unsigned short port) : handle(-1), addrlen(0), addr(nullptr)
{
    int ret = 0;

    do {
        if (port == 0) {
            ret = EINVAL;
            break;
        }

        handle = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (handle < 0) {
            ERR("socket() Fail()");
            break;
        }

        addrlen = sizeof(sockaddr_in);
        addr = static_cast<sockaddr *>(calloc(1, addrlen));
        if (!addr) {
            ERR("calloc() Fail()");
            break;
        }

        sockaddr_in *inet_addr = reinterpret_cast<sockaddr_in *>(addr);
        if (!inet_pton(AF_INET, host.c_str(), &inet_addr->sin_addr)) {
            ret = EINVAL;
            break;
        }

        inet_addr->sin_port = htons(port);
        inet_addr->sin_family = AF_INET;

        ret = connect(handle, addr, addrlen);
        if (ret < 0) {
            ERR("connect() Fail(%s, %d)", host.c_str(), port);
            break;
        }

        SetupOptions();
        return;
    } while (0);

    if (ret <= 0)
        ret = errno;

    free(addr);
    if (handle >= 0 && close(handle) < 0)
        ERR_CODE(errno, "close");
    throw std::runtime_error(strerror(ret));
}

TCP::TCP(int handle, sockaddr *addr, socklen_t szAddr) : handle(handle), addrlen(szAddr), addr(addr)
{
    SetupOptions();
}

TCP::~TCP(void)
{
    if (handle < 0)
        return;

    free(addr);
    if (close(handle) < 0)
        ERR_CODE(errno, "close");
}

void TCP::SetupOptions(void)
{
    int on = 1;

    int ret = setsockopt(handle, IPPROTO_IP, TCP_NODELAY, &on, sizeof(on));
    if (ret < 0) {
        ERR_CODE(errno, "delay option setting failed");
    }
}

void TCP::Send(const void *data, size_t &szData)
{
    int ret = send(handle, data, szData, 0);
    if (ret < 0) {
        ERR("Fail to send data, handle = %d, size = %zu", handle, szData);
        throw std::runtime_error(strerror(errno));
    }

    szData = ret;
}

void TCP::Recv(void *data, size_t &szData)
{
    int ret = recv(handle, data, szData, 0);
    if (ret < 0) {
        ERR("Fail to recv data, handle = %d, size = %zu", handle, szData);
        throw std::runtime_error(strerror(errno));
    }

    szData = ret;
}

int TCP::GetHandle(void)
{
    return handle;
}

void TCP::GetPeerInfo(std::string &host, unsigned short &port)
{
    char address[INET_ADDRSTRLEN] = {
          0,
    };

    if (!inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(this->addr)->sin_addr, address,
              sizeof(address)))
        throw std::runtime_error(strerror(errno));

    port = ntohs(reinterpret_cast<sockaddr_in *>(this->addr)->sin_port);
    host = address;
}

unsigned short TCP::GetPort(void)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(handle, reinterpret_cast<sockaddr *>(&addr), &addrlen) < 0)
        throw std::runtime_error(strerror(errno));

    return ntohs(addr.sin_port);
}
