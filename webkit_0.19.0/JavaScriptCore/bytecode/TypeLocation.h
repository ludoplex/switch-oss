/*
 * Copyright (C) 2014 Apple Inc. All Rights Reserved.
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

#ifndef TypeLocation_h
#define TypeLocation_h

#include "TypeSet.h"

namespace JSC {

enum TypeProfilerGlobalIDFlags {
    TypeProfilerNeedsUniqueIDGeneration = -1,
    TypeProfilerNoGlobalIDExists = -2,
    TypeProfilerReturnStatement = -3
};

typedef intptr_t GlobalVariableID;

class TypeLocation {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    TypeLocation()
        : m_lastSeenType(TypeNothing)
        , m_divotForFunctionOffsetIfReturnStatement(UINT_MAX)
        , m_instructionTypeSet(TypeSet::create())
        , m_globalTypeSet(nullptr)
    {
    }

    GlobalVariableID m_globalVariableID;
    RuntimeType m_lastSeenType;
    intptr_t m_sourceID;
    unsigned m_divotStart;
    unsigned m_divotEnd;
    unsigned m_divotForFunctionOffsetIfReturnStatement;
    RefPtr<TypeSet> m_instructionTypeSet;
    RefPtr<TypeSet> m_globalTypeSet;
};

} //namespace JSC

#endif //TypeLocation_h
