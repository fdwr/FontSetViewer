//+---------------------------------------------------------------------------
//
//  Contents:   DWrite helper functions for commonly needed tasks.
//
//  History:    2009-09-09  dwayner
//
//----------------------------------------------------------------------------

#include "precomp.h"
#include <windows.h>
#include <Dwrite.h>
#include "DwritEx.h"


////////////////////////////////////////


const static DWRITE_MATRIX identityTransform = {1,0,0,1,0,0};


// Load it dynamically.
HRESULT LoadDWrite(
    _In_z_ const wchar_t* dllPath,
    DWRITE_FACTORY_TYPE factoryType, // DWRITE_FACTORY_TYPE_SHARED
    _COM_Outptr_ IDWriteFactory** factory,
    _Out_ HMODULE& moduleHandle
    ) throw()
{
    typedef HRESULT (__stdcall DWriteCreateFactory_t)(
        __in DWRITE_FACTORY_TYPE,
        __in REFIID,
        _COM_Outptr_ IUnknown**
        );

    *factory = nullptr;
    moduleHandle = nullptr;

    ModuleHandle dwriteModule = LoadLibrary(dllPath);
    if (dwriteModule == nullptr)
        return HRESULT_FROM_WIN32(GetLastError());

    DWriteCreateFactory_t* factoryFunction = (DWriteCreateFactory_t*) GetProcAddress(dwriteModule, "DWriteCreateFactory");
    if (factoryFunction == nullptr)
        return HRESULT_FROM_WIN32(GetLastError());

    // Use an isolated factory to prevent polluting the global cache.
    HRESULT hr = factoryFunction(
                    DWRITE_FACTORY_TYPE_ISOLATED,
                    __uuidof(IDWriteFactory),
                    OUT (IUnknown**)factory
                    );

    if (SUCCEEDED(hr))
    {
        moduleHandle = dwriteModule.Detach();
    }

    return hr;
}


class CustomCollectionLocalFontFileEnumerator : public ComBase<IDWriteFontFileEnumerator>
{
protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontFileEnumerator, object);
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }


public:  
    CustomCollectionLocalFontFileEnumerator()
    :   remainingFontFileNames_(nullptr),
        findHandle_(INVALID_HANDLE_VALUE)
    { }  


    ~CustomCollectionLocalFontFileEnumerator()
    {
        if (findHandle_ != INVALID_HANDLE_VALUE)
        {
            FindClose(findHandle_);
        }
    }


    HRESULT Initialize(
        _In_ IDWriteFactory* factory,
        _In_reads_(fontFileNamesSize) const wchar_t* fontFileNames,
        uint32_t fontFileNamesSize
        ) throw()
    {  
        if (factory == nullptr || fontFileNames == nullptr)
            return E_INVALIDARG;

        factory_ = factory;
        remainingFontFileNames_ = fontFileNames;
        remainingFontFileNamesEnd_ = fontFileNames + fontFileNamesSize;
        return S_OK;
    }


    IFACEMETHODIMP MoveNext(_Out_ BOOL* hasCurrentFile)  
    {  
        HRESULT hr = S_OK;
        *hasCurrentFile = false;
        currentFontFile_ = nullptr;

        try
        {
            // Get next filename.
            for (;;)
            {
                // Check for the end of the list, either reaching the end
                // of the key or double-nul termination.
                if (remainingFontFileNames_ >= remainingFontFileNamesEnd_
                ||  remainingFontFileNames_[0] == '\0')
                {
                    return hr;
                }

                if (findHandle_ == INVALID_HANDLE_VALUE)
                {
                    // Get the first file matching the mask.
                    findHandle_ = FindFirstFile(remainingFontFileNames_, OUT &findData_);
                    if (findHandle_ == INVALID_HANDLE_VALUE)
                    {
                        auto errorCode = GetLastError();
                        if (errorCode == ERROR_FILE_NOT_FOUND)
                            return S_OK;

                        return HRESULT_FROM_WIN32(errorCode);
                    }
                }
                else if (!FindNextFile(findHandle_, OUT &findData_))
                {
                    // Move to next filename (skipping the nul).
                    remainingFontFileNames_ += wcslen(remainingFontFileNames_) + 1;

                    FindClose(findHandle_);
                    findHandle_ = INVALID_HANDLE_VALUE;

                    continue; // Move to next file mask.
                }

                if (!(findData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // Skip directories.
                    break; // Have our file.
            }

            // Concatenate the path and current file name.
            const wchar_t* fileNameStart = FindFileNameStart({ remainingFontFileNames_, remainingFontFileNamesEnd_ });
            fullPath_.assign(remainingFontFileNames_, fileNameStart);
            fullPath_ += findData_.cFileName;

            #if 0
            wprintf(L"%s\n", fullPath_.c_str());
            #endif

            currentFontFile_.Clear();
            IFR(factory_->CreateFontFileReference(
                fullPath_.c_str(),
                &findData_.ftLastWriteTime,
                &currentFontFile_
                ));

            *hasCurrentFile = true;
        }
        catch (std::bad_alloc)
        {
            return HRESULT_FROM_WIN32(E_OUTOFMEMORY); // This is the only exception type we need to worry about.
        }

        return hr;
    }  
  
    IFACEMETHODIMP GetCurrentFontFile(_COM_Outptr_ IDWriteFontFile** fontFile)  
    {  
        *fontFile = currentFontFile_;
        currentFontFile_->AddRef();
        return S_OK;
    }  
  
private:  
    ComPtr<IDWriteFactory> factory_;
    ComPtr<IDWriteFontFile> currentFontFile_;
    const wchar_t* remainingFontFileNames_;
    const wchar_t* remainingFontFileNamesEnd_;
    HANDLE findHandle_;
    WIN32_FIND_DATA findData_;
    std::wstring fullPath_;
};


class CustomCollectionFontFileEnumerator : public ComBase<IDWriteFontFileEnumerator, RefCountBaseStatic>
{
protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontFileEnumerator, object);
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }


