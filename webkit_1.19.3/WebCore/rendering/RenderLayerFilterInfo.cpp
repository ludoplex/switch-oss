/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "RenderLayerFilterInfo.h"

#include "CachedSVGDocument.h"
#include "CachedSVGDocumentReference.h"
#include "FilterEffectRenderer.h"
#include "RenderSVGResourceFilter.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

HashMap<const RenderLayer*, std::unique_ptr<RenderLayer::FilterInfo>>& RenderLayer::FilterInfo::map()
{
    static NeverDestroyed<HashMap<const RenderLayer*, std::unique_ptr<FilterInfo>>> map;
#if PLATFORM(WKC)
    if (map.isNull())
        map.construct();
#endif
    return map;
}

RenderLayer::FilterInfo* RenderLayer::FilterInfo::getIfExists(const RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));

    return layer.m_hasFilterInfo ? map().get(&layer) : nullptr;
}

RenderLayer::FilterInfo& RenderLayer::FilterInfo::get(RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));

    auto& info = map().add(&layer, nullptr).iterator->value;
    if (!info) {
        info = std::make_unique<FilterInfo>(layer);
        layer.m_hasFilterInfo = true;
    }
    return *info;
}

void RenderLayer::FilterInfo::remove(RenderLayer& layer)
{
    ASSERT(layer.m_hasFilterInfo == map().contains(&layer));
    if (!layer.m_hasFilterInfo)
        return;

    map().remove(&layer);
    layer.m_hasFilterInfo = false;
}

RenderLayer::FilterInfo::FilterInfo(RenderLayer& layer)
    : m_layer(layer)
{
}

RenderLayer::FilterInfo::~FilterInfo()
{
    removeReferenceFilterClients();
}

void RenderLayer::FilterInfo::setRenderer(RefPtr<FilterEffectRenderer>&& renderer)
{
    m_renderer = renderer;
}

void RenderLayer::FilterInfo::notifyFinished(CachedResource&)
{
    // FIXME: This really shouldn't have to invalidate layer composition,
    // but tests like css3/filters/effect-reference-delete.html fail if that doesn't happen.
    if (auto* enclosingElement = m_layer.enclosingElement())
        enclosingElement->invalidateStyleAndLayerComposition();
    m_layer.renderer().repaint();
}

void RenderLayer::FilterInfo::updateReferenceFilterClients(const FilterOperations& operations)
{
    removeReferenceFilterClients();
    for (auto& operation : operations.operations()) {
        if (!is<ReferenceFilterOperation>(*operation))
            continue;
        auto& referenceOperation = downcast<ReferenceFilterOperation>(*operation);
        auto* documentReference = referenceOperation.cachedSVGDocumentReference();
        if (auto* cachedSVGDocument = documentReference ? documentReference->document() : nullptr) {
            // Reference is external; wait for notifyFinished().
            cachedSVGDocument->addClient(*this);
            m_externalSVGReferences.append(cachedSVGDocument);
        } else {
            // Reference is internal; add layer as a client so we can trigger filter repaint on SVG attribute change.
            auto* filter = m_layer.renderer().document().getElementById(referenceOperation.fragment());
            if (!filter)
                continue;
            auto* renderer = filter->renderer();
            if (!is<RenderSVGResourceFilter>(renderer))
                continue;
            downcast<RenderSVGResourceFilter>(*renderer).addClientRenderLayer(&m_layer);
            m_internalSVGReferences.append(filter);
        }
    }
}

void RenderLayer::FilterInfo::removeReferenceFilterClients()
{
    for (auto& resourceHandle : m_externalSVGReferences)
        resourceHandle->removeClient(*this);
    m_externalSVGReferences.clear();

    for (auto& filter : m_internalSVGReferences) {
        if (auto* renderer = filter->renderer())
            downcast<RenderSVGResourceContainer>(*renderer).removeClientRenderLayer(&m_layer);
    }
    m_internalSVGReferences.clear();
}

} // namespace WebCore
