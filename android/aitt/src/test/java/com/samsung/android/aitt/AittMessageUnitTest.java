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

package com.samsung.android.aitt;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import org.junit.Test;

public class AittMessageUnitTest {

    private final String topic = "aittMessage/topic";
    private final String correlation = "correlation";
    private final String replyTopic = "aittMessage/replyTopic";
    private final int sequence = 007;
    private final boolean endSequence = true;
    private final String message = "Aitt Message";

    @Test
    public void testAittMessageInitialize_P() {
        AittMessage aittMessage = new AittMessage();
        assertNotNull("Not null AittMessage Object", aittMessage);
    }

    @Test
    public void testAittMessageInitializePayload_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        assertNotNull("Not null AittMessage Object", aittMessage);
    }

    @Test(expected = NullPointerException.class)
    public void testAittMessageInitializeInvalidPayload_N() throws NullPointerException {
        byte[] payload = null;
        AittMessage aittMessage = new AittMessage(payload);
        assertNull("Null AittMessage Object", aittMessage);
    }

    @Test
    public void testTopic_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.setTopic(topic);
        String newTopic = aittMessage.getTopic();
        assertEquals("Received topic and set topic are equal", topic, newTopic);
    }

    @Test
    public void testInvalidTopic_N() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        String newTopic = aittMessage.getTopic();
        assertNull("Received topic is null", newTopic);
    }

    @Test
    public void testCorrelation_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.setCorrelation(correlation);
        String newCorrelation = aittMessage.getCorrelation();
        assertEquals("Received correlation and set correlation are equal", correlation, newCorrelation);
    }

    @Test
    public void testInvalidCorrelation_N() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        String newCorrelation = aittMessage.getCorrelation();
        assertNull("Received Correlation is null", newCorrelation);
    }

    @Test
    public void testReplyTopic_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.setReplyTopic(replyTopic);
        String newReplyTopic = aittMessage.getReplyTopic();
        assertEquals("Received replyTopic and set replyTopic are equal", replyTopic, newReplyTopic);
    }

    @Test
    public void testInvalidReplyTopic_N() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        String newReplyTopic = aittMessage.getReplyTopic();
        assertNull("Received replyTopic is null", newReplyTopic);
    }

    @Test
    public void testSequence_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.setSequence(sequence);
        aittMessage.increaseSequence();
        int newSequence = aittMessage.getSequence();
        assertEquals("Received sequence and set sequence are equal", sequence + 1, newSequence);
    }

    @Test
    public void testInvalidSequence_N() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        int newSequence = aittMessage.getSequence();
        assertEquals("Received sequence is null", 0, newSequence);
    }

    @Test
    public void testEndSequence_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.setEndSequence(endSequence);
        boolean bool = aittMessage.isEndSequence();
        assertEquals("Received endSequence and set endSequence are equal", endSequence, bool);
    }

    @Test
    public void testInvalidEndSequence_N() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        boolean bool = aittMessage.isEndSequence();
        assertFalse("Received endSequence is false", bool);
    }

    @Test
    public void testPayload_P() {
        AittMessage aittMessage = new AittMessage();
        byte[] payload = message.getBytes();
        aittMessage.setPayload(payload);
        byte[] newPayload = aittMessage.getPayload();
        assertEquals("Received payload and set payload are equal", payload, newPayload);
    }

    @Test(expected = NullPointerException.class)
    public void testInvalidPayload_N() throws NullPointerException {
        AittMessage aittMessage = new AittMessage();
        byte[] payload = null;
        aittMessage.setPayload(payload);
    }

    @Test(expected = NullPointerException.class)
    public void testNullPayload_N() throws NullPointerException{
        AittMessage aittMessage = new AittMessage(null);
    }

    @Test
    public void testClearPayload_P() {
        byte[] payload = message.getBytes();
        AittMessage aittMessage = new AittMessage(payload);
        aittMessage.clearPayload();
        byte[] newPayload = aittMessage.getPayload();
        assertEquals("Received payload and expected payload are equal", 0, newPayload.length);
    }
}
