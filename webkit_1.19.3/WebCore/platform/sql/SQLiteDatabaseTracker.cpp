/*
 * Copyright (C) 2010, 2012 Apple Inc. All Rights Reserved.
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
#include "SQLiteDatabaseTracker.h"

#include <mutex>
#include <wtf/Lock.h>

namespace WebCore {

namespace SQLiteDatabaseTracker {

#if !PLATFORM(WKC)
static SQLiteDatabaseTrackerClient* s_staticSQLiteDatabaseTrackerClient = nullptr;
static unsigned s_transactionInProgressCounter = 0;

static Lock transactionInProgressMutex;
#else
WKC_DEFINE_GLOBAL_TYPE_ZERO(SQLiteDatabaseTrackerClient*, s_staticSQLiteDatabaseTrackerClient);
WKC_DEFINE_GLOBAL_UINT_ZERO(s_transactionInProgressCounter);

WKC_DEFINE_GLOBAL_TYPE_ZERO(Lock*, transactionInProgressMutex);
#endif

void setClient(SQLiteDatabaseTrackerClient* client)
{
    ASSERT(client);
    ASSERT(!s_staticSQLiteDatabaseTrackerClient || s_staticSQLiteDatabaseTrackerClient == client);
    s_staticSQLiteDatabaseTrackerClient = client;
}

void incrementTransactionInProgressCount()
{
    if (!s_staticSQLiteDatabaseTrackerClient)
        return;

#if PLATFORM(WKC)
    if (!transactionInProgressMutex)
        transactionInProgressMutex = new Lock();
    std::lock_guard<Lock> lock(*transactionInProgressMutex);
#else
    std::lock_guard<Lock> lock(transactionInProgressMutex);
#endif

    s_transactionInProgressCounter++;
    if (s_transactionInProgressCounter == 1)
        s_staticSQLiteDatabaseTrackerClient->willBeginFirstTransaction();
}

void decrementTransactionInProgressCount()
{
    if (!s_staticSQLiteDatabaseTrackerClient)
        return;

#if PLATFORM(WKC)
    if (!transactionInProgressMutex)
        transactionInProgressMutex = new Lock();
    std::lock_guard<Lock> lock(*transactionInProgressMutex);
#else
    std::lock_guard<Lock> lock(transactionInProgressMutex);
#endif

    ASSERT(s_transactionInProgressCounter);
    s_transactionInProgressCounter--;

    if (!s_transactionInProgressCounter)
        s_staticSQLiteDatabaseTrackerClient->didFinishLastTransaction();
}

#if !ASSERT_DISABLED
bool hasTransactionInProgress()
{
    return !s_staticSQLiteDatabaseTrackerClient || s_transactionInProgressCounter > 0;
}
#endif

} // namespace SQLiteDatabaseTracker

} // namespace WebCore
