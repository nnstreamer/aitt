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
package com.samsung.android.aittnative;

import android.util.Log;
import android.util.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * Jni Interface class as intermediate layer for android aitt and other transport modules to interact with JNI module.
 */
public class JniInterface {
    private static final String TAG = "JniInterface";
    private final Map<String, ArrayList<JniMessageCallback>> subscribeCallbacks = new HashMap<>();
    private final Map<String, Pair<Integer, JniDiscoveryCallback>> discoveryCallbacks = new HashMap<>();
    private JniConnectionCallback jniConnectionCallback;
    private long instance = 0;

    /*
      Load aitt-native library
     */
    static {
        try {
            System.loadLibrary("aitt-native");
        } catch (UnsatisfiedLinkError e) {
            // only ignore exception in non-android env
            if ("Dalvik".equals(System.getProperty("java.vm.name"))) throw e;
        }
    }

    /**
     * JNI callback interface to send data from JNI layer to Java layer(aitt or transport module)
     */
    public interface JniMessageCallback {
        void onDataReceived(String topic, byte[] data);
    }

    /**
     * JNI callback interface to send connection callback status to aitt layer
     */
    public interface JniConnectionCallback {
        void onConnectionStatusReceived(int status);
    }

    /**
     * JNI callback interface to receive discovery messages
     */
    public interface JniDiscoveryCallback {
        void onDiscoveryMessageReceived(String status, byte[] data);
    }

    /**
     * JNI interface API to initialize JNI module
     * @param id unique mqtt id
     * @param ip self IP address of device
     * @param clearSession to clear current session if client disconnects
     * @return returns the JNI instance object in long
     */
    public long init(String id, String ip, boolean clearSession) {
        instance = initJNI(id, ip, clearSession);
        return instance;
    }

    /**
     * JNI Interface API to connect to MQTT broker
     * @param brokerIp mqtt broker ip address
     * @param port mqtt broker port number
     */
    public void connect(String brokerIp, int port) {
        connectJNI(instance, brokerIp, port);
    }

    /**
     * JNI Interface API to subscribe to a topic
     * @param topic String to which applications can subscribe, to receive data
     * @param protocol Protocol supported by application, invoking subscribe
     * @param qos QoS at which the message should be delivered
     * @return returns the subscribe instance in long
     */
    public long subscribe(final String topic, JniMessageCallback callback, int protocol, int qos) {
        addCallBackToSubscribeMap(topic, callback);
        return subscribeJNI(instance, topic, protocol, qos);
    }

    /**
     * JNI Interface API to disconnect from broker
     */
    public void disconnect() {
        synchronized (this) {
            subscribeCallbacks.clear();
            discoveryCallbacks.clear();
            jniConnectionCallback = null;
        }
        disconnectJNI(instance);
    }

    /**
     * JNI Interface API to publish data to specified topic
     * @param topic String to which message needs to be published
     * @param data Byte message to be published
     * @param dataLen Size/length of the message to be published
     * @param protocol Protocol to be used to publish message
     * @param qos QoS at which the message should be delivered
     * @param retain Boolean to decide whether or not the message should be retained by the broker
     */
    public void publish(final String topic, final byte[] data, long dataLen, int protocol, int qos, boolean retain) {
        publishJNI(instance, topic, data, dataLen, protocol, qos, retain);
    }

    /**
     * JNI Interface API to unsubscribe the given topic
     * @param aittSubId Subscribe ID of the topics to be unsubscribed
     */
    public void unsubscribe(String topic, final long aittSubId) {
        synchronized (this) {
            subscribeCallbacks.remove(topic);
        }
        unsubscribeJNI(instance, aittSubId);
    }

    /**
     * JNI Interface API to set connection callback instance
     * @param cb callback instance of JniConnectionCallback interface
     */
    public void setConnectionCallback(JniConnectionCallback cb) {
        jniConnectionCallback = cb;
        setConnectionCallbackJNI(instance);
    }

