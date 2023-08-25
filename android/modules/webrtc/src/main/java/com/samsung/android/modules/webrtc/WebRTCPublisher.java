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

import static org.webrtc.SessionDescription.Type.OFFER;

import static java.lang.Math.min;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;

import com.github.luben.zstd.Zstd;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.Camera1Enumerator;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.CapturerObserver;
import org.webrtc.DataChannel;
import org.webrtc.IceCandidate;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.MediaStreamTrack;
import org.webrtc.NV21Buffer;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnection.SignalingState;
import org.webrtc.RtpReceiver;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoFrame;
import org.webrtc.VideoSource;
import org.webrtc.VideoTrack;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

public final class WebRTCPublisher extends WebRTC {

    private static final String TAG = "WebRTCPublisher";
    private static final String VIDEO_TRACK_ID = "AITT_WEBRTC_v0";
    private static final String MEDIA_STREAM_ID = "AITT_WEBRTC";
    private static final String SURFACE_TEXTURE_THREAD = "AITT_WEBRTC_SURFACE_TEXTURE";

    private MediaStream mediaStream;
    private VideoSource videoSource;
    private VideoTrack videoTrack;
    private MediaPacketCapturer mediaPacketCapturer;
    private VideoCapturer cameraCapturer;
    private CameraEnumerator cameraEnumerator;
    private List<String> cameraDeviceNames;

    public WebRTCPublisher(Context appContext, int width, int height, int fps)  {
        super(appContext, width, height, fps);
    }

    @Override
    public void setRemoteDescription(String sdp) {
        if (sdp == null || sdp.isEmpty())
            return;

        Log.d(TAG, "Got offer " + sdp);
        String offer = extractSDP(sdp);
        peerConnection.setRemoteDescription(new SimpleSdpObserver(), new SessionDescription(OFFER, offer));
    }

    @Override
    public void start() throws InstantiationException {
        if (peerConnection == null) {
            initializePeerConnectionFactory();
            initializePeerConnection();
        }
        configureStream();
        if (sourceType == SourceType.CAMERA && cameraCapturer != null)
            cameraCapturer.startCapture(frameWidth, frameHeight, frameRate);
    }

    @Override
    public void stop() {
        if (peerConnection == null)
            return;
        cleanStream();
        localDataChannel.dispose();
        localDataChannel = null;
        peerConnection.dispose();
        peerConnection = null;
        connectionFactory.dispose();
        connectionFactory = null;
    }

    @Override
    public boolean pushMediaPacket(byte[] packet) {
        if (sourceType != SourceType.MEDIA_PACKET)
            throw new RuntimeException("Push is not supported for this source type");
        return mediaPacketCapturer.send(packet, frameWidth, frameHeight);
    }

