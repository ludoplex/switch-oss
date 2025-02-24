/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
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
#include "EventPath.h"

#include "DOMWindow.h"
#include "Event.h"
#include "EventContext.h"
#include "EventNames.h"
#include "HTMLSlotElement.h"
#include "Node.h"
#include "PseudoElement.h"
#include "ShadowRoot.h"
#include "TouchEvent.h"

#if PLATFORM(WKC)
#include "MouseEvent.h"
#endif

namespace WebCore {

class WindowEventContext final : public EventContext {
public:
    WindowEventContext(Node&, DOMWindow&, EventTarget&, int closedShadowDepth);
private:
    void handleLocalEvents(Event&, EventInvokePhase) const final;
};

inline WindowEventContext::WindowEventContext(Node& node, DOMWindow& currentTarget, EventTarget& target, int closedShadowDepth)
    : EventContext(&node, &currentTarget, &target, closedShadowDepth)
{
}

void WindowEventContext::handleLocalEvents(Event& event, EventInvokePhase phase) const
{
    event.setTarget(m_target.get());
    event.setCurrentTarget(m_currentTarget.get());
    m_currentTarget->fireEventListeners(event, phase);
}

static inline bool shouldEventCrossShadowBoundary(Event& event, ShadowRoot& shadowRoot, EventTarget& target)
{
#if ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    // Video-only full screen is a mode where we use the shadow DOM as an implementation
    // detail that should not be detectable by the web content.
    if (is<Node>(target)) {
        if (auto* element = downcast<Node>(target).document().webkitCurrentFullScreenElement()) {
            // FIXME: We assume that if the full screen element is a media element that it's
            // the video-only full screen. Both here and elsewhere. But that is probably wrong.
            if (element->isMediaElement() && shadowRoot.host() == element)
                return false;
        }
    }
#endif

    bool targetIsInShadowRoot = is<Node>(target) && &downcast<Node>(target).treeScope().rootNode() == &shadowRoot;
    return !targetIsInShadowRoot || event.composed();
}

static Node* nodeOrHostIfPseudoElement(Node* node)
{
    return is<PseudoElement>(*node) ? downcast<PseudoElement>(*node).hostElement() : node;
}

class RelatedNodeRetargeter {
public:
    RelatedNodeRetargeter(Node& relatedNode, Node& target);

    Node* currentNode(Node& currentTreeScope);
    void moveToNewTreeScope(TreeScope* previousTreeScope, TreeScope& newTreeScope);

private:
    Node* nodeInLowestCommonAncestor();
    void collectTreeScopes();

    void checkConsistency(Node& currentTarget);