    /**
     * JNI Interface API to set discovery callback
     * @param topic String for which discovery information is required
     * @param callback callback instance of JniDiscoveryCallback interface
     */
    public void setDiscoveryCallback(String topic, JniDiscoveryCallback callback) {
        int cb = setDiscoveryCallbackJNI(instance, topic);
        synchronized (this) {
            discoveryCallbacks.put(topic, new Pair<>(cb, callback));
        }
    }

    /**
     * JNI Interface API to remove discovery callback
     * @param topic String for which discovery information is not required
     */
    public void removeDiscoveryCallback(String topic) {
        synchronized (this) {
            Pair<Integer, JniDiscoveryCallback> pair = discoveryCallbacks.get(topic);
            if (pair != null) {
                removeDiscoveryCallbackJNI(instance, pair.first);
            }
            discoveryCallbacks.remove(topic);
        }
    }

    /**
     * JNI Interface API to update discovery message
     * @param topic String for which discovery information is to be updated
     * @param discoveryMessage ByteArray containing discovery information
     */
    public void updateDiscoveryMessage(String topic, byte[] discoveryMessage) {
        updateDiscoveryMessageJNI(instance, topic, discoveryMessage, discoveryMessage.length);
    }

    /**
     * messageCallback API to receive data from JNI layer to JNI interface layer
     * @param topic Topic to which data is received
     * @param payload Data that is sent from JNI to JNI interface layer
     */
    void messageCallback(String topic, byte[] payload) {
        try {
            synchronized (this) {
                ArrayList<JniMessageCallback> cbList = subscribeCallbacks.get(topic);

                if (cbList != null) {
                    for (JniMessageCallback cb : cbList) {
                        cb.onDataReceived(topic, payload);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during messageReceived", e);
        }
    }

    /**
     * connectionStatusCallback API to receive connection status from JNI to JNI interface layer
     * @param status status of the device connection with mqtt broker
     */
    void connectionStatusCallback(int status) {
        if (jniConnectionCallback != null) {
            jniConnectionCallback.onConnectionStatusReceived(status);
        }
    }

    void discoveryMessageCallback(String topic, String status, byte[] message) {
        synchronized (this) {
            Pair<Integer, JniDiscoveryCallback> pair = discoveryCallbacks.get(topic);
            if (pair != null) {
                pair.second.onDiscoveryMessageReceived(status, message);
            }
        }
    }

    /**
     * Method to map JNI callback instance to topic
     *
     * @param topic    String to which application can subscribe
     * @param callback JniInterface callback instance created during JNI subscribe call
     */
    private void addCallBackToSubscribeMap(String topic, JniMessageCallback callback) {
        synchronized (this) {
            try {
                ArrayList<JniMessageCallback> cbList = subscribeCallbacks.get(topic);

                if (cbList != null) {
                    // check whether the list already contains same callback
                    if (!cbList.contains(callback)) {
                        cbList.add(callback);
                    }
                } else {
                    cbList = new ArrayList<>();
                    cbList.add(callback);
                    subscribeCallbacks.put(topic, cbList);
                }
            } catch (Exception e) {
                Log.e(TAG, "Error during JNI callback add", e);
            }
        }
    }

    /* native API set */
    /* Native API to initialize JNI */
    private native long initJNI(String id, String ip, boolean clearSession);

    /* Native API for connecting to broker */
    private native void connectJNI(long instance, final String host, int port);

    /* Native API for disconnecting from broker */
    private native void disconnectJNI(long instance);

    /* Native API for setting connection callback */
    private native void setConnectionCallbackJNI(long instance);

    /* Native API for setting discovery callback */
    private native int setDiscoveryCallbackJNI(long instance, final String topic);

    /* Native API for removing discovery callback */
    private native void removeDiscoveryCallbackJNI(long instance, int cbHandle);

    /* Native API for updating discovery message */
    private native void updateDiscoveryMessageJNI(long instance, final String topic, final byte[] data, long dataLen);

    /* Native API for publishing to a topic */
    private native void publishJNI(long instance, final String topic, final byte[] data, long dataLen, int protocol, int qos, boolean retain);

    /* Native API for subscribing to a topic */
    private native long subscribeJNI(long instance, final String topic, int protocol, int qos);

    /* Native API for unsubscribing a topic */
    private native void unsubscribeJNI(long instance, final long aittSubId);
}
