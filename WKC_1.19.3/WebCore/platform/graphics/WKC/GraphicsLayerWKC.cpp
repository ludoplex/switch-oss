/*
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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

#include "GraphicsLayer.h"

#include "GraphicsContext.h"
#include "FloatRect.h"
#include "FloatSize.h"

#include "helpers/privates/WKCGraphicsLayerPrivate.h"

#include "AnimationWKC.h"

#include "NotImplemented.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

namespace WebCore {

bool GraphicsLayer::supportsBackgroundColorContent()
{
    return true;
}

WKC::GraphicsLayerPrivate* GraphicsLayerWKC_wkc(const WebCore::GraphicsLayer* layer);

class GraphicsLayerWKC : public GraphicsLayer
{
public:
    GraphicsLayerWKC(GraphicsLayer::Type type, GraphicsLayerClient& client);

    virtual ~GraphicsLayerWKC();

    virtual void addChild(GraphicsLayer*) override;
    virtual void addChildAtIndex(GraphicsLayer*, int index) override;
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling) override;
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling) override;
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild) override;
    virtual void removeFromParent() override;

    virtual void setNeedsDisplay() override;
    virtual void setNeedsDisplayInRect(const FloatRect&, ShouldClipToLayer = ClipToLayer) override;
    virtual void setContentsNeedsDisplay() override;

    virtual void setContentsOpaque(bool b) override;
    virtual void setBackfaceVisibility(bool b) override;
    virtual void setOpacity(float opacity) override;
    virtual void setDrawsContent(bool drawsContent) override;

    virtual void setSize(const FloatSize& size) override;
    virtual void setTransform(const TransformationMatrix& t) override;
    virtual void setChildrenTransform(const TransformationMatrix& t) override;
    virtual void setAnchorPoint(const FloatPoint3D& p) override;
    virtual void setPosition(const FloatPoint& p) override;

    virtual void setContentsToImage(Image*) override;
    virtual void setContentsToSolidColor(const Color&) override;
    virtual void setContentsToPlatformLayer(PlatformLayer*, ContentsLayerPurpose) override;

    virtual ContentsLayerPurpose contentsLayerPurpose() const override;

    virtual bool setFilters(const FilterOperations&) override { return false; }

    virtual PlatformLayer* platformLayer() const override;

    virtual void flushCompositingState(const FloatRect& clipRect) override;
    virtual void flushCompositingStateForThisLayerOnly() override;
    virtual bool canThrottleLayerFlush() const override { return false; }

    virtual bool addAnimation(const KeyframeValueList&, const FloatSize&, const Animation*, const String&, double) override;
    virtual void pauseAnimation(const String&, double) override;
    virtual void removeAnimation(const String&) override;

    inline WKC::GraphicsLayerPrivate* wkc() const { return m_wkc; }

private:
    WKC::GraphicsLayerPrivate* m_wkc;
    PlatformLayer* m_activeLayer;
    ContentsLayerPurpose m_contentsLayerPurpose;

    bool syncAnimations(MonotonicTime);
    void scheduleLayerFlush();
    void layerFlushTimerFired();

    AnimationsWKC m_animations;
    MonotonicTime m_animationStartTime;
    Timer m_layerFlushTimer;
};

GraphicsLayerWKC::GraphicsLayerWKC(GraphicsLayer::Type type, GraphicsLayerClient& client)
    : GraphicsLayer(type, client)
    , m_wkc(new WKC::GraphicsLayerPrivate(this))
    , m_contentsLayerPurpose(NoContentsLayer)
    , m_layerFlushTimer(*this, &GraphicsLayerWKC::layerFlushTimerFired)
{
    m_activeLayer = 0;
}

GraphicsLayerWKC::~GraphicsLayerWKC()
{
    GraphicsLayer::willBeDestroyed();
    delete m_wkc;
}

void
GraphicsLayerWKC::addChild(GraphicsLayer* childlayer)
{
    bool changed = (childlayer->parent() != this);
    GraphicsLayer::addChild(childlayer);
    if (changed)
        GraphicsLayerWKC_wkc(childlayer)->layerDidAttachToTree(m_wkc);
}

void 
GraphicsLayerWKC::addChildAtIndex(GraphicsLayer* childlayer, int index)
{
    bool changed = (childlayer->parent() != this);
    GraphicsLayer::addChildAtIndex(childlayer, index);
    if (changed)
        GraphicsLayerWKC_wkc(childlayer)->layerDidAttachToTree(m_wkc);
}

void 
GraphicsLayerWKC::addChildAbove(GraphicsLayer* childlayer, GraphicsLayer* sibling)
{
    bool changed = (childlayer->parent() != this);
    GraphicsLayer::addChildAbove(childlayer, sibling);
    if (changed)
        GraphicsLayerWKC_wkc(childlayer)->layerDidAttachToTree(m_wkc);
}

void 
GraphicsLayerWKC::addChildBelow(GraphicsLayer* childlayer, GraphicsLayer* sibling)
{
    bool changed = (childlayer->parent() != this);
    GraphicsLayer::addChildBelow(childlayer, sibling);
    if (changed)
        GraphicsLayerWKC_wkc(childlayer)->layerDidAttachToTree(m_wkc);
}

bool 
GraphicsLayerWKC::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    bool changed = (newChild->parent() != this);
    bool replaced = GraphicsLayer::replaceChild(oldChild, newChild);
    if (replaced) {
        GraphicsLayerWKC_wkc(oldChild)->layerDidDetachFromTree();
        if (changed)
            GraphicsLayerWKC_wkc(newChild)->layerDidAttachToTree(m_wkc);
    }
    return replaced;
}

void 
GraphicsLayerWKC::removeFromParent()
{
    bool changed = (parent() != 0);
    GraphicsLayer::removeFromParent();
    if (changed)
        m_wkc->layerDidDetachFromTree();
}

void
GraphicsLayerWKC::setNeedsDisplay()
{
    m_wkc->setNeedsDisplay();
    bool added = m_wkc->addDirtyRect();
    if (added) {
        client().notifyFlushRequired(this);
    }
}

void
GraphicsLayerWKC::setNeedsDisplayInRect(const FloatRect& fr, ShouldClipToLayer)
{
    m_wkc->setNeedsDisplay();
    WKCFloatRect r = { fr.x(), fr.y(), fr.width(), fr.height() };
    bool added = m_wkc->addDirtyRect(r);
    if (added) {
        client().notifyFlushRequired(this);
    }
}

void
GraphicsLayerWKC::setContentsNeedsDisplay()
{
    bool added = m_wkc->addDirtyRect();
    if (added) {
        client().notifyFlushRequired(this);
    }
}

void
GraphicsLayerWKC::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;
    GraphicsLayer::setContentsOpaque(opaque);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setBackfaceVisibility(bool visible)
{
    if (m_backfaceVisibility == visible)
        return;
    GraphicsLayer::setBackfaceVisibility(visible);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setOpacity(float opacity)
{
    if (m_opacity == opacity)
        return;
    GraphicsLayer::setOpacity(opacity);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setDrawsContent(bool drawsContent)
{
    if (drawsContent == m_drawsContent)
        return;
    GraphicsLayer::setDrawsContent(drawsContent);
    if (m_drawsContent) {
        setNeedsDisplay();
    }
}

void
GraphicsLayerWKC::setSize(const FloatSize& size)
{
    GraphicsLayer::setSize(size);
}

void
GraphicsLayerWKC::setTransform(const TransformationMatrix& t)
{
    if (t == GraphicsLayer::transform())
        return;
    GraphicsLayer::setTransform(t);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setChildrenTransform(const TransformationMatrix& t)
{
    if (t == GraphicsLayer::childrenTransform())
        return;
    GraphicsLayer::setChildrenTransform(t);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setAnchorPoint(const FloatPoint3D& p)
{
    GraphicsLayer::setAnchorPoint(p);
}

void
GraphicsLayerWKC::setPosition(const FloatPoint& p)
{
    if (p == GraphicsLayer::position())
        return;
    GraphicsLayer::setPosition(p);
    client().notifyFlushRequired(this);
}

void
GraphicsLayerWKC::setContentsToImage(Image* img)
{
    m_wkc->setContentsToImage(img);
}

void
GraphicsLayerWKC::setContentsToSolidColor(const Color& c)
{
    m_wkc->setContentsToSolidColor(c);
}

void
GraphicsLayerWKC::setContentsToPlatformLayer(PlatformLayer* layer, ContentsLayerPurpose purpose)
{
    m_activeLayer = layer;
    m_contentsLayerPurpose = purpose;
}

GraphicsLayer::ContentsLayerPurpose
GraphicsLayerWKC::contentsLayerPurpose() const
{
    return m_contentsLayerPurpose;
}

PlatformLayer*
GraphicsLayerWKC::platformLayer() const
{
    if (m_activeLayer)
        return m_activeLayer;
    else
        return reinterpret_cast<PlatformLayer*>(m_wkc->offscreenLayer());
}

void
GraphicsLayerWKC::flushCompositingState(const FloatRect& clipRect)
{
    m_wkc->flushCompositingState(clipRect);
}

void
GraphicsLayerWKC::flushCompositingStateForThisLayerOnly()
{
    if (syncAnimations(MonotonicTime::now())) {
        scheduleLayerFlush();
    }
}

bool
GraphicsLayerWKC::addAnimation(const KeyframeValueList& valueList, const FloatSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
{
    ASSERT(!keyframesName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyTransform && valueList.property() != AnimatedPropertyOpacity)) {
        return false;
    }

    if (valueList.property() == AnimatedPropertyFilter) {
        return false;
    }

    bool listsMatch = false;
    bool hasBigRotation;

    if (valueList.property() == AnimatedPropertyTransform) {
        listsMatch = validateTransformOperations(valueList, hasBigRotation) >= 0;
    }

    const MonotonicTime currentTime = MonotonicTime::now();
    m_animations.add(AnimationWKC(keyframesName, valueList, boxSize, *anim, listsMatch, currentTime - Seconds(timeOffset), 0_s, AnimationWKC::AnimationState::Playing));
    // m_animationStartTime is the time of the first real frame of animation, now or delayed by a negative offset.
    if (Seconds(timeOffset) > 0_s) {
        m_animationStartTime = currentTime;
    } else {
        m_animationStartTime = currentTime - Seconds(timeOffset);
    }

    client().notifyAnimationStarted(this, "", m_animationStartTime);
    scheduleLayerFlush();
    return true;
}

void
GraphicsLayerWKC::pauseAnimation(const String& animationName, double timeOffset)
{
    m_animations.pause(animationName, Seconds(timeOffset));
}

void
GraphicsLayerWKC::removeAnimation(const String& animationName)
{
    m_animations.remove(animationName);
}

bool
GraphicsLayerWKC::syncAnimations(MonotonicTime time)
{
    AnimationWKC::ApplicationResult applicationResults;
    m_animations.apply(applicationResults, time);

    if (applicationResults.transform) {
        GraphicsLayer::setTransform(applicationResults.transform.value());
    }
    if (applicationResults.opacity) {
        GraphicsLayer::setOpacity(applicationResults.opacity.value());
    }
    // TODO: filter

    return applicationResults.hasRunningAnimations;
}

void
GraphicsLayerWKC::scheduleLayerFlush()
{
    if (m_layerFlushTimer.isActive()) {
        return;
    }

    m_layerFlushTimer.startOneShot(0_s);
}

void
GraphicsLayerWKC::layerFlushTimerFired()
{
    client().notifyFlushRequired(this);

    // In case an animation is running, we should flush again soon.
    if (m_animations.hasRunningAnimations()) {
        scheduleLayerFlush();
    }
}

std::unique_ptr<GraphicsLayer>
GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient& client, GraphicsLayer::Type type)
{
    return std::unique_ptr<GraphicsLayer>(new GraphicsLayerWKC(type, client));
}

WKC::GraphicsLayerPrivate*
GraphicsLayerWKC_wkc(const GraphicsLayer* layer)
{
    const GraphicsLayerWKC* wl = static_cast<const GraphicsLayerWKC*>(layer);
    return wl->wkc();
}

} // namespace
