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

using ImageUnpackFunc = void(*)(const byte *src, byte *dst, int numPixels);
using ImagePackFunc = void(*)(const byte *src, byte *dst, int numPixels);

struct ImageFormatInfo {
    const char *name;
    int size; // bytes per pixel or bytes per block
    int numComponents;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int type;
    ImageUnpackFunc unpackRGBA8888; // Unpack to RGBA_8_8_8_8
    ImagePackFunc packRGBA8888;     // Pack from RGBA_8_8_8_8`
    ImageUnpackFunc unpackRGBA32F;  // Unpack to RGBA_32F_32F_32F_32F
    ImagePackFunc packRGBA32F;      // Pack from RGBA_32F_32F_32F_32F
};

void DecompressDXT1(const Image &srcImage, Image &dstImage);
void DecompressDXT3(const Image &srcImage, Image &dstImage);
void DecompressDXT5(const Image &srcImage, Image &dstImage);
void DecompressDXN2(const Image &srcImage, Image &dstImage);

void DecompressPVRTC(const Image &srcImage, Image &dstImage, int do2BitMode);

void DecompressETC1(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGB8(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGBA8(const Image &srcImage, Image &dstImage);
void DecompressETC2_RGB8A1(const Image &srcImage, Image &dstImage);
void DecompressEAC_R11(const Image &srcImage, Image &dstImage, bool signedFormat);
void DecompressEAC_RG11(const Image &srcImage, Image &dstImage, bool signedFormat, bool normal);

void CompressDXT1(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressDXT3(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressDXT5(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressDXN2(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);

void CompressETC1(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressETC2_RGB8(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressETC2_RGBA1(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressETC2_RGBA8(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressEAC_R11(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressEAC_RG11(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressEAC_Signed_R11(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);
void CompressEAC_Signed_RG11(const Image &srcImage, Image &dstImage, Image::CompressionQuality::Enum compressionQuality);

bool CompressedFormatBlockDimensions(Image::Format::Enum imageFormat, int &blockWidth, int &blockHeight);
bool CompressedFormatMinDimensions(Image::Format::Enum imageFormat, int &minWidth, int &minHeight);

const ImageFormatInfo *GetImageFormatInfo(Image::Format::Enum imageFormat);

void RGBToYCoCg(short *YCoCg, const byte *rgb, int stride);
void RGBAToYCoCgA(short *YCoCgA, const byte *rgba, int stride);
void YCoCgToRGB(byte *rgb, int stride, const short *YCoCg);
void YCoCgAToRGBA(byte *rgba, int stride, const short *YCoCgA);

BE_NAMESPACE_END
