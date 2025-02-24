/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2017 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "NinePieceImage.h"

#include "GraphicsContext.h"
#include "ImageQualityController.h"
#include "LengthFunctions.h"
#include "RenderStyle.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/PointerComparison.h>
#include <wtf/text/TextStream.h>

namespace WebCore {

inline DataRef<NinePieceImage::Data>& NinePieceImage::defaultData()
{
    static NeverDestroyed<DataRef<Data>> data { Data::create() };
#if PLATFORM(WKC)
    if (data.isNull())
        data.construct(DataRef<Data> { Data::create() });
#endif
    return data.get();
}

NinePieceImage::NinePieceImage()
    : m_data(defaultData())
{
}

NinePieceImage::NinePieceImage(RefPtr<StyleImage>&& image, LengthBox imageSlices, bool fill, LengthBox borderSlices, LengthBox outset, ENinePieceImageRule horizontalRule, ENinePieceImageRule verticalRule)
    : m_data(Data::create(WTFMove(image), imageSlices, fill, borderSlices, outset, horizontalRule, verticalRule))
{
}

LayoutUnit NinePieceImage::computeSlice(Length length, LayoutUnit width, LayoutUnit slice, LayoutUnit extent)
{
    if (length.isRelative())
        return length.value() * width;
    if (length.isAuto())
        return slice;
    return valueForLength(length, extent);
}

LayoutBoxExtent NinePieceImage::computeSlices(const LayoutSize& size, const LengthBox& lengths, int scaleFactor)
{
    return {
        std::min(size.height(), valueForLength(lengths.top(), size.height())) * scaleFactor,
        std::min(size.width(), valueForLength(lengths.right(), size.width()))  * scaleFactor,
        std::min(size.height(), valueForLength(lengths.bottom(), size.height())) * scaleFactor,
        std::min(size.width(), valueForLength(lengths.left(), size.width()))  * scaleFactor
    };
}

LayoutBoxExtent NinePieceImage::computeSlices(const LayoutSize& size, const LengthBox& lengths, const FloatBoxExtent& widths, const LayoutBoxExtent& slices)
{
    return {
        computeSlice(lengths.top(), widths.top(), slices.top(), size.height()),
        computeSlice(lengths.right(), widths.right(), slices.right(), size.width()),
        computeSlice(lengths.bottom(), widths.bottom(), slices.bottom(), size.height()),
        computeSlice(lengths.left(), widths.left(), slices.left(), size.width())
    };
}

void NinePieceImage::scaleSlicesIfNeeded(const LayoutSize& size, LayoutBoxExtent& slices, float deviceScaleFactor)
{
    LayoutUnit width  = std::max<LayoutUnit>(1 / deviceScaleFactor, slices.left() + slices.right());
    LayoutUnit height = std::max<LayoutUnit>(1 / deviceScaleFactor, slices.top() + slices.bottom());

    float sliceScaleFactor = std::min((float)size.width() / width, (float)size.height() / height);

    if (sliceScaleFactor >= 1)
        return;

    // All slices are reduced by multiplying them by sliceScaleFactor.
    slices.top()    *= sliceScaleFactor;
    slices.right()  *= sliceScaleFactor;
    slices.bottom() *= sliceScaleFactor;
    slices.left()   *= sliceScaleFactor;
}

bool NinePieceImage::isEmptyPieceRect(ImagePiece piece, const LayoutBoxExtent& slices)
{
    if (piece == MiddlePiece)
        return false;

    auto horizontalSide = imagePieceHorizontalSide(piece);
    auto verticalSide = imagePieceVerticalSide(piece);
    return !((!horizontalSide || slices.at(*horizontalSide)) && (!verticalSide || slices.at(*verticalSide)));
}

bool NinePieceImage::isEmptyPieceRect(ImagePiece piece, const Vector<FloatRect>& destinationRects, const Vector<FloatRect>& sourceRects)
{
    return destinationRects[piece].isEmpty() || sourceRects[piece].isEmpty();
}

Vector<FloatRect> NinePieceImage::computeNineRects(const FloatRect& outer, const LayoutBoxExtent& slices, float deviceScaleFactor)
{
    FloatRect inner = outer;
    inner.move(slices.left(), slices.top());
    inner.contract(slices.left() + slices.right(), slices.top() + slices.bottom());
    ASSERT(outer.contains(inner));

    Vector<FloatRect> rects(MaxPiece);

    rects[TopLeftPiece]     = snapRectToDevicePixels(outer.x(),    outer.y(),    slices.left(),  slices.top(),    deviceScaleFactor);
    rects[BottomLeftPiece]  = snapRectToDevicePixels(outer.x(),    inner.maxY(), slices.left(),  slices.bottom(), deviceScaleFactor);
    rects[LeftPiece]        = snapRectToDevicePixels(outer.x(),    inner.y(),    slices.left(),  inner.height(),  deviceScaleFactor);

    rects[TopRightPiece]    = snapRectToDevicePixels(inner.maxX(), outer.y(),    slices.right(), slices.top(),    deviceScaleFactor);
    rects[BottomRightPiece] = snapRectToDevicePixels(inner.maxX(), inner.maxY(), slices.right(), slices.bottom(), deviceScaleFactor);
    rects[RightPiece]       = snapRectToDevicePixels(inner.maxX(), inner.y(),    slices.right(), inner.height(),  deviceScaleFactor);

    rects[TopPiece]         = snapRectToDevicePixels(inner.x(),    outer.y(),    inner.width(),  slices.top(),    deviceScaleFactor);
    rects[BottomPiece]      = snapRectToDevicePixels(inner.x(),    inner.maxY(), inner.width(),  slices.bottom(), deviceScaleFactor);

    rects[MiddlePiece]      = snapRectToDevicePixels(inner.x(),    inner.y(),    inner.width(),  inner.height(),  deviceScaleFactor);
    return rects;
}

FloatSize NinePieceImage::computeSideTileScale(ImagePiece piece, const Vector<FloatRect>& destinationRects, const Vector<FloatRect>& sourceRects)
{
    ASSERT(!isCornerPiece(piece) && !isMiddlePiece(piece));
    if (isEmptyPieceRect(piece, destinationRects, sourceRects))
        return FloatSize(1, 1);

    float scale;
    if (isHorizontalPiece(piece))
        scale = destinationRects[piece].height() / sourceRects[piece].height();
    else
        scale = destinationRects[piece].width() / sourceRects[piece].width();

    return FloatSize(scale, scale);
}

FloatSize NinePieceImage::computeMiddleTileScale(const Vector<FloatSize>& scales, const Vector<FloatRect>& destinationRects, const Vector<FloatRect>& sourceRects, ENinePieceImageRule hRule, ENinePieceImageRule vRule)
{
    FloatSize scale(1, 1);
    if (isEmptyPieceRect(MiddlePiece, destinationRects, sourceRects))
        return scale;

    // Unlike the side pieces, the middle piece can have "stretch" specified in one axis but not the other.
    // In fact the side pieces don't even use the scale factor unless they have a rule other than "stretch".
    if (hRule == StretchImageRule)
        scale.setWidth(destinationRects[MiddlePiece].width() / sourceRects[MiddlePiece].width());
    else if (!isEmptyPieceRect(TopPiece, destinationRects, sourceRects))
        scale.setWidth(scales[TopPiece].width());
    else if (!isEmptyPieceRect(BottomPiece, destinationRects, sourceRects))
        scale.setWidth(scales[BottomPiece].width());

    if (vRule == StretchImageRule)
        scale.setHeight(destinationRects[MiddlePiece].height() / sourceRects[MiddlePiece].height());
    else if (!isEmptyPieceRect(LeftPiece, destinationRects, sourceRects))
        scale.setHeight(scales[LeftPiece].height());
    else if (!isEmptyPieceRect(RightPiece, destinationRects, sourceRects))
        scale.setHeight(scales[RightPiece].height());

    return scale;
}

Vector<FloatSize> NinePieceImage::computeTileScales(const Vector<FloatRect>& destinationRects, const Vector<FloatRect>& sourceRects, ENinePieceImageRule hRule, ENinePieceImageRule vRule)
{
    Vector<FloatSize> scales(MaxPiece, FloatSize(1, 1));

    scales[TopPiece]    = computeSideTileScale(TopPiece,    destinationRects, sourceRects);
    scales[RightPiece]  = computeSideTileScale(RightPiece,  destinationRects, sourceRects);
    scales[BottomPiece] = computeSideTileScale(BottomPiece, destinationRects, sourceRects);
    scales[LeftPiece]   = computeSideTileScale(LeftPiece,   destinationRects, sourceRects);

    scales[MiddlePiece] = computeMiddleTileScale(scales, destinationRects, sourceRects, hRule, vRule);
    return scales;
}

void NinePieceImage::paint(GraphicsContext& graphicsContext, RenderElement* renderer, const RenderStyle& style, const LayoutRect& destination, const LayoutSize& source, float deviceScaleFactor, CompositeOperator op) const
{
    StyleImage* styleImage = image();
    ASSERT(styleImage);
    ASSERT(styleImage->isLoaded());

    LayoutBoxExtent sourceSlices = computeSlices(source, imageSlices(), styleImage->imageScaleFactor());
    LayoutBoxExtent destinationSlices = computeSlices(destination.size(), borderSlices(), style.borderWidth(), sourceSlices);

    scaleSlicesIfNeeded(destination.size(), destinationSlices, deviceScaleFactor);

    Vector<FloatRect> destinationRects = computeNineRects(destination, destinationSlices, deviceScaleFactor);
    Vector<FloatRect> sourceRects = computeNineRects(FloatRect(FloatPoint(), source), sourceSlices, deviceScaleFactor);
    Vector<FloatSize> tileScales = computeTileScales(destinationRects, sourceRects, horizontalRule(), verticalRule());

    RefPtr<Image> image = styleImage->image(renderer, source);
    if (!image)
        return;

    InterpolationQualityMaintainer interpolationMaintainer(graphicsContext, ImageQualityController::interpolationQualityFromStyle(style));
    for (ImagePiece piece = MinPiece; piece < MaxPiece; ++piece) {
        if ((piece == MiddlePiece && !fill()) || isEmptyPieceRect(piece, destinationRects, sourceRects))
            continue;

        if (isCornerPiece(piece)) {
            graphicsContext.drawImage(*image, destinationRects[piece], sourceRects[piece], op);
            continue;
        }

        Image::TileRule hRule = isHorizontalPiece(piece) ? static_cast<Image::TileRule>(horizontalRule()) : Image::StretchTile;
        Image::TileRule vRule = isVerticalPiece(piece) ? static_cast<Image::TileRule>(verticalRule()) : Image::StretchTile;
        graphicsContext.drawTiledImage(*image, destinationRects[piece], sourceRects[piece], tileScales[piece], hRule, vRule, op);
    }
}

inline NinePieceImage::Data::Data()
    : fill(false)
    , horizontalRule(StretchImageRule)
    , verticalRule(StretchImageRule)
{
}

inline NinePieceImage::Data::Data(RefPtr<StyleImage>&& image, LengthBox imageSlices, bool fill, LengthBox borderSlices, LengthBox outset, ENinePieceImageRule horizontalRule, ENinePieceImageRule verticalRule)
    : fill(fill)
    , horizontalRule(horizontalRule)
    , verticalRule(verticalRule)
    , image(WTFMove(image))
    , imageSlices(imageSlices)
    , borderSlices(borderSlices)
    , outset(outset)
{
}

inline NinePieceImage::Data::Data(const Data& other)
    : RefCounted<Data>()
    , fill(other.fill)
    , horizontalRule(other.horizontalRule)
    , verticalRule(other.verticalRule)
    , image(other.image)
    , imageSlices(other.imageSlices)
    , borderSlices(other.borderSlices)
    , outset(other.outset)
{
}

inline Ref<NinePieceImage::Data> NinePieceImage::Data::create()
{
    return adoptRef(*new Data);
}

inline Ref<NinePieceImage::Data> NinePieceImage::Data::create(RefPtr<StyleImage>&& image, LengthBox imageSlices, bool fill, LengthBox borderSlices, LengthBox outset, ENinePieceImageRule horizontalRule, ENinePieceImageRule verticalRule)
{
    return adoptRef(*new Data(WTFMove(image), imageSlices, fill, borderSlices, outset, horizontalRule, verticalRule));
}

Ref<NinePieceImage::Data> NinePieceImage::Data::copy() const
{
    return adoptRef(*new Data(*this));
}

bool NinePieceImage::Data::operator==(const Data& other) const
{
    return arePointingToEqualData(image, other.image)
        && imageSlices == other.imageSlices
        && fill == other.fill
        && borderSlices == other.borderSlices
        && outset == other.outset
        && horizontalRule == other.horizontalRule
        && verticalRule == other.verticalRule;
}

TextStream& operator<<(TextStream& ts, const NinePieceImage& image)
{
    ts << "style-image " << image.image() << " slices " << image.imageSlices();
    return ts;
}

}
