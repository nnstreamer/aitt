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

import static org.junit.Assert.assertNotNull;

import android.content.Context;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.EnumSet;

@RunWith(AndroidJUnit4.class)
public class AittUnitTest {
   private String id = "id101";
   private String ip = "127.0.0.1";
   private Context appContext = ApplicationProvider.getApplicationContext();
   private String brokerIp = "192.168.0.1";
   private int port = 1803;
   private String topic = "aitt/test";
   private String message = "test message";

   @Test
   public void testInitialize() {
      Aitt aitt = new Aitt(appContext, id, ip, false);
      assertNotNull(aitt);
      aitt.close();
   }

   @Test
   public void testInitializeOnlyId() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.close();
   }

   @Test(expected = IllegalArgumentException.class)
   public void testInitializeInvalidId() {
      String _id = "";
      Aitt aitt = new Aitt(appContext, _id);
      aitt.close();
   }

   @Test
   public void testConnect() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      aitt.close();
   }

   @Test
   public void testConnectWithoutIP() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(null);

      aitt.close();
   }

   @Test
   public void testDisconnect() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);
      aitt.disconnect();
      aitt.close();
   }

   @Test
   public void testPublishMqtt() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      byte[] payload = message.getBytes();
      aitt.publish(topic, payload);
      aitt.close();
   }

   @Test
   public void testPublishWebRTC() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      byte[] payload = message.getBytes();
      aitt.publish(topic, payload, Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE);
      aitt.close();
   }

   @Test(expected = IllegalArgumentException.class)
   public void testPublishInvalidTopic() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      String _topic = "";
      byte[] payload = message.getBytes();
      aitt.publish(_topic, payload);
      aitt.close();
   }

   @Test
   public void testPublishAnyProtocol() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      byte[] payload = message.getBytes();
      aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);
      aitt.close();
   }

   @Test
   public void testPublishProtocolSet() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      byte[] payload = message.getBytes();
      EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
      aitt.publish(topic, payload, protocols, Aitt.QoS.AT_MOST_ONCE);
      aitt.close();
   }

   @Test
   public void testSubscribeMqtt() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      aitt.subscribe(topic, new Aitt.SubscribeCallback() {
         @Override
         public void onMessageReceived(AittMessage message) {
            String _topic = message.getTopic();
            byte[] payload = message.getPayload();
            String correlation = message.getCorrelation();
         }
      });

      aitt.close();
   }

   @Test
   public void testSubscribeWebRTC() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                 @Override
                 public void onMessageReceived(AittMessage message) {
                    String _topic = message.getTopic();
                    byte[] payload = message.getPayload();
                    String correlation = message.getCorrelation();
                 }
              },
              Aitt.Protocol.WEBRTC, Aitt.QoS.AT_MOST_ONCE);

      aitt.close();
   }

   @Test(expected = IllegalArgumentException.class)
   public void testSubscribeInvalidTopic() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      String _topic = "";
      aitt.subscribe(_topic, new Aitt.SubscribeCallback() {
         @Override
         public void onMessageReceived(AittMessage message) {}
      });

      aitt.close();
   }

   @Test(expected = IllegalArgumentException.class)
   public void testSubscribeInvalidCallback() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      String _topic = "";
      aitt.subscribe(_topic, null);

      aitt.close();
   }

   @Test
   public void testSubscribeAnyProtocol() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                 @Override
                 public void onMessageReceived(AittMessage message) {
                    String _topic = message.getTopic();
                    byte[] payload = message.getPayload();
                    String correlation = message.getCorrelation();
                 }
              },
              Aitt.Protocol.UDP, Aitt.QoS.AT_MOST_ONCE);

      aitt.close();
   }

   @Test
   public void testSubscribeProtocolSet() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      EnumSet<Aitt.Protocol> protocols = EnumSet.of(Aitt.Protocol.MQTT, Aitt.Protocol.TCP);
      aitt.subscribe(topic, new Aitt.SubscribeCallback() {
                 @Override
                 public void onMessageReceived(AittMessage message) {
                    String _topic = message.getTopic();
                    byte[] payload = message.getPayload();
                    String correlation = message.getCorrelation();
                 }
              },
              protocols, Aitt.QoS.EXACTLY_ONCE);

      aitt.close();
   }

   @Test
   public void testUnsubscribe() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      aitt.subscribe(topic, new Aitt.SubscribeCallback() {
         @Override
         public void onMessageReceived(AittMessage message) {}
      });

      aitt.unsubscribe(topic);
      aitt.close();
   }

   @Test(expected = IllegalArgumentException.class)
   public void testUnsubscribeInvalidTopic() {
      Aitt aitt = new Aitt(appContext, id);
      assertNotNull(aitt);
      aitt.connect(brokerIp, port);

      String _topic = "";
      aitt.unsubscribe(_topic);
      aitt.close();
   }
}
