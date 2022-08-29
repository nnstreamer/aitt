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
import android.util.Log;
import android.util.Pair;

import androidx.annotation.Nullable;

import com.google.flatbuffers.FlexBuffers;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;

/**
 * Class creates a Java layer to operate between application and JNI layer for AITT C++
 * 1. Connect to MQTT broker
 * 2. Subscribe to a topic with protocol and other params
 * 3. Publish to a topic using protocol and other params
 * 4. Unsubscribe to a topic
 * 5. Invoke JNI api's which interact with aitt c++
 * 6. API to set MQTT Connection Callback
 */
public class Aitt {
    private static final String TAG = "AITT_ANDROID";
    private static final String INVALID_TOPIC = "Invalid topic";

    /**
     * Load aitt-android library
     */
    static {
        try {
            System.loadLibrary("aitt-android");
        }catch (UnsatisfiedLinkError e){
            // only ignore exception in non-android env
            if ("Dalvik".equals(System.getProperty("java.vm.name"))) throw e;
        }
    }
    private Map<String, ArrayList<SubscribeCallback>> subscribeCallbacks = new HashMap<>();
    private Map<String, HostTable> publishTable = new HashMap<>();
    private Map<String, Pair<Protocol, Object>> subscribeMap = new HashMap<>();
    private Map<String, Long> aittSubId = new HashMap<>();
    private ConnectionCallback connectionCallback = null;

    private long instance = 0;
    private String ip;
    private Context appContext;

    /**
     * QoS levels to define the guarantee of delivery for a message
     */
    public enum QoS {
        AT_MOST_ONCE,   // Fire and forget
        AT_LEAST_ONCE,  // Receiver is able to receive multiple times
        EXACTLY_ONCE,   // Receiver only receives exactly once
    }

    /**
     * List of protocols supported by AITT framework
     */
    public enum Protocol {
        MQTT(0x1 << 0),    // Publish message through the MQTT
        TCP(0x1 << 1),     // Publish message to peers using the TCP
        UDP(0x1 << 2),     // Publish message to peers using the UDP
        SRTP(0x1 << 3),    // Publish message to peers using the SRTP
        WEBRTC(0x1 << 4),  // Publish message to peers using the WEBRTC
        IPC(0x1 << 5);     // Publish message to peers using the IPC

        private final int value;

        Protocol(int value) {
            this.value = value;
        }

        public int getValue() {
            return value;
        }

        public static Protocol fromInt(long value) {
            for (Protocol type : values()) {
                if (type.getValue() == value) {
                    return type;
                }
            }
            return null;
        }
    }

    /**
     * HostTable to store String and PortTable instances
     */
    private static class HostTable {
        Map<String, PortTable> hostMap = new HashMap<>();
    }

    /**
     * PortTable to store port with protocol, webRTC transportHandler object instance
     */
    private static class PortTable {
        Map<Integer, Pair<Protocol, Object>> portMap = new HashMap<>();
    }

    /**
     * Interface to implement connection status callback
     */
    public interface ConnectionCallback {
        void onConnected();
        void onDisconnected();
        void onConnectionFailed();
    }

    /**
     * Interface to implement callback method for subscribe call
     */
    public interface SubscribeCallback {
        void onMessageReceived(AittMessage message);
    }

    /**
     * Aitt constructor to create AITT object
     * @param appContext context of the application invoking the constructor
     * @param id Unique identifier for the Aitt instance
     */
    public Aitt(Context appContext, String id) throws InstantiationException {
        this(appContext, id, Definitions.AITT_LOCALHOST, false);
    }

    /**
     * Aitt constructor to create AITT object
     * @param appContext context of the application invoking the constructor
     * @param id Unique identifier for the Aitt instance
     * @param ip IP address of the device, on which application is running
     * @param clearSession "clear" the current session when the client disconnects
     */
    public Aitt(Context appContext, String id, String ip, boolean clearSession) throws InstantiationException {
        if (appContext == null) {
            throw new IllegalArgumentException("Invalid appContext");
        }
        if (id == null || id.isEmpty()) {
            throw new IllegalArgumentException("Invalid id");
        }
        instance = initJNI(id, ip, clearSession);
        if (instance == 0L) {
            throw new InstantiationException("Failed to instantiate native instance");
        }
        this.ip = ip;
        this.appContext = appContext;
    }

