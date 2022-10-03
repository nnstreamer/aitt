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

#include <camera.h>

#include <functional>

class CameraHandler {
  public:
    using MediaPacketPreviewCallback = std::function<void(media_packet_h, void *)>;

    ~CameraHandler();
    int Init(const MediaPacketPreviewCallback &preview_cb, void *user_data);
    void Deinit(void);
    int StartPreview(void);
    int StopPreview(void);

    static const char *ErrorToString(const int error);
    static const char *StateToString(const camera_state_e state);

  private:
    void SettingCamera(const MediaPacketPreviewCallback &preview_cb, void *user_data);
    static void CameraPreviewCB(media_packet_h media_packet, void *user_data);

    camera_h handle_;
    bool is_started_;
    MediaPacketPreviewCallback media_packet_preview_cb_;
    void *user_data_;
};
