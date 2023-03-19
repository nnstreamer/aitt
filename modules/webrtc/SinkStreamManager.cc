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
#include "SinkStreamManager.h"

#include <flatbuffers/flexbuffers.h>
#include <gst/video/video-info.h>

#include <sstream>

#include "aitt_internal.h"

namespace AittWebRTCNamespace {
SinkStreamManager::SinkStreamManager(const std::string &topic, const std::string &aitt_id,
      const std::string &thread_id)
      : StreamManager(topic + "/SINK", topic + "/SRC", aitt_id, thread_id),
        frame_received_(0),
        video_appsrc_(nullptr),
        decode_pipeline_(nullptr)
{
}

SinkStreamManager::~SinkStreamManager()
{
    if (decode_pipeline_)
        gst_object_unref(decode_pipeline_);
}

void SinkStreamManager::SetWebRtcStreamCallbacks(WebRtcStream &stream)
{
    stream.GetEventHandler().SetOnStateChangedCb(std::bind(&SinkStreamManager::OnStreamStateChanged,
          this, std::placeholders::_1, std::ref(stream)));

    stream.GetEventHandler().SetOnIceCandidateCb(
          std::bind(&SinkStreamManager::OnIceCandidate, this));

    stream.GetEventHandler().SetOnEncodedFrameCb(
          std::bind(&SinkStreamManager::OnEncodedFrame, this, std::placeholders::_1));

    stream.GetEventHandler().SetOnTrackAddedCb(
          std::bind(&SinkStreamManager::OnTrackAdded, this, std::placeholders::_1));
}

void SinkStreamManager::OnStreamStateChanged(WebRtcState::Stream state, WebRtcStream &stream)
{
    DBG("OnSinkStreamStateChanged: %s", WebRtcState::StreamToStr(state).c_str());
    if (state == WebRtcState::Stream::NEGOTIATING) {
        stream.CreateOfferAsync(std::bind(&SinkStreamManager::OnOfferCreated, this,
              std::placeholders::_1, std::ref(stream)));
    }
}

void SinkStreamManager::OnOfferCreated(std::string sdp, WebRtcStream &stream)
{
    DBG("%s", __func__);
    stream.SetLocalDescription(sdp);
}

void SinkStreamManager::OnIceCandidate(void)
{
    if (ice_candidate_added_cb_)
        ice_candidate_added_cb_();
}

void SinkStreamManager::OnEncodedFrame(media_packet_h packet)
{
    /* media packet should be freed after use */
    GstBuffer *buffer;
    if (media_packet_get_extra(packet, (void **)(&buffer)) != MEDIA_PACKET_ERROR_NONE
          || !GST_IS_BUFFER(buffer)) {
        ERR("Failed to get gst buffer");
        return;
    }

    GstFlowReturn gst_ret = GST_FLOW_OK;
    g_signal_emit_by_name(G_OBJECT(video_appsrc_), "push-buffer", buffer, &gst_ret, nullptr);
    if (gst_ret != GST_FLOW_OK)
        ERR("failed to push buffer");

    media_packet_destroy(packet);
    if (++frame_received_ % 100 == 0)
        stream_.PrintStats();
}

void SinkStreamManager::OnTrackAdded(unsigned int id)
{
    DBG("%s", __func__);
    frame_received_ = 0;
}

void SinkStreamManager::BuildDecodePipeline(void)
{
    // TODO:What do we need to care about pipeline?
    GstElement *pipeline = gst_pipeline_new("decode-pipeline");
    GstElement *src = gst_element_factory_make("appsrc", nullptr);

    // TODO:How do we can know decoder type
    GstCaps *app_src_caps = gst_caps_new_simple("video/x-vp8", nullptr, nullptr);
    g_object_set(G_OBJECT(src), "format", GST_FORMAT_TIME, "caps", app_src_caps, nullptr);
    gst_caps_unref(app_src_caps);

    GstElement *dec = gst_element_factory_make("vp8dec", nullptr);
    GstElement *convert = gst_element_factory_make("videoconvert", nullptr);
    GstElement *filter = gst_element_factory_make("capsfilter", "I420toRGBCapsfilter");
    GstCaps *filter_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", NULL);
    g_object_set(G_OBJECT(filter), "caps", filter_caps, NULL);
    gst_caps_unref(filter_caps);
    GstElement *queue_fake_sink = gst_element_factory_make("queue2", "queue_fake_sink");
    GstElement *fake_sink = gst_element_factory_make("fakesink", "user_sink");

    g_object_set(G_OBJECT(fake_sink), "signal-handoffs", true, nullptr);
    g_signal_connect(fake_sink, "handoff", G_CALLBACK(OnSignalHandOff),
          static_cast<gpointer>(this));

    gst_bin_add_many(GST_BIN(pipeline), src, dec, convert, filter, queue_fake_sink, fake_sink,
          nullptr);

    if (!gst_element_link_many(src, dec, convert, filter, queue_fake_sink, fake_sink, nullptr))
        ERR("Failed to gst_element_link_many");

    auto state_change_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_change_ret == GST_STATE_CHANGE_FAILURE)
        ERR("failed to set state to PLAYING");

