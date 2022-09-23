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
#include "CameraHandler.h"

#include "aitt_internal.h"

#define RETURN_DEFINED_NAME_AS_STRING(defined_constant) \
    case defined_constant:                              \
        return #defined_constant;

CameraHandler::~CameraHandler(void)
{
    if (handle_) {
        camera_state_e state = CAMERA_STATE_NONE;

        int ret = camera_get_state(handle_, &state);
        if (ret != CAMERA_ERROR_NONE) {
            ERR("camera_get_state() Fail(%s)", ErrorToString(ret));
        }

        if (state == CAMERA_STATE_PREVIEW) {
            INFO("CameraHandler preview is not stopped (stop)");
            ret = camera_stop_preview(handle_);
            if (ret != CAMERA_ERROR_NONE) {
                ERR("camera_stop_preview() Fail(%s)", ErrorToString(ret));
            }
        }
    }

    if (handle_)
        camera_destroy(handle_);
}

CameraHandler::CameraHandler()
      : handle_(nullptr), is_started_(false), media_packet_preview_cb_(nullptr), user_data_(nullptr)
{
}

int CameraHandler::Init(const MediaPacketPreviewCallback &preview_cb, void *user_data)
{
    int ret = camera_create(CAMERA_DEVICE_CAMERA0, &handle_);
    if (ret != CAMERA_ERROR_NONE) {
        ERR("camera_create() Fail(%s)", ErrorToString(ret));
        return -1;
    }
    SettingCamera(preview_cb, user_data);

    return 0;
}

void CameraHandler::SettingCamera(const MediaPacketPreviewCallback &preview_cb, void *user_data)
{
    int ret = camera_set_media_packet_preview_cb(handle_, CameraPreviewCB, this);
    if (ret != CAMERA_ERROR_NONE) {
        ERR("camera_set_media_packet_preview_cb() Fail(%s)", ErrorToString(ret));
        return;
    }
    media_packet_preview_cb_ = preview_cb;
    user_data_ = user_data;
}

void CameraHandler::Deinit(void)
{
    if (!handle_) {
        ERR("Handler is nullptr");
        return;
    }

    is_started_ = false;
    media_packet_preview_cb_ = nullptr;
    user_data_ = nullptr;
}

int CameraHandler::StartPreview(void)
{
    camera_state_e state;
    int ret = camera_get_state(handle_, &state);
    if (ret != CAMERA_ERROR_NONE) {
        ERR("camera_get_state() Fail(%s)", ErrorToString(ret));
        return -1;
    }

    if (state == CAMERA_STATE_PREVIEW) {
        INFO("Preview is already started");
        is_started_ = true;
        return 0;
    }

    ret = camera_start_preview(handle_);
    if (ret != CAMERA_ERROR_NONE) {
        ERR("camera_start_preview() Fail(%s)", ErrorToString(ret));
        return -1;
    }

    is_started_ = true;

    return 0;
}

int CameraHandler::StopPreview(void)
{
    RETV_IF(handle_ == nullptr, -1);
    is_started_ = false;

    return 0;
}

void CameraHandler::CameraPreviewCB(media_packet_h media_packet, void *user_data)
{
    auto camera_handler = static_cast<CameraHandler *>(user_data);
    if (!camera_handler) {
        ERR("Invalid user_data");
        return;
    }

    if (!camera_handler->is_started_) {
        ERR("Preveiw is not started yet");
        return;
    }

    if (!camera_handler->media_packet_preview_cb_) {
        ERR("Preveiw cb is not set");
        return;
    }

    camera_handler->media_packet_preview_cb_(media_packet, camera_handler->user_data_);
}

const char *CameraHandler::ErrorToString(const int error)
{
    switch (error) {
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_NONE)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_INVALID_PARAMETER)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_INVALID_STATE)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_OUT_OF_MEMORY)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_DEVICE)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_INVALID_OPERATION)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_SECURITY_RESTRICTED)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_DEVICE_BUSY)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_DEVICE_NOT_FOUND)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_ESD)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_PERMISSION_DENIED)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_NOT_SUPPORTED)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_RESOURCE_CONFLICT)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_ERROR_SERVICE_DISCONNECTED)
    }

    return "Unknown error";
}

const char *CameraHandler::StateToString(const camera_state_e state)
{
    switch (state) {
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_STATE_NONE)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_STATE_CREATED)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_STATE_PREVIEW)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_STATE_CAPTURING)
        RETURN_DEFINED_NAME_AS_STRING(CAMERA_STATE_CAPTURED)
    }

    return "Unknown state";
}
