/*
 * Copyright (C) 2008, 2014 Apple Inc. All Rights Reserved.
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

#ifndef ThreadGlobalData_h
#define ThreadGlobalData_h

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringHash.h>

#include <wtf/ThreadSpecific.h>
#include <wtf/Threading.h>
using WTF::ThreadSpecific;

namespace WebCore {

    class ThreadTimers;

    struct CachedResourceRequestInitiators;
    struct EventNames;
    struct ICUConverterWrapper;
    struct TECConverterWrapper;

    class ThreadGlobalData {
        WTF_MAKE_NONCOPYABLE(ThreadGlobalData);
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        WEBCORE_EXPORT ThreadGlobalData();
        WEBCORE_EXPORT ~ThreadGlobalData();
        void destroy(); // called on workers to clean up the ThreadGlobalData before the thread exits.

        const CachedResourceRequestInitiators& cachedResourceRequestInitiators() { return *m_cachedResourceRequestInitiators; }
        EventNames& eventNames() { return *m_eventNames; }
        ThreadTimers& threadTimers() { return *m_threadTimers; }

#if !PLATFORM(WKC)
        ICUConverterWrapper& cachedConverterICU() { return *m_cachedConverterICU; }
#endif

#if PLATFORM(MAC)
        TECConverterWrapper& cachedConverterTEC() { return *m_cachedConverterTEC; }
#endif

#if USE(WEB_THREAD)
        void setWebCoreThreadData();
#endif

    private:
        std::unique_ptr<CachedResourceRequestInitiators> m_cachedResourceRequestInitiators;
        std::unique_ptr<EventNames> m_eventNames;
        std::unique_ptr<ThreadTimers> m_threadTimers;

#ifndef NDEBUG
        bool m_isMainThread;
#endif

#if !PLATFORM(WKC)
        std::unique_ptr<ICUConverterWrapper> m_cachedConverterICU;
#endif

#if PLATFORM(MAC)
        std::unique_ptr<TECConverterWrapper> m_cachedConverterTEC;
#endif

#if !PLATFORM(WKC)
        WEBCORE_EXPORT static ThreadSpecific<ThreadGlobalData>* staticData;
#else
        WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(ThreadSpecific<ThreadGlobalData>*, staticData);
#endif
#if USE(WEB_THREAD)
        WEBCORE_EXPORT static ThreadGlobalData* sharedMainThreadStaticData;
#endif
        WEBCORE_EXPORT friend ThreadGlobalData& threadGlobalData();
    };

#if USE(WEB_THREAD)
WEBCORE_EXPORT ThreadGlobalData& threadGlobalData();
#else
WEBCORE_EXPORT ThreadGlobalData& threadGlobalData() PURE_FUNCTION;
#endif
    
} // namespace WebCore

#endif // ThreadGlobalData_h