    @Override
    public boolean pushMessageData(byte[] message) {
        ByteBuffer chunkData;
        if (message.length < MAX_MESSAGE_SIZE) {
            ByteBuffer data = ByteBuffer.wrap(message);
            return localDataChannel.send(new DataChannel.Buffer(data, false));
        }
        try {
            byte[] compressData = Zstd.compress(message);
            int len = compressData.length;
            int i = 0;
            while (i < compressData.length) {
                byte[] chunk = Arrays.copyOfRange(compressData, i, i + min(len, MAX_MESSAGE_SIZE));
                chunkData = ByteBuffer.wrap(chunk);
                localDataChannel.send(new DataChannel.Buffer(chunkData, false));
                i += MAX_MESSAGE_SIZE;
                len -= MAX_MESSAGE_SIZE;
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during message sending", e);
            return false;
        }
        chunkData = ByteBuffer.wrap(EOF_MESSAGE.getBytes(StandardCharsets.UTF_8));
        return localDataChannel.send(new DataChannel.Buffer(chunkData, false));
    }

    @Override
    protected void initializePeerConnection() throws InstantiationException {
        PeerConnection.RTCConfiguration rtcConfig = new PeerConnection.RTCConfiguration(new ArrayList<>());

        PeerConnection.Observer pcObserver = new PeerConnection.Observer() {
            @Override
            public void onSignalingChange(PeerConnection.SignalingState signalingState) {
                Log.d(TAG, "onSignalingChange: " + signalingState);
                if (signalingState == SignalingState.HAVE_REMOTE_OFFER) {
                    createAnswer();
                }
            }

            @Override
            public void onIceConnectionChange(PeerConnection.IceConnectionState iceConnectionState) {
                Log.d(TAG, "onIceConnectionChange: " + iceConnectionState);
            }

            @Override
            public void onIceConnectionReceivingChange(boolean b) {
                Log.d(TAG, "onIceConnectionReceivingChange: " + b);
            }

            @Override
            public void onIceGatheringChange(PeerConnection.IceGatheringState iceGatheringState) {
                Log.d(TAG, "onIceGatheringChange: " + iceGatheringState);
            }

            @Override
            public void onIceCandidate(IceCandidate iceCandidate) {
                Log.d(TAG, "onIceCandidate: " + iceCandidate.sdp);
                try {
                    JSONObject candidateMessge = new JSONObject();
                    candidateMessge.put("candidate", iceCandidate.sdp);
                    candidateMessge.put("sdpMLineIndex", iceCandidate.sdpMLineIndex);

                    JSONObject iceMessage = new JSONObject();
                    iceMessage.put("ice", candidateMessge);
                    iceCandidates.add(iceMessage.toString());

                    if (iceCandidateAddedCallback != null)
                        iceCandidateAddedCallback.onIceCandidate();
                } catch (JSONException e) {
                    Log.e(TAG, "Failed to set ice candidate: " + e.getMessage());
                }
            }

            @Override
            public void onIceCandidatesRemoved(IceCandidate[] iceCandidates) {
                Log.d(TAG, "onIceCandidatesRemoved: ");
                removeIceCandidates(iceCandidates);
            }

            @Override
            public void onAddStream(MediaStream mediaStream) {
                Log.d(TAG, "onAddStream: " + mediaStream.videoTracks.size());
                VideoTrack remoteVideoTrack = mediaStream.videoTracks.get(0);
                remoteVideoTrack.setEnabled(true);
                // ToDo : update stream state as playing
            }

            @Override
            public void onRemoveStream(MediaStream mediaStream) {
                Log.d(TAG, "onRemoveStream: ");
            }

            @Override
            public void onDataChannel(DataChannel dataChannel) {
                Log.d(TAG, "onDataChannel: ");
            }

            @Override
            public void onRenegotiationNeeded() {
                Log.d(TAG, "onRenegotiationNeeded: ");
            }

            @Override
            public void onAddTrack(RtpReceiver rtpReceiver, MediaStream[] mediaStreams) {
                MediaStreamTrack track = rtpReceiver.track();
                if (track instanceof VideoTrack) {
                    Log.i(TAG, "onAddTrack");
                    VideoTrack remoteVideoTrack = (VideoTrack) track;
                    remoteVideoTrack.setEnabled(true);
                }
            }
        };

        peerConnection = connectionFactory.createPeerConnection(rtcConfig, pcObserver);
        if (peerConnection == null)
            throw new InstantiationException("Failed to create peerConnection");

        localDataChannel = peerConnection.createDataChannel("sendDataChannel", new DataChannel.Init());
    }

    @Override
    protected void configureStream() {
        if (peerConnection == null)
            throw new RuntimeException("Invalid stream state, peer connection not created");

        cleanStream();
        videoSource = connectionFactory.createVideoSource(false);
        createVideoTrack();
        createMediaStream();
        createVideoCapturer();
    }

    private void cleanStream() {
        if (mediaStream != null) {
            peerConnection.removeStream(mediaStream);
            if (videoTrack != null) {
                mediaStream.removeTrack(videoTrack);
                videoTrack.dispose();
                videoTrack = null;
            }
            mediaStream = null;
        }

        if (cameraCapturer != null) {
            try {
                cameraCapturer.stopCapture();
                cameraCapturer.dispose();
            } catch (InterruptedException e) {
                Log.e(TAG, "Failed to stop cameraCapturer:" + e.getMessage());
            }
        }

        if (videoSource != null) {
            videoSource.dispose();
            videoSource = null;
        }
    }

    private void createAnswer() {
        peerConnection.createAnswer(new SimpleSdpObserver() {
            @Override
            public void onCreateSuccess(SessionDescription sessionDescription) {
                Log.d(TAG, "CreateAnswer : onCreateSuccess");
                peerConnection.setLocalDescription(new SimpleSdpObserver(), sessionDescription);
                try {
                    JSONObject sdpMessage = new JSONObject();
                    sdpMessage.put("type", "answer");
                    sdpMessage.put("sdp", sessionDescription.description);

                    JSONObject answerMessage = new JSONObject();
                    answerMessage.put("sdp", sdpMessage);
                    localDescription = answerMessage.toString();
                } catch (JSONException e) {
                    Log.e(TAG, "Failed to set local description: " + e.getMessage());
                }
            }
        }, new MediaConstraints());
    }

    private void createVideoTrack() {
        videoTrack = connectionFactory.createVideoTrack(VIDEO_TRACK_ID, videoSource);
        videoTrack.setEnabled(true);
    }

    private void createMediaStream() {
        mediaStream = connectionFactory.createLocalMediaStream(MEDIA_STREAM_ID);
        mediaStream.addTrack(videoTrack);
        peerConnection.addStream(mediaStream);
    }

    private void createVideoCapturer() {
        if (sourceType == SourceType.CAMERA) {
            if (cameraDeviceNames == null)
                setCameraDeviceNames();
            for (String deviceName : cameraDeviceNames) {
                cameraCapturer = cameraEnumerator.createCapturer(deviceName , null);
                if (cameraCapturer != null)
                    break;
            }
            if (cameraCapturer != null) {
                SurfaceTextureHelper surfaceTextureHelper = SurfaceTextureHelper.create(SURFACE_TEXTURE_THREAD, eglBase.getEglBaseContext());
                cameraCapturer.initialize(surfaceTextureHelper, appContext, videoSource.getCapturerObserver());
            }
        } else {
            mediaPacketCapturer = new MediaPacketCapturer();
            mediaPacketCapturer.initialize(null, null, videoSource.getCapturerObserver());
        }
    }

    private void setCameraDeviceNames() {
        cameraDeviceNames = new ArrayList<>();
        if (Camera2Enumerator.isSupported(appContext))
            cameraEnumerator = new Camera2Enumerator(appContext);
        else
            cameraEnumerator = new Camera1Enumerator(true);

        String[] deviceNames = cameraEnumerator.getDeviceNames();
        for (String deviceName : deviceNames) {
            if(cameraEnumerator.isFrontFacing(deviceName)){
                cameraDeviceNames.add(deviceName);
            }
        }
    }

    /**
     * Class to implement Media Packet capturer
     */
    private static class MediaPacketCapturer implements VideoCapturer {
        private CapturerObserver capturerObserver;

        boolean send(byte[] frame, int width, int height) {
            long timestampNS = TimeUnit.MILLISECONDS.toNanos(SystemClock.elapsedRealtime());
            NV21Buffer buffer = new NV21Buffer(frame, width, height, null);
            VideoFrame videoFrame = new VideoFrame(buffer, 0, timestampNS);
            this.capturerObserver.onFrameCaptured(videoFrame);
            videoFrame.release();
            return true;
        }

        @Override
        public void initialize(SurfaceTextureHelper surfaceTextureHelper, Context context, CapturerObserver capturerObserver) {
            this.capturerObserver = capturerObserver;
        }

        @Override
        public void startCapture(int i, int i1, int i2) {}

        @Override
        public void stopCapture() {}

        @Override
        public void changeCaptureFormat(int i, int i1, int i2) {}

        @Override
        public void dispose() {}

        public boolean isScreencast() {
            return false;
        }
    }
}
