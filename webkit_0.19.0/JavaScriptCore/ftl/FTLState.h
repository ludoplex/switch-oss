/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
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

#ifndef FTLState_h
#define FTLState_h

#if ENABLE(FTL_JIT)

#include "DFGCommon.h"
#include "DFGGraph.h"
#include "FTLAbbreviations.h"
#include "FTLGeneratedFunction.h"
#include "FTLInlineCacheDescriptor.h"
#include "FTLJITCode.h"
#include "FTLJITFinalizer.h"
#include "FTLJSCall.h"
#include "FTLJSCallVarargs.h"
#include "FTLStackMaps.h"
#include "FTLState.h"
#include <wtf/Noncopyable.h>

namespace JSC { namespace FTL {

inline bool verboseCompilationEnabled()
{
    return DFG::verboseCompilationEnabled(DFG::FTLMode);
}

inline bool shouldDumpDisassembly()
{
    return DFG::shouldDumpDisassembly(DFG::FTLMode);
}

class State {
    WTF_MAKE_NONCOPYABLE(State);
    
public:
    State(DFG::Graph& graph);
    ~State();
    
    // None of these things is owned by State. It is the responsibility of
    // FTL phases to properly manage the lifecycle of the module and function.
    DFG::Graph& graph;
    LContext context;
    LModule module;
    LValue function;
    bool allocationFailed { false }; // Throw out the compilation once LLVM returns.
    RefPtr<JITCode> jitCode;
    GeneratedFunction generatedFunction;
    JITFinalizer* finalizer;
    unsigned handleStackOverflowExceptionStackmapID;
    unsigned handleExceptionStackmapID;
    unsigned capturedStackmapID;
    unsigned varargsSpillSlotsStackmapID;
    SegmentedVector<GetByIdDescriptor> getByIds;
    SegmentedVector<PutByIdDescriptor> putByIds;
    SegmentedVector<CheckInDescriptor> checkIns;
    Vector<JSCall> jsCalls;
    Vector<JSCallVarargs> jsCallVarargses;
    Vector<CString> codeSectionNames;
    Vector<CString> dataSectionNames;
    void* unwindDataSection;
    size_t unwindDataSectionSize;
    RefPtr<DataSection> stackmapsSection;
    
    void dumpState(const char* when);
    void dumpState(LModule, const char* when);

    HashSet<CString> nativeLoadedLibraries;

#if ENABLE(FTL_NATIVE_CALL_INLINING)
    HashMap<CString, CString> symbolTable;
#endif
};

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

#endif // FTLState_h

