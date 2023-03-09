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
      : is_start(false), player_(nullptr), display_(nullptr), mm_display_(nullptr)
{
    int ret;

    player_create(&player_);

    ret = player_set_media_packet_video_frame_decoded_cb(player_, VideoStreamDecodedCB, this);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_set_media_packet_video_frame_decoded_cb failed : %d", ret);
        return;
    }

    ret = mm_display_interface_init(&mm_display_);
    if (ret != 0) {
        ERR("mm_display_interface_init failed : %d", ret);
        return;
    }
}

RTSPClient::~RTSPClient(void)
{
    player_destroy(player_);
}

bool RTSPClient::IsStart(void)
{
    return is_start;
}

void RTSPClient::SetDisplay(void* display)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(display == nullptr);
    RET_IF(is_start == true);

    display_ = nullptr;

    ret = player_set_display(player_, PLAYER_DISPLAY_TYPE_NONE, nullptr);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_set_display failed: %d", ret);
        return;
    }

    ret = mm_display_interface_set_display(mm_display_, MM_DISPLAY_TYPE_EVAS, display, nullptr);
    if (ret != 0) {
        ERR("mm_display_interface_set_display failed: %d", ret);
        return;
    }

    ret = mm_display_interface_evas_set_mode(mm_display_, 0);
    if (ret != 0) {
        ERR("mm_display_interface_evas_set_mode failed: %d", ret);
        return;
    }
    ret = mm_display_interface_evas_set_rotation(mm_display_, 0);
    if (ret != 0) {
        ERR("mm_display_interface_evas_set_rotation failed: %d", ret);
        return;
    }
    ret = mm_display_interface_evas_set_visible(mm_display_, true);
    if (ret != 0) {
        ERR("mm_display_interface_evas_set_visible failed: %d", ret);
        return;
    }

    display_ = display;
}

void RTSPClient::SetUrl(const std::string& url)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(url.empty());
    RET_IF(is_start == true);

    ret = player_set_uri(player_, url.c_str());
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_set_uri failed: %d", ret);
        return;
    }
}

void RTSPClient::VideoStreamDecodedCB(media_packet_h packet, void* user_data)
{
    if (!packet || !user_data) {
        ERR("invalid param [packet:%p, user_data:%p]", packet, user_data);
        return;
    }

    RTSPClient* player = static_cast<RTSPClient*>(user_data);

    // Todo : Handle the copied media packet in another thread
    if (player->receive_cb.first != nullptr) {
        player->receive_cb.first(static_cast<void*>(packet), player->receive_cb.second);
    }

    if (player->display_ != nullptr) {
        mm_display_interface_evas_render(player->mm_display_, packet);
    }
}

void RTSPClient::PlayerPreparedCB(void* data)
{
    int ret = PLAYER_ERROR_NONE;

    player_h player = static_cast<player_h>(data);

    ret = player_start(player);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_prepare_async failed: %d", ret);
        return;
    }
}

void RTSPClient::Start(void)
{
    int ret = PLAYER_ERROR_NONE;

    RET_IF(player_ == nullptr);
    RET_IF(is_start == true);
    RET_IF(display_ == nullptr);

    ret = player_prepare_async(player_, PlayerPreparedCB, player_);
    if (ret != PLAYER_ERROR_NONE) {
        ERR("player_prepare_async failed: %d", ret);
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
        ERR("player_stop failed: %d", ret);
        return;
    }
}

void RTSPClient::SetReceiveCallback(const ReceiveCallback& cb, void* user_data)
{
    std::lock_guard<std::mutex> auto_lock(receive_cb_lock);

    receive_cb = std::make_pair(cb, user_data);
}

void RTSPClient::UnsetReceiveCallback()
{
    std::lock_guard<std::mutex> auto_lock(receive_cb_lock);

    receive_cb = std::make_pair(nullptr, nullptr);
}
