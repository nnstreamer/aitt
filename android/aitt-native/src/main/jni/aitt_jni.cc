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
#include <AittDiscoveryHelper.h>

#include "aitt_jni.h"

/**
 * This class is used as a native interface between the aitt java and aitt C++ layer. It has exposed
 * following APIs to communicate with aitt C++
 * 1. Native interface API to Connect to MQTT broker
 * 2. Native interface API to Subscribe to a topic with protocol and other params
 * 3. Native interface API to Publish to a topic using protocol and other params
 * 4. Native interface API to Unsubscribe to a topic
 * 5. Native interface API to disconnect MQTT connection
 * 6. Native interface API to set MQTT Connection Callback
 */

/* This cbContext is stored during subscribe, so that these cbContext can be used while invoking a
 * java layer APIs */
AittNativeInterface::CallbackContext AittNativeInterface::cbContext = {
        .jvm = nullptr,
        .messageCallbackMethodID = nullptr,
};

/* This constructor creates a new JNI interface object */
AittNativeInterface::AittNativeInterface(std::string &mqId, std::string &ip, bool clearSession)
        : cbObject(nullptr), aitt(mqId, ip, AittOption(clearSession, false)), discovery(nullptr) {
}

/* Destructor called automatically when  AittNativeInterface goes out of scope */
AittNativeInterface::~AittNativeInterface()
{
    if (cbObject != nullptr) {
        JNIEnv *env = nullptr;
        bool attached = false;
        int JNIStatus = cbContext.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
        if (JNIStatus == JNI_EDETACHED) {
            if (cbContext.jvm->AttachCurrentThread(&env, nullptr) != 0) {
                JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to attach current thread");
            } else {
                attached = true;
            }
        } else if (JNIStatus == JNI_EVERSION) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Unsupported version");
        }

        if (env != nullptr) {
            env->DeleteGlobalRef(cbObject);
            cbObject = nullptr;
        }
        if (attached) {
            cbContext.jvm->DetachCurrentThread();
        }
    }

    discovery = nullptr;
}

bool AittNativeInterface::CheckParams(JNIEnv *env, jobject jni_interface_object)
{
    if (env == nullptr || jni_interface_object == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or jobject is null");
        return false;
    } else {
        return true;
    }
}

/**
 * Convert the JNI string to C++ string
 * @param env JNI interface pointer
 * @param str JNI string to convert to C++ string
 * @return C++ string if the conversion is success, else return null
 */
std::string AittNativeInterface::GetStringUTF(JNIEnv *env, jstring str) {
    if (env == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env is null");
        return {};
    }
    const char *cstr = env->GetStringUTFChars(str, nullptr);
    if (cstr == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to get string UTF chars");
        return {};
    }
    std::string _str(cstr);
    env->ReleaseStringUTFChars(str, cstr);
    return _str;
}

/**
 * JNI API to connect to MQTT broker
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param host mqtt broker IP
 * @param port mqtt broker port
 */
void AittNativeInterface::Connect(JNIEnv *env, jobject jni_interface_object, jlong handle,
      jstring host, jint port)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string brokerIp = GetStringUTF(env, host);
    if (brokerIp.empty()) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Broker Ip is empty");
        return;
    }

    int brokerPort = (int) port;

    try {
        instance->aitt.Connect(brokerIp, brokerPort);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to connect");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

/**
 * JNI API to publish data to a given topic
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param topic subscribe topic
 * @param data data to be published
 * @param data_len data length of a publishing data
 * @param protocol publishing protocol
 * @param qos publishing qos
 * @param retain Currently used in MQTT to inform broker to retain data or not
 */
void AittNativeInterface::Publish(JNIEnv *env, jobject jni_interface_object, jlong handle,
      jstring topic, jbyteArray data, jlong data_len, jint protocol, jint qos, jboolean retain)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string customTopic = GetStringUTF(env, topic);
    if (customTopic.empty()) {
        return;
    }

    int num_bytes = (int)data_len;
    const char *cdata = (char *)env->GetByteArrayElements(data, nullptr);
    if (cdata == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to get byte array elements");
        return;
    }
    const void *_data = reinterpret_cast<const void *>(cdata);

    auto _protocol = static_cast<AittProtocol>(protocol);
    auto _qos = static_cast<AittQoS>(qos);
    bool _retain = (bool) retain;

    try {
        instance->aitt.Publish(customTopic, _data, num_bytes, _protocol, _qos, _retain);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to publish");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
    env->ReleaseByteArrayElements(data, reinterpret_cast<jbyte *>((char *) cdata), 0);
}

/**
 * JNI API to disconnect from MQTT broker
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 */
void AittNativeInterface::Disconnect(JNIEnv *env, jobject jni_interface_object, jlong handle)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    try {
        instance->aitt.Disconnect();
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to disconnect");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

bool AittNativeInterface::JniStatusCheck(JNIEnv *&env, int JNIStatus) {
    if (JNIStatus == JNI_EDETACHED) {
        if (cbContext.jvm->AttachCurrentThread(&env, nullptr) != 0) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to attach current thread");
            return false;
        }
    } else if (JNIStatus == JNI_EVERSION) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Unsupported version");
        return false;
    }
    return true;
}

