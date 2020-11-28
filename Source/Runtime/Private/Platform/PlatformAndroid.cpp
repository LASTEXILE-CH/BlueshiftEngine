// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Core/Str.h"
#include "PlatformGeneric.h"
#include "PlatformAndroid.h"
#include "Platform/PlatformSystem.h"
#include "PlatformUtils/Android/AndroidJNI.h"
#include <android/log.h>

BE_NAMESPACE_BEGIN

PlatformAndroid::PlatformAndroid() {
    window = nullptr;
}

void PlatformAndroid::Init() {
    PlatformGeneric::Init();
}

void PlatformAndroid::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformAndroid::SetMainWindowHandle(void *windowHandle) {
	window = (ANativeWindow *)windowHandle;
}

void PlatformAndroid::Quit() {
    exit(EXIT_SUCCESS);
}

void PlatformAndroid::Log(const char *msg) {
    __android_log_print(ANDROID_LOG_INFO, "Blueshift", "%s", msg);
}

void PlatformAndroid::Error(const char *msg) {
    JNIEnv *env = AndroidJNI::GetJavaEnv();

    jstring javaMsg = Str(msg).ToJavaString(env);
    AndroidJNI::CallVoidMethod(env, AndroidJNI::activity->clazz, AndroidJNI::javaMethod_showAlert, javaMsg);

    env->DeleteLocalRef(javaMsg);

    PlatformSystem::DebugBreak();

    Quit();
}

BE_NAMESPACE_END
