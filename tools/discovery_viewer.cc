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
#include <argp.h>
#include <flatbuffers/flexbuffers.h>
#include <mosquitto.h>

#include "FlexbufPrinter.h"
#include "aitt_internal.h"

struct ParsingData {
    ParsingData() : clean(false), broker_ip("127.0.0.1") {}
    bool clean;
    std::string broker_ip;
};
const char *argp_program_version = "aitt-discovery-viewer 1.0";

static char argp_doc[] =
      "AITT Discovery topic Viewer"
      "\v"
      "Tizen: <http://www.tizen.org>";

/* The options we understand. */
static struct argp_option options[] = {{"broker", 'b', "IP", 0, "broker ip address"},
      {"clean", 'c', 0, OPTION_ARG_OPTIONAL, "clean discovery topic"}, {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct ParsingData *args = (struct ParsingData *)state->input;

    switch (key) {
    case 'b':
        args->broker_ip = arg;
        break;
    case 'c':
        args->clean = true;
        break;
    case ARGP_KEY_NO_ARGS:
        // argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

class MQTTHandler {
  public:
    MQTTHandler()
    {
        int ret = mosquitto_lib_init();
        if (ret != MOSQ_ERR_SUCCESS)
            ERR("mosquitto_lib_init() Fail(%s)", mosquitto_strerror(ret));

        std::string id = std::to_string(rand() % 100);
        handle = mosquitto_new(id.c_str(), true, nullptr);
        if (handle == nullptr) {
            ERR("mosquitto_new() Fail");
            mosquitto_lib_cleanup();
            return;
        }
        mosquitto_int_option(handle, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

        mosquitto_message_v5_callback_set(handle,
              [](mosquitto *handle, void *user_data, const mosquitto_message *msg,
                    const mosquitto_property *props) {
                  std::string topic = msg->topic;
                  size_t end = topic.find("/", DISCOVERY_TOPIC_BASE.length());
                  std::string clientId = topic.substr(DISCOVERY_TOPIC_BASE.length(), end);
                  FlexbufPrinter printer;
                  if (msg->payloadlen)
                      printer.PrettyPrint(clientId, static_cast<const uint8_t *>(msg->payload),
                            msg->payloadlen);
              });

        ret = mosquitto_loop_start(handle);
        if (ret != MOSQ_ERR_SUCCESS)
            ERR("mosquitto_loop_start() Fail(%s)", mosquitto_strerror(ret));
    }

    void Connect(const std::string &broker_ip)
    {
        int ret = mosquitto_connect(handle, broker_ip.c_str(), 1883, 60);
        if (ret != MOSQ_ERR_SUCCESS)
            ERR("mosquitto_connect() Fail(%s)", mosquitto_strerror(ret));

        int mid = 0;
        ret = mosquitto_subscribe(handle, &mid, (DISCOVERY_TOPIC_BASE + "+").c_str(), 0);
        if (ret != MOSQ_ERR_SUCCESS)
            ERR("mosquitto_subscribe() Fail(%s)", mosquitto_strerror(ret));
    }

  private:
    mosquitto *handle;
};

int main(int argc, char **argv)
{
    struct ParsingData arg;
    struct argp argp_conf = {options, parse_opt, 0, argp_doc};

    argp_parse(&argp_conf, argc, argv, 0, 0, &arg);

    if (arg.clean) {
        ERR("Not Supported");
        return -1;
    }

    MQTTHandler mqtt;
    mqtt.Connect(arg.broker_ip);

    while (1) {
        sleep(10);
    }

    return 0;
}
