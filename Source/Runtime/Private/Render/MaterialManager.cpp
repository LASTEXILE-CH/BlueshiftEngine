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
#include "Asset/GuidMapper.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

Material *              MaterialManager::defaultMaterial;
Material *              MaterialManager::whiteMaterial;
Material *              MaterialManager::unlitMaterial;
Material *              MaterialManager::blendMaterial;
Material *              MaterialManager::whiteLightMaterial;
Material *              MaterialManager::zeroClampLightMaterial;
Material *              MaterialManager::defaultSkyboxMaterial;

MaterialManager         materialManager;

static int              materialCounter = 0;
static constexpr int    MaxMaterialCount = 65536;

void MaterialManager::Init() {
    cmdSystem.AddCommand("listMaterials", Cmd_ListMaterials);
    cmdSystem.AddCommand("reloadMaterial", Cmd_ReloadMaterial);

    materialHashMap.Init(1024, 65536, 1024);

    CreateEngineMaterials();
}

void MaterialManager::CreateEngineMaterials() {
    // Create default lit surface material
    defaultMaterial = AllocMaterial("_defaultMaterial");
    defaultMaterial->Create(va(
        "pass {\n"
        "   shader \"%s\" {\n"
        "       _ALBEDO \"1\"\n"
        "       albedoMap \"%s\"\n"
        "   }\n"
        "}", GuidMapper::standardShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces), GuidMapper::defaultTextureGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    defaultMaterial->permanence = true;

    // Create white lit surface material
    whiteMaterial = AllocMaterial("_whiteMaterial");
    whiteMaterial->Create(va(
        "pass {\n"
        "   useOwnerColor\n"
        "   shader \"%s\" {\n"
        "       albedoColor \"1 1 1\"\n"
        "   }\n"
        "}", GuidMapper::standardShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    whiteMaterial->permanence = true;

    // Create unlit surface material
    unlitMaterial = AllocMaterial("_unlitMaterial");
    unlitMaterial->Create(va(
        "pass {\n"
        "   useOwnerColor\n"
        "   shader \"%s\" {\n"
        "       albedoColor \"1 1 1\"\n"
        "   }\n"
        "}", GuidMapper::unlitShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    unlitMaterial->permanence = true;

    // Create blend unlit surface material
    blendMaterial = AllocMaterial("_blendMaterial");
    blendMaterial->Create(va(
        "pass {\n"
        "   renderingMode alphaBlend\n"
        "   useOwnerColor\n"
        "   blendFunc SRC_ALPHA ONE_MINUS_SRC_ALPHA\n"
        "   shader \"%s\" {\n"
        "       albedoColor \"1 1 1\"\n"
        "   }\n"
        "}", GuidMapper::unlitShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    blendMaterial->permanence = true;

    // Create white light material
    whiteLightMaterial = AllocMaterial("_whiteLightMaterial");
    whiteLightMaterial->Create(va(
        "light\n"
        "pass {\n"
        "   useOwnerColor\n"
        "   map \"%s\"\n"
        "}", GuidMapper::whiteTextureGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    whiteLightMaterial->permanence = true;

    // Create white light (zero clamped) material
    zeroClampLightMaterial = AllocMaterial("_zeroClampLightMaterial");
    zeroClampLightMaterial->Create(va(
        "light\n"
        "pass {\n"
        "   useOwnerColor\n"
        "   map \"%s\"\n"
        "}", GuidMapper::zeroClampTextureGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    zeroClampLightMaterial->permanence = true;

    // Create default skybox material
    defaultSkyboxMaterial = AllocMaterial("_defaultSkyboxMaterial");
    defaultSkyboxMaterial->Create(va(
        "pass {\n"
        "   cull twoSided\n"
        "   shader \"%s\" {\n"
        "       skyCubeMap \"Data/EngineTextures/default_sky.dds\"\n"
        "       rotation 0\n"
        "   }\n"
        "}", GuidMapper::skyboxCubemapShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces)));
    defaultSkyboxMaterial->permanence = true;
}

void MaterialManager::Shutdown() {
    cmdSystem.RemoveCommand("listMaterials");
    cmdSystem.RemoveCommand("reloadMaterial");
    
    for (int i = 0; i < materialHashMap.Count(); i++) {
        const auto *entry = materialManager.materialHashMap.GetByIndex(i);
        Material *material = entry->second;
        material->Purge();
    }
    
    materialHashMap.DeleteContents(true);
}

void MaterialManager::DestroyUnusedMaterials() {
    Array<Material *> removeArray;

    for (int i = 0; i < materialHashMap.Count(); i++) {
        const auto *entry = materialHashMap.GetByIndex(i);
        Material *material = entry->second;

        if (material && !material->permanence && material->refCount == 0) {
            removeArray.Append(material);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyMaterial(removeArray[i]);
    }
}

Material *MaterialManager::AllocMaterial(const char *hashName) {
    if (materialHashMap.Get(hashName)) {
        BE_FATALERROR("%s material already allocated", hashName);
    }

    if (materialHashMap.Count() == MaterialManager::MaxCount) {
        BE_FATALERROR("Material exceed maximum limits %i", MaterialManager::MaxCount);
    }
    
    Material *material = new Material;
    material->hashName = hashName;
    material->name = hashName;
    material->name.StripPath();
    material->name.StripFileExtension();
    material->version = Material::Version;
    material->refCount = 1;
    material->index = materialCounter++ % MaterialManager::MaxCount;
    materialHashMap.Set(material->hashName, material);

    return material;
}

void MaterialManager::DestroyMaterial(Material *material) {
    if (material->refCount > 1) {
        BE_LOG("MaterialManager::DestroyMaterial: material '%s' has %i reference count\n", material->hashName.c_str(), material->refCount);
    }

    materialHashMap.Remove(material->hashName);

    delete material;
}

Material *MaterialManager::FindMaterial(const char *hashName) const {
    const auto *entry = materialHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Material *MaterialManager::GetMaterial(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultMaterial;
    }

    Material *material = FindMaterial(hashName);
    if (material) {
        material->refCount++;
        return material;
    }

    material = AllocMaterial(hashName);
    if (!material->Load(hashName)) {
        DestroyMaterial(material);
        return defaultMaterial;
    }

    return material;
}

void MaterialManager::PrecacheMaterial(const char *filename) {
    Material *material = GetMaterial(filename);
    ReleaseMaterial(material);
}

static const char *HintName(Material::TextureHint::Enum hint) {
    switch (hint) {
    case Material::TextureHint::Light:
        return "Light";
    case Material::TextureHint::VertexColor:
        return "VertexColor";
    case Material::TextureHint::Lightmapped:
        return "Lightmapped";
    case Material::TextureHint::Sprite:
        return "Sprite";
    case Material::TextureHint::Overlay:
        return "Overlay";
    default:
        break;
    }

    return "None";
}

Material *MaterialManager::GetSingleTextureMaterial(const Texture *texture, Material::TextureHint::Enum hint) {
    if (!texture) {
        return defaultMaterial;
    }

    Str materialName = texture->GetHashName();
    materialName += Str("_") + HintName(hint);

    Material *material = FindMaterial(materialName.c_str());
    if (material) {
        material->refCount++;
        return material;
    }

    char buffer[1024];
    switch (hint) {
    case Material::TextureHint::Sprite:
        Str::snPrintf(buffer, sizeof(buffer),
            "noShadow\n"
            "pass {\n"
            "   renderingMode alphaBlend\n"
            "   blendFunc blend\n"
            "   mapPath \"%s\"\n"
            "}\n", texture->GetHashName());
        break;
    case Material::TextureHint::Light:
        Str::snPrintf(buffer, sizeof(buffer),
            "light\n"
            "pass {\n"
            "   mapPath \"%s\"\n"
            "   useOwnerColor\n"
            "}\n", texture->GetHashName());
        break;
    case Material::TextureHint::Overlay:
        Str::snPrintf(buffer, sizeof(buffer),
            "noShadow\n"
            "pass {\n"
            "   cull twoSided\n"
            "   renderingMode alphaBlend\n"
            "   blendFunc blend\n"
            "   vertexColor\n"
            "   mapPath \"%s\"\n"
            "}\n", texture->GetHashName());
        break;
    case Material::TextureHint::EnvCubeMap:
        Str::snPrintf(buffer, sizeof(buffer),
            "noShadow\n"
            "pass {\n"
            "   shader \"%s\" {\n"
            "       envCubeMap \"%s\"\n"
            "       mipLevel 0\n"
            "   }\n"
            "}\n", GuidMapper::envCubemapShaderGuid.ToString(Guid::Format::DigitsWithHyphensInBraces), resourceGuidMapper.Get(texture->GetHashName()).ToString(Guid::Format::DigitsWithHyphensInBraces));
        break;
    default:
        Str::snPrintf(buffer, sizeof(buffer),
            "pass {\n"
            "   mapPath \"%s\"\n"
            "}\n", texture->GetHashName());
        break;
    }

    material = AllocMaterial(materialName);
    if (!material->Create(buffer)) {
        DestroyMaterial(material);
        return defaultMaterial;
    }

    return material;
}

void MaterialManager::RenameMaterial(Material *material, const Str &newName) {
    const auto *entry = materialHashMap.Get(material->hashName);
    if (entry) {
        materialHashMap.Remove(material->hashName);

        material->hashName = newName;
        material->name = newName;
        material->name.StripPath();
        material->name.StripFileExtension();

        materialHashMap.Set(newName, material);
    }
}

void MaterialManager::ReleaseMaterial(Material *material, bool immediateDestroy) {
    if (material->permanence) {
        return;
    }

    if (material->refCount > 0) {
        material->refCount--;
    }

    if (immediateDestroy && material->refCount == 0) {
        DestroyMaterial(material);
    }
}

//--------------------------------------------------------------------------------------------------

void MaterialManager::Cmd_ListMaterials(const CmdArgs &args) {
    int count = 0;
    
    BE_LOG("NUM. REF. NAME\n");

    for (int i = 0; i < materialManager.materialHashMap.Count(); i++) {
        const auto *entry = materialManager.materialHashMap.GetByIndex(i);
        Material *material = entry->second;
 
        BE_LOG("%4d %4d %s\n",
            i,
            material->refCount,
            material->hashName.c_str());

        count++;
    }

    BE_LOG("total %i materials\n", count);
}

void MaterialManager::Cmd_ReloadMaterial(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("reloadMaterial <filename>\n");
        return;
    }

    if (!Str::Icmp(args.Argv(1), "all")) {
        int count = materialManager.materialHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto *entry = materialManager.materialHashMap.GetByIndex(i);
            Material *material = entry->second;
            if (!material) {
                continue;
            }

            material->Reload();
        }
    } else {
        Material *material = materialManager.FindMaterial(args.Argv(1));
        if (!material) {
            BE_WARNLOG("Couldn't find material to reload \"%s\"\n", args.Argv(1));
            return;
        }

        material->Reload();
    }
}

BE_NAMESPACE_END
