/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2018-2019 Apple Inc. All rights reserved.
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
#include "SVGURIReference.h"

#include "Document.h"
#include "Element.h"
#include "URL.h"
#include "XLinkNames.h"

#if PLATFORM(WKC)
#include "SVGElement.h"
#endif

namespace WebCore {

SVGURIReference::SVGURIReference(SVGElement* contextElement)
    : m_href(SVGAnimatedString::create(contextElement))
{
#if PLATFORM(WKC)
    WKC_DEFINE_STATIC_BOOL(onceFlag, false);
    if (!onceFlag) {
        PropertyRegistry::registerProperty<SVGNames::hrefAttr, &SVGURIReference::m_href>();
        PropertyRegistry::registerProperty<XLinkNames::hrefAttr, &SVGURIReference::m_href>();
        onceFlag = true;
    }
#else
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [] {
        PropertyRegistry::registerProperty<SVGNames::hrefAttr, &SVGURIReference::m_href>();
        PropertyRegistry::registerProperty<XLinkNames::hrefAttr, &SVGURIReference::m_href>();
    });
#endif
}

bool SVGURIReference::isKnownAttribute(const QualifiedName& attributeName)
{
    return PropertyRegistry::isKnownAttribute(attributeName);
}

void SVGURIReference::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (isKnownAttribute(name))
        m_href->setBaseValInternal(value);
}

String SVGURIReference::fragmentIdentifierFromIRIString(const String& url, const Document& document)
{
    size_t start = url.find('#');
    if (start == notFound)
        return emptyString();

    URL base = start ? URL(document.baseURL(), url.substring(0, start)) : document.baseURL();
    String fragmentIdentifier = url.substring(start);
    URL kurl(base, fragmentIdentifier);
    if (equalIgnoringFragmentIdentifier(kurl, document.url()))
        return fragmentIdentifier.substring(1);

    // The url doesn't have any fragment identifier.
    return emptyString();
}

static inline URL urlFromIRIStringWithFragmentIdentifier(const String& url, const Document& document, String& fragmentIdentifier)
{
    size_t startOfFragmentIdentifier = url.find('#');
    if (startOfFragmentIdentifier == notFound)
        return URL();

    // Exclude the '#' character when determining the fragmentIdentifier.
    fragmentIdentifier = url.substring(startOfFragmentIdentifier + 1);
    if (startOfFragmentIdentifier) {
        URL base(document.baseURL(), url.substring(0, startOfFragmentIdentifier));
        return URL(base, url.substring(startOfFragmentIdentifier));
    }

    return URL(document.baseURL(), url.substring(startOfFragmentIdentifier));
}

Element* SVGURIReference::targetElementFromIRIString(const String& iri, const Document& document, String* fragmentIdentifier, const Document* externalDocument)
{
    // If there's no fragment identifier contained within the IRI string, we can't lookup an element.
    String id;
    URL url = urlFromIRIStringWithFragmentIdentifier(iri, document, id);
    if (url == URL())
        return 0;

    if (fragmentIdentifier)
        *fragmentIdentifier = id;

    if (id.isEmpty())
        return 0;

    if (externalDocument) {
        // Enforce that the referenced url matches the url of the document that we've loaded for it!
        ASSERT(equalIgnoringFragmentIdentifier(url, externalDocument->url()));
        return externalDocument->getElementById(id);
    }

    // Exit early if the referenced url is external, and we have no externalDocument given.
    if (isExternalURIReference(iri, document))
        return 0;

    return document.getElementById(id);
}

}
