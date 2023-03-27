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

import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;

import android.content.Context;
import android.graphics.Bitmap;
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
import com.samsung.android.aitt.stream.AittStreamConfig;
import com.samsung.android.aitt.stream.AittStreamConfigBuilder;

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
    private static final int FRAME_WIDTH = 320;
    private static final int FRAME_HEIGHT = 240;

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
            frameImageBytes = new byte[totalPixels * componentsPerPixel / 2];

            // Convert Bitmap to NV21 byte array
            encodeYUV420SP(frameImageBytes, pixels, width, height);
            bitmap.recycle();
        } catch (IOException e) {
            Log.e(TAG, "Fail to get an orange image.");
        }
    }

    static void encodeYUV420SP(byte[] yuv420sp, int[] argb, int width, int height) {
        final int frameSize = width * height;

        int yIndex = 0;
        int uvIndex = frameSize;

        int index = 0;
        for (int j = 0; j < height; j++) {
            for (int i = 0; i < width; i++) {

                int R = (argb[index] & 0xff0000) >> 16;
                int G = (argb[index] & 0xff00) >> 8;
                int B = (argb[index] & 0xff) >> 0;

                // RGB to YUV algorithm
                int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;

                yuv420sp[yIndex++] = (byte) ((Y < 0) ? 0 : ((Y > 255) ? 255 : Y));
                if (j % 2 == 0 && index % 2 == 0) {
                    yuv420sp[uvIndex++] = (byte) ((V < 0) ? 0 : ((V > 255) ? 255 : V));
                    yuv420sp[uvIndex++] = (byte) ((U < 0) ? 0 : ((U > 255) ? 255 : U));
                }

                index++;
            }
        }
    }

    @Test
    public void testWebRTCBasicMessageStreaming_P() {
        String message = "Hello, WebRTC!!";

        try {
            Aitt subscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID, wifiIP, true);
            subscriber.connect(brokerIp, PORT);
            AittStream subscriberStream = subscriber.createStream(Aitt.Protocol.WEBRTC, TEST_MESSAGE_TOPIC, SUBSCRIBER);
            subscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCBasicMessageStreaming. Message size = " + data.length);
                if (!Arrays.equals(data, message.getBytes()))
                    throw new RuntimeException("A wrong test message(" + new String(data) + ") is received.");

                Log.i(TAG, "The correct test message(" + new String(data) + ") is received.");
                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_MESSAGE_TOPIC, PUBLISHER);
            publisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            subscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 1000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = publisher.publish(publisherStream, TEST_MESSAGE_TOPIC, message.getBytes());
                if (isPublished)
                    break;
            }
            Log.i(TAG, "A message is sent through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            publisherStream.disconnect();

            subscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCBasicMessageStreaming, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCSendLargeMessage_P() {
        byte[] largeBytes = new byte[MAX_MESSAGE_SIZE * 2];
        new Random().nextBytes(largeBytes);

        try {
            Aitt subscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID + LARGE_DATA_PREFIX, wifiIP, true);
            subscriber.connect(brokerIp, PORT);
            AittStream subscriberStream = subscriber.createStream(Aitt.Protocol.WEBRTC, TEST_LARGE_MESSAGE_TOPIC, SUBSCRIBER);
            subscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCSendLargeMessage. Message size = " + data.length);
                if (!Arrays.equals(data, largeBytes)) {
                    throw new RuntimeException("A wrong large message is received.");
                }
                Log.i(TAG, "The correct large message is received. size = " + data.length);
                if (looper != null) {
                    looper.quit();
                }
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + LARGE_DATA_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_LARGE_MESSAGE_TOPIC, PUBLISHER);
            publisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            subscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 3000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = publisher.publish(publisherStream, TEST_LARGE_MESSAGE_TOPIC, largeBytes);
                if (isPublished)
                    break;
                Thread.sleep(SLEEP_INTERVAL);
            }
            Log.i(TAG, "A large message is sent through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            publisherStream.disconnect();

            subscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCSendLargeMessage, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCVideoStreamingNoConfig_N() {
        try {
            Aitt subscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID + VIDEO_PREFIX, wifiIP, true);
            subscriber.connect(brokerIp, PORT);
            AittStream subscriberStream = subscriber.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, SUBSCRIBER);
            subscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCVideoStreamingNoConfig");

                // The default frame width for publish is 640 which does not match the actual image width
                assertNotEquals("Wrong width", subscriberStream.getStreamWidth(), FRAME_WIDTH);

                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);
            publisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            subscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 2000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                // TODO: Replace publish
                boolean isPublished = publisher.publish(publisherStream, TEST_VIDEO_TOPIC, frameImageBytes);
                if (isPublished)
                    break;
            }
            Log.i(TAG, "Video transmission has started through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            publisherStream.disconnect();

            subscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCVideoStreamingNoConfig, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCVideoStreamingWithConfig_P() {
        try {
            Aitt subscriber = new Aitt(appContext, AITT_WEBRTC_SERVER_ID + VIDEO_PREFIX, wifiIP, true);
            subscriber.connect(brokerIp, PORT);
            AittStream subscriberStream = subscriber.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, SUBSCRIBER);
            subscriberStream.setReceiveCallback(data -> {
                Log.i(TAG, "A callback is received in testWebRTCVideoStreamingWithConfig");
                if (FRAME_WIDTH != subscriberStream.getStreamWidth())
                    throw new RuntimeException("Wrong frame width");
                if (FRAME_HEIGHT != subscriberStream.getStreamHeight())
                    throw new RuntimeException("Wrong frame height");

                Log.i(TAG, "The correct test image is received.");

                if (looper != null)
                    looper.quit();
            });
            Log.i(TAG, "A WebRTC server and a subscriber stream are created.");

            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
            configBuilder.setWidth(FRAME_WIDTH);
            configBuilder.setHeight(FRAME_HEIGHT);
            publisherStream.setConfig(configBuilder.build());

            publisherStream.start();
            Log.i(TAG, "A WebRTC client and a publisher stream are created.");

            subscriberStream.start();
            Log.i(TAG, "The subscriber stream starts.");

            int intervalSum = 0;
            while (intervalSum < 2000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            while (true) {
                boolean isPublished = publisher.publish(publisherStream, TEST_VIDEO_TOPIC, frameImageBytes);
                if (isPublished)
                    break;
            }
            Log.i(TAG, "Video transmission has started through the publisher stream.");

            Looper.loop();
            Log.i(TAG, "A looper is finished.");

            publisherStream.disconnect();

            subscriberStream.stop();
        } catch (Exception e) {
            fail("Failed testWebRTCVideoStreamingWithConfig, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCStreamNullConfig_N() {
        try {
            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfig config = null;
            assertThrows(IllegalArgumentException.class, () -> publisherStream.setConfig(config));

            publisherStream.disconnect();
        } catch (Exception e) {
            fail("Failed testWebRTCStreamNullConfig, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCStreamConfigWithoutHeight_P() {
        try {
            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
            configBuilder.setWidth(FRAME_WIDTH);
            publisherStream.setConfig(configBuilder.build());
            publisherStream.start();

            publisherStream.disconnect();
        } catch (Exception e) {
            fail("Failed testWebRTCStreamConfigWithoutHeight, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCStreamConfigWithoutWidth_P() {
        try {
            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
            configBuilder.setHeight(FRAME_HEIGHT);
            publisherStream.setConfig(configBuilder.build());
            publisherStream.start();

            publisherStream.disconnect();
        } catch (Exception e) {
            fail("Failed testWebRTCStreamConfigWithoutWidth, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCStreamInvalidWidth_N() {
        try {
            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
            configBuilder.setWidth(-1);
            configBuilder.setHeight(FRAME_HEIGHT);
            assertThrows(IllegalArgumentException.class, () -> publisherStream.setConfig(configBuilder.build()));

            publisherStream.disconnect();
        } catch (Exception e) {
            fail("Failed testWebRTCStreamInvalidWidth, (" + e + ")");
        }
    }

    @Test
    public void testWebRTCStreamInvalidHeight_N() {
        try {
            Aitt publisher = new Aitt(appContext, AITT_WEBRTC_CLIENT_ID + VIDEO_PREFIX, wifiIP, true);
            publisher.connect(brokerIp, PORT);
            AittStream publisherStream = publisher.createStream(Aitt.Protocol.WEBRTC, TEST_VIDEO_TOPIC, PUBLISHER);

            AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
            configBuilder.setWidth(FRAME_WIDTH);
            configBuilder.setHeight(-2);
            assertThrows(IllegalArgumentException.class, () -> publisherStream.setConfig(configBuilder.build()));

            publisherStream.disconnect();
        } catch (Exception e) {
            fail("Failed testWebRTCStreamInvalidHeight, (" + e + ")");
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
