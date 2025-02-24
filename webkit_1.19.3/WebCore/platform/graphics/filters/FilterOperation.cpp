/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "FilterOperation.h"

#include "AnimationUtilities.h"
#include "CachedResourceLoader.h"
#include "CachedSVGDocumentReference.h"
#include "ColorUtilities.h"
#include "FilterEffect.h"
#include "SVGURIReference.h"
#include <wtf/text/TextStream.h>

#if PLATFORM(WKC)
// Annoyingly, wingdi.h #defines this.
#ifdef PASSTHROUGH
#undef PASSTHROUGH
#endif
#endif

namespace WebCore {
    
bool DefaultFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    
    return representedType() == downcast<DefaultFilterOperation>(operation).representedType();
}

ReferenceFilterOperation::ReferenceFilterOperation(const String& url, const String& fragment)
    : FilterOperation(REFERENCE)
    , m_url(url)
    , m_fragment(fragment)
{
}

ReferenceFilterOperation::~ReferenceFilterOperation() = default;
    
bool ReferenceFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    
    return m_url == downcast<ReferenceFilterOperation>(operation).m_url;
}

void ReferenceFilterOperation::loadExternalDocumentIfNeeded(CachedResourceLoader& cachedResourceLoader, const ResourceLoaderOptions& options)
{
    if (m_cachedSVGDocumentReference)
        return;
    if (!SVGURIReference::isExternalURIReference(m_url, *cachedResourceLoader.document()))
        return;
    m_cachedSVGDocumentReference = std::make_unique<CachedSVGDocumentReference>(m_url);
    m_cachedSVGDocumentReference->load(cachedResourceLoader, options);
}

void ReferenceFilterOperation::setFilterEffect(RefPtr<FilterEffect>&& filterEffect)
{
    m_filterEffect = WTFMove(filterEffect);
}

RefPtr<FilterOperation> BasicColorMatrixFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToPassthrough)
        return BasicColorMatrixFilterOperation::create(WebCore::blend(m_amount, passthroughAmount(), progress), m_type);
        
    const BasicColorMatrixFilterOperation* fromOperation = downcast<BasicColorMatrixFilterOperation>(from);
    double fromAmount = fromOperation ? fromOperation->amount() : passthroughAmount();
    return BasicColorMatrixFilterOperation::create(WebCore::blend(fromAmount, m_amount, progress), m_type);
}

bool BasicColorMatrixFilterOperation::transformColor(FloatComponents& colorComponents) const
{
    switch (m_type) {
    case GRAYSCALE: {
        ColorMatrix matrix = ColorMatrix::grayscaleMatrix(m_amount);
        matrix.transformColorComponents(colorComponents);
        return true;
    }
    case SEPIA: {
        ColorMatrix matrix = ColorMatrix::sepiaMatrix(m_amount);
        matrix.transformColorComponents(colorComponents);
        return true;
    }
    case HUE_ROTATE: {
        ColorMatrix matrix = ColorMatrix::hueRotateMatrix(m_amount);
        matrix.transformColorComponents(colorComponents);
        return true;
    }
    case SATURATE: {
        ColorMatrix matrix = ColorMatrix::saturationMatrix(m_amount);
        matrix.transformColorComponents(colorComponents);
        return true;
    }
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    return false;
}

inline bool BasicColorMatrixFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    const BasicColorMatrixFilterOperation& other = downcast<BasicColorMatrixFilterOperation>(operation);
    return m_amount == other.m_amount;
}

double BasicColorMatrixFilterOperation::passthroughAmount() const
{
    switch (m_type) {
    case GRAYSCALE:
    case SEPIA:
    case HUE_ROTATE:
        return 0;
    case SATURATE:
        return 1;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

RefPtr<FilterOperation> BasicComponentTransferFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToPassthrough)
        return BasicComponentTransferFilterOperation::create(WebCore::blend(m_amount, passthroughAmount(), progress), m_type);
        
    const BasicComponentTransferFilterOperation* fromOperation = downcast<BasicComponentTransferFilterOperation>(from);
    double fromAmount = fromOperation ? fromOperation->amount() : passthroughAmount();
    return BasicComponentTransferFilterOperation::create(WebCore::blend(fromAmount, m_amount, progress), m_type);
}

