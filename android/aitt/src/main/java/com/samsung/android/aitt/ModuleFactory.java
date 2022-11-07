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
package com.samsung.android.aitt;

import com.samsung.android.aitt.handler.IpcHandler;
import com.samsung.android.aitt.handler.ModuleHandler;
import com.samsung.android.aitt.handler.RTSPHandler;
import com.samsung.android.aitt.handler.WebRTCHandler;

class ModuleFactory {

    public static ModuleHandler createModuleHandler(Aitt.Protocol protocol) {
        ModuleHandler moduleHandler;
        switch (protocol) {
            case WEBRTC:
                moduleHandler = new WebRTCHandler();
                break;
            case IPC:
                moduleHandler = new IpcHandler();
                break;
            case RTSP:
                moduleHandler = new RTSPHandler();
                break;
            default:
                moduleHandler = null;
        }
        return moduleHandler;
    }
}
