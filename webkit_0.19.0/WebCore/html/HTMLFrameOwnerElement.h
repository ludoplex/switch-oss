/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLFrameOwnerElement_h
#define HTMLFrameOwnerElement_h

#include "HTMLElement.h"
#include <wtf/HashCountedSet.h>

namespace WebCore {

class DOMWindow;
class Frame;
class RenderWidget;
class SVGDocument;

class HTMLFrameOwnerElement : public HTMLElement {
public:
    virtual ~HTMLFrameOwnerElement();

    Frame* contentFrame() const { return m_contentFrame; }
    DOMWindow* contentWindow() const;
    WEBCORE_EXPORT Document* contentDocument() const;

    void setContentFrame(Frame*);
    void clearContentFrame();

    void disconnectContentFrame();

    // Most subclasses use RenderWidget (either RenderEmbeddedObject or RenderIFrame)
    // except for HTMLObjectElement and HTMLEmbedElement which may return any
    // RenderElement when using fallback content.
    RenderWidget* renderWidget() const;

    SVGDocument* getSVGDocument(ExceptionCode&) const;

    virtual ScrollbarMode scrollingMode() const { return ScrollbarAuto; }

    SandboxFlags sandboxFlags() const { return m_sandboxFlags; }

    void scheduleSetNeedsStyleRecalc(StyleChangeType = FullStyleChange);

    virtual bool isURLAllowed(const URL&) const { return true; }

protected:
    HTMLFrameOwnerElement(const QualifiedName& tagName, Document&);
    void setSandboxFlags(SandboxFlags);

private:
    virtual bool isKeyboardFocusable(KeyboardEvent*) const override;
    virtual bool isFrameOwnerElement() const override final { return true; }

    Frame* m_contentFrame;
    SandboxFlags m_sandboxFlags;
};

class SubframeLoadingDisabler {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit SubframeLoadingDisabler(ContainerNode* root)
        : m_root(root)
    {
        if (m_root)
            disabledSubtreeRoots().add(m_root);
    }

    ~SubframeLoadingDisabler()
    {
        if (m_root)
            disabledSubtreeRoots().remove(m_root);
    }

    static bool canLoadFrame(HTMLFrameOwnerElement&);

private:
    static HashCountedSet<ContainerNode*>& disabledSubtreeRoots()
    {
        DEPRECATED_DEFINE_STATIC_LOCAL(HashCountedSet<ContainerNode*>, nodes, ());
        return nodes;
    }

    ContainerNode* m_root;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::HTMLFrameOwnerElement)
    static bool isType(const WebCore::Node& node) { return node.isFrameOwnerElement(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // HTMLFrameOwnerElement_h
