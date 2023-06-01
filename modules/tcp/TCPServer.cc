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
#include "TCPServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <stdexcept>

#include "aitt_internal.h"

#define BACKLOG 10  // Accept only 10 simultaneously connections by default

namespace AittTCPNamespace {

TCP::Server::Server(const std::string &host, unsigned short &port, bool is_secure)
      : handle(-1), addr_(nullptr), addrlen_(0), secure(is_secure), key(), iv()
{
    int ret = 0;

    do {
        handle = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (handle < 0)
            break;

        addrlen_ = sizeof(sockaddr_in);
        addr_ = static_cast<sockaddr *>(calloc(1, sizeof(sockaddr_in)));
        if (!addr_)
            break;

        sockaddr_in *inet_addr = reinterpret_cast<sockaddr_in *>(addr_);
        if (!inet_pton(AF_INET, host.c_str(), &inet_addr->sin_addr)) {
            ret = EINVAL;
            break;
        }

        inet_addr->sin_port = htons(port);
        inet_addr->sin_family = AF_INET;

        int on = 1;
        ret = setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (ret < 0)
            break;

        ret = bind(handle, addr_, addrlen_);
        if (ret < 0)
            break;

        if (!port) {
            if (getsockname(handle, addr_, &addrlen_) < 0)
                break;
            port = ntohs(inet_addr->sin_port);
        }

        ret = listen(handle, BACKLOG);
        if (ret < 0)
            break;

        if (secure)
            AESEncryptor::GenerateKey(key, iv);

        return;
    } while (0);

    if (ret <= 0)
        ret = errno;

    free(addr_);

    if (handle >= 0 && close(handle) < 0)
        ERR_CODE(errno, "close");

    throw std::runtime_error(strerror(ret));
}

TCP::Server::~Server(void)
{
    if (handle < 0)
        return;

    free(addr_);
    if (close(handle) < 0)
        ERR_CODE(errno, "close");
}

std::unique_ptr<TCP> TCP::Server::AcceptPeer(void)
{
    sockaddr *peerAddr;
    socklen_t szAddr = sizeof(sockaddr_in);
    int peerHandle;

    peerAddr = static_cast<sockaddr *>(calloc(1, szAddr));
    if (!peerAddr)
        throw std::runtime_error(strerror(errno));

    peerHandle = accept(handle, peerAddr, &szAddr);
    if (peerHandle < 0) {
        free(peerAddr);
        throw std::runtime_error(strerror(errno));
    }
    ConnectInfo info;
    if (secure) {
        info.secure = true;
        memcpy(info.key, key, sizeof(key));
        memcpy(info.iv, iv, sizeof(iv));
    }
    return std::unique_ptr<TCP>(new TCP(peerHandle, peerAddr, szAddr, info));
}

int TCP::Server::GetHandle(void)
{
    return handle;
}

unsigned short TCP::Server::GetPort(void)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(handle, reinterpret_cast<sockaddr *>(&addr), &addrlen) < 0)
        throw std::runtime_error(strerror(errno));

    return ntohs(addr.sin_port);
}

const unsigned char *TCP::Server::GetCryptoKey(void)
{
    return key;
}

const unsigned char *TCP::Server::GetCryptoIv(void)
{
    return iv;
}

}  // namespace AittTCPNamespace
