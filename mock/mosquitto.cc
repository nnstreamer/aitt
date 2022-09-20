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

#include "MQMockTest.h"
#include "MQTTMock.h"
#include "aitt_internal.h"

MQTTMock *MQMockTest::mqttMock = nullptr;

extern "C" {

API int mosquitto_lib_init(void)
{
    return MQMockTest::GetMock().mosquitto_lib_init();
}

API int mosquitto_lib_cleanup(void)
{
    return MQMockTest::GetMock().mosquitto_lib_cleanup();
}

API struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *obj)
{
    return MQMockTest::GetMock().mosquitto_new(id, clean_session, obj);
}

API int mosquitto_int_option(struct mosquitto *mosq, enum mosq_opt_t option, int value)
{
    return MQMockTest::GetMock().mosquitto_int_option(mosq, option, value);
}

API void mosquitto_destroy(struct mosquitto *mosq)
{
    return MQMockTest::GetMock().mosquitto_destroy(mosq);
}

API int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username,
      const char *password)
{
    return MQMockTest::GetMock().mosquitto_username_pw_set(mosq, username, password);
}

API int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen,
      const void *payload, int qos, bool retain)
{
    return MQMockTest::GetMock().mosquitto_will_set(mosq, topic, payloadlen, payload, qos, retain);
}

API int mosquitto_will_clear(struct mosquitto *mosq)
{
    return MQMockTest::GetMock().mosquitto_will_clear(mosq);
}

API int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
    return MQMockTest::GetMock().mosquitto_connect(mosq, host, port, keepalive);
}

API int mosquitto_disconnect(struct mosquitto *mosq)
{
    return MQMockTest::GetMock().mosquitto_disconnect(mosq);
}

API int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen,
      const void *payload, int qos, bool retain)
{
    return MQMockTest::GetMock().mosquitto_publish(mosq, mid, topic, payloadlen, payload, qos,
          retain);
}

API int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
    return MQMockTest::GetMock().mosquitto_subscribe(mosq, mid, sub, qos);
}

API int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
    return MQMockTest::GetMock().mosquitto_unsubscribe(mosq, mid, sub);
}

API int mosquitto_loop_start(struct mosquitto *mosq)
{
    return MQMockTest::GetMock().mosquitto_loop_start(mosq);
}

API int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
    return MQMockTest::GetMock().mosquitto_loop_stop(mosq, force);
}

API void mosquitto_message_v5_callback_set(struct mosquitto *mosq,
      void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *,
            const struct mqtt5__property *))
{
    return MQMockTest::GetMock().mosquitto_message_v5_callback_set(mosq, on_message);
}

API void mosquitto_connect_v5_callback_set(struct mosquitto *mosq,
      void (*on_connect)(struct mosquitto *, void *, int, int, const mosquitto_property *))
{
    return MQMockTest::GetMock().mosquitto_connect_v5_callback_set(mosq, on_connect);
}

API void mosquitto_disconnect_v5_callback_set(struct mosquitto *mosq,
      void (*on_disconnect)(struct mosquitto *, void *, int, const mosquitto_property *))
{
    return MQMockTest::GetMock().mosquitto_disconnect_v5_callback_set(mosq, on_disconnect);
}

}  // extern "C"
