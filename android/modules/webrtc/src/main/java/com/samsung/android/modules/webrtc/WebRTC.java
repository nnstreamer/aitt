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
package com.samsung.android.modules.webrtc;

import android.content.Context;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.DataChannel;
import org.webrtc.DefaultVideoDecoderFactory;
import org.webrtc.DefaultVideoEncoderFactory;
import org.webrtc.EglBase;
import org.webrtc.HardwareVideoDecoderFactory;
import org.webrtc.HardwareVideoEncoderFactory;
import org.webrtc.IceCandidate;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;
import org.webrtc.VideoCodecInfo;
import org.webrtc.VideoDecoderFactory;
import org.webrtc.VideoEncoderFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * WebRTC class to implement webRTC functionalities
 */
public abstract class WebRTC {

    public static final String EOF_MESSAGE = "EOF";
    public static final int MAX_MESSAGE_SIZE = 32768;
    public static final int MAX_UNCOMPRESSED_MESSAGE_SIZE = 295936;
    private static final String TAG = "WebRTC";
    private static final String H264Codec = "H264";

    protected PeerConnection peerConnection;
    protected PeerConnectionFactory connectionFactory;
    protected EglBase eglBase;
    protected DataChannel localDataChannel;
    protected ReceiveDataCallback dataCallback;
    protected IceCandidateAddedCallback iceCandidateAddedCallback;
    protected String localDescription;
    protected List<String> iceCandidates = new ArrayList<>();
    protected String peerDiscoveryId;
    protected int frameWidth;
    protected int frameHeight;
    protected int frameRate;
    protected SourceType sourceType = SourceType.NULL;
    protected MediaFormat mediaFormat = MediaFormat.VP8;
    protected String codec;

    protected final Context appContext;

    private String peerId;

    /**
     * Interface to create data call back mechanism
     */
    public interface ReceiveDataCallback {
        void pushData(byte[] data);
    }

    /**
     * Callback triggered when ICE candidates are added
     */
    public interface IceCandidateAddedCallback {
        void onIceCandidate();
    }

    protected enum SourceType {
        CAMERA,
        MEDIA_PACKET,
        NULL;

        private static SourceType fromString(String str) {
            switch (str) {
                case "CAMERA":
                    return CAMERA;
                case "MEDIA_PACKET":
                    return MEDIA_PACKET;
                default:
                    throw new IllegalArgumentException("Invalid SOURCE_TYPE: " + str);
            }
        }
    }

    protected enum MediaFormat {
        I420,
        NV12,
        VP8,
        VP9,
        H264,
        JPEG,
        MAX;

        private static MediaFormat fromString(String str) {
            switch (str) {
                case "I420":
                    return I420;
                case "NV12":
                    return NV12;
                case "VP8":
                    return VP8;
                case "VP9":
                    return VP9;
                case "H264":
                    return H264;
                case "JPEG":
                    return JPEG;
                default:
                    return MAX;
            }
        }
    }

    /**
     * Method to set remote description of peer
     *
     * @param sdp String containing SDP of peer
     */
    public abstract void setRemoteDescription(String sdp);

    /**
     * Method to start WebRTC
     */
    public abstract void start() throws InstantiationException;

    protected abstract void initializePeerConnection() throws InstantiationException;

    protected abstract void configureStream();

    /**
     * WebRTC constructor to create webRTC instance
     *
     * @param appContext Application context creating webRTC instance
     * @param width Frame width
     * @param height Frame height
     * @param fps Frames per second
     */
    public WebRTC(Context appContext, int width, int height, int fps)  {
        if (appContext == null)
            throw new IllegalArgumentException("App context is null.");

        this.appContext = appContext;

        frameWidth = width;
        frameHeight = height;
        frameRate = fps;
    }

    /**
     * To create data call-back mechanism
     *
     * @param cb ReceiveDataCallback registered to receive a webrtc data
     */
    public void registerDataCallback(ReceiveDataCallback cb) {
        if (cb == null)
            throw new IllegalArgumentException("Callback is null");

        dataCallback = cb;
    }