public:  
    CustomCollectionFontFileEnumerator()
    :   currentFontFile_(nullptr),
        remainingFontFiles_(nullptr),
        remainingFontFilesEnd_(nullptr)
    { }


    HRESULT Initialize(
        _In_ IDWriteFactory* factory,
        _In_reads_(fontFileCount) IDWriteFontFile* const* fontFiles,
        uint32_t fontFilesCount
        ) throw()
    {  
        if (factory == nullptr || (fontFiles == nullptr && fontFilesCount > 0))
            return E_INVALIDARG;

        remainingFontFiles_ = fontFiles;
        remainingFontFilesEnd_ = fontFiles + fontFilesCount;
        return S_OK;
    }

    IFACEMETHODIMP MoveNext(_Out_ BOOL* hasCurrentFile)  
    {  
        *hasCurrentFile = (remainingFontFiles_ < remainingFontFilesEnd_);
        currentFontFile_ = *remainingFontFiles_;
        ++remainingFontFiles_;
        return S_OK;
    }  
  
    IFACEMETHODIMP GetCurrentFontFile(_COM_Outptr_ IDWriteFontFile** fontFile)  
    {  
        *fontFile = currentFontFile_;
        currentFontFile_->AddRef();
        return S_OK;
    }  
  
private:  
    IDWriteFontFile* currentFontFile_;
    IDWriteFontFile* const* remainingFontFiles_;
    IDWriteFontFile* const* remainingFontFilesEnd_;
};


class CustomFontCollectionLoader : public ComBase<IDWriteFontCollectionLoader, RefCountBaseStatic>
{
protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontCollectionLoader, object);
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }

public:
    IFACEMETHODIMP CreateEnumeratorFromKey(
        _In_ IDWriteFactory* factory,
        _In_bytecount_(collectionKeySize) void const* collectionKey,
        uint32_t collectionKeySize,
        _COM_Outptr_ IDWriteFontFileEnumerator** fontFileEnumerator
        )
    {
        if (collectionKey == nullptr || collectionKeySize < sizeof(IDWriteFontFileEnumerator))
            return E_INVALIDARG;

        // The collectionKey actually points to the address of the custom IDWriteFontFileEnumerator.
        IDWriteFontFileEnumerator* enumerator = *reinterpret_cast<IDWriteFontFileEnumerator* const*>(collectionKey);
        enumerator->AddRef();

        *fontFileEnumerator = enumerator;
  
        return S_OK;
    }  
  
    static IDWriteFontCollectionLoader* GetInstance()  
    {  
        return &singleton_;
    }  
  
