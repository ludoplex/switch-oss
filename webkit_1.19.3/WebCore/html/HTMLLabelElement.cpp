/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "HTMLLabelElement.h"

#include "Document.h"
#include "ElementIterator.h"
#include "Event.h"
#include "EventNames.h"
#include "FormAssociatedElement.h"
#include "HTMLFormControlElement.h"
#include "HTMLNames.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(HTMLLabelElement);

using namespace HTMLNames;

static LabelableElement* firstElementWithIdIfLabelable(TreeScope& treeScope, const AtomicString& id)
{
    auto element = makeRefPtr(treeScope.getElementById(id));
    if (!is<LabelableElement>(element))
        return nullptr;

    auto& labelableElement = downcast<LabelableElement>(*element);
    return labelableElement.supportLabels() ? &labelableElement : nullptr;
}

inline HTMLLabelElement::HTMLLabelElement(const QualifiedName& tagName, Document& document)
    : HTMLElement(tagName, document)
{
    ASSERT(hasTagName(labelTag));
}

Ref<HTMLLabelElement> HTMLLabelElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new HTMLLabelElement(tagName, document));
}

RefPtr<LabelableElement> HTMLLabelElement::control() const
{
    auto& controlId = attributeWithoutSynchronization(forAttr);
    if (controlId.isNull()) {
        // Search the children and descendants of the label element for a form element.
        // per http://dev.w3.org/html5/spec/Overview.html#the-label-element
        // the form element must be "labelable form-associated element".
        for (auto& labelableElement : descendantsOfType<LabelableElement>(*this)) {
            if (labelableElement.supportLabels())
                return const_cast<LabelableElement*>(&labelableElement);
        }
        return nullptr;
    }
    return isConnected() ? firstElementWithIdIfLabelable(treeScope(), controlId) : nullptr;
}

HTMLFormElement* HTMLLabelElement::form() const
{
    auto control = this->control();
    if (!is<HTMLFormControlElement>(control))
        return nullptr;
    return downcast<HTMLFormControlElement>(control.get())->form();
}

void HTMLLabelElement::setActive(bool down, bool pause)
{
    if (down == active())
        return;

    // Update our status first.
    HTMLElement::setActive(down, pause);

    // Also update our corresponding control.
    if (auto element = control())
        element->setActive(down, pause);
}

void HTMLLabelElement::setHovered(bool over)
{
    if (over == hovered())
        return;
        
    // Update our status first.
    HTMLElement::setHovered(over);

    // Also update our corresponding control.
    if (auto element = control())
        element->setHovered(over);
}

void HTMLLabelElement::defaultEventHandler(Event& event)
{
#if !PLATFORM(WKC)
    static bool processingClick = false;
#else
    WKC_DEFINE_STATIC_BOOL(processingClick, false);
#endif

    if (event.type() == eventNames().clickEvent && !processingClick) {
        auto control = this->control();

        // If we can't find a control or if the control received the click
        // event, then there's no need for us to do anything.
        if (!control || (is<Node>(event.target()) && control->containsIncludingShadowDOM(&downcast<Node>(*event.target())))) {
            HTMLElement::defaultEventHandler(event);
            return;
        }

        processingClick = true;

        control->dispatchSimulatedClick(&event);

        document().updateLayoutIgnorePendingStylesheets();
        if (control->isMouseFocusable())
            control->focus();

        processingClick = false;

        event.setDefaultHandled();
    }

    HTMLElement::defaultEventHandler(event);
}

bool HTMLLabelElement::willRespondToMouseClickEvents()
{
    auto element = control();
    return (element && element->willRespondToMouseClickEvents()) || HTMLElement::willRespondToMouseClickEvents();
}

void HTMLLabelElement::focus(bool restorePreviousSelection, FocusDirection direction)
{
    if (document().haveStylesheetsLoaded()) {
        document().updateLayout();
        if (isFocusable()) {
            // The value of restorePreviousSelection is not used for label elements as it doesn't override updateFocusAppearance.
            Element::focus(restorePreviousSelection, direction);
            return;
        }
    }

    // To match other browsers, always restore previous selection.
    if (auto element = control())
        element->focus(true, direction);
}

void HTMLLabelElement::accessKeyAction(bool sendMouseEvents)
{
    if (auto element = control())
        element->accessKeyAction(sendMouseEvents);
    else
        HTMLElement::accessKeyAction(sendMouseEvents);
}

} // namespace
