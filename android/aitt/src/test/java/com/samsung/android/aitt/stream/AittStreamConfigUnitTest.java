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
package com.samsung.android.aitt.stream;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class AittStreamConfigUnitTest {

   @Test
   public void testAittStreamConfigSetUrl_P() {
      AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
      String url = "rtsp://192.168.1.4:1935";
      configBuilder.setUrl(url);

      AittStreamConfig config = configBuilder.build();
      assertEquals("Received URL and set URl are equal", config.getUrl(), url);
   }

   @Test
   public void testAittStreamConfigSetId_P() {
      AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
      String id = "id1";
      configBuilder.setId(id);

      AittStreamConfig config = configBuilder.build();
      assertEquals("Received ID and set ID are equal", config.getId(), id);
   }

   @Test
   public void testAittStreamConfigSetPassword_P() {
      AittStreamConfigBuilder configBuilder = new AittStreamConfigBuilder();
      String password = "pwd1";
      configBuilder.setPassword(password);

      AittStreamConfig config = configBuilder.build();
      assertEquals("Received password and set password are equal", config.getPassword(), password);
   }
}
