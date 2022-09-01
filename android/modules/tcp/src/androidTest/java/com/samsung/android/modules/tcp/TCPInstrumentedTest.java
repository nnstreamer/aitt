package com.samsung.android.modules.tcp;

import android.content.Context;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

import com.samsung.android.aitt.Aitt;

@RunWith(AndroidJUnit4.class)
public class TCPInstrumentedTest {

    private static Context appContext;

    private static final String TAG = "AITT-ANDROID";
    private static final String aittId = "aitt";
    private final String topic = "aitt/test";
    private final String message = "test message";

    @BeforeClass
    public static void initialize() throws InstantiationException {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Test
    public void testPublishWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, aittId);

            assertNotNull("AITT instance is null.", aitt);
            // TODO: aitt.connect(brokerIp, port);

            byte[] payload = message.getBytes();
            aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);

            // TODO: aitt.disconnect();
        } catch(Exception e) {
            fail("Failed to execute testPublishWithTCP_P " + e);
        }
    }

    @Test
    public void testSubscribeWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, aittId);

            assertNotNull("AITT instance is null.", aitt);
            // TODO: aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
                Log.i(TAG, "Subscription callback is called");
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            // TODO: aitt.disconnect();
        } catch(Exception e) {
            fail("Failed testSubscribeWithTCP_P " + e);
        }
    }

    @Test
    public void testPublishSubscribeWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, aittId);

            assertNotNull("AITT instance is null.", aitt);
            // TODO: aitt.connect(brokerIp, port);

            aitt.subscribe(topic, message -> {
                String _topic = message.getTopic();
                byte[] _payload = message.getPayload();
                Log.i(TAG, "Topic = " + _topic + ", Payload = " + _payload);
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            byte[] payload = message.getBytes();
            aitt.publish(topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);

            // TODO: aitt.disconnect();
        } catch(Exception e) {
            fail("Failed testPublishSubscribeWithTCP_P " + e);
        }
    }
}
