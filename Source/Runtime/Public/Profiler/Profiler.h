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

#pragma once

#include "Containers/StaticArray.h"
#include "Containers/HashMap.h"
#include "Containers/Stack.h"
#include "Math/Math.h"
#include "RHI/RHIOpenGL.h"

BE_NAMESPACE_BEGIN

#ifdef ENABLE_PROFILER
    #define BE_PROFILE_INIT()                               BE1::profiler.Init()
    #define BE_PROFILE_SHUTDOWN()                           BE1::profiler.Shutdown()
    #define BE_PROFILE_SYNC_FRAME()                         BE1::profiler.SyncFrame()
    #define BE_PROFILE_STOP()                               BE1::profiler.SetFreeze(true)
    #define BE_PROFILE_START()                              BE1::profiler.SetFreeze(false)

    #define BE_PROFILE_TAG(var)                             CONCAT(profile_tag_, var)
    #define BE_PROFILE_DEFINE_TAG(var, name, color)         int BE_PROFILE_TAG(var) = BE1::profiler.CreateTag(name, color)
    #define BE_PROFILE_DECLARE_TAG(var)                     extern int BE_PROFILE_TAG(var)

    #define BE_PROFILE_CPU_SCOPE(var)                       BE1::ScopeProfileCPU CONCAT(profile_scope_cpu_, __LINE__)(BE_PROFILE_TAG(var))
    #define BE_PROFILE_GPU_SCOPE(var)                       BE1::ScopeProfileGPU CONCAT(profile_scope_gpu_, __LINE__)(BE_PROFILE_TAG(var))

    #define BE_PROFILE_CPU_SCOPE_STATIC_BASE(name, color)   static BE_PROFILE_DEFINE_TAG(CONCAT(cpu_, __LINE__), name, color); BE_PROFILE_CPU_SCOPE(CONCAT(cpu_, __LINE__))
    #define BE_PROFILE_CPU_SCOPE_STATIC_ARG1(name)          BE_PROFILE_CPU_SCOPE_STATIC_BASE(name, Color3::lightGray)
    #define BE_PROFILE_CPU_SCOPE_STATIC_ARG2(name, color)   BE_PROFILE_CPU_SCOPE_STATIC_BASE(name, color)
    #define BE_PROFILE_CPU_SCOPE_STATIC(...)                OVERLOADED_MACRO(BE_PROFILE_CPU_SCOPE_STATIC, __VA_ARGS__)

    #define BE_PROFILE_GPU_SCOPE_STATIC_BASE(name, color)   static BE_PROFILE_DEFINE_TAG(CONCAT(gpu_, __LINE__), name, color); BE_PROFILE_GPU_SCOPE(CONCAT(gpu_, __LINE__))
    #define BE_PROFILE_GPU_SCOPE_STATIC_ARG1(name)          BE_PROFILE_GPU_SCOPE_STATIC_BASE(name, Color3::lightGray)
    #define BE_PROFILE_GPU_SCOPE_STATIC_ARG2(name, color)   BE_PROFILE_GPU_SCOPE_STATIC_BASE(name, color)
    #define BE_PROFILE_GPU_SCOPE_STATIC(...)                OVERLOADED_MACRO(BE_PROFILE_GPU_SCOPE_STATIC, __VA_ARGS__)
#else
    #define BE_PROFILE_INIT()
    #define BE_PROFILE_SHUTDOWN()
    #define BE_PROFILE_SYNC_FRAME()
    #define BE_PROFILE_STOP()
    #define BE_PROFILE_START()
    #define BE_PROFILE_TAG(var)
    #define BE_PROFILE_DEFINE_TAG(var, name, color)
    #define BE_PROFILE_DECLARE_TAG(var)
    #define BE_PROFILE_SCOPE_CPU(var)
    #define BE_PROFILE_SCOPE_GPU(var)
    #define BE_PROFILE_CPU_SCOPE_STATIC(...)
    #define BE_PROFILE_GPU_SCOPE_STATIC(...)
#endif

class Profiler {
public:
    static const int            MaxRecordedFrames = 6;
    static const int            MaxTags = 1024;
    static const int            MaxCpuThreads = 32;
    static const int            MaxDepth = 32;

    static const uint64_t       InvalidTime = -1;

    static const int            MaxCpuMarkersPerFrame = 128;
    static const int            MaxCpuMarkersPerThread = MaxRecordedFrames * MaxCpuMarkersPerFrame;

    static const int            MaxGpuMarkersPerFrame = 128;
    static const int            MaxGpuMarkers = MaxRecordedFrames * MaxGpuMarkersPerFrame;

    static const int            MaxTagNameLength = 64;

    struct FreezeState {
        enum Enum {
            Unfrozen,
            Frozen,
            WaitingForFreeze,
            WaitingForUnfreeze
        };
    };

    struct Tag {
        char                    name[MaxTagNameLength];
        Color3                  color;
    };

    struct MarkerBase {
        int                     tagIndex;
        int                     stackDepth;                         // marker stack depth
        int                     frameCount;
    };

    struct CpuMarker : public MarkerBase {
        uint64_t                startTime;
        uint64_t                endTime;
    };

    struct GpuMarker : public MarkerBase {
        RHI::Handle             startQueryHandle;
        RHI::Handle             endQueryHandle;
    };

    struct CpuThreadInfo {
        uint64_t                threadId;
        CpuMarker               markers[MaxCpuMarkersPerThread];
        int                     currentIndex;                       // current marker index
        Stack<int>              indexStack;                         // marker index stack for recursive usage
        int                     frameIndexes[MaxRecordedFrames];    // start marker indexes for frames

