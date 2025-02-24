/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Device.h"

#if PLATFORM(IOS)

#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

MGDeviceClass deviceClass()
{
    static MGDeviceClass deviceClass = [] {
        int deviceClassNumber = MGGetSInt32Answer(kMGQDeviceClassNumber, MGDeviceClassInvalid);
        switch (deviceClassNumber) {
        case MGDeviceClassInvalid:
        case MGDeviceClassiPhone:
        case MGDeviceClassiPod:
        case MGDeviceClassiPad:
        case MGDeviceClassAppleTV:
        case MGDeviceClassWatch:
            break;
        default:
            ASSERT_NOT_REACHED();
        }
        return static_cast<MGDeviceClass>(deviceClassNumber);
    }();
    return deviceClass;
}

const String& deviceName()
{
#if TARGET_OS_IOS
    static const NeverDestroyed<String> deviceName = adoptCF(static_cast<CFStringRef>(MGCopyAnswer(kMGQDeviceName, nullptr))).get();
#if PLATFORM(WKC)
    if (deviceName.isNull())
        deviceName.construct(adoptCF(static_cast<CFStringRef>(MGCopyAnswer(kMGQDeviceName, nullptr))).get());
#endif
#else
    static const NeverDestroyed<String> deviceName { "iPhone"_s };
#endif
    return deviceName;
}

bool deviceHasIPadCapability()
{
    static bool deviceHasIPadCapability = MGGetBoolAnswer(kMGQiPadCapability);
    return deviceHasIPadCapability;
}

} // namespace WebCore

#endif // PLATFORM(IOS)
