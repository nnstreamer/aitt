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
import static org.junit.Assert.fail;

import android.content.Context;

import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.aitt.Aitt;
import com.samsung.android.aitt.stream.AittStream;

import org.junit.BeforeClass;
import org.junit.Test;

public class RTSPInstrumentedTest {

    private static final String TEST_TOPIC = "android/test/rtsp";
    private static final String AITT_ID = "AITT_ANDROID";
    private static final String ERROR_MESSAGE_AITT_NULL = "An AITT instance is null.";
    private static final int PORT = 1883;

    private static String brokerIp;
    private static Context appContext;

    @BeforeClass
    public static void initialize() {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        appContext.getString(R.string.testBrokerIp);
    }

    @Test
    public void testRTSPBasicStreaming() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AittStream subscriber = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, SUBSCRIBER);
            subscriber.setReceiveCallback(/* TODO */);
            subscriber.start();

            AittStream publisher = aitt.createStream(Aitt.Protocol.RTSP, TEST_TOPIC, PUBLISHER);
            publisher.setConfig(/* TODO */);
            publisher.start();

            publisher.stop();
            subscriber.stop();

            aitt.destroyStream(publisher);
            aitt.destroyStream(subscriber);
        } catch (Exception e) {
            fail("Failed testRTSPBasicStreaming, (" + e + ")");
        }
    }
}