private:  
    static CustomFontCollectionLoader singleton_;
};
  
CustomFontCollectionLoader CustomFontCollectionLoader::singleton_;
  
 
HRESULT CreateFontCollection(
    _In_ IDWriteFactory* factory,
    _In_reads_(fontFileNamesSize) const wchar_t* fontFileNames,
    _In_ uint32_t fontFileNamesSize,
    _COM_Outptr_ IDWriteFontCollection** fontCollection
    ) throw()
{  
    RETURN_ON_ZERO(CustomFontCollectionLoader::GetInstance(), E_FAIL);

    HRESULT hr = S_OK;

    CustomCollectionLocalFontFileEnumerator enumerator;
    IFR(enumerator.Initialize(factory, fontFileNames, fontFileNamesSize));
    auto* enumeratorAddress = static_cast<IDWriteFontFileEnumerator*>(&enumerator);
    enumeratorAddress->AddRef();

    // Pass the address of the enumerator as the unique key.
    IFR(factory->RegisterFontCollectionLoader(CustomFontCollectionLoader::GetInstance()));
    hr = factory->CreateCustomFontCollection(CustomFontCollectionLoader::GetInstance(), &enumeratorAddress, sizeof(&enumeratorAddress), OUT fontCollection);
    IFR(factory->UnregisterFontCollectionLoader(CustomFontCollectionLoader::GetInstance()));

    return hr;
}  


HRESULT CreateFontCollection(
    _In_ IDWriteFactory* factory,
    _In_reads_(fontFilesCount) IDWriteFontFile* const* fontFiles,
    uint32_t fontFilesCount,
    _COM_Outptr_ IDWriteFontCollection** fontCollection
    )
{  
    RETURN_ON_ZERO(CustomFontCollectionLoader::GetInstance(), E_FAIL);

    HRESULT hr = S_OK;
        
    CustomCollectionFontFileEnumerator enumerator;
    IFR(enumerator.Initialize(factory, fontFiles, fontFilesCount));
    auto* enumeratorAddress = static_cast<IDWriteFontFileEnumerator*>(&enumerator);

    IFR(factory->RegisterFontCollectionLoader(CustomFontCollectionLoader::GetInstance()));
    hr = factory->CreateCustomFontCollection(CustomFontCollectionLoader::GetInstance(), &enumeratorAddress, sizeof(&enumeratorAddress), OUT fontCollection);
    IFR(factory->UnregisterFontCollectionLoader(CustomFontCollectionLoader::GetInstance()));

    return hr;
}  


HRESULT CreateFontFaceFromFile(
    IDWriteFactory* factory,
    _In_z_ const wchar_t* fontFilePath,
    uint32_t fontFaceIndex,
    DWRITE_FONT_SIMULATIONS fontSimulations,
    _COM_Outptr_ IDWriteFontFace** fontFace
    ) throw()
{
    ComPtr<IDWriteFontFile> fontFile;
    IFR(factory->CreateFontFileReference(
            fontFilePath,
            nullptr, 
            OUT &fontFile
            ));

    BOOL isSupportedFontType;
    DWRITE_FONT_FILE_TYPE fontFileType;
    DWRITE_FONT_FACE_TYPE fontFaceType;
    uint32_t numberOfFaces;

    IFR(fontFile->Analyze(
            OUT &isSupportedFontType,
            OUT &fontFileType,
            OUT &fontFaceType,
            OUT &numberOfFaces
            ));

    if (!isSupportedFontType)
    {
        return DWRITE_E_FILEFORMAT;
    }

    assert(fontFaceIndex < numberOfFaces);

    IDWriteFontFile* fontFileArray[] = {fontFile};
    IFR(factory->CreateFontFace(
            fontFaceType,
            ARRAYSIZE(fontFileArray),
            fontFileArray,
            fontFaceIndex ,
            fontSimulations,
            OUT fontFace
            ));

    return S_OK;
}


