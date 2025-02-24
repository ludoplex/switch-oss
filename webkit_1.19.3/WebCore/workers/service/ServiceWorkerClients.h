/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(SERVICE_WORKER)

#include "ServiceWorkerClientQueryOptions.h"
#include "ServiceWorkerClientType.h"
#include "ServiceWorkerIdentifier.h"
#include <wtf/Ref.h>
#include <wtf/RefCounted.h>

#if PLATFORM(WKC)
#include <wtf/HashMap.h>
#endif

namespace WebCore {

class DeferredPromise;
class ScriptExecutionContext;
struct ServiceWorkerClientData;
struct ServiceWorkerClientIdentifier;

class ServiceWorkerClients : public RefCounted<ServiceWorkerClients> {
public:
    static Ref<ServiceWorkerClients> create()
    {
        return adoptRef(*new ServiceWorkerClients);
    }

    using ClientQueryOptions = ServiceWorkerClientQueryOptions;

    void get(ScriptExecutionContext&, const String& id, Ref<DeferredPromise>&&);
    void matchAll(ScriptExecutionContext&, const ClientQueryOptions&, Ref<DeferredPromise>&&);
    void openWindow(ScriptExecutionContext&, const String& url, Ref<DeferredPromise>&&);
    void claim(ScriptExecutionContext&, Ref<DeferredPromise>&&);

private:
    ServiceWorkerClients() = default;

    HashMap<DeferredPromise*, Ref<DeferredPromise>> m_pendingPromises;
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
