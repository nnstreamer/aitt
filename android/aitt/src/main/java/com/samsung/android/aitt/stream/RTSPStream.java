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

import android.util.Log;

import com.samsung.android.modules.rtsp.RTSPClient;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Class to implement RTSPStream functionalities
 */
public class RTSPStream implements AittStream {

    private static String TAG = "RTSPStream";
    private StreamRole streamRole;
    private RTSPClient rtspClient;
    private AittStream.StreamDataCallback subscribeCallback;

    /**
     * RTSPStream constructor
     * @param topic Topic to which streaming is invoked
     * @param streamRole Role of the RTSPStream object
     */
    private RTSPStream(String topic, StreamRole streamRole) {
        //TODO: create and assign topic to a local variable (topic)
        this.streamRole = streamRole;
    }

    /**
     * Create and return RTSPStream object for subscriber role
     * @param topic  Topic to which Subscribe role is set
     * @param streamRole Role of the RTSPStream object
     * @return RTSPStream object
     */
    public static RTSPStream createSubscriberStream(String topic, StreamRole streamRole) {
        if (streamRole != StreamRole.SUBSCRIBER)
            throw new IllegalArgumentException("The role of this stream is not subscriber.");

        return new RTSPStream(topic, streamRole);
    }

    /**
     * Create and return RTSPStream object for publisher role
     * @param topic  Topic to which Publisher role is set
     * @param streamRole Role of the RTSPStream object
     * @return RTSPStream object
     */
    public static RTSPStream createPublisherStream(String topic, StreamRole streamRole) {
        if (streamRole != StreamRole.PUBLISHER)
            throw new IllegalArgumentException("The role of this stream is not publisher.");

        return new RTSPStream(topic, streamRole);
    }

    /**
     * Method to set configuration
     */
    @Override
    public void setConfig() {
        // TODO: implement this function.
    }

    /**
     * Method to start stream
     */
    @Override
    public void start() {
        if (streamRole == StreamRole.SUBSCRIBER) {
            RTSPClient.ReceiveDataCallback dataCallback = new RTSPClient.ReceiveDataCallback() {
                @Override
                public void pushData(byte[] frame) {
                    subscribeCallback.pushStreamData(frame);
                }
            };

            rtspClient = new RTSPClient(new AtomicBoolean(false), dataCallback);
            RTSPClient.SocketConnectCallback cb = socketSuccess -> {
                if (socketSuccess) {
                    rtspClient.initRtspClient();
                    rtspClient.start();
                } else {
                    Log.e(TAG, "Error creating socket");
                }
            };
            rtspClient.createClientSocket(cb);
        } else {
            Log.d(TAG, "Publisher role is not yet supported");
        }
    }

    /**
     * Method to publish to a topic
     * @param topic String topic to which data is published
     * @param ip Ip of the receiver
     * @param port Port of the receiver
     * @param message Data to be published
     * @return returns status
     */
    @Override
    public boolean publish(String topic, String ip, int port, byte[] message) {
        // TODO: implement this function.
        return true;
    }

    /**
     * Method to disconnect from the broker
     */
    @Override
    public void disconnect() {
        // TODO: implement this function.
    }

    /**
     * Method to stop the stream
     */
    @Override
    public void stop() {
        if(streamRole == StreamRole.SUBSCRIBER)
            rtspClient.stop();
        else
            Log.d(TAG, "Publisher role is not yet supported");
    }

    /**
     * Method to pause the stream
     */
    public void pause() {
        // TODO: implement this function.
    }

    /**
     * Method to record the stream
     */
    public void record() {
        // TODO: implement this function.
    }

    /**
     * Method to set state callback
     */
    @Override
    public void setStateCallback() {
        // TODO: implement this function.
    }

    /**
     * Method to set subscribe callback
     * @param streamDataCallback subscribe callback object
     */
    @Override
    public void setReceiveCallback(AittStream.StreamDataCallback streamDataCallback) {
        if(streamRole == StreamRole.SUBSCRIBER)
            subscribeCallback = streamDataCallback;
        else
            throw new IllegalArgumentException("The role of this stream is not subscriber");
    }
}
