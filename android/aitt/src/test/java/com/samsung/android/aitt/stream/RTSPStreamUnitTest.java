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
package com.samsung.android.aitt.stream;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;

import org.junit.Test;

import java.util.concurrent.atomic.AtomicBoolean;

public class RTSPStreamUnitTest {

   private final String topic = "aitt/test/rtsp";
   private final String url = "rtsp://192.168.1.4:1935";
   private final String id = "id1";
   private final String password = "pwd1";
   private final String height = "640";
   private final String width = "480";

   @Test
   public void testCreatePublisherStream_P() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);
      assertNotNull("RTSPStream is not null", rtspStream);
   }

   @Test
   public void testCreatePublisherStreamInvalidRole_N() {
      assertThrows(IllegalArgumentException.class, () -> RTSPStream.createPublisherStream(topic, AittStream.StreamRole.SUBSCRIBER));
   }

   @Test
   public void testCreateSubscriberStream_P() {
      RTSPStream rtspStream = RTSPStream.createSubscriberStream(topic, AittStream.StreamRole.SUBSCRIBER);
      assertNotNull("RTSPStream is not null", rtspStream);
   }

   @Test
   public void testCreateSubscriberStreamInvalidRole_N() {
      assertThrows(IllegalArgumentException.class, () -> RTSPStream.createSubscriberStream(topic, AittStream.StreamRole.PUBLISHER));
   }

   @Test
   public void testSetConfig_P() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);

      rtspStream.setConfig("URI", url).setConfig("ID", id).setConfig("PASSWORD", password).setConfig("HEIGHT", height).setConfig("WIDTH", width);
   }

   @Test
   public void testSetConfigInvalidUrl_N() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);

      String url = "192.168.1.4:1935";
      assertThrows(IllegalArgumentException.class, () -> rtspStream.setConfig("URI", url));
   }

   @Test
   public void testSetReceiveCallbackInvalidRole_N() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);
      assertThrows(IllegalArgumentException.class, () -> rtspStream.setReceiveCallback(data -> {}));
   }

   @Test
   public void testStartPublisher_P() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);

      rtspStream.setConfig("URI", url).setConfig("ID", id).setConfig("PASSWORD", password).setConfig("HEIGHT", height).setConfig("WIDTH", width);

      rtspStream.setStateCallback(state -> assertEquals("Expected state is ready", state, AittStream.StreamState.READY));

      rtspStream.start();
   }

   @Test
   public void testStopPublisher_P() {
      RTSPStream rtspStream = RTSPStream.createPublisherStream(topic, AittStream.StreamRole.PUBLISHER);

      rtspStream.setConfig("URI", url).setConfig("ID", id).setConfig("PASSWORD", password).setConfig("HEIGHT", height).setConfig("WIDTH", width);

      AtomicBoolean isStarted = new AtomicBoolean(false);
      rtspStream.setStateCallback(state -> {
         if (!isStarted.get())
            isStarted.set(true);
         else {
            assertEquals("Expected state is Init", state, AittStream.StreamState.INIT);
            isStarted.set(false);
         }
      });

      rtspStream.start();

      rtspStream.stop();
   }

   @Test
   public void testStartSubscriber_P() {
      RTSPStream rtspStream = RTSPStream.createSubscriberStream(topic, AittStream.StreamRole.SUBSCRIBER);

      rtspStream.setStateCallback(state -> assertEquals("Expected state is ready", state, AittStream.StreamState.READY));
      rtspStream.setReceiveCallback(data -> {});

      rtspStream.start();
   }

   @Test
   public void testStopSubscriber_P() {
      RTSPStream rtspStream = RTSPStream.createSubscriberStream(topic, AittStream.StreamRole.SUBSCRIBER);

      AtomicBoolean isStarted = new AtomicBoolean(false);
      rtspStream.setStateCallback(state -> {
         if (!isStarted.get())
            isStarted.set(true);
         else {
            assertEquals("Expected state is Init", state, AittStream.StreamState.INIT);
            isStarted.set(false);
         }
      });

      rtspStream.setReceiveCallback(data -> {});
      rtspStream.start();

      rtspStream.stop();
   }
}
