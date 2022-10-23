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

#include <AittDiscovery.h>
#include <glib.h>
#include <gtest/gtest.h>

#include "Module.h"
#include "MosquittoMQ.h"
#include "SinkStreamManager.h"
#include "SrcStreamManager.h"
#include "StreamManager.h"
#include "WebRtcMessage.h"
#include "WebRtcStream.h"
#include "aitt_internal.h"

#define LOCAL_IP "127.0.0.1"
#define DEFAULT_BROKER_PORT 1883
#define WEBRTC_TOPIC_PREFIX "WEBRTC_"
#define TEST_TOPIC "TEST_TOPIC"
#define TEST_SRC_CLIENT_ID "TEST_SRC_CLIENT_ID"
#define TEST_SINK_CLIENT_ID "TEST_SINK_CLIENT_ID"

using namespace AittWebRTCNamespace;

class WebRtcMessageTest : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(WebRtcMessageTest, test_SDP_Message_Anytime)
{
    // TODO
}

TEST_F(WebRtcMessageTest, test_ICE_Message_Anytime)
{
    // TODO
}

TEST_F(WebRtcMessageTest, test_UNKNOWN_Message_Anytime)
{
    // TODO
}

TEST_F(WebRtcMessageTest, test_OnDevice)
{
    // TODO
}
class WebRtcSrcStreamTest : public testing::Test {
  protected:
    void SetUp() override { mainLoop_ = g_main_loop_new(nullptr, FALSE); }
    void TearDown() override { g_main_loop_unref(mainLoop_); }
    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop_);
        DBG("Go forward");
    }
    static void OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream,
          WebRtcSrcStreamTest *test)
    {
        DBG("OnStreamStateChanged");
        if (state == WebRtcState::Stream::NEGOTIATING) {
            auto on_offer_created =
                  std::bind(OnOfferCreated, std::placeholders::_1, std::ref(stream), test);
            stream.CreateOfferAsync(on_offer_created);
        }
    }
    static void OnOfferCreated(std::string sdp, WebRtcStream &stream, WebRtcSrcStreamTest *test)
    {
        DBG("%s", __func__);

        test->local_description_ = sdp;
        stream.SetLocalDescription(sdp);
    }

    static void OnSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream,
          WebRtcSrcStreamTest *test)
    {
        DBG("Singaling State: %s", WebRtcState::SignalingToStr(state).c_str());
    }

    static void OnIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          WebRtcSrcStreamTest *test)
    {
        DBG("IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
        g_main_loop_quit(test->mainLoop_);
    }
    GMainLoop *mainLoop_;
    std::string local_description_;
};

TEST_F(WebRtcSrcStreamTest, test_Create_WebRtcSrcStream_OnDevice)
{
    WebRtcStream stream{};
    EXPECT_EQ(true, stream.Create(true, false)) << "Failed to create source stream";
}

