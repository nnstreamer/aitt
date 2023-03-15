/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License"){
}
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
#include "RequestServer.h"

#include <chrono>
#include <future>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {

RequestServer::RequestServer() : stop_working_(false)
{
}

void RequestServer::Start(void)
{
    server_thread_ = std::thread(&RequestServer::ThreadMain, this);
}

void RequestServer::Stop(void)
{
    stop_working_ = true;
    if (server_thread_.joinable())
        server_thread_.join();
}

void RequestServer::PushBuffer(void *data, size_t data_len)
{
    if (!data || data_len == 0)
        return;

    if (sync_queue_.IsEmpty()) {
        auto frame = std::vector<unsigned char>(static_cast<unsigned char *>(data),
              static_cast<unsigned char *>(data) + data_len);

        sync_queue_.Push(frame);
    }
}

void RequestServer::ThreadMain(void)
{
    while (!stop_working_) {
        auto frame = sync_queue_.WaitedFront();
        if (frame.size() != 0 && on_frame_cb_)
            on_frame_cb_(&frame[0]);
        sync_queue_.Pop();
    }

    return;
}

void RequestServer::SetOnFrameCallback(OnFrameCallback cb)
{
    on_frame_cb_ = cb;
}
}  // namespace AittWebRTCNamespace
