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
#include "Core/Str.h"
#include "PlatformUtils/iOS/iOSDevice.h"
#include <sys/types.h>
#include <sys/sysctl.h>

BE_NAMESPACE_BEGIN

struct IOSDeviceModel {
    const char *name;
    int resolutionWidth, resolutionHeight;
};

// Matched with IOSDevice::Type enums
static IOSDeviceModel iOSModels[] = {
    // iPod
    { "iPod Touch 4th generation",  960, 640 },
    { "iPod Touch 5th generation",  1136, 640 },
    { "iPod Touch 6th generation",  1136, 640 },
    { "iPod Touch 7th generation",  1136, 640 },
    // iPhone
    { "iPhone 4",                   960, 640 },
    { "iPhone 4s",                  960, 640 },
    { "iPhone 5",                   1136, 640 },
    { "iPhone 5s",                  1136, 640 },
    { "iPhone 6",                   1334, 750 },
    { "iPhone 6 Plus",              1920, 1080 },
    { "iPhone 6s",                  1334, 750 },
    { "iPhone SE",                  1136, 640 },
    { "iPhone 7",                   1334, 750 },
    { "iPhone 7 Plus",              1920, 1080 },
    { "iPhone 8",                   1334, 750 },
    { "iPhone 8 Plus",              1920, 1080 },
    { "iPhone X",                   2436, 1125 },
    { "iPhone Xr",                  1792, 828 },
    { "iPhone Xs",                  2436, 1125 },
    { "iPhone Xs Max",              2688, 1242 },
    { "iPhone 11",                  1792, 828 },
    { "iPhone 11 Pro",              2436, 1125 },
    { "iPhone 11 Pro Max",          2688, 1242 },
    // iPad
    { "iPad 2",                     1024, 768 },
    { "iPad Mini",                  1024, 768 },
    { "iPad 3",                     2048, 1536 },
    { "iPad 4",                     2048, 1536 },
    { "iPad Air",                   2048, 1536 },
    { "iPad Mini 2",                2048, 1536 },
    { "iPad Mini 3",                2048, 1536 },
    { "iPad Air 2",                 2048, 1536 },
    { "iPad Mini 4",                2048, 1536 },
    { "iPad Pro 12.9",              2732, 2048 },
    { "iPad Pro 9.7",               2048, 1536 },
    { "iPad 5",                     2048, 1536 },
    { "iPad Pro 12.9 (2nd)",        2732, 2048 },
    { "iPad Pro 10.5 (2nd)",        2224, 1668 },
    { "iPad 6",                     2048, 1536 },
    { "iPad Pro 12.9 (3rd)",        2732, 2048 },
    { "iPad Pro 11 (3rd)",          2388, 1668 },
    { "iPad Mini 5",                2048, 1536 },
    { "iPad Air 3",                 2224, 1668 },
};

bool IOSDevice::IsIPod(IOSDevice::Type::Enum deviceType) {
    if (deviceType >= Type::IPodTouch4 && deviceType <= Type::IPodTouch6) { 
        return true;
    }
    return false;
}

bool IOSDevice::IsIPhone(IOSDevice::Type::Enum deviceType) {
    if (deviceType >= Type::IPhone4 && deviceType <= Type::IPhoneXR) { 
        return true;
    }
    return false;
}

bool IOSDevice::IsIPad(IOSDevice::Type::Enum deviceType) {
    if (deviceType >= Type::IPad2 && deviceType <= Type::IPadAir3) { 
        return true;
    }
    return false;
}

void IOSDevice::GetDeviceResolution(IOSDevice::Type::Enum deviceType, int &width, int &height) {
    int index = (int)deviceType;
    assert(index >= 0 && index < Type::UnknownDevice);

    const auto &model = iOSModels[index];
    width = model.resolutionWidth;
    height = model.resolutionHeight;
}

