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

#include <AITT.h>
#include <AittDiscovery.h>

#include <android/log.h>
#include <jni.h>
#include <string>

#define TAG "AITT_ANDROID_JNI"
#define JNI_LOG(a, b, c) __android_log_write(a, b, c)

using AITT = aitt::AITT;
using AittDiscovery = aitt::AittDiscovery;

class AittNativeInterface {
private:
    struct CallbackContext {
        JavaVM *jvm;
        jmethodID messageCallbackMethodID;
        jmethodID connectionCallbackMethodID;
        jmethodID discoveryCallbackMethodID;
    };

private:
    AittNativeInterface(std::string &mqId, std::string &ip, bool clearSession);

    virtual ~AittNativeInterface(void);

    void DiscoveryMessageCallback(const std::string &clientId, const std::string &status,
                                  const void *msg, const int szmsg);

    static std::string GetStringUTF(JNIEnv *env, jstring str);

    static bool CheckParams(JNIEnv *env, jobject jniInterfaceObject);

    static bool JniStatusCheck(JNIEnv *&env, int JNIStatus);

public:
    static jlong Init(JNIEnv *env, jobject jniInterfaceObject,
                      jstring id, jstring ip, jboolean clearSession);

    static void Connect(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                        jstring host, jint port);

    static jlong Subscribe(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                           jstring topic, jint protocol, jint qos);

    static void Publish(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                        jstring topic, jbyteArray data, jlong datalen, jint protocol,
                        jint qos, jboolean retain);

    static void Unsubscribe(JNIEnv *env, jobject jniInterfaceObject, jlong handle,
                            jlong aittSubId);

    static void Disconnect(JNIEnv *env, jobject jniInterfaceObject, jlong handle);

    static void SetConnectionCallback(JNIEnv *env, jobject jniInterfaceObject, jlong handle);

    static int SetDiscoveryCallback(JNIEnv *env, jobject jniInterfaceObject, jlong handle, jstring topic);

    static void RemoveDiscoveryCallback(JNIEnv *env, jobject jniInterfaceObject, jlong handle, jint cbHandle);

private:
    AITT aitt;
    AittDiscovery *discovery;
    jobject cbObject;
    static CallbackContext cbContext;
};
