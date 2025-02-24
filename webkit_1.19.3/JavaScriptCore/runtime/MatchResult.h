/*
 * Copyright (C) 2012, 2016 Apple Inc. All rights reserved.
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

#pragma once

#include <wtf/NotFound.h>
#include <wtf/PrintStream.h>

namespace JSC {

struct MatchResult;
#if CPU(ARM64) || CPU(X86_64)
using EncodedMatchResult = MatchResult;
#else
using EncodedMatchResult = uint64_t;
#endif

struct MatchResult {
    MatchResult()
        : start(WTF::notFound)
        , end(0)
    {
    }
    
    ALWAYS_INLINE MatchResult(size_t start, size_t end)
        : start(start)
        , end(end)
    {
    }

#if !(CPU(ARM64) || CPU(X86_64))
    ALWAYS_INLINE MatchResult(EncodedMatchResult match)
        : start(bitwise_cast<MatchResult>(match).start)
        , end(bitwise_cast<MatchResult>(match).end)
    {
    }
#endif

    ALWAYS_INLINE static MatchResult failed()
    {
        return MatchResult();
    }

    ALWAYS_INLINE explicit operator bool() const
    {
        return start != WTF::notFound;
    }

    ALWAYS_INLINE bool empty()
    {
        return start == end;
    }
    
    void dump(PrintStream&) const;

    size_t start;
    size_t end;
};

static_assert(sizeof(MatchResult) == sizeof(EncodedMatchResult), "Match result and EncodedMatchResult should be the same size");

} // namespace JSC
