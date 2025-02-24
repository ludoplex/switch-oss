/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "CSSValue.h"

namespace WebCore {

class CSSInheritedValue final : public CSSValue {
public:
    String customCSSText() const;

    bool equals(const CSSInheritedValue&) const { return true; }

#if COMPILER(MSVC)
    // FIXME: This should be private, but for some reason MSVC then fails to invoke it from LazyNeverDestroyed::construct.
public:
#else
private:
#if PLATFORM(WKC)
    friend class LazyNeverDestroyed<CSSInheritedValue, false>;
#else
    friend class LazyNeverDestroyed<CSSInheritedValue>;
#endif
#endif
    CSSInheritedValue()
        : CSSValue(InheritedClass)
    {
    }
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CSS_VALUE(CSSInheritedValue, isInheritedValue())
