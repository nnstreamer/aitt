package com.samsung.android.aitt.stream;

import android.content.Context;
import android.util.Log;

import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.Definitions;
import com.samsung.android.aitt.handler.ModuleHandler;
import com.samsung.android.aittnative.JniInterface;
import com.samsung.android.modules.webrtc.WebRTC;
import com.samsung.android.modules.webrtc.WebRTCServer;

import java.nio.ByteBuffer;

public final class WebRTCStream implements AittStream {

    private static final String TAG = "WebRTCStream";

    private final String topic;
    private final StreamRole streamRole;

    private int serverPort;
    private String ip;
    private JniInterface jniInterface;
    private WebRTC webrtc;
    private WebRTCServer ws = null;
    private StreamState streamState = StreamState.INIT;

    WebRTCStream(String topic, StreamRole streamRole, Context context) {
        this.topic = topic;
        this.streamRole = streamRole;

        if (streamRole == StreamRole.PUBLISHER)
            webrtc = new WebRTC(context);       // TODO: Currently, native stream servers are publishers.
        else
            ws = new WebRTCServer(context);     // TODO: Currently, native stream clients are subscribers.
    }

    public static WebRTCStream createSubscriberStream(String topic, StreamRole streamRole, Context context) {
        if (streamRole != StreamRole.SUBSCRIBER)
            throw new IllegalArgumentException("The role of this stream is not subscriber.");

        return new WebRTCStream(topic, streamRole, context);
    }

    public static WebRTCStream createPublisherStream(String topic, StreamRole streamRole, Context context) {
        if (streamRole != StreamRole.PUBLISHER)
            throw new IllegalArgumentException("The role of this stream is not publisher.");

        return new WebRTCStream(topic, streamRole, context);
    }

    @Override
    public void setConfig() {

    }

    @Override
    public void start() {
        if (streamRole == StreamRole.SUBSCRIBER) {
            serverPort = ws.start();
            if (serverPort < 0)
                throw new RuntimeException("Failed to start a WebRTC server socket.");
            Log.d(TAG, "Start a WebRTC Server, port = " + serverPort);

            byte[] publishData = wrapPublishData(topic, serverPort);
            jniInterface.publish(Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, publishData, publishData.length, Aitt.Protocol.MQTT.getValue(), Aitt.QoS.EXACTLY_ONCE.ordinal(), true);
        }
    }

    @Override
    public void disconnect() {
        if (webrtc != null)
            webrtc.disconnect();
    }

    @Override
    public void stop() {
        if (ws != null)
            ws.stop();
    }

    @Override
    public void setStateCallback() {

    }

    @Override
    public void setReceiveCallback(ModuleHandler.HandlerDataCallback handlerDataCallback) {
        if (handlerDataCallback == null)
            throw new IllegalArgumentException("The given callback is null.");

        if (streamRole == StreamRole.SUBSCRIBER) {
            WebRTC.ReceiveDataCallback cb = handlerDataCallback::pushHandlerData;
            ws.setDataCallback(cb);
        } else if (streamRole == StreamRole.PUBLISHER) {
            Log.e(TAG, "Invalid function call");
        }
    }

    public int getServerPort() {
        return serverPort;
    }

    public void setSelfIP(String ip) {
        this.ip = ip;
    }

    /**
     * Method to wrap topic, device IP address, and the port number of a webRTC server instance for publishing
     *
     * @param topic      Topic to which the application has subscribed to
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
    public boolean publish(String topic, String ip, int port, byte[] message) {
        if (streamRole == StreamRole.PUBLISHER) {
            if (webrtc == null) {
                Log.e(TAG, "A WebRTC instance is null.");
                return false;
            }

            if (streamState == StreamState.INIT) {
                webrtc.connect(ip, port);
                streamState = StreamState.READY;
                Log.d(TAG, "A WebRTC client is connected into a server.");
            }

            if (topic.endsWith(Definitions.RESPONSE_POSTFIX)) {
                Log.d(TAG, "A message is sent through a WebRTC publisher stream.");
                return webrtc.sendMessageData(message);
            } else {
                Log.d(TAG, "Video data are sent through a WebRTC publisher stream.");
                //ToDo - For now using sample app parameters, later fetch frameWidth & frameHeight from app
                int frameWidth = 640;
                int frameHeight = 480;
                webrtc.sendVideoData(message, frameWidth, frameHeight);
            }
        } else {
            Log.e(TAG, "publish() is not allowed to a subscriber stream.");
        }

        return true;
    }

    public void setJNIInterface(JniInterface jniInterface) {
        this.jniInterface = jniInterface;
    }
}