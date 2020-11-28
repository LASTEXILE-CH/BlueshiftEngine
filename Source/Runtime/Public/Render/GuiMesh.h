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

#include "Core/Vertex.h"
#include "BufferCache.h"
#include "RenderObject.h"
#include "Font.h"

BE_NAMESPACE_BEGIN

class Material;

struct GuiMeshSurf {
    const Material *        material;
    uint32_t                color;

    int                     numVerts;
    int                     numIndexes;
    
    BufferCache             vertexCache;
    BufferCache             indexCache;
};

class GuiMesh {
public:
    struct CoordFrame {
        enum Enum {
            CoordFrame2D,
            CoordFrame3D
        };
    };

    GuiMesh();

    CoordFrame::Enum        GetCoordFrame() const { return coordFrame; }
    void                    SetCoordFrame(CoordFrame::Enum frame) { coordFrame = frame; }

    int                     NumSurfaces() const { return surfaces.Count(); }
    const GuiMeshSurf *     Surface(int surfaceIndex) const { return &surfaces[surfaceIndex]; }

    void                    Clear();

    void                    SetClipRect(const Rect &clipRect);

    void                    SetColor(const Color4 &rgba);
    void                    SetTextBorderColor(const Color4 &rgba);

    void                    DrawPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    
    float                   DrawChar(float x, float y, float sx, float sy, Font *font, char32_t unicodeChar, RenderObject::TextDrawMode::Enum drawMode = RenderObject::TextDrawMode::Normal);

    void                    DrawText(Font *font, RenderObject::TextDrawMode::Enum drawMode, RenderObject::TextAnchor::Enum anchor,
                                RenderObject::TextHorzAlignment::Enum horzAlignment, float lineSpacing, float textScale, const Str &text);

    void                    DrawTextRect(Font *font, RenderObject::TextDrawMode::Enum drawMode, const RectF &rect,
                                RenderObject::TextHorzAlignment::Enum horzAlignment, RenderObject::TextVertAlignment::Enum vertAlignment,
                                RenderObject::TextHorzOverflow::Enum horzOverflow, RenderObject::TextVertOverflow::Enum vertOverflow, 
                                float lineSpacing, float textScale, const Str &text);

                            // Call this function when drawing ends
    void                    CacheIndexes();

    AABB                    Compute3DTextAABB(Font *font, RenderObject::TextAnchor::Enum anchor, float lineSpacing, float textScale, const Str &text) const;

private:
    void                    PrepareNextSurf();
    void                    DrawQuad(const VertexGeneric *verts, const Material *material);
    
    Array<GuiMeshSurf>      surfaces;
    GuiMeshSurf *           currentSurf;
    uint32_t                currentColor;
    uint32_t                currentTextBorderColor;

    int                     totalVerts;         ///< Total number of the vertices
    int                     totalIndexes;       ///< Total number of the indices

    CoordFrame::Enum        coordFrame;
    Rect                    clipRect;
};

BE_NAMESPACE_END
