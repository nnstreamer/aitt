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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;

import android.content.Context;

import com.google.flatbuffers.FlexBuffersBuilder;
import com.samsung.android.aitt.internal.Definitions;
import com.samsung.android.aitt.stream.AittStream;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.EnumSet;

@RunWith(RobolectricTestRunner.class)
@Config(shadows = ShadowJniInterface.class)
public class AittUnitTest {
    @Mock
    private final Context appContext = mock(Context.class);

    private static final int DISCOVERY_MESSAGES_COUNT = 6;
    private final String brokerIp = "192.168.0.1";
    private final int port = 1803;
    private final String topic = "aitt/test";
    private final String message = "test message";
    private final String aittId = "aitt";

    ShadowJniInterface shadowJniInterface = new ShadowJniInterface();

    private Method messageCallbackMethod;

    @Before
    public void initialize() {
        try {
            messageCallbackMethod = Aitt.class.getDeclaredMethod("messageCallback", String.class, byte[].class);
            messageCallbackMethod.setAccessible(true);
        } catch (Exception e) {
            fail("Failed to Initialize " + e);
        }
    }

    private byte[] createDiscoveryMessage(int count) {
        int start;
        FlexBuffersBuilder builder = new FlexBuffersBuilder(ByteBuffer.allocate(512));
        start = builder.startMap();
        switch (count) {
            case 1:
                /*
                 *           {
                 *             "status": "connected",
                 *             "host": "127.0.0.1",
                 *             "aitt/topic1": {
                 *                "protocol": TCP,
                 *                "port": 1000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.JOIN_NETWORK);
                builder.putString("host", Definitions.AITT_LOCALHOST);
                int secondStart = builder.startMap();
                builder.putInt("port", 1000);
                builder.putInt("protocol", Aitt.Protocol.TCP.getValue());
                builder.endMap("aitt/topic1", secondStart);
                break;
            case 2:
                /*
                 *           {
                 *             "status": "connected",
                 *             "host": "127.0.0.2",
                 *             "aitt/topic1": {
                 *                "protocol": MQTT,
                 *                "port": 2000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.JOIN_NETWORK);
                builder.putString("host", "127.0.0.2");
                secondStart = builder.startMap();
                builder.putInt("port", 2000);
                builder.putInt("protocol", Aitt.Protocol.MQTT.getValue());
                builder.endMap("aitt/topic1", secondStart);
                break;
            case 3:
                /*
                 *           {
                 *             "status": "connected",
                 *             "host": "127.0.0.1",
                 *             "aitt/topic2": {
                 *                "protocol": MQTT,
                 *                "port": 2000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.JOIN_NETWORK);
                builder.putString("host", Definitions.AITT_LOCALHOST);
                secondStart = builder.startMap();
                builder.putInt("port", 2000);
                builder.putInt("protocol", Aitt.Protocol.MQTT.getValue());
                builder.endMap("aitt/topic2", secondStart);
                break;
            case 4:
                /*
                 *           {
                 *             "status": "connected",
                 *             "host": "127.0.0.1",
                 *             "aitt/topic2": {
                 *                "protocol": TCP,
                 *                "port": 4000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.JOIN_NETWORK);
                builder.putString("host", Definitions.AITT_LOCALHOST);
                secondStart = builder.startMap();
                builder.putInt("port", 4000);
                builder.putInt("protocol", Aitt.Protocol.TCP.getValue());
                builder.endMap("aitt/topic2", secondStart);
                break;
            case 5:
                /*
                 *           {
                 *             "status": "connected",
                 *             "host": "127.0.0.1",
                 *             "aitt/topic2": {
                 *                "protocol": WEBRTC,
                 *                "port": 2000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.JOIN_NETWORK);
                builder.putString("host", Definitions.AITT_LOCALHOST);
                secondStart = builder.startMap();
                builder.putInt("port", 2000);
                builder.putInt("protocol", Aitt.Protocol.WEBRTC.getValue());
                builder.endMap("aitt/topic2", secondStart);
                break;
            case 6:
                /*
                 *           {
                 *             "status": "disconnected",
                 *             "host": "127.0.0.1",
                 *             "aitt/topic1": {
                 *                "protocol": TCP,
                 *                "port": 1000,
                 *             }
                 *            }
                 */
                builder.putString("status", Definitions.WILL_LEAVE_NETWORK);
                builder.putString("host", Definitions.AITT_LOCALHOST);
                secondStart = builder.startMap();
                builder.putInt("port", 1000);
                builder.putInt("protocol", Aitt.Protocol.TCP.getValue());
                builder.endMap("aitt/topic1", secondStart);
                break;
            default:
                return null;
        }
        builder.endMap(null, start);
        ByteBuffer bb = builder.finish();
        byte[] array = new byte[bb.remaining()];
        bb.get(array, 0, array.length);
        return array;
    }

