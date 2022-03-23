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
#include <cstring>

#include "MQTTMock.h"
#include "MQTTTest.h"

MQTTMock *MQTTTest::mqttMock = nullptr;

extern "C" {

int mosquitto_lib_init(void)
{
    return MQTTTest::GetMock().mosquitto_lib_init();
}

int mosquitto_lib_cleanup(void)
{
    return MQTTTest::GetMock().mosquitto_lib_cleanup();
}

struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *obj)
{
    return MQTTTest::GetMock().mosquitto_new(id, clean_session, obj);
}

int mosquitto_int_option(struct mosquitto *mosq, int option, int value)
{
    return MQTTTest::GetMock().mosquitto_int_option(mosq, option, value);
}

void mosquitto_destroy(struct mosquitto *mosq)
{
    return MQTTTest::GetMock().mosquitto_destroy(mosq);
}

int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen,
      const void *payload, int qos, bool retain)
{
    return MQTTTest::GetMock().mosquitto_will_set(mosq, topic, payloadlen, payload, qos, retain);
}

int mosquitto_will_clear(struct mosquitto *mosq)
{
    return MQTTTest::GetMock().mosquitto_will_clear(mosq);
}

int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
    return MQTTTest::GetMock().mosquitto_connect(mosq, host, port, keepalive);
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
    return MQTTTest::GetMock().mosquitto_disconnect(mosq);
}

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen,
      const void *payload, int qos, bool retain)
{
    return MQTTTest::GetMock().mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos,
          retain);
}

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
    return MQTTTest::GetMock().mosquitto_subscribe(mosq, mid, sub, qos);
}

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
    return MQTTTest::GetMock().mosquitto_unsubscribe(mosq, mid, sub);
}

int mosquitto_loop_start(struct mosquitto *mosq)
{
    return MQTTTest::GetMock().mosquitto_loop_start(mosq);
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
    return MQTTTest::GetMock().mosquitto_loop_stop(mosq, force);
}

void mosquitto_message_v5_callback_set(struct mosquitto *mosq,
      void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *,
            const struct mqtt5__property *))
{
    return MQTTTest::GetMock().mosquitto_message_v5_callback_set(mosq, on_message);
}

}  // extern "C"
