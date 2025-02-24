/*
 * Copyright (C) 2016 Apple Inc.  All rights reserved.
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

#pragma once

#include "Color.h"
#include "IntRect.h"
#include "IntSize.h"
#include "NativeImage.h"
#include "SharedBuffer.h"

namespace WebCore {

#if USE(CAIRO)
// Due to the pixman 16.16 floating point representation, cairo is not able to handle
// images whose size is bigger than 32768.
static const int cairoMaxImageSize = 32768;
#endif

class ImageBackingStore {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<ImageBackingStore> create(const IntSize& size, bool premultiplyAlpha = true)
    {
        return std::unique_ptr<ImageBackingStore>(new ImageBackingStore(size, premultiplyAlpha));
    }

    static std::unique_ptr<ImageBackingStore> create(const ImageBackingStore& other)
    {
        return std::unique_ptr<ImageBackingStore>(new ImageBackingStore(other));
    }

    NativeImagePtr image() const;

    bool setSize(const IntSize& size)
    {
        if (size.isEmpty())
            return false;

        Vector<char> buffer;
        size_t bufferSize = size.area().unsafeGet() * sizeof(RGBA32);

#if PLATFORM(WKC)
        if (bufferSize > buffer.capacity()) {
            if (!wkcMemoryCheckMemoryAllocatablePeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE)) {
                wkcMemoryNotifyMemoryAllocationErrorPeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE);
                return false;
            }
        }
        if (!buffer.tryReserveCapacity(bufferSize)) {
            wkcMemoryNotifyMemoryAllocationErrorPeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE);
            return false;
        }
#else
        if (!buffer.tryReserveCapacity(bufferSize))
            return false;
#endif

        buffer.grow(bufferSize);
        m_pixels = SharedBuffer::DataSegment::create(WTFMove(buffer));
        m_pixelsPtr = reinterpret_cast<RGBA32*>(const_cast<char*>(m_pixels->data()));
        m_size = size;
        m_frameRect = IntRect(IntPoint(), m_size);
        clear();
        return true;
    }

    void setFrameRect(const IntRect& frameRect)
    {
        ASSERT(!m_size.isEmpty());
        ASSERT(inBounds(frameRect));
        m_frameRect = frameRect;
    }

    const IntSize& size() const { return m_size; }
    const IntRect& frameRect() const { return m_frameRect; }

    void clear()
    {
        memset(m_pixelsPtr, 0, (m_size.area() * sizeof(RGBA32)).unsafeGet());
    }

    void clearRect(const IntRect& rect)
    {
        if (rect.isEmpty() || !inBounds(rect))
            return;

        size_t rowBytes = rect.width() * sizeof(RGBA32);
        RGBA32* start = pixelAt(rect.x(), rect.y());
        for (int i = 0; i < rect.height(); ++i) {
            memset(start, 0, rowBytes);
            start += m_size.width();
        }
    }

    void fillRect(const IntRect &rect, unsigned r, unsigned g, unsigned b, unsigned a)
    {
        if (rect.isEmpty() || !inBounds(rect))
            return;

        RGBA32* start = pixelAt(rect.x(), rect.y());
        RGBA32 pixelValue = this->pixelValue(r, g, b, a);
        for (int i = 0; i < rect.height(); ++i) {
            for (int j = 0; j < rect.width(); ++j)
                start[j] = pixelValue;
            start += m_size.width();
        }
    }

    void repeatFirstRow(const IntRect& rect)
    {
        if (rect.isEmpty() || !inBounds(rect))
            return;

        size_t rowBytes = rect.width() * sizeof(RGBA32);
        RGBA32* src = pixelAt(rect.x(), rect.y());
        RGBA32* dest = src + m_size.width();
        for (int i = 1; i < rect.height(); ++i) {
            memcpy(dest, src, rowBytes);
            dest += m_size.width();
        }
    }

    RGBA32* pixelAt(int x, int y) const
    {
        ASSERT(inBounds(IntPoint(x, y)));
        return m_pixelsPtr + y * m_size.width() + x;
    }

    void setPixel(RGBA32* dest, unsigned r, unsigned g, unsigned b, unsigned a)
    {
        ASSERT(dest);
        *dest = pixelValue(r, g, b, a);
    }

    void setPixel(int x, int y, unsigned r, unsigned g, unsigned b, unsigned a)
    {
        setPixel(pixelAt(x, y), r, g, b, a);
    }

    void blendPixel(RGBA32* dest, unsigned r, unsigned g, unsigned b, unsigned a)
    {
        if (!a)
            return;

        if (a >= 255 || !alphaChannel(*dest)) {
            setPixel(dest, r, g, b, a);
            return;
        }

        if (!m_premultiplyAlpha)
            *dest = makePremultipliedRGBA(redChannel(*dest), greenChannel(*dest), blueChannel(*dest), alphaChannel(*dest), false);

        unsigned d = 255 - a;

        r = fastDivideBy255(r * a + redChannel(*dest) * d);
        g = fastDivideBy255(g * a + greenChannel(*dest) * d);
        b = fastDivideBy255(b * a + blueChannel(*dest) * d);
        a += fastDivideBy255(d * alphaChannel(*dest));

        if (m_premultiplyAlpha)
            *dest = makeRGBA(r, g, b, a);
        else
            *dest = makeUnPremultipliedRGBA(r, g, b, a);
    }

    static bool isOverSize(const IntSize& size)
    {
#if USE(CAIRO)
        // FIXME: this is a workaround to avoid the cairo image size limit, but we should implement support for
        // bigger images. See https://bugs.webkit.org/show_bug.cgi?id=177227.
        //
        // If the image is bigger than the cairo limit it can't be displayed, so we don't even try to decode it.
        if (size.width() > cairoMaxImageSize || size.height() > cairoMaxImageSize)
            return true;
#endif
        static unsigned long long MaxPixels = ((1 << 29) - 1);
        unsigned long long pixels = static_cast<unsigned long long>(size.width()) * static_cast<unsigned long long>(size.height());
        return pixels > MaxPixels;
    }

private:
    ImageBackingStore(const IntSize& size, bool premultiplyAlpha = true)
        : m_premultiplyAlpha(premultiplyAlpha)
    {
        ASSERT(!size.isEmpty() && !isOverSize(size));
        setSize(size);
    }

    ImageBackingStore(const ImageBackingStore& other)
        : m_size(other.m_size)
        , m_premultiplyAlpha(other.m_premultiplyAlpha)
    {
        ASSERT(!m_size.isEmpty() && !isOverSize(m_size));
        Vector<char> buffer;
#if PLATFORM(WKC)
        size_t bufferSize = other.m_pixels->size();

        if (bufferSize > buffer.capacity()) {
            if (!wkcMemoryCheckMemoryAllocatablePeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE)) {
                wkcMemoryNotifyMemoryAllocationErrorPeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE);
                m_size = IntSize();
                return;
            }
        }
        if (!buffer.tryReserveCapacity(bufferSize)) {
            wkcMemoryNotifyMemoryAllocationErrorPeer(bufferSize, WKC_MEMORYALLOC_TYPE_IMAGE);
            m_size = IntSize();
            return;
        }

#endif
        buffer.append(other.m_pixels->data(), other.m_pixels->size());
        m_pixels = SharedBuffer::DataSegment::create(WTFMove(buffer));
        m_pixelsPtr = reinterpret_cast<RGBA32*>(const_cast<char*>(m_pixels->data()));
    }

    bool inBounds(const IntPoint& point) const
    {
        return IntRect(IntPoint(), m_size).contains(point);
    }

    bool inBounds(const IntRect& rect) const
    {
        return IntRect(IntPoint(), m_size).contains(rect);
    }

    RGBA32 pixelValue(unsigned r, unsigned g, unsigned b, unsigned a) const
    {
        if (m_premultiplyAlpha && !a)
            return 0;

        if (m_premultiplyAlpha && a < 255)
            return makePremultipliedRGBA(r, g, b, a, false);

        return makeRGBA(r, g, b, a);
    }

    RefPtr<SharedBuffer::DataSegment> m_pixels;
    RGBA32* m_pixelsPtr { nullptr };
    IntSize m_size;
    IntRect m_frameRect; // This will always just be the entire buffer except for GIF and PNG frames
    bool m_premultiplyAlpha { true };
};

}
