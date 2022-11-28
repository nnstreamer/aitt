package com.samsung.android.modules.webrtc;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockedStatic;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.util.Log;

public class WebRTCUnitTest {
    @Mock
    private final Context context = mock(Context.class);

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
    public void testWebRTCServerConstructor_P() {
        WebRTCServer webRTCServer = new WebRTCServer(context);
        assertNotNull("WebRTCServer instance not null", webRTCServer);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testWebRTCServerConstructor_N() throws IllegalArgumentException {
        WebRTCServer webRTCServer = new WebRTCServer(null);
        assertNotNull("WebRTCServer instance not null", webRTCServer);
    }

    @Test
    public void testWebRTCServerStart_P() {
        WebRTCServer webRTCServer = new WebRTCServer(context);
        webRTCServer.setDataCallback(frame -> {
        });
        webRTCServer.start();
    }

    @Test
    public void testWebRTCServerStart_N() {
        when(Log.e(any(String.class), any(String.class))).thenReturn(any(Integer.class), any(Integer.class));
        WebRTCServer webRTCServer = new WebRTCServer(context);
        webRTCServer.setDataCallback(null);
        assertEquals("Fail to start a WebRTC server", -1, webRTCServer.start());
    }

    @Test
    public void testWebRTCServerStop_P() {
        WebRTCServer webRTCServer = new WebRTCServer(context);
        webRTCServer.setDataCallback(frame -> {
        });
        webRTCServer.start();

        webRTCServer.stop();
    }

    @Test
    public void testWebRTCConstructor_P() {
        WebRTC webRTC = new WebRTC(context);
        assertNotNull("WebRTC instance not null", webRTC);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testWebRTCConstructor_N() throws IllegalArgumentException {
        WebRTC webRTC = new WebRTC(null);
        assertNotNull("WebRTC instance not null", webRTC);
    }
}
