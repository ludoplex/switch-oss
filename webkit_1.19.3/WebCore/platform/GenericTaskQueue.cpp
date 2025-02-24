/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "GenericTaskQueue.h"

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

TaskDispatcher<Timer>::TaskDispatcher()
{
}

void TaskDispatcher<Timer>::postTask(Function<void()>&& function)
{
    m_pendingTasks.append(WTFMove(function));
    pendingDispatchers().append(makeWeakPtr(*this));
    if (!sharedTimer().isActive())
        sharedTimer().startOneShot(0_s);
}

Timer& TaskDispatcher<Timer>::sharedTimer()
{
    ASSERT(isMainThread());
    static NeverDestroyed<Timer> timer([] { TaskDispatcher<Timer>::sharedTimerFired(); });
#if PLATFORM(WKC)
    if (timer.isNull())
        timer.construct([] { TaskDispatcher<Timer>::sharedTimerFired(); });
#endif
    return timer.get();
}

void TaskDispatcher<Timer>::sharedTimerFired()
{
    ASSERT(!sharedTimer().isActive());
    ASSERT(!pendingDispatchers().isEmpty());

    // Copy the pending events first because we don't want to process synchronously the new events
    // queued by the JS events handlers that are executed in the loop below.
    Deque<WeakPtr<TaskDispatcher<Timer>>> queuedDispatchers = WTFMove(pendingDispatchers());
    while (!queuedDispatchers.isEmpty()) {
        WeakPtr<TaskDispatcher<Timer>> dispatcher = queuedDispatchers.takeFirst();
        if (!dispatcher)
            continue;
        dispatcher->dispatchOneTask();
    }
}

Deque<WeakPtr<TaskDispatcher<Timer>>>& TaskDispatcher<Timer>::pendingDispatchers()
{
    ASSERT(isMainThread());
    static NeverDestroyed<Deque<WeakPtr<TaskDispatcher<Timer>>>> dispatchers;
#if PLATFORM(WKC)
    if (dispatchers.isNull())
        dispatchers.construct();
#endif
    return dispatchers.get();
}

void TaskDispatcher<Timer>::dispatchOneTask()
{
    ASSERT(!m_pendingTasks.isEmpty());
    auto task = m_pendingTasks.takeFirst();
    task();
}

}