        CpuThreadInfo() : indexStack(MaxDepth), currentIndex(0) {}
    };

    struct GpuThreadInfo {
        GpuMarker               markers[MaxGpuMarkers];
        int                     currentIndex;                       // current marker index
        Stack<int>              indexStack;                         // marker index stack for recursive usage
        int                     frameIndexes[MaxRecordedFrames];    // start marker indexes for frames

        GpuThreadInfo() : indexStack(MaxDepth), currentIndex(0) {}
    };

    struct FrameData {
        int                     frameCount;
        uint64_t                time;
    };

    void                        Init();
    void                        Shutdown();

                                // Call SyncFrame() on the starting frame.
    void                        SyncFrame();

    bool                        IsFrozen() const;
    bool                        IsUnfrozen() const;

    bool                        SetFreeze(bool freeze);

    template <typename Func>
    void                        IterateCpuMarkers(uint64_t threadId, Func func) const;

    template <typename Func>
    void                        IterateGpuMarkers(Func func) const;

    int                         CreateTag(const char *name, const Color3 &color);

    void                        PushCpuMarker(int tagIndex);
    void                        PopCpuMarker();

    void                        PushGpuMarker(int tagIndex);
    void                        PopGpuMarker();

private:
    CpuThreadInfo &             GetCpuThreadInfo();

    FreezeState::Enum           freezeState = FreezeState::Frozen;

    FrameData                   frameData[MaxRecordedFrames];
    int                         frameCount;                     ///< Incremental value for each SyncFrame() calls.
    int                         writeFrameIndex;
    int                         readFrameIndex;

    StaticArray<Tag, MaxTags>   tags;                           ///< tag informations.
    HashMap<uint64_t, CpuThreadInfo> cpuThreadInfoMap;
    GpuThreadInfo               gpuThreadInfo;
};

template <typename Func>
BE_INLINE void Profiler::IterateCpuMarkers(uint64_t threadId, Func func) const {
    if (readFrameIndex < 0) {
        return;
    }

    const FrameData &readFrame = frameData[readFrameIndex];

    if (readFrame.time != InvalidTime) {
        for (int i = 0; i < cpuThreadInfoMap.Count(); i++) {
            const CpuThreadInfo &ti = cpuThreadInfoMap.GetByIndex(i)->second;
            if (ti.threadId != threadId) {
                continue;
            }

            int startMarkerIndex = ti.frameIndexes[readFrameIndex];
            int endMarkerIndex = ti.frameIndexes[(readFrameIndex + 1) % COUNT_OF(ti.frameIndexes)];

            int markerIndex = startMarkerIndex;

            int skipMinDepth = INT_MAX;

            while (markerIndex != endMarkerIndex) {
                const CpuMarker &marker = ti.markers[markerIndex];

                int nextMarkerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
                bool isLeaf = nextMarkerIndex == endMarkerIndex || ti.markers[nextMarkerIndex].stackDepth <= marker.stackDepth;

                if (marker.stackDepth < skipMinDepth) {
                    const Tag &tag = tags[marker.tagIndex];

                    if (!func(tag.name, tag.color, marker.stackDepth, isLeaf, marker.startTime, marker.endTime)) {
                        skipMinDepth = marker.stackDepth + 1;
                    } else {
                        skipMinDepth = INT_MAX;
                    }
                }

                markerIndex = nextMarkerIndex;
            }
        }
    }
}

template <typename Func>
BE_INLINE void Profiler::IterateGpuMarkers(Func func) const {
    if (readFrameIndex < 0) {
        return;
    }

    const FrameData &readFrame = frameData[readFrameIndex];

    if (readFrame.time != InvalidTime) {
        const GpuThreadInfo &ti = gpuThreadInfo;

        int startMarkerIndex = ti.frameIndexes[readFrameIndex];
        int endMarkerIndex = ti.frameIndexes[(readFrameIndex + 1) % COUNT_OF(ti.frameIndexes)];

        int markerIndex = startMarkerIndex;

        int skipMinDepth = INT_MAX;

        while (markerIndex != endMarkerIndex) {
            const GpuMarker &marker = ti.markers[markerIndex];

            int nextMarkerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
            bool isLeaf = nextMarkerIndex == endMarkerIndex || ti.markers[nextMarkerIndex].stackDepth <= marker.stackDepth;

            if (marker.stackDepth < skipMinDepth) {
                if (!rhi.QueryResultAvailable(marker.startQueryHandle) || !rhi.QueryResultAvailable(marker.endQueryHandle)) {
                    continue;
                }
                uint64_t startTime = rhi.QueryResult(marker.startQueryHandle);
                uint64_t endTime = rhi.QueryResult(marker.endQueryHandle);

                const Tag &tag = tags[marker.tagIndex];

                if (!func(tag.name, tag.color, marker.stackDepth, isLeaf, startTime, endTime)) {
                    skipMinDepth = marker.stackDepth + 1;
                } else {
                    skipMinDepth = INT_MAX;
                }
            }

            markerIndex = (markerIndex + 1) % COUNT_OF(ti.markers);
        }
    }
}

extern Profiler profiler;

class ScopeProfileCPU {
public:
    ScopeProfileCPU(int tagIndex) { profiler.PushCpuMarker(tagIndex); }
    ~ScopeProfileCPU() { profiler.PopCpuMarker(); }
};

class ScopeProfileGPU {
public:
    ScopeProfileGPU(int tagIndex) { profiler.PushGpuMarker(tagIndex); }
    ~ScopeProfileGPU() { profiler.PopGpuMarker(); }
};

BE_NAMESPACE_END