HRESULT CreateTextLayout(
    IDWriteFactory* factory,
    __in_ecount(textLength) wchar_t const* text,
    uint32_t textLength,
    IDWriteTextFormat* textFormat,
    float maxWidth,
    float maxHeight,
    DWRITE_MEASURING_MODE measuringMode,
    __out IDWriteTextLayout** textLayout
    ) throw()
{
    if (measuringMode == DWRITE_MEASURING_MODE_NATURAL)
    {
        return factory->CreateTextLayout(
                            text,
                            textLength,
                            textFormat,
                            maxWidth,
                            maxHeight,
                            OUT textLayout
                            );
    }
    else
    {
        return factory->CreateGdiCompatibleTextLayout(
                            text,
                            textLength,
                            textFormat,
                            maxWidth,
                            maxHeight,
                            1, // pixels per DIP
                            nullptr, // no transform
                            (measuringMode == DWRITE_MEASURING_MODE_GDI_NATURAL) ? true : false,
                            OUT textLayout
                            );
    }
}


HRESULT GetLocalFileLoaderAndKey(
    IDWriteFontFace* fontFace,
    _Out_ void const** fontFileReferenceKey,
    _Out_ uint32_t& fontFileReferenceKeySize,
    _Out_ IDWriteLocalFontFileLoader** localFontFileLoader
    )
{
    *localFontFileLoader = nullptr;
    *fontFileReferenceKey = nullptr;
    fontFileReferenceKeySize = 0;

    if (fontFace == nullptr)
        return E_INVALIDARG;

    ComPtr<IDWriteFontFile> fontFiles[8];
    uint32_t fontFileCount = 0;

    IFR(fontFace->GetFiles(OUT &fontFileCount, nullptr));
    if (fontFileCount > ARRAYSIZE(fontFiles))
        return E_NOT_SUFFICIENT_BUFFER;

    IFR(fontFace->GetFiles(IN OUT &fontFileCount, OUT &fontFiles[0]));

    if (fontFileCount == 0 || fontFiles[0] == nullptr)
        return E_FAIL;

    ComPtr<IDWriteFontFileLoader> fontFileLoader;
    IFR(fontFiles[0]->GetLoader(OUT &fontFileLoader));
    IFR(fontFileLoader->QueryInterface(OUT localFontFileLoader));

    IFR(fontFiles[0]->GetReferenceKey(fontFileReferenceKey, OUT &fontFileReferenceKeySize));

    return S_OK;
}


HRESULT GetFilePath(
    IDWriteFontFace* fontFace,
    _Out_ std::wstring& filePath
    ) throw()
{
    filePath.clear();
    wchar_t filePathBuffer[MAX_PATH];
    filePathBuffer[0] = '\0';

    ComPtr<IDWriteLocalFontFileLoader> localFileLoader;
    uint32_t fontFileReferenceKeySize = 0;
    const void* fontFileReferenceKey = nullptr;

    IFR(GetLocalFileLoaderAndKey(fontFace, &fontFileReferenceKey, OUT fontFileReferenceKeySize, OUT &localFileLoader));

    IFR(localFileLoader->GetFilePathFromKey(
            fontFileReferenceKey,
            fontFileReferenceKeySize,
            OUT filePathBuffer,
            ARRAY_SIZE(filePathBuffer)
            ));

    try
    {
        filePath.assign(filePathBuffer);
    }
    catch (std::bad_alloc)
    {
        return HRESULT_FROM_WIN32(E_OUTOFMEMORY); // This is the only exception type we need to worry about.
    }

    return S_OK;
}


