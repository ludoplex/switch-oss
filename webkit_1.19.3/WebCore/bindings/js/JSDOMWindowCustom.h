/*
 *  Copyright (C) 2008, 2009 Apple Inc. All rights reseved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "JSDOMWindow.h"

namespace WebCore {

class AbstractDOMWindow;
class AbstractFrame;

inline JSDOMWindow* asJSDOMWindow(JSC::JSGlobalObject* globalObject)
{
    return JSC::jsCast<JSDOMWindow*>(globalObject);
}

inline const JSDOMWindow* asJSDOMWindow(const JSC::JSGlobalObject* globalObject)
{
    return static_cast<const JSDOMWindow*>(globalObject);
}

enum class DOMWindowType { Local, Remote };

template <DOMWindowType windowType>
bool jsDOMWindowGetOwnPropertySlotRestrictedAccess(JSDOMGlobalObject*, AbstractDOMWindow&, JSC::ExecState&, JSC::PropertyName, JSC::PropertySlot&, const String&);

void addCrossOriginWindowOwnPropertyNames(JSC::ExecState&, JSC::PropertyNameArray&);

} // namespace WebCore
