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

BE_NAMESPACE_BEGIN

class Mat3x4;
class Skeleton;
class Batch;

// Joint cache for HW skinning
class SkinningJointCache {
    friend class Batch;

public:
    struct SkinningMethod {
        enum Enum {
            Cpu,
            VertexShader,
            VertexTextureFetch
        };
    };

    SkinningJointCache() = delete;
    SkinningJointCache(int numJoints);
    ~SkinningJointCache();

    void                Purge();

    const BufferCache & GetBufferCache() const { return bufferCache; }

    void                Update(const Skeleton *skeleton, const Mat3x4 *jointMats);

    static bool         CapableGPUJointSkinning(SkinningMethod::Enum skinningMethod, int numJoints);

private:
    int                 numJoints;              // If motion blur is used, use twice the original model joints.
    Mat3x4 *            skinningJoints;         // Result matrix for animation.
    int                 jointIndexOffset[2];    // Current/Previous frame joint index offset for motion blur.
    BufferCache         bufferCache;            // Use for VTF skinning.
    int                 viewFrameCount;         // Marking number to indicate that the calculation has been completed in the current frame.
};

BE_INLINE SkinningJointCache::~SkinningJointCache() {
    Purge();
}

BE_NAMESPACE_END
