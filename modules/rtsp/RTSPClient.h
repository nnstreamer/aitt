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

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <functional>
#include <mutex>
#include <string>

#include "RTSPFrame.h"

class RTSPClient {
  public:
    explicit RTSPClient(const std::string &url);
    ~RTSPClient(void);

    using StateCallback = std::function<void(void *user_data)>;
    using DataCallback = std::function<void(RTSPFrame &frame, void *user_data)>;

    void SetStateCallback(const StateCallback &cb, void *user_data);
    void SetDataCallback(const DataCallback &cb, void *user_data);
    void UnsetStateCallback();
    void UnsetClientCallback();
    int GetState();

    void Start();
    void Stop();

    void CreatePipeline();
    void DestroyPipeline(void);

  private:
    static void OnPadAddedCB(GstElement *element, GstPad *pad, gpointer data);
    static void VideoStreamDecodedCB(GstElement *object, GstBuffer *buffer, GstPad *pad,
          gpointer data);
    static gboolean MessageReceived(GstBus *bus, GstMessage *message, gpointer data);

    std::string url;
    GstElement *pipeline;

    StateCallback state_cb;
    void *state_cb_user_data;

    DataCallback data_cb;
    void *data_cb_user_data;

    std::mutex state_cb_lock;
    std::mutex data_cb_lock;

    int state;
};
