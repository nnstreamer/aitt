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
import com.samsung.android.aitt.AittMessage;

import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.StringTokenizer;
import java.util.concurrent.atomic.AtomicBoolean;

@RunWith(AndroidJUnit4.class)
public class TCPInstrumentedTest {

    private static Context appContext;

    private static final String TAG = "AITT-ANDROID";
    private static final String AITT_ID = "AITT_ANDROID";
    private static final String BROKER_IP = "192.168.1.59"; // TODO: Replace with 'localhost' value.
    private static final int PORT = 1883;
    private static final String TEST_TOPIC = "android/test/tcp";
    private static final String TEST_MESSAGE = "This is a test message for TCP protocol.";
    private static final String ERROR_MESSAGE_AITT_NULL = "An AITT instance is null.";

    @BeforeClass
    public static void initialize() {
        appContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    }

    @Test
    public void testPublishWithTCP_P() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(BROKER_IP, PORT);

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
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(BROKER_IP, PORT);

            String _topic = "";
            byte[] payload = TEST_MESSAGE.getBytes();

            Assert.assertThrows(IllegalArgumentException.class, () -> {
                aitt.publish(_topic, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);
            });

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
            aitt.connect(BROKER_IP, PORT);

            aitt.subscribe(TEST_TOPIC, message -> {
                Log.i(TAG, "A subscription callback is called.");
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

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
            aitt.connect(BROKER_IP, PORT);

            Aitt.SubscribeCallback callback1 = message -> {
            };

            Aitt.SubscribeCallback callback2 = message -> {
            };

            aitt.subscribe(TEST_TOPIC, callback1);
            aitt.subscribe(TEST_TOPIC + "_Second", callback2);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPMultipleCallbacks_P, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPInvalidTopic_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(BROKER_IP, PORT);

            String _topic = "";
            assertThrows(IllegalArgumentException.class, () -> {
                aitt.subscribe(_topic, message -> {
                }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);
            });

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testSubscribeWithTCPInvalidTopic_N, (" + e + ")");
        }
    }

    @Test
    public void testSubscribeWithTCPInvalidCallback_N() {
        try {
            Aitt aitt = new Aitt(appContext, AITT_ID);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(BROKER_IP, PORT);

            String _topic = "topic";
            assertThrows(IllegalArgumentException.class, () -> {
                aitt.subscribe(_topic, null, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);
            });

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
            aitt.connect(BROKER_IP, PORT);
            aitt.subscribe(TEST_TOPIC, message -> {
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            aitt.unsubscribe(TEST_TOPIC);

            aitt.disconnect();
        } catch (Exception e) {
            fail("Failed testUnsubscribe_P, (" + e + ")");
        }
    }

    @Test
    public void testPublishSubscribeWithTCP_P() {
        try {
            String wifiIp = wifiIpAddress();
            Aitt aitt = new Aitt(appContext, AITT_ID, wifiIp, false);
            assertNotNull(ERROR_MESSAGE_AITT_NULL, aitt);
            aitt.connect(BROKER_IP, PORT);

            AtomicBoolean message_received = new AtomicBoolean(false);
            byte[] payload = TEST_MESSAGE.getBytes();
            aitt.subscribe(TEST_TOPIC, message -> {
                String _topic = message.getTopic();
                byte[] _payload = message.getPayload();
                Log.i(TAG, "Topic = " + _topic + ", Payload = " + _payload);
                Assert.assertEquals(_topic, TEST_TOPIC);
                Assert.assertArrayEquals(_payload, payload);
                String results = new String(_payload);
                Assert.assertEquals(results, TEST_MESSAGE);
                Log.i(TAG, "Received message = [" + results + "]");
                message_received.set(true);
            }, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE);

            sleep(500);

            aitt.publish(TEST_TOPIC, payload, Aitt.Protocol.TCP, Aitt.QoS.AT_LEAST_ONCE, false);

            sleep(500);

            Assert.assertTrue(message_received.get());
        } catch (Exception e) {
            fail("Failed to execute testPublishSubscribeWithTCP_P, (" + e + ")");
        }
    }

    private static String wifiIpAddress() {
        ConnectivityManager connectivityManager = (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network currentNetwork = connectivityManager.getActiveNetwork();
        LinkProperties linkProperties = connectivityManager.getLinkProperties(currentNetwork);
        for (LinkAddress linkAddress : linkProperties.getLinkAddresses()) {
            String targetIp = linkAddress.toString();
            if (targetIp.contains("192.168") == true) {
                StringTokenizer tokenizer = new StringTokenizer(targetIp, "/");
                if (tokenizer.hasMoreTokens() == true)
                    return tokenizer.nextToken();
            }
        }

        return null;
    }
}
