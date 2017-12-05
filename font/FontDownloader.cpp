#include "../precomp.inc"
#include <WinHttp.h>
#include <WinIoCtl.h>

using InternetHandle = AutoResource<HINTERNET, HandleResourceTypePolicy<HINTERNET, BOOL(WINAPI*)(HINTERNET), &WinHttpCloseHandle> >;

class InternetDownloader
{
protected:
    InternetHandle internetSession_;
    InternetHandle internetConnection_;
    InternetHandle internetRequest_;

public:
    HRESULT EnsureInternetSession()
    {
        if (internetSession_ == nullptr)
        {
            // Use WinHttpOpen to obtain a session handle.
            internetSession_ =
                WinHttpOpen(L"FontSetViewer Test Application 1.0",
                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                    WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS,
                    0       
                    );

            if (internetSession_ == nullptr)
                return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }


    static HRESULT GetServerAndFilePathFromUrl(
        std::wstring const& url,
        _Out_ std::wstring& serverName,
        _Out_ std::wstring& filePath
        )
    {
        URL_COMPONENTS urlComponents = {};
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.dwHostNameLength = (unsigned long)-1;
        urlComponents.dwUrlPathLength = (unsigned long)-1;

        if (!WinHttpCrackUrl(url.c_str(), static_cast<unsigned long>(url.size()), 0, OUT &urlComponents))
            return HRESULT_FROM_WIN32(GetLastError());

        serverName.reserve(urlComponents.dwHostNameLength);
        filePath.reserve(urlComponents.dwUrlPathLength);

        serverName.assign(urlComponents.lpszHostName, urlComponents.lpszHostName + urlComponents.dwHostNameLength);
        filePath.assign(urlComponents.lpszUrlPath, urlComponents.lpszUrlPath + urlComponents.dwUrlPathLength);

        return S_OK;
    }


    void clear()
    {
        internetRequest_.clear();
        internetConnection_.clear();
        internetSession_.clear();
    }


    HRESULT VerifySuccessStatusCode()
    {
        uint32_t statusCode = 0;
        unsigned long bufferByteLength = sizeof(statusCode);
        if (!WinHttpQueryHeaders(
            internetRequest_,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            OUT &statusCode,
            IN OUT &bufferByteLength,
            WINHTTP_NO_HEADER_INDEX
            ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Accept any informational or successful status codes, but not errors or redirects.
        if (statusCode < 100 || statusCode >= 300)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        return S_OK;
    }


    HRESULT PrepareDownloadRequest(
        std::wstring const& url
        )
    {
        std::wstring serverName;
        std::wstring filePath;
        IFR(GetServerAndFilePathFromUrl(url, OUT serverName, OUT filePath));

        IFR(EnsureInternetSession());

        // Specify an HTTP server.
        if (internetConnection_ == nullptr)
        {
            internetConnection_ =
                WinHttpConnect(
                    internetSession_,
                    serverName.c_str(),
                    INTERNET_DEFAULT_HTTP_PORT, // INTERNET_DEFAULT_HTTPS_PORT
                    0 // reserved
                    );

            if (internetConnection_ == nullptr)
                return HRESULT_FROM_WIN32(GetLastError());
        }

        // Create an HTTP request handle.
        internetRequest_ =
            WinHttpOpenRequest(
                internetConnection_,
                L"GET",
                filePath.c_str(),
                NULL, // Version, use HTTP 1.1
                WINHTTP_NO_REFERER, 
                WINHTTP_DEFAULT_ACCEPT_TYPES, 
                0 // WINHTTP_FLAG_SECURE
                );
        if (internetRequest_ == nullptr)
            return HRESULT_FROM_WIN32(GetLastError());

        return S_OK;
    }


    HRESULT SendDownloadRequest(
        uint64_t fileOffset,
        uint64_t fragmentSize
        )
    {
        // Send a request.
        // Range: bytes=1073152-64313343
        std::wstring additionalHeader;
        if (fileOffset != 0 || fragmentSize != UINT64_MAX)
        {
            // If the range is anything except the entire file, request a specific range.
            GetFormattedString(OUT additionalHeader, L"Range: bytes=%lld-%lld", fileOffset, fileOffset + fragmentSize - 1);
        }

        if (!WinHttpSendRequest(
                internetRequest_,
                additionalHeader.c_str(), // Or else WINHTTP_NO_ADDITIONAL_HEADERS and length 0
                static_cast<unsigned long>(additionalHeader.size()),
                WINHTTP_NO_REQUEST_DATA, 0,
                0,
                0
                ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (!WinHttpReceiveResponse(internetRequest_, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());

        IFR(VerifySuccessStatusCode());

        return S_OK;
    }


    HRESULT DownloadFile(
        std::wstring const& url,
        _Out_ std::vector<uint8_t>& buffer
        )
    {
        // This function only works with small enough files that fit into memory.

        IFR(PrepareDownloadRequest(url));
        IFR(SendDownloadRequest(0, UINT64_MAX));

        // Read the file size.
        uint64_t fileSize = 0;
        unsigned long headerBufferLength = sizeof(fileSize);
        if (!WinHttpQueryHeaders(
            internetRequest_,
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            OUT &fileSize,
            IN OUT &headerBufferLength,
            WINHTTP_NO_HEADER_INDEX
            ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (fileSize >= UINT32_MAX)
        {
            return E_OUTOFMEMORY;
        }

        uint64_t bytesActuallyRead;
        buffer.resize(static_cast<size_t>(fileSize));
        IFR(DownloadRequestIntoBuffer(buffer.data(), fileSize, OUT bytesActuallyRead));

        return S_OK;
    }


    HRESULT DownloadRequestIntoBuffer(
        _Out_writes_bytes_(bufferSize) void* buffer,
        uint64_t bytesToRead,
        _Out_ uint64_t& bytesActuallyRead
        )
    {
        // Read data until there is nothing left.
                
        uint8_t* byteBuffer = reinterpret_cast<uint8_t*>(buffer);
        uint64_t bytesReadTotal = 0;
        unsigned long bytesAvailable = 0;
        do 
        {
            // Check for available data.
            bytesAvailable = 0;
            if (!WinHttpQueryDataAvailable(internetRequest_, OUT &bytesAvailable))
                return HRESULT_FROM_WIN32(GetLastError());

            if (bytesAvailable > bytesToRead)
                bytesAvailable = static_cast<unsigned long>(bytesToRead);

            unsigned long bytesRead = 0;
            if (!WinHttpReadData(internetRequest_, OUT &byteBuffer[bytesReadTotal], bytesAvailable, OUT &bytesRead))
                return HRESULT_FROM_WIN32(GetLastError());

            bytesReadTotal += bytesRead;
            bytesToRead -= bytesRead;

        } while (bytesAvailable > 0 && bytesToRead > 0);

        bytesActuallyRead = bytesReadTotal;

        return S_OK;
    }


    HRESULT GetFileSizeAndDate(
        std::wstring const& url,
        _Out_ uint64_t& fileSize,
        _Out_ FILETIME& fileTime
        )
    {
        fileSize = 0;
        fileTime = {0};

        IFR(EnsureInternetSession());

        std::wstring serverName;
        std::wstring filePath;
        IFR(GetServerAndFilePathFromUrl(url, OUT serverName, OUT filePath));

        // Specify an HTTP server.
        internetConnection_ =
            WinHttpConnect(
                internetSession_,
                serverName.c_str(),
                INTERNET_DEFAULT_HTTP_PORT, // INTERNET_DEFAULT_HTTPS_PORT
                0 // reserved
                );

        if (internetConnection_ == nullptr)
            return HRESULT_FROM_WIN32(GetLastError());

        // Create an HTTP request handle.
        internetRequest_ =
            WinHttpOpenRequest(
                internetConnection_,
                L"HEAD",
                filePath.c_str(),
                nullptr, // Version, use HTTP 1.1
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0 // WINHTTP_FLAG_SECURE
                );

        if (internetRequest_ == nullptr)
            return HRESULT_FROM_WIN32(GetLastError());

        if (!WinHttpSendRequest(
                internetRequest_,
                WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0,
                0, // totalLength
                NULL // context
                ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (!WinHttpReceiveResponse(internetRequest_, nullptr))
            return HRESULT_FROM_WIN32(GetLastError());

        IFR(VerifySuccessStatusCode());

        // Read the file size.
        uint32_t fileSizeUint32;
        unsigned long headerBufferLength = sizeof(fileSizeUint32);
        if (!WinHttpQueryHeaders(
                internetRequest_,
                WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                OUT &fileSizeUint32,
                IN OUT &headerBufferLength,
                WINHTTP_NO_HEADER_INDEX
                ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        fileSize = fileSizeUint32; // Do I need to parse the number myself to get values larger than 32bits?

        // Read the file date.
        SYSTEMTIME systemTime;
        headerBufferLength = sizeof(systemTime);
        if (!WinHttpQueryHeaders(
                internetRequest_,
                WINHTTP_QUERY_LAST_MODIFIED | WINHTTP_QUERY_FLAG_SYSTEMTIME,
                WINHTTP_HEADER_NAME_BY_INDEX,
                OUT &systemTime,
                IN OUT &headerBufferLength,
                WINHTTP_NO_HEADER_INDEX
                ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        SystemTimeToFileTime(&systemTime, OUT &fileTime);

        return S_OK;
    }
};


interface RemoteFontFileStreamInterfaceBinding : public IDWriteRemoteFontFileStream
{
};

class RemoteFontFileStream : public ComBase<RemoteFontFileStreamInterfaceBinding>
{
public:
    static const uint64_t chunkSize_ = 16384;

protected:
    std::wstring url_;
    std::wstring fileName_;
    std::vector<uint8_t> chunkMap_;
    ComPtr<IDWriteFontDownloadQueue> downloadManager_; // optionally null
    ComPtr<IDWriteFontFileLoader> fontFileLoader_;
    uint64_t fileSize_ = 0;
    uint8_t* streamMemory_ = nullptr;
    FileHandle fileHandle_;
    FileHandle chunkMapFileHandle_;
    InternetDownloader internetDownloader_;

protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteRemoteFontFileStream, object);
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontFileStream, object);
        COM_BASE_RETURN_INTERFACE_AMBIGUOUS(iid, IUnknown, object, static_cast<IDWriteRemoteFontFileStream*>(this));
        COM_BASE_RETURN_NO_INTERFACE(object);
    }

public:
    RemoteFontFileStream()
    { }

    HRESULT Initialize(
        const wchar_t* url,
        IDWriteFontDownloadQueue* downloadManager,
        IDWriteFontFileLoader* fontFileLoader
        )
    {
        if (url == nullptr)
            return E_INVALIDARG;

        downloadManager_ = downloadManager;
        fontFileLoader_ = fontFileLoader;
        streamMemory_ = nullptr;
        url_ = url;

        // Create the file name from the URL.
        // Just use a simple munging to underscores for invalid characters.
        GetFileNameFromUrl(url_, OUT fileName_);

        fileHandle_ = CreateFile(
            fileName_.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

        if (fileHandle_ == INVALID_HANDLE_VALUE)
        {
            auto lastError = GetLastError();
            if (lastError == ERROR_FILE_NOT_FOUND || lastError == ERROR_PATH_NOT_FOUND)
            {
                if (downloadManager_ != nullptr)
                {
                    // downloadManager_->EnqueueFileFragmentDownload(
                    //     fontFileLoader_,
                    //     url_.c_str(),
                    //     (url_.size() + 1) * sizeof(url_[0]),
                    //     0, // fileOffset,
                    //     0 // fragmentSize
                    //     );
                }

                return DWRITE_E_REMOTEFONT;
            }
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Do not change the file time upon writes, leaving it the same as the
        // original server time, since later writes are just partial hydration
        // rather than actual meaningful content changes.
        FILETIME fileTime = { 0xFFFFFFFF, 0xFFFFFFFF };
        SetFileTime(fileHandle_, nullptr, &fileTime, &fileTime);

        LARGE_INTEGER fileSize;
        ::GetFileSizeEx(fileHandle_, OUT &fileSize);
        fileSize_ = fileSize.QuadPart;
        auto numberOfChunks = static_cast<uint32_t>((fileSize_ + chunkSize_ - 1) / chunkSize_);
        chunkMap_.resize(numberOfChunks);

        // Allocate space for later file reads, equal to the file size.
        streamMemory_ = reinterpret_cast<uint8_t*>(malloc(size_t(fileSize_)));
        if (streamMemory_ == nullptr)
            return HRESULT_FROM_WIN32(E_OUTOFMEMORY); // This is the only exception type we need to worry about.

        // Read the initial file contents into memory.
        ReadFile(fileHandle_, OUT &streamMemory_[0], static_cast<uint32_t>(fileSize_), nullptr, nullptr);

        // Read the chunk map in too.
        std::wstring chunkMapFileName = fileName_;
        chunkMapFileName.append(L"_ChunkMap");
        chunkMapFileHandle_ = CreateFile(
            chunkMapFileName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );
        if (chunkMapFileHandle_ == INVALID_HANDLE_VALUE)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        ReadFile(chunkMapFileHandle_, OUT chunkMap_.data(), static_cast<uint32_t>(chunkMap_.size()), nullptr, nullptr);

        return S_OK;
    }


    ~RemoteFontFileStream()
    {
        free(streamMemory_);
    }


    static void GetCachedFontFilesPath(_Out_ std::wstring& cachePath)
    {
        cachePath.resize(MAX_PATH + 1);
        auto fileNameStartingIndex = GetTempPath(MAX_PATH + 1, OUT &cachePath[0]);
        cachePath.resize(fileNameStartingIndex);
    }


    static void GetFileNameFromUrl(std::wstring const& url, _Inout_ std::wstring& fileName)
    {
        GetCachedFontFilesPath(OUT fileName);

        // Grab the file name from the URL.
        // Just use a simple munging to underscores for invalid characters.
        const size_t filePrefixLength = 11; // 'CachedFont_'
        auto fileNameStartingIndex = fileName.size();
        fileNameStartingIndex += filePrefixLength;
        fileName.append(L"CachedFont_", filePrefixLength);
        fileName.append(FindFileNameStart(url.c_str(), url.c_str() + url.size()));
        for (size_t i = fileNameStartingIndex, ci = fileName.size(); i < ci; ++i)
        {
            auto c = fileName[i];
            switch (c)
            {
            case '/':
            case '\\':
            case '?':
            case '*':
            case ':':
                fileName[i] = '_';
            }
        }
    }


    static HRESULT DeleteCachedFontFiles()
    {
        WIN32_FIND_DATA findData;

        // Get the first file matching the mask.
        wchar_t filePath[MAX_PATH + 1];
        auto fileNameStartingIndex = GetTempPath(ARRAYSIZE(filePath), OUT &filePath[0]);
        wcsncat_s(IN OUT filePath, L"CachedFont_*", ARRAYSIZE(filePath));

        HANDLE findHandle = FindFirstFile(filePath, OUT &findData);
        if (findHandle == INVALID_HANDLE_VALUE)
        {
            auto errorCode = GetLastError();
            if (errorCode == ERROR_FILE_NOT_FOUND)
                return S_OK;

            return HRESULT_FROM_WIN32(errorCode);
        }

        do
        {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue; // Skip directories.

            filePath[fileNameStartingIndex] = '\0';
            wcsncat_s(IN OUT filePath, findData.cFileName, ARRAYSIZE(filePath));
            DeleteFile(filePath);

        } while (FindNextFile(findHandle, OUT &findData));

        FindClose(findHandle);
        return S_OK;
    }


    IFACEMETHODIMP DownloadFileInformation()
    {
        return E_NOTIMPL;
    }


    HRESULT CheckFileFragment(
        uint64_t fileOffset,
        uint64_t fragmentSize,
        _Out_ BOOL* fragmentIsReady
        )
    {
        *fragmentIsReady = false;

        if (fileOffset + fragmentSize < fileOffset || fileOffset + fragmentSize > fileSize_)
            return E_INVALIDARG;

        static_assert(((chunkSize_ - 1) & chunkSize_) == 0, "Chunk size must be a power of two.");
        uint32_t const lowFilePosition = fileOffset & ~(chunkSize_ - 1);
        uint32_t const highFilePosition = (fileOffset + fragmentSize + chunkSize_ - 1) & ~(chunkSize_ - 1);
        uint32_t const lowChunkMapIndex = uint32_t(lowFilePosition / chunkSize_);
        uint32_t const highChunkMapIndex = std::min(uint32_t(highFilePosition / chunkSize_), static_cast<uint32_t>(chunkMap_.size()));

        bool allChunksArePresent = true;
        for (auto chunkMapIndex = lowChunkMapIndex; chunkMapIndex < highChunkMapIndex; ++chunkMapIndex)
        {
            if (!chunkMap_[chunkMapIndex])
            {
                allChunksArePresent = false;
                break;
            }
        }

        if (allChunksArePresent)
        {
            *fragmentIsReady = true;
        }
        else if (downloadManager_ != nullptr)
        {
            // downloadManager_->EnqueueFileFragmentDownload(
            //     fontFileLoader_,
            //     url_.c_str(),
            //     (url_.size() + 1) * sizeof(url_[0]),
            //     fileOffset,
            //     fragmentSize
            //     );
        }

        return S_OK;
    }


    IFACEMETHODIMP DownloadFileFragments(
        _In_reads_(fragmentCount) DWRITE_FILE_FRAGMENT const* fileFragments,
        uint32_t fragmentCount
        )
    {
        return E_NOTIMPL;
    }


    IFACEMETHODIMP ReadFileFragment(
        _Outptr_result_bytebuffer_(fragmentSize) void const** fragmentStart,
        uint64_t fileOffset,
        uint64_t fragmentSize,
        _Out_ void** fragmentContext
        )
    {
        *fragmentStart = &streamMemory_[fileOffset];
        *fragmentContext = nullptr;

        BOOL fileFragmentIsReady = false;
        IFR(CheckFileFragment(fileOffset, fragmentSize, OUT &fileFragmentIsReady));
        if (!fileFragmentIsReady)
            return DWRITE_E_REMOTEFONT;

        return S_OK;
    }


    IFACEMETHODIMP DownloadFileFragment(
        uint64_t fileOffset,
        uint64_t fragmentSize
        )
    {
        if (fragmentSize == 0)
            return S_OK;

        if (fileOffset + fragmentSize < fileOffset || fileOffset + fragmentSize > fileSize_)
            return E_INVALIDARG;

        static_assert(((chunkSize_ - 1) & chunkSize_) == 0, "Chunk size must be a power of two.");
        uint32_t const lowFilePosition = fileOffset & ~(chunkSize_ - 1);
        uint32_t const highFilePosition = (fileOffset + fragmentSize + chunkSize_ - 1) & ~(chunkSize_ - 1);
        uint32_t const totalBytesToRead = highFilePosition - lowFilePosition;
        uint32_t const lowChunkMapIndex  = uint32_t(lowFilePosition / chunkSize_);
        uint32_t const highChunkMapIndex = std::min(uint32_t(highFilePosition / chunkSize_), static_cast<uint32_t>(chunkMap_.size()));

        // Read the file from the server.
        uint64_t totalBytesActuallyRead;
        IFR(internetDownloader_.PrepareDownloadRequest(url_));
        IFR(internetDownloader_.SendDownloadRequest(lowFilePosition, totalBytesToRead));
        IFR(internetDownloader_.DownloadRequestIntoBuffer(&streamMemory_[lowFilePosition], totalBytesToRead, OUT totalBytesActuallyRead));

        LARGE_INTEGER filePosition; filePosition.QuadPart = lowFilePosition;
        SetFilePointerEx(fileHandle_, filePosition, nullptr, FILE_BEGIN);
        WriteFile(fileHandle_, &streamMemory_[lowFilePosition], static_cast<uint32_t>(totalBytesActuallyRead), nullptr, nullptr);

        // Update the chunk map.
        for (auto chunkMapIndex = lowChunkMapIndex; chunkMapIndex < highChunkMapIndex; ++chunkMapIndex)
        {
            chunkMap_[chunkMapIndex] = true;
        }

        // Updated the chunkmap file contents.
        SetFilePointer(chunkMapFileHandle_, lowChunkMapIndex, nullptr, FILE_BEGIN);
        WriteFile(chunkMapFileHandle_, chunkMap_.data() + lowChunkMapIndex, static_cast<uint32_t>(highChunkMapIndex - lowChunkMapIndex), nullptr, nullptr);

        return S_OK;
    }


    IFACEMETHODIMP IsFileFragmentLocal(
        uint64_t fileOffset,
        uint64_t fragmentSize,
        _Out_ BOOL* isLocal
        ) override
    {
        *isLocal = false;
        return E_NOTIMPL;
    }


    IFACEMETHODIMP_(DWRITE_LOCALITY) GetLocality() override
    {
        return DWRITE_LOCALITY_REMOTE;
    }


    IFACEMETHODIMP_(void) ReleaseFileFragment(
        void* fragmentContext
        )
    {
        // Nothing to do.
    }


    IFACEMETHODIMP GetFileSize(
        _Out_ uint64_t* fileSize
        )
    {
        *fileSize = fileSize_;
        return S_OK;
    }


    IFACEMETHODIMP GetLocalFileSize(
        _Out_ UINT64* localFileSize
        )
    {
        // hack: should check chunkmap.
        *localFileSize = fileSize_;
        return S_OK;
    }


    STDMETHOD(GetLastWriteTime)(
        _Out_ uint64_t* lastWriteTime
        )
    {
        if (!GetFileTime(fileHandle_, nullptr, nullptr, OUT reinterpret_cast<FILETIME*>(lastWriteTime)))
        {
            *lastWriteTime = 0;
        }
        return S_OK;
    }


    IFACEMETHODIMP PrefetchRemoteFileFragment(
        uint64_t fileOffset,
        uint64_t fragmentSize,
        _Out_ BOOL* fileFragmentIsReady
        )
    {
        IFR(CheckFileFragment(fileOffset, fragmentSize, OUT fileFragmentIsReady));
        return S_OK;
    }

    IFACEMETHODIMP CancelDownload() throw() override
    {
        return E_NOTIMPL;
    }
};


class RemoteStreamFontFileLoader : public ComBase<IDWriteRemoteFontFileLoader, RefCountBaseStatic>
{
protected:
    ComPtr<IDWriteFontDownloadQueue> downloadManager_;
    InternetDownloader internetDownloader_;

protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteRemoteFontFileLoader, object);
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontFileLoader, object);
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }

public:
    IFACEMETHODIMP CreateStreamFromKey(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        uint32_t fontFileReferenceKeySize,
        _COM_Outptr_ IDWriteFontFileStream** fontFileStream
        )
    {
        if (fontFileReferenceKey == nullptr || fontFileReferenceKeySize < sizeof(wchar_t))
            return E_INVALIDARG;

        const wchar_t* urlPointer = reinterpret_cast<const wchar_t*>(fontFileReferenceKey);

        auto* newFontFileStream = new RemoteFontFileStream();
        ComPtr<IDWriteFontFileStream> fontFileStreamScope(newFontFileStream);
        IFR(newFontFileStream->Initialize(urlPointer, downloadManager_, this));
        *fontFileStream = fontFileStreamScope.Detach();

        return S_OK;
    }  
  

    static RemoteStreamFontFileLoader* GetInstance()  
    {  
        return &singleton_;
    }


    IFACEMETHODIMP GetLocalityFromKey(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        uint32_t fontFileReferenceKeySize,
        _Out_ DWRITE_LOCALITY* fileLocality
        ) override
    {
        // Hack - always return partial.
        *fileLocality = DWRITE_LOCALITY_PARTIAL;
        return S_OK;
    }


    IFACEMETHODIMP CreateRemoteStreamFromKey(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        _COM_Outptr_ IDWriteRemoteFontFileStream** fontFileStream
        ) override
    {
        return E_NOTIMPL;
    }


    IFACEMETHODIMP DownloadStreamInformationFromKey(
        _In_reads_bytes_(fontFileReferenceKeySize) void const* fontFileReferenceKey,
        uint32_t fontFileReferenceKeySize
        )
    {
        const wchar_t* urlPointer = reinterpret_cast<const wchar_t*>(fontFileReferenceKey);
        std::wstring url(urlPointer);
        std::wstring fileName;
        RemoteFontFileStream::GetFileNameFromUrl(url, OUT fileName);

        auto fileAttributes = GetFileAttributes(fileName.c_str());
        if (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            return S_OK; // Already created.
        }

        // Read the date from the server.
        uint64_t fileSize;
        FILETIME fileTime = { 0xFFFFFFFF, 0xFFFFFFFF };
        internetDownloader_.GetFileSizeAndDate(url, OUT fileSize, OUT fileTime);

        // Create the sparse file.
        FileHandle fileHandle = CreateFile(
            fileName.c_str(),
            GENERIC_READ|GENERIC_WRITE,
            0, // No file sharing allowed until size and date or set, not even FILE_SHARE_READ.
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );
        if (fileHandle == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD bytesReturned;
        DeviceIoControl(
            fileHandle,
            FSCTL_SET_SPARSE,
            NULL,
            0,
            NULL,
            0,
            OUT &bytesReturned,
            NULL
            );
        // If an error occurs trying to set it to sparse, oh well. It's continuable.

        // Set the initial size and date.
        LARGE_INTEGER fileSizeLi;
        fileSizeLi.QuadPart = fileSize;
        SetFilePointerEx(fileHandle, fileSizeLi, nullptr, FILE_BEGIN);
        SetEndOfFile(fileHandle);
        SetFileTime(fileHandle, nullptr, &fileTime, &fileTime);

        fileHandle.clear();

        return S_OK;
    }


    void SetDownloadManager(IDWriteFontDownloadQueue* downloadManager)
    {
        downloadManager_ = downloadManager;
    }

    void Finalize() override
    {
        downloadManager_.clear();
        internetDownloader_.clear();
    }
  
private:  
    static RemoteStreamFontFileLoader singleton_;
};

RemoteStreamFontFileLoader RemoteStreamFontFileLoader::singleton_;


class RemoteFontDownloadManager : public ComBase<IDWriteFontDownloadQueue, RefCountBaseStatic>
{
private:
    struct EnqueuedRequest
    {
        ComPtr<IDWriteFontFileLoader> fontLoader;
        std::vector<uint8_t> fileKey;
        std::vector<Range> ranges;
    };

    struct RangeComparer
    {
        bool operator() (const Range& lhs, const Range& rhs) const throw()
        {
            if (lhs.begin < rhs.begin) return true;
            if (lhs.begin > rhs.begin) return false;
            return (lhs.end < rhs.end);
        }
    };

    static RemoteFontDownloadManager singleton_;
    std::vector<EnqueuedRequest> enqueuedRequests_;

protected:
    IFACEMETHODIMP QueryInterface(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IDWriteFontDownloadQueue, object);
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }

public:
    static IDWriteFontDownloadQueue* GetInstance()  
    {  
        return &singleton_;
    }

    STDMETHODIMP RegisterRemoteFontHandler(
        IDWriteFontDownloadListener* remoteFontHandler
        ) throw()
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP UnregisterRemoteFontHandler(
        IDWriteFontDownloadListener* remoteFontHandler
        ) throw()
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP AddListener(
        IDWriteFontDownloadListener* listener,
        _Out_ uint32_t* token
        ) throw() override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP RemoveListener(
        uint32_t token
        ) throw() override
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP_(BOOL) IsQueueEmpty() throw() override
    {
        return enqueuedRequests_.empty();
    }

    IFACEMETHODIMP EnqueueFileFragmentDownload(
        IDWriteFontFileLoader* fontLoader,
        _In_reads_(fileKeySize) void const* fileKey, 
        UINT32 fileKeySize,
        UINT64 fileOffset,
        UINT64 fragmentSize
        ) throw()
    {
        EnqueuedRequest* newRequest = nullptr;
        for (auto& existingRequest : enqueuedRequests_)
        {
            if (existingRequest.fileKey.size() == fileKeySize
            &&  memcmp(existingRequest.fileKey.data(), fileKey, fileKeySize) == 0)
            {
                // Found an existing request with the same file key.
                newRequest = &existingRequest;
            }
        }
        if (newRequest == nullptr)
        {
            enqueuedRequests_.resize(enqueuedRequests_.size() + 1);
            newRequest = &enqueuedRequests_.back();
            uint8_t const* byteKey = reinterpret_cast<uint8_t const*>(fileKey);
            newRequest->fontLoader = fontLoader;
            newRequest->fileKey.assign(byteKey, byteKey + fileKeySize);
        }
        Range range = { uint32_t(fileOffset), uint32_t(fileOffset + fragmentSize) };
        newRequest->ranges.push_back(range);

        return S_OK;
    }

    IFACEMETHODIMP EnqueueFontDownload(
        IDWriteFontFaceReference* fontFaceReference
        ) throw()
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP EnqueueCharactersDownload(
        IDWriteFontFaceReference* fontFaceReference,
        _In_reads_(characterCount) WCHAR const* characters,
        UINT32 characterCount
        ) throw()
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP EnqueueGlyphsDownload(
        IDWriteFontFaceReference* fontFaceReference,
        _In_reads_(glyphCount) UINT16 const* glyphs,
        UINT32 glyphCount
        ) throw()
    {
        return E_NOTIMPL;
    }

    IFACEMETHODIMP BeginDownload(_In_opt_ IUnknown* context) throw() override
    {
        if (enqueuedRequests_.empty())
            return S_FALSE;

        for (auto& request : enqueuedRequests_)
        {
            if (request.fontLoader == nullptr || request.ranges.empty())
                continue;

            // Sort all the range requests into ascending order,
            // coalescing adjacent ranges.

            auto& ranges = request.ranges;
            std::sort(ranges.begin(), ranges.end(), RangeComparer());
            size_t const oldRangesSize = ranges.size();
            size_t currentRangeIndex = 1, newRangesSize = 1;

            for ( ; currentRangeIndex < oldRangesSize; ++currentRangeIndex)
            {
                auto& previousRange = ranges[newRangesSize - 1];
                auto& currentRange = ranges[currentRangeIndex];
                if (currentRange.begin <= previousRange.end)
                {
                    previousRange.end = std::max(currentRange.end, previousRange.end); // Extend range.
                }
                else
                {
                    ranges[newRangesSize++] = currentRange; // Copy over a new range.
                }
            }
            ranges.resize(newRangesSize);

            // If there is an empty range, extend it to the granularity.
            if (ranges.size() == 1 && ranges.front().end == 0)
            {
                ranges.front().end = RemoteFontFileStream::chunkSize_;
            }

            ComPtr<IDWriteRemoteFontFileLoader> remoteFontFileLoader;
            request.fontLoader->QueryInterface(OUT &remoteFontFileLoader);
            if (remoteFontFileLoader != nullptr)
            {
                // remoteFontFileLoader->DownloadStreamInformationFromKey(
                //     request.fileKey.data(),
                //     uint32_t(request.fileKey.size())
                //     );
            }

            ComPtr<IDWriteFontFileStream> fontFileStream;
            ComPtr<IDWriteRemoteFontFileStream> remoteFontFileStream;
            request.fontLoader->CreateStreamFromKey(
                request.fileKey.data(),
                uint32_t(request.fileKey.size()),
                OUT &fontFileStream
                );

            if (fontFileStream != nullptr)
            {
                fontFileStream->QueryInterface(OUT &remoteFontFileStream);
            }
            if (remoteFontFileStream != nullptr)
            {
                for (auto const& range : ranges)
                {
                    // remoteFontFileStream->DownloadFileFragment(range.begin, range.end - range.begin);
                }
            }

            request.fontLoader.Clear();
        }

        CancelDownload();
        return S_OK;
    }

    IFACEMETHODIMP CancelDownload() throw() override
    {
        enqueuedRequests_.clear();
        return E_NOTIMPL;
    }

    void Finalize() override
    {
        //downloadManager_.clear();
    }
};

RemoteFontDownloadManager RemoteFontDownloadManager::singleton_;
