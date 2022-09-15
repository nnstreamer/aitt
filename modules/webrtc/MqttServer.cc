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
#include "MqttServer.h"

#include "MQProxy.h"
#include "aitt_internal.h"

#define MQTT_HANDLER_MSG_QOS 1
#define MQTT_HANDLER_MGMT_QOS 2

MqttServer::MqttServer(const Config &config)
      : mq(new aitt::MQProxy(config.GetLocalId(), AittOption(true, false))),
        connection_state_(ConnectionState::Disconnected)
{
    broker_ip_ = config.GetBrokerIp();
    broker_port_ = config.GetBrokerPort();
    id_ = config.GetLocalId();
    room_id_ = config.GetRoomId();
    source_id_ = config.GetSourceId();
    is_publisher_ = (id_ == source_id_);

    DBG("ID[%s] BROKER IP[%s] BROKER PORT [%d] ROOM[%s] %s", id_.c_str(), broker_ip_.c_str(),
          broker_port_, room_id_.c_str(), is_publisher_ ? "Publisher" : "Subscriber");

    mq->SetConnectionCallback(std::bind(&MqttServer::ConnectCallBack, this, std::placeholders::_1));
}

MqttServer::~MqttServer()
{
    // Prevent to call below callbacks after destructoring
    connection_state_changed_cb_ = nullptr;
    room_message_arrived_cb_ = nullptr;
}

void MqttServer::SetConnectionState(ConnectionState state)
{
    connection_state_ = state;
    if (connection_state_changed_cb_)
        connection_state_changed_cb_(state);
}

void MqttServer::ConnectCallBack(int status)
{
    if (status == AITT_CONNECTED)
        OnConnect();
    else
        OnDisconnect();
}

void MqttServer::OnConnect()
{
    INFO("Connected to signalling server");

    // Sometimes it seems that broker is silently disconnected/reconnected
    if (GetConnectionState() != ConnectionState::Connecting) {
        ERR("Invalid status");
        return;
    }

    SetConnectionState(ConnectionState::Connected);
    SetConnectionState(ConnectionState::Registering);
    try {
        RegisterWithServer();
    } catch (const std::runtime_error &e) {
        ERR("%s", e.what());
        SetConnectionState(ConnectionState::Connected);
    }
}

void MqttServer::OnDisconnect()
{
    INFO("mosquitto disconnected");

    SetConnectionState(ConnectionState::Disconnected);
    // TODO
}

void MqttServer::RegisterWithServer(void)
{
    if (connection_state_ != IfaceServer::ConnectionState::Registering) {
        ERR("Invaild status(%d)", (int)connection_state_);
        throw std::runtime_error("Invalid status");
    }

    // Notify Who is source?
    std::string source_topic = room_id_ + std::string("/source");
    if (is_publisher_) {
        mq->Publish(source_topic, id_.c_str(), id_.size(), AITT_QOS_EXACTLY_ONCE, true);
        SetConnectionState(ConnectionState::Registered);
    } else {
        mq->Subscribe(source_topic,
              std::bind(&MqttServer::HandleSourceTopic, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5),
              nullptr, AITT_QOS_EXACTLY_ONCE);
    }
}

void MqttServer::HandleSourceTopic(aitt::MSG *msg, const std::string &topic, const void *data,
      const size_t datalen, void *user_data)
{
    INFO("Source topic");
    if (connection_state_ != IfaceServer::ConnectionState::Registering) {
        ERR("Invaild status(%d)", (int)connection_state_);
        return;
    }

    if (is_publisher_) {
        ERR("Ignore");
    } else {
        std::string message(static_cast<const char *>(data), datalen);
        INFO("Set source ID %s", message.c_str());
        SetSourceId(message);
        SetConnectionState(ConnectionState::Registered);
    }
}

bool MqttServer::IsConnected(void)
{
    INFO("%s", __func__);

    return connection_state_ == IfaceServer::ConnectionState::Registered;
}

int MqttServer::Connect(void)
{
    std::string will_message = std::string("ROOM_PEER_LEFT ") + id_;
    mq->SetWillInfo(room_id_, will_message.c_str(), will_message.size(), AITT_QOS_EXACTLY_ONCE,
          false);

    SetConnectionState(ConnectionState::Connecting);
    mq->Connect(broker_ip_, broker_port_, std::string(), std::string());

    return 0;
}