    /**
     * To notify when ice-candidate is added
     *
     * @param cb IceCandidateAddedCallback triggered when a new ICE candidate has been found
     */
    public void registerIceCandidateAddedCallback(IceCandidateAddedCallback cb) {
        if (cb == null)
            throw new IllegalArgumentException("Callback is null.");

        iceCandidateAddedCallback = cb;
    }

    /**
     * Method to stop WebRTC
     */
    public void stop() {
        if (peerConnection == null)
            return;
        localDataChannel.dispose();
        localDataChannel = null;
        peerConnection.dispose();
        peerConnection = null;
        connectionFactory.dispose();
        connectionFactory = null;
    }

    /**
     * Method to get peer ID
     *
     * @return String ID of the peer
     */
    public String getPeerId() {
        return peerId;
    }

    /**
     * Method to get peer discovery ID
     *
     * @return String Discovery ID of the peer
     */
    public String getPeerDiscoveryId() {
        return peerDiscoveryId;
    }

    /**
     * Method to set peer discovery ID
     *
     * @param id Discovery ID of the peer
     * @return true if peer is not already added
     */
    public boolean setPeerDiscoveryId(String id) {
        if (peerDiscoveryId != null && !peerDiscoveryId.isEmpty()) {
            Log.e(TAG, "Stream peer already added");
            return false;
        }
        peerDiscoveryId = id;
        return true;
    }

    /**
     * Method to get local description
     *
     * @return Local description
     */
    public String getLocalDescription() {
        return localDescription;
    }

    /**
     * Method to get ICE candidates
     *
     * @return List of ICE candidates
     */
    public List<String> getIceCandidates() {
        return iceCandidates;
    }

    protected void removeIceCandidates(IceCandidate... candidates) {
        for (IceCandidate candidate: candidates) {
            iceCandidates.remove(candidate.toString());
        }
    }

    /**
     * Method to add peer information
     *
     * @param peerId ID of the peer
     * @param sdp SDP sent by peer
     * @param candidates List of ICE candidates
     */
    public void addPeer(String peerId, String sdp, List<String> candidates) {
        this.peerId = peerId;
        setRemoteDescription(sdp);
        updatePeer(candidates);
    }

    /**
     * Method to update ICE candidates
     *
     * @param candidates List of ICE candidates
     */
    public void updatePeer(List<String> candidates) {
        if (candidates == null)
            return;

        for (String candidate : candidates) {
            Log.d(TAG, "Received ICE candidate: " + candidate);
            String ice = extractICE(candidate);
            IceCandidate iceCandidate = new IceCandidate("video", 0, ice);
            peerConnection.addIceCandidate(iceCandidate);
        }
    }

    /**
     * Method to remove connected peer stream
     */
    public void removePeer() {
        if (peerDiscoveryId == null || peerDiscoveryId.isEmpty())
            return;

        peerDiscoveryId = null;
        peerId = null;

        localDescription = null;
        iceCandidates.clear();
    }

    public void setSourceType(String type) {
        SourceType newType = SourceType.fromString(type);
        if (sourceType == newType)
            return;
        sourceType = newType;
    }

    public void setMediaFormat(String format) {
        MediaFormat newFormat = MediaFormat.fromString(format);
        if (newFormat == MediaFormat.MAX) {
            throw new IllegalArgumentException("Invalid MEDIA_FORMAT type");
        }

        if (mediaFormat == newFormat)
            return;

        if (sourceType == SourceType.MEDIA_PACKET) {
            mediaFormat = newFormat;
        }
    }

    public void setDecodeCodec(String newCodec) {
        if (newCodec.equals(codec))
            return;

        if (sourceType == SourceType.NULL) {
            codec = newCodec;
        }
    }

    public void setFrameHeight(int height) {
        frameHeight = height;
    }

