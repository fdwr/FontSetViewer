// Include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#include "precomp.h"


__out_range(0, destMax)
size_t ConvertUtf16ToUtf32(
    __in_ecount(sourceCount) const wchar_t* sourceChars,
    __in size_t sourceCount,
    __out_ecount_part(destMax,0) char32_t* destChars,
    __in size_t destMax
    ) throw()
{
    // Can have more UTF16 code units than UTF32,
    // but never the other way around.

    size_t si = 0, di = 0;
    for ( ; di < destMax; ++di)
    {
        if (si >= sourceCount)
            break;

        char32_t ch = sourceChars[si++];

        // Just use the character if not a surrogate code point.
        // For unpaired surrogates, pass the isolated surrogate
        // through (rather than remap to U+FFFD).
        if (IsLeadingSurrogate(ch) && si < sourceCount)
        {
            char32_t leading  = ch;
            char32_t trailing = sourceChars[si];
            if (IsTrailingSurrogate(trailing))
            {
                ch = MakeUnicodeCodepoint(leading, trailing);
                ++si;
            }
        }
        destChars[di] = ch;
    }

    return di;
}


__out_range(0, destMax)
size_t ConvertUtf32ToUtf16(
    __in_ecount(sourceCount) const char32_t* sourceChars,
    __in size_t sourceCount,
    __out_ecount_part(destMax,0) wchar_t* destChars,
    __in size_t destMax
    ) throw()
{
    if ((signed)destMax <= 0)
        return 0;

    size_t si = 0, di = 0;

    for ( ; si < sourceCount; ++si)
    {
        if (di >= destMax)
            break;

        char32_t ch = sourceChars[si];

        if (ch > 0xFFFF && destMax - di >= 2)
        {
            // Split into leading and trailing surrogatse.
            // From http://unicode.org/faq/utf_bom.html#35
            destChars[di + 0] = GetLeadingSurrogate(ch);
            destChars[di + 1] = GetTrailingSurrogate(ch);
            ++di;
        }
        else
        {
            // A BMP character (or isolated surrogate)
            destChars[di + 0] = wchar_t(ch);
        }
        ++di;
    }

    return di;
}


struct UnicodeCharacterReader
{
    const wchar_t* current;
    const wchar_t* end;

    bool IsAtEnd()
    {
        return current >= end;
    }

    char32_t ReadNext()
    {
        if (current >= end)
            return 0;

        char32_t ch = *current;
        ++current;

        // Just use the character if not a surrogate code point.
        // For unpaired surrogates, pass the isolated surrogate
        // through (rather than remap to U+FFFD).
        if (IsLeadingSurrogate(ch) && current < end)
        {
            char32_t leading  = ch;
            char32_t trailing = *current;
            if (IsTrailingSurrogate(trailing))
            {
                ch = MakeUnicodeCodepoint(leading, trailing);
                ++current;
            }
        }

        return ch;
    }
};
