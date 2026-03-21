// linux/compat.h — Windows type compatibility shim for Linux port
// Phase 0: Minimal types needed for Crc32Dynamic
// Phase 1 will expand this with CString, HGLOBAL, CTime, etc.

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

// ---- Primitive type aliases ----
typedef uint32_t DWORD;
typedef uint8_t BYTE;
typedef uint8_t* LPBYTE;
typedef uint16_t WORD;
typedef unsigned long ULONG;
typedef unsigned long long ULONGLONG;
typedef int BOOL;
typedef int INT;
typedef unsigned int UINT;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef intptr_t INT_PTR;
typedef size_t SIZE_T;
typedef long LONG;
typedef long long __int64;
typedef unsigned long long ULONG_PTR;

#define TRUE 1
#define FALSE 0

// ---- Character types (UTF-8 on Linux) ----
typedef char TCHAR;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef unsigned char UCHAR;

// _T() is a no-op on Linux (UTF-8 native)
#define _T(x) x
#define _TEXT(x) x

// ---- Error codes ----
#define NO_ERROR 0
#define ERROR_CRC 23

// ---- Memory flags (unused but referenced) ----
#define GMEM_MOVEABLE 0x0002
#define GMEM_SHARE 0x2000

// ---- Misc macros ----
#ifndef NULL
#define NULL nullptr
#endif

#define ASSERT(x) ((void)0)
#define VERIFY(x) ((void)(x))
#define TRACE(...) ((void)0)

// ---- Max path ----
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
