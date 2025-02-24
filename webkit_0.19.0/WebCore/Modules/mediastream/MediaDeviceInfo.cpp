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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaDeviceInfo.h"

#if ENABLE(MEDIA_STREAM)

#include "ContextDestructionObserver.h"
#include "ScriptWrappable.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

Ref<MediaDeviceInfo> MediaDeviceInfo::create(ScriptExecutionContext* context, const String& label, const String& deviceId, const String& groupId, const String& kind)
{
    return adoptRef(*new MediaDeviceInfo(context, label, deviceId, groupId, kind));
}

MediaDeviceInfo::MediaDeviceInfo(ScriptExecutionContext* context, const String& label, const String& deviceId, const String& groupId, const String& kind)
    : ContextDestructionObserver(context)
    , m_label(label)
    , m_deviceId(deviceId)
    , m_groupId(groupId)
    , m_kind(kind)
{
}

const AtomicString& MediaDeviceInfo::audioInputType()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> audioinput("audioinput");
    return audioinput;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, audioinput, new AtomicString("audioinput"));
    return *audioinput;
#endif
}

const AtomicString& MediaDeviceInfo::audioOutputType()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> audiooutput("audiooutput");
    return audiooutput;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, audiooutput, new AtomicString("audiooutput"));
    return *audiooutput;
#endif
}

const AtomicString& MediaDeviceInfo::videoInputType()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> videoinput("videoinput");
    return videoinput;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, videoinput, new AtomicString("videoinput"));
    return *videoinput;
#endif
}

} // namespace WebCore

#endif