TEST_F(WebRtcSrcStreamTest, test_Start_WebRtcSrcStream_OnDevice)
{
    WebRtcStream stream{};
    EXPECT_EQ(true, stream.Create(true, false)) << "Failed to create source stream";
    EXPECT_EQ(true, stream.AttachCameraSource()) << "Failed to attach camera source";
    auto on_stream_state_changed_cb =
          std::bind(OnStreamStateChanged, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_signaling_state_changed_cb =
          std::bind(OnSignalingStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnSignalingStateNotifyCb(on_signaling_state_changed_cb);

    auto on_ice_gathering_state_changed_cb =
          std::bind(OnIceGatheringStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);
    stream.Start();
    IterateEventLoop();
}

TEST_F(WebRtcSrcStreamTest, test_Validate_WebRtcSrcStream_Discovery_Message_OnDevice)
{
    WebRtcStream stream{};
    EXPECT_EQ(true, stream.Create(true, false)) << "Failed to create source stream";
    EXPECT_EQ(true, stream.AttachCameraSource()) << "Failed to attach camera source";
    auto on_stream_state_changed_cb =
          std::bind(OnStreamStateChanged, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnStateChangedCb(on_stream_state_changed_cb);

    auto on_signaling_state_changed_cb =
          std::bind(OnSignalingStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnSignalingStateNotifyCb(on_signaling_state_changed_cb);

    auto on_ice_gathering_state_changed_cb =
          std::bind(OnIceGatheringStateNotify, std::placeholders::_1, std::ref(stream), this);
    stream.GetEventHandler().SetOnIceGatheringStateNotifyCb(on_ice_gathering_state_changed_cb);
    stream.Start();
    IterateEventLoop();
    EXPECT_EQ(WebRtcMessage::Type::SDP, WebRtcMessage::getMessageType(local_description_));
    auto ice_candidates = stream.GetIceCandidates();
    for (const auto &ice_candidate : ice_candidates) {
        EXPECT_EQ(WebRtcMessage::Type::ICE, WebRtcMessage::getMessageType(ice_candidate));
    }

    auto discovery_message = WebRtcMessage::GenerateDiscoveryMessage(TEST_TOPIC, true,
          local_description_, stream.GetIceCandidates());

    auto discovery_info = WebRtcMessage::ParseDiscoveryMessage(discovery_message);

    EXPECT_EQ(0, discovery_info.topic_.compare(TEST_TOPIC));
    EXPECT_EQ(true, discovery_info.is_src_);
    EXPECT_EQ(0, discovery_info.sdp_.compare(local_description_));
    for (const auto ice_candidate : ice_candidates) {
        bool is_ice_candidate_exists{false};

        for (const auto &discovered_ice_candidate : discovery_info.ice_candidates_) {
            if (discovered_ice_candidate.compare(ice_candidate) == 0)
                is_ice_candidate_exists = true;
        }
        EXPECT_EQ(true, is_ice_candidate_exists);
    }
}

class WebRtcSourceOffererTest : public testing::Test {
  protected:
    void SetUp() override { mainLoop_ = g_main_loop_new(nullptr, FALSE); }
    void TearDown() override { g_main_loop_unref(mainLoop_); }
    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop_);
        DBG("Go forward");
    }

    static void OnSrcStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("OnSrcStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
        if (state == WebRtcState::Stream::NEGOTIATING) {
            auto on_offer_created =
                  std::bind(OnOfferCreated, std::placeholders::_1, std::ref(stream), test);
            stream.CreateOfferAsync(on_offer_created);
        }
    }

    static void OnOfferCreated(std::string sdp, WebRtcStream &stream, WebRtcSourceOffererTest *test)
    {
        DBG("%s", __func__);

        test->src_description_ = sdp;
        stream.SetLocalDescription(sdp);
    }

    static void OnSrcSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("OnSrcSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
    }

    static void OnSrcIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("Source IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
        if (state == WebRtcState::IceGathering::COMPLETE) {
            test->sink_stream_.AddDiscoveryInformation(WebRtcMessage::GenerateDiscoveryMessage(
                  "TEST_TOPIC", true, test->src_description_, stream.GetIceCandidates()));
        }
    }

    static void OnSinkStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    }

    static void OnAnswerCreated(std::string sdp, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("%s", __func__);

        test->sink_description_ = sdp;
        stream.SetLocalDescription(sdp);
    }

    static void OnSinkSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("OnSinkSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
        if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
            auto on_answer_created =
                  std::bind(OnAnswerCreated, std::placeholders::_1, std::ref(stream), test);
            stream.CreateAnswerAsync(on_answer_created);
        }
    }

    static void OnSinkIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          WebRtcSourceOffererTest *test)
    {
        DBG("Sink IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
        if (WebRtcState::IceGathering::COMPLETE == state) {
            test->src_stream_.AddDiscoveryInformation(WebRtcMessage::GenerateDiscoveryMessage(
                  "TEST_TOPIC", true, test->sink_description_, stream.GetIceCandidates()));
        }
    }

    static void OnSinkStreamEncodedFrame(WebRtcSourceOffererTest *test)
    {
        DBG("OnSinkStreamEncodedFrame");
        if (g_main_loop_is_running(test->mainLoop_))
            g_main_loop_quit(test->mainLoop_);
    }

    WebRtcStream src_stream_;
    WebRtcStream sink_stream_;
    GMainLoop *mainLoop_;
    std::string src_description_;
    std::string sink_description_;
};

