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
#include "GlibMainLoop.h"

#include <glib.h>

#include <stdexcept>

#include "aitt_internal.h"

namespace aitt {

GlibMainLoop::GlibMainLoop()
{
    GMainContext *ctx = g_main_context_new();
    if (ctx == nullptr)
        throw std::runtime_error("g_main_context_new() Fail");

    loop = g_main_loop_new(ctx, FALSE);
    if (loop == nullptr) {
        g_main_context_unref(ctx);
        throw std::runtime_error("g_main_loop_new() Fail");
    }
    g_main_context_unref(ctx);
}

GlibMainLoop::~GlibMainLoop()
{
    g_main_loop_unref(loop);
}

void GlibMainLoop::Run()
{
    g_main_loop_run(loop);
}

bool GlibMainLoop::Quit()
{
    if (g_main_loop_is_running(loop) == FALSE) {
        ERR("main loop is not running");
        return false;
    }

    g_main_loop_quit(loop);
    return true;
}

void GlibMainLoop::AddIdle(const mainLoopCB &cb, MainLoopData *user_data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    cb_data->cb = cb;
    cb_data->data = user_data;
    cb_data->ctx = g_main_loop_get_context(loop);

    GSource *source = g_idle_source_new();
    g_source_set_priority(source, G_PRIORITY_HIGH);
    g_source_set_callback(source, CallbackHandler, cb_data, DestroyNotify);
    g_source_attach(source, cb_data->ctx);
    g_source_unref(source);
}

void GlibMainLoop::AddWatch(int fd, const mainLoopCB &cb, MainLoopData *user_data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    GMainContext *ctx = g_main_loop_get_context(loop);
    cb_data->ctx = ctx;
    cb_data->cb = cb;
    cb_data->data = user_data;
    cb_data->fd = fd;

    GIOChannel *channel = g_io_channel_unix_new(fd);
    GSource *source = g_io_create_watch(channel, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR));
    g_source_set_callback(source, (GSourceFunc)EventHandler, cb_data, DestroyNotify);

    g_source_attach(source, ctx);
    g_source_unref(source);

    callback_table_lock.lock();
    callback_table.insert(CallbackMap::value_type(fd, std::make_pair(source, cb_data)));
    callback_table_lock.unlock();
}

GlibMainLoop::MainLoopData *GlibMainLoop::RemoveWatch(int fd)
{
    GSource *source;
    MainLoopData *user_data = nullptr;

    std::lock_guard<std::mutex> autoLock(callback_table_lock);
    auto it = callback_table.find(fd);
    if (it == callback_table.end())
        return user_data;
    source = it->second.first;
    user_data = it->second.second->data;
    callback_table.erase(it);

    g_source_destroy(source);
    return user_data;
}

unsigned int GlibMainLoop::AddTimeout(int interval, const mainLoopCB &cb, MainLoopData *data)
{
    MainLoopCbData *cb_data = new MainLoopCbData();
    GMainContext *ctx = g_main_loop_get_context(loop);
    cb_data->ctx = ctx;
    cb_data->cb = cb;
    cb_data->data = data;

    GSource *source = g_timeout_source_new(interval);
    g_source_set_callback(source, CallbackHandler, cb_data, DestroyNotify);
    unsigned int id = g_source_attach(source, cb_data->ctx);
    g_source_unref(source);

    return id;
}

void GlibMainLoop::RemoveTimeout(unsigned int id)
{
    GSource *source;
    source = g_main_context_find_source_by_id(g_main_loop_get_context(loop), id);
    if (source)
        g_source_destroy(source);
}

gboolean GlibMainLoop::CallbackHandler(gpointer user_data)
{
    RETV_IF(user_data == nullptr, FALSE);

    MainLoopCbData *cb_data = static_cast<MainLoopCbData *>(user_data);

    return cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);
}

gboolean GlibMainLoop::EventHandler(GIOChannel *src, GIOCondition condition, gpointer user_data)
{
    RETV_IF(user_data == nullptr, FALSE);

    int ret = TRUE;
    MainLoopCbData *cb_data = static_cast<MainLoopCbData *>(user_data);

    if ((G_IO_HUP | G_IO_ERR) & condition) {
        ERR("Connection Error(%d)", condition);
        cb_data->result = (G_IO_HUP & condition) ? Event::HANGUP : Event::ERROR;
        ret = FALSE;
    }

    ret &= cb_data->cb(cb_data->result, cb_data->fd, cb_data->data);

    return ret;
}

void GlibMainLoop::DestroyNotify(gpointer data)
{
    MainLoopCbData *cb_data = static_cast<MainLoopCbData *>(data);
    delete cb_data;
}

GlibMainLoop::MainLoopCbData::MainLoopCbData()
      : data(nullptr), result(Event::OKAY), fd(-1), ctx(nullptr)
{
}

}  // namespace aitt
