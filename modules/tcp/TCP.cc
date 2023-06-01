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
#include "TCP.h"

#include <AittTypes.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "aitt_internal.h"

namespace AittTCPNamespace {

TCP::TCP(const std::string &host, const ConnectInfo &connect_info)
      : handle_(-1), addrlen_(0), addr_(nullptr), secure(false)
{
    int ret = 0;

    do {
        if (connect_info.port == 0) {
            ret = EINVAL;
            break;
        }

        handle_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (handle_ < 0) {
            ERR("socket() Fail()");
            break;
        }

        addrlen_ = sizeof(sockaddr_in);
        addr_ = static_cast<sockaddr *>(calloc(1, addrlen_));
        if (!addr_) {
            ERR("calloc() Fail()");
            break;
        }

        sockaddr_in *inet_addr = reinterpret_cast<sockaddr_in *>(addr_);
        if (!inet_pton(AF_INET, host.c_str(), &inet_addr->sin_addr)) {
            ret = EINVAL;
            break;
        }

        inet_addr->sin_port = htons(connect_info.port);
        inet_addr->sin_family = AF_INET;

        ret = connect(handle_, addr_, addrlen_);
        if (ret < 0) {
            ERR("connect() Fail(%s, %d)", host.c_str(), connect_info.port);
            break;
        }

        SetupOptions(connect_info);
        return;
    } while (0);

    if (ret <= 0)
        ERR_CODE(errno, "TCP::TCP(%d) Fail", ret);

    free(addr_);
    if (0 <= handle_ && close(handle_) < 0)
        ERR_CODE(errno, "close");
    throw std::runtime_error("TCP::TCP() Fail");
}

TCP::TCP(int handle, sockaddr *addr, socklen_t szAddr, const ConnectInfo &connect_info)
      : handle_(handle), addrlen_(szAddr), addr_(addr), secure(false)
{
    SetupOptions(connect_info);
}

TCP::~TCP(void)
{
    if (handle_ < 0)
        return;

    free(addr_);
    if (close(handle_) < 0)
        ERR_CODE(errno, "close");
}

void TCP::SetupOptions(const ConnectInfo &connect_info)
{
    int on = 1;

    int ret = setsockopt(handle_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    if (ret < 0) {
        ERR_CODE(errno, "delay option setting failed");
    }

    if (connect_info.secure) {
        secure = true;
        crypto.Init(connect_info.key, connect_info.iv);
    }
}

int32_t TCP::Send(const void *data, int32_t data_size)
{
    int32_t sent = 0;
    while (sent < data_size) {
        int ret = send(handle_, static_cast<const char *>(data) + sent, data_size - sent, 0);
        if (ret < 0) {
            ERR("send(%d, %d) Fail(%d)", handle_, data_size, errno);
            throw std::runtime_error("send() Fail");
        }

        sent += ret;
    }
    return sent;
}

void TCP::SendSizedData(const void *data, int32_t data_size)
{
    RET_IF(data_size < 0);

    if (secure)
        return SendSizedDataSecure(data, data_size);
    else
        return SendSizedDataNormal(data, data_size);
}

int32_t TCP::Recv(void *data, int32_t data_size)
{
    int32_t received = 0;
    while (received < data_size) {
        int ret = recv(handle_, static_cast<char *>(data) + received, data_size - received, 0);
        if (ret < 0) {
            ERR("recv(%d, %d) Fail(%d)", handle_, data_size, errno);
            return -1;
        }
        if (ret == 0) {
            ERR("disconnected");
            return -ENOTCONN;
        }

        received += ret;
    }

    return received;
}

int32_t TCP::RecvSizedData(void **data)
{
    if (secure)
        return RecvSizedDataSecure(data);
    else
        return RecvSizedDataNormal(data);
}

int32_t TCP::HandleZeroMsg(void **data)
{
    // distinguish between connection problems and zero-size messages
    INFO("Got a zero-size message.");
    *data = nullptr;
    return 0;
}

int TCP::GetHandle(void)
{
    return handle_;
}

void TCP::GetPeerInfo(std::string &host, unsigned short &port)
{
    char address[INET_ADDRSTRLEN] = {
          0,
    };

    if (!inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in *>(this->addr_)->sin_addr, address,
              sizeof(address)))
        throw std::runtime_error("inet_ntop() Fail");

    port = ntohs(reinterpret_cast<sockaddr_in *>(this->addr_)->sin_port);
    host = address;
}

unsigned short TCP::GetPort(void)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (getsockname(handle_, reinterpret_cast<sockaddr *>(&addr), &addrlen) < 0)
        throw std::runtime_error("getsockname() Fail");

