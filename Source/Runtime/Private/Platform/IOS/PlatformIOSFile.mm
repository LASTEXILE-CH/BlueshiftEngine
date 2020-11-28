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
#include "Platform/PlatformFile.h"
#include "IO/FileSystem.h"
#include <sys/stat.h>
#include <sys/mman.h>

BE_NAMESPACE_BEGIN

PlatformIOSFile::PlatformIOSFile(FILE *fp) : PlatformPosixFile(fp) {
}

PlatformIOSFile::~PlatformIOSFile() {
}

Str PlatformIOSFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = PlatformFile::GetBasePath();
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.BackSlashesToSlashes();
    return normalizedFilename;
}

Str PlatformIOSFile::NormalizeDirectoryName(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = PlatformFile::GetBasePath();
        normalizedDirname.AppendPath(dirname);
    }
    normalizedDirname.BackSlashesToSlashes();
    
    int length = normalizedDirname.Length();
    if (length > 0) {
        if (normalizedDirname[length - 1] != '/') {
            normalizedDirname.Append('/');
        }
    }
    
    return normalizedDirname;
}

PlatformBaseFile *PlatformIOSFile::OpenFileRead(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(ConvertToIOSPath(normalizedFilename, false), "rb");
    if (!fp) {
        fp = fopen(ConvertToIOSPath(normalizedFilename, true), "rb");
        if (!fp) {
            return nullptr;
        }
    }
    
    return new PlatformIOSFile(fp);
}

PlatformBaseFile *PlatformIOSFile::OpenFileWrite(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(ConvertToIOSPath(normalizedFilename, true), "wb");
    if (!fp) {
        return nullptr;
    }
    
    return new PlatformIOSFile(fp);
}

PlatformBaseFile *PlatformIOSFile::OpenFileAppend(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(ConvertToIOSPath(normalizedFilename, true), "ab");
    if (!fp) {
        return nullptr;
    }
    
    return new PlatformIOSFile(fp);
}

bool PlatformIOSFile::FileExists(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
        if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
            return false;
        }
    }
    return S_ISREG(fileInfo.st_mode);
}

size_t PlatformIOSFile::FileSize(const char *filename) {
    struct stat fileInfo;
    fileInfo.st_size = -1;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
        if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
            return false;
        }
    }
    
    // make sure to return -1 for directories
    if (S_ISDIR(fileInfo.st_mode)) {
        fileInfo.st_size = -1;
    }
    
    return fileInfo.st_size;
}

bool PlatformIOSFile::IsFileWritable(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
        if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
            return false;
        }
    }
    return (fileInfo.st_mode & S_IWUSR) != 0;
}

bool PlatformIOSFile::IsReadOnly(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    Str filePath = ConvertToIOSPath(normalizedFilename, false);
    if (access(filePath, F_OK) == -1) {
        if (access(ConvertToIOSPath(normalizedFilename, true), F_OK) == -1) {
            return false; // file doesn't exist
        }
    }
    
    if (access(filePath, W_OK) == -1) {
        return errno == EACCES;
    }
    return false;
}

bool PlatformIOSFile::RemoveFile(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    return unlink(ConvertToIOSPath(normalizedFilename, true));
}

bool PlatformIOSFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    Str normalizedSrcFilename = ConvertToIOSPath(NormalizeFilename(srcFilename), false);
    Str normalizedDstFilename = ConvertToIOSPath(NormalizeFilename(dstFilename), true);
    return rename(normalizedSrcFilename, normalizedDstFilename) != -1;
}

bool PlatformIOSFile::DirectoryExists(const char *dirname) {
    struct stat fileInfo;
    Str normalizedDirname = NormalizeFilename(dirname);
    if (stat(ConvertToIOSPath(normalizedDirname, false), &fileInfo) == -1) {
        if (stat(ConvertToIOSPath(normalizedDirname, true), &fileInfo) == -1) {
            return false;
        }
    }
    return S_ISDIR(fileInfo.st_mode);
}

