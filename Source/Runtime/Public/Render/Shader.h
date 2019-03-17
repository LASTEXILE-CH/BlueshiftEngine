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

/*
-------------------------------------------------------------------------------

    Shader

-------------------------------------------------------------------------------
*/

#include "Containers/HashMap.h"
#include "Math/Math.h"
#include "Core/CmdArgs.h"
#include "Core/Property.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

class Texture;

class Shader {
    friend class ShaderManager;
    friend class Batch;

public:
    enum Flag {
        Shadowing               = BIT(0),
        LitSurface              = BIT(1),
        SkySurface              = BIT(2),
        LoadedFromFile          = BIT(8)
    };

    enum Lang {
        GlslShader,
        CgShader
    };

    enum ShaderFlag {
        VertexShader            = BIT(0),
        FragmentShader          = BIT(1),
        GeometryShader          = BIT(2)
    };

    enum PropertyType {
        BoolType,
        IntType,
        FloatType,
        Vec2Type,
        Vec3Type,
        Vec4Type,
        Color3Type,
        Color4Type,
        Texture2DType,
        Texture3DType,
        TextureCubeType,
    };

    enum BuiltInConstant {
        ModelViewMatrixConst,
        ModelViewMatrixTransposeConst,
        ViewMatrixConst,
        ViewMatrixTransposeConst,
        ProjectionMatrixConst,
        ProjectionMatrixTransposeConst,
        ViewProjectionMatrixConst,
        ViewProjectionMatrixTransposeConst,
        ModelViewProjectionMatrixConst,
        ModelViewProjectionMatrixTransposeConst,
        InstanceIndexesConst,
        LocalToWorldMatrixConst,
        WorldToLocalMatrixConst,
        TextureMatrixSConst,
        TextureMatrixTConst,
        ConstantColorConst,
        VertexColorScaleConst,
        VertexColorAddConst,
        PerforatedAlphaConst,
        ViewOriginConst,
        LightVecConst,
        LightTextureMatrixConst,
        LightColorConst,
        LightFallOffMatrixConst,
        LightFallOffExponentConst,
        JointsConst,
        InvJointsMapSizeConst,
        SkinningBaseTcConst,
        JointIndexOffsetConst,
        ShadowProjMatrixConst,
        ShadowCascadeProjMatrixConst,
        ShadowSplitFarConst,
        Probe0SpecularCubeMapMaxMipLevelConst,
        Probe0PositionConst,
        Probe0MinsConst,
        Probe0MaxsConst,
        Probe1SpecularCubeMapMaxMipLevelConst,
        Probe1PositionConst,
        Probe1MinsConst,
        Probe1MaxsConst,
        MaxBuiltInConstants
    };

    enum BuiltInSampler {
        CubicNormalCubeMapSampler,
        IndirectionCubeMapSampler,
        AlbedoMapSampler,
        NormalMapSampler,
        JointsMapSampler,
        LightProjectionMapSampler,
        ShadowMapSampler,
        ShadowArrayMapSampler,
        Probe0DiffuseCubeMapSampler,
        Probe0SpecularCubeMapSampler,
        Probe1DiffuseCubeMapSampler,
        Probe1SpecularCubeMapSampler,
        MaxBuiltInSamplers
    };

    struct Define {
        Define() {}
        Define(const char *name, int value) : name(name), value(value) {}
        Define &operator=(const Define &rhs) { this->name = rhs.name; this->value = rhs.value; return *this;  }

        Str                 name;
        int                 value;
    };

    struct Property {
        Variant             data;
        Texture *           texture;
    };

    Shader();
    ~Shader();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    bool                    HasVertexShader() const { return !!(shaderFlags & VertexShader); }
    bool                    HasFragmentShader() const { return !!(shaderFlags & FragmentShader); }
    bool                    HasGeometryShader() const { return !!(shaderFlags & GeometryShader); }

    bool                    IsOriginalShader() const { return !originalShader; }
    bool                    IsInstantiatedShader() const { return !!originalShader; }

    int                     GetFlags() const;

                            /// Create instantiated shader.
    Shader *                InstantiateShader(const Array<Define> &defineArray);

                            /// Reinstantiate itself.
    void                    Reinstantiate();

                            /// Returns original shader. Instantiated shader has it's original shader
    Shader *                GetOriginalShader() const { return originalShader; }

    Shader *                GetPerforatedVersion();
    Shader *                GetPremulAlphaVersion();
    Shader *                GetIndirectLitVersion();
    Shader *                GetDirectLitVersion();
    Shader *                GetIndirectLitDirectLitVersion();
    Shader *                GetParallelShadowVersion();
    Shader *                GetSpotShadowVersion();
    Shader *                GetPointShadowVersion();
    Shader *                GetGPUSkinningVersion(int index);
    Shader *                GetGPUInstancingVersion();

