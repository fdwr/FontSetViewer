//+---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft, 2008. All rights reserved.
//
//  Contents:   Shared utility functions.
//
//----------------------------------------------------------------------------
#include "precomp.h"


#if 0
bool TestBit(void const* memoryBase, uint32_t bitIndex) throw()
{
    return _bittest( reinterpret_cast<long const*>(memoryBase), bitIndex) != 0;
}


bool ClearBit(void* memoryBase, uint32_t bitIndex) throw()
{
    return _bittestandreset( reinterpret_cast<long*>(memoryBase), bitIndex) != 0;
}


bool SetBit(void* memoryBase, uint32_t bitIndex) throw()
{
    return _bittestandset( reinterpret_cast<long*>(memoryBase), bitIndex) != 0;
}
#endif


// Internal version. Should call the other two overloads publicly.
void GetFormattedString(IN OUT std::wstring& returnString, bool shouldConcatenate, const wchar_t* formatString, va_list vargs) 
{
    if (formatString != nullptr)
    {
        const size_t increasedLen = _vscwprintf(formatString, vargs) + 1; // Get string length, plus one for NUL
        const size_t oldLen = shouldConcatenate ? returnString.size() : 0;
        const size_t newLen = oldLen + increasedLen;
        returnString.resize(newLen);
        _vsnwprintf_s(&returnString[oldLen], increasedLen, increasedLen, formatString, vargs);
        returnString.resize(newLen - 1); // trim off the NUL
    }
}


void GetFormattedString(OUT std::wstring& returnString, const wchar_t* formatString, ...) 
{
    va_list vargs = nullptr;
    va_start(vargs, formatString); // initialize variable arguments
    GetFormattedString(OUT returnString, /*shouldConcatenate*/false, formatString, vargs);
    va_end(vargs); // Reset variable arguments
}


void AppendFormattedString(IN OUT std::wstring& returnString, const wchar_t* formatString, ...) 
{
    va_list vargs = nullptr;
    va_start(vargs, formatString); // initialize variable arguments
    GetFormattedString(OUT returnString, /*shouldConcatenate*/true, formatString, vargs);
    va_end(vargs); // Reset variable arguments
}


namespace
{
    struct LastError
    {
        LastError()
        {
            hr = S_OK;
            SetLastError(NOERROR);
        }

        // Update the HRESULT if a Win32 error occurred.
        // Otherwise, if success or already have an error,
        // leave the existing error code alone. This
        // preserves the first error code that happens, and
        // it avoids later check's of cleanup functions from
        // clobbering the error.
        void Check()
        {
            if (SUCCEEDED(hr))
            {
                DWORD lastError = GetLastError();
                if (lastError != NOERROR)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }

        HRESULT hr;
    };
}


HRESULT CopyToClipboard(const std::wstring& text)
{
    // Copies selected text to clipboard.

    LastError lastError;

    const uint32_t textLength = static_cast<uint32_t>(text.length());
    const wchar_t* rawInputText = text.c_str();
    
    // Open and empty existing contents.
    if (OpenClipboard(nullptr))
    {
        if (EmptyClipboard())
        {
            // Allocate room for the text
            const uint32_t byteSize = sizeof(wchar_t) * (textLength + 1);
            HGLOBAL hClipboardData  = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);

            if (hClipboardData != nullptr)
            {
                void* memory = GlobalLock(hClipboardData);
                wchar_t* rawOutputText = reinterpret_cast<wchar_t*>(memory);

                if (memory != nullptr)
                {
                    // Copy text to memory block.
                    memcpy(rawOutputText, rawInputText, byteSize);
                    rawOutputText[textLength] = '\0'; // explicit nul terminator in case there is none
                    GlobalUnlock(hClipboardData);

                    if (SetClipboardData(CF_UNICODETEXT, hClipboardData) != nullptr)
                    {
                        hClipboardData = nullptr; // system now owns the clipboard, so don't touch it.
                        // lastError.hr = S_OK;
                    }
                    lastError.Check();
                }
                lastError.Check();
                GlobalFree(hClipboardData); // free if failed (still non-null)
            }
            lastError.Check();
        }
        lastError.Check();
        CloseClipboard();
    }
    lastError.Check();

