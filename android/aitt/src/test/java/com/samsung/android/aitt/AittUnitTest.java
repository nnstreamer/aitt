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
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;

import android.content.Context;

import com.google.flatbuffers.FlexBuffersBuilder;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.api.support.membermodification.MemberMatcher;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.EnumSet;

@RunWith(PowerMockRunner.class)
@PrepareForTest(Aitt.class)
public class AittUnitTest {
   @Mock
   private final Context appContext = mock(Context.class);

   private static final int DISCOVERY_MESSAGES_COUNT = 6;
   private final String brokerIp = "192.168.0.1";
   private final int port = 1803;
   private final String topic = "aitt/test";
   private final String message = "test message";
   private final String aittId = "aitt";

   private Method messageCallbackMethod;

   @Before
   public void initialize() {
      try {
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "initJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return 1L;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "connectJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return null;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "disconnectJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return null;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "setConnectionCallbackJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return null;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "publishJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return null;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "subscribeJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return 1L;
            }
         });
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "unsubscribeJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return null;
            }
         });

         messageCallbackMethod = Aitt.class.getDeclaredMethod("messageCallback", String.class, byte[].class);
         messageCallbackMethod.setAccessible(true);
      } catch(Exception e) {
         fail("Failed to mock Aitt " + e);
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
      bb.get(array,0,array.length);
      return array;
   }

   @Test
   public void testAittConstructor_P(){
      String id = "aitt";
      try {
         Aitt aitt = new Aitt(appContext, id);
         assertNotNull("Aitt Instance not null", aitt);
      } catch(Exception e) {
         fail("Failed testInitialize " + e);
      }
   }

   @Test(expected = IllegalArgumentException.class)
   public void testInitializeInvalidId_N() {
      String _id = "";
      try {
         Aitt aitt = new Aitt(appContext, _id);
         aitt.close();
      } catch(InstantiationException e) {
         fail("Error during testInitializeInvalidId" + e);
      }
   }

   @Test(expected = IllegalArgumentException.class)
   public void testInitializeInvalidContext_N() {
      String _id = "";
      try {
         Aitt aitt = new Aitt(null, _id);
         aitt.close();
      } catch(InstantiationException e) {
         fail("Error during testInitializeInvalidContext" + e);
      }
   }

   @Test(expected = InstantiationException.class)
   public void testConstructorFail_N() throws InstantiationException {
      try{
         PowerMockito.replace(MemberMatcher.method(Aitt.class, "initJNI")).with(new InvocationHandler() {
            @Override
            public Object invoke(Object o, Method method, Object[] objects) throws Throwable {
               return 0L;
            }
         });
      } catch(Exception e) {
         fail("Failed to replace method" + e);
      }
      String id = "aitt";
      Aitt aitt = new Aitt(appContext,id);
      aitt.close();
   }

   @Test
   public void testConnect_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.close();
      } catch(Exception e) {
         fail("Failed testConnect " + e);
      }
   }

   @Test
   public void testConnectWithoutIP_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(null);

         aitt.close();
      } catch(Exception e) {
         fail("Failed testConnectWithoutIP " + e);
      }
   }

   @Test
   public void testDisconnect_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testDisconnect " + e);
      }
   }

   @Test
   public void testPublishMqtt_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         byte[] payload = message.getBytes();
         aitt.publish(topic, payload);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testPublishMqtt " + e);
      }
   }

   @Test
   public void testPublishWebRTC_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         byte[] payload = message.getBytes();
         aitt.publish(topic, payload, Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE, false);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testPublishWebRTC " + e);
      }
   }

   @Test
   public void testPublishInvalidTopic_N(){
      try {
         Aitt aitt = new Aitt(appContext, aittId);
         aitt.connect(brokerIp, port);
         String _topic = "";
         byte[] payload = message.getBytes();

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.publish(_topic, payload);
         });

         aitt.disconnect();
      } catch(Exception e){
         fail("Failed testPublishInvalidTopic" + e);
      }
   }

   @Test
   public void testPublishAnyProtocol_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         byte[] payload = message.getBytes();
         aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testPublishAnyProtocol " + e);
      }
   }

   @Test
   public void testPublishProtocolSet_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         byte[] payload = message.getBytes();
         EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
         aitt.publish(topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE, false);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testPublishProtocolSet " + e);
      }
   }

   @Test
   public void testPublishInvalidProtocol_N(){
      try{
         Aitt aitt = new Aitt(appContext, aittId);
         aitt.connect(brokerIp,port);
         byte[] payload = message.getBytes();
         EnumSet<Aitt.Protocol> protocols = EnumSet.noneOf(Aitt.Protocol.class);

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.publish(topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE, false);
         });

         aitt.disconnect();
      } catch(Exception e){
         fail("Failed testPublishInvalidProtocol" + e);
      }
   }

   @Test
   public void testSubscribeMqtt_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
            @Override
            public void onMessageReceived(AittMessage message) {
               String _topic = message.getTopic();
               byte[] payload = message.getPayload();
            }
         });

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeMqtt " + e);
      }
   }

   @Test
   public void testSubscribeWebRTC_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                    @Override
                    public void onMessageReceived(AittMessage message) {
                       String _topic = message.getTopic();
                       byte[] payload = message.getPayload();
                    }
                 },
                 Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeWebRTC " + e);
      }
   }


   @Test
   public void testSubscribeInvalidTopic_N() {

      try{
         Aitt aitt = new Aitt(appContext, aittId);
         aitt.connect(brokerIp, port);

         String _topic = "";

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.subscribe(_topic, new Aitt.SubscribeCallback() {
               @Override
               public void onMessageReceived(AittMessage message) {
               }
            });
         });

         aitt.disconnect();
      } catch(Exception e){
         fail("Failed testSubscribeInvalidTopic " + e);
      }
   }

   @Test
   public void testSubscribeInvalidCallback_N() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         aitt.connect(brokerIp, port);

         String _topic = "topic";

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.subscribe(_topic, null);
         });

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeInvalidCallback " + e);
      }
   }

   @Test
   public void testSubscribeAnyProtocol_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                    @Override
                    public void onMessageReceived(AittMessage message) {
                       String _topic = message.getTopic();
                       byte[] payload = message.getPayload();
                    }
                 },
                 Aitt.Protocol.UDP, Aitt.QoS.AT_MOST_ONCE);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeAnyProtocol " + e);
      }
   }

   @Test
   public void testSubscribeInvalidProtocol_N() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         aitt.connect(brokerIp, port);
         EnumSet<Aitt.Protocol> protocols = EnumSet.noneOf(Aitt.Protocol.class);

         assertThrows(IllegalArgumentException.class, () -> {
                    aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                               @Override
                               public void onMessageReceived(AittMessage message) {
                                  String _topic = message.getTopic();
                                  byte[] payload = message.getPayload();
                               }
                            },
                            protocols, Aitt.QoS.AT_MOST_ONCE);
                 });

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeAnyProtocol " + e);
      }
   }

   @Test
   public void testSubscribeProtocolSet_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                    @Override
                    public void onMessageReceived(AittMessage message) {
                       String _topic = message.getTopic();
                       byte[] payload = message.getPayload();
                    }
                 },
                 protocols, Aitt.QoS.EXACTLY_ONCE);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeProtocolSet " + e);
      }
   }

   @Test
   public void testUnsubscribe_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
            @Override
            public void onMessageReceived(AittMessage message) {
            }
         });

         aitt.unsubscribe(topic);
         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testUnsubscribe " + e);
      }
   }

   @Test
   public void testUnsubscribeInvalidTopic_N() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         aitt.connect(brokerIp, port);
         String _topic = "";

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.unsubscribe(_topic);
         });

         aitt.disconnect();
      } catch(Exception e){
         fail("Failed testUnsubscribeInvalidTopic " + e);
      }
   }

   @Test
   public void testSetConnectionCallback_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
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
      } catch(Exception e) {
         fail("Failed testSetConnectionCallback " + e);
      }
   }

   @Test
   public void testSetConnectionCallbackInvalidCallback_N() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertThrows(IllegalArgumentException.class, () -> {
            aitt.setConnectionCallback(null);
         });

         aitt.connect(brokerIp, port);
         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSetConnectionCallbackInvalidCallback " + e);
      }
   }

   @Test
   public void testSubscribeMultipleCallbacks_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         Aitt.SubscribeCallback callback1 = message -> {};

         Aitt.SubscribeCallback callback2 = message -> {};

         aitt.subscribe(topic, callback1);
         aitt.subscribe(topic, callback2);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeMultipleCallbacks " + e);
      }
   }

   // The test covers different cases of updating the publish table
   @Test
   public void testDiscoveryMessageCallbackConnected_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         int counter = 1;
         while (counter < DISCOVERY_MESSAGES_COUNT) {
            byte[] discoveryMessage = createDiscoveryMessage(counter);
            messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, (Object) discoveryMessage);
            counter++;
         }

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testDiscoveryMessageCallback " + e);
      }
   }

   @Test
   public void testDiscoveryMessageCallbackDisconnected_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         int counter = 1;
         byte[] discoveryMessage = createDiscoveryMessage(counter);
         messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, (Object) discoveryMessage);

         counter = 6;
         byte[] disconnectMessage = createDiscoveryMessage(counter);
         messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, (Object) disconnectMessage);
         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testDiscoveryMessageCallback " + e);
      }
   }

   @Test
   public void testDiscoveryMessageCallbackEmptyPayload_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);

         assertNotNull("Aitt Instance not null", aitt);
         aitt.connect(brokerIp, port);

         byte[] discoveryMessage = new byte[0];
         messageCallbackMethod.invoke(aitt, Definitions.JAVA_SPECIFIC_DISCOVERY_TOPIC, (Object) discoveryMessage);

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testDiscoveryMessageCallbackEmptyPayload " + e);
      }
   }

   @Test
   public void testSubscribeCallbackVerifyTopic_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
            @Override
            public void onMessageReceived(AittMessage aittMessage) {
               String recvTopic = aittMessage.getTopic();
               assertEquals("Received topic and subscribed topic are equal", recvTopic, topic);
            }
         });

         messageCallbackMethod.invoke(aitt, topic, message.getBytes(StandardCharsets.UTF_8));

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeCallback " + e);
      }
   }

   @Test
   public void testSubscribeCallbackVerifyPayload_P() {
      try {
         Aitt aitt = new Aitt(appContext, aittId);
         aitt.connect(brokerIp, port);

         aitt.subscribe(topic, new Aitt.SubscribeCallback() {
            @Override
            public void onMessageReceived(AittMessage aittMessage) {
               String recvMessage = new String(aittMessage.getPayload(), StandardCharsets.UTF_8);
               assertEquals("Received message and sent message are equal", message, recvMessage);
            }
         });

         messageCallbackMethod.invoke(aitt, topic, message.getBytes(StandardCharsets.UTF_8));

         aitt.disconnect();
      } catch(Exception e) {
         fail("Failed testSubscribeCallback " + e);
      }
   }
}
