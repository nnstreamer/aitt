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

import static com.samsung.android.aitt.internal.Definitions.DEFAULT_FPS;
import static com.samsung.android.aitt.internal.Definitions.DEFAULT_STREAM_HEIGHT;
import static com.samsung.android.aitt.internal.Definitions.DEFAULT_STREAM_WIDTH;

import android.content.Context;
import android.util.Log;

import com.google.flatbuffers.FlexBuffers;
import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.aitt.internal.Definitions;
import com.samsung.android.aittnative.JniInterface;
import com.samsung.android.modules.webrtc.WebRTC;
import com.samsung.android.modules.webrtc.WebRTCPublisher;
import com.samsung.android.modules.webrtc.WebRTCSubscriber;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public final class WebRTCStream implements AittStream {

    private static final String TAG = "WebRTCStream";
    private static final String SINK = "/SINK";
    private static final String SRC = "/SRC";
    private static final String START = "START";
    private static final String STOP = "STOP";
    private static final String ID = "id";
    private static final String PEER_ID = "peer_id";
    private static final String SDP = "sdp";
    private static final String ICE_CANDIDATES = "ice_candidates";

    private final String publishTopic;
    private final String watchTopic;
    private final StreamRole streamRole;
    private final String id;
    private final WebRTC webrtc;

    private JniInterface jniInterface;
    private StreamState streamState = StreamState.INIT;
    private StreamStateChangeCallback stateChangeCallback = null;

    WebRTCStream(String topic, StreamRole streamRole, Context context) {
        this.streamRole = streamRole;

        id = createId();
        if (streamRole == StreamRole.PUBLISHER) {
            publishTopic = topic + SRC;
            watchTopic = topic + SINK;
            webrtc = new WebRTCPublisher(context, DEFAULT_STREAM_WIDTH, DEFAULT_STREAM_HEIGHT, DEFAULT_FPS);
        } else {
            publishTopic = topic + SINK;
            watchTopic = topic + SRC;
            webrtc = new WebRTCSubscriber(context, DEFAULT_STREAM_WIDTH, DEFAULT_STREAM_HEIGHT, DEFAULT_FPS);
        }
    }

    public static WebRTCStream createSubscriberStream(String topic, StreamRole streamRole, Context context) {
        if (streamRole != StreamRole.SUBSCRIBER)
            throw new IllegalArgumentException("The role of this stream is not subscriber.");

        return new WebRTCStream(topic, streamRole, context);
    }

    public static WebRTCStream createPublisherStream(String topic, StreamRole streamRole, Context context) {
        if (streamRole != StreamRole.PUBLISHER)
            throw new IllegalArgumentException("The role of this stream is not publisher.");

        return new WebRTCStream(topic, streamRole, context);
    }

    @Override
    public AittStream setConfig(String key, String value) {
        if (streamRole == StreamRole.SUBSCRIBER && !"DECODE_CODEC".equals(key))
            throw new IllegalArgumentException("Key provided does not match with the stream role");

        if (streamState == StreamState.READY)
            throw new RuntimeException("Stream is already started, cannot change configuration now");

        if (key == null)
            throw new IllegalArgumentException("Invalid key");

        switch (key) {
            case "SOURCE_TYPE":
                webrtc.setSourceType(value);
                break;
            case "MEDIA_FORMAT":
                webrtc.setMediaFormat(value);
                break;
            case "DECODE_CODEC":
                webrtc.setDecodeCodec(value);
                break;
            case "HEIGHT":
                int height = Integer.parseInt(value);
                if (height <= 0)
                    throw new IllegalArgumentException("Invalid frame height");
                else
                    webrtc.setFrameHeight(height);
                break;
            case "WIDTH":
                int width = Integer.parseInt(value);
                if (width <= 0)
                    throw new IllegalArgumentException("Invalid frame width");
                else
                    webrtc.setFrameWidth(width);
                break;
            case "FRAME_RATE":
                int fps = Integer.parseInt(value);
                if (fps <= 0)
                    throw new IllegalArgumentException("Invalid frame rate");
                else
                    webrtc.setFrameRate(fps);
                break;
            default:
                throw new IllegalArgumentException("Invalid key");
        }
        return this;
    }

    @Override
    public void start() throws InstantiationException {
        if (streamState == StreamState.READY || streamState == StreamState.PLAYING) {
            Log.e(TAG, "Stream already started");
            return;
        }

        webrtc.registerIceCandidateAddedCallback(() -> publishDiscoveryMessage(generateDiscoveryMessage()));
        webrtc.start();
        if (streamRole == StreamRole.PUBLISHER) {
            FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
            fbb.putString(START);
            ByteBuffer buffer = fbb.finish();
            byte[] data = new byte[buffer.remaining()];
            buffer.get(data, 0, data.length);
            publishDiscoveryMessage(data);
        } else if (streamRole == StreamRole.SUBSCRIBER && webrtc.getPeerDiscoveryId() != null) {
            byte[] discData = generateDiscoveryMessage();
            publishDiscoveryMessage(discData);
        }
        updateState(StreamState.READY);
    }

    @Override
    public void disconnect() {
        stop();
        if (jniInterface != null)
            jniInterface.removeDiscoveryCallback(watchTopic);
    }

    @Override
    public void stop() {
        if (streamState == StreamState.INIT) {
            return;
        }

        webrtc.stop();
        FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        fbb.putString(STOP);
        ByteBuffer buffer = fbb.finish();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data, 0, data.length);
        if (jniInterface != null)
            jniInterface.updateDiscoveryMessage(publishTopic, data);
        updateState(StreamState.INIT);
    }

    @Override
    public void setStateCallback(StreamStateChangeCallback callback) {
        stateChangeCallback = callback;
    }

    @Override
    public void setReceiveCallback(StreamDataCallback streamDataCallback) {
        if (streamDataCallback == null)
            throw new IllegalArgumentException("The given callback is null.");

        if (streamRole == StreamRole.SUBSCRIBER) {
            webrtc.registerDataCallback(streamDataCallback::pushStreamData);
        } else if (streamRole == StreamRole.PUBLISHER) {
            Log.e(TAG, "Invalid function call");
        }
    }

    @Override
    public int getStreamHeight() {
        return webrtc.getFrameHeight();
    }

    @Override
    public int getStreamWidth() {
        return webrtc.getFrameWidth();
    }

    @Override
    public boolean push(byte[] message) {
        if (streamRole == StreamRole.SUBSCRIBER) {
            throw new RuntimeException("Push is not allowed with a subscriber stream");
        }

        if (publishTopic.contains(Definitions.RESPONSE_POSTFIX)) {
            Log.d(TAG, "A message is sent through a WebRTC publisher stream.");
            return webrtc.pushMessageData(message);
        } else {
            Log.d(TAG, "Media-packet is sent through a WebRTC publisher stream.");
            return webrtc.pushMediaPacket(message);
        }
    }

    public void setJNIInterface(JniInterface jniInterface) {
        this.jniInterface = jniInterface;

        jniInterface.setDiscoveryCallback(watchTopic, (clientId, status, data) -> {
            if (status.compareTo(Definitions.WILL_LEAVE_NETWORK) == 0) {
                webrtc.removePeer(clientId);
                updateState(StreamState.INIT);
                return;
            }

            ByteBuffer buffer = ByteBuffer.wrap(data);
            if (FlexBuffers.getRoot(buffer).isString())
                handleStreamState(clientId, buffer);
            else
                handleStreamInfo(clientId, buffer);
        });
    }

    private String createId() {
        return UUID.randomUUID().toString();
    }

    private void handleStreamState(String clientId, ByteBuffer buffer) {
        String peerState = FlexBuffers.getRoot(buffer).asString();
        if (STOP.compareTo(peerState) == 0) {
            updateState(StreamState.INIT);
            webrtc.removePeer(clientId);
        } else if (START.compareTo(peerState) == 0) {
            if (streamRole == StreamRole.SUBSCRIBER)
                webrtc.setPeerDiscoveryId(clientId);
        } else {
            Log.e(TAG, "Invalid message");
        }
    }

    private void handleStreamInfo(String clientId, ByteBuffer buffer) {
        FlexBuffers.Map map = FlexBuffers.getRoot(buffer).asMap();
        if (!isValidInfo(map)) {
            Log.e(TAG, "Invalid WebRTC stream information");
            return;
        }

        Log.d(TAG, "Got discovery message");
        String id = map.get(ID).asString();
        String peerId = map.get(PEER_ID).asString();
        String sdp = map.get(SDP).asString();
        FlexBuffers.Vector vector = map.get(ICE_CANDIDATES).asVector();
        int size = vector.size();
        List<String> iceCandidates = new ArrayList<>();
        for (int i = 0; i < size; i++)
            iceCandidates.add(vector.get(i).asString());

        if (webrtc.getPeerDiscoveryId() == null) {
            if (webrtc.setPeerDiscoveryId(clientId))
                webrtc.addPeer(id, sdp, iceCandidates);
        }
        else if(webrtc.getPeerDiscoveryId().compareTo(clientId) != 0)
            Log.e(TAG, "Invalid peer discovery ID");
        else if (this.id.compareTo(peerId) != 0)
            Log.e(TAG, "Discovery message for different id");
        else if(webrtc.getPeerId() == null)
            webrtc.addPeer(id, sdp, iceCandidates);
        else
            webrtc.updatePeer(iceCandidates);
    }

    private boolean isValidInfo(FlexBuffers.Map map) {
        boolean hasId = map.get(ID).isString();
        boolean hasPeerId = map.get(PEER_ID).isString();
        boolean hasSdp = map.get(SDP).isString();
        boolean hasIceCandidate = map.get(ICE_CANDIDATES).isVector();
        return hasId & hasPeerId & hasSdp & hasIceCandidate;
    }

    private byte[] generateDiscoveryMessage() {
        FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        {
            int smap = fbb.startMap();
            fbb.putString(ID, id);
            String peerId = webrtc.getPeerId();
            if (peerId == null)
                peerId = webrtc.getPeerDiscoveryId();
            fbb.putString(PEER_ID, peerId);
            fbb.putString(SDP, webrtc.getLocalDescription());
            int svec = fbb.startVector();
            List<String> iceCandidates = webrtc.getIceCandidates();
            for (String candidate : iceCandidates) {
                fbb.putString(candidate);
            }
            fbb.endVector(ICE_CANDIDATES, svec, false, false);
            fbb.endMap(null, smap);
        }
        ByteBuffer buffer = fbb.finish();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data, 0, data.length);
        return data;
    }

    private void publishDiscoveryMessage(byte[] data) {
        if (jniInterface != null) {
            jniInterface.updateDiscoveryMessage(publishTopic, data);
        }
    }

    private void updateState(StreamState state) {
        streamState = state;
        if (stateChangeCallback != null)
            stateChangeCallback.pushStataChange(streamState);
    }
}
