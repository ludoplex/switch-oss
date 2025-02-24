/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once

#include "JSDOMWrapper.h"
#include "TestActiveDOMObject.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestActiveDOMObject : public JSDOMWrapper<TestActiveDOMObject> {
public:
    using Base = JSDOMWrapper<TestActiveDOMObject>;
    static JSTestActiveDOMObject* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestActiveDOMObject>&& impl)
    {
        JSTestActiveDOMObject* ptr = new (NotNull, JSC::allocateCell<JSTestActiveDOMObject>(globalObject->vm().heap)) JSTestActiveDOMObject(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestActiveDOMObject* toWrapped(JSC::VM&, JSC::JSValue);
    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info(), JSC::NonArray);
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
public:
    static const unsigned StructureFlags = JSC::HasStaticPropertyTable | Base::StructureFlags;
protected:
    JSTestActiveDOMObject(JSC::Structure*, JSDOMGlobalObject&, Ref<TestActiveDOMObject>&&);

    void finishCreation(JSC::VM&);
};

class JSTestActiveDOMObjectOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestActiveDOMObject*)
{
    static NeverDestroyed<JSTestActiveDOMObjectOwner> owner;
#if PLATFORM(WKC)
    if (owner.isNull())
        owner.construct();
#endif
    return &owner.get();
}

inline void* wrapperKey(TestActiveDOMObject* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject*, TestActiveDOMObject&);
inline JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestActiveDOMObject* impl) { return impl ? toJS(state, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject*, Ref<TestActiveDOMObject>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::ExecState* state, JSDOMGlobalObject* globalObject, RefPtr<TestActiveDOMObject>&& impl) { return impl ? toJSNewlyCreated(state, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestActiveDOMObject> {
    using WrapperClass = JSTestActiveDOMObject;
    using ToWrappedReturnType = TestActiveDOMObject*;
};

} // namespace WebCore
