/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2010 Rob Buis <buis@kde.org>
    Copyright (C) Research In Motion Limited 2010. All rights reserved.

    Based on khtml code by:
    Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
    Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
    Copyright (C) 2002-2003 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Apple Inc.

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

#include "config.h"
#include "SVGRenderStyle.h"

#include "CSSPrimitiveValue.h"
#include "CSSValueList.h"
#include "IntRect.h"
#include "NodeRenderStyle.h"
#include "SVGElement.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

static const SVGRenderStyle& defaultSVGStyle()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<DataRef<SVGRenderStyle>> style(SVGRenderStyle::createDefaultStyle());
    return *style.get().get();
#else
    WKC_DEFINE_STATIC_PTR(SVGRenderStyle*, style, &SVGRenderStyle::createDefaultStyle().leakRef());
    return *style;
#endif
}

Ref<SVGRenderStyle> SVGRenderStyle::createDefaultStyle()
{
    return adoptRef(*new SVGRenderStyle(CreateDefault));
}

SVGRenderStyle::SVGRenderStyle()
    : fill(defaultSVGStyle().fill)
    , stroke(defaultSVGStyle().stroke)
    , text(defaultSVGStyle().text)
    , inheritedResources(defaultSVGStyle().inheritedResources)
    , stops(defaultSVGStyle().stops)
    , misc(defaultSVGStyle().misc)
    , shadowSVG(defaultSVGStyle().shadowSVG)
    , layout(defaultSVGStyle().layout)
    , resources(defaultSVGStyle().resources)
{
    setBitDefaults();
}

SVGRenderStyle::SVGRenderStyle(CreateDefaultType)
    : fill(StyleFillData::create())
    , stroke(StyleStrokeData::create())
    , text(StyleTextData::create())
    , inheritedResources(StyleInheritedResourceData::create())
    , stops(StyleStopData::create())
    , misc(StyleMiscData::create())
    , shadowSVG(StyleShadowSVGData::create())
    , layout(StyleLayoutData::create())
    , resources(StyleResourceData::create())
{
    setBitDefaults();
}

inline SVGRenderStyle::SVGRenderStyle(const SVGRenderStyle& other)
    : RefCounted<SVGRenderStyle>()
    , svg_inherited_flags(other.svg_inherited_flags)
    , svg_noninherited_flags(other.svg_noninherited_flags)
    , fill(other.fill)
    , stroke(other.stroke)
    , text(other.text)
    , inheritedResources(other.inheritedResources)
    , stops(other.stops)
    , misc(other.misc)
    , shadowSVG(other.shadowSVG)
    , layout(other.layout)
    , resources(other.resources)
{
}

Ref<SVGRenderStyle> SVGRenderStyle::copy() const
{
    return adoptRef(*new SVGRenderStyle(*this));
}

SVGRenderStyle::~SVGRenderStyle()
{
}

bool SVGRenderStyle::operator==(const SVGRenderStyle& other) const
{
    return fill == other.fill
        && stroke == other.stroke
        && text == other.text
        && stops == other.stops
        && misc == other.misc
        && shadowSVG == other.shadowSVG
        && layout == other.layout
        && inheritedResources == other.inheritedResources
        && resources == other.resources
        && svg_inherited_flags == other.svg_inherited_flags
        && svg_noninherited_flags == other.svg_noninherited_flags;
}

bool SVGRenderStyle::inheritedNotEqual(const SVGRenderStyle* other) const
{
    return fill != other->fill
        || stroke != other->stroke
        || text != other->text
        || inheritedResources != other->inheritedResources
        || svg_inherited_flags != other->svg_inherited_flags;
}

void SVGRenderStyle::inheritFrom(const SVGRenderStyle* svgInheritParent)
{
    if (!svgInheritParent)
        return;

    fill = svgInheritParent->fill;
    stroke = svgInheritParent->stroke;
    text = svgInheritParent->text;
    inheritedResources = svgInheritParent->inheritedResources;

    svg_inherited_flags = svgInheritParent->svg_inherited_flags;
}

void SVGRenderStyle::copyNonInheritedFrom(const SVGRenderStyle* other)
{
    svg_noninherited_flags = other->svg_noninherited_flags;
    stops = other->stops;
    misc = other->misc;
    shadowSVG = other->shadowSVG;
    layout = other->layout;
    resources = other->resources;
}

Vector<PaintType, 3> SVGRenderStyle::paintTypesForPaintOrder() const
{
    Vector<PaintType, 3> paintOrder;
    switch (this->paintOrder()) {
    case PaintOrderNormal:
        FALLTHROUGH;
    case PaintOrderFill:
        paintOrder.append(PaintTypeFill);
        paintOrder.append(PaintTypeStroke);
        paintOrder.append(PaintTypeMarkers);
        break;
    case PaintOrderFillMarkers:
        paintOrder.append(PaintTypeFill);
        paintOrder.append(PaintTypeMarkers);
        paintOrder.append(PaintTypeStroke);
        break;
    case PaintOrderStroke:
        paintOrder.append(PaintTypeStroke);
        paintOrder.append(PaintTypeFill);
        paintOrder.append(PaintTypeMarkers);
        break;
    case PaintOrderStrokeMarkers:
        paintOrder.append(PaintTypeStroke);
        paintOrder.append(PaintTypeMarkers);
        paintOrder.append(PaintTypeFill);
        break;
    case PaintOrderMarkers:
        paintOrder.append(PaintTypeMarkers);
        paintOrder.append(PaintTypeFill);
        paintOrder.append(PaintTypeStroke);
        break;
    case PaintOrderMarkersStroke:
        paintOrder.append(PaintTypeMarkers);
        paintOrder.append(PaintTypeStroke);
        paintOrder.append(PaintTypeFill);
        break;
    };
    return paintOrder;
}

