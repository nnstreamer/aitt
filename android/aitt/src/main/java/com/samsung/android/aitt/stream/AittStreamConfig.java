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

// Class to set stream configuration parameters
public class AittStreamConfig {

   private final String url;
   private final String id;
   private final String password;
   // More fields can be added for other streams

   public AittStreamConfig(String url, String id, String password) {
      this.url = url;
      this.id = id;
      this.password = password;
   }

   public String getUrl() {
      return this.url;
   }

   public String getId() {
      return this.id;
   }

   public String getPassword() {
      return this.password;
   }
}
