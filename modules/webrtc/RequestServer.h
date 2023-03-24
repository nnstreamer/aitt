/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <functional>
#include <thread>
#include <vector>

#include "SyncQueue.h"

namespace AittWebRTCNamespace {
class RequestServer {
  public:
    using OnFrameCallback = std::function<void(void *)>;
    RequestServer();
    void Start(void);
    void Stop(void);
    void PushBuffer(void *data, size_t data_len);
    void ThreadMain(void);
    void SetOnFrameCallback(OnFrameCallback cb);

  private:
    bool stop_working_;
    std::thread server_thread_;
    SyncQueue<std::vector<unsigned char>> sync_queue_;
    OnFrameCallback on_frame_cb_;
};
}  // namespace AittWebRTCNamespace