    return lastError.hr;
}


HRESULT CopyImageToClipboard(
    HWND hwnd,
    HDC hdc,
    bool isUpsideDown
    )
{
    DIBSECTION sourceBitmapInfo = {};
    HBITMAP sourceBitmap = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
    GetObject(sourceBitmap, sizeof(sourceBitmapInfo), &sourceBitmapInfo);

    LastError lastError;

    if (sourceBitmapInfo.dsBm.bmBitsPixel <= 8
    ||  sourceBitmapInfo.dsBm.bmBits == nullptr
    ||  sourceBitmapInfo.dsBm.bmWidth <= 0
    ||  sourceBitmapInfo.dsBm.bmHeight <= 0)
    {
        // Don't support paletted images, only true color.
        // Only support bitmaps where the pixels are accessible.
        return E_NOTIMPL;
    }

    if (OpenClipboard(hwnd))
    {
        if (EmptyClipboard())
        {
            const size_t pixelsByteSize = sourceBitmapInfo.dsBmih.biSizeImage;
            const size_t bufferLength = sizeof(sourceBitmapInfo.dsBmih) + pixelsByteSize;
            HGLOBAL bufferHandle = GlobalAlloc(GMEM_MOVEABLE, bufferLength);
            uint8_t* buffer = reinterpret_cast<uint8_t*>(GlobalLock(bufferHandle));

            // Copy the header.
            memcpy(buffer, &sourceBitmapInfo.dsBmih, sizeof(sourceBitmapInfo.dsBmih));

            if (isUpsideDown)
            {
                // The image is a bottom-up orientation. Though, since
                // Windows legacy bitmaps are upside down too, the two
                // upside-downs cancel out.
                memcpy(buffer + sizeof(sourceBitmapInfo.dsBmih), sourceBitmapInfo.dsBm.bmBits, pixelsByteSize);
            }
            else
            {
                // We have a standard top-down image, but DIBs when shared
                // on the clipboard are actually upside down. Simply flipping
                // the height to negative works for a few applications, but
                // it confuses most.

                const size_t bytesPerRow = sourceBitmapInfo.dsBm.bmWidthBytes;
                uint8_t* destRow         = buffer + sizeof(sourceBitmapInfo.dsBmih);
                uint8_t* sourceRow       = reinterpret_cast<uint8_t*>(sourceBitmapInfo.dsBm.bmBits)
                                            + (sourceBitmapInfo.dsBm.bmHeight - 1) * bytesPerRow;

                // Copy each scanline in backwards order.
                for (long y = 0; y < sourceBitmapInfo.dsBm.bmHeight; ++y)
                {
                    memcpy(destRow, sourceRow, bytesPerRow);
                    sourceRow = PtrAddByteOffset(sourceRow, -ptrdiff_t(bytesPerRow));
                    destRow   = PtrAddByteOffset(destRow,    bytesPerRow);
                }
            }

            GlobalUnlock(bufferHandle);

            if (SetClipboardData(CF_DIB, bufferHandle) == nullptr)
            {
                lastError.Check();
                GlobalFree(bufferHandle);
            }
        }
        lastError.Check();

        CloseClipboard();
    }
    lastError.Check();

    return lastError.hr;
}


HRESULT PasteFromClipboard(OUT std::wstring& text)
{
    // Copy Unicode text from clipboard.

    LastError lastError;

    if (OpenClipboard(nullptr))
    {
        HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT);

        if (hClipboardData != nullptr)
        {
            // Get text and size of text.
            size_t byteSize                 = GlobalSize(hClipboardData);
            void* memory                    = GlobalLock(hClipboardData); // [byteSize] in bytes
            const wchar_t* rawText          = reinterpret_cast<const wchar_t*>(memory);
            uint32_t textLength             = static_cast<uint32_t>(wcsnlen(rawText, byteSize / sizeof(wchar_t)));

            if (memory != nullptr)
            {
                try
                {
                    text.assign(rawText, textLength);
                    // lastError.hr = S_OK;
                }
                catch (std::bad_alloc)
                {
                    lastError.hr = E_OUTOFMEMORY;
                }

                GlobalUnlock(hClipboardData);
            }
            lastError.Check();
        }
        lastError.Check();
        CloseClipboard();
    }
    lastError.Check();

    return lastError.hr;
}


