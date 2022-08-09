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

   /**
    * AittMessage constructor for initializing
    */
   public AittMessage() {
      setPayload(new byte[]{});
   }

   /**
    * AittMessage constructor with params for initializing
    * @param payload The message data set as payload
    */
   public AittMessage(byte[] payload) {
      setPayload(payload);
   }

   /**
    * Method to get Topic assigned with AittMessage object
    * @return The string topic
    */
   public String getTopic() {
      return topic;
   }

   /**
    * Method to set topic to AittMessage object
    * @param topic String topic to be assigned to the object
    */
   public void setTopic(String topic) {
      this.topic = topic;
   }

   /**
    * Method to get Correlation
    * @return the correlation
    */
   public String getCorrelation() {
      return correlation;
   }

   /**
    * Method to set correlation
    * @param correlation correlation string to be set
    */
   public void setCorrelation(String correlation) {
      this.correlation = correlation;
   }

   /**
    * Method to get the reply topic
    * @return the string of reply topic
    */
   public String getReplyTopic() {
      return replyTopic;
   }

   /**
    * Method to set reply topic
    * @param replyTopic String that is set as reply topic
    */
   public void setReplyTopic(String replyTopic) {
      this.replyTopic = replyTopic;
   }

   /**
    * Method used to get sequence
    * @return the sequence
    */
   public int getSequence() {
      return sequence;
   }

   /**
    * Method used to set sequence
    * @param sequence the sequence value to be set
    */
   public void setSequence(int sequence) {
      this.sequence = sequence;
   }

   /**
    * Method used to increase the sequence by one
    */
   public void increaseSequence() {
      sequence = sequence+1;
   }

   /**
    * Method used to set endSequence
    * @param endSequence boolean value to be set to end sequence
    */
   public void setEndSequence(boolean endSequence) {
      this.endSequence = endSequence;
   }

   /**
    * Method used to get if sequence is ended
    * @return The state of sequence
    */
   public boolean isEndSequence() {
      return endSequence;
   }

   /**
    * Method used to retrieve payload
    * @return The data in byte[] format
    */
   public byte[] getPayload() {
      return payload;
   }

   /**
    * Method used to clear the payload
    */
   public void clearPayload() {
      this.payload = new byte[]{};
   }

   /**
    * Method used to set payload to AittMessage object
    * @param payload the byte[] message/payload to be set
    */
   public void setPayload(byte[] payload) {
      if (payload == null) {
         throw new NullPointerException();
      }
      this.payload = payload;
   }
}
