/*
 * Copyright 2022-2023 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include <string>

#define RTSP_INFO_SERVER_STATE "server_state"
#define RTSP_INFO_URI "URI"
#define RTSP_INFO_ID "ID"
#define RTSP_INFO_PASSWORD "Password"

class RTSPInfo {
  public:
    RTSPInfo(void);
    ~RTSPInfo(void);

    void SetURI(const std::string &uri);
    std::string GetURI();
    void SetID(const std::string &id);
    std::string GetID();
    void SetPassword(const std::string &password);
    std::string GetPassword();

    std::string GetCompleteURI();

  private:
    std::string uri_;
    std::string id_;
    std::string password_;
};
