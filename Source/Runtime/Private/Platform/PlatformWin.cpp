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
#include "PlatformWin.h"
#include "Platform/PlatformSystem.h"
#include "Platform/Windows/PlatformWinUtils.h"

BE_NAMESPACE_BEGIN

static int          originalMouseParms[3];
static int          newMouseParms[3];
static POINT        oldCursorPos;
static Point        windowCenter;

static RECT GetScreenWindowRect(HWND hwnd) {
    int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    RECT window_rect;
    ::GetWindowRect(hwnd, &window_rect);
    if (window_rect.left < sx) {
        window_rect.left = sx;
    }

    if (window_rect.top < sy) {
        window_rect.top = sy;
    }

    if (window_rect.right >= cx - 1) {
        window_rect.right = cx - 1;
    }

    if (window_rect.bottom >= cy - 1) {
        window_rect.bottom = cy - 1;
    }

    return window_rect;
}

PlatformWin::PlatformWin() {
}

void PlatformWin::Init() {
    PlatformGeneric::Init();

    hwnd = nullptr;

    cursorLocked = false;
}

void PlatformWin::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformWin::EnableMouse(bool enable) {
#if 0
    SystemParametersInfo(SPI_GETMOUSE, 0, originalMouseParms, 0);
    newMouseParms[0] = 0;
    newMouseParms[1] = 0;
    newMouseParms[2] = 1;
#endif

    GetCursorPos(&oldCursorPos);

    rawMouseDeltaOld.x = 0;
    rawMouseDeltaOld.y = 0;

    PlatformGeneric::EnableMouse(enable);
}

void PlatformWin::SetMainWindowHandle(void *windowHandle) {
    hwnd = (HWND)windowHandle;
}

void PlatformWin::Quit() {
    exit(0);
}

void PlatformWin::Log(const char *text) {
    int len = PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t *wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    OutputDebugString(wText);
}

void PlatformWin::Error(const char *text) {
    int len = PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t *wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    MessageBox(hwnd ? hwnd : GetDesktopWindow(), wText, L"Error", MB_OK);

    PlatformSystem::DebugBreak();

    Quit();
}

bool PlatformWin::IsCursorLocked() const {
    return cursorLocked;
}

bool PlatformWin::LockCursor(bool lock) {
    if (!hwnd || !mouseEnabled) {
        return false;
    }

    bool oldLocked = cursorLocked;

    if (lock ^ cursorLocked) {
        cursorLocked = lock;

        if (lock) {
#if 0
            SystemParametersInfo(SPI_SETMOUSE, 0, newMouseParms, 0);
#endif

            RECT windowRect = GetScreenWindowRect(hwnd);
            windowCenter.x = (windowRect.left + windowRect.right) / 2;
            windowCenter.y = (windowRect.top + windowRect.bottom) / 2;

            GetCursorPos(&oldCursorPos);
            SetCursorPos(windowCenter.x, windowCenter.y);

            rawMouseDeltaOld.x = 0;
            rawMouseDeltaOld.y = 0;

            SetCapture(hwnd);

            // Lock the cursor in the screen.
            ClipCursor(&windowRect);
            while (ShowCursor(FALSE) >= 0);
        } else {
#if 0
            SystemParametersInfo(SPI_SETMOUSE, 0, originalMouseParms, 0);
#endif

            SetCursorPos(oldCursorPos.x, oldCursorPos.y);

            ClipCursor(nullptr);
            ReleaseCapture();
            while (ShowCursor(TRUE) < 0);
        }
    }

    return oldLocked;
}

void PlatformWin::GetMousePos(Point &pos) const {
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    ScreenToClient(hwnd, &cursorPos);

    pos.Set(cursorPos.x, cursorPos.y);
}

void PlatformWin::GenerateMouseDeltaEvent() {
    if (!hwnd || !mouseEnabled) {
        return;
    }

    if (!cursorLocked) {
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        Point mouseDelta;
        mouseDelta.x = cursorPos.x - oldCursorPos.x;
        mouseDelta.y = cursorPos.y - oldCursorPos.y;

        QueEvent(Platform::EventType::MouseDelta, mouseDelta.x, mouseDelta.y, 0, nullptr);

        oldCursorPos = cursorPos;
    } else {
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        Point rawMouseDelta;
        rawMouseDelta.x = cursorPos.x - windowCenter.x;
        rawMouseDelta.y = cursorPos.y - windowCenter.y;

        if (rawMouseDelta.x || rawMouseDelta.y) {
            SetCursorPos(windowCenter.x, windowCenter.y);
        }

        Point mouseDelta;
        mouseDelta.x = rawMouseDelta.x;
        mouseDelta.y = rawMouseDelta.y;
        
        rawMouseDeltaOld.x = rawMouseDelta.x;
        rawMouseDeltaOld.y = rawMouseDelta.y;

        if (mouseDelta.x != 0 || mouseDelta.y != 0) {
            QueEvent(Platform::EventType::MouseDelta, mouseDelta.x, mouseDelta.y, 0, nullptr);
        }

        if (rawMouseDelta.x != 0 || rawMouseDelta.y != 0) {
            SetCursorPos(windowCenter.x, windowCenter.y);
        }
    }
}

BE_NAMESPACE_END
