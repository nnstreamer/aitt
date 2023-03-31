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
package com.samsung.android.modules.rtsp;

import static com.samsung.android.aitt.stream.AittStream.StreamRole.PUBLISHER;
import static com.samsung.android.aitt.stream.AittStream.StreamRole.SUBSCRIBER;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.stream.AittStream;
import com.samsung.android.aitt.stream.AittStreamConfigBuilder;

import org.junit.BeforeClass;
import org.junit.Test;

import java.util.StringTokenizer;

public class RTSPInstrumentedTest {

    private static final String TEST_TOPIC = "android/test/rtsp";
    private static final String TAG = "RTSPInstrumentationTest";
    private static final String AITT_ID = "AITT_ANDROID";
    private static final String ERROR_MESSAGE_AITT_NULL = "An AITT instance is null.";
    private static final int PORT = 1883;
    private static final String RTSP_URL = "rtsp://192.168.1.4:1935";
    private static final String USERNAME = "username";
    private static final String PASSWORD = "password";
    private static final int WIDTH = 640;
    private static final int HEIGHT = 480;

    private static String brokerIp;
    private static String wifiIP;
    private static Context appContext;

    @BeforeClass
    public static void initialize() {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        // IMPORTANT NOTE: Should give test arguments as follows.
        // if using Android studio: Run -> Edit Configurations -> Find 'Instrumentation arguments'
        //                         -> press '...' button -> add the name as "brokerIp" and the value
        //                         (Broker WiFi IP) of broker argument
        // if using gradlew commands: Add "-e brokerIp [Broker WiFi IP]"
        brokerIp = InstrumentationRegistry.getArguments().getString("brokerIp");
        wifiIP = wifiIpAddress();
    }

    @Test
    public void testRTSPStreamWithoutCredentials_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream subscriber = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, SUBSCRIBER);

            AittStream.StreamDataCallback callback = data -> {
                //Do something
            };
            subscriber.setReceiveCallback(callback);
            subscriber.start();

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setHeight(HEIGHT);
            builder.setWidth(WIDTH);
            publisher.setConfig(builder.build());
            publisher.start();

            publisher.stop();
            subscriber.stop();

            aitt.destroyStream(publisher);
            aitt.destroyStream(subscriber);
        } catch (Exception e) {
            fail("Failed testRTSPStreamWithoutCredentials, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamWithEmptyCredentials_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream subscriber = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, SUBSCRIBER);

            AittStream.StreamDataCallback callback = data -> {
                //Do something
            };
            subscriber.setReceiveCallback(callback);
            subscriber.start();

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId("");
            builder.setPassword("");
            builder.setHeight(HEIGHT);
            builder.setWidth(WIDTH);
            publisher.setConfig(builder.build());
            publisher.start();

            publisher.stop();
            subscriber.stop();

            aitt.destroyStream(publisher);
            aitt.destroyStream(subscriber);
        } catch (Exception e) {
            fail("Failed testRTSPStreamWithEmptyCredentials, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamWithCredentials_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream subscriber = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, SUBSCRIBER);

            AittStream.StreamDataCallback callback = data -> {
                //Do something
            };
            subscriber.setReceiveCallback(callback);
            subscriber.start();

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId(USERNAME);
            builder.setPassword(PASSWORD);
            builder.setHeight(HEIGHT);
            builder.setWidth(WIDTH);
            publisher.setConfig(builder.build());
            publisher.start();

            publisher.stop();
            subscriber.stop();

            aitt.destroyStream(publisher);
            aitt.destroyStream(subscriber);
        } catch (Exception e) {
            fail("Failed testRTSPStreamWithCredentials, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamWithoutHeight_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId(USERNAME);
            builder.setPassword(PASSWORD);
            builder.setWidth(WIDTH);
            publisher.setConfig(builder.build());
            publisher.start();

            publisher.stop();
            aitt.destroyStream(publisher);
        } catch (Exception e) {
            fail("Failed testRTSPStreamWithoutHeight, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamWithoutWidth_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId(USERNAME);
            builder.setPassword(PASSWORD);
            builder.setHeight(HEIGHT);
            publisher.setConfig(builder.build());
            publisher.start();

            publisher.stop();
            aitt.destroyStream(publisher);
        } catch (Exception e) {
            fail("Failed testRTSPStreamWithoutWidth, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamInvalidHeight_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            aitt.connect(brokerIp, PORT);

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId(USERNAME);
            builder.setPassword(PASSWORD);
            builder.setHeight(-1);
            assertThrows(IllegalArgumentException.class, () -> publisher.setConfig(builder.build()));

            publisher.disconnect();
        } catch (Exception e) {
            fail("Failed testRTSPStreamInvalidHeight, (" + e + ")");
        }
    }

    @Test
    public void testRTSPStreamInvalidWidth_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIP, true);
            aitt.connect(brokerIp, PORT);

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            AittStreamConfigBuilder builder = new AittStreamConfigBuilder();
            builder.setUrl(RTSP_URL);
            builder.setId(USERNAME);
            builder.setPassword(PASSWORD);
            builder.setWidth(-1);
            assertThrows(IllegalArgumentException.class, () -> publisher.setConfig(builder.build()));

            publisher.disconnect();
        } catch (Exception e) {
            fail("Failed testRTSPStreamInvalidWidth, (" + e + ")");
        }
    }

    private static String wifiIpAddress() {
        ConnectivityManager connectivityManager = (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network currentNetwork = connectivityManager.getActiveNetwork();
        LinkProperties linkProperties = connectivityManager.getLinkProperties(currentNetwork);
        for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) {
            String targetIp = linkAddress.toString();
            Log.d(TAG, "Searched IP = " + targetIp);
            if (targetIp.contains("192.168")) {
                StringTokenizer tokenizer = new StringTokenizer(targetIp, "/");
                if (tokenizer.hasMoreTokens())
                    return tokenizer.nextToken();
            }
        }
        return null;
    }
}