    void                    Bind() const;

                            /// Returns constant index with the given name.
    int                     GetConstantIndex(const char *name) const;

                            /// Returns constant block index with the given name.
    int                     GetConstantBlockIndex(const char *name) const;

                            /// Sets constant of integer type.
    void                    SetConstant1i(int index, const int constant) const;
    void                    SetConstant2i(int index, const int *constant) const;
    void                    SetConstant3i(int index, const int *constant) const;
    void                    SetConstant4i(int index, const int *constant) const;
    void                    SetConstant1i(const char *name, const int constant) const;
    void                    SetConstant2i(const char *name, const int *constant) const;
    void                    SetConstant3i(const char *name, const int *constant) const;
    void                    SetConstant4i(const char *name, const int *constant) const;

                            /// Sets constant of float type.
    void                    SetConstant1f(int index, const float constant) const;
    void                    SetConstant2f(int index, const float *constant) const;
    void                    SetConstant3f(int index, const float *constant) const;
    void                    SetConstant4f(int index, const float *constant) const;
    void                    SetConstant2f(int index, const Vec2 &constant) const;
    void                    SetConstant3f(int index, const Vec3 &constant) const;
    void                    SetConstant4f(int index, const Vec4 &constant) const;
    void                    SetConstant1f(const char *name, const float constant) const;
    void                    SetConstant2f(const char *name, const float *constant) const;
    void                    SetConstant3f(const char *name, const float *constant) const;
    void                    SetConstant4f(const char *name, const float *constant) const;
    void                    SetConstant2f(const char *name, const Vec2 &constant) const;
    void                    SetConstant3f(const char *name, const Vec3 &constant) const;
    void                    SetConstant4f(const char *name, const Vec4 &constant) const;

                            /// Sets matrix constant.
    void                    SetConstant2x2f(int index, bool rowMajor, const Mat2 &constant) const;
    void                    SetConstant3x3f(int index, bool rowMajor, const Mat3 &constant) const;
    void                    SetConstant4x4f(int index, bool rowMajor, const Mat4 &constant) const;
    void                    SetConstant4x3f(int index, bool rowMajor, const Mat3x4 &constant) const;
    void                    SetConstant2x2f(const char *name, bool rowMajor, const Mat2 &constant) const;
    void                    SetConstant3x3f(const char *name, bool rowMajor, const Mat3 &constant) const;
    void                    SetConstant4x4f(const char *name, bool rowMajor, const Mat4 &constant) const;
    void                    SetConstant4x3f(const char *name, bool rowMajor, const Mat3x4 &constant) const;

                            /// Sets array constant of integer type.
    void                    SetConstantArray1i(int index, int num, const int *constant) const;
    void                    SetConstantArray2i(int index, int num, const int *constant) const;
    void                    SetConstantArray3i(int index, int num, const int *constant) const;
    void                    SetConstantArray4i(int index, int num, const int *constant) const;
    void                    SetConstantArray1i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray2i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray3i(const char *name, int num, const int *constant) const;
    void                    SetConstantArray4i(const char *name, int num, const int *constant) const;

                            /// Sets array constant of float type.
    void                    SetConstantArray1f(int index, int num, const float *constant) const;
    void                    SetConstantArray2f(int index, int num, const float *constant) const;
    void                    SetConstantArray3f(int index, int num, const float *constant) const;
    void                    SetConstantArray4f(int index, int num, const float *constant) const;
    void                    SetConstantArray2f(int index, int num, const Vec2 *constant) const;
    void                    SetConstantArray3f(int index, int num, const Vec3 *constant) const;
    void                    SetConstantArray4f(int index, int num, const Vec4 *constant) const;
    void                    SetConstantArray1f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray2f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray3f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray4f(const char *name, int num, const float *constant) const;
    void                    SetConstantArray2f(const char *name, int num, const Vec2 *constant) const;
    void                    SetConstantArray3f(const char *name, int num, const Vec3 *constant) const;
    void                    SetConstantArray4f(const char *name, int num, const Vec4 *constant) const;

                            /// Sets matrix array constant.
    void                    SetConstantArray2x2f(int index, bool rowMajor, int num, const Mat2 *constant) const;
    void                    SetConstantArray3x3f(int index, bool rowMajor, int num, const Mat3 *constant) const;
    void                    SetConstantArray4x4f(int index, bool rowMajor, int num, const Mat4 *constant) const;
    void                    SetConstantArray4x3f(int index, bool rowMajor, int num, const Mat3x4 *constant) const;
    void                    SetConstantArray2x2f(const char *name, bool rowMajor, int num, const Mat2 *constant) const;
    void                    SetConstantArray3x3f(const char *name, bool rowMajor, int num, const Mat3 *constant) const;
    void                    SetConstantArray4x4f(const char *name, bool rowMajor, int num, const Mat4 *constant) const;
    void                    SetConstantArray4x3f(const char *name, bool rowMajor, int num, const Mat3x4 *constant) const;

