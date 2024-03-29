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

#include "Module.h"

#include <flatbuffers/flexbuffers.h>

#include "aitt_internal.h"
#include "AittException.h"

#define RTSP_DBG(fmt, ...)                                       \
    do {                                                         \
        if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) \
            DBG("[RTSP_SERVER] " fmt, ##__VA_ARGS__);            \
        else                                                     \
            DBG("[RTSP_CLIENT] " fmt, ##__VA_ARGS__);            \
    } while (0)

#define RTSP_ERR(fmt, ...)                                       \
    do {                                                         \
        if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) \
            ERR("[RTSP_SERVER] " fmt, ##__VA_ARGS__);            \
        else                                                     \
            ERR("[RTSP_CLIENT] " fmt, ##__VA_ARGS__);            \
    } while (0)

namespace AittRTSPNamespace {

Module::Module(AittDiscovery &discovery, const std::string &topic, AittStreamRole role)
      : discovery_(discovery),
        topic_(topic),
        role_(role),
        server_state(AittStreamState::AITT_STREAM_STATE_INIT),
        client_state(AittStreamState::AITT_STREAM_STATE_INIT)
{
    RTSP_DBG("RTSP Module constructor : %s", topic_.c_str());

    client.SetReceiveCallback(
          [&](void *obj, void *user_data) {
              if (receive_cb.first != nullptr) {
                  receive_cb.first(this, obj, receive_cb.second);
              }
          },
          nullptr);

    client.SetStateCallback(
          [&](int state, void *user_data) {
              UpdateState(role_, static_cast<AittStreamState>(state));
          },
          nullptr);

    discovery_cb_ = discovery_.AddDiscoveryCB(topic,
          std::bind(&Module::DiscoveryMessageCallback, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

Module::~Module(void)
{
    RTSP_DBG("RTSP Module destroyer : %s", topic_.c_str());

    client.UnsetReceiveCallback();
    client.UnsetStateCallback();
    discovery_.RemoveDiscoveryCB(discovery_cb_);
}

AittStream *Module::SetConfig(const std::string &key, const std::string &value)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) {
        if (key == "URI") {
            if (value.find("rtsp://") != 0) {
                RTSP_ERR("rtsp uri validation check failed");
                throw aitt::AittException(aitt::AittException::INVALID_ARG);
            }

            info.SetURI(value);
        } else if (key == "ID") {
            info.SetID(value);
        } else if (key == "Password") {
            info.SetPassword(value);
        }
    } else {
        if (key == "FPS") {
            int fps;
            try {
                fps = std::stoi(value);
            } catch (std::exception &e) {
                RTSP_ERR("An exception(%s) occurs during SetFPS().", e.what());
                throw aitt::AittException(aitt::AittException::INVALID_ARG);
            }
            client.SetFPS(fps);
        }
    }
    return this;
}

AittStream *Module::SetConfig(const std::string &key, void *obj)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER) {
        if (key == "display") {
            RTSP_DBG("Set Evas object for display");
            client.SetDisplay(obj);
        }
    }

    return this;
}

std::string Module::GetFormat(void)
{
    return std::string();
}

int Module::GetWidth(void)
{
    return 0;
}

int Module::GetHeight(void)
{
    return 0;
}

void Module::Start(void)
{
    RTSP_DBG("Start");

    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) {
        UpdateState(role_, AittStreamState::AITT_STREAM_STATE_READY);
        UpdateDiscoveryMsg();
    } else {
        if (server_state == AittStreamState::AITT_STREAM_STATE_READY) {
            RTSP_DBG("Start in Start() method");
            client.Start();
            UpdateState(role_, AittStreamState::AITT_STREAM_STATE_PLAYING);
        } else {
            RTSP_DBG("The pipeline not yet created. Wait..");
            UpdateState(role_, AittStreamState::AITT_STREAM_STATE_READY);
        }
    }
}

void Module::Stop(void)
{
    RTSP_DBG("Stop");

    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER) {
        UpdateState(role_, AittStreamState::AITT_STREAM_STATE_INIT);
        UpdateDiscoveryMsg();
    } else {
        if (client_state == AittStreamState::AITT_STREAM_STATE_PLAYING) {
            RTSP_DBG("Stop in Start() method");
            client.Stop();
        }

        UpdateState(role_, AittStreamState::AITT_STREAM_STATE_INIT);
    }
}

