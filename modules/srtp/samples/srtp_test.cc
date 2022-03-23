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
#include <SRTP.h>
#include <glib.h>
#include <log.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

#define SEND_STRING "hello"
#define SEND_INTERVAL 1000

__thread __aitt__tls__ __aitt;

class AittSrtpSample {
  public:
    explicit AittSrtpSample(const std::vector<uint8_t> &key, std::unique_ptr<aitt::UDP> udpClient,
          std::unique_ptr<aitt::UDP> udpServer)
          : client(std::make_unique<aitt::SRTP>(std::move(udpClient), key)),
            server(std::make_unique<aitt::SRTP>(std::move(udpServer), key))
    {
    }
    virtual ~AittSrtpSample(void) {}

    std::unique_ptr<aitt::SRTP> client;
    std::unique_ptr<aitt::SRTP> server;
};

int main(int argc, char *argv[])
{
    GMainLoop *mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        ERR("Failed to create a main loop");
        return 1;
    }

    struct EventData {
        GSource source_;
        GPollFD fd;
        AittSrtpSample *sample;
        unsigned short port;
    };

    GSourceFuncs srcs = {
          [](GSource *src, gint *timeout) -> gboolean {
              *timeout = 1;
              return FALSE;
          },
          [](GSource *src) -> gboolean {
              EventData *ed = reinterpret_cast<EventData *>(src);

              if ((ed->fd.revents & G_IO_IN) == G_IO_IN)
                  return TRUE;
              if ((ed->fd.revents & G_IO_ERR) == G_IO_ERR)
                  return TRUE;

              return FALSE;
          },
          [](GSource *src, GSourceFunc callback, gpointer user_data) -> gboolean {
              EventData *ed = reinterpret_cast<EventData *>(src);

              if ((ed->fd.revents & G_IO_ERR) == G_IO_ERR) {
                  ERR("Error!");
                  return FALSE;
              }

              char buffer[10];
              void *ptr = static_cast<void *>(buffer);
              size_t szData = sizeof(SEND_STRING);
              ed->sample->server->Recv(ptr, szData);
              std::cout << "Gots [" + std::string(buffer) + "]" << std::endl;

              return TRUE;
          },
          nullptr,
    };

    GSource *src = g_source_new(&srcs, sizeof(EventData));
    if (!src) {
        g_main_loop_unref(mainLoop);
        ERR("g_source_new failed");
        return 1;
    }

    EventData *ed = reinterpret_cast<EventData *>(src);

    ed->port = 1234;
    std::unique_ptr<aitt::UDP> udpClient(std::make_unique<aitt::UDP>());
    std::unique_ptr<aitt::UDP> udpServer(std::make_unique<aitt::UDP>("0.0.0.0", ed->port));

    try {
        uint8_t key[] = {
              0x73, 0x57, 0x9e, 0x73, 0x7e, 0xf5, 0xed, 0xd6, 0xbb, 0xeb, 0x5f, 0x79, 0x6d, 0xbf,
              0x3b, 0xf3, 0x9e, 0xfc, 0xef, 0xdd, 0x1a, 0x7f, 0xbd, 0x5c, 0xe1, 0xe7, 0xbd, 0x7f,
              0xce, 0x7d, 0x7b, 0x5f, 0x7b, 0x6b, 0x8d, 0x78, 0x6b, 0xbf, 0x1d, 0xe5, 0xa6, 0xdc,
              0xef,
              0x8e,  // 0x75,
        };

        ed->sample = new AittSrtpSample(std::vector<uint8_t>(key, key + sizeof(key)), std::move(udpClient),
              std::move(udpServer));
    } catch (std::exception &e) {
        ERR("new: %s", e.what());
        g_source_unref(src);
        g_main_loop_unref(mainLoop);
        return 1;
    }

    ed->fd.fd = ed->sample->server->GetHandle();
    ed->fd.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll(src, &ed->fd);
    guint id = g_source_attach(src, g_main_loop_get_context(mainLoop));
    g_source_unref(src);
    if (id == 0) {
        delete ed->sample;
        g_source_destroy(src);
        g_main_loop_unref(mainLoop);
        return 1;
    }

    guint timeoutId = g_timeout_add(
          SEND_INTERVAL,
          [](gpointer data) -> gboolean {
              EventData *ed = reinterpret_cast<EventData *>(data);
              size_t szBuffer = sizeof(SEND_STRING);
              std::cout << "Send[" << SEND_STRING << "]" << std::endl;
              ed->sample->client->Send(SEND_STRING, szBuffer, "127.0.0.1", ed->port);
              return TRUE;
          },
          src);

    g_main_loop_run(mainLoop);

    delete ed->sample;
    g_source_destroy(src);
    g_source_remove(timeoutId);
    g_main_loop_unref(mainLoop);
    return 0;
}
