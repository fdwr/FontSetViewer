// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// Contents:    Main user interface window.
//
//----------------------------------------------------------------------------
#pragma once


class MainWindow
{
public:
    MainWindow(HWND hwnd);
    HRESULT Initialize();
    static WPARAM RunMessageLoop();

    static ATOM RegisterWindowClass();
    static INT_PTR CALLBACK StaticDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    DialogProcResult CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    STDMETHODIMP ApplyLogFontFilter(const LOGFONT& logFont);

public:
    const static wchar_t* g_windowClassName;

    enum class FontCollectionFilterMode
    {
        None, // Display all font face references without any grouping.

        // Modes that have a 1:1 correspondence with DWRITE_FONT_PROPERTY_ID.
        WssFamilyName,
        TypographicFamilyName,
        WssFaceName,
        FullName,
        Win32FamilyName,
        PostscriptName,
        DesignedScriptTag,
        SupportedScriptTag,
        SemanticTag,
        Weight,
        Stretch,
        Style,
        TypographicFaceName,

        Total,
    };

protected:
    struct FontCollectionEntry
    {
        std::wstring name;              // Name of the entry, depending on the property type (current filter mode).
        uint32_t firstFontIndex;        // Index of the first font for this entry.
        uint32_t fontCount;             // number of fonts this entry contains.

        // Cached information about the font to use for this entry
        // (which corresponds to firstFontIndex).
        std::wstring familyName;
        DWRITE_FONT_WEIGHT fontWeight;  // = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STRETCH fontStretch;// = DWRITE_FONT_STRETCH_NORMAL;
        DWRITE_FONT_STYLE fontStyle;    // = DWRITE_FONT_STYLE_NORMAL;

        std::vector<DWRITE_FONT_AXIS_VALUE> fontAxisValues;
        std::vector<DWRITE_FONT_AXIS_RANGE> fontAxisRanges;
        std::wstring filePath;
        uint32_t fontFaceIndex;         // Within an OpenType collection.
        DWRITE_FONT_SIMULATIONS fontSimulations;

        int CompareStrings(const std::wstring& a, const std::wstring& b) const
        {
            return ::CompareStringW(
                LOCALE_INVARIANT,
                NORM_IGNORECASE,
                a.c_str(),
                static_cast<int32_t>(a.length()),
                b.c_str(),
                static_cast<int32_t>(b.length())
                );
        }

        bool operator < (const FontCollectionEntry& other)
        {
            int comparison;
            comparison = CompareStrings(name, other.name);
            if (comparison  != CSTR_EQUAL           )   return comparison == CSTR_LESS_THAN;

            if (fontWeight  != other.fontWeight     )   return fontWeight     < other.fontWeight;
            if (fontStretch != other.fontStretch    )   return fontStretch    < other.fontStretch;
            if (fontStyle   != other.fontStyle      )   return fontStyle      < other.fontStyle;

            comparison = CompareStrings(familyName, other.familyName);
            if (comparison  != CSTR_EQUAL           )   return comparison == CSTR_LESS_THAN;

            return false;
        }
    };

