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
#include "RTSPClient.h"

#include "aitt_internal.h"

RTSPClient::RTSPClient() : pipeline(nullptr), state(0)
{
}

RTSPClient::~RTSPClient()
{
}

void RTSPClient::OnPadAddedCB(GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;

    GstPadLinkReturn ret;

    GstElement *decoder = (GstElement *)data;

    /* We can now link this pad with the rtsp-decoder sink pad */

    DBG("Dynamic pad created, linking source/demuxer");
    DBG("element name: [%s], pad name : [%s]", GST_ELEMENT_NAME(element), GST_PAD_NAME(pad));

    sinkpad = gst_element_get_static_pad(decoder, "sink");

    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked(sinkpad)) {
        DBG("*** We are already linked ***");
        gst_object_unref(sinkpad);
        return;
    } else {
        DBG("proceeding to linking ...");
    }

    ret = gst_pad_link(pad, sinkpad);

    if (GST_PAD_LINK_FAILED(ret)) {
        ERR("failed to link dynamically");
    } else {
        DBG("dynamically link successful");
    }

    gst_object_unref(sinkpad);
}

void RTSPClient::VideoStreamDecodedCB(GstElement *object, GstBuffer *buffer, GstPad *pad,
      gpointer data)
{
    if (data == nullptr)
        return;

    RTSPClient *client = static_cast<RTSPClient *>(data);
    RTSPFrame frame(buffer, pad);

    /* need queueing and delete old frame */
    std::lock_guard<std::mutex> auto_lock(client->data_cb_lock);
    if (client->data_cb.first != nullptr)
        client->data_cb.first(frame, client->data_cb.second);
}

gboolean RTSPClient::MessageReceived(GstBus *bus, GstMessage *message, gpointer data)
{
    GstMessageType type = message->type;

    switch (type) {
    case GST_MESSAGE_ERROR:
        GError *gerror;
        gchar *debug;

        gst_message_parse_error(message, &gerror, &debug);

        ERR("Error : %s", debug);

        g_error_free(gerror);
        g_free(debug);

        break;
    default:
        break;
    }

    return TRUE;
}

void RTSPClient::CreatePipeline(const std::string &url)
{
    if (url.empty() == true) {
        ERR("RTSP Server url is empty");
        return;
    }

    if (pipeline != nullptr) {
        ERR("pipeline already exists");
        return;
    }

    DBG("Create Pipeline with url : %s", url.c_str());

    GstBus *bus;
    GstElement *rtspsrc;
    GstElement *rtph264depay;
    GstElement *h264parse;
    GstElement *avdec_h264;
    GstElement *videoqueue0;
    GstElement *videoconvert;
    GstElement *video_sink;

    gst_init(nullptr, nullptr);

    pipeline = gst_pipeline_new("rtsp pipeline");
    if (!pipeline) {
        ERR("pipeline is null");
        return;
    }

    rtph264depay = gst_element_factory_make("rtph264depay", "rtph264depay0");
    h264parse = gst_element_factory_make("h264parse", "h264parse0");
    rtspsrc = gst_element_factory_make("rtspsrc", "rtspsrc0");
    avdec_h264 = gst_element_factory_make("decodebin", "avdec_h2640");
    videoqueue0 = gst_element_factory_make("queue", "videoqueue0");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert0");
    video_sink = gst_element_factory_make("fakesink", "fakesink0");

    g_object_set(G_OBJECT(video_sink), "sync", FALSE, NULL);
    g_object_set(G_OBJECT(rtspsrc), "location", url.c_str(), NULL);
    g_object_set(G_OBJECT(rtspsrc), "latency", 0, NULL);
    g_object_set(G_OBJECT(rtspsrc), "buffer-mode", 3, NULL);

    gst_bin_add_many(GST_BIN(pipeline), rtspsrc, rtph264depay, h264parse, avdec_h264, videoqueue0,
          videoconvert, video_sink, NULL);

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, MessageReceived, pipeline);

    if (!gst_element_link_many(rtph264depay, h264parse, avdec_h264, NULL)) {
        ERR("Linking part (A)-1 Fail...");
        return;
    }

    if (!gst_element_link_many(videoqueue0, videoconvert, video_sink, NULL)) {
        ERR("Linking part (A)-2 Fail...");
        return;
    }

    if (!g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(OnPadAddedCB), rtph264depay)) {
        ERR("Linking part (1) with part (A)-1 Fail...");
    }

    if (!g_signal_connect(avdec_h264, "pad-added", G_CALLBACK(OnPadAddedCB), videoqueue0)) {
        ERR("Linking part (2) with part (A)-2 Fail...");
    }

    g_object_set(G_OBJECT(video_sink), "signal-handoffs", TRUE, NULL);
    if (!g_signal_connect(video_sink, "handoff", G_CALLBACK(VideoStreamDecodedCB), this)) {
        ERR("Linking part (2) with part (B)-2 Fail...");
    }

    DBG("Pipeline created");
}

void RTSPClient::DestroyPipeline(void)
{
    DBG("Destroy Pipeline");
}

void RTSPClient::SetStateCallback(const StateCallback &cb, void *user_data)
{
    std::lock_guard<std::mutex> auto_lock(state_cb_lock);

    state_cb = std::make_pair(cb, user_data);
}

void RTSPClient::SetDataCallback(const DataCallback &cb, void *user_data)
{
    std::lock_guard<std::mutex> auto_lock(data_cb_lock);

    data_cb = std::make_pair(cb, user_data);
}

void RTSPClient::UnsetStateCallback()
{
    std::lock_guard<std::mutex> auto_lock(state_cb_lock);

    state_cb = std::make_pair(nullptr, nullptr);
}

void RTSPClient::UnsetClientCallback()
{
    std::lock_guard<std::mutex> auto_lock(data_cb_lock);

    data_cb = std::make_pair(nullptr, nullptr);
}

int RTSPClient::GetState()
{
    return state;
}

void RTSPClient::Start()
{
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void RTSPClient::Stop()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
}
