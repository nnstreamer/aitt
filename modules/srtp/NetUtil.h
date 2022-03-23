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
#pragma once

#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>
#include <vector>

namespace aitt {

class NetUtil {
  public:
    struct Interface {
        Interface(void);
        virtual ~Interface(void) = default;

        std::string name;
        std::string address;
        std::string broadcast;
        std::string MAC;

        bool is_up;
        bool is_loopback;
        bool multicast_enabled;
        bool broadcast_enabled;
    };

    NetUtil(void);
    // NOTE:
    // The "handle" will not be closed by the NetUtil
    explicit NetUtil(int handle);

    virtual ~NetUtil(void);

    void GetInterfaceList(std::vector<Interface> &list);
    int GetIfaceAddr(const std::string &iface, in_addr &addr);

  private:
    int GetIfaceBroadcast(ifreq &req, std::string &address);
    int GetIfaceMAC(ifreq &req, std::string &mac);
    int GetIfaceFlags(ifreq &req, short &flags);

    int handle;
    bool need_to_close;
};

}  // namespace aitt