bool BasicComponentTransferFilterOperation::transformColor(FloatComponents& colorComponents) const
{
    switch (m_type) {
    case OPACITY:
        colorComponents.components[3] *= m_amount;
        return true;
    case INVERT: {
        float oneMinusAmount = 1.f - m_amount;
        colorComponents.components[0] = 1 - (oneMinusAmount + colorComponents.components[0] * (m_amount - oneMinusAmount));
        colorComponents.components[1] = 1 - (oneMinusAmount + colorComponents.components[1] * (m_amount - oneMinusAmount));
        colorComponents.components[2] = 1 - (oneMinusAmount + colorComponents.components[2] * (m_amount - oneMinusAmount));
        return true;
    }
    case CONTRAST: {
        float intercept = -(0.5f * m_amount) + 0.5f;
        colorComponents.components[0] = clampTo<float>(intercept + m_amount * colorComponents.components[0], 0, 1);
        colorComponents.components[1] = clampTo<float>(intercept + m_amount * colorComponents.components[1], 0, 1);
        colorComponents.components[2] = clampTo<float>(intercept + m_amount * colorComponents.components[2], 0, 1);
        return true;
    }
    case BRIGHTNESS:
        colorComponents.components[0] = std::max<float>(m_amount * colorComponents.components[0], 0);
        colorComponents.components[1] = std::max<float>(m_amount * colorComponents.components[1], 0);
        colorComponents.components[2] = std::max<float>(m_amount * colorComponents.components[2], 0);
        return true;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
    return false;
}

inline bool BasicComponentTransferFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    const BasicComponentTransferFilterOperation& other = downcast<BasicComponentTransferFilterOperation>(operation);
    return m_amount == other.m_amount;
}

double BasicComponentTransferFilterOperation::passthroughAmount() const
{
    switch (m_type) {
    case OPACITY:
        return 1;
    case INVERT:
        return 0;
    case CONTRAST:
        return 1;
    case BRIGHTNESS:
        return 1;
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}
    
bool InvertLightnessFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;

    return true;
}
    
RefPtr<FilterOperation> InvertLightnessFilterOperation::blend(const FilterOperation* from, double, bool)
{
    if (from && !from->isSameType(*this))
        return this;

    // This filter is not currently blendable.
    return InvertLightnessFilterOperation::create();
}

bool InvertLightnessFilterOperation::transformColor(FloatComponents& sRGBColorComponents) const
{
    FloatComponents hslComponents = sRGBToHSL(sRGBColorComponents);
    
    // Rotate the hue 180deg.
    hslComponents.components[0] = fmod(hslComponents.components[0] + 0.5f, 1.0f);
    
    // Convert back to RGB.
    sRGBColorComponents = HSLToSRGB(hslComponents);
    
    // Apply the matrix. See rdar://problem/41146650 for how this matrix was derived.
    float matrixValues[20] = {
       -0.770,  0.059, -0.089, 0, 1,
        0.030, -0.741, -0.089, 0, 1,
        0.030,  0.059, -0.890, 0, 1,
        0,      0,      0,     1, 0
    };

    ColorMatrix toDarkModeMatrix(matrixValues);
    toDarkModeMatrix.transformColorComponents(sRGBColorComponents);
    return true;
}

bool InvertLightnessFilterOperation::inverseTransformColor(FloatComponents& sRGBColorComponents) const
{
    FloatComponents rgbComponents = sRGBColorComponents;
    // Apply the matrix.
    float matrixValues[20] = {
        -1.300, -0.097,  0.147, 0, 1.25,
        -0.049, -1.347,  0.146, 0, 1.25,
        -0.049, -0.097, -1.104, 0, 1.25,
         0,      0,      0,     1, 0
    };
    ColorMatrix toLightModeMatrix(matrixValues);
    toLightModeMatrix.transformColorComponents(rgbComponents);

    // Convert to HSL.
    FloatComponents hslComponents = sRGBToHSL(rgbComponents);
    // Hue rotate by 180deg.
    hslComponents.components[0] = fmod(hslComponents.components[0] + 0.5f, 1.0f);
    // And return RGB.
    sRGBColorComponents = HSLToSRGB(hslComponents);
    return true;
}

bool BlurFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    
    return m_stdDeviation == downcast<BlurFilterOperation>(operation).stdDeviation();
}
    
RefPtr<FilterOperation> BlurFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;

    LengthType lengthType = m_stdDeviation.type();

    if (blendToPassthrough)
        return BlurFilterOperation::create(WebCore::blend(m_stdDeviation, Length(lengthType), progress));

    const BlurFilterOperation* fromOperation = downcast<BlurFilterOperation>(from);
    Length fromLength = fromOperation ? fromOperation->m_stdDeviation : Length(lengthType);
    return BlurFilterOperation::create(WebCore::blend(fromLength, m_stdDeviation, progress));
}
    
