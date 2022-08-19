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

import android.content.Context;

class IpcHandler implements TransportHandler {

    @Override
    public void setAppContext(Context appContext) {
        //ToDo : Implement setAppContext method
    }

    @Override
    public void setSelfIP(String ip) {
        //ToDo : Implement setSelfIP method
    }

    @Override
    public byte[] getPublishData() {
        //ToDo : Implement getPublishData method
        return null;
    }

    @Override
    public void subscribe(String topic, HandlerDataCallback handlerDataCallback) {
        //ToDo : Implement subscribe method
    }

    @Override
    public void publish(String topic, String ip, int port, byte[] message) {
        //ToDo : Implement publish method
    }

    @Override
    public void unsubscribe() {
        //ToDo : Implement unsubscribe method
    }

    @Override
    public void disconnect() {
        //ToDo : Implement disconnect method
    }
}
