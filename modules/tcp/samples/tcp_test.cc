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
#include "../TCP.h"

#include <getopt.h>
#include <glib.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "../TCPServer.h"

//#define _LOG_WITH_TIMESTAMP
#include "aitt_internal.h"
#ifdef _LOG_WITH_TIMESTAMP
__thread __aitt__tls__ __aitt;
#endif

#define HELLO_STRING "hello"
#define BYE_STRING "bye"
#define SEND_INTERVAL 1000

using namespace AittTCPNamespace;

class AittTcpSample {
  public:
    AittTcpSample(const std::string &host, unsigned short &port)
          : server(new TCP::Server(host, port))
    {
    }
    virtual ~AittTcpSample(void) {}

    std::unique_ptr<TCP::Server> server;
};

int main(int argc, char *argv[])
{
    const option opts[] = {
          {
                .name = "server",
                .has_arg = 0,
                .flag = nullptr,
                .val = 's',
          },
          {
                .name = "host",
                .has_arg = 1,
                .flag = nullptr,
                .val = 'h',
          },
          {
                .name = "port",
                .has_arg = 1,
                .flag = nullptr,
                .val = 'p',
          },
    };
    int c;
    int idx;
    bool isServer = false;
    std::string host = "127.0.0.1";
    unsigned short port = 0;

    while ((c = getopt_long(argc, argv, "sh:up:", opts, &idx)) != -1) {
        switch (c) {
        case 's':
            isServer = true;
            break;
        case 'h':
            host = optarg;
            break;
        case 'p':
            port = std::stoi(optarg);
            break;
        default:
            break;
        }
    }

    INFO("Host[%s] port[%u]", host.c_str(), port);

    struct EventData {
        GSource source_;
        GPollFD fd;
        AittTcpSample *sample;
    };

    guint timeoutId = 0;
    GSource *src = nullptr;
    EventData *ed = nullptr;

    GMainLoop *mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        ERR("Failed to create a main loop");
        return 1;
    }

    // Handling the server/client events
    if (isServer) {
        GSourceFuncs srcs = {
              [](GSource *src, gint *timeout) -> gboolean {
                  *timeout = 1;
                  return FALSE;
              },
              [](GSource *src) -> gboolean {
                  EventData *ed = reinterpret_cast<EventData *>(src);
                  RETV_IF(ed == nullptr, FALSE);

                  if ((ed->fd.revents & G_IO_IN) == G_IO_IN)
                      return TRUE;
                  if ((ed->fd.revents & G_IO_ERR) == G_IO_ERR)
                      return TRUE;

                  return FALSE;
              },
              [](GSource *src, GSourceFunc callback, gpointer user_data) -> gboolean {
                  EventData *ed = reinterpret_cast<EventData *>(src);
                  RETV_IF(ed == nullptr, FALSE);

                  if ((ed->fd.revents & G_IO_ERR) == G_IO_ERR) {
                      ERR("Error!");
                      return FALSE;
                  }

                  std::unique_ptr<TCP> peer = ed->sample->server->AcceptPeer();

                  INFO("Assigned port: %u, %u", ed->sample->server->GetPort(), peer->GetPort());
                  std::string peerHost;
                  unsigned short peerPort = 0;
                  peer->GetPeerInfo(peerHost, peerPort);
                  INFO("Peer Info: %s %u", peerHost.c_str(), peerPort);

                  char buffer[10];
                  void *ptr = static_cast<void *>(buffer);
                  int32_t szData = sizeof(HELLO_STRING);
                  peer->Recv(ptr, szData);
                  INFO("Gots[%s]", buffer);

                  szData = sizeof(BYE_STRING);
                  peer->Send(BYE_STRING, szData);
                  INFO("Reply to client[%s]", BYE_STRING);

                  return TRUE;
              },
              nullptr,
        };

        src = g_source_new(&srcs, sizeof(EventData));
        if (!src) {
            g_main_loop_unref(mainLoop);
            ERR("g_source_new failed");
            return 1;
        }

        ed = reinterpret_cast<EventData *>(src);

        try {
            ed->sample = new AittTcpSample(host, port);
        } catch (std::exception &e) {
            ERR("new: %s", e.what());
            g_source_unref(src);
            g_main_loop_unref(mainLoop);
            return 1;
        }

        INFO("host: %s, port: %u", host.c_str(), port);

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
    } else {
        static struct Main {
            const std::string &host;
            unsigned short port;
        } main_data = {
              .host = host,
              .port = port,
        };
        // Now the server is ready.
        // Let's create a new client and communicate with the server within every
        // SEND_INTERTVAL
        timeoutId = g_timeout_add(
              SEND_INTERVAL,
              [](gpointer data) -> gboolean {
                  Main *ctx = static_cast<Main *>(data);
                  TCP::ConnectInfo info;
                  info.port = ctx->port;
                  std::unique_ptr<TCP> client(new TCP(ctx->host, info));

                  INFO("Assigned client port: %u", client->GetPort());

                  INFO("Send[%s]", HELLO_STRING);
                  int32_t szBuffer = sizeof(HELLO_STRING);
                  client->Send(HELLO_STRING, szBuffer);

                  char buffer[10];
                  void *ptr = static_cast<void *>(buffer);
                  szBuffer = sizeof(BYE_STRING);
                  client->Recv(ptr, szBuffer);
                  INFO("Replied with[%s]", buffer);

                  // Send oneshot message, and disconnect from the server
                  return TRUE;
              },
              &main_data);
    }

    g_main_loop_run(mainLoop);

    if (src) {
        delete ed->sample;
        g_source_destroy(src);
    }
    if (timeoutId)
        g_source_remove(timeoutId);
    g_main_loop_unref(mainLoop);
    return 0;
}
