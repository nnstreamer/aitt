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
package com.samsung.android.modules.rtsp;

import android.net.Uri;
import android.util.Log;

import androidx.annotation.NonNull;

import com.alexvas.rtsp.RtspClient;
import com.alexvas.utils.NetUtils;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.net.Socket;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * RTSPClient class to implement RTSP client side functionalities
 */
public class RTSPClient {
    private static final String TAG = "RTSPClient";
    private static volatile Socket clientSocket;
    private static final int socketTimeout = 10000;
    private String rtspUrl = null;
    private int height;
    private int width;
    private final AtomicBoolean exitFlag;
    private final ReceiveDataCallback streamCb;
    private RtspClient mRtspClient;
    private H264Decoder decoder;
    private byte[] sps;
    private byte[] pps;
    private DecoderState decoderState = DecoderState.INIT;

    /**
     * State of decoder
     */
    enum DecoderState {
        INIT,
        READY
    }

    /**
     * Interface to implement DataCallback from RTSP module to RTSP stream
     */
    public interface ReceiveDataCallback {
        void pushData(byte[] data);
    }

    /**
     * Interface to implement socket connection callback
     */
    public interface SocketConnectCallback {
        void socketConnect(Boolean bool);
    }

    /**
     * RTSPClient class constructor
     *
     * @param exitFlag AtomicBoolean flag to exit execution
     * @param cb       callback object to send data to upper layer
     */
    public RTSPClient(AtomicBoolean exitFlag, ReceiveDataCallback cb) {
        this.exitFlag = exitFlag;
        streamCb = cb;
    }

    /**
     * Method to create a client socket for RTSP connection with RTSP server
     *
     * @param socketCB socket connection callback to notify success/failure of socket creation
     */
    public void createClientSocket(SocketConnectCallback socketCB) {
        if (rtspUrl == null || rtspUrl.isEmpty()) {
            Log.e(TAG, "Failed create client socket: Invalid RTSP URL");
            return;
        }

        Uri uri = Uri.parse(rtspUrl);
        try {
            Thread thread = new Thread(() -> {
                try {
                    clientSocket = NetUtils.createSocketAndConnect(uri.getHost(), uri.getPort(), socketTimeout);
                    if (clientSocket != null)
                        socketCB.socketConnect(true);
                } catch (Exception e) {
                    socketCB.socketConnect(false);
                    Log.e(TAG, "Exception in RTSP client socket creation");
                }
            });

            thread.start();

        } catch (Exception e) {
            Log.e(TAG, "Exception in RTSP client socket creation");
        }
    }

    /**
     * Method to create RtspClient object to access RTSP lib from dependency
     */
    public void initRtspClient(String id, String password) {

        RtspClient.RtspClientListener clientListener = new RtspClient.RtspClientListener() {
            @Override
            public void onRtspConnecting() {
                Log.d(TAG, "Connecting to RTSP server");
            }

            @Override
            public void onRtspConnected(@NonNull @NotNull RtspClient.SdpInfo sdpInfo) {
                Log.d(TAG, "Connected to RTSP server");
                if (sdpInfo.videoTrack != null) {
                    sps = sdpInfo.videoTrack.sps;
                    pps = sdpInfo.videoTrack.pps;
                }
            }

            @Override
            public void onRtspVideoNalUnitReceived(@NonNull @NotNull byte[] bytes, int i, int i1, long l) {
                Log.i(TAG, "RTSP video stream callback -- video NAL units received.  bytes.length = " + bytes.length);
                if (decoderState == DecoderState.INIT) {
                    decoder.initH264Decoder(sps, pps);
                    decoderState = DecoderState.READY;
                }
                else if (decoderState == DecoderState.READY)
                    decoder.setRawH264Data(bytes);
            }

            @Override
            public void onRtspAudioSampleReceived(@NonNull @NotNull byte[] bytes, int i, int i1, long l) {
                //TODO : Decode the Audio Nal units (AAC encoded) received using audio decoder
                Log.d(TAG, "RTSP audio stream callback");
            }

            @Override
            public void onRtspDisconnected() {
                decoder.stopDecoder();
                Log.d(TAG, "Disconnected from RTSP server");
            }

            @Override
            public void onRtspFailedUnauthorized() {
                Log.d(TAG, "onRtspFailedUnauthorized");
            }

            @Override
            public void onRtspFailed(@androidx.annotation.Nullable @Nullable String s) {
                Log.d(TAG, "onRtspFailed");
            }
        };

        Uri uri = Uri.parse(rtspUrl);

        decoder = new H264Decoder(streamCb, height, width);
        if ("".equalsIgnoreCase(id)) {
            mRtspClient = new RtspClient.Builder(clientSocket, uri.toString(), exitFlag, clientListener)
                    .requestAudio(false)
                    .requestVideo(true)
                    .withDebug(true)
                    .withUserAgent("RTSP sample Client")
                    .build();
        } else {
            mRtspClient = new RtspClient.Builder(clientSocket, uri.toString(), exitFlag, clientListener)
                    .requestAudio(false)
                    .requestVideo(true)
                    .withCredentials(id, password)
                    .withDebug(true)
                    .withUserAgent("RTSP sample Client")
                    .build();
        }
    }

    /**
     * Method to start RTSP streaming
     */
    public void start() {
        mRtspClient.execute();
    }

    /**
     * Method to stop RTSP streaming
     */
    public void stop() {
        try {
            NetUtils.closeSocket(clientSocket);
            decoder.stopDecoder();
        } catch (Exception E) {
            Log.e(TAG, "Error closing socket");
        }
    }

    /**
     * Method to set RTSP URL
     *
     * @param url String for RTSP URL
     */
    public void setRtspUrl(String url) {
        rtspUrl = url;
    }

    /**
     * Method to set RTSP frame resolution
     *
     * @param height Height of the RTSP stream
     * @param width  Width of the RTSP stream
     */
    public void setResolution(int height, int width) {
        this.height = height;
        this.width = width;
    }
}
