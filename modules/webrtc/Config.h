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

#pragma once

#include <string>

class Config {
  public:
    Config() : disable_ssl_(false), broker_port_(0), user_data_length_(0) {};
    Config(const std::string &id, const std::string &broker_ip, int broker_port,
          const std::string &room_id, const std::string &source_id = std::string(""))
          : local_id_(id),
            room_id_(room_id),
            source_id_(source_id),
            disable_ssl_(false),
            broker_ip_(broker_ip),
            broker_port_(broker_port),
            user_data_length_(0){};
    std::string GetLocalId(void) const { return local_id_; };
    void SetLocalId(const std::string &local_id) { local_id_ = local_id; };
    std::string GetRoomId(void) const { return room_id_; };
    void SetRoomId(const std::string &room_id) { room_id_ = room_id; };
    std::string GetSourceId(void) const { return source_id_; };
    void SetSourceId(const std::string &source_id) { source_id_ = source_id; };
    void SetSignalingServerUrl(const std::string &signaling_server_url)
    {
        signaling_server_url_ = signaling_server_url;
    };
    std::string GetSignalingServerUrl(void) const { return signaling_server_url_; };
    void SetDisableSSl(bool disable_ssl) { disable_ssl_ = disable_ssl; };
    bool GetDisableSSl(void) const { return disable_ssl_; };
    std::string GetBrokerIp(void) const { return broker_ip_; };
    void SetBrokerIp(const std::string &broker_ip) { broker_ip_ = broker_ip; };
    int GetBrokerPort(void) const { return broker_port_; };
    void SetBrokerPort(int port) { broker_port_ = port; };
    unsigned int GetUserDataLength(void) const { return user_data_length_; };
    void SetUserDataLength(unsigned int user_data_length) { user_data_length_ = user_data_length; };

  private:
    std::string local_id_;
    std::string room_id_;
    std::string source_id_;
    std::string signaling_server_url_;
    bool disable_ssl_;
    std::string broker_ip_;
    int broker_port_;
    unsigned int user_data_length_;
};
