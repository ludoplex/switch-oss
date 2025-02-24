/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2009 Adam Barth. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NavigationScheduler_h
#define NavigationScheduler_h

#include "FrameLoaderTypes.h"
#include "Timer.h"
#include <wtf/Forward.h>

namespace WebCore {

class Document;
class FormSubmission;
class Frame;
class ScheduledNavigation;
class SecurityOrigin;
class SubstituteData;
class URL;

class NavigationDisabler {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    NavigationDisabler()
    {
        s_navigationDisableCount++;
    }
    ~NavigationDisabler()
    {
        ASSERT(s_navigationDisableCount);
        s_navigationDisableCount--;
    }
    static bool isNavigationAllowed() { return !s_navigationDisableCount; }

private:
#if !PLATFORM(WKC)
    static unsigned s_navigationDisableCount;
#else
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(unsigned, s_navigationDisableCount);
#endif
};

class NavigationScheduler {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit NavigationScheduler(Frame&);
    ~NavigationScheduler();

    bool redirectScheduledDuringLoad();
    bool locationChangePending();

    void scheduleRedirect(Document* initiatingDocument, double delay, const URL&);
    void scheduleLocationChange(Document* initiatingDocument, SecurityOrigin&, const URL&, const String& referrer, LockHistory = LockHistory::Yes, LockBackForwardList = LockBackForwardList::Yes);
    void scheduleFormSubmission(PassRefPtr<FormSubmission>);
    void scheduleRefresh(Document* initiatingDocument);
    void scheduleHistoryNavigation(int steps);
    void scheduleSubstituteDataLoad(const URL& baseURL, const SubstituteData&);
    void schedulePageBlock(Document& originDocument);

    void startTimer();

    void cancel(bool newLoadInProgress = false);
    void clear();

private:
    bool shouldScheduleNavigation() const;
    bool shouldScheduleNavigation(const URL&) const;

    void timerFired();
    void schedule(std::unique_ptr<ScheduledNavigation>);

    static LockBackForwardList mustLockBackForwardList(Frame& targetFrame);

    Frame& m_frame;
    Timer m_timer;
    std::unique_ptr<ScheduledNavigation> m_redirect;
};

} // namespace WebCore

#endif // NavigationScheduler_h
