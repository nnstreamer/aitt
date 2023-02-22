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

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;

import androidx.annotation.NonNull;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class AudioDecoder {
    private static final String TAG = "AudioDecoder";
    private final int sampleRate;
    private final int channelCount;
    private final byte[] codecConfig;
    private int outChannel;
    private AudioTrack audioTrack;
    private MediaCodec mCodec;

    private Handler audioInputBufferHandler = null;
    private Handler audioOutputBufferHandler = null;
    private HandlerThread audioInputBufferThread = null;
    private HandlerThread audioOutputBufferThread = null;

    private final BlockingQueue<byte[]> queue = new LinkedBlockingQueue<>();

    public AudioDecoder(int audioSampleRate, int audioChannelCount, byte[] audioCodecConfig) {
        sampleRate = audioSampleRate;
        channelCount = audioChannelCount;
        if (audioCodecConfig == null)
            codecConfig = new byte[0];
        else
            codecConfig = Arrays.copyOf(audioCodecConfig, audioCodecConfig.length);
    }

    public void initAudioDecoder() {
        // Create Input thread handler
        audioInputBufferThread = new HandlerThread("audioInputBufferThread", Process.THREAD_PRIORITY_DEFAULT);
        audioInputBufferThread.start();
        audioInputBufferHandler = new Handler(audioInputBufferThread.getLooper());
        // Create Output thread handler
        audioOutputBufferThread = new HandlerThread("audioOutputBufferThread", Process.THREAD_PRIORITY_DEFAULT);
        audioOutputBufferThread.start();
        audioOutputBufferHandler = new Handler(audioOutputBufferThread.getLooper());

        MediaFormat format = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRate, channelCount);

        byte[] csd0 = this.codecConfig;
        if(csd0 == null)
            csd0 = getAacDecoderConfigData(MediaCodecInfo.CodecProfileLevel.AACObjectLC, sampleRate, channelCount);

        format.setByteBuffer("csd-0", ByteBuffer.wrap(csd0));
        format.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);

        try {
            // Get an instance of MediaCodec and give it its Mime type
            mCodec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_AUDIO_AAC);
            // Set a callback for codec to receive input and output buffer free indexes
            mCodec.setCallback(new MediaCodec.Callback() {
                @Override
                public void onInputBufferAvailable(@NonNull MediaCodec mediaCodec, int i) {
                    audioInputBufferHandler.post(() -> {
                        try {
                            // Execute once data is available from the queue
                            byte[] frame = queue.take();
                            ByteBuffer buffer = mCodec.getInputBuffer(i);
                            buffer.put(frame, 0, frame.length);
                            // Inform decoder to process the frame
                            mCodec.queueInputBuffer(i, 0, frame.length, 0, 0);
                        } catch (Exception e) {
                            Log.e(TAG, "Failed to provide NAL units to decoder");
                        }
                    });
                }

                @Override
                public void onOutputBufferAvailable(@NonNull MediaCodec mediaCodec, int i, @NonNull MediaCodec.BufferInfo bufferInfo) {
                    audioOutputBufferHandler.post(() -> {
                        try {
                            //Decoded frames are available in output buffer get it using index i
                            ByteBuffer outputBuffer = mCodec.getOutputBuffer(i);
                            byte[] chunk = new byte[bufferInfo.size];
                            outputBuffer.get(chunk);
                            outputBuffer.clear();
                            mCodec.releaseOutputBuffer(i, true);
                            if (chunk.length != 0) {
                                Log.d(TAG, "Decoding of audio frame is completed");
                                audioTrack.write(chunk, bufferInfo.offset, bufferInfo.offset + bufferInfo.size);
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
            Log.d(TAG, "initAudioDecoder done");
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize audio decoder");
        }

        if (channelCount > 1)
            outChannel = AudioFormat.CHANNEL_OUT_STEREO;
        else
            outChannel = AudioFormat.CHANNEL_OUT_MONO;

        int outAudio = AudioFormat.ENCODING_PCM_16BIT;
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, outChannel, outAudio);
        Log.d(TAG, "sample rate : " + sampleRate + " outChannel : " + outChannel + " bufferSize : " + bufferSize);

        audioTrack = new AudioTrack(
                new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                        .build(),
                new AudioFormat.Builder()
                        .setEncoding(outAudio)
                        .setChannelMask(outChannel)
                        .setSampleRate(sampleRate)
                        .build(),
                bufferSize,
                AudioTrack.MODE_STREAM,
                0);

        audioTrack.play();
    }

    /**
     * Method to receive encoded video NAL units
     * @param data data received in encoded format
     */
    public void setRawAudioData(byte[] data) {
        try {
            Log.d(TAG, "Add audio frame to the Queue");
            queue.put(data);
        } catch (Exception e) {
            Log.e(TAG, "Failed to add audio frame to Queue");
        }
    }

    /**
     * Method to stop decoder once execution is finished
     */
    public void stopDecoder() {
        Log.d(TAG, "stopDecoders is invoked");
        try {
            if (audioInputBufferThread != null) {
                audioInputBufferThread.quit();
                audioInputBufferThread = null;
                audioInputBufferHandler = null;
            }
            if (audioOutputBufferThread != null) {
                audioOutputBufferThread.quit();
                audioOutputBufferThread = null;
                audioOutputBufferHandler = null;
            }
            mCodec.stop();
            mCodec.release();
            audioTrack.flush();
            audioTrack.release();
        } catch (Exception e) {
            Log.e(TAG, "Failed to release decoder");
        }
    }

    private byte[] getAacDecoderConfigData(int audioProfile, int sampleRate, int channels) {
        // AOT_LC = 2
        // 0001 0000 0000 0000
        int extraDataAac = audioProfile << 11;
        switch (sampleRate)
        {
            case 7350:
                extraDataAac |= 0x600; break;
            case 8000:
                extraDataAac |= 0x580; break;
            case 11025:
                extraDataAac |= 0x500; break;
            case 12000:
                extraDataAac |= 0x480; break;
            case 16000:
                extraDataAac |= 0x400; break;
            case 22050:
                extraDataAac |= 0x380; break;
            case 24000:
                extraDataAac |= 0x300; break;
            case 32000:
                extraDataAac |= 0x280; break;
            case 44100:
                extraDataAac |= 0x200; break;
            case 48000:
                extraDataAac |= 0x180; break;
            case 64000:
                extraDataAac |= 0x100; break;
            case 88200:
                extraDataAac |= 0x80; break;
            default:
                //Do nothing
        }

        extraDataAac |= channels << 3;
        byte[] extraData = new byte[2];
        extraData[0] = ((byte)((extraDataAac & 0xFF00) >> 8));
        extraData[1] = ((byte)(extraDataAac & 0xFF));
        return extraData;
    }
}