    struct FontCollectionFilter
    {
        FontCollectionFilterMode mode;
        uint32_t selectedFontIndex; // Previous font collection index used when applying this filter.
        std::wstring parameter;
    };

protected:
    void OnSize();
    void OnMove();
    DialogProcResult CALLBACK OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
    DialogProcResult CALLBACK OnNotification(HWND hwnd, WPARAM wParam, LPARAM lParam);
    DialogProcResult CALLBACK OnDragAndDrop(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void OnKeyDown(UINT keyCode);
    void OnMenuPopup(HMENU menu, UINT position, BOOL isSystemWindowMenu);
    void MarkNeedsRepaint();
    bool ProcessCommandLine(_In_z_ const wchar_t* commandLine);
    static void ShowHelp();
    STDMETHODIMP ParseCommandLine(_In_z_ const wchar_t* commandLine);

    STDMETHODIMP OnChooseFont();
    STDMETHODIMP OpenFontFiles();
    STDMETHODIMP ReloadSystemFontSet();
    STDMETHODIMP ChooseColor(IN OUT uint32_t& color);
    STDMETHODIMP CopyToClipboard(bool copyPlainText = false);
    STDMETHODIMP CopyImageToClipboard();
    STDMETHODIMP PasteFromClipboard();
    STDMETHODIMP InitializeLanguageMenu();
    STDMETHODIMP InitializeFontCollectionListUI();
    STDMETHODIMP InitializeFontCollectionFilterUI();
    STDMETHODIMP UpdateFontCollectionListUI(uint32_t newSelectedItem = 0);
    STDMETHODIMP UpdateFontCollectionFilterUI();
    STDMETHODIMP RebuildFontCollectionList();
    STDMETHODIMP DrawFontCollectionIconPreview(const NMLVCUSTOMDRAW* customDraw);
    STDMETHODIMP RebuildFontCollectionListFromFileNames(_In_opt_z_ wchar_t const* baseFilePath, array_ref<wchar_t const> fileNames);
    STDMETHODIMP InitializeBlankFontCollection();
    void ResetFontList();

    STDMETHODIMP GetFontProperty(
        IDWriteFont* font,
        FontCollectionFilterMode filterMode,
        wchar_t const* languageName,
        _Out_ std::wstring& fontPropertyValue,
        _Out_ std::vector<std::pair<uint32_t,uint32_t> >& fontPropertyValueTokens
        );

    STDMETHODIMP AddFontToFontCollectionList(
        IDWriteFont* font,
        uint32_t firstFontIndex,
        _In_z_ wchar_t const* languageName,
        const std::wstring& name
        );

    bool DoesFontFilterApply(
        FontCollectionFilterMode filterMode,
        const std::wstring& filterParameter,
        const std::wstring& fontPropertyValue,
        const std::vector<std::pair<uint32_t, uint32_t> > fontPropertyValueTokens
        );

    bool SplitFontProperty(
        FontCollectionFilterMode filterMode,
        const std::wstring& fontPropertyValue,
        OUT std::vector<std::wstring>& fontPropertyValueArray
        );

    HRESULT SetCurrentFilter(
        FontCollectionFilterMode filterMode_ = FontCollectionFilterMode::None
        );

    HRESULT PushFilter(
        uint32_t selectedFontIndex // Must be < fontCollectionList_.size()
        );

    HRESULT PopFilter(
        uint32_t newFilterLevel
        );

    HRESULT CreateDefaultFontSet();

    enum AppendLogMode
    {
        AppendLogModeImmediate,
        AppendLogModeCached,
        AppendLogModeMessageBox,
    };
    void AppendLog(AppendLogMode appendLogMode, const wchar_t* logMessage, ...);

protected:
    HWND hwnd_ = nullptr;
    HMONITOR hmonitor_ = nullptr;
    HMODULE dwriteModule_ = nullptr;
    HIMAGELIST fontCollectionImageList_ = nullptr;

    ComPtr<IDWriteFactory>              dwriteFactory_;
    ComPtr<IDWriteRenderingParams>      renderingParams_;
    ComPtr<IDWriteBitmapRenderTarget>   renderTarget_;
    ComPtr<IDWriteFontSet>              fontSet_;
    ComPtr<IDWriteFontCollection>       fontCollection_;

    uint32_t fontColor_ = 0xFF000000;
    uint32_t backgroundColor_ = 0xFFFFFFFF;
    uint32_t highlightColor_ = 0xFFFFFFFF;
    uint32_t highlightBackgroundColor_ = 0xFFFF0000;
    uint32_t faintSelectionColor_ = 0xFF808080;
    uint32_t currentLanguageIndex_ = 0; // English US
    bool includeRemoteFonts_ = false;
    bool wantSortedFontList_ = true;
    bool showFontPreview_ = true;

    std::vector<FontCollectionEntry> fontCollectionList_;
    std::vector<FontCollectionFilter> fontCollectionFilters_;
    std::wstring cachedLog_;
    std::map<std::wstring, uint32_t> fontCollectionListStringMap_;
    FontCollectionFilterMode filterMode_ = FontCollectionFilterMode::TypographicFamilyName;

private:
    // No copy construction allowed.
    MainWindow(const MainWindow& b);
    MainWindow& operator=(const MainWindow&);
public:
    ~MainWindow();
};