bool PlatformIOSFile::CreateDirectory(const char *dirname) {
    Str normalizedDirname = ConvertToIOSPath(NormalizeFilename(dirname), true);
    CFStringRef directory = StringToCFString(normalizedDirname.c_str());
    bool Result = [[NSFileManager defaultManager] createDirectoryAtPath:(__bridge NSString *)directory
                                            withIntermediateDirectories:true
                                                             attributes:nil
                                                                  error:nil];
    CFRelease(directory);
    return Result;
}

bool PlatformIOSFile::RemoveDirectory(const char *dirname) {
    Str normalizedDirname = ConvertToIOSPath(NormalizeFilename(dirname), true);
    return rmdir(normalizedDirname) == 0;
}

const char *PlatformIOSFile::ExecutablePath() {
    static char path[512] = "";
    if (!path[0]) {
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        strcpy(path, (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

Str PlatformIOSFile::ConvertToIOSPath(const Str &filename, bool forWrite) {
    if (FileSystem::IsAbsolutePath(filename)) {
        return filename;
    }
    
    Str result;
    Str appPath = PlatformFile::ExecutablePath();
    Str relFilename = filename.ToRelativePath(appPath);
    
    if (!forWrite) {
        static NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        const char *cstr = (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding];
        result = cstr;
        result.AppendPath(relFilename);
    } else {
        static NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        const char *cstr = (const char *)[documentPath cStringUsingEncoding:NSUTF8StringEncoding];
        result = cstr;
        result.AppendPath(relFilename);
    }
    
    return result;
}

PlatformIOSFileMapping::PlatformIOSFileMapping(int fileHandle, size_t size, const void *data) {
    this->fileHandle = fileHandle;
    this->size = size;
    this->data = data;
}

PlatformIOSFileMapping::~PlatformIOSFileMapping() {
    int retval = munmap((void *)data, size);
    if (retval != 0) {
        BE_ERRLOG("Unable to unmap memory\n");
    }
    msync((void *)data, size, MS_SYNC);
    close(fileHandle);
}

void PlatformIOSFileMapping::Touch() {
    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);

    uint32_t checkSum = 0;
    for (byte *ptr = (byte *)data; ptr < (byte *)data + size; ptr += pageSize) {
        checkSum += *(uint32_t *)ptr;
    }
}

PlatformIOSFileMapping *PlatformIOSFileMapping::OpenFileRead(const char *filename) {
    Str normalizedFilename = PlatformIOSFile::NormalizeFilename(filename);
    int fd = open(PlatformIOSFile::ConvertToIOSPath(normalizedFilename, false), O_RDONLY);
    if (fd == -1) {
        fd = open(PlatformIOSFile::ConvertToIOSPath(normalizedFilename, true), O_RDONLY);
        if (fd == -1) {
            return nullptr;
        }
    }

    struct stat fs;
    fstat(fd, &fs);
    size_t size = fs.st_size;

    void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (!data) {
        BE_ERRLOG("PlatformIOSFileMapping::OpenFileRead: Couldn't map %s to memory\n", filename);
        return nullptr;
    }

    return new PlatformIOSFileMapping(fd, size, data);
}

PlatformIOSFileMapping *PlatformIOSFileMapping::OpenFileReadWrite(const char *filename, int newSize) {
    Str normalizedFilename = PlatformIOSFile::NormalizeFilename(filename);
    int fd = open(PlatformIOSFile::ConvertToIOSPath(normalizedFilename, false), O_RDWR | O_CREAT);
    if (fd == -1) {
        fd = open(PlatformIOSFile::ConvertToIOSPath(normalizedFilename, true), O_RDWR | O_CREAT);
        if (fd == -1) {
            BE_ERRLOG("PlatformIOSFileMapping::OpenFileReadWrite: Couldn't open %s\n", filename);
            return nullptr;
        }
    }

    size_t size = newSize;
    if (size == 0) {
        struct stat fs;
        fstat(fd, &fs);
        size = fs.st_size;
    }

    void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!data) {
        BE_ERRLOG("PlatformIOSFileMapping::OpenFileReadWrite: Couldn't map %s to memory\n", filename);
        return nullptr;
    }

    return new PlatformIOSFileMapping(fd, size, data);
}

BE_NAMESPACE_END
