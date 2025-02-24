/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#if ENABLE(MEDIA_STREAM)

#include "NavigatorUserMediaError.h"

#include <wtf/NeverDestroyed.h>

namespace WebCore {

const AtomicString& NavigatorUserMediaError::permissionDeniedErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> permissionDenied("PermissionDeniedError", AtomicString::ConstructFromLiteral);
    return permissionDenied;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, permissionDenied, 0)
        if (!permissionDenied)
            permissionDenied = new AtomicString("PermissionDeniedError", AtomicString::ConstructFromLiteral);
    return *permissionDenied;
#endif
}

const AtomicString& NavigatorUserMediaError::constraintNotSatisfiedErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> constraintNotSatisfied("ConstraintNotSatisfiedError", AtomicString::ConstructFromLiteral);
    return constraintNotSatisfied;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, constraintNotSatisfied, 0);
    if (!constraintNotSatisfied)
        constraintNotSatisfied = new AtomicString("ConstraintNotSatisfiedError", AtomicString::ConstructFromLiteral);
    return *constraintNotSatisfied;
#endif
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