bool DropShadowFilterOperation::operator==(const FilterOperation& operation) const
{
    if (!isSameType(operation))
        return false;
    const DropShadowFilterOperation& other = downcast<DropShadowFilterOperation>(operation);
    return m_location == other.m_location && m_stdDeviation == other.m_stdDeviation && m_color == other.m_color;
}
    
RefPtr<FilterOperation> DropShadowFilterOperation::blend(const FilterOperation* from, double progress, bool blendToPassthrough)
{
    if (from && !from->isSameType(*this))
        return this;

    if (blendToPassthrough)
        return DropShadowFilterOperation::create(
            WebCore::blend(m_location, IntPoint(), progress),
            WebCore::blend(m_stdDeviation, 0, progress),
            WebCore::blend(m_color, Color(Color::transparent), progress));

    const DropShadowFilterOperation* fromOperation = downcast<DropShadowFilterOperation>(from);
    IntPoint fromLocation = fromOperation ? fromOperation->location() : IntPoint();
    int fromStdDeviation = fromOperation ? fromOperation->stdDeviation() : 0;
    Color fromColor = fromOperation ? fromOperation->color() : Color(Color::transparent);
    
    return DropShadowFilterOperation::create(
        WebCore::blend(fromLocation, m_location, progress),
        WebCore::blend(fromStdDeviation, m_stdDeviation, progress),
        WebCore::blend(fromColor, m_color, progress));
}

TextStream& operator<<(TextStream& ts, const FilterOperation& filter)
{
    switch (filter.type()) {
    case FilterOperation::REFERENCE:
        ts << "reference";
        break;
    case FilterOperation::GRAYSCALE: {
        const auto& colorMatrixFilter = downcast<BasicColorMatrixFilterOperation>(filter);
        ts << "grayscale(" << colorMatrixFilter.amount() << ")";
        break;
    }
    case FilterOperation::SEPIA: {
        const auto& colorMatrixFilter = downcast<BasicColorMatrixFilterOperation>(filter);
        ts << "sepia(" << colorMatrixFilter.amount() << ")";
        break;
    }
    case FilterOperation::SATURATE: {
        const auto& colorMatrixFilter = downcast<BasicColorMatrixFilterOperation>(filter);
        ts << "saturate(" << colorMatrixFilter.amount() << ")";
        break;
    }
    case FilterOperation::HUE_ROTATE: {
        const auto& colorMatrixFilter = downcast<BasicColorMatrixFilterOperation>(filter);
        ts << "hue-rotate(" << colorMatrixFilter.amount() << ")";
        break;
    }
    case FilterOperation::INVERT: {
        const auto& componentTransferFilter = downcast<BasicComponentTransferFilterOperation>(filter);
        ts << "invert(" << componentTransferFilter.amount() << ")";
        break;
    }
    case FilterOperation::APPLE_INVERT_LIGHTNESS: {
        ts << "apple-invert-lightness()";
        break;
    }
    case FilterOperation::OPACITY: {
        const auto& componentTransferFilter = downcast<BasicComponentTransferFilterOperation>(filter);
        ts << "opacity(" << componentTransferFilter.amount() << ")";
        break;
    }
    case FilterOperation::BRIGHTNESS: {
        const auto& componentTransferFilter = downcast<BasicComponentTransferFilterOperation>(filter);
        ts << "brightness(" << componentTransferFilter.amount() << ")";
        break;
    }
    case FilterOperation::CONTRAST: {
        const auto& componentTransferFilter = downcast<BasicComponentTransferFilterOperation>(filter);
        ts << "contrast(" << componentTransferFilter.amount() << ")";
        break;
    }
    case FilterOperation::BLUR: {
        const auto& blurFilter = downcast<BlurFilterOperation>(filter);
        ts << "blur(" << blurFilter.stdDeviation().value() << ")"; // FIXME: should call floatValueForLength() but that's outisde of platform/.
        break;
    }
    case FilterOperation::DROP_SHADOW: {
        const auto& dropShadowFilter = downcast<DropShadowFilterOperation>(filter);
        ts << "drop-shadow(" << dropShadowFilter.x() << " " << dropShadowFilter.y() << " " << dropShadowFilter.location() << " ";
        ts << dropShadowFilter.color() << ")";
        break;
    }
    case FilterOperation::PASSTHROUGH:
        ts << "passthrough";
        break;
    case FilterOperation::DEFAULT: {
        const auto& defaultFilter = downcast<DefaultFilterOperation>(filter);
        ts << "default type=" << (int)defaultFilter.representedType();
        break;
    }
    case FilterOperation::NONE:
        ts << "none";
        break;
    }
    return ts;
}

} // namespace WebCore
