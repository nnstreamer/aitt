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

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

/**
 * Class to implement WebRTC server related functionalities
 */
public final class WebRTCServer {

    private static final String TAG = "WebRTCServer";

    private final Context appContext;
    private final List<WebRTC> connectionList = new ArrayList<>();

    private WebRTC.ReceiveDataCallback dataCallback;
    private ServerSocket serverSocket = null;
    private ServerThread serverThread = null;
    private Thread thread;

    /**
     * WebRTCServer constructor to create its instance
     *
     * @param appContext Application context of the app creating WebRTCServer instance
     */
    public WebRTCServer(Context appContext) {
        this.appContext = appContext;
    }

    /**
     * Setter to set a WebRTC ReceiveDataCallback
     *
     * @param dataCallback Data callback object to create call back mechanism
     */
    public void setDataCallback(WebRTC.ReceiveDataCallback dataCallback) {
        this.dataCallback = dataCallback;
    }

    /**
     * Method to start WebRTCServer instance
     *
     * @return Returns Port number on success and -1 on failure
     */
    public int start() {
        if (dataCallback == null) {
            Log.e(TAG, "Data callback is null.");
            return -1;
        }

        try {
            serverSocket = new ServerSocket(0);
        } catch (IOException e) {
            Log.e(TAG, "Error during start", e);
            return -1;
        }
        serverThread = new ServerThread();
        thread = new Thread(serverThread);
        thread.start();
        return serverSocket.getLocalPort();
    }

    /**
     * Method to stop running WebRTC server instance
     */
    public void stop() {
        if (serverThread != null) {
            serverThread.stop();
        }
        try {
            if (serverSocket != null) {
                serverSocket.close();
            }
        } catch (IOException e) {
            Log.e(TAG, "Error during stop", e);
        }
        for (WebRTC web : connectionList) {
            web.disconnect();
        }
    }

    /**
     * Class to implement a server thread
     */
    private class ServerThread implements Runnable {
        private volatile boolean isRunning = true;

        @Override
        public void run() {
            while (isRunning) {
                try {
                    Socket socket = serverSocket.accept();
                    WebRTC web = new WebRTC(appContext, socket);
                    web.connect();
                    web.registerDataCallback(dataCallback);
                    connectionList.add(web);
                } catch (IOException e) {
                    isRunning = false;
                    Log.e(TAG, "Error during run", e);
                }
            }
        }

        public void stop() {
            isRunning = false;
        }
    }
}
