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

RTSPClient::RTSPClient(void)
      : is_start(false), display_(nullptr), capture_interval_(2), capture_source_id_(0)
{
    player_create(&player_);
}

RTSPClient::~RTSPClient(void)
{
    player_state_e state = PLAYER_STATE_NONE;

    g_source_remove(capture_source_id_);

    player_get_state(player_, &state);
    if (state != PLAYER_STATE_IDLE) {
        player_unprepare(player_);
    }

    player_destroy(player_);
}

bool RTSPClient::IsStart(void)
{
    return is_start;
}

player_h RTSPClient::GetPlayer(void)
{
    return player_;
}

void RTSPClient::SetCaptureInterval(int interval)
{
    capture_interval_ = interval;
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

void RTSPClient::SetUri(const std::string &uri)
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
    player_h player = client->GetPlayer();

    int ret = player_capture_video(player, VideoCapturedCB, user_data);
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
    player_h player = client->GetPlayer();

    ret = player_start(player);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_start() Fail(%d)", ret);
        return;
    }

    client->capture_source_id_ = g_timeout_add_seconds(client->capture_interval_, TimeoutCB, data);
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
        capture_source_id_ = g_timeout_add_seconds(capture_interval_, TimeoutCB, this);
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