HRESULT GetFileModifiedDate(
    IDWriteFontFace* fontFace,
    _Out_ FILETIME& fileTime
    ) throw()
{
    const static FILETIME zeroFileTime = {};
    fileTime = zeroFileTime;
    ComPtr<IDWriteLocalFontFileLoader> localFileLoader;
    uint32_t fontFileReferenceKeySize = 0;
    const void* fontFileReferenceKey = nullptr;

    IFR(GetLocalFileLoaderAndKey(fontFace, &fontFileReferenceKey, OUT fontFileReferenceKeySize, OUT &localFileLoader));

    IFR(localFileLoader->GetLastWriteTimeFromKey(
            fontFileReferenceKey,
            fontFileReferenceKeySize,
            OUT &fileTime
            ));

    return S_OK;
}


HRESULT GetLocalizedStringLanguage(
    IDWriteLocalizedStrings* strings,
    uint32_t stringIndex,
    OUT std::wstring& value
    )
{
    value.clear();

    if (strings == nullptr)
        return E_INVALIDARG;

    uint32_t length = 0;
    strings->GetLocaleNameLength(stringIndex, OUT &length);
    if (length == 0 || length == UINT32_MAX)
        return S_OK;

    try
    {
        value.resize(length);
    }
    catch (std::bad_alloc)
    {
        return HRESULT_FROM_WIN32(E_OUTOFMEMORY); // This is the only exception type we need to worry about.
    }
    IFR(strings->GetLocaleName(stringIndex, OUT &value[0], length + 1));

    return S_OK;
}


HRESULT GetLocalizedString(
    IDWriteLocalizedStrings* strings,
    _In_opt_z_ const wchar_t* preferredLanguage,
    OUT std::wstring& value
    )
{
    value.clear();

    if (strings == nullptr)
        return E_INVALIDARG;

    uint32_t stringIndex = 0;
    BOOL exists = false;
    if (preferredLanguage != nullptr)
    {
        strings->FindLocaleName(preferredLanguage, OUT &stringIndex, OUT &exists);
    }

    if (!exists)
    {
        strings->FindLocaleName(L"en-us", OUT &stringIndex, OUT &exists);
    }

    if (!exists)
    {
        stringIndex = 0; // Just try the first index.
    }

    return GetLocalizedString(strings, stringIndex, OUT value);
}


HRESULT GetLocalizedString(
    IDWriteLocalizedStrings* strings,
    uint32_t stringIndex,
    OUT std::wstring& value
    )
{
    value.clear();
    if (strings == nullptr || strings->GetCount() == 0)
        return S_OK;

    uint32_t length = 0;
    IFR(strings->GetStringLength(stringIndex, OUT &length));
    if (length == 0 || length == UINT32_MAX)
        return S_OK;

    try
    {
        value.resize(length);
    }
    catch (std::bad_alloc)
    {
        return HRESULT_FROM_WIN32(E_OUTOFMEMORY); // This is the only exception type we need to worry about.
    }
    IFR(strings->GetString(stringIndex, OUT &value[0], length + 1));

    return S_OK;
}


HRESULT GetFontFaceNameWws(
    IDWriteFont* font,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    )
{
    value.clear();
    if (font == nullptr)
        return E_INVALIDARG;

    ComPtr<IDWriteLocalizedStrings> fontFaceNames;
    IFR(font->GetFaceNames(OUT &fontFaceNames));
    IFR(GetLocalizedString(fontFaceNames, languageName, OUT value));

    return S_OK;
}


HRESULT GetFontFamilyNameWws(
    IDWriteFont* font,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    )
{
    value.clear();
    if (font == nullptr)
        return E_INVALIDARG;

    ComPtr<IDWriteLocalizedStrings> fontFamilyNames;
    ComPtr<IDWriteFontFamily> fontFamily;
    IFR(font->GetFontFamily(OUT &fontFamily));
    IFR(fontFamily->GetFamilyNames(OUT &fontFamilyNames));
    IFR(GetLocalizedString(fontFamilyNames, languageName, OUT value));

    return S_OK;
}


HRESULT GetFontFamilyNameWws(
    IDWriteFontFamily* fontFamily,
    _In_z_ wchar_t const* languageName,
    OUT std::wstring& value
    )
{
    value.clear();
    if (fontFamily == nullptr)
        return E_INVALIDARG;

    ComPtr<IDWriteLocalizedStrings> fontFamilyNames;
    IFR(fontFamily->GetFamilyNames(OUT &fontFamilyNames));
    IFR(GetLocalizedString(fontFamilyNames, languageName, OUT value));

    return S_OK;
}


