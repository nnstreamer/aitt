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
package com.samsung.android.aitt.stream;

public interface AittStream {

    /**
     * Role of the stream object
     */
    enum StreamRole {
        PUBLISHER,
        SUBSCRIBER
    }

    /**
     * State of the stream object
     */
    enum StreamState {
        INIT,
        READY,
        PLAYING
    }

    /**
     * Callback to get stream data
     */
    interface StreamDataCallback {
        void pushStreamData(byte[] data);
    }

    /**
     * Callback to get stream state change
     */
    interface StreamStateChangeCallback {
        void pushStataChange(StreamState state);
    }

    /**
     * Method to set configuration
     */
    AittStream setConfig(String key, String value);

    /**
     * Method to start stream
     */
    void start();

    /**
     * Method to push data on a topic
     * @param message Data to be published
     * @return returns status
     */
    boolean push(byte[] message);

    /**
     * Method to disconnect from the broker
     */
    void disconnect();

    /**
     * Method to stop the stream
     */
    void stop();

    /**
     * Method to set state callback
     */
    void setStateCallback(StreamStateChangeCallback stateChangeCallback);

    /**
     * Method to set subscribe callback
     * @param streamDataCallback subscribe callback object
     */
    void setReceiveCallback(StreamDataCallback streamDataCallback);

    /**
     * Method to receive stream height
     * @return returns height of the stream
     */
    int getStreamHeight();

    /**
     * Method to receive stream width
     * @return returns width of the stream
     */
    int getStreamWidth();

}
