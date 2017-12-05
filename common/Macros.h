// Include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once


////////////////////////////////////////
// Common macro definitions:

// Disable unknown pragma warnings (for when when compiling directly with VC)
#pragma warning(disable: 4068)

// The C++0x keyword is supported in Visual Studio 2010
#if _MSC_VER < 1600
#ifndef nullptr
#define nullptr 0
#endif
#endif

// The static assertion keyword is supported in Visual Studio 2010
#if _MSC_VER < 1600
#ifndef static_assert
#define static_assert(exp,msg)
#endif
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES       // yes, we DO want PI defined
#endif

#if !defined(DEBUG) && !defined(NDEBUG)
#define DEBUG
#endif

#if (_MSC_VER >= 1400) // ensure that a virtual method overrides an actual base method.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

#ifndef UUIDOF
#define UUIDOF(iid) __uuidof(iid)
#endif

#ifndef ARRAY_SIZE 
#define ARRAY_SIZE ARRAYSIZE
#endif

#ifndef IFR
#define IFR(hrIn) { HRESULT hrOut = (hrIn); if (FAILED(hrOut)) {return hrOut; } }
#endif

#ifndef RETURN_ON_FAIL // For any negative HRESULT code
#define RETURN_ON_FAIL(hrin, retval) { HRESULT hr = (hrin); if (FAILED(hr)) {return (retval); } }
#endif

#ifndef RETURN_ON_ZERO // For null, zero, or false
#define RETURN_ON_ZERO(exp, retval) if (!(exp)) {return (retval);}
#endif

// Use the double macro technique to stringize the actual value of s
#ifndef STRINGIZE_
#define STRINGIZE_(s) STRINGIZE2_(s)
#define STRINGIZE2_(s) #s
#endif

// Weird... windowsx.h lacks this function
#ifndef SetWindowStyle
#define SetWindowStyle(hwnd, value) ((DWORD)SetWindowLong(hwnd, GWL_STYLE, value))
#endif

#ifndef FAILURE_LOCATION 
//-#define FAILURE_LOCATION L"\r\nFunction: " __FUNCTION__ L"\r\nLine: " STRINGIZE_(__LINE__)
#define FAILURE_LOCATION
#endif

#ifndef ALIGNOF
// ALIGNOF macro yields the byte alignment of the specified type.
#ifdef _MSC_VER
// Use Microsoft language extension
#define ALIGNOF(T) __alignof(T)
#else
// Use GCC language extension
#define ALIGNOF(T) __alignof__(T)
#endif
#endif
