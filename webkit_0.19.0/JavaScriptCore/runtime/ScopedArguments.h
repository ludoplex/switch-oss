/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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

#ifndef ScopedArguments_h
#define ScopedArguments_h

#include "GenericArguments.h"
#include "JSLexicalEnvironment.h"

namespace JSC {

// This is an Arguments-class object that we create when you say "arguments" inside a function,
// and one or more of the arguments may be captured in the function's activation. The function
// will copy its formally declared arguments into the activation and then create this object. This
// object will store the overflow arguments, if there are any. This object will use the symbol
// table's ScopedArgumentsTable and the activation, or its overflow storage, to handle all indexed
// lookups.
class ScopedArguments : public GenericArguments<ScopedArguments> {
private:
    ScopedArguments(VM&, Structure*, unsigned totalLength);
    void finishCreation(VM&, JSFunction* callee, ScopedArgumentsTable*, JSLexicalEnvironment*);
    using Base = GenericArguments<ScopedArguments>;

public:
    // Creates an arguments object but leaves it uninitialized. This is dangerous if we GC right
    // after allocation.
    static ScopedArguments* createUninitialized(VM&, Structure*, JSFunction* callee, ScopedArgumentsTable*, JSLexicalEnvironment*, unsigned totalLength);
    
    // Creates an arguments object and initializes everything to the empty value. Use this if you
    // cannot guarantee that you'll immediately initialize all of the elements.
    static ScopedArguments* create(VM&, Structure*, JSFunction* callee, ScopedArgumentsTable*, JSLexicalEnvironment*, unsigned totalLength);
    
    // Creates an arguments object by copying the arguments from the stack.
    static ScopedArguments* createByCopying(ExecState*, ScopedArgumentsTable*, JSLexicalEnvironment*);
    
    // Creates an arguments object by copying the arguments from a well-defined stack location.
    static ScopedArguments* createByCopyingFrom(VM&, Structure*, Register* argumentsStart, unsigned totalLength, JSFunction* callee, ScopedArgumentsTable*, JSLexicalEnvironment*);
    
    static void visitChildren(JSCell*, SlotVisitor&);
    
    uint32_t internalLength() const
    {
        return m_totalLength;
    }
    
    uint32_t length(ExecState* exec) const
    {
        if (UNLIKELY(m_overrodeThings))
            return get(exec, exec->propertyNames().length).toUInt32(exec);
        return internalLength();
    }
    
    bool canAccessIndexQuickly(uint32_t i) const
    {
        if (i >= m_totalLength)
            return false;
        unsigned namedLength = m_table->length();
        if (i < namedLength)
            return !!m_table->get(i);
        return !!overflowStorage()[i - namedLength].get();
    }

    bool canAccessArgumentIndexQuicklyInDFG(uint32_t i) const
    {
        return canAccessIndexQuickly(i);
    }
    
    JSValue getIndexQuickly(uint32_t i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(canAccessIndexQuickly(i));
        unsigned namedLength = m_table->length();
        if (i < namedLength)
            return m_scope->variableAt(m_table->get(i)).get();
        return overflowStorage()[i - namedLength].get();
    }

    void setIndexQuickly(VM& vm, uint32_t i, JSValue value)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(canAccessIndexQuickly(i));
        unsigned namedLength = m_table->length();
        if (i < namedLength)
            m_scope->variableAt(m_table->get(i)).set(vm, m_scope.get(), value);
        else
            overflowStorage()[i - namedLength].set(vm, this, value);
    }

    WriteBarrier<JSFunction>& callee()
    {
        return m_callee;
    }
    
    bool overrodeThings() const { return m_overrodeThings; }
    void overrideThings(VM&);
    void overrideThingsIfNecessary(VM&);
    void overrideArgument(VM&, uint32_t index);
    
    void copyToArguments(ExecState*, VirtualRegister firstElementDest, unsigned offset, unsigned length);

    DECLARE_INFO;
    
    static Structure* createStructure(VM&, JSGlobalObject*, JSValue prototype);
    
    static ptrdiff_t offsetOfOverrodeThings() { return OBJECT_OFFSETOF(ScopedArguments, m_overrodeThings); }
    static ptrdiff_t offsetOfTotalLength() { return OBJECT_OFFSETOF(ScopedArguments, m_totalLength); }
    static ptrdiff_t offsetOfTable() { return OBJECT_OFFSETOF(ScopedArguments, m_table); }
    static ptrdiff_t offsetOfScope() { return OBJECT_OFFSETOF(ScopedArguments, m_scope); }
    
    static size_t overflowStorageOffset()
    {
        return WTF::roundUpToMultipleOf<sizeof(WriteBarrier<Unknown>)>(sizeof(ScopedArguments));
    }
    
    static size_t allocationSize(unsigned overflowArgumentsLength)
    {
        return overflowStorageOffset() + sizeof(WriteBarrier<Unknown>) * overflowArgumentsLength;
    }

private:
    WriteBarrier<Unknown>* overflowStorage() const
    {
        return bitwise_cast<WriteBarrier<Unknown>*>(
            bitwise_cast<char*>(this) + overflowStorageOffset());
    }
    
    
    bool m_overrodeThings; // True if length, callee, and caller are fully materialized in the object.
    unsigned m_totalLength; // The length of declared plus overflow arguments.
    WriteBarrier<JSFunction> m_callee;
    WriteBarrier<ScopedArgumentsTable> m_table;
    WriteBarrier<JSLexicalEnvironment> m_scope;
};

} // namespace JSC

#endif // ScopedArguments_h

