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
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <AittTypes.h>
/**
 * @addtogroup CAPI_AITT_MODULE
 * @{
 */

/**
 * @brief @a aitt_h is an opaque data structure to represent AITT service handle.
 * @since_tizen 7.0
 */
typedef struct aitt_handle *aitt_h;

/**
 * @brief @a aitt_msg_h is an opaque data structure to represent AITT message handle.
 * @since_tizen 7.0
 * @see aitt_sub_fn
 * @see aitt_msg_get_topic()
 */
typedef void *aitt_msg_h;

/**
 * @brief @a aitt_sub_h is an opaque data structure to represent AITT subscribe ID.
 * @since_tizen 7.0
 * @see aitt_subscribe(), aitt_unsubscribe()
 */
typedef AittSubscribeID aitt_sub_h;

/**
 * @brief Enumeration for protocol.
 * @since_tizen 7.0
 */
typedef enum AittProtocol aitt_protocol_e;

/**
 * @brief Enumeration for MQTT QoS.
 *        It only works with the AITT_TYPE_MQTT
 * @since_tizen 7.0
 */
typedef enum AittQoS aitt_qos_e;

/**
 * @brief Enumeration for AITT error code.
 * @since_tizen 7.0
 */
typedef enum AittError aitt_error_e;

/**
 * @brief Specify the type of function passed to aitt_subscribe().
 * @details When the aitt get message, it is called, immediately.
 * @since_tizen 7.0
 * @param[in] msg_handle aitt message handle. The handle has topic name and so on. @c aitt_msg_h
 * @param[in] msg pointer to the data received
 * @param[in] msg_len the size of the @c msg (bytes)
 * @param[in] user_data The user data to pass to the function
 *
 * @pre The callback must be registered using aitt_subscribe(), and aitt_subscribe_full()
 *
 * @see aitt_subscribe()
 * @see aitt_subscribe_full()
 */
typedef void (
      *aitt_sub_fn)(aitt_msg_h msg_handle, const void *msg, size_t msg_len, void *user_data);

/**
 * @brief Create a new AITT service instance.
 * @detail If id is NULL or empty string, id will be generated automatically.
 *         If my_ip is NULL or empty string, my_ip will be set as 127.0.0.1.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] id Unique identifier in local network
 * @param[in] my_ip Own device ip address for connecting by others
 * @return @c handle of AITT service
 *         otherwise NULL value on failure
 * @see aitt_destroy()
 */
aitt_h aitt_new(const char *id, const char *my_ip);

/**
 * @brief Enumeration for option.
 * @since_tizen 7.0
 */
typedef enum {
    AITT_OPT_UNKNOWN, /**< Unknown */
} aitt_option_e;

/**
 * @brief Set the contents of a @c handle related with @c option to @c value
 * @detail The @c value can be NULL for removing the content
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] option value of @a aitt_option_e.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 *
 * @see aitt_get_option()
 */
int aitt_set_option(aitt_h handle, aitt_option_e option, const char *value);

/**
 * @brief Returns the string value of a @c handle related with @c option
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] option value of @a aitt_option_e.
 * @return @c value related with @c option
 *         otherwise NULL value
 *
 * @see aitt_set_option()
 */
const char *aitt_get_option(aitt_h handle, aitt_option_e option);

/**
 * @brief Configure will information for a aitt instance.
 * @detail By default, clients do not have a will. This must be called before calling aitt_connect()
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic the topic on which to publish the will.
 * @param[in] msg pointer to the data to send.
 * @param[in] msg_len the size of the @c msg (bytes). Valid values are between 0 and 268,435,455.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service.
 * @param[in] retain set to true to make the will a retained message.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 *
 * @see aitt_connect()
 */
int aitt_will_set(aitt_h handle, const char *topic, const void *msg, const size_t msg_len,
      aitt_qos_e qos, bool retain);

/**
 * @brief Release memory of the AITT service instance.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service;
 * @see aitt_new()
 */
void aitt_destroy(aitt_h handle);

