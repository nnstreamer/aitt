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

#include "WebRtcMessage.h"
#include "WebRtcStream.h"
#include "aitt_internal.h"

#define TEST_TOPIC "TEST_TOPIC"

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

    EXPECT_EQ(0, discovery_info.topic.compare(TEST_TOPIC));
    EXPECT_EQ(true, discovery_info.is_src);
    EXPECT_EQ(0, discovery_info.sdp.compare(local_description_));
    for (const auto ice_candidate : ice_candidates) {
        bool is_ice_candidate_exists{false};

        for (const auto &discovered_ice_candidate : discovery_info.ice_candidates) {
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
