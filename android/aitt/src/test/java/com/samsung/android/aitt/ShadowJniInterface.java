package com.samsung.android.aitt;

import com.samsung.android.aittnative.JniInterface;

import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@Implements(JniInterface.class)
public class ShadowJniInterface {
    private static boolean initReturn = true;

    @Implementation
    public long init(String id, String ip, boolean clearSession) {
        if (initReturn)
            return 1L;
        else
            return 0L;
    }

    @Implementation
    public void connect(long instance, final String host, int port) {
    }

    @Implementation
    public void publish(long instance, final String topic, final byte[] data, long datalen, int protocol, int qos, boolean retain) {
    }

    @Implementation
    public void disconnect(long instance) {

    }

    @Implementation
    public long subscribe(long instance, final String topic, int protocol, int qos) {
        return 1L;
    }

    @Implementation
    public void unsubscribe(long instance, final long aittSubId) {
    }

    public void setInitReturn(boolean initReturn) {
        this.initReturn = initReturn;
    }
}