    Node& m_relatedNode;
    Node* m_retargetedRelatedNode;
    Vector<TreeScope*, 8> m_ancestorTreeScopes;
    unsigned m_lowestCommonAncestorIndex { 0 };
    bool m_hasDifferentTreeRoot { false };
};

EventPath::EventPath(Node& originalTarget, Event& event)
{
    buildPath(originalTarget, event);

    if (auto* relatedTarget = event.relatedTarget())
        setRelatedTarget(originalTarget, *relatedTarget);

#if ENABLE(TOUCH_EVENTS)
    if (is<TouchEvent>(event))
        retargetTouchLists(downcast<TouchEvent>(event));
#endif
}

void EventPath::buildPath(Node& originalTarget, Event& event)
{
    using MakeEventContext = std::unique_ptr<EventContext> (*)(Node&, EventTarget*, EventTarget*, int closedShadowDepth);
    MakeEventContext makeEventContext = [] (Node& node, EventTarget* currentTarget, EventTarget* target, int closedShadowDepth) {
        return std::make_unique<EventContext>(&node, currentTarget, target, closedShadowDepth);
    };
    if (is<MouseEvent>(event) || event.isFocusEvent()) {
        makeEventContext = [] (Node& node, EventTarget* currentTarget, EventTarget* target, int closedShadowDepth) -> std::unique_ptr<EventContext> {
            return std::make_unique<MouseOrFocusEventContext>(node, currentTarget, target, closedShadowDepth);
        };
    }
#if ENABLE(TOUCH_EVENTS)
    if (is<TouchEvent>(event)) {
        makeEventContext = [] (Node& node, EventTarget* currentTarget, EventTarget* target, int closedShadowDepth) -> std::unique_ptr<EventContext> {
            return std::make_unique<TouchEventContext>(node, currentTarget, target, closedShadowDepth);
        };
    }
#endif

    Node* node = nodeOrHostIfPseudoElement(&originalTarget);
    Node* target = node ? eventTargetRespectingTargetRules(*node) : nullptr;
    int closedShadowDepth = 0;
    // Depths are used to decided which nodes are excluded in event.composedPath when the tree is mutated during event dispatching.
    // They could be negative for nodes outside the shadow tree of the target node.
    while (node) {
        while (node) {
            m_path.append(makeEventContext(*node, eventTargetRespectingTargetRules(*node), target, closedShadowDepth));

            if (is<ShadowRoot>(*node))
                break;

            ContainerNode* parent = node->parentNode();
            if (UNLIKELY(!parent)) {
                // https://dom.spec.whatwg.org/#interface-document
                if (is<Document>(*node) && event.type() != eventNames().loadEvent) {
                    ASSERT(target);
                    if (target) {
                        if (auto* window = downcast<Document>(*node).domWindow())
                            m_path.append(std::make_unique<WindowEventContext>(*node, *window, *target, closedShadowDepth));
                    }
                }
                return;
            }

            auto* shadowRootOfParent = parent->shadowRoot();
            if (UNLIKELY(shadowRootOfParent)) {
                if (auto* assignedSlot = shadowRootOfParent->findAssignedSlot(*node)) {
                    if (shadowRootOfParent->mode() != ShadowRootMode::Open)
                        closedShadowDepth++;
                    // node is assigned to a slot. Continue dispatching the event at this slot.
                    parent = assignedSlot;
                }
            }
            node = parent;
        }

        bool exitingShadowTreeOfTarget = &target->treeScope() == &node->treeScope();
        ShadowRoot& shadowRoot = downcast<ShadowRoot>(*node);
        if (!shouldEventCrossShadowBoundary(event, shadowRoot, originalTarget))
            return;
        node = shadowRoot.host();
        if (shadowRoot.mode() != ShadowRootMode::Open)
            closedShadowDepth--;
        if (exitingShadowTreeOfTarget)
            target = eventTargetRespectingTargetRules(*node);
    }
}

void EventPath::setRelatedTarget(Node& origin, EventTarget& relatedTarget)
{
    if (!is<Node>(relatedTarget) || m_path.isEmpty())
        return;

    auto& relatedNode = downcast<Node>(relatedTarget);
    RelatedNodeRetargeter retargeter(relatedNode, *m_path[0]->node());

    bool originIsRelatedTarget = &origin == &relatedNode;
    Node& rootNodeInOriginTreeScope = origin.treeScope().rootNode();
    TreeScope* previousTreeScope = nullptr;
    size_t originalEventPathSize = m_path.size();
    for (unsigned contextIndex = 0; contextIndex < originalEventPathSize; contextIndex++) {
        auto& ambgiousContext = *m_path[contextIndex];
        if (!is<MouseOrFocusEventContext>(ambgiousContext))
            continue;
        auto& context = downcast<MouseOrFocusEventContext>(ambgiousContext);

        Node& currentTarget = *context.node();
        TreeScope& currentTreeScope = currentTarget.treeScope();
        if (UNLIKELY(previousTreeScope && &currentTreeScope != previousTreeScope))
            retargeter.moveToNewTreeScope(previousTreeScope, currentTreeScope);

        Node* currentRelatedNode = retargeter.currentNode(currentTarget);
        if (UNLIKELY(!originIsRelatedTarget && context.target() == currentRelatedNode)) {
            m_path.shrink(contextIndex);
            break;
        }

        context.setRelatedTarget(currentRelatedNode);

        if (UNLIKELY(originIsRelatedTarget && context.node() == &rootNodeInOriginTreeScope)) {
            m_path.shrink(contextIndex + 1);
            break;
        }

        previousTreeScope = &currentTreeScope;
    }
}

#if ENABLE(TOUCH_EVENTS)

void EventPath::retargetTouch(TouchEventContext::TouchListType type, const Touch& touch)
{
    auto* eventTarget = touch.target();
    if (!is<Node>(eventTarget))
        return;

    RelatedNodeRetargeter retargeter(downcast<Node>(*eventTarget), *m_path[0]->node());
    TreeScope* previousTreeScope = nullptr;
    for (auto& context : m_path) {
        Node& currentTarget = *context->node();
        TreeScope& currentTreeScope = currentTarget.treeScope();
        if (UNLIKELY(previousTreeScope && &currentTreeScope != previousTreeScope))
            retargeter.moveToNewTreeScope(previousTreeScope, currentTreeScope);

        if (is<TouchEventContext>(*context)) {
            Node* currentRelatedNode = retargeter.currentNode(currentTarget);
            downcast<TouchEventContext>(*context).touchList(type).append(touch.cloneWithNewTarget(currentRelatedNode));
        }

        previousTreeScope = &currentTreeScope;
    }
}

void EventPath::retargetTouchList(TouchEventContext::TouchListType type, const TouchList* list)
{
    for (unsigned i = 0, length = list ? list->length() : 0; i < length; ++i)
        retargetTouch(type, *list->item(i));
}

void EventPath::retargetTouchLists(const TouchEvent& event)
{
    retargetTouchList(TouchEventContext::Touches, event.touches());
    retargetTouchList(TouchEventContext::TargetTouches, event.targetTouches());
    retargetTouchList(TouchEventContext::ChangedTouches, event.changedTouches());
}

#endif

// https://dom.spec.whatwg.org/#dom-event-composedpath
// Any node whose depth computed in EventPath::buildPath is greater than the context object is excluded.
// Because we can exit out of a closed shadow tree and re-enter another closed shadow tree via a slot,
// we decrease the *allowed depth* whenever we moved to a "shallower" (closer-to-document) tree.
Vector<EventTarget*> EventPath::computePathUnclosedToTarget(const EventTarget& target) const
{
    Vector<EventTarget*> path;
    auto pathSize = m_path.size();
    RELEASE_ASSERT(pathSize);
    path.reserveInitialCapacity(pathSize);

    auto currentTargetIndex = m_path.findMatching([&target] (auto& context) {
        return context->currentTarget() == &target;
    });
    RELEASE_ASSERT(currentTargetIndex != notFound);
    auto currentTargetDepth = m_path[currentTargetIndex]->closedShadowDepth();

    auto appendTargetWithLesserDepth = [&path] (const EventContext& currentContext, int& currentDepthAllowed) {
        auto depth = currentContext.closedShadowDepth();
        bool contextIsInsideInnerShadowTree = depth > currentDepthAllowed;
        if (contextIsInsideInnerShadowTree)
            return;
        bool movedOutOfShadowTree = depth < currentDepthAllowed;
        if (movedOutOfShadowTree)
            currentDepthAllowed = depth;
        path.uncheckedAppend(currentContext.currentTarget());
    };

    auto currentDepthAllowed = currentTargetDepth;
    auto i = currentTargetIndex;
    do {
        appendTargetWithLesserDepth(*m_path[i], currentDepthAllowed);
    } while (i--);
    path.reverse();

    currentDepthAllowed = currentTargetDepth;
    for (auto i = currentTargetIndex + 1; i < pathSize; ++i)
        appendTargetWithLesserDepth(*m_path[i], currentDepthAllowed);
    
    return path;
}

EventPath::EventPath(const Vector<Element*>& targets)
{
    // FIXME: This function seems wrong. Why are we not firing events in the closed shadow trees?
    for (auto* target : targets) {
        ASSERT(target);
        Node* origin = *targets.begin();
        if (!target->isClosedShadowHidden(*origin))
            m_path.append(std::make_unique<EventContext>(target, target, origin, 0));
    }
}

EventPath::EventPath(const Vector<EventTarget*>& targets)
{
    for (auto* target : targets) {
        ASSERT(target);
        ASSERT(!is<Node>(target));
        m_path.append(std::make_unique<EventContext>(nullptr, target, *targets.begin(), 0));
    }
}

static Node* moveOutOfAllShadowRoots(Node& startingNode)
{
    Node* node = &startingNode;
    while (node->isInShadowTree())
        node = downcast<ShadowRoot>(node->treeScope().rootNode()).host();
    return node;
}

RelatedNodeRetargeter::RelatedNodeRetargeter(Node& relatedNode, Node& target)
    : m_relatedNode(relatedNode)
    , m_retargetedRelatedNode(&relatedNode)
{
    auto& targetTreeScope = target.treeScope();
    TreeScope* currentTreeScope = &m_relatedNode.treeScope();
    if (LIKELY(currentTreeScope == &targetTreeScope && target.isConnected() && m_relatedNode.isConnected()))
        return;

    if (&currentTreeScope->documentScope() != &targetTreeScope.documentScope()) {
        m_hasDifferentTreeRoot = true;
        m_retargetedRelatedNode = nullptr;
        return;
    }
    if (relatedNode.isConnected() != target.isConnected()) {
        m_hasDifferentTreeRoot = true;
        m_retargetedRelatedNode = moveOutOfAllShadowRoots(relatedNode);
        return;
    }

    collectTreeScopes();

    // FIXME: We should collect this while constructing the event path.
    Vector<TreeScope*, 8> targetTreeScopeAncestors;
    for (TreeScope* currentTreeScope = &targetTreeScope; currentTreeScope; currentTreeScope = currentTreeScope->parentTreeScope())
        targetTreeScopeAncestors.append(currentTreeScope);
    ASSERT_WITH_SECURITY_IMPLICATION(!targetTreeScopeAncestors.isEmpty());

    unsigned i = m_ancestorTreeScopes.size();
    unsigned j = targetTreeScopeAncestors.size();
    ASSERT_WITH_SECURITY_IMPLICATION(m_ancestorTreeScopes.last() == targetTreeScopeAncestors.last());
    while (m_ancestorTreeScopes[i - 1] == targetTreeScopeAncestors[j - 1]) {
        i--;
        j--;
        if (!i || !j)
            break;
    }

    bool lowestCommonAncestorIsDocumentScope = i + 1 == m_ancestorTreeScopes.size();
    if (lowestCommonAncestorIsDocumentScope && !relatedNode.isConnected() && !target.isConnected()) {
        Node& relatedNodeAncestorInDocumentScope = i ? *downcast<ShadowRoot>(m_ancestorTreeScopes[i - 1]->rootNode()).shadowHost() : relatedNode;
        Node& targetAncestorInDocumentScope = j ? *downcast<ShadowRoot>(targetTreeScopeAncestors[j - 1]->rootNode()).shadowHost() : target;
        if (&targetAncestorInDocumentScope.rootNode() != &relatedNodeAncestorInDocumentScope.rootNode()) {
            m_hasDifferentTreeRoot = true;
            m_retargetedRelatedNode = moveOutOfAllShadowRoots(relatedNode);
            return;
        }
    }

    m_lowestCommonAncestorIndex = i;
    m_retargetedRelatedNode = nodeInLowestCommonAncestor();
}

inline Node* RelatedNodeRetargeter::currentNode(Node& currentTarget)
{
    checkConsistency(currentTarget);
    return m_retargetedRelatedNode;
}

void RelatedNodeRetargeter::moveToNewTreeScope(TreeScope* previousTreeScope, TreeScope& newTreeScope)
{
    if (m_hasDifferentTreeRoot)
        return;

    auto& currentRelatedNodeScope = m_retargetedRelatedNode->treeScope();
    if (previousTreeScope != &currentRelatedNodeScope) {
        // currentRelatedNode is still outside our shadow tree. New tree scope may contain currentRelatedNode
        // but there is no need to re-target it. Moving into a slot (thereby a deeper shadow tree) doesn't matter.
        return;
    }

    bool enteredSlot = newTreeScope.parentTreeScope() == previousTreeScope;
    if (enteredSlot) {
        if (m_lowestCommonAncestorIndex) {
            if (m_ancestorTreeScopes.isEmpty())
                collectTreeScopes();
            bool relatedNodeIsInSlot = m_ancestorTreeScopes[m_lowestCommonAncestorIndex - 1] == &newTreeScope;
            if (relatedNodeIsInSlot) {
                m_lowestCommonAncestorIndex--;
                m_retargetedRelatedNode = nodeInLowestCommonAncestor();
                ASSERT(&newTreeScope == &m_retargetedRelatedNode->treeScope());
            }
        } else
            ASSERT(m_retargetedRelatedNode == &m_relatedNode);
    } else {
        ASSERT(previousTreeScope->parentTreeScope() == &newTreeScope);
        m_lowestCommonAncestorIndex++;
        ASSERT_WITH_SECURITY_IMPLICATION(m_ancestorTreeScopes.isEmpty() || m_lowestCommonAncestorIndex < m_ancestorTreeScopes.size());
        m_retargetedRelatedNode = downcast<ShadowRoot>(currentRelatedNodeScope.rootNode()).host();
        ASSERT(&newTreeScope == &m_retargetedRelatedNode->treeScope());
    }
}

inline Node* RelatedNodeRetargeter::nodeInLowestCommonAncestor()
{
    if (!m_lowestCommonAncestorIndex)
        return &m_relatedNode;
    auto& rootNode = m_ancestorTreeScopes[m_lowestCommonAncestorIndex - 1]->rootNode();
    return downcast<ShadowRoot>(rootNode).host();
}

void RelatedNodeRetargeter::collectTreeScopes()
{
    ASSERT(m_ancestorTreeScopes.isEmpty());
    for (TreeScope* currentTreeScope = &m_relatedNode.treeScope(); currentTreeScope; currentTreeScope = currentTreeScope->parentTreeScope())
        m_ancestorTreeScopes.append(currentTreeScope);
    ASSERT_WITH_SECURITY_IMPLICATION(!m_ancestorTreeScopes.isEmpty());
}

#if ASSERT_DISABLED

inline void RelatedNodeRetargeter::checkConsistency(Node&)
{
}

#else

void RelatedNodeRetargeter::checkConsistency(Node& currentTarget)
{
    if (!m_retargetedRelatedNode)
        return;
    ASSERT(!currentTarget.isClosedShadowHidden(*m_retargetedRelatedNode));
    ASSERT(m_retargetedRelatedNode == &currentTarget.treeScope().retargetToScope(m_relatedNode));
}

#endif

}
