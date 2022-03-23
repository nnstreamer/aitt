package com.samsung.android.modules.webrtc;

import android.content.Context;
import android.os.Looper;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class WebRTCServer {
    private WebRTC.DataType dataType;
    private ServerSocket serverSocket;
    private Context appContext;
    private WebRTC.ReceiveDataCallback dataCallback;
    private List<WebRTC> connectionList = new ArrayList<>();
    private Thread serverThread;
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
        serverThread.start();
        return serverSocket.getLocalPort();
    }

    public void stop(){
        try {
            serverThread.stop();
            serverSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        for(WebRTC web : connectionList){
            web.disconnect();
        }
    }

    private class ServerThread extends Thread{

        @Override
        public void run() {
            Looper.prepare();
            while(true){
                try {
                    Socket socket = serverSocket.accept();
                    WebRTC web = new WebRTC(dataType , appContext , socket);
                    web.connect();
                    web.registerDataCallback(dataCallback);
                    connectionList.add(web);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
