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

#include <MQ.h>

#include "Config.h"
#include "IfaceServer.h"

class MqttServer : public IfaceServer {
  public:
    explicit MqttServer(const Config &config);
    virtual ~MqttServer();

    void SetConnectionStateChangedCb(
          std::function<void(ConnectionState)> connection_state_changed_cb) override
    {
        connection_state_changed_cb_ = connection_state_changed_cb;
    };
    void UnsetConnectionStateChangedCb(void) override { connection_state_changed_cb_ = nullptr; };

    bool IsConnected(void) override;
    int Connect(void) override;
    int Disconnect(void) override;
    int SendMessage(const std::string &peer_id, const std::string &msg) override;

    static std::string GetConnectionStateStr(ConnectionState state);
    void RegisterWithServer(void);
    void JoinRoom(const std::string &room_id);
    void SetConnectionState(ConnectionState state);
    ConnectionState GetConnectionState(void) const { return connection_state_; };
    std::string GetId(void) const { return id_; };
    std::string GetSourceId(void) const { return source_id_; };
    void SetSourceId(const std::string &source_id) { source_id_ = source_id; };

    void SetRoomMessageArrivedCb(std::function<void(const std::string &)> room_message_arrived_cb)
    {
        room_message_arrived_cb_ = room_message_arrived_cb;
    };
    void UnsetRoomMessageArrivedCb(void) { room_message_arrived_cb_ = nullptr; }

  private:
    void OnConnect();
    void OnDisconnect();
    void ConnectCallBack(int status);
    void HandleRoomTopic(aitt::MSG *msg, const std::string &topic, const void *data,
          const size_t datalen, void *user_data);
    void HandleSourceTopic(aitt::MSG *msg, const std::string &topic, const void *data,
          const size_t datalen, void *user_data);
    void HandleMessageTopic(aitt::MSG *msg, const std::string &topic, const void *data,
          const size_t datalen, void *user_data);

    std::string broker_ip_;
    int broker_port_;
    std::string id_;
    std::string room_id_;
    std::string source_id_;
    bool is_publisher_;
    aitt::MQ mq;

    ConnectionState connection_state_;
    std::function<void(ConnectionState)> connection_state_changed_cb_;
    std::function<void(const std::string &)> room_message_arrived_cb_;
};
