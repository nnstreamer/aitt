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

/**
 * An interface to create a transportHandler and provide APIs for protocol specific implementation
 */
interface TransportHandler {
    /**
     * Method to implement protocol specific subscribe functionalities
     * @param topic String topic to which subscribe is called
     * @param handlerDataCallback callback object to send data from transport handler layer to aitt layer
     */
    void subscribe(String topic, HandlerDataCallback handlerDataCallback);

    /**
     * Method to implement protocol specific publish functionalities
     * @param topic String topic to which publish is called
     * @param ip IP address of the destination
     * @param port port number of the destination
     * @param message message to be published to specific topic
     */
    void publish(String topic, String ip, int port, byte[] message);

    /**
     * Method to implement protocol specific unsubscribe functionalities
     */
    void unsubscribe();

    /**
     * Method to implement protocol specific disconnect functionalities
     */
    void disconnect();

    /**
     * Method to set application context to transport handler
     * @param appContext application context
     */
    void setAppContext(Context appContext);

    /**
     * Method to set IP address of self device to transport handler
     * @param ip IP address of the device
     */
    void setSelfIP(String ip);

    /**
     * Method to get data to publish self details
     * @return returns details of self device
     */
    byte[] getPublishData();

    /**
     * Interface to implement handler data callback mechanism
     */
    interface HandlerDataCallback{
        void pushHandlerData(byte[] data);
    }
}
