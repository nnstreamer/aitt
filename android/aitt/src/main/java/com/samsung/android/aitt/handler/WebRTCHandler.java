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

import static com.samsung.android.aitt.stream.WebRTCStream.createPublisherStream;
import static com.samsung.android.aitt.stream.WebRTCStream.createSubscriberStream;

import android.content.Context;
import android.util.Log;

import com.samsung.android.aitt.Aitt;

import com.samsung.android.aitt.stream.AittStream;

import java.security.InvalidParameterException;

public final class WebRTCHandler extends StreamHandler {

    private static final String TAG = "WebRTCHandler";

    @Override
    public void setAppContext(Context context) {
    }

    @Override
    public AittStream newStreamModule(Aitt.Protocol protocol, String topic, AittStream.StreamRole role, Context context) {
        if (protocol != Aitt.Protocol.WEBRTC)
            throw new InvalidParameterException("Invalid protocol");

        try {
            if (role == AittStream.StreamRole.SUBSCRIBER) {
                return createSubscriberStream(topic, role, context);
            } else {
                return createPublisherStream(topic, role, context);
            }
        } catch (Exception e) {
            Log.e(TAG, "Fail to create an AittStream instance.");
        }

        return null;
    }
}