int Module::Push(void *obj)
{
    return AITT_ERROR_NOT_SUPPORTED;
}

void Module::UpdateDiscoveryMsg()
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER)
        return;

    flexbuffers::Builder fbb;
    fbb.Map([this, &fbb]() {
        fbb.Int(RTSP_INFO_SERVER_STATE, static_cast<int>(server_state));
        fbb.String(RTSP_INFO_URI, info.GetURI());
        fbb.String(RTSP_INFO_ID, info.GetID());
        fbb.String(RTSP_INFO_PASSWORD, info.GetPassword());
    });
    fbb.Finish();

    auto buf = fbb.GetBuffer();
    discovery_.UpdateDiscoveryMsg(topic_, buf.data(), buf.size());
}

void Module::UpdateState(AittStreamRole role, AittStreamState state)
{
    if (role == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        server_state = state;
    else
        client_state = state;

    if (role == role_ && state_cb.first != nullptr) {
        state_cb.first(this, state, state_cb.second);
    }
}

void Module::DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
      const void *msg, const int szmsg)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        return;

    RTSP_DBG("DiscoveryMessageCallback called");

    if (!status.compare(AittDiscovery::WILL_LEAVE_NETWORK)) {
        AittStreamState next_state = AittStreamState::AITT_STREAM_STATE_INIT;

        if (client_state == AittStreamState::AITT_STREAM_STATE_PLAYING) {
            client.Stop();
            next_state = AittStreamState::AITT_STREAM_STATE_READY;
        }

        UpdateState(role_, next_state);
        return;
    }

    // ToDo : Update discovery callback to accommodate new parameters (streamHeight, streamWidth) in
    // discovery message
    auto map = flexbuffers::GetRoot(static_cast<const uint8_t *>(msg), szmsg).AsMap();
    if (map.size() != 4) {
        RTSP_ERR("RTSP Info validation check failed");
        return;
    }

    UpdateState(AittStreamRole::AITT_STREAM_ROLE_PUBLISHER,
          static_cast<AittStreamState>(map[RTSP_INFO_SERVER_STATE].AsInt64()));
    info.SetURI(map[RTSP_INFO_URI].AsString().c_str());
    info.SetID(map[RTSP_INFO_ID].AsString().c_str());
    info.SetPassword(map[RTSP_INFO_PASSWORD].AsString().c_str());

    RTSP_DBG("server_state : %d, uri : %s, id : %s, passwd : %s", server_state,
          info.GetURI().c_str(), info.GetID().c_str(), info.GetPassword().c_str());

    client.SetURI(info.GetCompleteURI());

    if (server_state == AittStreamState::AITT_STREAM_STATE_READY) {
        if (client_state == AittStreamState::AITT_STREAM_STATE_READY) {
            RTSP_DBG("Start in DiscoveryMessage Callback");
            client.Start();
            UpdateState(AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER,
                  AittStreamState::AITT_STREAM_STATE_PLAYING);
        }
    } else if (server_state == AittStreamState::AITT_STREAM_STATE_INIT) {
        if (client_state == AittStreamState::AITT_STREAM_STATE_PLAYING) {
            RTSP_DBG("Stop in DiscoveryMessage Callback");
            client.Stop();
            UpdateState(AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER,
                  AittStreamState::AITT_STREAM_STATE_READY);
        }
    }
}

void Module::SetStateCallback(StateCallback cb, void *user_data)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        return;

    state_cb = std::make_pair(cb, user_data);
}

void Module::SetReceiveCallback(ReceiveCallback cb, void *user_data)
{
    if (role_ == AittStreamRole::AITT_STREAM_ROLE_PUBLISHER)
        return;

    receive_cb = std::make_pair(cb, user_data);
}

}  // namespace AittRTSPNamespace
