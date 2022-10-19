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
package com.samsung.android.modules.ipc;

import android.content.Context;
import android.util.Log;

import com.newtronlabs.sharedmemory.IRemoteSharedMemory;
import com.newtronlabs.sharedmemory.RemoteMemoryAdapter;
import com.newtronlabs.sharedmemory.SharedMemoryProducer;
import com.newtronlabs.sharedmemory.prod.memory.ISharedMemory;

import java.io.IOException;
import java.nio.ByteBuffer;

public class Ipc {
    private static final String TAG = "IPC_ANDROID";
    private static final String regionName = "Test-Region";
    private static final int sizeInBytes = 470000; // 640*480 frame size
    private Consumer consumer = null;
    private Thread thread = null;
    private int frameCount = 1;
    private static final int frameIndex = 4;
    private static final int getFrameCountIndex = 460900;
    private final byte[] byteLargeArray = new byte[470000];
    private final Context appContext;
    private IRemoteSharedMemory remoteMemory;
    private ISharedMemory sharedMemory;
    private RecieveFrameCallback frameCallback;
    private boolean closeIpcMemory = false;

    public Ipc(Context context, RecieveFrameCallback frameCallback) {
        this.frameCallback = frameCallback;
        this.appContext = context;
    }

    public Ipc(Context context) {
        this.appContext = context;
    }

    public interface RecieveFrameCallback {
        void pushFrame(byte[] frame);
    }

    private class Consumer implements Runnable {
        @Override
        public void run() {
            String producerAppId = "com.example.androidclient";
            byte[] dataBytes;
            int globalFrameCount = -1 ;
            int consumerFrameCount;

            while (true) {
                remoteMemory = RemoteMemoryAdapter.getDefaultAdapter().getSharedMemory(appContext, producerAppId, regionName);
                if (remoteMemory == null) {
                    Log.d(TAG, "Failed to access shared memory");
                } else {
                    Log.d(TAG, "Shared memory present");
                    dataBytes = new byte[remoteMemory.getSize()];
                    break;
                }
            }

            byte[] sample = new byte[4];

            while (!closeIpcMemory) {
                try {
                    remoteMemory.readBytes(dataBytes, 0, 0, dataBytes.length);
                    System.arraycopy(dataBytes, getFrameCountIndex, sample, 0, frameIndex);
                    consumerFrameCount = ByteBuffer.wrap(sample).getInt();

                    if (globalFrameCount != consumerFrameCount) {
                        Log.d(TAG, "New frame received : " + globalFrameCount + "&" + consumerFrameCount);
                        frameCallback.pushFrame(dataBytes);
                        globalFrameCount = consumerFrameCount;
                        Thread.sleep(20);
                    } else {
                        Log.d(TAG, "Repeated frame received : " + globalFrameCount + "&" + consumerFrameCount);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Exception thrown while reading shared memory");
                }
            }
        }
    }

    public void initProducer() {
        try {
            sharedMemory = SharedMemoryProducer.getInstance().allocate(regionName, sizeInBytes);
            if (sharedMemory == null) {
                Log.d(TAG, "No memory is allocated");
            } else {
                Log.d(TAG, "Memory allocated : " + sharedMemory.length());
            }
        } catch (IOException ex) {
            Log.e(TAG,"Exception thrown while shared memory creation");
        }
    }

    public void writeToMemory(byte[] byteArray) {
        try {
            System.arraycopy(byteArray, 0, byteLargeArray, 0, byteArray.length);
            ByteBuffer buffer = ByteBuffer.allocate(4);
            buffer.putInt(frameCount);
            System.arraycopy(buffer.array(), 0, byteLargeArray, getFrameCountIndex, 4);
            sharedMemory.writeBytes(byteLargeArray, 0, 0, byteLargeArray.length);
            frameCount++;
        } catch (Exception e) {
            Log.e(TAG, "Exception thrown while writing to shared memory");
        }
    }

    public void close() {
        try {
            closeIpcMemory = true;
            Log.d(TAG, "Closing IPC memory : " + closeIpcMemory);
            if (sharedMemory != null) {
                sharedMemory.close();
            }
            if (remoteMemory != null) {
                remoteMemory.close();
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to close shared memory resources");
        }
    }

    public void initConsumer() {
        consumer = new Consumer();
        thread = new Thread(consumer);
        thread.start();
    }
}
