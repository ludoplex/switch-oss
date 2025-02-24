/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSCanvasRenderingContext2D.h"

#if PLATFORM(WKC)
using namespace JSC;
#include "JSNodeCustom.h"
#endif

namespace WebCore {

bool JSCanvasRenderingContext2DOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, JSC::SlotVisitor& visitor)
{
    JSCanvasRenderingContext2D* jsCanvasRenderingContext = jsCast<JSCanvasRenderingContext2D*>(handle.slot()->asCell());
    void* root = WebCore::root(jsCanvasRenderingContext->wrapped().canvas());
    return visitor.containsOpaqueRoot(root);
}

void JSCanvasRenderingContext2D::visitAdditionalChildren(JSC::SlotVisitor& visitor)
{
    visitor.addOpaqueRoot(root(wrapped().canvas()));
}

} // namespace WebCore