    public void setFrameWidth(int width) {
        frameWidth = width;
    }

    /**
     * Method to get received Frame height
     *
     * @return Received frame height
     */
    public int getFrameHeight() {
        return frameHeight;
    }

    /**
     * Method to get received Frame width
     *
     * @return Received frame width
     */
    public int getFrameWidth() {
        return frameWidth;
    }

    public void setFrameRate(int fps) {
        frameRate = fps;
    }

    /**
     * Method used to push media data
     *
     * @param packet  Media packet in byte format
     * @return true if message is successfully sent, false otherwise
     */
    public boolean pushMediaPacket(byte[] packet) {
        Log.d(TAG, "Push video data");
        return true;
    }

    /**
     * Method to push message data, if the message size is greater than MAX_MESSAGE_SIZE, message will be compressed before sending
     *
     * @param message message to be sent in byte format
     * @return true if message is successfully sent, false otherwise
     */
    public boolean pushMessageData(byte[] message) {
        Log.d(TAG, "Push message data");
        return true;
    }

    protected String extractSDP(String sdpMessage) {
        String sdp = null;
        try {
            JSONObject jsonMessage = new JSONObject(sdpMessage);
            sdp = jsonMessage.getJSONObject("sdp").getString("sdp");
        } catch (JSONException e) {
            Log.e(TAG, "Failed to extract sdp: " + e.getMessage());
        }
        return sdp;
    }

    private String extractICE(String iceMessage) {
        String ice = null;
        try {
            JSONObject jsonMessage = new JSONObject(iceMessage);
            ice = jsonMessage.getJSONObject("ice").getString("candidate");
        } catch (JSONException e) {
            Log.e(TAG, "Failed to extract sdp: " + e.getMessage());
        }
        return ice;
    }

    /**
     * Method to initialize peer connection factory
     */
    protected void initializePeerConnectionFactory() {
        eglBase = EglBase.create();
        VideoEncoderFactory encoderFactory;
        VideoDecoderFactory decoderFactory;
        if (mediaFormat == MediaFormat.H264 || H264Codec.equals(codec)) {
            encoderFactory = new HardwareVideoEncoderFactory(eglBase.getEglBaseContext(), false, true);
            decoderFactory = new HardwareVideoDecoderFactory(eglBase.getEglBaseContext());
            VideoCodecInfo[] videoCodecInfo = encoderFactory.getSupportedCodecs();
            for (int i = 0; i < videoCodecInfo.length; i++) {
                Log.d(TAG, "Supported codecs : " + i + " " + videoCodecInfo[i].name);
            }
        } else {
            encoderFactory = new DefaultVideoEncoderFactory(eglBase.getEglBaseContext(), true, true);
            decoderFactory = new DefaultVideoDecoderFactory(eglBase.getEglBaseContext());
        }
        PeerConnectionFactory.initialize(PeerConnectionFactory.InitializationOptions.builder(appContext).setEnableInternalTracer(true).createInitializationOptions());
        PeerConnectionFactory.Builder builder = PeerConnectionFactory.builder().setVideoEncoderFactory(encoderFactory).setVideoDecoderFactory(decoderFactory);
        builder.setOptions(null);
        connectionFactory = builder.createPeerConnectionFactory();
    }

    /**
     * Class to implement SDP observer
     */
    protected static class SimpleSdpObserver implements SdpObserver {
        private static final String TAG = "SimpleSdpObserver";

        @Override
        public void onCreateSuccess(SessionDescription sessionDescription) {
            Log.d(TAG, "onCreateSuccess: " + sessionDescription.description);
        }

        @Override
        public void onSetSuccess() {
            Log.d(TAG, "onSetSuccess:");
        }

        @Override
        public void onCreateFailure(String s) {
            Log.d(TAG, "onCreateFailure: Reason = " + s);
        }

        @Override
        public void onSetFailure(String s) {
            Log.d(TAG, "onSetFailure: Reason = " + s);
        }
    }
}
