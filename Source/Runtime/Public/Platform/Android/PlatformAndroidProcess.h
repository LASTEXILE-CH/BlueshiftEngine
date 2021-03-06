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
#define USE_BASE_PLATFORM_LINUX_PROCESS
#include "../Linux/PlatformLinuxProcess.h"

BE_NAMESPACE_BEGIN

struct ProcessHandle {
    ProcessHandle(void *task = nullptr) { }
    bool IsValid() const { return false; }
    void Close() {}
};

class BE_API PlatformAndroidProcess : public PlatformLinuxProcess {
public:
    static const wchar_t *      ExecutableFileName();

    static const wchar_t *      ComputerName();
    static const wchar_t *      UserName();
};

typedef PlatformAndroidProcess  PlatformProcess;

BE_NAMESPACE_END