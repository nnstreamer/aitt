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

#include "RTSPClient.h"

#define MSEC_PER_SECOND 1000

RTSPClient::RTSPClient(void) : is_start(false), display_(nullptr), fps_(2), capture_source_id_(0)
{
    int ret = PLAYER_ERROR_NONE;

    ret = player_create(&player_);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_create() Fail(%d)", ret);
        throw std::runtime_error("player_create() Fail");
    }
}

RTSPClient::~RTSPClient(void)
{
    int ret = PLAYER_ERROR_NONE;

    g_source_remove(capture_source_id_);

    ret = player_destroy(player_);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_destroy() Fail(%d)", ret);
    }
}

bool RTSPClient::IsStart(void)
{
    return is_start;
}

void RTSPClient::SetFPS(int fps)
{
    fps_ = fps;
}

void RTSPClient::SetDisplay(void *display)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(display == nullptr);
    RET_IF(is_start == true);

    display_ = nullptr;

    ret = player_set_display(player_, PLAYER_DISPLAY_TYPE_EVAS, display);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_set_display() Fail(%d)", ret);
        return;
    }

    display_ = display;
}

void RTSPClient::SetURI(const std::string &uri)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(uri.empty());
    RET_IF(is_start == true);

    ret = player_set_uri(player_, uri.c_str());
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_set_uri(%s) Fail(%d)", uri.c_str(), ret);
        return;
    }
}

void RTSPClient::VideoCapturedCB(unsigned char *frame, int width, int height, unsigned int size,
      void *user_data)
{
    if (!frame || !user_data) {
        ERR("invalid param [frame:%p, user_data:%p]", frame, user_data);
        return;
    }

    RTSPClient *client = static_cast<RTSPClient *>(user_data);

    if (client->receive_cb.first != nullptr) {
        client->receive_cb.first(frame, client->receive_cb.second);
    }
}

gboolean RTSPClient::TimeoutCB(gpointer user_data)
{
    if (!user_data) {
        ERR("invalid param [user_data:%p]", user_data);
        return FALSE;
    }

    RTSPClient *client = static_cast<RTSPClient *>(user_data);
    if (client->player_ == nullptr) {
        ERR("player handle is nullptr");
        return FALSE;
    }

    int ret = player_capture_video(client->player_, VideoCapturedCB, user_data);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_capture_video() Fail(%d)", ret);
        return FALSE;
    }

    return TRUE;
}

void RTSPClient::PlayerPreparedCB(void *data)
{
    int ret = PLAYER_ERROR_NONE;

    RTSPClient *client = static_cast<RTSPClient *>(data);
    if (client->player_ == nullptr) {
        ERR("player handle is nullptr");
        return;
    }

    ret = player_start(client->player_);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_start() Fail(%d)", ret);
        return;
    }

    client->capture_source_id_ = g_timeout_add(MSEC_PER_SECOND / client->fps_, TimeoutCB, data);
}

void RTSPClient::Start(void)
{
    int ret = PLAYER_ERROR_NONE;
    player_state_e state = PLAYER_STATE_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(is_start == true);
    RET_IF(display_ == nullptr);

    ret = player_get_state(player_, &state);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_get_state() Fail(%d)", ret);
        return;
    }

    switch (state) {
    case PLAYER_STATE_IDLE:
        ret = player_prepare_async(player_, PlayerPreparedCB, this);
        if (ret != PLAYER_ERROR_NONE) {
            ERR("player_prepare_async() Fail(%d)", ret);
            return;
        }
        break;
    case PLAYER_STATE_READY:
    case PLAYER_STATE_PAUSED:
        ret = player_start(player_);
        if (ret != PLAYER_ERROR_NONE) {
            ERR("player_start() Fail(%d)", ret);
            return;
        }
        capture_source_id_ = g_timeout_add(MSEC_PER_SECOND / fps_, TimeoutCB, this);
        break;
    default:
        ERR("Invalid State : %d", state);
        return;
    }

    is_start = true;
}

void RTSPClient::Stop(void)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(is_start == false);

    is_start = false;

    ret = player_stop(player_);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_stop() Fail(%d)", ret);
        return;
    }
}

void RTSPClient::SetReceiveCallback(const ReceiveCallback &cb, void *user_data)
{
    std::lock_guard<std::mutex> auto_lock(receive_cb_lock);

    receive_cb = std::make_pair(cb, user_data);
}

void RTSPClient::UnsetReceiveCallback()
{
    std::lock_guard<std::mutex> auto_lock(receive_cb_lock);

    receive_cb = std::make_pair(nullptr, nullptr);
}
