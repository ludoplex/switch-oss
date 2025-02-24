/*
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
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
#include "JSCPoison.h"

#include "Options.h"
#include <mutex>
#include <wtf/HashSet.h>

namespace JSC {

#define DEFINE_POISON(poisonID) \
    uintptr_t POISON_KEY_NAME(poisonID);
FOR_EACH_JSC_POISON(DEFINE_POISON)

void initializePoison()
{
#if PLATFORM(WKC)
    WKC_DEFINE_GLOBAL_BOOL(initializeOnceFlag, false);
    if (!initializeOnceFlag) {
        initializeOnceFlag = true;
#else
    static std::once_flag initializeOnceFlag;
    std::call_once(initializeOnceFlag, [] {
#endif
        if (!Options::usePoisoning())
            return;

#define INITIALIZE_POISON(poisonID) \
    POISON_KEY_NAME(poisonID) = makePoison();

        FOR_EACH_JSC_POISON(INITIALIZE_POISON)
#if PLATFORM(WKC)
    }
#else
    });
#endif
}

} // namespace JSC