TEST_F(WebRtcSourceOffererTest, test_Start_WebRtcStream_OnDevice)
{
    EXPECT_EQ(true, src_stream_.Create(true, false)) << "Failed to create source stream";
    EXPECT_EQ(true, src_stream_.AttachCameraSource()) << "Failed to attach camera source";
    auto on_src_stream_state_changed_cb =
          std::bind(OnSrcStreamStateChanged, std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnStateChangedCb(on_src_stream_state_changed_cb);

    auto on_src_signaling_state_changed_cb =
          std::bind(OnSrcSignalingStateNotify, std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnSignalingStateNotifyCb(on_src_signaling_state_changed_cb);

    auto on_src_ice_gathering_state_changed_cb = std::bind(OnSrcIceGatheringStateNotify,
          std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnIceGatheringStateNotifyCb(
          on_src_ice_gathering_state_changed_cb);
    src_stream_.Start();

    EXPECT_EQ(true, sink_stream_.Create(false, false)) << "Failed to create sink stream";
    auto on_sink_stream_state_changed_cb =
          std::bind(OnSinkStreamStateChanged, std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnStateChangedCb(on_sink_stream_state_changed_cb);

    auto on_sink_signaling_state_changed_cb = std::bind(OnSinkSignalingStateNotify,
          std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnSignalingStateNotifyCb(on_sink_signaling_state_changed_cb);

    auto on_sink_ice_gathering_state_changed_cb = std::bind(OnSinkIceGatheringStateNotify,
          std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnIceGatheringStateNotifyCb(
          on_sink_ice_gathering_state_changed_cb);

    auto on_sink_encoded_frame_cb = std::bind(OnSinkStreamEncodedFrame, this);
    sink_stream_.GetEventHandler().SetOnEncodedFrameCb(on_sink_encoded_frame_cb);
    sink_stream_.Start();
    IterateEventLoop();
}

class WebRtcSinkOffererTest : public testing::Test {
  protected:
    void SetUp() override { mainLoop_ = g_main_loop_new(nullptr, FALSE); }
    void TearDown() override { g_main_loop_unref(mainLoop_); }
    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop_);
        DBG("Go forward");
    }

    static void OnSinkStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
        if (state == WebRtcState::Stream::NEGOTIATING) {
            auto on_offer_created =
                  std::bind(OnOfferCreated, std::placeholders::_1, std::ref(stream), test);
            stream.CreateOfferAsync(on_offer_created);
        }
    }

    static void OnOfferCreated(std::string sdp, WebRtcStream &stream, WebRtcSinkOffererTest *test)
    {
        DBG("%s", __func__);

        test->sink_description_ = sdp;
        stream.SetLocalDescription(sdp);
    }

    static void OnSinkSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("OnSinkSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
    }

    static void OnSinkIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("Sink IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
        if (state == WebRtcState::IceGathering::COMPLETE) {
            test->src_stream_.AddDiscoveryInformation(WebRtcMessage::GenerateDiscoveryMessage(
                  "TEST_TOPIC", true, test->sink_description_, stream.GetIceCandidates()));
        }
    }

    static void OnSrcStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("OnSrcStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    }

    static void OnAnswerCreated(std::string sdp, WebRtcStream &stream, WebRtcSinkOffererTest *test)
    {
        DBG("%s", __func__);

        test->src_description_ = sdp;
        stream.SetLocalDescription(sdp);
    }

    static void OnSrcSignalingStateNotify(WebRtcState::Signaling state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("OnSrcSignalingStateNotify: %s", WebRtcState::SignalingToStr(state).c_str());
        if (state == WebRtcState::Signaling::HAVE_REMOTE_OFFER) {
            auto on_answer_created =
                  std::bind(OnAnswerCreated, std::placeholders::_1, std::ref(stream), test);
            stream.CreateAnswerAsync(on_answer_created);
        }
    }

    static void OnSrcIceGatheringStateNotify(WebRtcState::IceGathering state, WebRtcStream &stream,
          WebRtcSinkOffererTest *test)
    {
        DBG("Src IceGathering State: %s", WebRtcState::IceGatheringToStr(state).c_str());
        if (WebRtcState::IceGathering::COMPLETE == state) {
            test->sink_stream_.AddDiscoveryInformation(WebRtcMessage::GenerateDiscoveryMessage(
                  "TEST_TOPIC", true, test->src_description_, stream.GetIceCandidates()));
        }
    }

    static void OnSinkStreamEncodedFrame(WebRtcSinkOffererTest *test)
    {
        DBG("OnSinkStreamEncodedFrame");
        if (g_main_loop_is_running(test->mainLoop_))
            g_main_loop_quit(test->mainLoop_);
    }

    WebRtcStream src_stream_;
    WebRtcStream sink_stream_;
    GMainLoop *mainLoop_;
    std::string src_description_;
    std::string sink_description_;
};

TEST_F(WebRtcSinkOffererTest, test_Start_WebRtcStream_OnDevice)
{
    EXPECT_EQ(true, src_stream_.Create(true, false)) << "Failed to create source stream";
    EXPECT_EQ(true, src_stream_.AttachCameraSource()) << "Failed to attach camera source";
    auto on_src_stream_state_changed_cb =
          std::bind(OnSrcStreamStateChanged, std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnStateChangedCb(on_src_stream_state_changed_cb);

    auto on_src_signaling_state_changed_cb =
          std::bind(OnSrcSignalingStateNotify, std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnSignalingStateNotifyCb(on_src_signaling_state_changed_cb);

    auto on_src_ice_gathering_state_changed_cb = std::bind(OnSrcIceGatheringStateNotify,
          std::placeholders::_1, std::ref(src_stream_), this);
    src_stream_.GetEventHandler().SetOnIceGatheringStateNotifyCb(
          on_src_ice_gathering_state_changed_cb);
    src_stream_.Start();

    EXPECT_EQ(true, sink_stream_.Create(false, false)) << "Failed to create sink stream";
    auto on_sink_stream_state_changed_cb =
          std::bind(OnSinkStreamStateChanged, std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnStateChangedCb(on_sink_stream_state_changed_cb);

    auto on_sink_signaling_state_changed_cb = std::bind(OnSinkSignalingStateNotify,
          std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnSignalingStateNotifyCb(on_sink_signaling_state_changed_cb);

    auto on_sink_ice_gathering_state_changed_cb = std::bind(OnSinkIceGatheringStateNotify,
          std::placeholders::_1, std::ref(sink_stream_), this);
    sink_stream_.GetEventHandler().SetOnIceGatheringStateNotifyCb(
          on_sink_ice_gathering_state_changed_cb);

    auto on_sink_encoded_frame_cb = std::bind(OnSinkStreamEncodedFrame, this);
    sink_stream_.GetEventHandler().SetOnEncodedFrameCb(on_sink_encoded_frame_cb);
    sink_stream_.Start();
    IterateEventLoop();
}

class StreamManagerTest : public testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(StreamManagerTest, test_Create_StreamManager_Anytime)
{
    std::stringstream s_stream;
    s_stream << std::this_thread::get_id();
    std::unique_ptr<StreamManager> sink_manager(
          new SinkStreamManager(TEST_TOPIC, TEST_SINK_CLIENT_ID, s_stream.str()));
    std::unique_ptr<StreamManager> src_manager(
          new SrcStreamManager(TEST_TOPIC, TEST_SRC_CLIENT_ID, s_stream.str()));
}

class SinkStreamManagerTest : public testing::Test {
  protected:
    SinkStreamManagerTest()
          : discovery_cb_(-1),
            test_topic_(std::string(WEBRTC_TOPIC_PREFIX) + std::string(TEST_TOPIC)),
            discovery_engine_(TEST_SINK_CLIENT_ID),
            mainLoop_(nullptr) {};
    void SetUp() override
    {
        mainLoop_ = g_main_loop_new(nullptr, FALSE);
        discovery_engine_.SetMQ(std::unique_ptr<aitt::MQ>(
              new aitt::MosquittoMQ(std::string(TEST_SINK_CLIENT_ID) + 'd', false)));
        discovery_engine_.Start(LOCAL_IP, DEFAULT_BROKER_PORT, std::string(), std::string());
    }

    void TearDown() override
    {
        discovery_engine_.SetMQ(nullptr);
        g_main_loop_unref(mainLoop_);
    }
    void IterateEventLoop(void)
    {
        g_main_loop_run(mainLoop_);
        DBG("Go forward");
    }

    static void OnDiscovered(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg, SinkStreamManagerTest *test)
    {
        DBG("OnDiscovered");
        if (g_main_loop_is_running(test->mainLoop_))
            g_main_loop_quit(test->mainLoop_);
    }

    static void OnStreamReady(WebRtcStream &stream, SinkStreamManagerTest *test)
    {
        DBG("OnStreamReady");
        auto discovery_message = WebRtcMessage::GenerateDiscoveryMessage(test->test_topic_, false,
              stream.GetLocalDescription(), stream.GetIceCandidates());
        test->discovery_engine_.UpdateDiscoveryMsg(test->test_topic_, discovery_message.data(),
              discovery_message.size());
    }

    int discovery_cb_;
    std::string test_topic_;
    aitt::AittDiscovery discovery_engine_;
    GMainLoop *mainLoop_;
};

TEST_F(SinkStreamManagerTest, test_SinkStreamManager_Start_OnDevice)
{
    std::stringstream s_stream;
    s_stream << std::this_thread::get_id();
    std::unique_ptr<StreamManager> sink_manager(
          new SinkStreamManager(test_topic_, TEST_SINK_CLIENT_ID, s_stream.str()));

    auto on_discovered = std::bind(OnDiscovered, std::placeholders::_1, std::placeholders::_2,
          std::placeholders::_3, std::placeholders::_4, this);

    discovery_cb_ = discovery_engine_.AddDiscoveryCB(test_topic_, on_discovered);

    auto on_stream_ready = std::bind(OnStreamReady, std::placeholders::_1, this);
    sink_manager->SetStreamReadyCallback(on_stream_ready);

    sink_manager->Start();
    IterateEventLoop();
    discovery_engine_.RemoveDiscoveryCB(discovery_cb_);
}

class SinkSrcStreamManagerTest : public testing::Test {
  protected:
    SinkSrcStreamManagerTest()
          : start_sink_id_(0),
            start_src_id_(0),
            stop_sink_id_(0),
            stop_src_id_(0),
            stop_sink_first_(true),
            sink_discovery_cb_(-1),
            src_discovery_cb_(-1),
            test_topic_(std::string(WEBRTC_TOPIC_PREFIX) + std::string(TEST_TOPIC)),
            sink_discovery_engine_(std::string(TEST_SINK_CLIENT_ID)),
            src_discovery_engine_(std::string(TEST_SRC_CLIENT_ID)),
            mainLoop_(nullptr)
    {
        std::stringstream s_stream;
        s_stream << std::this_thread::get_id();
        // Construct stream manager
        sink_manager_ = std::unique_ptr<StreamManager>(
              new SinkStreamManager(test_topic_, TEST_SINK_CLIENT_ID, s_stream.str()));
        src_manager_ = std::unique_ptr<StreamManager>(
              new SrcStreamManager(test_topic_, TEST_SRC_CLIENT_ID, s_stream.str()));
    };

    void SetUp() override
    {
        // create g_main_loop & connect broker
        mainLoop_ = g_main_loop_new(nullptr, FALSE);

        sink_discovery_engine_.SetMQ(std::unique_ptr<aitt::MQ>(
              new aitt::MosquittoMQ(std::string(TEST_SINK_CLIENT_ID) + 'd', false)));
        sink_discovery_engine_.Start(LOCAL_IP, DEFAULT_BROKER_PORT, std::string(), std::string());

        auto discovered_at_sink = std::bind(DiscoveredAtSink, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this);
        sink_discovery_cb_ = sink_discovery_engine_.AddDiscoveryCB(test_topic_, discovered_at_sink);
        sink_discovery_engine_.Restart();

        src_discovery_engine_.SetMQ(std::unique_ptr<aitt::MQ>(
              new aitt::MosquittoMQ(std::string(TEST_SRC_CLIENT_ID) + 'd', false)));
        src_discovery_engine_.Start(LOCAL_IP, DEFAULT_BROKER_PORT, std::string(), std::string());

        auto discovered_at_src = std::bind(DiscoveredAtSrc, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, this);
        src_discovery_cb_ = src_discovery_engine_.AddDiscoveryCB(test_topic_, discovered_at_src);
        src_discovery_engine_.Restart();
    }

    void TearDown() override
    {
        // disconnect broker & destroy g_main_loop

        if (sink_discovery_engine_.HasValidMQ()) {
            sink_discovery_engine_.RemoveDiscoveryCB(sink_discovery_cb_);
            sink_discovery_engine_.Stop();
            sink_discovery_engine_.SetMQ(nullptr);
        }

        if (src_discovery_engine_.HasValidMQ()) {
            src_discovery_engine_.RemoveDiscoveryCB(src_discovery_cb_);
            src_discovery_engine_.Stop();
            src_discovery_engine_.SetMQ(nullptr);
        }

        sink_manager_->Stop();
        src_manager_->Stop();

        g_main_loop_unref(mainLoop_);
    }

    void IterateEventLoop(void)
    {
        DBG("Go forward");
        g_main_loop_run(mainLoop_);
    }

    void AddStopEventLoop(void) { g_timeout_add_seconds(10, GSourceStopEventLoop, this); }

    static gboolean GSourceStopEventLoop(gpointer data)
    {
        DBG("GSourceStopEventLoop");
        auto test = static_cast<SinkSrcStreamManagerTest *>(data);
        if (!test)
            return FALSE;

        if (g_main_loop_is_running(test->mainLoop_))
            g_main_loop_quit(test->mainLoop_);

        return FALSE;
    }

    void StartSinkStream()
    {
        auto on_sink_stream_ready = std::bind(OnSinkStreamReady, std::placeholders::_1, this);
        sink_manager_->SetStreamReadyCallback(on_sink_stream_ready);

        auto on_encoded_frame = std::bind(OnEncodedFrame, std::placeholders::_1, this);
        static_cast<SinkStreamManager *>(sink_manager_.get())
              ->SetOnEncodedFrameCallback(on_encoded_frame);

        sink_manager_->Start();
    }

    void StartSrcStream()
    {
        auto on_src_stream_ready = std::bind(OnSrcStreamReady, std::placeholders::_1, this);
        src_manager_->SetStreamReadyCallback(on_src_stream_ready);

        src_manager_->Start();
    }

    static void DiscoveredAtSink(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg, SinkSrcStreamManagerTest *test)
    {
        // TODO: Rearrange below
        DBG("DiscoveredAtSink");
        if (!clientId.compare(test->sink_manager_->GetClientId())) {
            DBG("but has same ID");
            return;
        }

        auto sink_manager = static_cast<SinkStreamManager *>(test->sink_manager_.get());
        if (!status.compare(aitt::AittDiscovery::WILL_LEAVE_NETWORK)) {
            sink_manager->HandleRemovedClient(clientId);
            return;
        }

        sink_manager->HandleDiscoveredStream(clientId,
              std::vector<uint8_t>(static_cast<const uint8_t *>(msg),
                    static_cast<const uint8_t *>(msg) + szmsg));
    }

    static void DiscoveredAtSrc(const std::string &clientId, const std::string &status,
          const void *msg, const int szmsg, SinkSrcStreamManagerTest *test)
    {
        // TODO: Rearrange below
        DBG("DiscoveredAtSrc");
        if (!clientId.compare(test->src_manager_->GetClientId())) {
            DBG("but has same ID");
            return;
        }

        auto src_manager = static_cast<SrcStreamManager *>(test->src_manager_.get());

        if (!status.compare(aitt::AittDiscovery::WILL_LEAVE_NETWORK)) {
            src_manager->HandleRemovedClient(clientId);
            return;
        }

        src_manager->HandleDiscoveredStream(clientId,
              std::vector<uint8_t>(static_cast<const uint8_t *>(msg),
                    static_cast<const uint8_t *>(msg) + szmsg));
    }

    static void OnSinkStreamReady(WebRtcStream &stream, SinkSrcStreamManagerTest *test)
    {
        DBG("OnSinkStreamReady");

        auto discovery_message = WebRtcMessage::GenerateDiscoveryMessage(test->test_topic_, false,
              stream.GetLocalDescription(), stream.GetIceCandidates());
        test->sink_discovery_engine_.UpdateDiscoveryMsg(test->test_topic_, discovery_message.data(),
              discovery_message.size());
    }

    static void OnSrcStreamReady(WebRtcStream &stream, SinkSrcStreamManagerTest *test)
    {
        DBG("OnSrcStreamReady");

        auto discovery_message = WebRtcMessage::GenerateDiscoveryMessage(test->test_topic_, true,
              stream.GetLocalDescription(), stream.GetIceCandidates());
        test->src_discovery_engine_.UpdateDiscoveryMsg(test->test_topic_, discovery_message.data(),
              discovery_message.size());
    }

    static void OnEncodedFrame(WebRtcStream &stream, SinkSrcStreamManagerTest *test)
    {
        if (test->stop_sink_first_)
            test->AddIdleStopSinkStream();
        else
            test->AddIdleStopSrcStream();
    }

    void StartSinkFirst(void)
    {
        start_sink_id_ = g_timeout_add_seconds(1, GSourceStartSink, this);
        start_src_id_ = g_timeout_add_seconds(4, GSourceStartSrc, this);
    }

    void StartSrcFirst(void)
    {
        start_sink_id_ = g_timeout_add_seconds(4, GSourceStartSink, this);
        start_src_id_ = g_timeout_add_seconds(1, GSourceStartSrc, this);
    }

    static gboolean GSourceStartSink(gpointer data)
    {
        auto test = static_cast<SinkSrcStreamManagerTest *>(data);
        if (!test)
            return FALSE;
        test->StartSinkStream();

        return FALSE;
    }

    static gboolean GSourceStartSrc(gpointer data)
    {
        auto test = static_cast<SinkSrcStreamManagerTest *>(data);
        if (!test)
            return FALSE;

        test->StartSrcStream();

        return FALSE;
    }

    void AddIdleStopSinkStream(void)
    {
        if (!stop_sink_id_)
            stop_sink_id_ = g_idle_add(GSourceStopSinkStream, this);
    }

    static gboolean GSourceStopSinkStream(gpointer data)
    {
        auto test = static_cast<SinkSrcStreamManagerTest *>(data);
        if (!test)
            return FALSE;

        static_cast<SinkStreamManager *>(test->sink_manager_.get())->Stop();
        if (test->sink_discovery_engine_.HasValidMQ()) {
            test->sink_discovery_engine_.RemoveDiscoveryCB(test->sink_discovery_cb_);
            test->sink_discovery_engine_.Stop();
            test->sink_discovery_engine_.SetMQ(nullptr);
        }

        return FALSE;
    }

    void AddIdleStopSrcStream(void)
    {
        if (!stop_src_id_)
            g_idle_add(GSourceStopSrcStream, this);
    }

    static gboolean GSourceStopSrcStream(gpointer data)
    {
        auto test = static_cast<SinkSrcStreamManagerTest *>(data);
        if (!test)
            return FALSE;

        static_cast<SrcStreamManager *>(test->src_manager_.get())->Stop();
        if (test->src_discovery_engine_.HasValidMQ()) {
            test->src_discovery_engine_.RemoveDiscoveryCB(test->src_discovery_cb_);
            test->src_discovery_engine_.Stop();
            test->src_discovery_engine_.SetMQ(nullptr);
        }

        return FALSE;
    }

    guint start_sink_id_;
    guint start_src_id_;
    guint stop_sink_id_;
    guint stop_src_id_;
    bool stop_sink_first_;
    int sink_discovery_cb_;
    int src_discovery_cb_;
    std::string test_topic_;
    std::unique_ptr<StreamManager> sink_manager_;
    std::unique_ptr<StreamManager> src_manager_;
    aitt::AittDiscovery sink_discovery_engine_;
    aitt::AittDiscovery src_discovery_engine_;
    // TODO does this needs to be here?
    GMainLoop *mainLoop_;
};

// TEST METHOD
//  Sink_Sink: Sink appear -> Source appear -> Sink disappear -> Source disappear
//  Sink_Src : Sink appear -> Source appear -> Source disappear -> Sink disappear
//  Src_Sink : Source appear -> Sink appear -> Sink disappear -> Source disappear
//  Src_Src  : Source appear -> Sink appear -> Source disappear -> Sink disappear

TEST_F(SinkSrcStreamManagerTest, test_Sink_Sink_OnDevice)
{
    stop_sink_first_ = true;
    StartSinkFirst();
    AddStopEventLoop();
    IterateEventLoop();
}

TEST_F(SinkSrcStreamManagerTest, test_Sink_Src_OnDevice)
{
    stop_sink_first_ = false;
    StartSinkFirst();
    AddStopEventLoop();
    IterateEventLoop();
}

TEST_F(SinkSrcStreamManagerTest, test_Src_Sink_OnDevice)
{
    stop_sink_first_ = true;
    StartSrcFirst();
    AddStopEventLoop();
    IterateEventLoop();
}

TEST_F(SinkSrcStreamManagerTest, test_Src_Src_OnDevice)
{
    stop_sink_first_ = false;
    StartSrcFirst();
    AddStopEventLoop();
    IterateEventLoop();
}

class ModuleTest : public testing::Test {
  protected:
    ModuleTest()
          : test_topic_(std::string(WEBRTC_TOPIC_PREFIX) + std::string(TEST_TOPIC)),
            sink_discovery_engine_(std::string(TEST_SINK_CLIENT_ID)),
            src_discovery_engine_(std::string(TEST_SRC_CLIENT_ID)),
            sink_module_(nullptr),
            src_module_(nullptr),
            mainLoop_(nullptr){};

    void SetUp() override
    {
        // create g_main_loop & connect broker
        mainLoop_ = g_main_loop_new(nullptr, FALSE);

        sink_discovery_engine_.SetMQ(std::unique_ptr<aitt::MQ>(
              new aitt::MosquittoMQ(std::string(TEST_SINK_CLIENT_ID) + 'd', false)));
        sink_discovery_engine_.Start(LOCAL_IP, DEFAULT_BROKER_PORT, std::string(), std::string());
        sink_discovery_engine_.Restart();
        sink_module_ = std::unique_ptr<Module>(new Module(sink_discovery_engine_, test_topic_,
              AittStreamRole::AITT_STREAM_ROLE_SUBSCRIBER));

        src_discovery_engine_.SetMQ(std::unique_ptr<aitt::MQ>(
              new aitt::MosquittoMQ(std::string(TEST_SRC_CLIENT_ID) + 'd', false)));
        src_discovery_engine_.Start(LOCAL_IP, DEFAULT_BROKER_PORT, std::string(), std::string());
        src_discovery_engine_.Restart();
        src_module_ = std::unique_ptr<Module>(new Module(src_discovery_engine_, test_topic_,
              AittStreamRole::AITT_STREAM_ROLE_PUBLISHER));
    }

    void TearDown() override
    {
        // disconnect broker & destroy g_main_loop

        if (sink_discovery_engine_.HasValidMQ()) {
            sink_discovery_engine_.Stop();
            sink_discovery_engine_.SetMQ(nullptr);
        }

        if (src_discovery_engine_.HasValidMQ()) {
            src_discovery_engine_.Stop();
            src_discovery_engine_.SetMQ(nullptr);
        }

        g_main_loop_unref(mainLoop_);
    }

    void IterateEventLoop(void)
    {
        DBG("Go forward");
        g_main_loop_run(mainLoop_);
    }

    void AddStopEventLoop(void) { g_timeout_add_seconds(15, GSourceStopEventLoop, this); }

    static gboolean GSourceStopEventLoop(gpointer data)
    {
        DBG("GSourceStopEventLoop");
        auto test = static_cast<ModuleTest *>(data);
        if (!test)
            return FALSE;

        if (g_main_loop_is_running(test->mainLoop_))
            g_main_loop_quit(test->mainLoop_);

        return FALSE;
    }

    std::string test_topic_;
    aitt::AittDiscovery sink_discovery_engine_;
    aitt::AittDiscovery src_discovery_engine_;
    std::unique_ptr<Module> sink_module_;
    std::unique_ptr<Module> src_module_;
    GMainLoop *mainLoop_;
};

TEST_F(ModuleTest, test_OnDevice)
{
    sink_module_->Start();
    src_module_->Start();
    AddStopEventLoop();
    IterateEventLoop();
}