    return ntohs(addr.sin_port);
}

void TCP::SendSizedDataNormal(const void *data, int32_t data_size)
{
    int32_t fixed_data_size = data_size;
    if (0 == data_size) {
        // distinguish between connection problems and zero-size messages
        INFO("Send a zero-size message.");
        fixed_data_size = INT32_MAX;
    }

    int32_t size_len = sizeof(fixed_data_size);
    Send(static_cast<const void *>(&fixed_data_size), size_len);
    Send(data, data_size);
}

int32_t TCP::RecvSizedDataNormal(void **data)
{
    int32_t result;

    int32_t data_len = 0;
    int32_t size_len = sizeof(data_len);
    result = Recv(static_cast<void *>(&data_len), size_len);
    if (result < 0) {
        ERR("Recv() Fail(%d)", result);
        return result;
    }

    if (data_len == INT32_MAX)
        return HandleZeroMsg(data);

    if (AITT_MESSAGE_MAX < data_len) {
        ERR("Invalid Size(%d)", data_len);
        return -1;
    }
    void *data_buf = malloc(data_len);
    Recv(data_buf, data_len);
    if (data_len < 0) {
        ERR("Recv() Fail(%d)", data_len);
        free(data_buf);
    } else {
        *data = data_buf;
    }

    return data_len;
}

void TCP::SendSizedDataSecure(const void *data, int32_t data_size)
{
    int32_t fixed_data_size = data_size;
    if (0 == data_size) {
        // distinguish between connection problems and zero-size messages
        INFO("Send a zero-size message.");
        fixed_data_size = INT32_MAX;
    }

    int32_t size_len;
    unsigned char *size_buf =
          static_cast<unsigned char *>(malloc(crypto.GetCryptogramSize(sizeof(int32_t))));

    if (data_size) {
        unsigned char *data_buf =
              static_cast<unsigned char *>(malloc(crypto.GetCryptogramSize(data_size)));
        int32_t data_len =
              crypto.Encrypt(static_cast<const unsigned char *>(data), data_size, data_buf);
        size_len = crypto.Encrypt((unsigned char *)&data_len, sizeof(data_len), size_buf);
        Send(size_buf, size_len);
        Send(data_buf, data_len);
        free(data_buf);
    } else {
        size_len =
              crypto.Encrypt((unsigned char *)&fixed_data_size, sizeof(fixed_data_size), size_buf);
        Send(size_buf, size_len);
    }
    free(size_buf);
}

int32_t TCP::RecvSizedDataSecure(void **data)
{
    int32_t result;

    int32_t cipher_size_len = crypto.GetCryptogramSize(sizeof(int32_t));
    unsigned char *cipher_size_buf = static_cast<unsigned char *>(malloc(cipher_size_len));
    result = Recv(cipher_size_buf, cipher_size_len);
    if (result < 0) {
        ERR("Recv() Fail(%d)", result);
        free(cipher_size_buf);
        return result;
    }

    unsigned char *plain_size_buf = static_cast<unsigned char *>(malloc(cipher_size_len));
    int32_t cipher_data_len = 0;
    crypto.Decrypt(cipher_size_buf, cipher_size_len, plain_size_buf);
    memcpy(&cipher_data_len, plain_size_buf, sizeof(cipher_data_len));
    free(plain_size_buf);
    free(cipher_size_buf);
    if (cipher_data_len == INT32_MAX)
        return HandleZeroMsg(data);

    if (AITT_MESSAGE_MAX < cipher_data_len) {
        ERR("Invalid Size(%d)", cipher_data_len);
        return -1;
    }
    unsigned char *cipher_data_buf = static_cast<unsigned char *>(malloc(cipher_data_len));
    Recv(cipher_data_buf, cipher_data_len);
    unsigned char *data_buf = static_cast<unsigned char *>(malloc(cipher_data_len));
    result = crypto.Decrypt(cipher_data_buf, cipher_data_len, data_buf);
    if (result < 0) {
        ERR("Decrypt() Fail(%d)", result);
        free(data_buf);
    } else {
        *data = data_buf;
    }
    free(cipher_data_buf);
    return result;
}

TCP::ConnectInfo::ConnectInfo() : port(0), num_of_cb(0), secure(false), key(), iv()
{
}

}  // namespace AittTCPNamespace