StyleDifference SVGRenderStyle::diff(const SVGRenderStyle* other) const
{
    // NOTE: All comparisions that may return StyleDifferenceLayout have to go before those who return StyleDifferenceRepaint

    // If kerning changes, we need a relayout, to force SVGCharacterData to be recalculated in the SVGRootInlineBox.
    if (text != other->text)
        return StyleDifferenceLayout;

    // If resources change, we need a relayout, as the presence of resources influences the repaint rect.
    if (resources != other->resources)
        return StyleDifferenceLayout;

    // If markers change, we need a relayout, as marker boundaries are cached in RenderSVGPath.
    if (inheritedResources != other->inheritedResources)
        return StyleDifferenceLayout;

    // All text related properties influence layout.
    if (svg_inherited_flags._textAnchor != other->svg_inherited_flags._textAnchor
        || svg_inherited_flags._writingMode != other->svg_inherited_flags._writingMode
        || svg_inherited_flags._glyphOrientationHorizontal != other->svg_inherited_flags._glyphOrientationHorizontal
        || svg_inherited_flags._glyphOrientationVertical != other->svg_inherited_flags._glyphOrientationVertical
        || svg_noninherited_flags.f._alignmentBaseline != other->svg_noninherited_flags.f._alignmentBaseline
        || svg_noninherited_flags.f._dominantBaseline != other->svg_noninherited_flags.f._dominantBaseline
        || svg_noninherited_flags.f._baselineShift != other->svg_noninherited_flags.f._baselineShift)
        return StyleDifferenceLayout;

    // Text related properties influence layout.
    bool miscNotEqual = misc != other->misc;
    if (miscNotEqual && misc->baselineShiftValue != other->misc->baselineShiftValue)
        return StyleDifferenceLayout;

    // These properties affect the cached stroke bounding box rects.
    if (svg_inherited_flags._capStyle != other->svg_inherited_flags._capStyle
        || svg_inherited_flags._joinStyle != other->svg_inherited_flags._joinStyle)
        return StyleDifferenceLayout;

    // Shadow changes require relayouts, as they affect the repaint rects.
    if (shadowSVG != other->shadowSVG)
        return StyleDifferenceLayout;

    // The x or y properties require relayout.
    if (layout != other->layout)
        return StyleDifferenceLayout; 

    // Some stroke properties, requires relayouts, as the cached stroke boundaries need to be recalculated.
    if (stroke != other->stroke) {
        if (stroke->width != other->stroke->width
            || stroke->paintType != other->stroke->paintType
            || stroke->paintColor != other->stroke->paintColor
            || stroke->paintUri != other->stroke->paintUri
            || stroke->miterLimit != other->stroke->miterLimit
            || stroke->dashArray != other->stroke->dashArray
            || stroke->dashOffset != other->stroke->dashOffset
            || stroke->visitedLinkPaintColor != other->stroke->visitedLinkPaintColor
            || stroke->visitedLinkPaintUri != other->stroke->visitedLinkPaintUri
            || stroke->visitedLinkPaintType != other->stroke->visitedLinkPaintType)
            return StyleDifferenceLayout;

        // Only the stroke-opacity case remains, where we only need a repaint.
        ASSERT(stroke->opacity != other->stroke->opacity);
        return StyleDifferenceRepaint;
    }

    // vector-effect changes require a re-layout.
    if (svg_noninherited_flags.f._vectorEffect != other->svg_noninherited_flags.f._vectorEffect)
        return StyleDifferenceLayout;

    // NOTE: All comparisions below may only return StyleDifferenceRepaint

    // Painting related properties only need repaints. 
    if (miscNotEqual) {
        if (misc->floodColor != other->misc->floodColor
            || misc->floodOpacity != other->misc->floodOpacity
            || misc->lightingColor != other->misc->lightingColor)
            return StyleDifferenceRepaint;
    }

    // If fill changes, we just need to repaint. Fill boundaries are not influenced by this, only by the Path, that RenderSVGPath contains.
    if (fill->paintType != other->fill->paintType || fill->paintColor != other->fill->paintColor
        || fill->paintUri != other->fill->paintUri || fill->opacity != other->fill->opacity)
        return StyleDifferenceRepaint;

    // If gradient stops change, we just need to repaint. Style updates are already handled through RenderSVGGradientSTop.
    if (stops != other->stops)
        return StyleDifferenceRepaint;

    // Changes of these flags only cause repaints.
    if (svg_inherited_flags._colorRendering != other->svg_inherited_flags._colorRendering
        || svg_inherited_flags._shapeRendering != other->svg_inherited_flags._shapeRendering
        || svg_inherited_flags._clipRule != other->svg_inherited_flags._clipRule
        || svg_inherited_flags._fillRule != other->svg_inherited_flags._fillRule
        || svg_inherited_flags._colorInterpolation != other->svg_inherited_flags._colorInterpolation
        || svg_inherited_flags._colorInterpolationFilters != other->svg_inherited_flags._colorInterpolationFilters)
        return StyleDifferenceRepaint;

    if (svg_noninherited_flags.f.bufferedRendering != other->svg_noninherited_flags.f.bufferedRendering)
        return StyleDifferenceRepaint;

    if (svg_noninherited_flags.f.maskType != other->svg_noninherited_flags.f.maskType)
        return StyleDifferenceRepaint;

    return StyleDifferenceEqual;
}

}
