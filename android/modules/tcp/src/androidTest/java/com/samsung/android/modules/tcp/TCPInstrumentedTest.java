package com.samsung.android.modules.tcp;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.fail;

import static java.lang.Thread.sleep;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import com.samsung.android.aitt.Aitt;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.StringTokenizer;
import java.util.concurrent.atomic.AtomicBoolean;

@RunWith(AndroidJUnit4.class)
public class TCPInstrumentedTest {

    private static Context appContext;

    private static final String TAG = "TCPInstrumentedTest";
    private static final String AITT_ID = "AITT_ANDROID";
    private static final String TEST_TOPIC = "android/test/tcp";
    private static final String TEST_TOPIC_SECURE = "android/test/tcp_secure";
    private static final String TEST_MESSAGE = "This is a test message for TCP protocol.";
    private static final String ERROR_MESSAGE_AITT_NULL = "An AITT instance is null.";
    private static final int PORT = 1883;
    private static final int SLEEP_LIMIT = 2000;
    private static final int SLEEP_INTERVAL = 100;

    private static String brokerIp;

    @BeforeClass
    public static void initialize() {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
        // IMPORTANT NOTE: Should give test arguments as follows.
        // if using Android studio: Run -> Edit Configurations -> Find 'Instrumentation arguments'
        //                         -> press '...' button -> add the name as "brokerIp" and the value
        //                         (Broker WiFi IP) of broker argument
        // if using gradlew commands: Add "-e brokerIp [Broker WiFi IP]"
        brokerIp = InstrumentationRegistry.getArguments().getString("brokerIp");
    }

    @Test
    public void testPublishWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            byte[] payload = TEST_MESSAGE.getBytes();
            aitt.publish(TEST_TOPIC, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);
        } catch (Exception e) {
            fail("Failed to execute testPublishWithTCP_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishWithTCPInvalidTopic_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "";
            byte[] payload = TEST_MESSAGE.getBytes();

            Assert.assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishWithTCPInvalidTopic_N, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            aitt.subscribe(TEST_TOPIC, message -> Log.i(TAG, "A subscription callback is called."), Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed to execute testSubscribeWithTCP_P, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPMultipleCallbacks_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            Aitt.SubscribeCallback callback1 = message -> {
            };

            Aitt.SubscribeCallback callback2 = message -> {
            };

            aitt.subscribe(TEST_TOPIC, callback1, Aitt.Protocol.TCP);
            aitt.subscribe(TEST_TOPIC + "_Second", callback2, Aitt.Protocol.TCP);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPMultipleCallbacks_P, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPInvalidTopic_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPInvalidTopic_N, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPInvalidCallback_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "topic";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, null, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPInvalidCallback_N, (" + e + ")");
        }
    }

    @Test
    public void testUnsubscribeWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);
            aitt.subscribe(TEST_TOPIC, message -> {
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            aitt.unsubscribe(TEST_TOPIC);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testUnsubscribeWithTCP_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishSubscribeWithTCP_P() {
        try {
            String wifiIp = wifiIpAddress();
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIp, false);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AtomicBoolean message_received = new AtomicBoolean(false);
            byte[] payload = TEST_MESSAGE.getBytes();
            aitt.subscribe(TEST_TOPIC, message -> {
                String _topic = message.getTopic();
                byte[] _payload = message.getPayload();
                Log.i(TAG, "Topic = " + _topic + ", Payload = " + Arrays.toString(_payload));
                Assert.assertEquals(_topic, TEST_TOPIC);
                Assert.assertArrayEquals(_payload, payload);
                String results = new String(_payload);
                Assert.assertEquals(results, TEST_MESSAGE);
                Log.i(TAG, "Received message = [" + results + "]");
                message_received.set(true);
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            int intervalSum = 0;
            while (intervalSum < SLEEP_LIMIT) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            aitt.publish(TEST_TOPIC, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);
            Log.i(TAG, "A message is sent through the publisher.");

            intervalSum = 0;
            while (intervalSum < SLEEP_LIMIT) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            Assert.assertTrue(message_received.get());
        } catch (Exception e) {
            fail("Failed to execute testPublishSubscribeWithTCP_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishWithTCPSecure_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            byte[] payload = TEST_MESSAGE.getBytes();
            aitt.publish(TEST_TOPIC, payload, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE, false);
        } catch (Exception e) {
            fail("Failed to execute testPublishWithTCPSecure_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishWithTCPSecureInvalidTopic_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "";
            byte[] payload = TEST_MESSAGE.getBytes();

            Assert.assertThrows(IllegalArgumentException.class, () -> aitt.publish(_topic, payload, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE, false));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testPublishWithTCPSecureInvalidTopic_N, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPSecure_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            aitt.subscribe(TEST_TOPIC, message -> Log.i(TAG, "A subscription callback is called."), Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed to execute testSubscribeWithTCPSecure_P, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPSecureMultipleCallbacks_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            Aitt.SubscribeCallback callback1 = message -> {
            };

            Aitt.SubscribeCallback callback2 = message -> {
            };

            aitt.subscribe(TEST_TOPIC, callback1, Aitt.Protocol.TCP_SECURE);
            aitt.subscribe(TEST_TOPIC + "_Second", callback2, Aitt.Protocol.TCP_SECURE);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPSecureMultipleCallbacks_P, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPSecureInvalidTopic_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, message -> {
            }, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPSecureInvalidTopic_N, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPSecureInvalidCallback_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            aitt.connect(brokerIp, PORT);

            String _topic = "topic";
            assertThrows(IllegalArgumentException.class, () -> aitt.subscribe(_topic, null, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE));

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPSecureInvalidCallback_N, (" + e + ")");
        }
    }

    @Test
    public void testUnsubscribeWithTCPSecure_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);
            aitt.subscribe(TEST_TOPIC, message -> {
            }, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE);

            aitt.unsubscribe(TEST_TOPIC);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testUnsubscribeWithTCPSecure_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishSubscribeWithTCPSecure_P() {
        try {
            String wifiIp = wifiIpAddress();
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIp, false);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(brokerIp, PORT);

            AtomicBoolean message_received = new AtomicBoolean(false);
            byte[] payload = TEST_MESSAGE.getBytes();
            aitt.subscribe(TEST_TOPIC_SECURE, message -> {
                String _topic = message.getTopic();
                byte[] _payload = message.getPayload();
                Log.i(TAG, "Topic = " + _topic + ", Payload = " + Arrays.toString(_payload));
                Assert.assertEquals(_topic, TEST_TOPIC_SECURE);
                Assert.assertArrayEquals(_payload, payload);
                String results = new String(_payload);
                Assert.assertEquals(results, TEST_MESSAGE);
                Log.i(TAG, "Received message = [" + results + "]");
                message_received.set(true);
            }, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE);

            int intervalSum = 0;
            while (intervalSum < SLEEP_LIMIT) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            aitt.publish(TEST_TOPIC_SECURE, payload, Aitt.Protocol.TCP_SECURE, Aitt.QoS.AT_LEAST_ONCE, false);
            Log.i(TAG, "A message is sent through the publisher.");

            intervalSum = 0;
            while (intervalSum < 5000) {
                Thread.sleep(SLEEP_INTERVAL);
                intervalSum += SLEEP_INTERVAL;
            }

            Assert.assertTrue(message_received.get());
        } catch (Exception e) {
            fail("Failed to execute testPublishSubscribeWithTCPSecure_P, (" + e + ")");
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
