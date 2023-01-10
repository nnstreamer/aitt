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

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;

import androidx.annotation.NonNull;

import java.nio.ByteBuffer;

/**
 * Class to implement H264 decoder functionalities
 */
public class H264Decoder {

    private static final String TAG = "H264Decoder";
    private ByteBuffer iFrame;
    private final int width;
    private final int height;
    private final RTSPClient.ReceiveDataCallback streamCb;
    private MediaCodec mCodec;
    private Handler inputBufferHandler = null;
    private Handler outputBufferHandler = null;
    private HandlerThread inputBufferThread = null;
    private HandlerThread outputBufferThread = null;

    /**
     * H264Decoder constructor
     * @param cb data callback to send data to application
     * @param height height of rtsp frames
     * @param width width of rtsp frames
     */
    public H264Decoder(RTSPClient.ReceiveDataCallback cb, int height, int width) {
        streamCb = cb;
        this.height = height;
        this.width = width;
    }

    /**
     * Method to initialize H264 decoder object
     * @param sps Sequence parameter set to set codec format
     * @param pps Picture parameter set to set codec format
     */
    public void initH264Decoder(byte[] sps, byte[] pps) {
        // Create Input thread handler
        inputBufferThread = new HandlerThread("inputBufferThread", Process.THREAD_PRIORITY_DEFAULT);
        inputBufferThread.start();
        inputBufferHandler = new Handler(inputBufferThread.getLooper());
        // Create Output thread handler
        outputBufferThread = new HandlerThread("outputBufferThread", Process.THREAD_PRIORITY_DEFAULT);
        outputBufferThread.start();
        outputBufferHandler = new Handler(outputBufferThread.getLooper());
        // Create the format settings for the MediaCodec
        MediaFormat format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
        // Set the SPS frame
        format.setByteBuffer("csd-0", ByteBuffer.wrap(sps));
        // Set the PPS frame
        format.setByteBuffer("csd-1", ByteBuffer.wrap(pps));
        // Set the buffer size
        format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, width * height);

        try {
            // Get an instance of MediaCodec and give it its Mime type
            mCodec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
            // Set a callback for codec to receive input and output buffer free indexes
            mCodec.setCallback(new MediaCodec.Callback() {
                @Override
                public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {
                    inputBufferHandler.post(() -> {
                        try {
                            ByteBuffer buffer = mCodec.getInputBuffer(i);
                            byte[] frame;
                            //TODO:Wait here till u get a frames, we can use lock/unlock here instead of loop ?
                            do {
                                frame = nextFrame();
                            } while (frame == null);
                            buffer.put(frame);
                            clearFrame();
                            // Inform decoder to process the frame
                            mCodec.queueInputBuffer(i, 0, frame.length, 0, 0);
                        } catch (Exception e) {
                            Log.e(TAG, "Failed to provide NAL units to decoder");
                        }
                    });
                }

                @Override
                public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int i, @NonNull MediaCodec.BufferInfo bufferInfo) {
                    outputBufferHandler.post(() -> {
                        try {
                            // Decoded frames are available in output buffer get it using index i
                            ByteBuffer outputBuffer = mCodec.getOutputBuffer(i);
                            byte[] arr = new byte[bufferInfo.size - bufferInfo.offset];
                            outputBuffer.get(arr);
                            mCodec.releaseOutputBuffer(i, true);
                            if (arr.length != 0) {
                                Log.d(TAG, "Decoding of frame is completed");
                                streamCb.pushData(arr);
                            }
                        } catch (Exception e) {
                            Log.e(TAG, "Failed to get decoded frames from decoder");
                        }
                    });
                }

                @Override
                public void onError(@NonNull MediaCodec mediaCodec, @NonNull MediaCodec.CodecException e) {
                    Log.e(TAG, "onError,please check codec error");
                }

                @Override
                public void onOutputFormatChanged(@NonNull MediaCodec mediaCodec, @NonNull MediaFormat mediaFormat) {
                    Log.i(TAG, "onOutputFormatChanged,please handle this changed format");
                }
            });
            // Configure the codec
            mCodec.configure(format, null, null, 0);
            // Start the codec
            mCodec.start();
            Log.d(TAG, "initH264Decoder done");
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize decoder");
        }
    }

    /**
     * Method to receive encoded video NAL units
     * @param data data received in encoded format
     */
    public void setRawH264Data(byte[] data) {
        //Todo: Implement a Queue to store encoded frames
        try {
            synchronized (this) {
                iFrame = ByteBuffer.wrap(data);
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to copy iFrame data");
        }
    }

    /**
     * Method to make iFrame null once its content is used.
     */
    private void clearFrame() {
        try {
            synchronized (this) {
                iFrame = null;
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to clear iFrame data");
        }
    }

    /**
     * Method to get next frame
     * @return encoded frame data
     */
    private byte[] nextFrame() {
        synchronized (this) {
            if (iFrame != null)
                return iFrame.array();
            else
                return null;
        }
    }

    /**
     * Method to stop decoder once execution is finished
     */
    public void stopDecoder() {
        Log.d(TAG, "stopDecoders is invoked");
        try {
            if (inputBufferThread != null) {
                inputBufferThread.quit();
                inputBufferThread = null;
                inputBufferHandler = null;
            }
            if (outputBufferThread != null) {
                outputBufferThread.quit();
                outputBufferThread = null;
                outputBufferHandler = null;
            }
            mCodec.stop();
            mCodec.release();
        } catch (Exception e) {
            Log.e(TAG, "Failed to release decoder");
        }
    }
}
