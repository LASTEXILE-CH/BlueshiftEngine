#include "Precompiled.h"
#include "Math/Math.h"
#include "SIMD/SIMD.h"
#include "Core/CVars.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

static const int MaxSources = 12;

static CVar     s_khz("s_khz", "44", CVar::Flag::Integer | CVar::Flag::Archive, "");
static CVar     s_doppler("s_doppler", "1.0", CVar::Flag::Float | CVar::Flag::Archive, "");
static CVar     s_rolloff("s_rolloff", "2.0", CVar::Flag::Float | CVar::Flag::Archive, "");

bool SoundSystem::InitDevice(void *windowHandle) {
    BE_LOG("Initializing OpenSL ES...\n");

    // Create the SL engine object
    SLresult result = slCreateEngine(&slEngineObject, 0, nullptr, 0, nullptr, nullptr);
    assert(SL_RESULT_SUCCESS == result);

    // Realize the SL engine object
    result = (*slEngineObject)->Realize(slEngineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // Get the SL engine interface, which is needed in order to create other objects
    result = (*slEngineObject)->GetInterface(slEngineObject, SL_IID_ENGINE, &slEngine);
    assert(SL_RESULT_SUCCESS == result);

    // Create output mix object
    result = (*slEngine)->CreateOutputMix(slEngine, &slOutputMixObject, 0, nullptr, nullptr);
    assert(SL_RESULT_SUCCESS == result);

    // Realize the output mix object
    result = (*slOutputMixObject)->Realize(slOutputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

#if 0
    // Create listener object
    const SLInterfaceID listener_ids[] = { SL_IID_3DLOCATION, SL_IID_3DSOURCE };
    const SLboolean listener_req[] = { SL_BOOLEAN_TRUE,  SL_BOOLEAN_TRUE };
    result = (*slEngine)->CreateListener(slEngine, &slListenerObject, 1, listener_ids, listener_req);
    assert(SL_RESULT_SUCCESS == result);

    // Realize the listener object
    result = (*slListenerObject)->Realize(slListenerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
#endif

    // Initialize sources
    for (int sourceIndex = 0; sourceIndex < MaxSources; sourceIndex++) {
        SoundSource *source = new SoundSource;
        sources.Append(source);
        freeSources.Append(source);
    }

    return true;
}

void SoundSystem::ShutdownDevice() {
    BE_LOG("Shutting down OpenSL ES...\n");

    // Delete sources
    for (int sourceIndex = 0; sourceIndex < sources.Count(); sourceIndex++) {
        SoundSource *source = sources[sourceIndex];
        delete source;
    }

    sources.Clear();
    freeSources.Clear();

    // Destroy output mix object, and invalidate all associated interfaces
    if (slOutputMixObject) {
        (*slOutputMixObject)->Destroy(slOutputMixObject);
        slOutputMixObject = nullptr;
    }

    // Destroy engine object, and invalidate all associated interfaces
    if (slEngineObject) {
        (*slEngineObject)->Destroy(slEngineObject);
        slEngineObject = nullptr;
        slEngine = nullptr;
    }
}

void SoundSystem::PlaceListenerInternal(const Vec3 &pos, const Vec3 &forward, const Vec3 &up) {
}

BE_NAMESPACE_END