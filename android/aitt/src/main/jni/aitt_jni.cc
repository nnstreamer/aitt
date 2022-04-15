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
#include "aitt_jni.h"

AittNativeInterface::CallbackContext AittNativeInterface::cbContext = {
        .jvm = nullptr,
        .messageCallbackMethodID = nullptr,
};

AittNativeInterface::AittNativeInterface(std::string &mqId, std::string &ip, bool clearSession)
        : cbObject(nullptr)
        , aitt(mqId, ip, clearSession)
{
}

AittNativeInterface::~AittNativeInterface(void)
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
}

std::string AittNativeInterface::GetStringUTF(JNIEnv *env, jstring str)
{
    if (env == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env is null");
        return nullptr;
    }
    const char *cstr = env->GetStringUTFChars(str, 0);
    if (cstr == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to get string UTF chars");
        return nullptr;
    }
    std::string _str(cstr);
    env->ReleaseStringUTFChars(str, cstr);
    return _str;
}

void AittNativeInterface::Java_com_samsung_android_aitt_Aitt_connectJNI(JNIEnv *env, jobject jniInterfaceObject,
                                          jlong handle, jstring host, jint port)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or Jobject is null");
        return;
    }
    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string brokerIp = GetStringUTF(env, host);
    if (brokerIp.empty()) {
        return;
    }

    int brokerPort = (int)port;

    try {
        instance->aitt.Connect(brokerIp, brokerPort);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to connect");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

void AittNativeInterface::Java_com_samsung_android_aitt_Aitt_publishJNI(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                                          jstring topic, jbyteArray data, jlong datalen, jint protocol, jint qos, jboolean retain)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or jobject is null");
        return;
    }
    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string customTopic = GetStringUTF(env, topic);
    if (customTopic.empty()) {
        return;
    }

    int num_bytes = (int)datalen;
    const char *cdata = (char *)env->GetByteArrayElements(data, 0);
    if (cdata == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to get byte array elements");
        return;
    }
    const void *_data = reinterpret_cast<const void*>(cdata);

    AittProtocol _protocol = static_cast<AittProtocol>(protocol);
    AITT::QoS _qos = static_cast<AITT::QoS>(qos);
    bool _retain = (bool)retain;

    try {
        instance->aitt.Publish(customTopic, _data, num_bytes, _protocol,
                               _qos, _retain);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to publish");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
    env->ReleaseByteArrayElements(data, reinterpret_cast<jbyte *>((char *)cdata), 0);
}

void AittNativeInterface::Java_com_samsung_android_aitt_Aitt_disconnectJNI(JNIEnv *env, jobject jniInterfaceObject, jlong handle)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or Jobject is null");
        return;
    }
    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(handle);
    try {
        instance->aitt.Disconnect();
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to disconnect");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

jlong AittNativeInterface::Java_com_samsung_android_aitt_Aitt_subscribeJNI(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                                             jstring topic, jint protocol, jint qos)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or Jobject is null");
        return 0L;
    }
    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(handle);
    std::string customTopic = GetStringUTF(env, topic);
    if (customTopic.empty()) {
        return 0L;
    }

    AittProtocol _protocol = static_cast<AittProtocol>(protocol);
    AITT::QoS _qos = static_cast<AITT::QoS>(qos);

    AittSubscribeID _id = nullptr;
    try {
        _id = instance->aitt.Subscribe(
                customTopic,
                [&](aitt::MSG *handle, const void *msg, const int szmsg, void *cbdata) -> void {
                    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(cbdata);
                    JNIEnv* env;
                    int JNIStatus = cbContext.jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
                    if (JNIStatus == JNI_EDETACHED) {
                        if (cbContext.jvm->AttachCurrentThread(&env, nullptr) != 0) {
                            JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to attach current thread");
                            return;
                        }
                    } else if (JNIStatus == JNI_EVERSION) {
                        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Unsupported version");
                        return;
                    }
                    if (env != nullptr && instance->cbObject != nullptr) {
                        jstring _topic = env->NewStringUTF(handle->GetTopic().c_str());
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

                        env->CallVoidMethod(instance->cbObject, cbContext.messageCallbackMethodID, _topic, array);
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
    return (jlong)_id;
}

void AittNativeInterface::Java_com_samsung_android_aitt_Aitt_unsubscribeJNI(JNIEnv *env, jobject jniInterfaceObject,
                                              jlong handle, jlong aittSubId)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or Jobject is null");
        return;
    }
    AittNativeInterface *instance = reinterpret_cast<AittNativeInterface *>(handle);
    void *subId = reinterpret_cast<void *>(aittSubId);
    try {
        instance->aitt.Unsubscribe(static_cast<AittSubscribeID>(subId));
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to unsubscribe");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
    }
}

jlong AittNativeInterface::Java_com_samsung_android_aitt_Aitt_initJNI(JNIEnv *env, jobject jniInterfaceObject,
                                        jstring id, jstring ip, jboolean clearSession)
{
    if (env == nullptr || jniInterfaceObject == nullptr) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Env or Jobject is null");
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
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Failed to create new instance");
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
        return 0L;
    }

    if (env->GetJavaVM(&cbContext.jvm) != JNI_OK) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Unable to get Java VM");
        delete instance;
        instance = nullptr;
        return 0L;
    }
    try {
        instance->cbObject = env->NewGlobalRef(jniInterfaceObject);

        jclass callbackClass = env->FindClass("com/samsung/android/aitt/Aitt");
        cbContext.messageCallbackMethodID = env->GetMethodID(callbackClass, "messageCallback",
                                                               "(Ljava/lang/String;[B)V");
        env->DeleteLocalRef(callbackClass);
    } catch (std::exception &e) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, e.what());
        delete instance;
        instance = nullptr;
        return 0L;
    }

    return reinterpret_cast<long>(instance);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **)(&env), JNI_VERSION_1_6) != JNI_OK) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "Not a valid JNI version");
        return JNI_ERR;
    }
    jclass klass = env->FindClass("com/samsung/android/aitt/Aitt");
    if (nullptr == klass) {
        JNI_LOG(ANDROID_LOG_ERROR, TAG, "klass is null");
        return JNI_ERR;
    }
    static JNINativeMethod aitt_jni_methods[] = {
            {"initJNI", "(Ljava/lang/String;Ljava/lang/String;Z)J",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_initJNI)},
            {"connectJNI", "(JLjava/lang/String;I)V",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_connectJNI)},
            {"subscribeJNI", "(JLjava/lang/String;II)J",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_subscribeJNI)},
            {"publishJNI", "(JLjava/lang/String;[BJIIZ)V",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_publishJNI)},
            {"unsubscribeJNI", "(JJ)V",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_unsubscribeJNI)},
            {"disconnectJNI", "(J)V",
                    reinterpret_cast<void *>(AittNativeInterface::Java_com_samsung_android_aitt_Aitt_disconnectJNI)}};
    if (env->RegisterNatives(klass, aitt_jni_methods,
                             sizeof(aitt_jni_methods) / sizeof(aitt_jni_methods[0]))) {
        env->DeleteLocalRef(klass);
        return JNI_ERR;
    }
    env->DeleteLocalRef(klass);
    JNI_LOG(ANDROID_LOG_INFO, TAG, "JNI loaded successfully");
    return JNI_VERSION_1_6;
}
