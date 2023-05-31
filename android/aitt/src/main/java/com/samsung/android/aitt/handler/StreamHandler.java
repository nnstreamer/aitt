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
package com.samsung.android.aitt.handler;

import android.content.Context;

import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.stream.AittStream;

public class StreamHandler implements ModuleHandler {

    /**
     * Method to create and return stream object
     * @param protocol Streaming protocol
     * @param topic String topic to which subscribe/publish is called
     * @param role Role of the stream object
     * @param context context of the application
     * @return returns stream object
     */
    AittStream newStreamModule(Aitt.Protocol protocol, String topic, AittStream.StreamRole role, Context context) throws InstantiationException {
        return null;
    }

    /**
     * Method to set application context to stream object
     * @param appContext application context
     */
    @Override
    public void setAppContext(Context appContext) {
        // //ToDo : Not needed for now
    }
}
