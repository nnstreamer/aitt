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
package com.samsung.android.aitt.handler;

import android.content.Context;
import android.util.Log;
import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.Definitions;
import com.samsung.android.modules.ipc.Ipc;

import java.nio.ByteBuffer;

public class IpcHandler implements TransportHandler {

    private static final String TAG = "IpcHandler";
    private Context context;
    private String ip;
    private Ipc ipc;
    private byte[] publishData;

    public IpcHandler() {
        //ToDo : Copy jni interface and use to communicate with JNI
    }

    @Override
    public void setAppContext(Context appContext) {
        context = appContext;
    }

    @Override
    public void setSelfIP(String ip) {
        this.ip = ip;
    }

    @Override
    public byte[] getPublishData() {
        return publishData;
    }

    @Override
    public void subscribe(String topic, HandlerDataCallback handlerDataCallback) {
        publishData = wrapPublishData(topic);
        try {
            Ipc.ReceiveFrameCallback cb = handlerDataCallback::pushHandlerData;
            ipc = new Ipc(context, cb);
            ipc.initConsumer();

        } catch (Exception e) {
            Log.e(TAG, "Failed to subscribe to IPC");
        }
    }

    /**
     * Method to wrap topic, device IP address, webRTC server instance port number for publishing
     *
     * @param topic      Topic to which the application has subscribed to
     * @return Byte data wrapped, contains topic, device IP, webRTC server port number
     */
    private byte[] wrapPublishData(String topic) {
        FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        {
            int smap = fbb.startMap();
            fbb.putString(Definitions.STATUS, Definitions.JOIN_NETWORK);
            fbb.putString("host", ip);
            {
                int smap1 = fbb.startMap();
                fbb.putInt("protocol", Aitt.Protocol.IPC.getValue());
                fbb.putInt("port", Definitions.DEFAULT_IPC_PORT);
                fbb.endMap(topic, smap1);
            }
            fbb.endMap(null, smap);
        }
        ByteBuffer buffer = fbb.finish();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data, 0, data.length);
        return data;
    }

    @Override
    public void publish(String topic, String ip, int port, byte[] message) {
        if (ipc == null)
        {
            ipc = new Ipc(context);
            ipc.initProducer();
        }
        ipc.writeToMemory(message);
    }

    @Override
    public void unsubscribe() {
        if (ipc != null)
            ipc.close();
    }

    @Override
    public void disconnect() {
        //ToDo : Implement disconnect method
    }
}
