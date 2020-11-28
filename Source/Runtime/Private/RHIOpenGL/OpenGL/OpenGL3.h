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
===============================================================================

    OpenGL 3.2

===============================================================================
*/

#include "OpenGL.h"

BE_NAMESPACE_BEGIN

class OpenGL3 : public OpenGLBase {
public:
    static constexpr int    GLSL_VERSION = 150;
    static const char *     GLSL_VERSION_STRING;

    static void             Init();

    static bool             SupportsPolygonMode() { return true; }
    static bool             SupportsPackedFloat() { return true; }
    static bool             SupportsDepthClamp() { return true; }
    static bool             SupportsDepthBufferFloat() { return true; }
    static bool             SupportsPixelBuffer() { return true; }
    static bool             SupportsDiscardFrameBuffer() { return true; }
    static bool             SupportsFrameBufferSRGB() { return true; }
    static bool             SupportsTextureRectangle() { return true; }
    static bool             SupportsTextureArray() { return true; }
    static bool             SupportsTextureBuffer() { return true; }
    static bool             SupportsTextureCompressionS3TC() { return true; }
    static bool             SupportsTextureCompressionLATC() { return true; }
    static bool             SupportsTextureCompressionRGTC() { return true; }
    static bool             SupportsCompressedGenMipmaps() { return true; }
    static bool             SupportsGeometryShader() { return true; }
    static bool             SupportsInstancedArrays() { return supportsInstancedArrays; }
    static bool             SupportsDrawIndirect() { return supportsDrawIndirect; }
    static bool             SupportsMultiDrawIndirect() { return supportsMultiDrawIndirect; }
    static bool             SupportsProgramBinary() { return gglProgramBinary != nullptr; }
    static bool             SupportsTimestampQueries() { return supportsTimestampQueries; }

    static void APIENTRY    DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

    static void             QueryTimestampCounter(GLuint queryId);
    static uint32_t         QueryResult32(GLuint queryId);
    static uint64_t         QueryResult64(GLuint queryId);

    static void             PolygonMode(GLenum face, GLenum mode) { gglPolygonMode(face, mode); }
    static void             ClearDepth(GLdouble depth) { gglClearDepth(depth); }
    static void             DepthRange(GLdouble znear, GLdouble zfar) { gglDepthRange(znear, zfar); }
    static void             DrawBuffer(GLenum buffer) { gglDrawBuffer(buffer); }
    static void             ReadBuffer(GLenum buffer) { gglReadBuffer(buffer); }
    static void             DrawBuffers(GLsizei count, const GLenum *buffers) { gglDrawBuffers(count, buffers); }
    static void             TexBuffer(GLenum internalFormat, GLuint buffer) { gglTexBuffer(GL_TEXTURE_BUFFER, internalFormat, buffer); }
    static void             VertexAttribDivisor(int index, int divisor) { gglVertexAttribDivisorARB(index, divisor); }
    static void             DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) { gglDrawElementsBaseVertex(mode, count, type, indices, basevertex); }
    static void             DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) { gglDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex); }
    static void             DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect);
    static void             MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
    static void             CopyImageSubData(GLuint src, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dst, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

    static void             SetTextureSwizzling(GLenum target, Image::Format::Enum format);
    static bool             ImageFormatToGLFormat(Image::Format::Enum imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static bool             SupportedImageFormat(Image::Format::Enum imageFormat) { return ImageFormatToGLFormat(imageFormat, false, nullptr, nullptr, nullptr); }
    static Image::Format::Enum ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap);

private:
    static bool             supportsInstancedArrays;
    static bool             supportsDrawIndirect;
    static bool             supportsMultiDrawIndirect;
    static bool             supportsTimestampQueries;
};

BE_NAMESPACE_END
