// Include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once


enum UnicodeCodePoint
{
    UnicodeNbsp                     = 0x0000A0,
    UnicodeSoftHyphen               = 0x0000AD,
    UnicodeEnQuadSpace              = 0x002000,
    UnicodeZeroWidthSpace           = 0x00200B,
    UnicodeIdeographicSpace         = 0x003000,
    UnicodeInlineObject             = 0x00FFFC,   // for embedded objects
    UnicodeReplacementCharacter     = 0x00FFFD,   // for invalid sequences
    UnicodeMax                      = 0x10FFFF,
    UnicodeTotal                    = 0x110000,
};


inline bool IsSurrogate(UINT32 ch) throw()
{
    // 0xD800 <= ch <= 0xDFFF
    return (ch & 0xF800) == 0xD800;
}


inline bool IsLeadingSurrogate(UINT32 ch) throw()
{
    // 0xD800 <= ch <= 0xDBFF
    return (ch & 0xFC00) == 0xD800;
}


inline bool IsTrailingSurrogate(UINT32 ch) throw()
{
    // 0xDC00 <= ch <= 0xDFFF
    return (ch & 0xFC00) == 0xDC00;
}


inline bool IsCharacterBeyondBmp(char32_t ch) throw()
{
    return ch >= 0x10000;
}


// Split into leading and trailing surrogatse.
// From http://unicode.org/faq/utf_bom.html#35
inline wchar_t GetLeadingSurrogate(char32_t ch)
{
    return wchar_t(0xD800 + (ch >> 10)  - (0x10000 >> 10));
}


inline wchar_t GetTrailingSurrogate(char32_t ch)
{
    return wchar_t(0xDC00 + (ch & 0x3FF));
}


inline char32_t MakeUnicodeCodepoint(uint32_t utf16Leading, uint32_t utf16Trailing) throw()
{
    return ((utf16Leading & 0x03FF) << 10 | (utf16Trailing & 0x03FF)) + 0x10000;
}


__out_range(0, return)
    size_t ConvertUtf16ToUtf32(
    __in_ecount(sourceCount) const wchar_t* sourceChars,
    __in size_t sourceCount,
    __out_ecount_part(destMax,return) char32_t* destChars,
    __in size_t destMax
    ) throw();


__out_range(0, return)
size_t ConvertUtf32ToUtf16(
    __in_ecount(sourceCount) const char32_t* sourceChars,
    __in size_t sourceCount,
    __out_ecount_part(destMax,return) wchar_t* destChars,
    __in size_t destMax
    ) throw();


bool IsCharacterSimple(char32_t ch);
bool IsStringSimple(
    __in_ecount(textLength) const wchar_t* text,
    __in size_t textLength
    );


void GetSimpleCharacters(__out std::vector<char32_t>& characters);


__if_exists(DWRITE_UNICODE_RANGE)
{
    typedef DWRITE_UNICODE_RANGE UnicodeRange;
}
__if_not_exists(DWRITE_UNICODE_RANGE)
{
    /// <summary>
    /// Range of Unicode codepoints.
    /// </summary>
    struct UnicodeRange
    {
        /// <summary>
        /// The first codepoint in the Unicode range.
        /// </summary>
        UINT32 first;

        /// <summary>
        /// The last codepoint in the Unicode range.
        /// </summary>
        UINT32 last;
    };
};
