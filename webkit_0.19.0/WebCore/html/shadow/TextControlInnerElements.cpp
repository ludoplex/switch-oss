/*
 * Copyright (C) 2006, 2008, 2010, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "TextControlInnerElements.h"

#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "PlatformMouseEvent.h"
#include "RenderSearchField.h"
#include "RenderTextControl.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "TextEvent.h"
#include "TextEventInputType.h"
#include <wtf/Ref.h>

namespace WebCore {

using namespace HTMLNames;

TextControlInnerContainer::TextControlInnerContainer(Document& document)
    : HTMLDivElement(divTag, document)
{
}

Ref<TextControlInnerContainer> TextControlInnerContainer::create(Document& document)
{
    return adoptRef(*new TextControlInnerContainer(document));
}
    
RenderPtr<RenderElement> TextControlInnerContainer::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderTextControlInnerContainer>(*this, WTF::move(style));
}

TextControlInnerElement::TextControlInnerElement(Document& document)
    : HTMLDivElement(divTag, document)
{
    setHasCustomStyleResolveCallbacks();
}

Ref<TextControlInnerElement> TextControlInnerElement::create(Document& document)
{
    return adoptRef(*new TextControlInnerElement(document));
}

RefPtr<RenderStyle> TextControlInnerElement::customStyleForRenderer(RenderStyle&)
{
    RenderTextControlSingleLine& parentRenderer = downcast<RenderTextControlSingleLine>(*shadowHost()->renderer());
    return parentRenderer.createInnerBlockStyle(&parentRenderer.style());
}

// ---------------------------

inline TextControlInnerTextElement::TextControlInnerTextElement(Document& document)
    : HTMLDivElement(divTag, document)
{
    setHasCustomStyleResolveCallbacks();
}

Ref<TextControlInnerTextElement> TextControlInnerTextElement::create(Document& document)
{
    return adoptRef(*new TextControlInnerTextElement(document));
}

void TextControlInnerTextElement::defaultEventHandler(Event* event)
{
    // FIXME: In the future, we should add a way to have default event listeners.
    // Then we would add one to the text field's inner div, and we wouldn't need this subclass.
    // Or possibly we could just use a normal event listener.
    if (event->isBeforeTextInsertedEvent() || event->type() == eventNames().webkitEditableContentChangedEvent) {
        Element* shadowAncestor = shadowHost();
        // A TextControlInnerTextElement can have no host if its been detached,
        // but kept alive by an EditCommand. In this case, an undo/redo can
        // cause events to be sent to the TextControlInnerTextElement. To
        // prevent an infinite loop, we must check for this case before sending
        // the event up the chain.
        if (shadowAncestor)
            shadowAncestor->defaultEventHandler(event);
    }
    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

RenderPtr<RenderElement> TextControlInnerTextElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition&)
{
    return createRenderer<RenderTextControlInnerBlock>(*this, WTF::move(style));
}

RenderTextControlInnerBlock* TextControlInnerTextElement::renderer() const
{
    return downcast<RenderTextControlInnerBlock>(HTMLDivElement::renderer());
}

RefPtr<RenderStyle> TextControlInnerTextElement::customStyleForRenderer(RenderStyle&)
{
    RenderTextControl& parentRenderer = downcast<RenderTextControl>(*shadowHost()->renderer());
    return parentRenderer.createInnerTextStyle(&parentRenderer.style());
}

// ----------------------------

inline SearchFieldResultsButtonElement::SearchFieldResultsButtonElement(Document& document)
    : HTMLDivElement(divTag, document)
{
}

Ref<SearchFieldResultsButtonElement> SearchFieldResultsButtonElement::create(Document& document)
{
    return adoptRef(*new SearchFieldResultsButtonElement(document));
}

void SearchFieldResultsButtonElement::defaultEventHandler(Event* event)
{
    // On mousedown, bring up a menu, if needed
    HTMLInputElement* input = downcast<HTMLInputElement>(shadowHost());
    if (input && event->type() == eventNames().mousedownEvent && is<MouseEvent>(*event) && downcast<MouseEvent>(*event).button() == (unsigned short)LeftButton) {
        input->focus();
        input->select();
#if !PLATFORM(IOS)
        RenderSearchField& renderer = downcast<RenderSearchField>(*input->renderer());
        if (renderer.popupIsVisible())
            renderer.hidePopup();
        else if (input->maxResults() > 0)
            renderer.showPopup();
#endif
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

#if !PLATFORM(IOS)
bool SearchFieldResultsButtonElement::willRespondToMouseClickEvents()
{
    return true;
}
#endif

// ----------------------------

inline SearchFieldCancelButtonElement::SearchFieldCancelButtonElement(Document& document)
    : HTMLDivElement(divTag, document)
{
    setPseudo(AtomicString("-webkit-search-cancel-button", AtomicString::ConstructFromLiteral));
#if !PLATFORM(IOS)
    setAttribute(aria_labelAttr, AXSearchFieldCancelButtonText());
#endif
    setAttribute(roleAttr, AtomicString("button", AtomicString::ConstructFromLiteral));
}

Ref<SearchFieldCancelButtonElement> SearchFieldCancelButtonElement::create(Document& document)
{
    return adoptRef(*new SearchFieldCancelButtonElement(document));
}

void SearchFieldCancelButtonElement::defaultEventHandler(Event* event)
{
    RefPtr<HTMLInputElement> input(downcast<HTMLInputElement>(shadowHost()));
    if (!input || input->isDisabledOrReadOnly()) {
        if (!event->defaultHandled())
            HTMLDivElement::defaultEventHandler(event);
        return;
    }

    if (event->type() == eventNames().mousedownEvent && is<MouseEvent>(*event) && downcast<MouseEvent>(*event).button() == (unsigned short)LeftButton) {
        input->focus();
        input->select();
        event->setDefaultHandled();
    }

    if (event->type() == eventNames().clickEvent) {
        input->setValueForUser(emptyString());
        input->onSearch();
        event->setDefaultHandled();
    }

    if (!event->defaultHandled())
        HTMLDivElement::defaultEventHandler(event);
}

#if !PLATFORM(IOS)
bool SearchFieldCancelButtonElement::willRespondToMouseClickEvents()
{
    const HTMLInputElement* input = downcast<HTMLInputElement>(shadowHost());
    if (input && !input->isDisabledOrReadOnly())
        return true;

    return HTMLDivElement::willRespondToMouseClickEvents();
}
#endif

}
