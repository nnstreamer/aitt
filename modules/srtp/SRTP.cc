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
#include "SRTP.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "log.h"

namespace aitt {

std::atomic<int> SRTP::initialized = {0};

SRTP::SRTP(std::unique_ptr<UDP> udp, const std::vector<uint8_t> &key, unsigned char TTL)
      : TTL(TTL),
        secret_key(key),
        srtp_ctx(nullptr),
        seq(0),
        ts(0),
        ssrc(0xdeadbeef),
        udp(std::move(udp))
{
    if (initialized == 0) {
        INFO("SRTP Version: %s [0x%x]", srtp_get_version_string(), srtp_get_version());
        srtp_err_status_t ret = srtp_init();
        if (ret)
            throw std::runtime_error("initialize the srtp - error code: " + std::to_string(ret));
    }
    initialized++;

    // NOTE:
    // Okay, all done. the socket is prepared.
    // Now, let's prepare the SRTP.
    srtp_policy_t policy;
    memset(static_cast<void *>(&policy), 0, sizeof(policy));

    DBG("Generated key length: %zd bytes", secret_key.size());

    srtp_crypto_policy_set_aes_gcm_256_8_auth(&policy.rtp);
    srtp_crypto_policy_set_aes_gcm_256_8_auth(&policy.rtcp);

    policy.ssrc.type = ssrc_specific;
    policy.ssrc.value = ssrc;
    policy.key = secret_key.data();
    policy.next = nullptr;
    policy.window_size = 128;
    policy.allow_repeat_tx = 0;
    policy.rtp.sec_serv = sec_serv_conf_and_auth;
    policy.rtcp.sec_serv = sec_serv_none;
    policy.rtp.auth_tag_len = 8;  // 8 or 16

    DBG("Required key length: %d bytes", policy.rtp.cipher_key_len);

    srtp_err_status_t status = srtp_create(&srtp_ctx, &policy);
    if (status)
        throw std::runtime_error("Failed to init: " + std::to_string(status));
}

SRTP::~SRTP(void)
{
    srtp_dealloc(srtp_ctx);

    initialized--;
    if (initialized == 0) {
        DBG("Finalize AITT");
        srtp_err_status_t ret = srtp_shutdown();
        if (ret)
            ERR("Unable to shutdown srtp with error: %d", ret);
    }
}

void SRTP::Send(const void *sendData, size_t &szSendData, const std::string &host,
      unsigned short port)
{
    srtp_err_status_t status;
    size_t pkt_len = szSendData + sizeof(message);

    message *msg = static_cast<message *>(calloc(1, pkt_len + SRTP_MAX_TRAILER_LEN));
    if (!msg)
        throw std::runtime_error("calloc: " + std::string(strerror(errno)));

    msg->header.ssrc = htonl(ssrc);
    msg->header.pt = 0x1;
    msg->header.version = 2;
    msg->header.seq = htons(++seq);
    msg->header.ts = htonl(ts++);

    memcpy(msg->payload, sendData, szSendData);

    // NOTE:
    // Cast to the signed int type from unsigned, should be carefully taken cared whether
    // it is not overflowed
    status = srtp_protect(srtp_ctx, &msg->header, reinterpret_cast<int *>(&pkt_len));
    if (status) {
        free(msg);
        throw std::runtime_error("srtp protection failed: " + std::to_string(status));
    }

    DBG("Packet size is updated to %zd from %zd", pkt_len, szSendData + sizeof(message));

    size_t sent_bytes = pkt_len;
    udp->Send(static_cast<void *>(msg), sent_bytes, host, port);
    DBG("UDP Packet is %s", (sent_bytes < pkt_len) ? "truncated" : "sent");

    free(msg);
}

void SRTP::Recv(void *recv_data, size_t &data_size, std::string *host, unsigned short *port)
{
    size_t pkt_len = sizeof(message) + data_size + SRTP_MAX_TRAILER_LEN;
    DBG("Recv %zd, data %zu, pktLen %zd", sizeof(message), data_size, pkt_len);

    message *msg = static_cast<message *>(calloc(1, pkt_len));
    if (!msg)
        throw std::runtime_error("calloc: " + std::string(strerror(errno)));

    size_t recv_bytes = pkt_len;
    udp->Recv(static_cast<void *>(msg), recv_bytes, host, port);
    DBG("UDP Packet is %s (%zd < %zd)", ((recv_bytes < pkt_len) ? "truncated" : "received"),
          recv_bytes, pkt_len);

    if (msg->header.version != 2) {
        free(msg);
        throw std::runtime_error("invalid version");
    }

    DBG("Sequence: %d", ntohs(msg->header.seq));
    DBG("TS: %d", ntohs(msg->header.ts));

    pkt_len = recv_bytes;
    srtp_err_status_t status =
          srtp_unprotect(srtp_ctx, &msg->header, reinterpret_cast<int *>(&pkt_len));
    if (status) {
        free(msg);
        throw std::runtime_error("unprotect failed with " + std::to_string(status));
    }
    DBG("Packet size is updated to %zd from %zd", pkt_len, recv_bytes);

    if (data_size > pkt_len - sizeof(msg->header)) {
        DBG("Payload size is resized to %zd from %zd", pkt_len - sizeof(msg->header), data_size);
        data_size = pkt_len - sizeof(msg->header);
    }
    memcpy(recv_data, msg->payload, data_size);
    free(msg);
}

int SRTP::GetHandle(void)
{
    return udp->GetHandle();
}

}  // namespace aitt
