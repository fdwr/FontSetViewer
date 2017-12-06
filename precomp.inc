// include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "sdkddkver.h"

// Disable unknown pragma warnings (for when when compiling directly with VC)
// and not under build.exe.
#pragma warning(disable: 4068)

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER              // Allow use of features specific to Windows 7 or later.
#define WINVER 0x0701       // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows 7 or later.                   
#define _WIN32_WINNT 0x0601 // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0710 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0700    // Change this to the appropriate value to target other versions of IE.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#endif

//////////////////////////////

#ifndef BUILD_OPTIMIZATION_STRING
#if defined(DEBUG)
#define BUILD_OPTIMIZATION_STRING L"Debug"
#else
#define BUILD_OPTIMIZATION_STRING L"Release"
#endif
#endif

#ifndef BUILD_ARCHITECTURE_STRING 
#if defined(_M_IX86)
#define BUILD_ARCHITECTURE_STRING L"x86 32-bit"
#elif defined(_M_X64)
#define BUILD_ARCHITECTURE_STRING L"64-bit"
#else
#define BUILD_ARCHITECTURE_STRING L"Unknown"
#endif
#endif

#ifndef BUILD_TITLE_STRING
#define BUILD_TITLE_STRING L"FontCollectionViewer"
#endif

#ifndef APPLICATION_TITLE
#define APPLICATION_TITLE L"Font Collection Viewer"
#endif


//////////////////////////////

#include "common\Macros.h"

//////////////////////////////
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <xutility>
#include <math.h>
#include <new>
#include <algorithm>
#include <numeric>
#include <stdint.h>
#include <string>
#include <vector>
#include <assert.h>
#include <intrin.h>
#include <functional>
#include <map>
#include <set>


// Work around conflict in VS2010 of IntSafe.h conflicting with stdint.h
#define _INTSAFE_H_INCLUDED_

#ifndef NTDDI_WIN10_RS3
#define NTDDI_WIN10_RS3                     0x0A000003  /* ABRACADABRA_WIN10_RS3 */
#endif
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN10_RS3


//////////////////////////////
// Windows Header Files:

#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <StrSafe.h>

#include <ShlObj.h>
#include <ShlWApi.h>
#include <Knownfolders.h>       // for font folder path
#include <CommDlg.h>
#include <ShellApi.h>

//////////////////////////////
// DirectX headers

#if 1 // Newer version if SDK is older.
#include "sdk_headers/DCommon.h"
#include "sdk_headers/DWrite_3.h"
#else
#include <DWrite_3.h>
#endif

#include <D2d1.h>
#include <D2d1Helper.h>
//#include <WinCodec.h>

//////////////////////////////
// Common headers

#include "common/Common.h"
#include "common/AutoResource.h"
#include "common/Pointers.h"
#include "common/Unicode.h"
#include "common/FileHelpers.h"
#include "Common/TextTreeParser.h"
#include "Common/WindowUtility.h"

//////////////////////////////

// Need this for common controls.
// Otherwise the app either looks ugly,
// or it doesn't show anything at all
// (except an empty window).
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#endif
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
// If __ImageBase does not exist in your linker,
// try GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, &pObjectInModule, &handle) instead.
#endif
