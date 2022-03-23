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

#include <netinet/in.h>
#include <srtp2/srtp.h>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "UDP.h"

namespace aitt {

class SRTP {
  public:
    // TODO:
    // SRTP is able to run on the TCP too, so it is necessary, we may need consider the
    // common interface definition for the TCP and the UDP.
    SRTP(std::unique_ptr<UDP> udp, const std::vector<uint8_t> &key, unsigned char TTL = 5);
    virtual ~SRTP(void);

    void Send(const void *data, size_t &data_size, const std::string &host, unsigned short port);
    void Recv(void *data, size_t &data_size, std::string *host = nullptr,
          unsigned short *port = nullptr);

    int GetHandle(void);

  private:
    typedef struct {
        unsigned char cc : 4;      /* CSRC count             */
        unsigned char x : 1;       /* header extension flag  */
        unsigned char p : 1;       /* padding flag           */
        unsigned char version : 2; /* protocol version       */
        unsigned char pt : 7;      /* payload type           */
        unsigned char m : 1;       /* marker bit             */
        uint16_t seq;              /* sequence number        */
        uint32_t ts;               /* timestamp              */
        uint32_t ssrc;             /* synchronization source */
    } srtp_hdr_t;

    struct message {
        srtp_hdr_t header;
        unsigned char payload[1];  // NOTE: '1' for the android compiler compatibility
        // NOTE:
        // This message buffer must allocate more data for the SRTP tail message
    };

    static std::atomic<int> initialized;

    unsigned char TTL;
    std::vector<uint8_t> secret_key;
    srtp_ctx_t *srtp_ctx;
    uint16_t seq;
    uint32_t ts;
    uint32_t ssrc;
    std::unique_ptr<UDP> udp;
};

}  // namespace aitt