                            /// Sets constant buffer.
    void                    SetConstantBuffer(int index, int bindingIndex) const;
    void                    SetConstantBuffer(const char *name, int bindingIndex) const;

                            /// Returns sampler unit index with the given name.
    int                     GetSamplerUnit(const char *name) const;

                            /// Sets texture.
    void                    SetTexture(int unit, const Texture *texture) const;
    void                    SetTexture(const char *name, const Texture *texture) const;

                            /// Sets texture array.
    void                    SetTextureArray(const char *name, int num, const Texture **textures) const;

    bool                    Create(const char *text, const char *baseDir);

    const Shader *          AddRefCount() const { refCount++; return this; }
    int                     GetRefCount() const { return refCount; }

    void                    Purge();

    bool                    Load(const char *filename);
    bool                    Reload();

    const StrHashMap<PropertyInfo> &GetPropertyInfoHashMap() const;

private:
    bool                    ParseProperties(Lexer &lexer);
    Shader *                GenerateSubShader(const Str &shaderNamePostfix, const Str &vsHeaderText, const Str &fsHeaderText, bool shadowing, int skinningWeightCount, bool instancing);
    bool                    GenerateGpuSkinningVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText, bool shadowing, bool instancing);
    bool                    GeneratePerforatedVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText, bool shadowing, bool genGpuSkinningVersion, bool genGpuInstancingVersion);
    bool                    GeneratePremulAlphaVersion(Shader *shader, const Str &shaderNamePrefix, const Str &vpText, const Str &fpText, bool shadowing, bool genGpuSkinningVersion, bool genGpuInstancingVersion);
    bool                    InstantiateShaderInternal(const Array<Define> &defineArray);

    bool                    Finish(bool genPerforatedVersion, bool genGpuSkinningVersion, bool genGpuInstancingVersion, bool genParallelShadowVersion, bool genSpotShadowVersion, bool genPointShadowVersion);
    bool                    ProcessShaderText(const char *text, const char *baseDir, const Array<Define> &defineArray, Str &outStr) const;
    bool                    ProcessIncludeRecursive(const char *baseDir, Str &text) const;

    static const char *     MangleNameWithDefineList(const Str &basename, const Array<Shader::Define> &defineArray, Str &mangledName);

    Str                     hashName;
    Str                     name;
    Str                     baseDir;                    ///< Base directory to include other shaders
    int                     flags = 0;
    mutable int             refCount = 0;
    bool                    permanence = false;
    int                     frameCount = 0;

    RHI::Handle             shaderHandle = RHI::NullShader;
    int                     shaderFlags = 0;
    Str                     vsText;                     ///< Vertex shader souce code text
    Str                     fsText;                     ///< Fragment shader source code text
    int                     builtInConstantIndices[MaxBuiltInConstants];
    int                     builtInSamplerUnits[MaxBuiltInSamplers];

    Array<Define>           defineArray;                ///< Define list for instantiated shader

    Shader *                originalShader = nullptr;   ///< Original shader pointer. Instantiated shader has a pointer to the original shader
    Array<Shader *>         instantiatedShaders;

    Shader *                perforatedVersion = nullptr;
    Shader *                premulAlphaVersion = nullptr;
    Shader *                indirectLitVersion = nullptr;
    Shader *                directLitVersion = nullptr;
    Shader *                indirectLitDirectLitVersion = nullptr;
    Shader *                parallelShadowVersion = nullptr;
    Shader *                spotShadowVersion = nullptr;
    Shader *                pointShadowVersion = nullptr;
    Shader *                gpuSkinningVersion[3] = { nullptr, };
    Shader *                gpuInstancingVersion = nullptr;

    StrHashMap<PropertyInfo> propertyInfoHashMap;
};

BE_INLINE Shader::Shader() {
}

BE_INLINE Shader::~Shader() {
    Purge();
}

