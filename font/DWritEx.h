//+---------------------------------------------------------------------------
//
//  Contents:   DWrite helper functions for commonly needed tasks.
//
//  History:    2009-09-09  dwayner
//
//----------------------------------------------------------------------------

HRESULT LoadDWrite(
    _In_z_ const wchar_t* dllPath,
    DWRITE_FACTORY_TYPE factoryType, // DWRITE_FACTORY_TYPE_SHARED
    _COM_Outptr_ IDWriteFactory** factory,
    _Out_ HMODULE& moduleHandle
    ) throw();

HRESULT CreateFontFaceFromFile(
    IDWriteFactory* factory,
    _In_z_ const wchar_t* fontFilePath,
    uint32_t fontFaceIndex,
    DWRITE_FONT_SIMULATIONS fontSimulations,  // usually just DWRITE_FONT_SIMULATIONS_NONE
    _COM_Outptr_ IDWriteFontFace** fontFace
    ) throw();

HRESULT CreateFontCollection(
    _In_ IDWriteFactory* factory,
    _In_bytecount_(fontFileNamesSize) const wchar_t* fontFileNames,
    _In_ uint32_t fontFileNamesSize, // Number of wchar_t's, not number file name count
    _COM_Outptr_ IDWriteFontCollection** fontCollection
    ) throw();

HRESULT CreateFontCollection(
    _In_ IDWriteFactory* factory,
    _In_reads_(fontFilesCount) IDWriteFontFile* const* fontFiles,
    uint32_t fontFilesCount,
    _COM_Outptr_ IDWriteFontCollection** fontCollection
    ) throw();

HRESULT GetFilePath(
    IDWriteFontFace* fontFace,
    OUT std::wstring& filePath
    ) throw();

HRESULT GetFileModifiedDate(
    IDWriteFontFace* fontFace,
    _Out_ FILETIME& fileTime
    ) throw();

HRESULT CreateTextLayout(
    IDWriteFactory* factory,
    __in_ecount(textLength) wchar_t const* text,
    uint32_t textLength,
    IDWriteTextFormat* textFormat,
    float maxWidth,
    float maxHeight,
    DWRITE_MEASURING_MODE measuringMode,
    __out IDWriteTextLayout** textLayout
    ) throw();

HRESULT GetLocalizedStringLanguage(
    IDWriteLocalizedStrings* strings,
    uint32_t stringIndex,
    OUT std::wstring& value
    ) throw();

// Try to get the string in the preferred language, else English, else the first index.
// The function does not return an error if the string does exist, just empty string.
HRESULT GetLocalizedString(
    IDWriteLocalizedStrings* strings,
    _In_opt_z_ const wchar_t* preferredLanguage,
    OUT std::wstring& value
    ) throw();

// Get the string from the given index.
// The function does not return an error if the string does exist, just empty string.
HRESULT GetLocalizedString(
    IDWriteLocalizedStrings* strings,
    uint32_t stringIndex,
    OUT std::wstring& value
    ) throw();

HRESULT GetFaceNames(
    IDWriteFont* font,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    );

// Returns the family name of thte font (in the language requested, if available, else the default English name).
HRESULT GetFontFamilyName(
    IDWriteFont* font,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    ) throw();

HRESULT GetFontFamilyName(
    IDWriteFontFamily* fontFamily,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    ) throw();

HRESULT GetLocalizedString(
    IDWriteFontSet* fontSet,
    uint32_t fontSetIndex,
    DWRITE_FONT_PROPERTY_ID fontPropertyId,
    _In_opt_z_ const wchar_t* preferredLanguage,
    OUT std::wstring& value
    ) throw();

bool IsKnownFontFileExtension(_In_z_ const wchar_t* fileExtension) throw();

// Draw a text layout to a bitmap render target.
HRESULT DrawTextLayout(
    IDWriteBitmapRenderTarget* renderTarget,
    IDWriteRenderingParams* renderingParams,
    IDWriteTextLayout* textLayout,
    float x,
    float y
    );
