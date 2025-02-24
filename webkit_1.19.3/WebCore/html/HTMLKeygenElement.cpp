/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2006, 2010, 2012-2016 Apple Inc. All rights reserved.
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
#include "HTMLKeygenElement.h"

#include "Attribute.h"
#include "DOMFormData.h"
#include "Document.h"
#include "ElementChildIterator.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "HTMLOptionElement.h"
#include "SSLKeyGenerator.h"
#include "ShadowRoot.h"
#include "Text.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>

using namespace WebCore;

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(HTMLKeygenElement);

using namespace HTMLNames;

class KeygenSelectElement final : public HTMLSelectElement {
    WTF_MAKE_ISO_ALLOCATED_INLINE(KeygenSelectElement);
public:
    static Ref<KeygenSelectElement> create(Document& document)
    {
        return adoptRef(*new KeygenSelectElement(document));
    }

protected:
    KeygenSelectElement(Document& document)
        : HTMLSelectElement(selectTag, document, 0)
    {
        static NeverDestroyed<AtomicString> pseudoId("-webkit-keygen-select", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
        if (pseudoId.isNull())
            pseudoId.construct("-webkit-keygen-select", AtomicString::ConstructFromLiteral);
#endif
        setPseudo(pseudoId);
    }

private:
    Ref<Element> cloneElementWithoutAttributesAndChildren(Document& targetDocument) override
    {
        return create(targetDocument);
    }
};

inline HTMLKeygenElement::HTMLKeygenElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
    : HTMLFormControlElementWithState(tagName, document, form)
{
    ASSERT(hasTagName(keygenTag));

    // Create a select element with one option element for each key size.
    Vector<String> keys;
    getSupportedKeySizes(keys);

    auto select = KeygenSelectElement::create(document);
    for (auto& key : keys) {
        auto option = HTMLOptionElement::create(document);
        select->appendChild(option);
        option->appendChild(Text::create(document, key));
    }

    ensureUserAgentShadowRoot().appendChild(select);
}

Ref<HTMLKeygenElement> HTMLKeygenElement::create(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
{
    return adoptRef(*new HTMLKeygenElement(tagName, document, form));
}

void HTMLKeygenElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    // Reflect disabled attribute on the shadow select element
    if (name == disabledAttr)
        shadowSelect()->setAttribute(name, value);

    HTMLFormControlElement::parseAttribute(name, value);
}

bool HTMLKeygenElement::isKeytypeRSA() const
{
    const auto& keyType = attributeWithoutSynchronization(keytypeAttr);
    return keyType.isNull() || equalLettersIgnoringASCIICase(keyType, "rsa");
}

void HTMLKeygenElement::setKeytype(const AtomicString& value)
{
    setAttributeWithoutSynchronization(keytypeAttr, value);
}

String HTMLKeygenElement::keytype() const
{
    return isKeytypeRSA() ? "rsa"_s : emptyString();
}

bool HTMLKeygenElement::appendFormData(DOMFormData& formData, bool)
{
    // Only RSA is supported at this time.
    if (!isKeytypeRSA())
        return false;
    auto value = document().signedPublicKeyAndChallengeString(shadowSelect()->selectedIndex(), attributeWithoutSynchronization(challengeAttr), document().baseURL());
    if (value.isNull())
        return false;
    formData.append(name(), value);
    return true;
}

const AtomicString& HTMLKeygenElement::formControlType() const
{
    static NeverDestroyed<const AtomicString> keygen("keygen", AtomicString::ConstructFromLiteral);
#if PLATFORM(WKC)
    if (keygen.isNull())
        keygen.construct("keygen", AtomicString::ConstructFromLiteral);
#endif
    return keygen;
}

void HTMLKeygenElement::reset()
{
    static_cast<HTMLFormControlElement*>(shadowSelect())->reset();
}

bool HTMLKeygenElement::shouldSaveAndRestoreFormControlState() const
{
    return false;
}

HTMLSelectElement* HTMLKeygenElement::shadowSelect() const
{
    auto root = userAgentShadowRoot();
    if (!root)
        return nullptr;

    return childrenOfType<HTMLSelectElement>(*root).first();
}

} // namespace
