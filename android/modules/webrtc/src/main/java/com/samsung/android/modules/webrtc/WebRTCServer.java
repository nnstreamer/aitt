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

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class WebRTCServer {
    private WebRTC.DataType dataType;
    private ServerSocket serverSocket = null;
    private Context appContext;
    private WebRTC.ReceiveDataCallback dataCallback;
    private List<WebRTC> connectionList = new ArrayList<>();
    private ServerThread serverThread = null;
    private Thread thread = null;

    public WebRTCServer(Context appContext, WebRTC.DataType dataType, WebRTC.ReceiveDataCallback dataCallback){
        this.appContext = appContext;
        this.dataType = dataType;
        this.dataCallback = dataCallback;
    }

    // Returns Port Number
    public int start(){
        try {
            serverSocket = new ServerSocket(0);
        } catch (IOException e) {
            e.printStackTrace();
            return -1;
        }
        serverThread = new ServerThread();
        thread = new Thread(serverThread);
        thread.start();
        return serverSocket.getLocalPort();
    }

    public void stop(){
        if (serverThread != null) {
            serverThread.stop();
        }
        try {
            if (serverSocket != null) {
                serverSocket.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        for(WebRTC web : connectionList){
            web.disconnect();
        }
    }

    private class ServerThread implements Runnable{
        private volatile boolean isRunning = true;

        @Override
        public void run() {
            while(isRunning){
                try {
                    Socket socket = serverSocket.accept();
                    WebRTC web = new WebRTC(dataType , appContext , socket);
                    web.connect();
                    web.registerDataCallback(dataCallback);
                    connectionList.add(web);
                } catch (IOException e) {
                    e.printStackTrace();
                    isRunning = false;
                }
            }
        }

        public void stop() {
            isRunning = false;
        }
    }
}