    decode_pipeline_ = pipeline;
    video_appsrc_ = src;
}

void SinkStreamManager::OnSignalHandOff(GstElement *object, GstBuffer *buffer, GstPad *pad,
      void *user_data)
{
    auto manager = static_cast<SinkStreamManager *>(user_data);
    RET_IF(manager == nullptr);
    GstCaps *caps = gst_pad_get_current_caps(pad);
    RET_IF(caps == nullptr);

    GstVideoInfo vinfo;
    if (!gst_video_info_from_caps(&vinfo, caps)) {
        gst_caps_unref(caps);
        DBG("failed to gst_video_info_from_caps()");
        return;
    }
    RET_IF(vinfo.width == 0 || vinfo.height == 0 || vinfo.finfo == nullptr);
    manager->SetFormat(vinfo.finfo->name, vinfo.width, vinfo.height);
    manager->HandleFrame(buffer);
}

void SinkStreamManager::HandleFrame(GstBuffer *buffer)
{
    if (!buffer)
        return;

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    request_server_.PushBuffer(map.data, map.size);
}

void SinkStreamManager::HandleStreamState(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    DBG("%s", __func__);
    auto src_state = flexbuffers::GetRoot(message).ToString();

    if (src_state.compare("START") == 0)
        HandleStartStream(discovery_id);
    else if (src_state.compare("STOP") == 0)
        HandleRemovedClient(discovery_id);
    else
        ERR("Invalid message %s", src_state.c_str());
}

void SinkStreamManager::HandleStartStream(const std::string &discovery_id)
{
    DBG("%s", __func__);
    if (!peer_aitt_id_.empty()) {
        ERR("There's stream already");
        return;
    }

    DBG("Sink Stream Started");
    AddStream(discovery_id);
    BuildDecodePipeline();
    request_server_.Start();
}

void SinkStreamManager::AddStream(const std::string &discovery_id)
{
    SetWebRtcStreamCallbacks(stream_);
    stream_.Create(false, false);
    stream_.Start();

    std::stringstream s_stream;
    s_stream << static_cast<void *>(&stream_);

    stream_.SetStreamId(std::string(thread_id_ + s_stream.str()));
    peer_aitt_id_ = discovery_id;
}

void SinkStreamManager::HandleStreamInfo(const std::string &discovery_id,
      const std::vector<uint8_t> &message)
{
    if (!WebRtcMessage::IsValidStreamInfo(message)) {
        ERR("Invalid streams info");
        return;
    }

    // have a stream at Current status
    auto stream = flexbuffers::GetRoot(message).AsMap();
    auto id = stream["id"].AsString().str();
    auto peer_id = stream["peer_id"].AsString().str();
    auto sdp = stream["sdp"].AsString().str();
    std::vector<std::string> ice_candidates;
    auto ice_info = stream["ice_candidates"].AsVector();
    for (size_t ice_idx = 0; ice_idx < ice_info.size(); ++ice_idx)
        ice_candidates.push_back(ice_info[ice_idx].AsString().str());
    UpdateStreamInfo(discovery_id, id, peer_id, sdp, ice_candidates);
}

void SinkStreamManager::UpdateStreamInfo(const std::string &discovery_id, const std::string &id,
      const std::string &peer_id, const std::string &sdp,
      const std::vector<std::string> &ice_candidates)
{
    // There's only one stream for a aitt ID
    if (peer_aitt_id_ != discovery_id) {
        ERR("No matching stream");
        return;
    }

    if (peer_id.compare(stream_.GetStreamId()) != 0) {
        ERR("Different ID");
        return;
    }

    if (stream_.GetPeerId().size() == 0) {
        DBG("first update");
        stream_.SetPeerId(id);
        stream_.AddPeerInformation(sdp, ice_candidates);
    } else
        stream_.UpdatePeerInformation(ice_candidates);
}

std::vector<uint8_t> SinkStreamManager::GetDiscoveryMessage(void)
{
    std::vector<uint8_t> message;

    flexbuffers::Builder fbb;
    fbb.Map([&] {
        fbb.String("id", stream_.GetStreamId());
        fbb.String("peer_id", stream_.GetPeerId());
        fbb.String("sdp", stream_.GetLocalDescription());
        fbb.Vector("ice_candidates", [&]() {
            for (const auto &candidate : stream_.GetIceCandidates()) {
                fbb.String(candidate);
            }
        });
    });
    fbb.Finish();

    message = fbb.GetBuffer();
    return message;
}

void SinkStreamManager::SetOnFrameCallback(OnFrameCallback cb)
{
    request_server_.SetOnFrameCallback(cb);
}

}  // namespace AittWebRTCNamespace
