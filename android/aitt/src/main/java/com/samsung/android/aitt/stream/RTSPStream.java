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

import com.google.flatbuffers.FlexBuffers;
import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.aitt.internal.Definitions;
import com.samsung.android.aittnative.JniInterface;
import com.samsung.android.modules.rtsp.RTSPClient;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Class to implement RTSPStream functionalities
 */
public class RTSPStream implements AittStream {

    private static final String TAG = "RTSPStream";
    private static final String SERVER_STATE = "server_state";
    private static final String URL = "url";
    private static final String ID = "id";
    private static final String PASSWORD = "password";
    private static final String URL_PREFIX = "rtsp://";

    private final StreamRole streamRole;
    private final String topic;
    private final Map<String, String> info = new HashMap<>();
    private RTSPClient rtspClient;
    private StreamDataCallback streamCallback;
    private StreamStateChangeCallback stateChangeCallback = null;
    private JniInterface jniInterface;
    private StreamState serverState = StreamState.INIT;
    private StreamState clientState = StreamState.INIT;

    /**
     * RTSPStream constructor
     * @param topic Topic to which streaming is invoked
     * @param streamRole Role of the RTSPStream object
     */
    private RTSPStream(String topic, StreamRole streamRole) {
        this.topic = topic;
        this.streamRole = streamRole;

        if (streamRole == StreamRole.SUBSCRIBER) {
            RTSPClient.ReceiveDataCallback dataCallback = frame -> {
                if (streamCallback != null)
                    streamCallback.pushStreamData(frame);
            };

            rtspClient = new RTSPClient(new AtomicBoolean(false), dataCallback);
        }
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
     * @param config AittStreamConfig object
     */
    @Override
    public void setConfig(AittStreamConfig config) {
        if (config == null) {
            Log.e(TAG, "Invalid configuration");
            return;
        }

        if (config.getUrl() != null) {
            String url = config.getUrl();
            if (!url.startsWith(URL_PREFIX)) {
                Log.e(TAG, "Invalid RTSP URL");
                return;
            }
            info.put(URL, config.getUrl());
        }
        if (config.getId() != null) {
            info.put(ID, config.getId());
        }
        if (config.getPassword() != null) {
            info.put(PASSWORD, config.getPassword());
        }
    }

    /**
     * Method to start stream
     */
    @Override
    public void start() {
        if (streamRole == StreamRole.SUBSCRIBER) {
            if (serverState == StreamState.READY) {
                startRtspClient();
            } else {
                Log.d(TAG, "RTSP server not yet ready");
                updateState(streamRole, StreamState.READY);
            }
        } else {
            updateState(streamRole, StreamState.READY);
            updateDiscoveryMessage();
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
        //ToDo : disconnect and stop can be merged
        jniInterface.removeDiscoveryCallback(topic);
    }

    /**
     * Method to stop the stream
     */
    @Override
    public void stop() {
        if(streamRole == StreamRole.SUBSCRIBER) {
            if (clientState == StreamState.PLAYING)
                rtspClient.stop();
            updateState(streamRole, StreamState.INIT);
        } else {
            updateState(streamRole, StreamState.INIT);
            updateDiscoveryMessage();
        }
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
    public void setStateCallback(StreamStateChangeCallback callback) {
        this.stateChangeCallback = callback;
    }

    /**
     * Method to set subscribe callback
     * @param streamDataCallback subscribe callback object
     */
    @Override
    public void setReceiveCallback(StreamDataCallback streamDataCallback) {
        if(streamRole == StreamRole.SUBSCRIBER)
            streamCallback = streamDataCallback;
        else
            throw new IllegalArgumentException("The role of this stream is not subscriber");
    }

    /**
     * Method to set subscribe callback
     * @param jniInterface JniInterface object
     */
    public void setJNIInterface(JniInterface jniInterface) {
        this.jniInterface = jniInterface;

        jniInterface.setDiscoveryCallback(topic, (status, data) -> {
            Log.d(TAG, "Received discovery callback");
            if (streamRole == StreamRole.PUBLISHER)
                return;

            if (status.compareTo(Definitions.WILL_LEAVE_NETWORK) == 0) {
                if (clientState == StreamState.PLAYING) {
                    rtspClient.stop();
                    updateState(streamRole, StreamState.READY);
                } else {
                    updateState(streamRole, StreamState.INIT);
                }

                return;
            }

            ByteBuffer buffer = ByteBuffer.wrap(data);
            FlexBuffers.Map map = FlexBuffers.getRoot(buffer).asMap();
            if (map.size() != 4) {
                Log.e(TAG, "Invalid RTSP discovery message");
                return;
            }

            StreamState state = StreamState.values()[map.get(SERVER_STATE).asInt()];
            updateState(StreamRole.PUBLISHER, state);
            info.put(URL, map.get(URL).asString());
            info.put(ID, map.get(ID).asString());
            info.put(PASSWORD, map.get(PASSWORD).asString());

            String url = createCompleteUrl();
            Log.d(TAG, "RTSP URL : " + url);
            if (url.isEmpty()) {
                return;
            }
            rtspClient.setRtspUrl(url);

            if (serverState == StreamState.READY) {
                if (clientState == StreamState.READY) {
                    startRtspClient();
                }
            } else if (serverState == StreamState.INIT) {
                if (clientState == StreamState.PLAYING) {
                    rtspClient.stop();
                    updateState(streamRole, StreamState.READY);
                }
            }
        });
    }

    private String createCompleteUrl() {
        String completeUrl = info.get(URL);
        if (completeUrl == null || completeUrl.isEmpty())
            return "";

        String id = info.get(ID);
        if (id == null || id.isEmpty())
            return completeUrl;

        String password = info.get(PASSWORD);
        if (password == null || password.isEmpty())
            return completeUrl;

        completeUrl = new StringBuilder(completeUrl).insert(URL_PREFIX.length(), id + ":" + password + "@").toString();
        return completeUrl;
    }

    private void startRtspClient() {
        RTSPClient.SocketConnectCallback cb = socketSuccess -> {
            if (socketSuccess) {
                rtspClient.initRtspClient();
                rtspClient.start();
                updateState(streamRole, StreamState.PLAYING);
            } else {
                Log.e(TAG, "Error creating socket");
            }
        };

        rtspClient.createClientSocket(cb);
    }

    private void updateDiscoveryMessage() {
        FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        {
            int smap = fbb.startMap();
            fbb.putInt(SERVER_STATE, serverState.ordinal());
            fbb.putString(URL, info.get(URL));
            fbb.putString(ID, info.get(ID));
            fbb.putString(PASSWORD, info.get(PASSWORD));
            fbb.endMap(null, smap);
        }
        ByteBuffer buffer = fbb.finish();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data, 0, data.length);

        jniInterface.updateDiscoveryMessage(topic, data);
    }

    private void updateState(StreamRole role, StreamState state) {
        if (role == StreamRole.PUBLISHER)
            serverState = state;
        else
            clientState = state;

        if (role == streamRole && stateChangeCallback != null)
            stateChangeCallback.pushStataChange(state);
    }
}
