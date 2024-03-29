package com.samsung.android.modules.webrtc;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockedStatic;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;

import android.content.Context;
import android.util.Log;

public class WebRTCUnitTest {
    @Mock
    private final Context context = mock(Context.class);

    private static final int WIDTH = 320;
    private static final int HEIGHT = 240;
    private static final int FPS = 30;

    private static MockedStatic<Log> mockedLog;

    @BeforeClass
    public static void beforeClass() {
        mockedLog = mockStatic(Log.class);
    }

    @AfterClass
    public static void afterClass() {
        mockedLog.close();
    }

    @Test
    public void testWebRTCPublisherConstructor_N() {
        try {
            assertThrows(IllegalArgumentException.class, () ->  { new WebRTCPublisher(null, WIDTH, HEIGHT, FPS); });
        }  catch (Exception e) {
            fail("Failed to create WebRTCPublisher" + e);
        }
    }

    @Test
    public void testWebRTCSubscriberConstructor_N() {
        try {
            assertThrows(IllegalArgumentException.class, () ->  { new WebRTCSubscriber(null, WIDTH, HEIGHT, FPS); });
        } catch (Exception e) {
            fail("Failed to create WebRTCSubscriber" + e);
        }
    }
}