// Refernce: https://www.theiphonewiki.com/wiki/Models
IOSDevice::Type::Enum IOSDevice::GetIOSDeviceType() {
    // default to unknown
    static IOSDevice::Type::Enum deviceType = IOSDevice::Type::UnknownDevice;
    
    // if we've already figured it out, return it
    if (deviceType != IOSDevice::Type::UnknownDevice) {
        return deviceType;
    }
    
    // get the device hardware type string length
    size_t deviceIDLen;
    sysctlbyname("hw.machine", nullptr, &deviceIDLen, nullptr, 0);
    
    // get the device hardware type
    char *deviceID = (char *)malloc(deviceIDLen);
    sysctlbyname("hw.machine", deviceID, &deviceIDLen, nullptr, 0);
    
    // convert to NSString
    BE1::Str deviceIDString([NSString stringWithCString:deviceID encoding:NSUTF8StringEncoding]);
    free(deviceID);
    
    if (!deviceIDString.Cmpn("iPod", 4)) {
        // get major revision number
        int major = deviceIDString[4] - '0';
        
        if (major == 4) {
            deviceType = Type::IPodTouch4;
        } else if (major == 5) {
            deviceType = Type::IPodTouch5;
        } else if (major == 7) {
            deviceType = Type::IPodTouch6;
        } else if (major == 9) {
            deviceType = Type::IPodTouch7;
        } else if (major >= 10) {
            deviceType = Type::IPodTouch7;
        }
    } else if (!deviceIDString.Cmpn("iPad", 4)) {
        // get major revision number
        int major = deviceIDString[4] - '0';
        int minor = deviceIDString[6] - '0';
        
        if (major == 2) {
            if (minor >= 5) {
                deviceType = Type::IPadMini;
            } else {
                deviceType = Type::IPad2;
            }
        } else if (major == 3) {
            if (minor <= 3) {
                deviceType = Type::IPad3;
            } else if (minor >= 4) {
                deviceType = Type::IPad4;
            }
        } else if (major == 4) {
            if (minor >= 8) {
                deviceType = Type::IPadMini3;
            } else if (minor >= 4) {
                deviceType = Type::IPadMini2;
            } else {
                deviceType = Type::IPadAir;
            }
        } else if (major == 5) {
            if (minor >= 3) {
                deviceType = Type::IPadAir2;
            } else {
                deviceType = Type::IPadMini4;
            }
        } else if (major == 6) {
            if (minor >= 11) {
                deviceType = Type::IPad5;
            } else if (minor >= 7) {
                deviceType = Type::IPadPro_12_9;
            } else {
                deviceType = Type::IPadPro_9_7;
            }
        } else if (major == 7) {
            if (minor >= 5) {
                deviceType = Type::IPad6;
            } else if (minor >= 3) {
                deviceType = Type::IPadPro2_10_5;
            } else {
                deviceType = Type::IPadPro2_12_9;
            }
        } else if (major == 8) {
            if (minor >= 5) {
                deviceType = Type::IPadPro3_12_9;
            } else {
                deviceType = Type::IPadPro3_11;
            }
        } else if (major == 11) {
            if (minor >= 3) {
                deviceType = Type::IPadAir3;
            } else {
                deviceType = Type::IPadMini5;
            }
        } else if (major >= 12) {
            CGSize result = [[UIScreen mainScreen] bounds].size;
            float aspectRatio = result.width / result.height;

            if (aspectRatio == 2388.0f / 1668.0f) {
                deviceType = Type::IPadAir3;
            } else if (aspectRatio == 2048.0f / 1536.0f) {
                deviceType = Type::IPadMini5;
            } else if (aspectRatio == 2732.0f / 2048.0f) {
                deviceType = Type::IPadPro3_12_9;
            }
        }
    } else if (!deviceIDString.Cmpn("iPhone", 6)) {
        int major = atoi(&deviceIDString[6]);
        int commaIndex = deviceIDString.Find(',');
        int minor = deviceIDString[commaIndex + 1] - '0';
        
        if (major == 3) {
            deviceType = Type::IPhone4;
        } else if (major == 4) {
            deviceType = Type::IPhone4S;
        } else if (major == 5) {
            deviceType = Type::IPhone5;
        } else if (major == 6) {
            deviceType = Type::IPhone5S;
        } else if (major == 7) {
            if (minor == 1) {
                deviceType = Type::IPhone6Plus;
            } else if (minor == 2) {
                deviceType = Type::IPhone6;
            }
        } else if (major == 8) {
            if (minor == 1) {
                deviceType = Type::IPhone6S;
            } else if (minor == 2) {
                deviceType = Type::IPhone6SPlus;
            } else if (minor == 4) {
                deviceType = Type::IPhoneSE;
            }
        } else if (major == 9) {
            if (minor == 1 || minor == 3) {
                deviceType = Type::IPhone7;
            } else if (minor == 2 || minor == 4) {
                deviceType = Type::IPhone7Plus;
            }
        } else if (major == 10) {
            if (minor == 1 || minor == 4) {
                deviceType = Type::IPhone8;
            } else if (minor == 2 || minor == 5) {
                deviceType = Type::IPhone8Plus;
            } else if (minor == 3 || minor == 6) {
                deviceType = Type::IPhoneX;
            }
        } else if (major == 11) {
            if (minor == 2) {
                deviceType = Type::IPhoneXS;
            } else if (minor == 4 || minor == 6) {
                deviceType = Type::IPhoneXSMax;
            } else if (minor == 8) {
                deviceType = Type::IPhoneXR;
            }
        } else if (major == 12) {
            if (minor == 1) {
                deviceType = Type::IPhone11;
            } else if (minor == 3) {
                deviceType = Type::IPhone11Pro;
            } else if (minor == 5) {
                deviceType = Type::IPhone11ProMax;
            }
        } else if (major >= 13) {
            if ([UIScreen mainScreen].scale > 2.5f) {
                deviceType = Type::IPhone11ProMax;
            } else {
                deviceType = Type::IPhone11Pro;
            }
        }
    } else if (!deviceIDString.Cmpn("x86", 3)) { // simulator
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
            CGSize result = [[UIScreen mainScreen] bounds].size;
            if (result.height >= 586) {
                deviceType = Type::IPhone5;
            } else {
                deviceType = Type::IPhone4S;
            }
        } else {
            if ([[UIScreen mainScreen] scale] > 1.0f) {
                deviceType = Type::IPad3;
            } else {
                deviceType = Type::IPad2;
            }
        }
    }
    
    return deviceType;
}

BE_NAMESPACE_END

