/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <glib.h>
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
    using StateCallback = std::function<void(int state, void *user_data)>;

    void SetReceiveCallback(const ReceiveCallback &cb, void *user_data);
    void UnsetReceiveCallback(void);
    void SetStateCallback(const StateCallback &cb, void *user_data);
    void UnsetStateCallback(void);
    bool IsStart(void);
    void SetDisplay(void *display);
    void SetURI(const std::string &uri);
    void SetFPS(int fps);
    void Start(void);
    void Stop(void);

  private:
    static void PlayerPreparedCB(void *user_data);
    static void VideoCapturedCB(unsigned char *frame, int width, int height, unsigned int size,
          void *user_data);
    static void PlayerInterruptedCB(player_interrupted_code_e interrupted_code, void *user_data);
    static gboolean TimeoutCB(gpointer user_data);

    bool is_start;
    player_h player_;
    void *display_;
    int fps_;
    guint capture_source_id_;

    std::pair<ReceiveCallback, void *> receive_cb;
    std::pair<StateCallback, void *> state_cb;
};