/**
 * JNI API to subscribe to a given topic
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param topic subscribe topic
 * @param protocol subscribe protocol
 * @param qos subscribe qos
 * @return AittSubscribeID as long
 */
jlong AittNativeInterface::Subscribe(JNIEnv *env, jobject jni_interface_object, jlong handle,
      jstring topic, jint protocol, jint qos)
{
    if (!CheckParams(env, jni_interface_object)) {
        return 0L;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string customTopic = GetStringUTF(env, topic);
    if (customTopic.empty()) {
        return 0L;
    }

    auto _protocol = static_cast<AittProtocol>(protocol);
    auto _qos = static_cast<AittQoS>(qos);

    AittSubscribeID _id = nullptr;
    try {
        _id = instance->aitt.Subscribe(
              customTopic,
              [&](AittMsg *handle, const void *msg, const int msg_size, void *cbdata) -> void {
                  auto *instance = reinterpret_cast<AittNativeInterface *>(cbdata);
                  JNIEnv *env;
                  int JNIStatus =
                        cbContext.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

                  if (!JniStatusCheck(env, JNIStatus)) {
                      return;
                  }

                  if (env != nullptr && instance->cbObject != nullptr) {
                      jstring _topic = env->NewStringUTF(handle->GetTopic().c_str());
                      if (env->ExceptionCheck() == true) {
                          JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new UTF string");
                          cbContext.jvm->DetachCurrentThread();
                          return;
                      }

                      jbyteArray array = env->NewByteArray(msg_size);
                      auto _msg = reinterpret_cast<unsigned char *>(const_cast<void *>(msg));
                      env->SetByteArrayRegion(array, 0, msg_size, reinterpret_cast<jbyte *>(_msg));
                      if (env->ExceptionCheck() == true) {
                          JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to set byte array");
                          cbContext.jvm->DetachCurrentThread();
                          return;
                      }

                      env->CallVoidMethod(instance->cbObject, cbContext.messageCallbackMethodID,
                            _topic, array);
                      if (env->ExceptionCheck() == true) {
                          JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to call void method");
                          cbContext.jvm->DetachCurrentThread();
                          return;
                      }
                  }
                  cbContext.jvm->DetachCurrentThread();
              },
              reinterpret_cast<void *>(instance), _protocol, _qos);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to subscribe");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
    return (jlong) _id;
}

/**
 * JNI API to unsubscribe
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param aitt subscribe id received in subscribe()
 */
void AittNativeInterface::Unsubscribe(JNIEnv *env, jobject jni_interface_object, jlong handle,
      jlong aittSubId)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    void *subId = reinterpret_cast<void *>(aittSubId);
    try {
        instance->aitt.Unsubscribe(static_cast<AittSubscribeID>(subId));
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to unsubscribe");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

/**
 * JNI API to set the connection callback with aitt C++
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 */
void AittNativeInterface::SetConnectionCallback(JNIEnv *env, jobject jni_interface_object,
      jlong handle)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);

    try {
        instance->aitt.SetConnectionCallback(
              [&](AITT &handle, int status, void *user_data) -> void {
                  auto *instance = reinterpret_cast<AittNativeInterface *>(user_data);
                  JNIEnv *env;
                  int JNIStatus =
                        cbContext.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

                  if (!JniStatusCheck(env, JNIStatus)) {
                      return;
                  }

                  if (env != nullptr && instance->cbObject != nullptr) {
                      env->CallVoidMethod(instance->cbObject, cbContext.connectionCallbackMethodID,
                            (jint)status);
                      if (env->ExceptionCheck() == true) {
                          JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to call void method");
                          cbContext.jvm->DetachCurrentThread();
                          return;
                      }
                  }
                  cbContext.jvm->DetachCurrentThread();
              },
              reinterpret_cast<void *>(instance));
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to set connection callback");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

/**
 * JNI API to set the discovery callback with aitt C++
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param topic String for which discovery message is to be received
 * @return callback handle as int
 */
jint AittNativeInterface::SetDiscoveryCallback(JNIEnv *env, jobject jni_interface_object,
      jlong handle, jstring topic)
{
    if (!CheckParams(env, jni_interface_object)) {
        return -1;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string _topic = GetStringUTF(env, topic);
    if (_topic.empty()) {
        return -1;
    }

    int cb = instance->discovery->AddDiscoveryCB(_topic,
                std::bind(&AittNativeInterface::DiscoveryMessageCallback, instance, _topic,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    return (jint) cb;
}

/**
 * JNI API to remove the discovery callback with aitt C++
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param cbHandle Discovery callback handle
 */
void AittNativeInterface::RemoveDiscoveryCallback(JNIEnv *env, jobject jni_interface_object,
      jlong handle, jint cbHandle)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    instance->discovery->RemoveDiscoveryCB(cbHandle);
}

/**
 * JNI API to update discovery message
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param handle AittNativeInterface object
 * @param topic String for which discovery message to be updated
 * @param data ByteArray containing discovery message
 * @param data_len int length of discovery message
 */
void AittNativeInterface::UpdateDiscoveryMessage(JNIEnv *env, jobject jni_interface_object,
      jlong handle, jstring topic, jbyteArray data, jlong data_len)
{
    if (!CheckParams(env, jni_interface_object)) {
        return;
    }

    auto *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string _topic = GetStringUTF(env, topic);
    if (_topic.empty()) {
        return;
    }

    int num_bytes = (int)data_len;
    const char *cdata = (char *)env->GetByteArrayElements(data, nullptr);
    if (cdata == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to get byte array elements");
        return;
    }
    const void *_data = reinterpret_cast<const void *>(cdata);

    instance->discovery->UpdateDiscoveryMsg(_topic, _data, num_bytes);
}

void AittNativeInterface::DiscoveryMessageCallback(const std::string &topic, const std::string &clientId,
      const std::string &status, const void *msg, const int szmsg)
{
    JNIEnv *env;
    int JNIStatus = cbContext.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    if (!JniStatusCheck(env, JNIStatus)) {
        return;
    }

    if (env != nullptr) {
        jstring _topic = env->NewStringUTF(topic.c_str());
        if (env->ExceptionCheck() == true) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new UTF string");
            cbContext.jvm->DetachCurrentThread();
            return;
        }

        jstring _clientId = env->NewStringUTF(clientId.c_str());
        if (env->ExceptionCheck() == true) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new UTF string");
            cbContext.jvm->DetachCurrentThread();
            return;
        }

        jstring _status = env->NewStringUTF(status.c_str());
        if (env->ExceptionCheck() == true) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new UTF string");
            cbContext.jvm->DetachCurrentThread();
            return;
        }

        jbyteArray array = env->NewByteArray(szmsg);
        auto _msg = reinterpret_cast<unsigned char *>(const_cast<void *>(msg));
        env->SetByteArrayRegion(array, 0, szmsg, reinterpret_cast<jbyte *>(_msg));
        if (env->ExceptionCheck() == true) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to set byte array");
            cbContext.jvm->DetachCurrentThread();
            return;
        }

        env->CallVoidMethod(cbObject, cbContext.discoveryCallbackMethodID, _topic, _clientId, _status, array);
        if (env->ExceptionCheck() == true) {
            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to call void method");
            cbContext.jvm->DetachCurrentThread();
            return;
        }
    }
    cbContext.jvm->DetachCurrentThread();
}

/**
 * Creates a AittNativeInterface object which inturn creates a aitt C++ instance
 * @param env JNI interface pointer
 * @param jni_interface_object JNI interface object
 * @param id unique mqtt id
 * @param ip self IP address of device
 * @param clearSession to clear current session if client disconnects
 * @return returns the aitt interface object in long
 */
jlong AittNativeInterface::Init(JNIEnv *env, jobject jni_interface_object, jstring id, jstring ip,
      jboolean clearSession)
{
    if (!CheckParams(env, jni_interface_object)) {
        return JNI_ERR;
    }

    std::string mqId = GetStringUTF(env, id);
    if (mqId.empty()) {
        return 0L;
    }

    std::string selfIp = GetStringUTF(env, ip);
    if (selfIp.empty()) {
        return 0L;
    }

    bool _clearSession = clearSession;

    AittNativeInterface *instance;
    try {
        instance = new AittNativeInterface(mqId, selfIp, _clearSession);
        aitt::AittDiscoveryHelper discoveryHelper;
        instance->discovery = discoveryHelper.GetAittDiscovery(instance->aitt);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new instance");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
        return 0L;
    }

    if (env->GetJavaVM(&cbContext.jvm) != JNI_OK) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Unable to get Java VM");
        delete instance;
        return 0L;
    }
    try {
        instance->cbObject = env->NewGlobalRef(jni_interface_object);

        jclass callbackClass = env->FindClass("com/samsung/android/aittnative/JniInterface");
        cbContext.messageCallbackMethodID =
                env->GetMethodID(callbackClass, "messageCallback", "(Ljava/lang/String;[B)V");
        cbContext.connectionCallbackMethodID =
                env->GetMethodID(callbackClass, "connectionStatusCallback", "(I)V");
        cbContext.discoveryCallbackMethodID =
                env->GetMethodID(callbackClass, "discoveryMessageCallback", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[B)V");
        env->DeleteLocalRef(callbackClass);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
        delete instance;
        return 0L;
    }

    return reinterpret_cast<long>(instance);
}

/**
 * The VM calls this method automatically, when the native library is loaded.
 * @param vm JVM instance
 * @param reserved reserved for future
 * @return JNI instance being used
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Not a valid JNI version");
        return JNI_ERR;
    }
    jclass klass = env->FindClass("com/samsung/android/aittnative/JniInterface");
    if (nullptr == klass) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "klass is null");
        return JNI_ERR;
    }
    static JNINativeMethod aitt_jni_methods[] = {
            {"initJNI",                     "(Ljava/lang/String;Ljava/lang/String;Z)J", reinterpret_cast<void *>(AittNativeInterface::Init)},
            {"connectJNI",                  "(JLjava/lang/String;I)V",                  reinterpret_cast<void *>(AittNativeInterface::Connect)},
            {"subscribeJNI",                "(JLjava/lang/String;II)J",                 reinterpret_cast<void *>(AittNativeInterface::Subscribe)},
            {"publishJNI",                  "(JLjava/lang/String;[BJIIZ)V",             reinterpret_cast<void *>(AittNativeInterface::Publish)},
            {"unsubscribeJNI",              "(JJ)V",                                    reinterpret_cast<void *>(AittNativeInterface::Unsubscribe)},
            {"disconnectJNI",               "(J)V",                                     reinterpret_cast<void *>(AittNativeInterface::Disconnect)},
            {"setConnectionCallbackJNI",    "(J)V",                                     reinterpret_cast<void *>(AittNativeInterface::SetConnectionCallback)},
            {"setDiscoveryCallbackJNI",     "(JLjava/lang/String;)I",                   reinterpret_cast<void *>(AittNativeInterface::SetDiscoveryCallback)},
            {"removeDiscoveryCallbackJNI",  "(JI)V",                                    reinterpret_cast<void *>(AittNativeInterface::RemoveDiscoveryCallback)},
            {"updateDiscoveryMessageJNI",   "(JLjava/lang/String;[BJ)V",                reinterpret_cast<void *>(AittNativeInterface::UpdateDiscoveryMessage)}};
    if (env->RegisterNatives(klass, aitt_jni_methods,
                             sizeof(aitt_jni_methods) / sizeof(aitt_jni_methods[0]))) {
        env->DeleteLocalRef(klass);
        return JNI_ERR;
    }
    env->DeleteLocalRef(klass);
    JNI_LOG(ANDROID_LOG_INFO, TAG, "JNI loaded successfully");
    return JNI_VERSION_1_6;
}
