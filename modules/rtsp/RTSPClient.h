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

#include <mm_display_interface.h>
#include <player.h>

#include <functional>
#include <mutex>
#include <string>

#include "aitt_internal.h"

class RTSPClient {
  public:
    explicit RTSPClient(void);
    ~RTSPClient(void);

    using ReceiveCallback = std::function<void(void *obj, void *user_data)>;

    void SetReceiveCallback(const ReceiveCallback &cb, void *user_data);
    void UnsetReceiveCallback(void);
    bool IsStart(void);

    void SetDisplay(void *display);
    void SetUrl(const std::string &url);
    void Start(void);
    void Stop(void);

  private:
    static void PlayerPreparedCB(void *data);
    static void VideoStreamDecodedCB(media_packet_h packet, void *user_data);

    bool is_start;
    player_h player_;
    void *display_;
    mm_display_interface_h mm_display_;

    std::pair<ReceiveCallback, void *> receive_cb;

    std::mutex receive_cb_lock;
};