/**
 * @brief Connect to mqtt broker.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] broker_ip IP address of the broker to connect to
 * @param[in] port the network port to connect to. Usually 1883.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_connect(aitt_h handle, const char *broker_ip, int port);

/**
 * @brief Connect to mqtt broker as aitt_connect(), but takes username and password.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] broker_ip IP address of the broker to connect to
 * @param[in] port the network port to connect to. Usually 1883.
 * @param[in] username the username to send as a string, or NULL to disable authentication
 * @param[in] password the password to send as a string
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_connect_full(aitt_h handle, const char *broker_ip, int port, const char *username,
      const char *password);

/**
 * @brief Disconnect from the broker.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_disconnect(aitt_h handle);

/**
 * @brief Publish a message on a given topic using MQTT, QoS0(At most once).
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic null terminated string of the topic to publish to.
 * @param[in] msg pointer to the data to send.
 * @param[in] msg_len the size of the @c msg (bytes). Valid values are between 0 and 268,435,455.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_publish(aitt_h handle, const char *topic, const void *msg, const size_t msg_len);

/**
 * @brief Publish a message on a given topic as aitt_publish(), but takes protocols and qos.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic null terminated string of the topic to publish to.
 * @param[in] msg pointer to the data to send.
 * @param[in] msg_len the size of the @c msg (bytes). Valid values are between 0 and 268,435,455.
 * @param[in] protocols value of @a aitt_protocol_e. The value can be bitwise-or'd.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_publish_full(aitt_h handle, const char *topic, const void *msg, const size_t msg_len,
      int protocols, aitt_qos_e qos);

/**
 * @brief Publish a message on a given topic as aitt_publish_full(),
 *        but takes reply topic and callback for the reply.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic null terminated string of the topic to publish to.
 * @param[in] msg pointer to the data to send.
 * @param[in] msg_len the size of the @c msg (bytes). Valid values are between 0 and 268,435,455.
 * @param[in] protocols value of @a aitt_protocol_e. The value can be bitwise-or'd.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service.
 * @param[in] correlation value indicating the Correlation.
 * @param[in] cb The callback function to invoke
 * @param[in] user_data The user data to pass to the function
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_publish_with_reply(aitt_h handle, const char *topic, const void *msg, const size_t msg_len,
      aitt_protocol_e protocols, aitt_qos_e qos, const char *correlation, aitt_sub_fn cb,
      void *user_data);

/**
 * @brief Send reply message to regarding topic.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] msg_handle Handle of published message(to reply).
 * @param[in] reply pointer to the data to send.
 * @param[in] reply_len the size of the @c reply (bytes).
 *            Valid values are between 0 and 268,435,455.
 * @param[in] end boolean value indicating the reply message is end or not.
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_send_reply(aitt_h handle, aitt_msg_h msg_handle, const void *reply, const size_t reply_len,
      bool end);

/**
 * @brief Get topic name from @c handle
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle aitt message handle
 * @return topic name on success otherwise NULL value on failure
 */
const char *aitt_msg_get_topic(aitt_msg_h handle);

/**
 * @brief Subscribe to a topic on MQTT with QoS0(at most once).
 * @details Sets a function to be called when the aitt get messages related with @c topic
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic null terminated string of the topic to subscribe to.
 * @param[in] cb The callback function to invoke
 * @param[in] user_data The user data to pass to the function
 * @param[out] sub_handle Handle of subscribed topic
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_subscribe(aitt_h handle, const char *topic, aitt_sub_fn cb, void *user_data,
      aitt_sub_h *sub_handle);

/**
 * @brief Subscribe to a topic, but takes protocols and qos.
 * @details Sets a function to be called when the aitt get messages related with @c topic
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] topic null terminated string of the topic to subscribe to.
 * @param[in] cb The callback function to invoke
 * @param[in] user_data The user data to pass to the function
 * @param[in] protocols value of @a aitt_protocol_e.
 * @param[in] qos integer value 0, 1 or 2 indicating the Quality of Service.
 * @param[out] sub_handle Handle of subscribed topic
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 */
int aitt_subscribe_full(aitt_h handle, const char *topic, aitt_sub_fn cb, void *user_data,
      aitt_protocol_e protocols, aitt_qos_e qos, aitt_sub_h *sub_handle);

/**
 * @brief Unsubscribe from a topic.
 * @details Removes the subscription of changes with given ID.
 * @since_tizen 7.0
 * @privlevel public
 * @param[in] handle Handle of AITT service
 * @param[in] sub_handle Handle of subscribed topic
 * @return @c 0 on success
 *         otherwise a negative error value
 * @retval #AITT_ERROR_NONE  Success
 * @retval #AITT_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #AITT_ERROR_SYSTEM System errors
 *
 * @see aitt_subscribe(), aitt_subscribe_full()
 */
int aitt_unsubscribe(aitt_h handle, aitt_sub_h sub_handle);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
