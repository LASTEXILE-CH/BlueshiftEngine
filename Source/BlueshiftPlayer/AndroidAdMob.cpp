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
#include "Application.h"
#include "android_native_app_glue.h"
#include "AndroidAdMob.h"

RewardBasedVideoAd rewardBasedVideoAd;

static jmethodID javaMethod_initializeAds = nullptr;
static jmethodID javaMethod_requestRewardedVideoAd = nullptr;
static jmethodID javaMethod_isRewardedVideoAdLoaded = nullptr;
static jmethodID javaMethod_showRewardedVideoAd = nullptr;

void RewardBasedVideoAd::RegisterLuaModule(LuaCpp::State *state) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jclass javaClassActivity = env->GetObjectClass(BE1::AndroidJNI::activity->clazz);

    javaMethod_initializeAds = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "initializeAds", "(Ljava/lang/String;)V", false);
    javaMethod_requestRewardedVideoAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "requestRewardedVideoAd", "(Ljava/lang/String;[Ljava/lang/String;)V", false);
    javaMethod_isRewardedVideoAdLoaded = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "isRewardedVideoAdLoaded", "()Z", false);
    javaMethod_showRewardedVideoAd = BE1::AndroidJNI::FindMethod(env, javaClassActivity, "showRewardedVideoAd", "()V", false);

    env->DeleteLocalRef(javaClassActivity);

    state->RegisterModule("admob", [](LuaCpp::Module &module) {
        LuaCpp::Selector _RewardBasedVideoAd = module["RewardBasedVideoAd"];
        
        _RewardBasedVideoAd.SetObj(rewardBasedVideoAd);
        _RewardBasedVideoAd.AddObjMembers(rewardBasedVideoAd,                                        
            "init", &RewardBasedVideoAd::Init,
            "request", &RewardBasedVideoAd::Request,
            "is_ready", &RewardBasedVideoAd::IsReady,
            "present", &RewardBasedVideoAd::Present);
    });
}

void RewardBasedVideoAd::Init(const char *appID) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jstring javaAppID = BE1::Str(appID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_initializeAds, javaAppID);

    env->DeleteLocalRef(javaAppID);
}

void RewardBasedVideoAd::Request(const char *unitID, const char *testDevices) {
    if (!unitID || !unitID[0]) {
        // Google-provided test ad units
        unitID = "ca-app-pub-3940256099942544/1712485313";
    }

    BE1::StrArray testDeviceList;
    BE1::SplitStringIntoList(testDeviceList, testDevices, " ");

    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jobjectArray javaTestDevices = (jobjectArray)env->NewObjectArray(testDeviceList.Count(), env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < testDeviceList.Count(); i++) {
        env->SetObjectArrayElement(javaTestDevices, i, env->NewStringUTF(testDeviceList[i].c_str()));
    }

    jstring javaUnitID = BE1::Str(unitID).ToJavaString(env);

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_requestRewardedVideoAd, javaUnitID, javaTestDevices);

    env->DeleteLocalRef(javaUnitID);
}
    
bool RewardBasedVideoAd::IsReady() const {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    return BE1::AndroidJNI::CallBooleanMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_isRewardedVideoAdLoaded);
}

void RewardBasedVideoAd::Present() {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, javaMethod_showRewardedVideoAd);
}

extern "C" {

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidRewardUser(JNIEnv *env, jobject thiz, jstring javaType, jint javaAmount) { 
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_reward_user"];
    if (function.IsFunction()) {
        const char *type = env->GetStringUTFChars(javaType, nullptr);
        function(type, (int)javaAmount);
        env->ReleaseStringUTFChars(javaType, type);
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidReceiveAd(JNIEnv *env, jobject thiz) { 
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_receive_ad"];
    if (function.IsFunction()) {
        function();
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidOpen(JNIEnv *env, jobject thiz) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_open"];
    if (function.IsFunction()) {
        function();
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidStartPlaying(JNIEnv *env, jobject thiz) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_start_playing"];
    if (function.IsFunction()) {
        function();
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidClose(JNIEnv *env, jobject thiz) { 
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_close"];
    if (function.IsFunction()) {
        function();
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdWillLeaveApplication(JNIEnv *env, jobject thiz) { 
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["will_leave_application"];
    if (function.IsFunction()) {
        function();
    }
}

JNIEXPORT void JNICALL Java_com_polygontek_BlueshiftPlayer_GameAdMobActivity_rewardBasedVideoAdDidFailToLoad(JNIEnv *env, jobject thiz, jstring javaErrorMessage) {
    LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_fail_to_load"];
    if (function.IsFunction()) {
        const char *errorMessage = env->GetStringUTFChars(javaErrorMessage, nullptr);
        function(errorMessage);
        env->ReleaseStringUTFChars(javaErrorMessage, errorMessage);
    }
}

}