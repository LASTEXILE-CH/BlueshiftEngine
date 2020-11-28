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
#include "IO/FileSystem.h"
#include "Platform/PlatformFile.h"
#include "PlatformUtils/Android/AndroidJNI.h"
#include <android/asset_manager.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>
#include <fcntl.h>

BE_NAMESPACE_BEGIN

PlatformAndroidFile::PlatformAndroidFile(FILE *fp) {
    this->fp = fp;
    this->asset = 0;
}

PlatformAndroidFile::PlatformAndroidFile(AAsset *asset) {
    this->fp = 0;
    this->asset = asset;
}

PlatformAndroidFile::~PlatformAndroidFile() {
    if (fp) {
        fclose(fp);
    } 
    if (asset) {
        AAsset_close(asset);
    }
}

int PlatformAndroidFile::Tell() const {
    if (fp) {
        return (int)ftell(fp);
    }
    if (asset) {
        return (int)AAsset_getLength(asset) - (int)AAsset_getRemainingLength(asset);
    }
    return -1;
}

int PlatformAndroidFile::Size() const {
    if (fp) {
        struct stat fileInfo;
        fileInfo.st_size = -1;
        if (fstat(fileno(fp), &fileInfo) < 0) {
            return -1;
        }
        return (int)fileInfo.st_size;
    } 
    if (asset) {
        return (int)AAsset_getLength(asset);
    }
    return -1;
}

int PlatformAndroidFile::Seek(long offset, Origin::Enum origin) {
    assert(offset >= 0);

    int whence;
    switch (origin) {
    case Origin::Start:
        whence = SEEK_SET;
        break;
    case Origin::End:
        whence = SEEK_END;
        break;
    case Origin::Current:
        whence = SEEK_CUR;
        break;
    default:
        assert(0);
        break;
    }

    if (fp) {
        return fseek(fp, offset, whence);
    }
    if (asset) {
        return AAsset_seek(asset, offset, whence) >= 0 ? 0 : -1;
    }
    return -1;
}

size_t PlatformAndroidFile::Read(void *buffer, size_t bytesToRead) const {
    assert(bytesToRead > 0);
    if (fp) {
        size_t readBytes = fread(buffer, 1, bytesToRead, fp);
        if (readBytes == -1) {
            BE_FATALERROR("PlatformAndroidFile::Read: -1 bytes read");
            return 0;
        }
        return readBytes;
    }
    if (asset) {
        size_t avail = (size_t)AAsset_getRemainingLength(asset);
        if (avail == 0) {
            return 0;
        }
        if (bytesToRead > avail) {
            bytesToRead = avail;
        }
        jint readBytes = AAsset_read(asset, buffer, bytesToRead);
        return (size_t)readBytes;
    }
    return 0;
}

bool PlatformAndroidFile::Write(const void *buffer, size_t bytesToWrite) {
    if (!fp) {
        // Can't write to asset.
        return false;
    }

    byte *ptr = (byte *)buffer;
    size_t writeSize = bytesToWrite;
    bool failedOnce = false;

    while (writeSize) {
        size_t written = fwrite(ptr, 1, writeSize, fp);
        if (!written) {
            if (!failedOnce) {
                failedOnce = true;
            }
            else {
                BE_WARNLOG("PlatformAndroidFile::Write: 0 bytes written");
                return false;
            }
        }
        else if (written == -1) {
            BE_WARNLOG("PlatformAndroidFile::Write: -1 bytes written");
            return false;
        }

        writeSize -= written;
        ptr += written;
    }

    fflush(fp);
    return true;
}

Str PlatformAndroidFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = GetBasePath();
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.CleanPath(PATHSEPERATOR_CHAR);

    return normalizedFilename;
}

Str PlatformAndroidFile::NormalizeDirectoryName(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = GetBasePath();
        normalizedDirname.AppendPath(dirname);
    }
    normalizedDirname.CleanPath(PATHSEPERATOR_CHAR);

    int length = normalizedDirname.Length();
    if (length > 0) {
        if (normalizedDirname[length - 1] != PATHSEPERATOR_CHAR) {
            normalizedDirname.Append(PATHSEPERATOR_CHAR);
        }
    }

    return normalizedDirname;
}