    @Test
    public void testAittConstructor_P() {
        String id = "aitt";
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, id);
            assertNotNull("Aitt Instance not null", aitt);
        } catch (Exception e) {
            fail("Failed testAittConstructor " + e);
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInitializeInvalidId_N() {
        String _id = "";
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, _id);
            aitt.close();
        } catch (InstantiationException e) {
            fail("Error during testInitializeInvalidId" + e);
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInitializeInvalidIp_N() {
        String ip = "";
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId, ip, false);
            aitt.close();
        } catch (InstantiationException e) {
            fail("Error during testInitializeInvalidIp" + e);
        }
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInitializeInvalidContext_N() {
        String _id = "";
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(null, _id);
            aitt.close();
        } catch (InstantiationException e) {
            fail("Error during testInitializeInvalidContext" + e);
        }
    }

    @Test(expected = InstantiationException.class)
    public void testConstructorFail_N() throws InstantiationException {
        shadowJniInterface.setInitReturn(false);
        String id = "aitt";
        Aitt aitt = new Aitt(appContext, id);
        aitt.close();
    }

    @Test
    public void testConnect_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);

            aitt.connect(brokerIp, port);

            aitt.close();
        } catch (Exception e) {
            fail("Failed testConnect " + e);
        }
    }

    @Test
    public void testSetWillInfoMqtt_P() {
        try {
            String willInfo = "Test Will info data";
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            byte[] data = willInfo.getBytes();
            aitt.setWillInfo(topic, data, Aitt.QoS.AT_MOST_ONCE, false);
            aitt.connect(brokerIp, port);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSetWillInfoMqtt_P " + e);
        }
    }

    @Test
    public void testWithoutWillInfo_N() {
        try {
            String willInfo = "";
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            byte[] data = willInfo.getBytes();
            assertThrows(IllegalArgumentException.class, () -> aitt.setWillInfo(topic, data, Aitt.QoS.AT_MOST_ONCE, false));
            aitt.connect(brokerIp, port);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testWithoutWillInfo_N " + e);
        }
    }

    @Test
    public void testWillInfoWithoutTopic_N() {
        try {
            String _topic = "";
            String willInfo = "Test Will info data";
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            byte[] data = willInfo.getBytes();
            assertThrows(IllegalArgumentException.class, () -> aitt.setWillInfo(_topic, data, Aitt.QoS.AT_MOST_ONCE, false));
            aitt.connect(brokerIp, port);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testWillInfoWithoutTopic_N " + e);
        }
    }

    @Test
    public void testConnectWithoutIP_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);

            aitt.connect(null);

            aitt.close();
        } catch (Exception e) {
            fail("Failed testConnectWithoutIP " + e);
        }
    }

    @Test
    public void testDisconnect_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testDisconnect " + e);
        }
    }

    @Test
    public void testPublishMqtt_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            aitt.publish(topic, payload);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishMqtt " + e);
        }
    }

    @Test
    public void testPublishMqttInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            byte[] payload = message.getBytes();
            assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishMqttInvalidTopic" + e);
        }
    }

    @Test
    public void testPublishIpc_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            aitt.publish(topic, payload, Aitt.Protocol.IPC, Aitt.QoS.AT_MOST_ONCE, false);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishIpc " + e);
        }
    }

    @Test
    public void testPublishIpcInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            byte[] payload = message.getBytes();
            assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload, Aitt.Protocol.IPC, Aitt.QoS.AT_MOST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishIpcInvalidTopic" + e);
        }
    }

    @Test
    public void testPublishAnyProtocol_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            aitt.publish(topic, payload, Aitt.Protocol.MQTT);
            aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.EXACTLY_ONCE);
            aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, true);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishAnyProtocol " + e);
        }
    }

    @Test
    public void testPublishAnyProtocolInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            byte[] payload = message.getBytes();
            assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishAnyProtocolInvalidTopic" + e);
        }
    }

    @Test
    public void testPublishProtocolSet_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
            aitt.publish(topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE, false);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishProtocolSet " + e);
        }
    }

    @Test
    public void testPublishProtocolSetInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            byte[] payload = message.getBytes();
            EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
            assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishProtocolSetInvalidTopic " + e);
        }
    }

    @Test
    public void testPublishInvalidProtocol_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            EnumSet<Aitt.Protocol> protocols = EnumSet.noneOf(Aitt.Protocol.class);
            assertThrows(IllegalArgumentException.class, () -> aitt.publish(topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishInvalidProtocol" + e);
        }
    }

    @Test
    public void testSubscribeMqtt_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
            });

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeMqtt " + e);
        }
    }

    @Test
    public void testSubscribeMqttInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeMqttInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeMqttInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeMqttInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeTCP_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
            }, Aitt.Protocol.TCP);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeTCP " + e);
        }
    }

    @Test
    public void testSubscribeTcpInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.TCP));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeTcpInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeTcpInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null, Aitt.Protocol.TCP));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeTcpInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeWebRTC_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
            }, Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWebRTC " + e);
        }
    }

    @Test
    public void testSubscribeWebRTCInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWebRTCInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeWebRTCInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null, Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWebRTCInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeIpc_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
            }, Aitt.Protocol.IPC, Aitt.QoS.AT_MOST_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeIpc " + e);
        }
    }

    @Test
    public void testSubscribeIpcInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.IPC, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeIpcInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeIpcInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null, Aitt.Protocol.IPC, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeIpcInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeAnyProtocol_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_MOST_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeAnyProtocol " + e);
        }
    }

    @Test
    public void testSubscribeAnyProtocolInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeAnyProtocolInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeAnyProtocolInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null, Aitt.Protocol.TCP, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeAnyProtocolInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeInvalidProtocol_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            EnumSet<Aitt.Protocol> protocols = EnumSet.noneOf(Aitt.Protocol.class);
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, message -> {
            }, protocols, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeInvalidProtocol " + e);
        }
    }

    @Test
    public void testSubscribeProtocolSet_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
            aitt.subscribe(topic, message -> {
            }, protocols, Aitt.QoS.EXACTLY_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeProtocolSet " + e);
        }
    }

    @Test
    public void testSubscribeProtocolSetInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, protocols, Aitt.QoS.EXACTLY_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeProtocolSetInvalidTopic " + e);
        }
    }

    @Test
    public void testSubscribeProtocolSetInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(topic, null, protocols, Aitt.QoS.AT_MOST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeProtocolSetInvalidCallback " + e);
        }
    }

    @Test
    public void testUnsubscribe_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);
            aitt.subscribe(topic, message -> {
            });

            aitt.unsubscribe(topic);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testUnsubscribe " + e);
        }
    }

    @Test
    public void testUnsubscribeInvalidTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.unsubscribe(_topic));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testUnsubscribeInvalidTopic " + e);
        }
    }

    @Test
    public void testSetConnectionCallback_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);

            aitt.setConnectionCallback(new Aitt.ConnectionCallback() {
                @Override
                public void onConnected() {
                }

                @Override
                public void onDisconnected() {
                }

                @Override
                public void onConnectionFailed() {
                }
            });

            aitt.connect(brokerIp, port);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSetConnectionCallback " + e);
        }
    }

    @Test
    public void testSetConnectionCallbackInvalidCallback_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);

            assertThrows(IllegalArgumentException.class, () -> aitt.setConnectionCallback(null));

            aitt.connect(brokerIp, port);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSetConnectionCallbackInvalidCallback " + e);
        }
    }

    @Test
    public void testSubscribeMultipleCallbacks_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            Aitt.SubscribeCallback callback1 = message -> {
            };
            aitt.subscribe(topic, callback1);

            Aitt.SubscribeCallback callback2 = message -> {
            };
            aitt.subscribe(topic, callback2);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeMultipleCallbacks " + e);
        }
    }

    // The test covers different cases of updating the publish table
    @Test
    public void testDiscoveryMessageCallbackConnected_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            int counter = 1;
            while (counter < DISCOVERY_MESSAGES_COUNT) {
                byte[] discoveryMessage = createDiscoveryMessage(counter);
                messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, discoveryMessage);
                counter++;
            }

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testDiscoveryMessageCallbackConnected " + e);
        }
    }

    @Test
    public void testDiscoveryMessageCallbackDisconnected_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            int counter = 1;
            byte[] discoveryMessage = createDiscoveryMessage(counter);
            messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, discoveryMessage);

            counter = 6;
            byte[] disconnectMessage = createDiscoveryMessage(counter);
            messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, disconnectMessage);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testDiscoveryMessageCallbackDisconnected " + e);
        }
    }

    @Test
    public void testDiscoveryMessageCallbackEmptyPayload_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            byte[] discoveryMessage = new byte[0];
            messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, discoveryMessage);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testDiscoveryMessageCallbackEmptyPayload " + e);
        }
    }

    @Test
    public void testSubscribeCallbackVerifyTopic_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, aittMessage -> {
                String receivedTopic = aittMessage.getTopic();
                assertEquals("Received topic and subscribed topic are equal", receivedTopic, topic);
            });
            messageCallbackMethod.invoke(aitt, topic, message.getBytes(StandardCharsets.UTF_8));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeCallbackVerifyTopic " + e);
        }
    }

    @Test
    public void testSubscribeCallbackVerifyPayload_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            aitt.subscribe(topic, aittMessage -> {
                String receivedMessage = new String(aittMessage.getPayload(), StandardCharsets.UTF_8);
                assertEquals("Received message and sent message are equal", message, receivedMessage);
            });
            messageCallbackMethod.invoke(aitt, topic, message.getBytes(StandardCharsets.UTF_8));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeCallbackVerifyPayload " + e);
        }
    }

    @Test
    public void testCreateRTSPPublisherStream_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            AittStream rtspPublisher = aitt.createStream(Aitt.Protocol.RTSP, topic, AittStream.StreamRole.PUBLISHER);

            aitt.destroyStream(rtspPublisher);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPPublisherStream " + e);
        }
    }

    @Test
    public void testCreateRTSPSubscriberStream_P() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            AittStream rtspSubscriber = aitt.createStream(Aitt.Protocol.RTSP, topic, AittStream.StreamRole.SUBSCRIBER);

            aitt.destroyStream(rtspSubscriber);
            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPSubscriberStream " + e);
        }
    }

    @Test
    public void testCreateRTSPSubscriberStreamEmptyTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.RTSP, "", AittStream.StreamRole.SUBSCRIBER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPSubscriberStreamEmptyTopic " + e);
        }
    }

    @Test
    public void testCreateRTSPPublisherStreamEmptyTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.RTSP, "", AittStream.StreamRole.PUBLISHER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPPublisherStreamEmptyTopic " + e);
        }
    }

    @Test
    public void testCreateRTSPSubscriberStreamNullTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.RTSP, null, AittStream.StreamRole.SUBSCRIBER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPSubscriberStreamNullTopic " + e);
        }
    }

    @Test
    public void testCreateRTSPPublisherStreamNullTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.RTSP, null, AittStream.StreamRole.PUBLISHER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateRTSPPublisherStreamNullTopic " + e);
        }
    }

    @Test
    public void testCreateWebRTCSubscriberStreamEmptyTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.WEBRTC, "", AittStream.StreamRole.SUBSCRIBER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateWebRTCSubscriberStreamEmptyTopic " + e);
        }
    }

    @Test
    public void testCreateWebRTCPublisherStreamEmptyTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.WEBRTC, "", AittStream.StreamRole.PUBLISHER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateWebRTCPublisherStreamEmptyTopic " + e);
        }
    }

    @Test
    public void testCreateWebRTCSubscriberStreamNullTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.WEBRTC, null, AittStream.StreamRole.SUBSCRIBER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateWebRTCSubscriberStreamNullTopic " + e);
        }
    }

    @Test
    public void testCreateWebRTCPublisherStreamNullTopic_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            assertThrows(IllegalArgumentException.class, () -> aitt.createStream(Aitt.Protocol.WEBRTC, null, AittStream.StreamRole.PUBLISHER));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateWebRTCPublisherStreamNullTopic " + e);
        }
    }

    @Test
    public void testCreateMqttStream_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            AittStream aittStream = aitt.createStream(Aitt.Protocol.MQTT, topic, AittStream.StreamRole.PUBLISHER);
            assertNull(aittStream);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateMqttStream " + e);
        }
    }

    @Test
    public void testCreateTcpStream_N() {
        try {
            shadowJniInterface.setInitReturn(true);
            Aitt aitt = new Aitt(appContext, aittId);
            aitt.connect(brokerIp, port);

            AittStream aittStream = aitt.createStream(Aitt.Protocol.TCP, topic, AittStream.StreamRole.PUBLISHER);
            assertNull(aittStream);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testCreateTcpStream " + e);
        }
    }
}