    /**
     * Method to set connection status callback
     * @param callback ConnectionCallback to which status should be updated
     */
    public void setConnectionCallback(ConnectionCallback callback) {
        if (callback == null) {
            throw new IllegalArgumentException("Invalid callback");
        }
        connectionCallback = callback;
        setConnectionCallbackJNI(instance);
    }

    /**
     * Method to connect to MQTT broker
     * @param brokerIp Broker IP address to which, device has to connect
     */
    public void connect(@Nullable String brokerIp) {
        connect(brokerIp, Definitions.AITT_PORT);
    }

    /**
     * Method to connect to MQTT broker
     * @param brokerIp Broker IP address to which, device has to connect
     * @param port Broker port number to which, device has to connect
     */
    public void connect(@Nullable String brokerIp, int port) {
        if (brokerIp == null || brokerIp.isEmpty()) {
            brokerIp = Definitions.AITT_LOCALHOST;
        }
        connectJNI(instance, brokerIp, port);
        //Subscribe to java discovery topic
        subscribeJNI(instance, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, Protocol.MQTT.getValue(), QoS.EXACTLY_ONCE.ordinal());
    }

    /**
     * Method to disconnect from MQTT broker
     */
    public void disconnect() {
        publishJNI(instance, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, new byte[0], 0, Protocol.MQTT.getValue(), QoS.AT_LEAST_ONCE.ordinal(), true);

        disconnectJNI(instance);
        try {
            close();
        } catch (Exception e) {
            Log.e(TAG, "Error during disconnect", e);
        }
    }

    /**
     * Method to publish message to a specific topic
     * @param topic String to which message needs to be published
     * @param message Byte message that needs to be published
     */
    public void publish(String topic, byte[] message) {
        EnumSet<Protocol> protocolSet = EnumSet.of(Protocol.MQTT);
        publish(topic, message, protocolSet, QoS.AT_MOST_ONCE, false);
    }

    /**
     * Method to publish message to a specific topic
     * @param topic String to which message needs to be published
     * @param message Byte message that needs to be published
     * @param protocol Protocol to be used to publish message
     * @param qos QoS at which the message should be delivered
     * @param retain Boolean to decide whether or not the message should be retained by the broker
     */
    public void publish(String topic, byte[] message, Protocol protocol, QoS qos, boolean retain) {
        EnumSet<Protocol> protocolSet = EnumSet.of(protocol);
        publish(topic, message, protocolSet, qos, retain);
    }

