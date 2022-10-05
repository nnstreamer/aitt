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

#include "RTSPFrame.h"

#include <gst/video/video-info.h>

#include "aitt_internal.h"

RTSPFrame::RTSPFrame(GstBuffer *buffer, GstPad *pad)
      : width(0), height(0), data(nullptr), data_size(0)
{
    GetDecodedInfoFromPad(buffer, pad);
}

void RTSPFrame::GetDecodedInfoFromPad(GstBuffer *buffer, GstPad *pad)
{
    RET_IF(pad == nullptr);

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (caps == nullptr) {
        ERR("caps is nullptr");
        return;
    }

    GstVideoInfo vinfo;
    if (!gst_video_info_from_caps(&vinfo, caps)) {
        gst_caps_unref(caps);
        ERR("failed to gst_video_info_from_caps()");
        return;
    }

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *string_format = gst_structure_get_string(structure, "format");
    if (string_format) {
        DBG("format %s", string_format);

        format = (std::string)string_format;
    }

    gst_caps_unref(caps);

    if (vinfo.width == 0 || vinfo.height == 0) {
        ERR("width, height is %d, %d", vinfo.width, vinfo.height);
        return;
    }

    width = vinfo.width;
    height = vinfo.height;
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    data = map.data;
    data_size = map.size;
}

int RTSPFrame::GetWidth(void) const
{
    return width;
}

int RTSPFrame::GetHeight(void) const
{
    return height;
}

int RTSPFrame::GetDataSize(void) const
{
    return data_size;
}

void *RTSPFrame::GetData(void) const
{
    return data;
}

std::string RTSPFrame::GetFormat(void)
{
    return format;
}