int MqttServer::Disconnect(void)
{
    if (is_publisher_) {
        INFO("remove retained");
        std::string source_topic = room_id_ + std::string("/source");
        mq->Publish(source_topic, nullptr, 0, AITT_QOS_AT_LEAST_ONCE, true);
    }

    std::string left_message = std::string("ROOM_PEER_LEFT ") + id_;
    mq->Publish(room_id_, left_message.c_str(), left_message.size(), AITT_QOS_AT_LEAST_ONCE, false);

    mq->Disconnect();

    room_id_ = std::string("");

    SetConnectionState(ConnectionState::Disconnected);
    return 0;
}

int MqttServer::SendMessage(const std::string &peer_id, const std::string &msg)
{
    if (room_id_.empty()) {
        ERR("Invaild status");
        return -1;
    }
    if (peer_id.size() == 0 || msg.size() == 0) {
        ERR("Invalid parameter");
        return -1;
    }

    std::string receiver_topic = room_id_ + std::string("/") + peer_id;
    std::string server_formatted_msg = "ROOM_PEER_MSG " + id_ + " " + msg;
    mq->Publish(receiver_topic, server_formatted_msg.c_str(), server_formatted_msg.size(),
          AITT_QOS_AT_LEAST_ONCE);

    return 0;
}

std::string MqttServer::GetConnectionStateStr(ConnectionState state)
{
    std::string state_str;
    switch (state) {
    case IfaceServer::ConnectionState::Disconnected: {
        state_str = std::string("Disconnected");
        break;
    }
    case IfaceServer::ConnectionState::Connecting: {
        state_str = std::string("Connecting");
        break;
    }
    case IfaceServer::ConnectionState::Connected: {
        state_str = std::string("Connected");
        break;
    }
    case IfaceServer::ConnectionState::Registering: {
        state_str = std::string("Registering");
        break;
    }
    case IfaceServer::ConnectionState::Registered: {
        state_str = std::string("Registered");
        break;
    }
    }

    return state_str;
}

void MqttServer::JoinRoom(const std::string &room_id)
{
    if (room_id.empty() || room_id != room_id_) {
        ERR("Invaild room id");
        throw std::runtime_error(std::string("Invalid room_id"));
    }

    // Subscribe PEER_JOIN PEER_LEFT
    mq->Subscribe(room_id_,
          std::bind(&MqttServer::HandleRoomTopic, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                std::placeholders::_5),
          nullptr, AITT_QOS_EXACTLY_ONCE);

    // Subscribe PEER_MSG
    std::string receiving_topic = room_id + std::string("/") + id_;
    mq->Subscribe(receiving_topic,
          std::bind(&MqttServer::HandleMessageTopic, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                std::placeholders::_5),
          nullptr, AITT_QOS_AT_LEAST_ONCE);

    INFO("Subscribe room topics");

    if (!is_publisher_) {
        std::string join_message = std::string("ROOM_PEER_JOINED ") + id_;
        mq->Publish(room_id_, join_message.c_str(), join_message.size(), AITT_QOS_EXACTLY_ONCE);
    }
}

void MqttServer::HandleRoomTopic(aitt::MSG *msg, const std::string &topic, const void *data,
      const size_t datalen, void *user_data)
{
    std::string message(static_cast<const char *>(data), datalen);
    INFO("Room topic(%s, %s)", topic.c_str(), message.c_str());

    std::string peer_id;
    if (message.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        peer_id = message.substr(17, std::string::npos);
    } else if (message.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        peer_id = message.substr(15, std::string::npos);
    } else {
        ERR("Invalid type of Room message %s", message.c_str());
        return;
    }

    if (peer_id == id_) {
        ERR("ignore");
        return;
    }

    if (is_publisher_) {
        if (room_message_arrived_cb_)
            room_message_arrived_cb_(message);
    } else {
        // TODO: ADHOC, will handle this by room
        if (peer_id != source_id_) {
            ERR("peer(%s) is Not source(%s)", peer_id.c_str(), source_id_.c_str());
            return;
        }

        if (room_message_arrived_cb_)
            room_message_arrived_cb_(message);
    }
}

void MqttServer::HandleMessageTopic(aitt::MSG *msg, const std::string &topic, const void *data,
      const size_t datalen, void *user_data)
{
    INFO("Message topic");
    std::string message(static_cast<const char *>(data), datalen);

    if (room_message_arrived_cb_)
        room_message_arrived_cb_(message);
}
