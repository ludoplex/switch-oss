/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Google, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
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

#ifndef FrameLoaderStateMachine_h
#define FrameLoaderStateMachine_h

#include <wtf/Noncopyable.h>

namespace WebCore {

// Encapsulates a state machine for FrameLoader. Note that this is different from FrameState,
// which stores the state of the current load that FrameLoader is executing.
class FrameLoaderStateMachine {
    WTF_MAKE_NONCOPYABLE(FrameLoaderStateMachine);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    FrameLoaderStateMachine();

    // Once a load has been committed, the state may
    // alternate between CommittedFirstRealLoad and FirstLayoutDone.
    // Otherwise, the states only go down the list.
    enum State {
        CreatingInitialEmptyDocument,
        DisplayingInitialEmptyDocument,
        DisplayingInitialEmptyDocumentPostCommit,
        CommittedFirstRealLoad,
        FirstLayoutDone
    };

    WEBCORE_EXPORT bool committingFirstRealLoad() const;
    bool committedFirstRealDocumentLoad() const;
    bool creatingInitialEmptyDocument() const;
    WEBCORE_EXPORT bool isDisplayingInitialEmptyDocument() const;
    WEBCORE_EXPORT bool firstLayoutDone() const;
    void advanceTo(State);

private:
    State m_state;
};

} // namespace WebCore

#endif // FrameLoaderStateMachine_h
