/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "SourceAlpha.h"

#include "Color.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "TextStream.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

Ref<SourceAlpha> SourceAlpha::create(FilterEffect& sourceEffect)
{
    return adoptRef(*new SourceAlpha(sourceEffect));
}

const AtomicString& SourceAlpha::effectName()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, s_effectName, ("SourceAlpha", AtomicString::ConstructFromLiteral));
    return s_effectName;
}

void SourceAlpha::determineAbsolutePaintRect()
{
    inputEffect(0)->determineAbsolutePaintRect();
    setAbsolutePaintRect(inputEffect(0)->absolutePaintRect());
}

void SourceAlpha::platformApplySoftware()
{
    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;
    GraphicsContext* filterContext = resultImage->context();

    ImageBuffer* imageBuffer = inputEffect(0)->asImageBuffer();
#if PLATFORM(WKC)
    if (!imageBuffer)
        return;
#else
    ASSERT(imageBuffer);
#endif

    FloatRect imageRect(FloatPoint(), absolutePaintRect().size());
    filterContext->fillRect(imageRect, Color::black, ColorSpaceDeviceRGB);
    filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, IntPoint(), CompositeDestinationIn);
}

void SourceAlpha::dump()
{
}

TextStream& SourceAlpha::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[SourceAlpha]\n";
    return ts;
}

SourceAlpha::SourceAlpha(FilterEffect& sourceEffect)
    : FilterEffect(sourceEffect.filter())
{
    setOperatingColorSpace(sourceEffect.operatingColorSpace());
    inputEffects().append(&sourceEffect);
}

} // namespace WebCore
