/*
 * Copyright (c) 2021-2022 Samsung Electronics Co., Ltd All Rights Reserved
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

#include <glib.h>
#include <gtest/gtest.h>

#include <chrono>
#include <set>
#include <thread>

#include "AITTEx.h"
#include "Config.h"
#include "MqttServer.h"
#include "aitt_internal.h"

#define DEFAULT_BROKER_IP "127.0.0.1"
#define DEFAULT_BROKER_PORT 1883

#define DEFAULT_WEBRTC_SRC_ID "webrtc_src"
#define DEFAULT_FIRST_SINK_ID "webrtc_first_sink"
#define DEFAULT_SECOND_SINK_ID "webrtc_second_sink"
#define DEFAULT_ROOM_ID AITT_MANAGED_TOPIC_PREFIX "webrtc/room/Room.webrtc"

class MqttServerTest : public testing::Test {
  protected:
    void SetUp() override
    {
        webrtc_src_config_ = Config(DEFAULT_WEBRTC_SRC_ID, DEFAULT_BROKER_IP, DEFAULT_BROKER_PORT,
              DEFAULT_ROOM_ID, DEFAULT_WEBRTC_SRC_ID);
        webrtc_first_sink_config_ = Config(DEFAULT_FIRST_SINK_ID, DEFAULT_BROKER_IP,
              DEFAULT_BROKER_PORT, DEFAULT_ROOM_ID);
        webrtc_second_sink_config_ = Config(DEFAULT_SECOND_SINK_ID, DEFAULT_BROKER_IP,
              DEFAULT_BROKER_PORT, DEFAULT_ROOM_ID);

        loop_ = g_main_loop_new(nullptr, FALSE);
    }

    void TearDown() override { g_main_loop_unref(loop_); }

  protected:
    Config webrtc_src_config_;
    Config webrtc_first_sink_config_;
    Config webrtc_second_sink_config_;
    GMainLoop *loop_;
};
static void onConnectionStateChanged(IfaceServer::ConnectionState state, MqttServer &server,
      GMainLoop *loop)
{
    if (state == IfaceServer::ConnectionState::Registered) {
        EXPECT_EQ(server.IsConnected(), true) << "should return Connected";
        g_main_loop_quit(loop);
    }
}

TEST_F(MqttServerTest, Positive_Connect_Anytime)
{
    try {
        MqttServer server(webrtc_src_config_);
        EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";

        auto on_connection_state_changed =
              std::bind(onConnectionStateChanged, std::placeholders::_1, std::ref(server), loop_);
        server.SetConnectionStateChangedCb(on_connection_state_changed);

        server.Connect();

        g_main_loop_run(loop_);

        server.UnsetConnectionStateChangedCb();
        server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}
static int Positive_Connect_Src_Sinks_Anytime_connect_count;
static void onConnectionStateChangedPositive_Connect_Src_Sinks_Anytime(
      IfaceServer::ConnectionState state, MqttServer &server, GMainLoop *loop)
{
    if (state == IfaceServer::ConnectionState::Registered) {
        EXPECT_EQ(server.IsConnected(), true) << "should return Connected";
        ++Positive_Connect_Src_Sinks_Anytime_connect_count;
        if (Positive_Connect_Src_Sinks_Anytime_connect_count == 3) {
            g_main_loop_quit(loop);
        }
    }
}

TEST_F(MqttServerTest, Positive_Connect_Src_Sinks_Anytime)
{
    try {
        Positive_Connect_Src_Sinks_Anytime_connect_count = 0;
        MqttServer src_server(webrtc_src_config_);
        EXPECT_EQ(src_server.IsConnected(), false) << "Should return not connected";

        auto on_src_connection_state_changed =
              std::bind(onConnectionStateChangedPositive_Connect_Src_Sinks_Anytime,
                    std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetConnectionStateChangedCb(on_src_connection_state_changed);

        src_server.Connect();

        MqttServer first_sink_server(webrtc_first_sink_config_);
        EXPECT_EQ(first_sink_server.IsConnected(), false) << "Should return not connected";

        auto on_first_sink_connection_state_changed =
              std::bind(onConnectionStateChangedPositive_Connect_Src_Sinks_Anytime,
                    std::placeholders::_1, std::ref(first_sink_server), loop_);
        first_sink_server.SetConnectionStateChangedCb(on_first_sink_connection_state_changed);

        first_sink_server.Connect();

        MqttServer second_sink_server(webrtc_second_sink_config_);
        EXPECT_EQ(second_sink_server.IsConnected(), false) << "Should return not connected";

        auto on_second_sink_connection_state_changed =
              std::bind(onConnectionStateChangedPositive_Connect_Src_Sinks_Anytime,
                    std::placeholders::_1, std::ref(second_sink_server), loop_);
        second_sink_server.SetConnectionStateChangedCb(on_second_sink_connection_state_changed);

        second_sink_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        first_sink_server.UnsetConnectionStateChangedCb();
        second_sink_server.UnsetConnectionStateChangedCb();
        src_server.Disconnect();
        first_sink_server.Disconnect();
        second_sink_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

TEST_F(MqttServerTest, Negative_Disconnect_Anytime)
{
    EXPECT_THROW(
          {
              try {
                  MqttServer server(webrtc_src_config_);
                  EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";

                  server.Disconnect();

                  g_main_loop_run(loop_);
              } catch (const aitt::AITTEx &e) {
                  // and this tests that it has the correct message
                  throw;
              }
          },
          aitt::AITTEx);
}

TEST_F(MqttServerTest, Positive_Disconnect_Anytime)
{
    try {
        MqttServer server(webrtc_src_config_);
        EXPECT_EQ(server.IsConnected(), false);

        auto on_connection_state_changed =
              std::bind(onConnectionStateChanged, std::placeholders::_1, std::ref(server), loop_);
        server.SetConnectionStateChangedCb(on_connection_state_changed);

        server.Connect();

        g_main_loop_run(loop_);

        server.UnsetConnectionStateChangedCb();
        server.Disconnect();

        EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

TEST_F(MqttServerTest, Negative_Register_Anytime)
{
    EXPECT_THROW(
          {
              try {
                  MqttServer server(webrtc_src_config_);
                  EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";

                  server.RegisterWithServer();
              } catch (const std::runtime_error &e) {
                  // and this tests that it has the correct message
                  throw;
              }
          },
          std::runtime_error);
}

TEST_F(MqttServerTest, Negative_JoinRoom_Invalid_Parameter_Anytime)
{
    EXPECT_THROW(
          {
              try {
                  MqttServer server(webrtc_src_config_);
                  EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";

                  server.JoinRoom(std::string("InvalidRoomId"));

              } catch (const std::runtime_error &e) {
                  // and this tests that it has the correct message
                  throw;
              }
          },
          std::runtime_error);
}

static void joinRoomOnRegisteredQuit(IfaceServer::ConnectionState state, MqttServer &server,
      GMainLoop *loop)
{
    if (state != IfaceServer::ConnectionState::Registered) {
        return;
    }

    EXPECT_EQ(server.IsConnected(), true) << "should return Connected";
    try {
        server.JoinRoom(DEFAULT_ROOM_ID);
        g_main_loop_quit(loop);
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

TEST_F(MqttServerTest, Positive_JoinRoom_Anytime)
{
    try {
        MqttServer server(webrtc_src_config_);
        EXPECT_EQ(server.IsConnected(), false) << "Should return not connected";

        auto join_room_on_registered =
              std::bind(joinRoomOnRegisteredQuit, std::placeholders::_1, std::ref(server), loop_);
        server.SetConnectionStateChangedCb(join_room_on_registered);

        server.Connect();

        g_main_loop_run(loop_);

        server.UnsetConnectionStateChangedCb();
        server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

static void joinRoomOnRegistered(IfaceServer::ConnectionState state, MqttServer &server)
{
    if (state != IfaceServer::ConnectionState::Registered) {
        return;
    }

    EXPECT_EQ(server.IsConnected(), true) << "should return Connected";
    try {
        server.JoinRoom(DEFAULT_ROOM_ID);
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

static void onSrcMessage(const std::string &msg, MqttServer &server, GMainLoop *loop)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        std::string peer_id = msg.substr(17, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_FIRST_SINK_ID)), 0)
              << "Not expected peer" << peer_id;

    } else if (msg.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        std::string peer_id = msg.substr(15, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_FIRST_SINK_ID)), 0)
              << "Not expected peer" << peer_id;
        g_main_loop_quit(loop);
    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

static void onSinkMessage(const std::string &msg, MqttServer &server, GMainLoop *loop)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        std::string peer_id = msg.substr(17, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer" << peer_id;
        server.Disconnect();
    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

TEST_F(MqttServerTest, Positive_src_sink)
{
    try {
        MqttServer src_server(webrtc_src_config_);
        auto join_room_on_registered_src =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(src_server));
        src_server.SetConnectionStateChangedCb(join_room_on_registered_src);

        auto on_src_message =
              std::bind(onSrcMessage, std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetRoomMessageArrivedCb(on_src_message);
        src_server.Connect();

        MqttServer sink_server(webrtc_first_sink_config_);
        auto join_room_on_registered_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(sink_server));
        sink_server.SetConnectionStateChangedCb(join_room_on_registered_sink);

        auto on_sink_message =
              std::bind(onSinkMessage, std::placeholders::_1, std::ref(sink_server), loop_);
        sink_server.SetRoomMessageArrivedCb(on_sink_message);

        sink_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        sink_server.UnsetConnectionStateChangedCb();
        src_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

TEST_F(MqttServerTest, Positive_sink_src)
{
    try {
        MqttServer sink_server(webrtc_first_sink_config_);
        auto join_room_on_registered_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(sink_server));
        sink_server.SetConnectionStateChangedCb(join_room_on_registered_sink);

        auto on_sink_message =
              std::bind(onSinkMessage, std::placeholders::_1, std::ref(sink_server), loop_);
        sink_server.SetRoomMessageArrivedCb(on_sink_message);

        sink_server.Connect();

        MqttServer src_server(webrtc_src_config_);
        auto join_room_on_registered_src =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(src_server));
        src_server.SetConnectionStateChangedCb(join_room_on_registered_src);

        auto on_src_message =
              std::bind(onSrcMessage, std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetRoomMessageArrivedCb(on_src_message);
        src_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        sink_server.UnsetConnectionStateChangedCb();
        src_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

static void onSrcMessageDisconnect(const std::string &msg, MqttServer &server, GMainLoop *loop)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        std::string peer_id = msg.substr(17, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_FIRST_SINK_ID)), 0)
              << "Not expected peer" << peer_id;
        server.Disconnect();

    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

static void onSinkMessageDisconnect(const std::string &msg, MqttServer &server, GMainLoop *loop)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        std::string peer_id = msg.substr(17, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer" << peer_id;
    } else if (msg.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        std::string peer_id = msg.substr(15, std::string::npos);
        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer" << peer_id;
        g_main_loop_quit(loop);
    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

TEST_F(MqttServerTest, Positive_src_sink_disconnect_src_first_Anytime)
{
    try {
        MqttServer src_server(webrtc_src_config_);
        auto join_room_on_registered_src =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(src_server));
        src_server.SetConnectionStateChangedCb(join_room_on_registered_src);

        auto on_src_message =
              std::bind(onSrcMessageDisconnect, std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetRoomMessageArrivedCb(on_src_message);
        src_server.Connect();

        MqttServer sink_server(webrtc_first_sink_config_);
        auto join_room_on_registered_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(sink_server));
        sink_server.SetConnectionStateChangedCb(join_room_on_registered_sink);

        auto on_sink_message = std::bind(onSinkMessageDisconnect, std::placeholders::_1,
              std::ref(sink_server), loop_);
        sink_server.SetRoomMessageArrivedCb(on_sink_message);

        sink_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        sink_server.UnsetConnectionStateChangedCb();
        sink_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

TEST_F(MqttServerTest, Positive_sink_src_disconnect_src_first_Anytime)
{
    try {
        MqttServer sink_server(webrtc_first_sink_config_);
        auto join_room_on_registered_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(sink_server));
        sink_server.SetConnectionStateChangedCb(join_room_on_registered_sink);

        auto on_sink_message = std::bind(onSinkMessageDisconnect, std::placeholders::_1,
              std::ref(sink_server), loop_);
        sink_server.SetRoomMessageArrivedCb(on_sink_message);

        sink_server.Connect();

        MqttServer src_server(webrtc_src_config_);
        auto join_room_on_registered_src =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(src_server));
        src_server.SetConnectionStateChangedCb(join_room_on_registered_src);

        auto on_src_message =
              std::bind(onSrcMessageDisconnect, std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetRoomMessageArrivedCb(on_src_message);
        src_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        sink_server.UnsetConnectionStateChangedCb();
        sink_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}

static int handled_sink;
static int expected_sink;

std::set<std::string> sink_set;

static void onSrcMessageThreeWay(const std::string &msg, MqttServer &server, GMainLoop *loop)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        auto peer_id = msg.substr(17, std::string::npos);
        sink_set.insert(peer_id);
        server.SendMessage(peer_id, "Three");

    } else if (msg.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        auto peer_id = msg.substr(15, std::string::npos);

        if (sink_set.find(peer_id) != sink_set.end())
            sink_set.erase(peer_id);

        if (sink_set.size() == 0 && handled_sink == expected_sink)
            g_main_loop_quit(loop);

    } else if (msg.compare(0, 13, "ROOM_PEER_MSG") == 0) {
        auto peer_msg = msg.substr(14, std::string::npos);
        std::size_t pos = peer_msg.find(' ');
        if (pos == std::string::npos)
            FAIL() << "Invalid type of peer message" << msg;

        auto peer_id = peer_msg.substr(0, pos);
        auto received_msg = peer_msg.substr(pos + 1, std::string::npos);

        if (received_msg.compare("Way") == 0) {
            server.SendMessage(peer_id, "HandShake");
            ++handled_sink;
        } else
            FAIL() << "Can't understand message" << received_msg;

    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

static void onSinkMessageThreeWay(const std::string &msg, MqttServer &server)
{
    if (msg.compare(0, 16, "ROOM_PEER_JOINED") == 0) {
        auto peer_id = msg.substr(17, std::string::npos);

        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer" << peer_id;

    } else if (msg.compare(0, 14, "ROOM_PEER_LEFT") == 0) {
        auto peer_id = msg.substr(15, std::string::npos);

        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer" << peer_id;

        server.Disconnect();

    } else if (msg.compare(0, 13, "ROOM_PEER_MSG") == 0) {
        auto peer_msg = msg.substr(14, std::string::npos);
        std::size_t pos = peer_msg.find(' ');
        if (pos == std::string::npos)
            FAIL() << "Invalid type of peer message" << msg;

        auto peer_id = peer_msg.substr(0, pos);
        auto received_msg = peer_msg.substr(pos + 1, std::string::npos);

        EXPECT_EQ(peer_id.compare(std::string(DEFAULT_WEBRTC_SRC_ID)), 0)
              << "Not expected peer " << peer_id;

        if (received_msg.compare("Three") == 0)
            server.SendMessage(peer_id, "Way");
        else if (received_msg.compare("HandShake") == 0)
            server.Disconnect();
        else
            FAIL() << "Can't understand message" << received_msg;
    } else {
        FAIL() << "Invalid type of Room message " << msg;
    }
}

TEST_F(MqttServerTest, Positive_SendMessageThreeWay_Src_Sinks1_Anytime)
{
    try {
        handled_sink = 0;
        expected_sink = 2;
        MqttServer src_server(webrtc_src_config_);

        auto join_room_on_registered_src =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(src_server));
        src_server.SetConnectionStateChangedCb(join_room_on_registered_src);

        auto on_src_message =
              std::bind(onSrcMessageThreeWay, std::placeholders::_1, std::ref(src_server), loop_);
        src_server.SetRoomMessageArrivedCb(on_src_message);
        src_server.Connect();

        MqttServer first_sink_server(webrtc_first_sink_config_);

        auto join_room_on_registered_first_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(first_sink_server));
        first_sink_server.SetConnectionStateChangedCb(join_room_on_registered_first_sink);

        auto on_first_sink_message =
              std::bind(onSinkMessageThreeWay, std::placeholders::_1, std::ref(first_sink_server));
        first_sink_server.SetRoomMessageArrivedCb(on_first_sink_message);
        first_sink_server.Connect();

        MqttServer second_sink_server(webrtc_second_sink_config_);

        auto join_room_on_registered_second_sink =
              std::bind(joinRoomOnRegistered, std::placeholders::_1, std::ref(second_sink_server));
        second_sink_server.SetConnectionStateChangedCb(join_room_on_registered_second_sink);

        auto on_second_sink_message =
              std::bind(onSinkMessageThreeWay, std::placeholders::_1, std::ref(second_sink_server));
        second_sink_server.SetRoomMessageArrivedCb(on_second_sink_message);

        second_sink_server.Connect();

        g_main_loop_run(loop_);

        src_server.UnsetConnectionStateChangedCb();
        first_sink_server.UnsetConnectionStateChangedCb();
        second_sink_server.UnsetConnectionStateChangedCb();
        src_server.Disconnect();
    } catch (...) {
        FAIL() << "Expected No throw";
    }
}
