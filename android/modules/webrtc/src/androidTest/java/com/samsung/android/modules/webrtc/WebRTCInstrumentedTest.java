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

import static com.samsung.android.aitt.stream.AittStream.StreamRole.PUBLISHER;
import static com.samsung.android.aitt.stream.AittStream.StreamRole.SUBSCRIBER;
import static org.junit.Assert.fail;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.os.Looper;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.stream.AittStream;
import com.samsung.android.aitt.stream.WebRTCStream;

import org.junit.BeforeClass;
import org.junit.Test;

import java.util.Arrays;
import java.util.StringTokenizer;

public class WebRTCInstrumentedTest {

    private static final String TEST_TOPIC = "android/test/webrtc/_AittRe_";
    private static final String AITT_WEBRTC_SERVER_ID = "AITT_ANDROID_WEBRTC_SERVER";
    private static final String AITT_WEBRTC_CLIENT_ID = "AITT_ANDROID_WEBRTC_CLIENT";
    private static final String TAG = "WebRTCInstrumentedTest";
    private static final int PORT = 1883;
    private static final int SLEEP_INTERVAL = 100;

    private static String brokerIp;
    private static Context appContext;
    private static Looper looper;

    @BeforeClass
    public static void initialize() {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        // IMPORTANT NOTE: Should give test arguments as follows.
        // if using Android studio: Run -> Edit Configurations -> Find 'Instrumentation arguments'
        //                         -> press '...' button -> add the name as "brokerIp" and the value
        //                         (Broker WiFi IP) of broker argument
        // if using gradlew commands: Add "-e brokerIp [Broker WiFi IP]"
        brokerIp = InstrumentationRegistry.getArguments().getString("brokerIp");

        if (Looper.myLooper() == null) {
            Looper.prepare();
            looper = Looper.myLooper();
            Log.i(TAG, "ALooper is prepared in this test case.");
        }
    }

    @Test
    public void testWebRTCBasicStreaming() {
        String message = "Hello, WebRTC!!";

        try {
            String ip = wifiIpAddress();

            Aitt serverSubscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID, ip, true);
            serverSubscriber.connect(brokerIp, PORT);
            AittStream serverSubscriberStream = serverSubscriber.createStream(Aitt.Protocol.WEBRTC, TEST_TOPIC, SUBSCRIBER);
            serverSubscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "Callback is received.");
                if (Arrays.equals(data, message.getBytes()) == false)
                    throw new RuntimeException("A wrong test message(" + new String(data) + ") is received.");

                Log.i(TAG, "The correct test message(" + new String(data) + ") is received.");
                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream is created.");

            Aitt clientPublisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID, ip, true);
            clientPublisher.connect(brokerIp, PORT);
            AittStream clientPublisherStream = clientPublisher.createStream(Aitt.Protocol.WEBRTC, TEST_TOPIC, PUBLISHER);
            clientPublisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream is created.");

            serverSubscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 1000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            int serverPort = ((WebRTCStream) serverSubscriberStream).getServerPort();
            Log.d(TAG, "Server port = " + serverPort);
            while (true) {
                // TODO: Replace publish
                boolean isPublished = clientPublisher.publish(clientPublisherStream, TEST_TOPIC, message.getBytes(), Aitt.Protocol.WEBRTC);
                if (isPublished)
                    break;
            }
            Log.i(TAG, "A message is sent through a publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");
        } catch (Exception e) {
            fail("Failed testWebRTCBasicStreaming, (" + e + ")");
        }
    }

    private static String wifiIpAddress() {
        ConnectivityManager connectivityManager = (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network currentNetwork = connectivityManager.getActiveNetwork();
        LinkProperties linkProperties = connectivityManager.getLinkProperties(currentNetwork);
        for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) {
            String targetIp = linkAddress.toString();
            if (targetIp.contains("192.168")) {
                StringTokenizer tokenizer = new StringTokenizer(targetIp, "/");
                if (tokenizer.hasMoreTokens())
                    return tokenizer.nextToken();
            }
        }

        return null;
    }
}
