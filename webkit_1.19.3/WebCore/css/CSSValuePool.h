/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#pragma once

#include "CSSInheritedValue.h"
#include "CSSInitialValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSRevertValue.h"
#include "CSSUnsetValue.h"
#include "CSSValueKeywords.h"
#include "ColorHash.h"
#include <utility>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class CSSValueList;

enum class FromSystemFontID { No, Yes };

class CSSValuePool {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static CSSValuePool& singleton();

    RefPtr<CSSValueList> createFontFaceValue(const AtomicString&);
    Ref<CSSPrimitiveValue> createFontFamilyValue(const String&, FromSystemFontID = FromSystemFontID::No);
    Ref<CSSInheritedValue> createInheritedValue() { return m_inheritedValue.get(); }
    Ref<CSSInitialValue> createImplicitInitialValue() { return m_implicitInitialValue.get(); }
    Ref<CSSInitialValue> createExplicitInitialValue() { return m_explicitInitialValue.get(); }
    Ref<CSSUnsetValue> createUnsetValue() { return m_unsetValue.get(); }
    Ref<CSSRevertValue> createRevertValue() { return m_revertValue.get(); }
    Ref<CSSPrimitiveValue> createIdentifierValue(CSSValueID identifier);
    Ref<CSSPrimitiveValue> createIdentifierValue(CSSPropertyID identifier);
    Ref<CSSPrimitiveValue> createColorValue(const Color&);
    Ref<CSSPrimitiveValue> createValue(double value, CSSPrimitiveValue::UnitType);
    Ref<CSSPrimitiveValue> createValue(const String& value, CSSPrimitiveValue::UnitType type) { return CSSPrimitiveValue::create(value, type); }
    Ref<CSSPrimitiveValue> createValue(const Length& value, const RenderStyle& style) { return CSSPrimitiveValue::create(value, style); }
    Ref<CSSPrimitiveValue> createValue(const LengthSize& value, const RenderStyle& style) { return CSSPrimitiveValue::create(value, style); }
    template<typename T> static Ref<CSSPrimitiveValue> createValue(T&& value) { return CSSPrimitiveValue::create(std::forward<T>(value)); }

    void drain();

private:
    CSSValuePool();

    typedef HashMap<Color, RefPtr<CSSPrimitiveValue>> ColorValueCache;
    ColorValueCache m_colorValueCache;

    typedef HashMap<AtomicString, RefPtr<CSSValueList>> FontFaceValueCache;
    FontFaceValueCache m_fontFaceValueCache;

    typedef HashMap<std::pair<String, bool>, RefPtr<CSSPrimitiveValue>> FontFamilyValueCache;
    FontFamilyValueCache m_fontFamilyValueCache;

    friend class WTF::NeverDestroyed<CSSValuePool>;

#if !PLATFORM(WKC)
    LazyNeverDestroyed<CSSInheritedValue> m_inheritedValue;
    LazyNeverDestroyed<CSSInitialValue> m_implicitInitialValue;
    LazyNeverDestroyed<CSSInitialValue> m_explicitInitialValue;
    LazyNeverDestroyed<CSSUnsetValue> m_unsetValue;
    LazyNeverDestroyed<CSSRevertValue> m_revertValue;

    LazyNeverDestroyed<CSSPrimitiveValue> m_transparentColor;
    LazyNeverDestroyed<CSSPrimitiveValue> m_whiteColor;
    LazyNeverDestroyed<CSSPrimitiveValue> m_blackColor;
#else
    LazyNeverDestroyed<CSSInheritedValue, false> m_inheritedValue;
    LazyNeverDestroyed<CSSInitialValue, false> m_implicitInitialValue;
    LazyNeverDestroyed<CSSInitialValue, false> m_explicitInitialValue;
    LazyNeverDestroyed<CSSUnsetValue, false> m_unsetValue;
    LazyNeverDestroyed<CSSRevertValue, false> m_revertValue;

    LazyNeverDestroyed<CSSPrimitiveValue, false> m_transparentColor;
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_whiteColor;
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_blackColor;
#endif

    static const int maximumCacheableIntegerValue = 255;

#if !PLATFORM(WKC)
    LazyNeverDestroyed<CSSPrimitiveValue> m_pixelValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue> m_percentValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue> m_numberValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue> m_identifierValues[numCSSValueKeywords];
#else
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_pixelValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_percentValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_numberValues[maximumCacheableIntegerValue + 1];
    LazyNeverDestroyed<CSSPrimitiveValue, false> m_identifierValues[numCSSValueKeywords];
#endif
};

} // namespace WebCore