HRESULT GetLocalizedString(
    IDWriteFontSet* fontSet,
    uint32_t fontSetIndex,
    DWRITE_FONT_PROPERTY_ID fontPropertyId,
    _In_opt_z_ const wchar_t* preferredLanguage,
    OUT std::wstring& value
    ) throw()
{
    value.clear();
    if (fontSet == nullptr)
        return S_OK;

    ComPtr<IDWriteLocalizedStrings> localizedStrings;
    BOOL dummyExists;
    IFR(fontSet->GetPropertyValues(fontSetIndex, fontPropertyId, OUT &dummyExists, OUT &localizedStrings));
    
    return GetLocalizedString(localizedStrings, preferredLanguage, OUT value);
}


bool IsKnownFontFileExtension(_In_z_ const wchar_t* fileExtension) throw()
{
    return (_wcsicmp(fileExtension, L".otf") == 0
        ||  _wcsicmp(fileExtension, L".ttf") == 0
        ||  _wcsicmp(fileExtension, L".ttc") == 0
        ||  _wcsicmp(fileExtension, L".tte") == 0
            );
}


class BitmapRenderTargetTextRenderer : public IDWriteTextRenderer
{
public:

    BitmapRenderTargetTextRenderer(
        IDWriteBitmapRenderTarget* renderTarget,
        IDWriteRenderingParams* renderingParams
        )
    :   renderTarget_(renderTarget),
        renderingParams_(renderingParams)
    { }

    HRESULT STDMETHODCALLTYPE DrawGlyphRun(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_ IUnknown* clientDrawingEffects
        ) throw()
    {
        if (glyphRun->glyphCount <= 0)
            return S_OK;

        uint32_t textColor = 0x000000;
        renderTarget_->DrawGlyphRun(
                baselineOriginX,
                baselineOriginY,
                DWRITE_MEASURING_MODE_NATURAL,
                glyphRun,
                renderingParams_,
                textColor,
                nullptr // don't need blackBoxRect
                );

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawUnderline(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_ IUnknown* clientDrawingEffects
        ) throw()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawStrikethrough(
        _In_ void* clientDrawingContext,
        _In_ float baselineOriginX,
        _In_ float baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_ IUnknown* clientDrawingEffects
        ) throw()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawInlineObject(
        _In_ void* clientDrawingContext,
        _In_ float originX,
        _In_ float originY,
        _In_ IDWriteInlineObject* inlineObject,
        _In_ BOOL isSideways,
        _In_ BOOL isRightToLeft,
        _In_ IUnknown* clientDrawingEffects
        ) throw()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) throw()
    {
        *isDisabled = false;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentTransform(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) throw()
    {
        *transform = identityTransform;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(
        _In_opt_ void* clientDrawingContext,
        _Out_ float* pixelsPerDip
        ) throw()
    {
        *pixelsPerDip = 1.0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) throw()
    {
        *object = nullptr;
        return E_NOINTERFACE;
    }

    unsigned long STDMETHODCALLTYPE AddRef() throw()
    {
        return 1; // Static stack class
    }

    unsigned long STDMETHODCALLTYPE Release() throw()
    {
        return 1; // Static stack class
    }

    IDWriteBitmapRenderTarget* renderTarget_; // Weak pointers because class is stack local.
    IDWriteRenderingParams* renderingParams_;
};


HRESULT DrawTextLayout(
    IDWriteBitmapRenderTarget* renderTarget,
    IDWriteRenderingParams* renderingParams,
    IDWriteTextLayout* textLayout,
    float x,
    float y
    )
{
    if (renderTarget == nullptr || renderingParams == nullptr || textLayout == nullptr)
        return E_INVALIDARG;

    BitmapRenderTargetTextRenderer textRenderer(renderTarget, renderingParams);
    return textLayout->Draw(nullptr, &textRenderer, x, y);
}
