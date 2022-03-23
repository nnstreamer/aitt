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

public class AittMessage {
   private String topic;
   private String correlation;
   private String replyTopic;
   private int sequence;
   private boolean endSequence;
   private byte[] payload;

   public AittMessage() {
      setPayload(new byte[]{});
   }

   public AittMessage(byte[] payload) {
      setPayload(payload);
   }

   public String getTopic() {
      return topic;
   }

   public void setTopic(String topic) {
      this.topic = topic;
   }

   public String getCorrelation() {
      return correlation;
   }

   public void setCorrelation(String correlation) {
      this.correlation = correlation;
   }

   public String getReplyTopic() {
      return replyTopic;
   }

   public void setReplyTopic(String replyTopic) {
      this.replyTopic = replyTopic;
   }

   public int getSequence() {
      return sequence;
   }

   public void setSequence(int sequence) {
      this.sequence = sequence;
   }

   public void increaseSequence() {
      sequence = sequence+1;
   }

   public void setEndSequence(boolean endSequence) {
      this.endSequence = endSequence;
   }

   public boolean isEndSequence() {
      return endSequence;
   }

   public byte[] getPayload() {
      return payload;
   }

   public void clearPayload() {
      this.payload = new byte[]{};
   }

   public void setPayload(byte[] payload) {
      if (payload == null) {
         throw new NullPointerException();
      }
      this.payload = payload;
   }
}
