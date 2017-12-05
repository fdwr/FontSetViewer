//+---------------------------------------------------------------------------
//
//  Contents:   Helper for enumerating files.
//
//  History:    2013-10-04   dwayner    Created
//
//----------------------------------------------------------------------------
#pragma once

HRESULT ReadTextFile(const wchar_t* filename, OUT std::wstring& text) throw(); // Read UTF-8 or ASCII
HRESULT WriteTextFile(const wchar_t* filename, const std::wstring& text) throw(); // Write as UTF-8
HRESULT WriteTextFile(const wchar_t* filename, __in_ecount(textLength) const wchar_t* text, uint32_t textLength) throw(); // Write as UTF-8
HRESULT ReadBinaryFile(const wchar_t* filename, OUT std::vector<uint8_t>& fileBytes);
HRESULT WriteBinaryFile(const wchar_t* filename, const std::vector<uint8_t>& fileData);
HRESULT WriteBinaryFile(_In_z_ const wchar_t* filename, _In_reads_bytes_(fileDataSize) const void* fileData, uint32_t fileDataSize);

std::wstring GetActualFileName(array_ref<const wchar_t> fileName);
std::wstring GetFullFileName(array_ref<const wchar_t> fileName);

// Expand the given path and mask into a list of nul-separated filenames.
HRESULT EnumerateMatchingFiles(
    __in_z_opt const wchar_t* fileDirectory,
    __in_z_opt wchar_t const* originalFileMask,
    IN OUT std::wstring& fileNames // Appended onto any existing names. It's safe for this to alias fileDirectory.
    );

uint8_t* NtosCompressData(uint8_t* sourceBuffer, uint32_t sourceBufferSize, DWORD* dwSizeOut);

uint8_t* NtosDecompressData(_In_reads_bytes_(sourceBufferSize) uint8_t* sourceBuffer, uint32_t sourceBufferSize, DWORD* dwSizeOut);

const wchar_t* FindFileNameStart(array_ref<const wchar_t> fileName);

bool FileContainsWildcard(array_ref<const wchar_t> fileName);

#if 0
//+---------------------------------------------------------------------------
//
//  Contents:   Helper for enumerating files.
//
//  History:    2013-10-04   dwayner    Created
//
//----------------------------------------------------------------------------
#pragma once

HRESULT ReadTextFile(const wchar_t* filename, OUT std::wstring& text); // Read UTF-8 or ASCII
HRESULT WriteTextFile(const wchar_t* filename, const std::wstring& text); // Write as UTF-8
HRESULT WriteTextFile(const wchar_t* filename, __in_ecount(textLength) const wchar_t* text, uint32_t textLength); // Write as UTF-8
HRESULT ReadBinaryFile(const wchar_t* filename, OUT std::vector<uint8_t>& fileBytes);
HRESULT WriteBinaryFile(const wchar_t* filename, const std::vector<uint8_t>& fileData);
HRESULT WriteBinaryFile(_In_z_ const wchar_t* filename, _In_reads_bytes_(fileDataSize) const void* fileData, uint32_t fileDataSize);

// Expand the given path and mask into a list of nul-separated filenames.
HRESULT EnumerateMatchingFiles(
    __in_z_opt const wchar_t* fileDirectory,
    __in_z_opt wchar_t const* originalFileMask,
    IN OUT std::wstring& fileNames // Appended onto any existing names. It's safe for this to alias fileDirectory.
    );

uint8_t* NtosCompressData(uint8_t* sourceBuffer, size_t sourceBufferSize, DWORD* dwSizeOut);

uint8_t* NtosDecompressData(_In_reads_bytes_(sourceBufferSize) uint8_t* sourceBuffer, size_t sourceBufferSize, DWORD* dwSizeOut);

const wchar_t* FindFileNameStart(array_ref<const wchar_t> fileName);

bool FileContainsWildcard(array_ref<const wchar_t> fileName);
#endif
