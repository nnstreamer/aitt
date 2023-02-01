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
import static com.samsung.android.modules.webrtc.WebRTC.MAX_MESSAGE_SIZE;

import static org.junit.Assert.fail;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ImageDecoder;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.ColorInt;
import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.stream.AittStream;

import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;
import java.util.Arrays;
import java.util.Random;
import java.util.StringTokenizer;

public class WebRTCInstrumentedTest {

    private static final String TEST_MESSAGE_TOPIC = "android/test/webrtc/_AittRe_";
    private static final String TEST_LARGE_MESSAGE_TOPIC = "android/test/large/_AittRe_";
    private static final String TEST_VIDEO_TOPIC = "android/test/webrtc/video";
    private static final String LARGE_DATA_PREFIX = "_LARGE_DATA";
    private static final String VIDEO_PREFIX = "_VIDEO";
    private static final String AITT_WEBRTC_SERVER_ID = "AITT_WEBRTC_SERVER";
    private static final String AITT_WEBRTC_CLIENT_ID = "AITT_WEBRTC_CLIENT";
    private static final String ORANGE_IMAGE_FILE_NAME = "320_240_image.jpg";
    private static final String TAG = "WebRTCInstrumentedTest";
    private static final int PORT = 1883;
    private static final int SLEEP_INTERVAL = 100;

    private static String brokerIp;
    private static String wifiIP;
    private static Context appContext;
    private static Looper looper;
    private static byte[] frameImageBytes;

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
        Log.d(TAG, "Final WiFi IP = " + wifiIP);

        getRGBImage(appContext);

        if (Looper.myLooper() == null) {
            Looper.prepare();
            looper = Looper.myLooper();
            Log.i(TAG, "ALooper is prepared in this test case.");
        }
    }

    private static void getRGBImage(Context context) {
        ImageDecoder.Source imageSource = ImageDecoder.createSource(context.getResources().getAssets(), ORANGE_IMAGE_FILE_NAME);
        try {
            Bitmap bitmap = ImageDecoder.decodeBitmap(imageSource).copy(Bitmap.Config.ARGB_8888, true);
            int width = bitmap.getWidth();
            int height = bitmap.getHeight();
            int totalPixels = width * height;
            @ColorInt int[] pixels = new int[totalPixels];
            bitmap.getPixels(pixels, 0, width, 0, 0, width, height);
            int componentsPerPixel = 3;
            frameImageBytes = new byte[totalPixels * componentsPerPixel];
            for (int i = 0; i < totalPixels; i++) {
                @ColorInt int pixel = pixels[i];
                int red = Color.red(pixel);
                int green = Color.green(pixel);
                int blue = Color.blue(pixel);
                frameImageBytes[i * componentsPerPixel] = (byte) red;
                frameImageBytes[i * componentsPerPixel + 1] = (byte) green;
                frameImageBytes[i * componentsPerPixel + 2] = (byte) blue;
            }
        } catch (IOException e) {
            Log.e(TAG, "Fail to get an orange image.");
        }
    }

    @Test
    public void testWebRTCBasicMessageStreaming() {
        String message = "Hello, WebRTC!!";

        try {
            Aitt serverSubscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID, wifiIP, true);
            serverSubscriber.connect(brokerIp, PORT);
            AittStream serverSubscriberStream = serverSubscriber.createStream(Aitt.Protocol.WEBRTC, TEST_MESSAGE_TOPIC, SUBSCRIBER);
            serverSubscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCBasicMessageStreaming. Message size = " + data.length);
                if (Arrays.equals(data, message.getBytes()) == false)
                    throw new RuntimeException("A wrong test message(" + new String(data) + ") is received.");

                Log.i(TAG, "The correct test message(" + new String(data) + ") is received.");
                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt clientPublisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID, wifiIP, true);
            clientPublisher.connect(brokerIp, PORT);
            AittStream clientPublisherStream = clientPublisher.createStream(Aitt.Protocol.WEBRTC, TEST_MESSAGE_TOPIC, PUBLISHER);
            clientPublisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            serverSubscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 1000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = clientPublisher.publish(clientPublisherStream, TEST_MESSAGE_TOPIC, message.getBytes());
                if (isPublished)
                    break;
            }
            Log.i(TAG, "A message is sent through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            clientPublisherStream.disconnect();

            serverSubscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCBasicMessageStreaming, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCSendLargeMessage() {
        byte[] largeBytes = new byte[MAX_MESSAGE_SIZE * 2];
        new Random().nextBytes(largeBytes);

        try {
            Aitt serverSubscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID + LARGE_DATA_PREFIX, wifiIP, true);
            serverSubscriber.connect(brokerIp, PORT);
            AittStream serverSubscriberStream = serverSubscriber.createStream(Aitt.Protocol.WEBRTC, TEST_LARGE_MESSAGE_TOPIC, SUBSCRIBER);
            serverSubscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCSendLargeMessage. Message size = " + data.length);
                if (Arrays.equals(data, largeBytes) == false) {
                    throw new RuntimeException("A wrong large message is received.");
                }
                Log.i(TAG, "The correct large message is received. size = " + data.length);
                if (looper != null) {
                    looper.quit();
                }
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt clientPublisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + LARGE_DATA_PREFIX, wifiIP, true);
            clientPublisher.connect(brokerIp, PORT);
            AittStream clientPublisherStream = clientPublisher.createStream(Aitt.Protocol.WEBRTC, TEST_LARGE_MESSAGE_TOPIC, PUBLISHER);
            clientPublisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            serverSubscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 3000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = clientPublisher.publish(clientPublisherStream, TEST_LARGE_MESSAGE_TOPIC, largeBytes);
                if (isPublished)
                    break;
                Thread.sleep(SLEEP_INTERVAL);
            }
            Log.i(TAG, "A large message is sent through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            clientPublisherStream.disconnect();

            serverSubscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCBasicMessageStreaming, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCBasicVideoStreaming() {
        try {
            Aitt serverSubscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID + VIDEO_PREFIX, wifiIP, true);
            serverSubscriber.connect(brokerIp, PORT);
            AittStream serverSubscriberStream = serverSubscriber.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, SUBSCRIBER);
            serverSubscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCBasicVideoStreaming.");
                if (Arrays.equals(data, frameImageBytes) == false)
                    throw new RuntimeException("A wrong test image is received.");

                Log.i(TAG, "The correct test image is received.");

                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt clientPublisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            clientPublisher.connect(brokerIp, PORT);
            AittStream clientPublisherStream = clientPublisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);
            clientPublisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            serverSubscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 2000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = clientPublisher.publish(clientPublisherStream, TEST_VIDEO_TOPIC, frameImageBytes);
                if (isPublished)
                    break;
            }
            Log.i(TAG, "Video transmission has started through the publisher stream.");

            // TODO: Enable this looper.
//            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            clientPublisherStream.disconnect();

            serverSubscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCBasicVideoStreaming, (" + e + ")");
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
