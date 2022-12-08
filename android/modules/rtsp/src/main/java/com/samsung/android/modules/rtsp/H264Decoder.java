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
import android.util.Log;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Class to implement H264 decoder functionalities
 */
public class H264Decoder {

    private String TAG = "H264Decoder";
    private ByteBuffer iFrame;
    private final static int width = 240;  //TODO : Modify width and height based on discovery message
    private final static int height = 320;
    private AtomicBoolean exitFlag;
    private RTSPClient.ReceiveDataCallback streamCb;

    private MediaCodec mCodec;

    // Async task that takes H264 frames and uses the decoder to update the Surface Texture
    private DecodeFramesTask mFrameTask;

    /**
     * H264Decoder constructor
     * @param cb data callback to send data to application
     * @param exitFlag flag to begin/terminate decoder execution
     */
    public H264Decoder(RTSPClient.ReceiveDataCallback cb, AtomicBoolean exitFlag) {
        streamCb = cb;
        this.exitFlag = exitFlag;
    }

    /**
     * Method to initialize H264 decoder object
     * @param sps Sequence parameter set to set codec format
     * @param pps Picture parameter set to set codec format
     */
    public void initH264Decoder(byte[] sps, byte[] pps) {
        //Create the format settings for the MediaCodec
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
            // Configure the codec
            mCodec.configure(format, null, null, 0);
            // Start the codec
            mCodec.start();
            Log.d(TAG, "initH264Decoder done");
            // Create the AsyncTask to get the frames and decode them using the Codec
            mFrameTask = new DecodeFramesTask();
            Thread thread = new Thread(mFrameTask);
            thread.start();
        } catch (Exception e){
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
        exitFlag.set(true);
        try {
            mCodec.stop();
            mCodec.release();
        } catch (Exception e) {
            Log.e(TAG, "Failed to release decoder");
        }
    }

    /**
     * A task to decode frames on a different thread
     */
    private class DecodeFramesTask implements Runnable {

        @Override
        public void run() {

            while (!exitFlag.get()) {
                byte[] frame = nextFrame();

                if (frame == null || frame.length == 0)
                    continue;

                // clear iFrame to avoid redundant frame decoding
                clearFrame();
                // Now we need to give it to the Codec to decode the frame

                // Get the input buffer from the decoder
                // Pass in -1 here as in this example we don't have a playback time reference
                int inputIndex = mCodec.dequeueInputBuffer(-1);
                // If the buffer number is valid use the buffer with that index
                if (inputIndex >= 0) {
                    ByteBuffer buffer = mCodec.getInputBuffer(inputIndex);

                    try {
                        buffer.put(frame);
                    } catch(NullPointerException e) {
                        Log.e(TAG, "Failed to put frame to buffer");
                    }

                    // Tell the decoder to process the frame
                    mCodec.queueInputBuffer(inputIndex, 0, frame.length, 0, 0);
                }

                // Stop decoder execution
                if (exitFlag.get())
                    break;

                MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
                int outputIndex = mCodec.dequeueOutputBuffer(info, 5000);

                if (outputIndex >= 0) {
                    ByteBuffer outputBuffer = mCodec.getOutputBuffer(outputIndex);
                    outputBuffer.position(info.offset);
                    outputBuffer.limit(info.offset + info.size);
                    byte[] arr = new byte[outputBuffer.remaining()];
                    outputBuffer.get(arr);

                    if(arr.length != 0) {
                        Log.d(TAG, "Decoding of frame is completed");
                        streamCb.pushData(arr);
                    }
                    mCodec.releaseOutputBuffer(outputIndex, true);
                }

                // wait for the next frame to be ready, the server/IP camera supports ~30fps
                try {
                    Thread.sleep(33);
                } catch (Exception e) {
                    Log.e(TAG, "Failed to put thread sleep");
                }
            }
        }
    }
}