PlatformBaseFile *PlatformAndroidFile::OpenFileRead(const char *filename) {
    // FIXME: Read newest file
    // Read from external data directory
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename.c_str(), "rb");
    if (fp) {
        return new PlatformAndroidFile(fp);
    }
    // Read by asset manager
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        return new PlatformAndroidFile(asset);
    }        
    return nullptr;
}

PlatformBaseFile *PlatformAndroidFile::OpenFileWrite(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "wb");
    if (!fp) {
        return nullptr;
    }
    return new PlatformAndroidFile(fp);
}

PlatformBaseFile *PlatformAndroidFile::OpenFileAppend(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "ab");
    if (!fp) {
        return nullptr;
    }
    return new PlatformAndroidFile(fp);
}

bool PlatformAndroidFile::FileExists(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
        return true;
    }
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        AAsset_close(asset);
        return true;
    }
    return false;
}

size_t PlatformAndroidFile::FileSize(const char *filename) {
    struct stat fileInfo;
    fileInfo.st_size = -1;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
        return fileInfo.st_size;
    }
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        size_t size = (size_t)AAsset_getLength(asset);
        AAsset_close(asset);
        return size;
    }
    return 0;
}

bool PlatformAndroidFile::IsFileWritable(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return true;
    }
    return (fileInfo.st_mode & S_IWUSR) != 0;
}

bool PlatformAndroidFile::IsReadOnly(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (access(normalizedFilename, F_OK) == -1) {
        return false; // file doesn't exist
    }
    if (access(normalizedFilename, W_OK) == -1) {
        return errno == EACCES;
    }
    return false;
}

bool PlatformAndroidFile::SetReadOnly(const char *filename, bool readOnly) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return false;
    }
    
    if (readOnly) {
        fileInfo.st_mode &= ~S_IWUSR;
    } else {
        fileInfo.st_mode |= S_IWUSR;
    }
    return chmod(normalizedFilename, fileInfo.st_mode) == 0;
}

bool PlatformAndroidFile::RemoveFile(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (remove(normalizedFilename)) {
        BE_LOG("failed to remove file '%s'\n", normalizedFilename.c_str());
        return false;
    }

    return true;
}

bool PlatformAndroidFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    Str normalizedSrcFilename = NormalizeFilename(srcFilename);
    Str normalizedDstFilename = NormalizeFilename(dstFilename);
    return rename(normalizedSrcFilename, normalizedDstFilename) == 0;
}

int PlatformAndroidFile::GetFileMode(const char *filename) {
    struct stat fileInfo;
    if (stat(NormalizeFilename(filename), &fileInfo) != 0) {
        return -1;
    }
    int fileMode = 0;
    if (fileInfo.st_mode & S_IRUSR) {
        fileMode |= Mode::Readable;
    }
    if (fileInfo.st_mode & S_IWUSR) {
        fileMode |= Mode::Writable;
    }
    if (fileInfo.st_mode & S_IXUSR) {
        fileMode |= Mode::Executable;
    }
    return fileMode;
}

void PlatformAndroidFile::SetFileMode(const char *filename, int fileMode) {
    mode_t mode = 0;
    if (fileMode & Mode::Readable) {
        mode |= S_IRUSR;
    }
    if (fileMode & Mode::Writable) {
        mode |= S_IWUSR;
    }
    if (fileMode & Mode::Executable) {
        mode |= S_IXUSR;
    }
    chmod(NormalizeFilename(filename), mode);
}

DateTime PlatformAndroidFile::GetTimeStamp(const char *filename) {
    static const DateTime epoch(1970, 1, 1);
    struct stat fileInfo;

    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return DateTime::MinValue();
    }
    
    Timespan timeSinceEpoch = Timespan::FromSeconds(fileInfo.st_mtime);
    return epoch + timeSinceEpoch;
}

void PlatformAndroidFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
}

bool PlatformAndroidFile::DirectoryExists(const char *dirname) {
    struct stat fileInfo;
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    if (stat(normalizedDirname, &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode)) {
        return true;
    }
    return false;
}

bool PlatformAndroidFile::CreateDirectory(const char *dirname) {
    if (DirectoryExists(dirname)) {
        return true;
    }
    return mkdir(NormalizeDirectoryName(dirname), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}

bool PlatformAndroidFile::RemoveDirectory(const char *dirname) {
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    return rmdir(normalizedDirname) == 0;
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);
    if (rv) {
        perror(fpath);
    }

    return rv;
}

bool PlatformAndroidFile::RemoveDirectoryTree(const char *dirname) {
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    return nftw(normalizedDirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
}

const char *PlatformAndroidFile::Cwd() {
    static char cwd[MaxAbsolutePath];

    getcwd(cwd, sizeof(cwd) - 1);
    cwd[sizeof(cwd) - 1] = 0;
    return cwd;
}

bool PlatformAndroidFile::SetCwd(const char *dirname) {
    return chdir(dirname) == 0;
}

const char *PlatformAndroidFile::ExecutablePath() {
    return AndroidJNI::activity->externalDataPath;
}

static void ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files) {
    FileInfo    fileInfo;
    char        path[MaxAbsolutePath];
    char        subpath[MaxAbsolutePath];
    char        filename[MaxAbsolutePath];

    if (subdir[0]) {
        Str::snPrintf(path, sizeof(path), "%s/%s", directory, subdir);
    } else {
        Str::snPrintf(path, sizeof(path), "%s", directory);
    }

    DIR *dp = opendir(path);
    if (dp) {
        while (dirent *dent = readdir(dp)) {
            if (!Str::Cmp(dent->d_name, ".") || !Str::Cmp(dent->d_name, "..")) {
                continue;
            }

            if (dent->d_type & DT_DIR) {
                if (subdir[0]) {
                    Str::snPrintf(subpath, sizeof(subpath), "%s/%s", subdir, dent->d_name);
                } else {
                    Str::snPrintf(subpath, sizeof(subpath), "%s", dent->d_name);
                }

                if (includeSubDir) {
                    fileInfo.isSubDir = true;
                    fileInfo.filename = subpath;
                    files.Append(fileInfo);
                }

                ListFilesRecursive(directory, subpath, nameFilter, includeSubDir, files);
            } else if (Str::Filter(nameFilter, dent->d_name, false)) {
                if (subdir[0]) {
                    Str::snPrintf(filename, sizeof(filename), "%s/%s", subdir, dent->d_name);
                } else {
                    Str::snPrintf(filename, sizeof(filename), "%s", dent->d_name);
                }

                fileInfo.isSubDir = false;
                fileInfo.filename = filename;
                files.Append(fileInfo);
            }
        }

        closedir(dp);
    }

    AAssetDir *assetDir = AAssetManager_openDir(AndroidJNI::activity->assetManager, path);
    if (assetDir) {
        FileInfo fileInfo;
        while (const char *filename = AAssetDir_getNextFileName(assetDir)) {
            if (!Str::Filter(nameFilter, filename)) {
                continue;
            }

            fileInfo.isSubDir = false;
            fileInfo.filename = filename;
            files.Append(fileInfo);
        }

        AAssetDir_close(assetDir);
    }
}

int PlatformAndroidFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
    if (!nameFilter) {
        nameFilter = "*";
    }

    Str normalizedDirectory = NormalizeFilename(directory);

    files.Clear();

    if (recursive) {
        ListFilesRecursive(normalizedDirectory, "", nameFilter, includeSubDir, files);
    } else {
        DIR *dp = opendir(normalizedDirectory);
        if (dp) {
            FileInfo fileInfo;
            while (dirent *dent = readdir(dp)) {
                if (!Str::Filter(nameFilter, dent->d_name)) {
                    continue;
                }

                if (includeSubDir) {
                    fileInfo.isSubDir = (dent->d_type & DT_DIR) != 0;
                } else {
                    if (dent->d_type & DT_DIR) {
                        continue;
                    }
                }

                fileInfo.filename = dent->d_name;
                files.Append(fileInfo);
            }
            closedir(dp);
        }

        AAssetDir *assetDir = AAssetManager_openDir(AndroidJNI::activity->assetManager, directory);
        if (assetDir) {
            FileInfo fileInfo;
            while (const char *filename = AAssetDir_getNextFileName(assetDir)) {
                if (!Str::Filter(nameFilter, filename)) {
                    continue;
                }

                fileInfo.isSubDir = false;
                fileInfo.filename = filename;
                files.Append(fileInfo);
            }

            AAssetDir_close(assetDir);
        }
    }

    return files.Count();
}

off_t PlatformAndroidFileMapping::pageMask = 0;

PlatformAndroidFileMapping::PlatformAndroidFileMapping(int fileHandle, size_t size, const void *data) {
    this->fileHandle = fileHandle;
    this->size = size;
    this->data = data;
}

PlatformAndroidFileMapping::~PlatformAndroidFileMapping() {
    int retval = munmap((byte *)data - alignBytes, size);
    if (retval != 0) {
        BE_ERRLOG("Unable to unmap memory\n");
    }
    close(fileHandle);
}

void PlatformAndroidFileMapping::Touch() {
    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);

    uint32_t checkSum = 0;
    for (byte *ptr = (byte *)data; ptr < (byte *)data + size; ptr += pageSize) {
        checkSum += *(uint32_t *)ptr;
    }
}

PlatformAndroidFileMapping *PlatformAndroidFileMapping::OpenFileRead(const char *filename) {
    off_t startOffset;
    size_t size;

    Str normalizedFilename = PlatformAndroidFile::NormalizeFilename(filename);
    int fd = open(normalizedFilename, O_RDONLY);
    if (fd < 0) {
        AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_UNKNOWN);
        if (!asset) {
            return nullptr;
        }

        off_t fileLength;
        // NOTE: AAsset_openFileDescriptor will work only with files that are not compressed.
        fd = AAsset_openFileDescriptor(asset, &startOffset, &fileLength);
        assert(fd > 0);
        size = fileLength;
        AAsset_close(asset);
    } else {
        struct stat fs;
        fstat(fd, &fs);
        size = fs.st_size;
        startOffset = 0;
    }

    if (pageMask == 0) {
        pageMask = getpagesize() - 1;
    }
    off_t alignBytes = (startOffset & pageMask);

    void *data = mmap(nullptr, size + alignBytes, PROT_READ, MAP_SHARED, fd, startOffset - alignBytes);
    if (!data) {
        BE_ERRLOG("PlatformAndroidFileMapping::OpenFileRead: Couldn't map %s to memory\n", filename);
        assert(0);
        close(fd);
        return nullptr;
    }
    assert(((size_t)data & pageMask) == 0);

    auto newFileMapping = new PlatformAndroidFileMapping(fd, size, (byte *)data + alignBytes);
    newFileMapping->alignBytes = alignBytes;

    return newFileMapping;
}

PlatformAndroidFileMapping *PlatformAndroidFileMapping::OpenFileReadWrite(const char *filename, int newSize) {
    Str normalizedFilename = PlatformAndroidFile::NormalizeFilename(filename);
    int fd = open(normalizedFilename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        BE_ERRLOG("PlatformAndroidFileMapping::OpenFileReadWrite: Couldn't open %s\n", filename);
        return nullptr;
    }

    size_t size = newSize;
    if (size == 0) {
        struct stat fs;
        fstat(fd, &fs);
        size = fs.st_size;
    }

    void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!data) {
        BE_ERRLOG("PlatformAndroidFileMapping::OpenFileReadWrite: Couldn't map %s to memory\n", filename);
        return nullptr;
    }

    return new PlatformAndroidFileMapping(fd, size, data);
}

BE_NAMESPACE_END
