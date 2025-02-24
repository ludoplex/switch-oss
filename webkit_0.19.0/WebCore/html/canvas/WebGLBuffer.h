/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef WebGLBuffer_h
#define WebGLBuffer_h

#include "WebGLSharedObject.h"

#include <wtf/Forward.h>
#include <wtf/Optional.h>

namespace JSC {
class ArrayBuffer;
class ArrayBufferView;
}

namespace WebCore {

class WebGLBuffer final : public WebGLSharedObject {
public:
    virtual ~WebGLBuffer();

    static Ref<WebGLBuffer> create(WebGLRenderingContextBase*);

    bool associateBufferData(GC3Dsizeiptr size);
    bool associateBufferData(JSC::ArrayBuffer*);
    bool associateBufferData(JSC::ArrayBufferView*);
    bool associateBufferSubData(GC3Dintptr offset, JSC::ArrayBuffer*);
    bool associateBufferSubData(GC3Dintptr offset, JSC::ArrayBufferView*);

    void disassociateBufferData();

    GC3Dsizeiptr byteLength() const;
    const JSC::ArrayBuffer* elementArrayBuffer() const { return m_elementArrayBuffer.get(); }

    // Gets the cached max index for the given type if one has been set.
    WTF::Optional<unsigned> getCachedMaxIndex(GC3Denum type);
    // Sets the cached max index for the given type.
    void setCachedMaxIndex(GC3Denum type, unsigned value);

    GC3Denum getTarget() const { return m_target; }
    void setTarget(GC3Denum);

    bool hasEverBeenBound() const { return object() && m_target; }

protected:
    WebGLBuffer(WebGLRenderingContextBase*);

    virtual void deleteObjectImpl(GraphicsContext3D*, Platform3DObject) override;

private:
    virtual bool isBuffer() const override { return true; }

    GC3Denum m_target;

    RefPtr<JSC::ArrayBuffer> m_elementArrayBuffer;
    GC3Dsizeiptr m_byteLength;

    // Optimization for index validation. For each type of index
    // (i.e., UNSIGNED_SHORT), cache the maximum index in the
    // entire buffer.
    //
    // This is sufficient to eliminate a lot of work upon each
    // draw call as long as all bound array buffers are at least
    // that size.
    struct MaxIndexCacheEntry {
        GC3Denum type;
        unsigned maxIndex;
    };
    // OpenGL ES 2.0 only has two valid index types (UNSIGNED_BYTE
    // and UNSIGNED_SHORT) plus one extension (UNSIGNED_INT).
    MaxIndexCacheEntry m_maxIndexCache[4];
    unsigned int m_nextAvailableCacheEntry;

    // Clears all of the cached max indices.
    void clearCachedMaxIndices();

    // Helper function called by the three associateBufferData().
    bool associateBufferDataImpl(const void* data, GC3Dsizeiptr byteLength);
    // Helper function called by the two associateBufferSubData().
    bool associateBufferSubDataImpl(GC3Dintptr offset, const void* data, GC3Dsizeiptr byteLength);
};

} // namespace WebCore

#endif // WebGLBuffer_h
