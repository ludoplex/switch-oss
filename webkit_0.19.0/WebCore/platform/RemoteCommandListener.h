/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef RemoteCommandListener_h
#define RemoteCommandListener_h

#include "PlatformMediaSession.h"

namespace WebCore {

class RemoteCommandListenerClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~RemoteCommandListenerClient() { }
    virtual void didReceiveRemoteControlCommand(PlatformMediaSession::RemoteControlCommandType, const PlatformMediaSession::RemoteCommandArgument*) = 0;
    virtual bool supportsSeeking() const = 0;
};

class RemoteCommandListener {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    WEBCORE_EXPORT static std::unique_ptr<RemoteCommandListener> create(RemoteCommandListenerClient&);
    RemoteCommandListener(RemoteCommandListenerClient& client) : m_client(client) { }
    virtual ~RemoteCommandListener() { }

    virtual void updateSupportedCommands() { }

    RemoteCommandListenerClient& client() const { return m_client; }

protected:
    RemoteCommandListenerClient& m_client;
};

}

#endif