void TrimSpaces(IN OUT std::wstring& text)
{
    // Trim space (U+0020) and tab. It does not trim all whitespace, like U+200X
    // or the new line controls.

    // Trim trailing spaces
    size_t lastPos = text.find_last_not_of(L" \t");
    if (lastPos != std::string::npos)
    {
        text.erase(lastPos+1);
    }

    // Trim leading spaces
    size_t firstPos = text.find_first_not_of(L" \t");
    if (firstPos != 0)
    {
        if (firstPos == std::string::npos)
            firstPos = text.size();
        text.erase(0, firstPos);
    }
}


void GetCommandLineArguments(_Inout_ std::wstring& commandLine)
{
    // Get the original command line argument string as a single string,
    // not the preparsed argv or CommandLineToArgvW, which fragments everything
    // into separate words which aren't really useable when you have your own
    // syntax to parse. Skip the module filename because we're only interested
    // in the parameters following it.

    // Skip the module name.
    const wchar_t* initialCommandLine = GetCommandLine();
    wchar_t ch = initialCommandLine[0];
    if (ch == '"')
    {
        do
        {
            ch = *(++initialCommandLine);
        } while (ch != '\0' && ch != '"');
        if (ch == '"')
            ++initialCommandLine;
    }
    else
    {
        while (ch != '\0' && ch != ' ')
        {
            ch = *(++initialCommandLine);
        }
    }

    // Assign it and strip any leading or trailing spaces.
    auto commandLineLength = wcslen(initialCommandLine);
    commandLine.assign(initialCommandLine, &initialCommandLine[commandLineLength]);
    TrimSpaces(IN OUT commandLine);
}


void ConvertText(
    array_ref<char const> utf8text,
    OUT std::wstring& utf16text
    )
{
    // This function can only throw if out-of-memory when resizing utf16text.
    // If utf16text is already reserve()'d, no exception will happen.

    // Skip byte-order-mark.
    const static uint8_t utf8bom[] = {0xEF,0xBB,0xBF};
    int32_t startingOffset = 0;
    if (utf8text.size() >= ARRAY_SIZE(utf8bom)
    &&  memcmp(utf8text.data(), utf8bom, ARRAY_SIZE(utf8bom)) == 0)
    {
        startingOffset = ARRAYSIZE(utf8bom);
    }

    utf16text.resize(utf8text.size());

    // Convert UTF-8 to UTF-16.
    int32_t charsConverted =
        MultiByteToWideChar(
            CP_UTF8,
            0, // no flags for UTF8 (we allow invalid characters for testing)
            (LPCSTR)&utf8text[startingOffset],
            int32_t(utf8text.size() - startingOffset),
            OUT const_cast<wchar_t*>(utf16text.data()), // workaround issue http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-active.html#2391
            int32_t(utf16text.size())
            );

    // Shrink to actual size.
    utf16text.resize(charsConverted);
}


void ConvertText(
    array_ref<wchar_t const> utf16text,
    OUT std::string& utf8text
    )
{
    // Convert UTF-8 to UTF-16.
    int32_t charsConverted =
        WideCharToMultiByte(
        CP_UTF8,
        0, // no flags for UTF8 (we allow invalid characters for testing)
        utf16text.data(),
        int32_t(utf16text.size()),
        nullptr, // null destination buffer the first time to get estimate.
        0,
        nullptr, // defaultChar
        nullptr // usedDefaultChar
        );

    utf8text.resize(charsConverted);

    // Convert UTF-8 to UTF-16.
    WideCharToMultiByte(
        CP_UTF8,
        0, // no flags for UTF8 (we allow invalid characters for testing)
        utf16text.data(),
        int32_t(utf16text.size()),
        OUT &utf8text[0],
        int32_t(utf8text.size()),
        nullptr, // defaultChar
        nullptr // usedDefaultChar
        );
}
