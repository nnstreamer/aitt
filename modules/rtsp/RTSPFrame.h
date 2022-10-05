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

#include <string>

class RTSPFrame {
  public:
    RTSPFrame() = default;
    RTSPFrame(GstBuffer *buffer, GstPad *pad);

    int GetWidth(void) const;
    int GetHeight(void) const;
    int GetDataSize(void) const;
    void *GetData(void) const;
    std::string GetFormat(void);

  private:
    int width;
    int height;
    void *data;
    int data_size;
    std::string format;

    void GetDecodedInfoFromPad(GstBuffer *buffer, GstPad *pad);
};
