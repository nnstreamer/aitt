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
#include "MosquittoMock.h"

CMOCK_MOCK_FUNCTION0(MosquittoMock, mosquitto_lib_init, int(void));
CMOCK_MOCK_FUNCTION0(MosquittoMock, mosquitto_lib_cleanup, int(void));
CMOCK_MOCK_FUNCTION3(MosquittoMock, mosquitto_new,
      struct mosquitto *(const char *id, bool clean_session, void *obj));
CMOCK_MOCK_FUNCTION3(MosquittoMock, mosquitto_int_option,
      int(struct mosquitto *mosq, enum mosq_opt_t option, int value));
CMOCK_MOCK_FUNCTION1(MosquittoMock, mosquitto_destroy, void(struct mosquitto *mosq));
CMOCK_MOCK_FUNCTION3(MosquittoMock, mosquitto_username_pw_set,
      int(struct mosquitto *mosq, const char *username, const char *password));
CMOCK_MOCK_FUNCTION6(MosquittoMock, mosquitto_will_set,
      int(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos,
            bool retain));
CMOCK_MOCK_FUNCTION1(MosquittoMock, mosquitto_will_clear, int(struct mosquitto *mosq));
CMOCK_MOCK_FUNCTION4(MosquittoMock, mosquitto_connect,
      int(struct mosquitto *mosq, const char *host, int port, int keepalive));
CMOCK_MOCK_FUNCTION1(MosquittoMock, mosquitto_disconnect, int(struct mosquitto *mosq));
CMOCK_MOCK_FUNCTION7(MosquittoMock, mosquitto_publish,
      int(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload,
            int qos, bool retain));
CMOCK_MOCK_FUNCTION4(MosquittoMock, mosquitto_subscribe,
      int(struct mosquitto *mosq, int *mid, const char *sub, int qos));
CMOCK_MOCK_FUNCTION3(MosquittoMock, mosquitto_unsubscribe,
      int(struct mosquitto *mosq, int *mid, const char *sub));
CMOCK_MOCK_FUNCTION1(MosquittoMock, mosquitto_loop_start, int(struct mosquitto *mosq));
CMOCK_MOCK_FUNCTION2(MosquittoMock, mosquitto_loop_stop, int(struct mosquitto *mosq, bool force));
CMOCK_MOCK_FUNCTION2(MosquittoMock, mosquitto_message_v5_callback_set,
      void(struct mosquitto *mosq,
            void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *,
                  const struct mqtt5__property *)));
CMOCK_MOCK_FUNCTION2(MosquittoMock, mosquitto_connect_v5_callback_set,
      void(struct mosquitto *mosq,
            void (*on_connect)(struct mosquitto *, void *, int, int, const mosquitto_property *)));
CMOCK_MOCK_FUNCTION2(MosquittoMock, mosquitto_disconnect_v5_callback_set,
      void(struct mosquitto *mosq,
            void (*on_disconnect)(struct mosquitto *, void *, int, const mosquitto_property *)));