    /**
     * Method to publish message to a specific topic
     * @param topic String to which message needs to be published
     * @param message Byte message that needs to be published
     * @param protocols Protocol to be used to publish message
     * @param qos QoS at which the message should be delivered
     * @param retain Boolean to decide whether or not the message should be retained by the broker
     */
    public void publish(String topic, byte[] message, EnumSet<Protocol> protocols, QoS qos, boolean retain) {

        checkParams(topic, protocols);
        int jniProtocols = 0;

        for (Protocol p : protocols) {
            if (!classifyProtocol(p)) {
                jniProtocols += p.getValue();
                protocols.remove(p);
            }
        }

        if(jniProtocols > 0) {
            publishJNI(instance, topic, message, message.length, jniProtocols, qos.ordinal(), retain);
        }

        for(Protocol pro : protocols) {
            try {
                synchronized (this) {
                    if (!publishTable.containsKey(topic)) {
                        Log.e(TAG, "Invalid publish request over unsubscribed topic");
                        return;
                    }
                    HostTable hostTable = publishTable.get(topic);
                    for (String hostIp : hostTable.hostMap.keySet()) {
                        PortTable portTable = hostTable.hostMap.get(hostIp);
                        for (Integer port : portTable.portMap.keySet()) {
                            Object transportHandler = portTable.portMap.get(port).second;
                            if(portTable.portMap.get(port).first == pro)
                                publishHandler(pro, portTable, topic, transportHandler, hostIp, port, message);
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Error during publish", e);
            }
        }
    }

    /**
     * Method to create transportHandler and publish message based on protocol
     * @param protocol protocol using which data needs to be published
     * @param portTable portTable has information about port and associated protocol with transport Handler object
     * @param topic The topic to which data is published
     * @param transportHandlerObject transportHandler object used to publish message
     * @param ip IP address of the destination
     * @param port Port number of the destination
     * @param message Data to be tranferred over WebRTC
     */
    private void publishHandler(Protocol protocol, PortTable portTable, String topic, Object transportHandlerObject, String ip, int port, byte[] message){
        TransportHandler transportHandler;
        if(transportHandlerObject == null){
            transportHandler = TransportFactory.createTransport(protocol);
            if(transportHandler!=null)
                transportHandler.setAppContext(appContext);
            portTable.portMap.replace(port, new Pair<>(protocol, transportHandler));
        }else{
            transportHandler = (TransportHandler) transportHandlerObject;
        }

        if(transportHandler!=null){
            transportHandler.publish(topic,ip, port, message);
        }
    }

    /**
     * Method to differentiate android specific protocol
     * @param protocols Protocol to be classified
     * @return true if the protocol is specific to android implementation
     */
    private boolean classifyProtocol(Protocol protocols){
        return protocols.equals(Protocol.WEBRTC) || protocols.equals(Protocol.IPC);
    }

    /**
     * Method to subscribe to a specific topic
     * @param topic String to which applications can subscribe, to receive data
     * @param callback Callback object specific to a subscribe call
     */
    public void subscribe(String topic, SubscribeCallback callback) {
        EnumSet<Protocol> protocolSet = EnumSet.of(Protocol.MQTT);
        subscribe(topic, callback, protocolSet, QoS.AT_MOST_ONCE);
    }

    /**
     * Method to subscribe to a specific topic
     * @param topic String to which applications can subscribe, to receive data
     * @param callback Callback object specific to a subscribe call
     * @param protocol Protocol supported by application, invoking subscribe
     * @param qos QoS at which the message should be delivered
     */
    public void subscribe(String topic, SubscribeCallback callback, Protocol protocol, QoS qos) {
        EnumSet<Protocol> protocolSet = EnumSet.of(protocol);
        subscribe(topic, callback, protocolSet, qos);
    }

    /**
     * Method to subscribe to a specific topic
     * @param topic String to which applications can subscribe, to receive data
     * @param callback Callback object specific to a subscribe call
     * @param protocols Protocol supported by application, invoking subscribe
     * @param qos QoS at which the message should be delivered
     */
    public void subscribe(String topic, SubscribeCallback callback, EnumSet<Protocol> protocols, QoS qos) {

        checkParams(topic, protocols);

        if (callback == null) {
            throw new IllegalArgumentException("Invalid callback");
        }

        int jniProtocols = 0;

        for (Protocol p : protocols) {
            if (!classifyProtocol(p)) {
                jniProtocols += p.getValue();
                protocols.remove(p);
            }
        }

        if(jniProtocols > 0) {
            Long pObject = subscribeJNI(instance, topic, jniProtocols, qos.ordinal());
            synchronized (this) {
                aittSubId.put(topic, pObject);
            }
        }

        for(Protocol pro : protocols) {
            try {
                TransportHandler transportHandler = TransportFactory.createTransport(pro);

                if (transportHandler != null) {
                    synchronized (this) {
                        subscribeMap.put(topic, new Pair(pro, transportHandler));
                    }

                    transportHandler.setAppContext(appContext);
                    transportHandler.setSelfIP(ip);
                    transportHandler.subscribe(topic, data -> {
                        AittMessage message = new AittMessage(data);
                        message.setTopic(topic);
                        messageReceived(message);
                    });
                    byte[] data = transportHandler.getPublishData();
                    publishJNI(instance, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, data, data.length, Protocol.MQTT.value, QoS.EXACTLY_ONCE.ordinal(), true);
                }
            } catch (Exception e) {
                Log.e(TAG, "Error during subscribe", e);
            }
        }

        addCallBackToSubscribeMap(topic, callback);
    }

    /**
     * Method to verify Aitt pub & sub parameters
     * @param topic String to which applications can subscribe, to receive data
     * @param protocols Protocol supported by application, invoking subscribe
     */
    private void checkParams(String topic, EnumSet<Protocol> protocols){
        if (topic == null || topic.isEmpty()) {
            throw new IllegalArgumentException(INVALID_TOPIC);
        }

        if (protocols.isEmpty()) {
            throw new IllegalArgumentException("Invalid protocols");
        }
    }

    /**
     * Method to map subscribe callback instance to subscribing topic
     * @param topic String to which application can subscribe
     * @param callback Subscribe callback instance created during subscribe call
     */
    private void addCallBackToSubscribeMap(String topic, SubscribeCallback callback) {
        synchronized (this) {
            try {
                ArrayList<SubscribeCallback> cbList = subscribeCallbacks.get(topic);

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
                Log.e(TAG, "Error during callback add", e);
            }
        }
    }

    /**
     * Method to unsubscribe to a topic, subscribed by application
     * @param topic String topic to which application had subscribed
     */
    public void unsubscribe(String topic) {
        if (topic == null || topic.isEmpty()) {
            throw new IllegalArgumentException(INVALID_TOPIC);
        }

        boolean isRemoved = false;
        try {
            synchronized (this) {
                if (subscribeMap.containsKey(topic) && subscribeMap.get(topic).first == Protocol.WEBRTC) {
                    TransportHandler transportHandler = (TransportHandler) subscribeMap.get(topic).second;
                    transportHandler.unsubscribe();
                    subscribeMap.remove(topic);
                    isRemoved = true;
                }
            }

            if (!isRemoved) {
                Long paittSubId = null;
                synchronized (this) {
                    if (aittSubId.containsKey(topic)) {
                        paittSubId = aittSubId.get(topic);
                    }
                }
                if (paittSubId != null) {
                    unsubscribeJNI(instance, paittSubId);
                }
            }

            synchronized (this) {
                subscribeCallbacks.remove(topic);
                aittSubId.remove(topic);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during unsubscribe", e);
        }
    }

    /**
     * Method invoked from JNI layer to Java layer for MQTT connection status update
     * @param status Status of the MQTT connection
     *   0: MQTT Connection disconnected
     *   1: MQTT connection success
     *   2: MQTT connection failed
     */
    private void connectionStatusCallback(int status) {

        switch (status) {
            case 0:
                connectionCallback.onDisconnected();
                break;
            case 1:
                connectionCallback.onConnected();
                break;
            case 2:
                connectionCallback.onConnectionFailed();
                break;
            default:
                break;
        }
    }

    /**
     * Method invoked from JNI layer to Java layer for message exchange
     * @param topic Topic to which message callback is called
     * @param payload Byte data shared from JNI layer to Java layer
     */
    private void messageCallback(String topic, byte[] payload) {
        try {
            if (topic.compareTo(Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC) == 0) {
                if (payload.length <= 0) {
                    Log.e(TAG, "Invalid payload, Ignore");
                    return;
                }
                discoveryMessageCallback(payload);
            } else {
                AittMessage message = new AittMessage(payload);
                message.setTopic(topic);
                messageReceived(message);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error processing callback ", e);
        }
    }

    /**
     * This API is called when MQTT subscribe callback is invoked at aitt C++.
     * It has discovery information in a "payload"
     * @param payload
     *  Flexbuffer discovery message expected
     *           {
     *             "status": "connected",
     *             "host": "127.0.0.1",
     *             "/customTopic/aitt/faceRecog": {
     *                "protocol": 1,
     *                "port": 108081,
     *             },
     *             "/customTopic/aitt/ASR": {
     *                "protocol": 2,
     *                "port": 102020,
     *             },
     *
     *             ...
     *
     *              "/customTopic/aitt/+": {
     *                "protocol": 3,
     *                "port": 20123,
     *             },
     *            }
    */
    
    /**
     * Method to receive discovery message with device, protocol and other details and update publish table
     * @param payload Byte data having discovery related message
     */
    private void discoveryMessageCallback(byte[] payload) {
        try {
            ByteBuffer buffer = ByteBuffer.wrap(payload);
            FlexBuffers.Map map = FlexBuffers.getRoot(buffer).asMap();
            String host = map.get("host").asString();
            String status = map.get(Definitions.STATUS).asString();
            if (status != null && status.compareTo(Definitions.WILL_LEAVE_NETWORK) == 0) {
                synchronized (this) {
                    for (Map.Entry<String, HostTable> entry : publishTable.entrySet()) {
                        HostTable hostTable = entry.getValue();
                        if (hostTable != null) {
                            hostTable.hostMap.remove(host);
                        }
                    }
                }
                return;
            }

            FlexBuffers.KeyVector topics = map.keys();
            for (int i = 0; i < topics.size(); i++) {
                String _topic = topics.get(i).toString();
                if (_topic.compareTo("host") == 0 || _topic.compareTo(Definitions.STATUS) == 0) {
                    continue;
                }

                FlexBuffers.Map _map = map.get(_topic).asMap();
                int port = _map.get("port").asInt();
                long p = _map.get("protocol").asUInt();
                Protocol protocol = Protocol.fromInt(p);
                updatePublishTable(_topic, host, port, protocol);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during discovery callback processing", e);
        }
    }

    /**
     * Method used to update Publish table of the application
     * @param topic The topic to which, other parties have subscribed to
     * @param host String which specifies a particular host
     * @param port Port of the party which subscribed to given topic
     * @param protocol protocol supported by the party which subscribed to given topic
     */
    private void updatePublishTable(String topic, String host, int port, Protocol protocol) {
        synchronized(this) {
            if (!publishTable.containsKey(topic)) {
                PortTable portTable = new PortTable();
                portTable.portMap.put(port, new Pair(protocol, null));
                HostTable hostTable = new HostTable();
                hostTable.hostMap.put(host, portTable);
                publishTable.put(topic, hostTable);
                return;
            }

            HostTable hostTable = publishTable.get(topic);
            if (!hostTable.hostMap.containsKey(host)) {
                PortTable portTable = new PortTable();
                portTable.portMap.put(port, new Pair(protocol, null));
                hostTable.hostMap.put(host, portTable);
                return;
            }

            PortTable portTable = hostTable.hostMap.get(host);
            if (portTable.portMap.containsKey(port)) {
                portTable.portMap.replace(port, new Pair(protocol, null));
                return;
            }

            portTable.portMap.put(port, new Pair(protocol, null));
        }
    }

    /**
     * Method that receives message from JNI layer for topics other than discovery topics
     * @param message The data received from JNI layer to be sent to application layer
     */
    private void messageReceived(AittMessage message) {
        try {
            String topic = message.getTopic();
            synchronized (this) {
                ArrayList<SubscribeCallback> cbList = subscribeCallbacks.get(topic);

                if (cbList != null) {
                    for (int i = 0; i < cbList.size(); i++) {
                        cbList.get(i).onMessageReceived(message);
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during messageReceived", e);
        }
    }

    /**
     * Method to close all the callbacks and release resources
     */
    public void close() {
        synchronized (this) {
            if(subscribeCallbacks!=null) {
                subscribeCallbacks.clear();
                subscribeCallbacks = null;
            }
            if(aittSubId!=null) {
                aittSubId.clear();
                aittSubId = null;
            }
        }
    }

    /* native API's set */
    /* Native API to initialize JNI */
    private native long initJNI(String id, String ip, boolean clearSession);

    /* Native API for connecting to broker */
    private native void connectJNI(long instance, final String host, int port);

    /* Native API for disconnecting from broker */
    private native void disconnectJNI(long instance);

    /* Native API for setting connection callback */
    private native void setConnectionCallbackJNI(long instance);

    /* Native API for publishing to a topic */
    private native void publishJNI(long instance, final String topic, final byte[] data, long datalen, int protocol, int qos, boolean retain);

    /* Native API for subscribing to a topic */
    private native long subscribeJNI(long instance, final String topic, int protocol, int qos);

    /* Native API for unsubscribing a topic */
    private native void unsubscribeJNI(long instance, final long aittSubId);
}
