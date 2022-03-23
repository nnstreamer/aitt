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
#pragma once

#include <gmock/gmock.h>

class MQTTMock {
  public:
    MQTTMock(void) = default;
    virtual ~MQTTMock(void) = default;

    MOCK_METHOD0(mosquitto_lib_init, int(void));
    MOCK_METHOD0(mosquitto_lib_cleanup, int(void));
    MOCK_METHOD3(mosquitto_new, struct mosquitto *(const char *id, bool clean_session, void *obj));
    MOCK_METHOD3(mosquitto_int_option, int(struct mosquitto *mosq, int option, int value));
    MOCK_METHOD1(mosquitto_destroy, void(struct mosquitto *mosq));
    MOCK_METHOD6(mosquitto_will_set, int(struct mosquitto *mosq, const char *topic, int payloadlen,
                                           const void *payload, int qos, bool retain));
    MOCK_METHOD1(mosquitto_will_clear, int(struct mosquitto *mosq));
    MOCK_METHOD4(mosquitto_connect,
          int(struct mosquitto *mosq, const char *host, int port, int keepalive));
    MOCK_METHOD1(mosquitto_disconnect, int(struct mosquitto *mosq));
    MOCK_METHOD7(mosquitto_publish,
          int(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen,
                const void *payload, int qos, bool retain));
    MOCK_METHOD4(mosquitto_subscribe,
          int(struct mosquitto *mosq, int *mid, const char *sub, int qos));
    MOCK_METHOD3(mosquitto_unsubscribe, int(struct mosquitto *mosq, int *mid, const char *sub));
    MOCK_METHOD1(mosquitto_loop_start, int(struct mosquitto *mosq));
    MOCK_METHOD2(mosquitto_loop_stop, int(struct mosquitto *mosq, bool force));
    MOCK_METHOD2(mosquitto_message_v5_callback_set,
          void(struct mosquitto *mosq,
                void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *,
                      const struct mqtt5__property *)));
};
