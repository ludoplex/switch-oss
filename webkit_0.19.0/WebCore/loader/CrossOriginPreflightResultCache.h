/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#ifndef CrossOriginPreflightResultCache_h
#define CrossOriginPreflightResultCache_h

#include "URLHash.h"
#include "ResourceHandleTypes.h"
#if !PLATFORM(WKC)
#include <chrono>
#else
#include <wkcchrono.h>
#endif
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class HTTPHeaderMap;
class ResourceResponse;

class CrossOriginPreflightResultCacheItem {
    WTF_MAKE_NONCOPYABLE(CrossOriginPreflightResultCacheItem); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit CrossOriginPreflightResultCacheItem(StoredCredentials credentials)
        : m_credentials(credentials)
    {
    }

    bool parse(const ResourceResponse&, String& errorDescription);
    bool allowsCrossOriginMethod(const String&, String& errorDescription) const;
    bool allowsCrossOriginHeaders(const HTTPHeaderMap&, String& errorDescription) const;
    bool allowsRequest(StoredCredentials, const String& method, const HTTPHeaderMap& requestHeaders) const;

private:
    // FIXME: A better solution to holding onto the absolute expiration time might be
    // to start a timer for the expiration delta that removes this from the cache when
    // it fires.
    std::chrono::steady_clock::time_point m_absoluteExpiryTime;
    StoredCredentials m_credentials;
    HashSet<String> m_methods;
    HashSet<String, CaseFoldingHash> m_headers;
};

class CrossOriginPreflightResultCache {
    WTF_MAKE_NONCOPYABLE(CrossOriginPreflightResultCache); WTF_MAKE_FAST_ALLOCATED;

public:
    WEBCORE_EXPORT static CrossOriginPreflightResultCache& singleton();

    void appendEntry(const String& origin, const URL&, std::unique_ptr<CrossOriginPreflightResultCacheItem>);
    bool canSkipPreflight(const String& origin, const URL&, StoredCredentials, const String& method, const HTTPHeaderMap& requestHeaders);

    WEBCORE_EXPORT void empty();

private:
    friend NeverDestroyed<CrossOriginPreflightResultCache>;
    CrossOriginPreflightResultCache();

    HashMap<std::pair<String, URL>, std::unique_ptr<CrossOriginPreflightResultCacheItem>> m_preflightHashMap;
};

} // namespace WebCore

#endif
