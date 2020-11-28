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
#include "Core/Heap.h"
#include "Core/Str.h"
#include "IO/FileSystem.h"
#include "Sound/Pcm.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

bool Pcm::IsOpened() const {
    if (fp) {
        return true;
    }
    return false;
}

bool Pcm::IsLoaded() const {
    if (data) {
        return true;
    }
    return false;
}

void Pcm::Purge() {
    if (fp) {
        Close();
    }

    if (data) {
        Free();
    }
}

bool Pcm::Create(int numChannels, int sampleRates, int bitsWidth, int size, const byte *data) {
    Purge();

    this->channels = numChannels;
    this->sampleRates = sampleRates;
    this->bitsWidth = bitsWidth;
    this->size = size;
    this->data = (byte *)Mem_Alloc16(size);
    if (data) {
        simdProcessor->Memcpy(this->data, data, size);
    }

    return true;
}

void Pcm::Free() {
    if (!data) {
        assert(0);
        return;
    }

    Mem_AlignedFree(data);
    data = nullptr;
}

bool Pcm::Open(const char *filename) {
    Purge();
    
    File *fp = fileSystem.OpenFile(filename, File::Mode::Read);
    if (!fp) {
        return false;
    }

    this->fp = fp;

    if (Str::CheckExtension(filename, ".wav")) {
        fileType = FileType::Wav;

        if (BeginDecodeFile_Wav()) {
            return true;
        }
    } else if (Str::CheckExtension(filename, ".ogg")) {
        fileType = FileType::Ogg;

        if (BeginDecodeFile_OggVorbis()) {
            return true;
        }
    } 

    BE_WARNLOG("Unsupported sound file '%s'\n", filename);
    fileSystem.CloseFile(fp);

    return false;
}

void Pcm::Close() {
    if (!fp) {
        assert(0);
        return;
    }

    if (fileType == FileType::Ogg) {
        EndDecodeFile_OggVorbis();
    } else if (fileType == FileType::Wav) {
        EndDecodeFile_Wav();
    }

    fileSystem.CloseFile(fp);
    fp = nullptr;
}

int Pcm::Read(int size, byte *buffer) {
    int read = 0;

    if (fileType == FileType::Wav) {
        read = DecodeFile_Wav(buffer, size);
    } else if (fileType == FileType::Ogg) {
        read = DecodeFile_OggVorbis(buffer, size);
    }

    readPos += read;

    return read;
}

void Pcm::Seek(int offset) {
    if (fileType == FileType::Wav) {
        SeekFile_Wav(offset);
    } else if (fileType == FileType::Ogg) {
        SeekFile_OggVorbis(offset);
    }

    readPos = offset;
}

bool Pcm::Load(const char *filename) {
    Purge();
    
    byte *base;
    size_t fileSize = fileSystem.LoadFile(filename, true, (void **)&base);
    if (!base) {
        return false;
    }

    const uint32_t id = *(uint32_t *)base;

    if (id == MAKE_FOURCC('R', 'I', 'F', 'F')) { // RIFF WAVE
        fileType = FileType::Wav;

        if (DecodeMemory_Wav(base, fileSize)) {
            fileSystem.FreeFile(base);
            return true;
        }
    } else if (id == MAKE_FOURCC('O', 'g', 'g', 'S')) { // Ogg Vorbis
        fileType = FileType::Ogg;

        if (DecodeMemory_OggVorbis(base, fileSize)) {
            fileSystem.FreeFile(base);
            return true;
        }
    }

    fileSystem.FreeFile(base);

    return false;
}

BE_NAMESPACE_END