class ShaderManager {
    friend class Shader;

public:
    enum PredefinedOriginalShader {
        WriteValueShader,
        DrawArrayTextureShader,
        UnlitShader,
        SelectionIdShader,
        DepthShader,
        DepthNormalShader,
        ConstantColorShader,
        VertexColorShader,
        ObjectMotionBlurShader,
        StandardSpecShader,
        StandardShader,
        PhongShader,
        SkyboxCubemapShader,
        SkyboxSixSidedShader,
        EnvCubemapShader,
        FogLightShader,
        BlendLightShader,
        PostObjectMotionBlurShader,
        PostCameraMotionBlurShader,
        PostPassThruShader,
        PostPassThruColorShader,
        PostPassThruCubeFaceShader,
        Downscale2x2Shader,
        Downscale4x4Shader,
        Downscale4x4LogLumShader,
        Downscale4x4ExpLumShader,
        Blur5xShader,
        Blur7xShader,
        Blur15xShader,
        BlurBilinear3xShader,
        BlurBilinear4xShader,
        BlurBilinear8xShader,
        BlurAlphaMaskedBilinear8xShader,
        KawaseBlurShader,
        RadialBlurShader,
        AoBlurShader,
        PostColorTransformShader,
        PostGammaCorrectionShader,
        LinearizeDepthShader,
        CopyDownscaledCocToAlphaShader,
        CopyColorAndCocShader,
        ApplyDofShader,
        SunShaftsMaskGenShader,
        SunShaftsGenShader,
        SunShaftsDisplayShader,
        LuminanceAdaptationShader,
        BrightFilterShader,
        LdrFinalShader,
        HdrFinalShader,
        ChromaShiftShader,
        SsaoShader,
        GenerateHomShader,
        QueryHomShader,
        MaxPredefinedOriginalShaders
    };
   
    void                    Init();
    void                    Shutdown();

    Shader *                AllocShader(const char *name);
    Shader *                FindShader(const char *name) const;
    Shader *                GetShader(const char *name);

    void                    ReleaseShader(Shader *shader, bool immediateDestroy = false);
    void                    DestroyShader(Shader *shader);
    void                    DestroyUnusedShaders();

    void                    PrecacheShader(const char *name);

    void                    RenameShader(Shader *shader, const Str &newName);

    void                    ReloadShaders();
    void                    ReloadLitSurfaceShaders();

    bool                    FindGlobalHeader(const char *text) const;
    void                    AddGlobalHeader(const char *text);
    void                    RemoveGlobalHeader(const char *text);

    static Shader *         originalShaders[MaxPredefinedOriginalShaders];

    static Shader *         writeValueShader;
    static Shader *         drawArrayTextureShader;
    static Shader *         unlitShader;
    static Shader *         selectionIdShader;
    static Shader *         depthShader;
    static Shader *         depthNormalShader;
    static Shader *         constantColorShader;
    static Shader *         vertexColorShader;
    static Shader *         objectMotionBlurShader;
    static Shader *         standardDefaultShader;
    static Shader *         standardDefaultIndirectLitShader;
    static Shader *         standardDefaultDirectLitShader;
    static Shader *         standardDefaultIndirectLitDirectLitShader;
    static Shader *         skyboxCubemapShader;
    static Shader *         skyboxSixSidedShader;
    static Shader *         envCubemapShader;
    static Shader *         fogLightShader;
    static Shader *         blendLightShader;
    static Shader *         postObjectMotionBlurShader;
    static Shader *         postCameraMotionBlurShader;
    static Shader *         postPassThruShader;
    static Shader *         postPassThruColorShader;
    static Shader *         postPassThruCubeFaceShader;
    static Shader *         downscale2x2Shader;
    static Shader *         downscale4x4Shader;
    static Shader *         downscale4x4LogLumShader;
    static Shader *         downscale4x4ExpLumShader;
    static Shader *         blur5xShader;
    static Shader *         blur7xShader;
    static Shader *         blur15xShader;
    static Shader *         blurBilinear3xShader;
    static Shader *         blurBilinear4xShader;
    static Shader *         blurBilinear8xShader;
    static Shader *         blurAlphaMaskedBilinear8xShader;
    static Shader *         kawaseBlurShader;
    static Shader *         radialBlurShader;
    static Shader *         aoBlurShader;
    static Shader *         postColorTransformShader;
    static Shader *         postGammaCorrectionShader;
    static Shader *         linearizeDepthShader;
    static Shader *         copyDownscaledCocToAlphaShader;
    static Shader *         copyColorAndCocShader;
    static Shader *         applyDofShader;
    static Shader *         sunShaftsMaskGenShader;
    static Shader *         sunShaftsGenShader;
    static Shader *         sunShaftsDisplayShader;
    static Shader *         brightFilterShader;
    static Shader *         luminanceAdaptationShader;
    static Shader *         ldrFinalShader;
    static Shader *         hdrFinalShader;
    static Shader *         chromaShiftShader;
    static Shader *         ssaoShader;
    static Shader *         generateHomShader;
    static Shader *         queryHomShader;

private:
    void                    InitGlobalDefines();
    void                    LoadEngineShaders();
    void                    InstantiateEngineShaders();

    static void             Cmd_ListShaders(const CmdArgs &args);
    static void             Cmd_ReloadShader(const CmdArgs &args);

    StrIHashMap<Shader *>   shaderHashMap;

    StrArray                globalHeaderList;
};

extern ShaderManager        shaderManager;

BE_NAMESPACE_END
