/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "TextEncoding.h"

#include "TextCodec.h"
#include "TextEncodingRegistry.h"
#include <unicode/unorm.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringView.h>

namespace WebCore {

static const TextEncoding& UTF7Encoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalUTF7Encoding("UTF-7");
    return globalUTF7Encoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalUTF7Encoding, new TextEncoding("UTF-7"));
    return *globalUTF7Encoding;
#endif
}

TextEncoding::TextEncoding(const char* name)
    : m_name(atomicCanonicalTextEncodingName(name))
    , m_backslashAsCurrencySymbol(backslashAsCurrencySymbol())
{
    // Aliases are valid, but not "replacement" itself.
    if (equalLettersIgnoringASCIICase(name, "replacement"))
        m_name = nullptr;
}

TextEncoding::TextEncoding(const String& name)
    : m_name(atomicCanonicalTextEncodingName(name))
    , m_backslashAsCurrencySymbol(backslashAsCurrencySymbol())
{
    // Aliases are valid, but not "replacement" itself.
    if (equalLettersIgnoringASCIICase(name, "replacement"))
        m_name = nullptr;
}

String TextEncoding::decode(const char* data, size_t length, bool stopOnError, bool& sawError) const
{
    if (!m_name)
        return String();

    return newTextCodec(*this)->decode(data, length, true, stopOnError, sawError);
}

#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
// NOTE: ICU's unorm_quickCheck and unorm_normalize functions are deprecated.

Vector<uint8_t> TextEncoding::encode(StringView text, UnencodableHandling handling) const
{
    if (!m_name || text.isEmpty())
        return { };

    // FIXME: Consider adding a fast case for ASCII.

    // FIXME: What's the right place to do normalization?
    // It's a little strange to do it inside the encode function.
    // Perhaps normalization should be an explicit step done before calling encode.

    auto upconvertedCharacters = text.upconvertedCharacters();

    const UChar* source = upconvertedCharacters;
    unsigned sourceLength = text.length();

    Vector<UChar> normalizedCharacters;

    UErrorCode err = U_ZERO_ERROR;
    if (unorm_quickCheck(source, sourceLength, UNORM_NFC, &err) != UNORM_YES) {
        // First try using the length of the original string, since normalization to NFC rarely increases length.
        normalizedCharacters.grow(sourceLength);
        int32_t normalizedLength = unorm_normalize(source, sourceLength, UNORM_NFC, 0, normalizedCharacters.data(), sourceLength, &err);
        if (err == U_BUFFER_OVERFLOW_ERROR) {
            err = U_ZERO_ERROR;
            normalizedCharacters.resize(normalizedLength);
            normalizedLength = unorm_normalize(source, sourceLength, UNORM_NFC, 0, normalizedCharacters.data(), normalizedLength, &err);
        }
        ASSERT(U_SUCCESS(err));

        source = normalizedCharacters.data();
        sourceLength = normalizedLength;
    }

    return newTextCodec(*this)->encode(StringView { source, sourceLength }, handling);
}

#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic pop
#endif

const char* TextEncoding::domName() const
{
    if (noExtendedTextEncodingNameUsed())
        return m_name;

    // We treat EUC-KR as windows-949 (its superset), but need to expose 
    // the name 'EUC-KR' because the name 'windows-949' is not recognized by
    // most Korean web servers even though they do use the encoding
    // 'windows-949' with the name 'EUC-KR'. 
    // FIXME: This is not thread-safe. At the moment, this function is
    // only accessed in a single thread, but eventually has to be made
    // thread-safe along with usesVisualOrdering().
#if !PLATFORM(WKC)
    static const char* const a = atomicCanonicalTextEncodingName("windows-949");
#else
    WKC_DEFINE_STATIC_TYPE(const char*, a, atomicCanonicalTextEncodingName("windows-949"));
#endif
    if (m_name == a)
        return "EUC-KR";
    return m_name;
}

bool TextEncoding::usesVisualOrdering() const
{
    if (noExtendedTextEncodingNameUsed())
        return false;

#if !PLATFORM(WKC)
    static const char* const a = atomicCanonicalTextEncodingName("ISO-8859-8");
#else
    WKC_DEFINE_STATIC_TYPE(const char*, a, atomicCanonicalTextEncodingName("ISO-8859-8"));
#endif
    return m_name == a;
}

bool TextEncoding::isJapanese() const
{
    return isJapaneseEncoding(m_name);
}

UChar TextEncoding::backslashAsCurrencySymbol() const
{
    return shouldShowBackslashAsCurrencySymbolIn(m_name) ? 0x00A5 : '\\';
}

bool TextEncoding::isNonByteBasedEncoding() const
{
    return *this == UTF16LittleEndianEncoding() || *this == UTF16BigEndianEncoding();
}

bool TextEncoding::isUTF7Encoding() const
{
    if (noExtendedTextEncodingNameUsed())
        return false;

    return *this == UTF7Encoding();
}

const TextEncoding& TextEncoding::closestByteBasedEquivalent() const
{
    if (isNonByteBasedEncoding())
        return UTF8Encoding();
    return *this; 
}

// HTML5 specifies that UTF-8 be used in form submission when a form is 
// is a part of a document in UTF-16 probably because UTF-16 is not a 
// byte-based encoding and can contain 0x00. By extension, the same
// should be done for UTF-32. In case of UTF-7, it is a byte-based encoding,
// but it's fraught with problems and we'd rather steer clear of it.
const TextEncoding& TextEncoding::encodingForFormSubmission() const
{
    if (isNonByteBasedEncoding() || isUTF7Encoding())
        return UTF8Encoding();
    return *this;
}

const TextEncoding& ASCIIEncoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalASCIIEncoding("ASCII");
    return globalASCIIEncoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalASCIIEncoding, new TextEncoding("ASCII"));
    return *globalASCIIEncoding;
#endif
}

const TextEncoding& Latin1Encoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalLatin1Encoding("latin1");
    return globalLatin1Encoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalLatin1Encoding, new TextEncoding("latin1"));
    return *globalLatin1Encoding;
#endif
}

const TextEncoding& UTF16BigEndianEncoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalUTF16BigEndianEncoding("UTF-16BE");
    return globalUTF16BigEndianEncoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalUTF16BigEndianEncoding, new TextEncoding("UTF-16BE"));
    return *globalUTF16BigEndianEncoding;
#endif
}

const TextEncoding& UTF16LittleEndianEncoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalUTF16LittleEndianEncoding("UTF-16LE");
    return globalUTF16LittleEndianEncoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalUTF16LittleEndianEncoding, new TextEncoding("UTF-16LE"));
    return *globalUTF16LittleEndianEncoding;
#endif
}

const TextEncoding& UTF8Encoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalUTF8Encoding("UTF-8");
    ASSERT(globalUTF8Encoding.isValid());
    return globalUTF8Encoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalUTF8Encoding, new TextEncoding("UTF-8"));
    ASSERT(globalUTF8Encoding->isValid());
    return *globalUTF8Encoding;
#endif
}

const TextEncoding& WindowsLatin1Encoding()
{
#if !PLATFORM(WKC)
    static TextEncoding globalWindowsLatin1Encoding("WinLatin-1");
    return globalWindowsLatin1Encoding;
#else
    WKC_DEFINE_STATIC_TYPE(TextEncoding*, globalWindowsLatin1Encoding, new TextEncoding("WinLatin-1"));
    return *globalWindowsLatin1Encoding;
#endif
}

} // namespace WebCore
