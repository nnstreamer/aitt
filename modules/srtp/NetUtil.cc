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
#include "NetUtil.h"

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "log.h"

namespace aitt {

NetUtil::NetUtil(void) : handle(-1), need_to_close(true)
{
    handle = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (handle < 0)
        throw std::runtime_error(strerror(errno));
}

NetUtil::NetUtil(int handle) : handle(handle), need_to_close(false)
{
}

NetUtil::~NetUtil(void)
{
    if (handle < 0)
        return;

    if (need_to_close && close(handle) < 0)
        ERR_CODE(errno, "close");
}

int NetUtil::GetIfaceAddr(const std::string &iface, in_addr &addr)
{
    ifreq ifr;

    if (iface.length() > sizeof(ifr.ifr_name))
        return ENAMETOOLONG;

    strcpy(ifr.ifr_name, iface.c_str());

    if (ioctl(handle, SIOCGIFADDR, &ifr) < 0)
        return errno;

    addr = reinterpret_cast<sockaddr_in *>(&ifr.ifr_addr)->sin_addr;
    return 0;
}

int NetUtil::GetIfaceBroadcast(ifreq &req, std::string &address)
{
    char ipv4addr[INET_ADDRSTRLEN] = {
          0,
    };
    int ret = ioctl(handle, SIOCGIFBRDADDR, &req);
    if (ret < 0)
        return errno;

    sockaddr_in *in = reinterpret_cast<sockaddr_in *>(&req.ifr_broadaddr);
    if (!inet_ntop(AF_INET, &in->sin_addr, ipv4addr, sizeof(ipv4addr)))
        return EFAULT;

    address = ipv4addr;
    return 0;
}

int NetUtil::GetIfaceMAC(ifreq &req, std::string &mac)
{
    int ret = ioctl(handle, SIOCGIFHWADDR, &req);
    if (ret < 0)
        return errno;

    if (req.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
        char tmp_buf[18] = {
              0,
        };
        unsigned char *hwaddr = reinterpret_cast<unsigned char *>(req.ifr_hwaddr.sa_data);

        snprintf(tmp_buf, sizeof(tmp_buf), "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", hwaddr[0], hwaddr[1],
              hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

        mac = tmp_buf;
        ret = 0;
    } else if (req.ifr_hwaddr.sa_family == ARPHRD_LOOPBACK) {
        DBG("Loopback devices has no MAC address");
        ret = ENOENT;
    } else {
        DBG("sa_family: %d", req.ifr_hwaddr.sa_family);
        ret = ENOENT;
    }

    return ret;
}

int NetUtil::GetIfaceFlags(ifreq &req, short &flags)
{
    int ret = ioctl(handle, SIOCGIFFLAGS, &req);
    if (ret < 0)
        return errno;

    flags = req.ifr_flags;
    return 0;
}

void NetUtil::GetInterfaceList(std::vector<Interface> &list)
{
    ifconf v = {
          0,
    };

    int ret = ioctl(handle, SIOCGIFCONF, &v);
    if (ret < 0)
        throw std::runtime_error(strerror(errno));
    if (v.ifc_len < 0)
        throw std::runtime_error("Invalid information");
    if (!v.ifc_len)
        return;

    const int if_count = v.ifc_len / sizeof(*v.ifc_req);
    const int if_size = sizeof(*v.ifc_req);

    v.ifc_buf = static_cast<char *>(calloc(if_count, if_size));
    if (!v.ifc_buf)
        throw std::runtime_error(strerror(errno));

    ret = ioctl(handle, SIOCGIFCONF, &v);
    if (ret < 0) {
        free(v.ifc_buf);
        throw std::runtime_error(std::string("SIOCGIFCONF: ") + strerror(errno));
    }

    for (int i = 0; i < if_count; ++i) {
        Interface iface;
        char ipv4addr[INET_ADDRSTRLEN] = {
              0,
        };

        iface.name = v.ifc_req[i].ifr_name;

        sockaddr_in *in = reinterpret_cast<sockaddr_in *>(&v.ifc_req[i].ifr_addr);
        if (!inet_ntop(AF_INET, &in->sin_addr, ipv4addr, sizeof(ipv4addr))) {
            free(v.ifc_buf);
            list.clear();
            throw std::runtime_error("Invalid address found");
        }
        iface.address = ipv4addr;

        ret = GetIfaceBroadcast(v.ifc_req[i], iface.broadcast);
        if (ret)
            ERR_CODE(ret, "GetIfaceBroadcast");

        ret = GetIfaceMAC(v.ifc_req[i], iface.MAC);
        if (ret)
            ERR_CODE(ret, "GetIfaceMAC");

        short flags = 0;
        ret = GetIfaceFlags(v.ifc_req[i], flags);
        if (ret) {
            ERR_CODE(ret, "GetIfaceFlags");
        } else {
            iface.is_up = !!(flags & IFF_UP);
            iface.is_loopback = !!(flags && IFF_LOOPBACK);
            iface.broadcast_enabled = !!(flags & IFF_BROADCAST);
            iface.multicast_enabled = !!(flags & IFF_MULTICAST);
        }

        list.push_back(iface);
    }

    free(v.ifc_buf);
}

inline NetUtil::Interface::Interface(void)
      : name(),
        address(),
        broadcast(),
        MAC(),
        is_up(false),
        is_loopback(false),
        multicast_enabled(false),
        broadcast_enabled(false)
{
}

}  // namespace aitt
