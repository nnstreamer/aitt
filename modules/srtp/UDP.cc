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
#include "UDP.h"

#include <arpa/inet.h>
#include <log.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "NetUtil.h"

namespace aitt {

UDP::UDP(const std::string &host, unsigned short &port) : UDP()
{
    int ret = 0;

    do {
        addrlen = sizeof(sockaddr_in);
        addr = static_cast<sockaddr *>(calloc(1, addrlen));
        if (!addr) {
            ret = errno;
            break;
        }

        sockaddr_in *in = reinterpret_cast<sockaddr_in *>(addr);
        if (!inet_pton(AF_INET, host.c_str(), &in->sin_addr)) {
            ret = EINVAL;
            break;
        }

        in->sin_port = htons(port);
        in->sin_family = AF_INET;

        int on = 1;
        ret = setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (ret < 0)
            ERR_CODE(errno, "setsockopt");

        ret = bind(handle, addr, addrlen);
        if (ret < 0) {
            ret = errno;
            break;
        }

        if (!port) {
            if (getsockname(handle, addr, &addrlen) < 0) {
                ret = errno;
                break;
            }
            port = ntohs(in->sin_port);
        }

        return;
    } while (0);

    if (handle >= 0 && close(handle) < 0)
        ERR_CODE(errno, "close");
    free(addr);
    throw std::runtime_error(strerror(ret));
}

UDP::UDP(void) : handle(-1), addr(nullptr), addrlen(0)
{
    int on = 1;
    int off = 0;

    handle = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (handle < 0) {
        free(addr);
        addr = nullptr;
        throw std::runtime_error(strerror(errno));
    }

    int ret = setsockopt(handle, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    if (ret < 0)
        ERR_CODE(errno, "setsockopt");

    ret = setsockopt(handle, IPPROTO_IP, IP_MULTICAST_LOOP, &off, sizeof(off));
    if (ret < 0)
        ERR_CODE(errno, "setsockopt");
}

UDP::~UDP(void)
{
    if (handle < 0)
        return;

    if (close(handle) < 0)
        ERR_CODE(errno, "close");
    free(addr);
}

void UDP::Send(const void *data, size_t &szData, const std::string &host, unsigned short port)
{
    sockaddr_in inAddr;
    socklen_t szAddr = sizeof(sockaddr_in);

    if (!inet_pton(AF_INET, host.c_str(), &inAddr.sin_addr))
        throw std::runtime_error("Invalid address" + host);

    inAddr.sin_port = htons(port);
    inAddr.sin_family = AF_INET;

    int ret = sendto(handle, data, szData, 0, reinterpret_cast<sockaddr *>(&inAddr), szAddr);
    if (ret < 0)
        throw std::runtime_error(strerror(errno));

    szData = ret;
}

void UDP::Recv(void *data, size_t &szData, std::string *host, unsigned short *port)
{
    sockaddr_in inAddr;
    socklen_t szAddr = sizeof(sockaddr_in);

    int ret = recvfrom(handle, data, szData, 0, reinterpret_cast<sockaddr *>(&inAddr), &szAddr);
    if (ret < 0)
        throw std::runtime_error(strerror(errno));
    szData = ret;

    if (host) {
        char tmpBuffer[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &inAddr.sin_addr, tmpBuffer, sizeof(tmpBuffer)))
            throw std::runtime_error("Invalid address parsed" + std::string(tmpBuffer));

        *host = tmpBuffer;
    }

    if (port)
        *port = ntohs(inAddr.sin_port);
}

int UDP::GetHandle(void)
{
    return handle;
}

void UDP::JoinMulticastGroup(const std::string &peer, const std::string *iface, std::string *source)
{
    if (source) {
        ip_mreq_source mreq = {
              0,
        };
        int ret = GetMulticastSourceRequest(peer, *source, mreq, iface);
        if (ret)
            throw std::runtime_error(std::string("error: ") + strerror(ret));

        ret = setsockopt(handle, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &mreq, sizeof(mreq));
        if (ret < 0)
            throw std::runtime_error(strerror(errno));
    } else {
        ip_mreqn mreq = {
              0,
        };
        int ret = GetMulticastRequest(peer, mreq, iface);
        if (ret != 0)
            throw std::runtime_error(std::string("GetMulticastRequest error: ") + strerror(ret));

        ret = setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if (ret < 0)
            throw std::runtime_error(strerror(errno));
    }
}

void UDP::LeaveMulticastGroup(const std::string &peer, const std::string *iface,
      std::string *source)
{
    if (source) {
        ip_mreq_source mreq;
        int ret = GetMulticastSourceRequest(peer, *source, mreq, iface);
        if (ret)
            throw std::runtime_error(strerror(ret));

        ret = setsockopt(handle, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, &mreq, sizeof(mreq));
        if (ret < 0)
            throw std::runtime_error(strerror(errno));
    } else {
        ip_mreqn mreq;
        int ret = GetMulticastRequest(peer, mreq, iface);
        if (ret)
            throw std::runtime_error(strerror(ret));

        ret = setsockopt(handle, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        if (ret < 0)
            throw std::runtime_error(strerror(errno));
    }
}

void UDP::SetMulticastInterface(const std::string &iface)
{
    ip_mreqn mreq;

    if (iface.empty()) {
        mreq.imr_address.s_addr = htonl(INADDR_ANY);
        mreq.imr_ifindex = 0;
    } else if (!inet_pton(AF_INET, iface.c_str(), &mreq.imr_address)) {
        mreq.imr_address.s_addr = htonl(INADDR_ANY);
        mreq.imr_ifindex = if_nametoindex(iface.c_str());
        if (!mreq.imr_ifindex)
            throw std::runtime_error(std::string("Invalid iface: ") + strerror(errno));
    } else {
        mreq.imr_ifindex = 0;
    }

    int ret = setsockopt(handle, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq));
    if (ret < 0)
        throw std::runtime_error(strerror(errno));
}

int UDP::GetMulticastRequest(const std::string &peer, ip_mreqn &mreq, const std::string *iface)
{
    if (!inet_pton(AF_INET, peer.c_str(), &mreq.imr_multiaddr))
        return EINVAL;
    if (!IN_MULTICAST(htonl(mreq.imr_multiaddr.s_addr)))
        return EINVAL;

    if (!iface || iface->empty()) {
        mreq.imr_ifindex = 0;
        mreq.imr_address.s_addr = htonl(INADDR_ANY);
    } else if (!inet_pton(AF_INET, iface->c_str(), &mreq.imr_address)) {
        mreq.imr_address.s_addr = htonl(INADDR_ANY);
        mreq.imr_ifindex = if_nametoindex(iface->c_str());
        if (!mreq.imr_ifindex)
            return errno;
    } else {
        mreq.imr_ifindex = 0;
    }

    return 0;
}

int UDP::GetMulticastSourceRequest(const std::string &peer, const std::string &source,
      ip_mreq_source &mreq, const std::string *iface)
{
    if (!inet_pton(AF_INET, peer.c_str(), &mreq.imr_multiaddr))
        return EINVAL;
    if (!IN_MULTICAST(htonl(mreq.imr_multiaddr.s_addr)))
        return EINVAL;

    if (!iface || iface->empty()) {
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    } else if (!inet_pton(AF_INET, iface->c_str(), &mreq.imr_interface)) {
        NetUtil netUtil(handle);
        int ret = netUtil.GetIfaceAddr(*iface, mreq.imr_interface);
        if (ret)
            return ret;
    }

    if (source.empty())
        mreq.imr_sourceaddr.s_addr = htonl(INADDR_ANY);
    else if (!inet_pton(AF_INET, source.c_str(), &mreq.imr_sourceaddr))
        return EINVAL;

    return 0;
}

}  // namespace aitt
