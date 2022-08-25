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
package com.samsung.android.aitt;

import android.content.Context;

import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.modules.webrtc.WebRTC;
import com.samsung.android.modules.webrtc.WebRTCServer;

import java.nio.ByteBuffer;

class WebRTCHandler implements TransportHandler {
    private Context appContext;
    private String ip;
    private byte[] publishData;
    private WebRTC webrtc;
    private WebRTCServer ws;
    //ToDo - For now using sample app parameters, later fetch frameWidth & frameHeight from app
    private final Integer frameWidth = 640;
    private final Integer frameHeight = 480;

    @Override
    public void setAppContext(Context appContext) {
        this.appContext = appContext;
    }

    @Override
    public void setSelfIP(String ip) {
        this.ip = ip;
    }

    @Override
    public void subscribe(String topic, HandlerDataCallback handlerDataCallback) {
        WebRTC.ReceiveDataCallback cb = data -> {
            handlerDataCallback.pushHandlerData(data);
        };
        WebRTC.DataType dataType = topic.endsWith(Definitions.RESPONSE_POSTFIX) ? WebRTC.DataType.MESSAGE : WebRTC.DataType.VIDEOFRAME;
        ws = new WebRTCServer(appContext, cb);
        int serverPort = ws.start();
        if (serverPort < 0) {
            throw new IllegalArgumentException("Failed to start webRTC server-socket");
        }

        publishData = wrapPublishData(topic, serverPort);
    }

    @Override
    public byte[] getPublishData() {
        return publishData;
    }

    /**
     * Method to wrap topic, device IP address, webRTC server instance port number for publishing
     * @param topic Topic to which the application has subscribed to
     * @param serverPort Port number of the WebRTC server instance
     * @return Byte data wrapped, contains topic, device IP, webRTC server port number
     */
    private byte[] wrapPublishData(String topic, int serverPort) {
        FlexBuffersBuilder fbb = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        {
            int smap = fbb.startMap();
            fbb.putString(Definitions.STATUS, Definitions.JOIN_NETWORK);
            fbb.putString("host", ip);
            {
                int smap1 = fbb.startMap();
                fbb.putInt("protocol", Aitt.Protocol.WEBRTC.getValue());
                fbb.putInt("port", serverPort);
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
        WebRTC.DataType dataType = topic.endsWith(Definitions.RESPONSE_POSTFIX) ? WebRTC.DataType.MESSAGE : WebRTC.DataType.VIDEOFRAME;
        if (webrtc == null) {
            webrtc = new WebRTC(appContext);
            webrtc.connect(ip, port);
        }
        if (dataType == WebRTC.DataType.MESSAGE) {
            webrtc.sendMessageData(message);
        } else if (dataType == WebRTC.DataType.VIDEOFRAME) {
            webrtc.sendVideoData(message, frameWidth, frameHeight);
        }
    }

    @Override
    public void unsubscribe() {
        ws.stop();
    }

    @Override
    public void disconnect() {

    }
}
