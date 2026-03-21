// linux/compat.h — Windows/MFC type compatibility shim for Ditto Linux port
//
// This file provides typedefs, wrappers, and inline functions that let
// Ditto's existing C++ source compile on Linux without MFC/Win32.
// Included via StdAfx.h when LINUX_PORT is defined.

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVector>

// ============================================================================
// Primitive type aliases
// ============================================================================
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

// ============================================================================
// Character types (UTF-8 on Linux — _T() is a no-op)
// ============================================================================
typedef char TCHAR;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef unsigned char UCHAR;

#define _T(x) x
#define _TEXT(x) x

// ============================================================================
// Clipboard format type and standard format constants
// On Windows these are registered dynamically. On Linux we use fixed IDs
// matching the Windows values so the DB schema is compatible.
// ============================================================================
typedef unsigned int CLIPFORMAT;

#define CF_TEXT             1
#define CF_BITMAP           2
#define CF_METAFILEPICT     3
#define CF_SYLK             4
#define CF_DIF              5
#define CF_TIFF             6
#define CF_OEMTEXT          7
#define CF_DIB              8
#define CF_PALETTE          9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12
#define CF_UNICODETEXT      13
#define CF_ENHMETAFILE      14
#define CF_HDROP            15
#define CF_LOCALE           16
#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083
#define CF_DSPENHMETAFILE   0x008E

// RegisterClipboardFormat — on Linux, just hash the name to a uint
inline CLIPFORMAT RegisterClipboardFormat(const char *name) {
    // Simple hash — produces stable IDs for custom format names
    unsigned int hash = 0xC000; // Start above standard format range
    while (*name) {
        hash = hash * 31 + static_cast<unsigned char>(*name++);
    }
    return static_cast<CLIPFORMAT>(hash);
}

// GetClipboardFormatName — on Linux, not available for custom formats
// Ditto only calls this in GetFormatName() fallback for unknown types
inline int GetClipboardFormatName(CLIPFORMAT /*format*/, char *buf, int bufSize) {
    if (bufSize > 0) buf[0] = '\0';
    return 0;
}

// ============================================================================
// Error codes
// ============================================================================
#define NO_ERROR 0
#define ERROR_CRC 23

// ============================================================================
// Memory flags (referenced but not meaningful on Linux)
// ============================================================================
#define GMEM_MOVEABLE 0x0002
#define GMEM_SHARE 0x2000

// ============================================================================
// Misc macros
// ============================================================================
#ifndef NULL
#define NULL nullptr
#endif

#define ASSERT(x) ((void)0)
#define VERIFY(x) ((void)(x))
#define TRACE(...) ((void)0)
#define DEBUG_NEW new

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

// _MSC_VER guard — we're not MSVC
#ifdef _MSC_VER
#undef _MSC_VER
#endif

// ============================================================================
// HGLOBAL — wrapper struct over QByteArray
//
// Ditto pattern: GlobalAlloc → GlobalLock → write → GlobalUnlock → pass handle
// On Linux, HGLOBAL is a pointer to a heap-allocated QByteArray container.
// GlobalLock returns the raw data pointer. GlobalUnlock is a no-op.
// ============================================================================
struct HGlobalData {
    QByteArray data;
};
typedef HGlobalData* HGLOBAL;

inline HGLOBAL GlobalAlloc(unsigned int /*flags*/, size_t size) {
    auto *h = new HGlobalData();
    h->data.resize(static_cast<int>(size));
    h->data.fill(0);
    return h;
}

inline void* GlobalLock(HGLOBAL h) {
    return h ? h->data.data() : nullptr;
}

inline bool GlobalUnlock(HGLOBAL /*h*/) {
    return true; // no-op
}

inline size_t GlobalSize(HGLOBAL h) {
    return h ? static_cast<size_t>(h->data.size()) : 0;
}

inline HGLOBAL GlobalFree(HGLOBAL h) {
    delete h;
    return nullptr;
}

inline void LocalFree(void* /*p*/) {
    // no-op — Linux doesn't use LocalAlloc
}

// Helper from Misc.h — check if HGLOBAL has valid data
inline BOOL IsValid(HGLOBAL h) {
    if (!h) return FALSE;
    void *p = GlobalLock(h);
    GlobalUnlock(h);
    return (p != nullptr && GlobalSize(h) > 0) ? TRUE : FALSE;
}

// NewGlobalP — allocate and copy from a buffer (used in Misc.h)
inline HGLOBAL NewGlobalP(const void* pBuf, size_t nLen) {
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen);
    if (h && pBuf) {
        void *p = GlobalLock(h);
        memcpy(p, pBuf, nLen);
        GlobalUnlock(h);
    }
    return h;
}

inline HGLOBAL NewGlobal(size_t nLen) {
    return GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen);
}

inline HGLOBAL NewGlobalH(HGLOBAL hSource, size_t nLen) {
    if (!hSource) return nullptr;
    void *pSrc = GlobalLock(hSource);
    HGLOBAL h = NewGlobalP(pSrc, nLen);
    GlobalUnlock(hSource);
    return h;
}

// ============================================================================
// CString — thin wrapper around QString with MFC-compatible API
//
// This is the most critical piece. Ditto uses CString extensively:
// Format(), Find(), Left(), Right(), Mid(), Replace(), MakeLower(),
// GetBuffer(), ReleaseBuffer(), IsEmpty(), GetLength(), GetAt(),
// operator+, operator+=, Compare(), CompareNoCase()
//
// On Windows, CString is wide (UTF-16). On Linux, we use UTF-8 via QString.
// Format() uses printf-style strings where %s means char* on Linux.
// ============================================================================
class CString : public QString {
public:
    CString() : QString() {}
    CString(const char *s) : QString(s ? QString::fromUtf8(s) : QString()) {}
    CString(const wchar_t *s) : QString(s ? QString::fromWCharArray(s) : QString()) {}
    CString(const QString &s) : QString(s) {}
    CString(const QByteArray &b) : QString(QString::fromUtf8(b)) {}
    CString(const char *s, int len) : QString(QString::fromUtf8(s, len)) {}
    CString(const wchar_t *s, int len) : QString(QString::fromWCharArray(s, len)) {}

    // Implicit conversion to const char* via cached UTF-8
    operator const char*() const {
        m_utf8Cache = toUtf8();
        return m_utf8Cache.constData();
    }

    // MFC CString API
    int GetLength() const { return static_cast<int>(size()); }
    BOOL IsEmpty() const { return isEmpty() ? TRUE : FALSE; }

    CString Left(int n) const { return CString(left(n)); }
    CString Right(int n) const { return CString(right(n)); }
    CString Mid(int pos, int n = -1) const { return CString(mid(pos, n)); }

    int Find(const char *s, int start = 0) const { return static_cast<int>(indexOf(QString::fromUtf8(s), start)); }
    int Find(char c, int start = 0) const { return static_cast<int>(indexOf(QChar::fromLatin1(c), start)); }
    int ReverseFind(char c) const { return static_cast<int>(lastIndexOf(QChar::fromLatin1(c))); }

    int Replace(const char *oldStr, const char *newStr) {
        QString o = QString::fromUtf8(oldStr);
        QString n = QString::fromUtf8(newStr);
        int count = 0;
        int pos = 0;
        while ((pos = static_cast<int>(indexOf(o, pos))) != -1) {
            QString::replace(pos, o.size(), n);
            pos += static_cast<int>(n.size());
            count++;
        }
        return count;
    }

    void MakeLower() { *this = CString(toLower()); }
    void MakeUpper() { *this = CString(toUpper()); }

    CString& TrimRight() {
        *this = CString(trimmed());
        return *this;
    }

    CString& TrimLeft() {
        *this = CString(trimmed());
        return *this;
    }

    char GetAt(int i) const {
        m_utf8Cache = toUtf8();
        return (i >= 0 && i < m_utf8Cache.size()) ? m_utf8Cache.at(i) : '\0';
    }

    // GetBuffer / ReleaseBuffer — for C-style string manipulation
    char* GetBuffer(int minLen = 0) {
        m_utf8Cache = toUtf8();
        if (minLen > m_utf8Cache.size())
            m_utf8Cache.resize(minLen);
        return m_utf8Cache.data();
    }

    void ReleaseBuffer(int newLen = -1) {
        if (newLen >= 0)
            m_utf8Cache.resize(newLen);
        *this = CString(QString::fromUtf8(m_utf8Cache));
    }

    // Format — printf-style
    void Format(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[4096];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        *this = CString(buf);
    }

    // FormatV — printf-style with va_list (used by execDMLEx etc.)
    void FormatV(const char *fmt, va_list args) {
        char buf[4096];
        vsnprintf(buf, sizeof(buf), fmt, args);
        *this = CString(buf);
    }

    // Comparison
    int Compare(const char *s) const { return QString::compare(QString::fromUtf8(s)); }
    int CompareNoCase(const char *s) const { return QString::compare(QString::fromUtf8(s), Qt::CaseInsensitive); }

    // Operators
    CString& operator+=(const char *s) { append(QString::fromUtf8(s)); return *this; }
    CString& operator+=(const CString &s) { append(s); return *this; }
    CString& operator+=(char c) { append(QChar::fromLatin1(c)); return *this; }

    friend CString operator+(const CString &a, const CString &b) {
        CString r(a);
        r.append(b);
        return r;
    }
    friend CString operator+(const CString &a, const char *b) {
        CString r(a);
        r.append(QString::fromUtf8(b));
        return r;
    }
    friend CString operator+(const char *a, const CString &b) {
        CString r(a);
        r.append(b);
        return r;
    }

private:
    mutable QByteArray m_utf8Cache;
};

// CStringA — narrow string, wraps QByteArray
class CStringA : public QByteArray {
public:
    CStringA() : QByteArray() {}
    CStringA(const char *s) : QByteArray(s ? s : "") {}
    CStringA(const char *s, int len) : QByteArray(s, len) {}
    CStringA(const QByteArray &b) : QByteArray(b) {}

    operator const char*() const { return constData(); }

    int GetLength() const { return static_cast<int>(size()); }
    BOOL IsEmpty() const { return isEmpty() ? TRUE : FALSE; }

    int Find(const char *s, int start = 0) const { return static_cast<int>(indexOf(s, start)); }
    int Find(char c, int start = 0) const { return static_cast<int>(indexOf(c, start)); }

    CStringA Left(int n) const { return CStringA(left(n)); }
    CStringA Right(int n) const { return CStringA(right(n)); }
    CStringA Mid(int pos, int n = -1) const { return CStringA(mid(pos, n)); }

    int Replace(const char *oldStr, const char *newStr) {
        int count = 0;
        QByteArray o(oldStr), n(newStr);
        int pos = 0;
        while ((pos = static_cast<int>(indexOf(o, pos))) != -1) {
            QByteArray::replace(pos, o.size(), n);
            pos += n.size();
            count++;
        }
        return count;
    }

    void MakeLower() {
        for (int i = 0; i < size(); i++)
            if (data()[i] >= 'A' && data()[i] <= 'Z')
                data()[i] += 32;
    }

    void Format(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buf[4096];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        *this = CStringA(buf);
    }

    char GetAt(int i) const { return (i >= 0 && i < size()) ? at(i) : '\0'; }

    CStringA Trim() const { return CStringA(trimmed()); }

    char* GetBuffer(int minLen = 0) {
        if (minLen > size()) resize(minLen);
        return data();
    }
    void ReleaseBuffer(int newLen = -1) {
        if (newLen >= 0) resize(newLen);
    }

    CStringA& operator+=(const char *s) { append(s); return *this; }
    CStringA& operator+=(const CStringA &s) { append(s); return *this; }
    CStringA& operator+=(char c) { append(c); return *this; }

    friend CStringA operator+(const CStringA &a, const CStringA &b) {
        CStringA r(a);
        r.append(b);
        return r;
    }
    friend CStringA operator+(const CStringA &a, const char *b) {
        CStringA r(a);
        r.append(b);
        return r;
    }
    friend CStringA operator+(const char *a, const CStringA &b) {
        CStringA r(a);
        r.append(b);
        return r;
    }

    friend bool operator==(const CStringA &a, const char *b) { return a == QByteArray(b); }
    friend bool operator!=(const CStringA &a, const char *b) { return a != QByteArray(b); }
};

// CStringW — wide string, alias for CString (UTF-8 on Linux)
typedef CString CStringW;

// ============================================================================
// CTime — wraps QDateTime with MFC-compatible API
// ============================================================================
class CTime {
public:
    CTime() : m_dt(QDateTime::fromSecsSinceEpoch(0)) {}
    CTime(time_t t) : m_dt(QDateTime::fromSecsSinceEpoch(t)) {}
    CTime(const QDateTime &dt) : m_dt(dt) {}
    CTime(int y, int m, int d, int h, int mi, int s)
        : m_dt(QDateTime(QDate(y, m, d), QTime(h, mi, s))) {}

    static CTime GetCurrentTime() { return CTime(QDateTime::currentDateTime()); }

    time_t GetTime() const { return m_dt.toSecsSinceEpoch(); }
    int GetYear() const { return m_dt.date().year(); }
    int GetMonth() const { return m_dt.date().month(); }
    int GetDay() const { return m_dt.date().day(); }
    int GetHour() const { return m_dt.time().hour(); }
    int GetMinute() const { return m_dt.time().minute(); }
    int GetSecond() const { return m_dt.time().second(); }

    CString Format(const char *fmt) const {
        // Convert strftime-style to QDateTime format
        QString qfmt = QString::fromUtf8(fmt);
        qfmt.replace("%Y", "yyyy").replace("%m", "MM").replace("%d", "dd")
             .replace("%H", "hh").replace("%M", "mm").replace("%S", "ss");
        return CString(m_dt.toString(qfmt));
    }

    bool operator==(const CTime &o) const { return m_dt == o.m_dt; }
    bool operator!=(const CTime &o) const { return m_dt != o.m_dt; }
    bool operator<(const CTime &o) const { return m_dt < o.m_dt; }
    bool operator>(const CTime &o) const { return m_dt > o.m_dt; }

private:
    QDateTime m_dt;
};

// ============================================================================
// CArray — template alias to QVector
// ============================================================================
template<typename T, typename TArg = T>
class CArray : public QVector<T> {
public:
    INT_PTR GetCount() const { return static_cast<INT_PTR>(QVector<T>::size()); }
    INT_PTR GetSize() const { return GetCount(); }
    INT_PTR Add(const T &item) { QVector<T>::append(item); return GetCount() - 1; }
    T& ElementAt(int i) { return (*this)[i]; }
    const T& ElementAt(int i) const { return (*this)[i]; }
    void RemoveAt(int i) { QVector<T>::remove(i); }
    void RemoveAll() { QVector<T>::clear(); }
    void SetSize(int n) { QVector<T>::resize(n); }
};

// CList — minimal replacement (used by CClipList)
template<typename T, typename TArg>
class CList : public std::vector<T> {
public:
    INT_PTR GetCount() const { return static_cast<INT_PTR>(std::vector<T>::size()); }
    void AddTail(const T &item) { std::vector<T>::push_back(item); }
    void RemoveAll() { std::vector<T>::clear(); }
};

// ============================================================================
// CStringArray — MFC string array
// ============================================================================
class CStringArray : public QVector<CString> {
public:
    INT_PTR GetCount() const { return static_cast<INT_PTR>(QVector<CString>::size()); }
    INT_PTR Add(const CString &s) { QVector<CString>::append(s); return GetCount() - 1; }
    void RemoveAll() { QVector<CString>::clear(); }
};

// ============================================================================
// StrF — Ditto's printf-style string formatter (used everywhere)
// ============================================================================
inline CString StrF(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    return CString(buf);
}

// ============================================================================
// Log — Ditto's logging function (simplified for Linux)
// ============================================================================
inline void Log(const CString &msg) {
    fprintf(stderr, "[Qitto] %s\n", static_cast<const char*>(msg));
}

// Overloads matching Ditto's Log signatures
inline void Log(const char *msg, bool = false, CString = CString(), long = 0) {
    fprintf(stderr, "[Qitto] %s\n", msg);
}

// ============================================================================
// Unicode macros — no-ops on Linux (UTF-8 native)
// ============================================================================
#define STRLEN(s) strlen(s)
#define STRSTR(a, b) strstr(a, b)
#define STRCMP(a, b) strcmp(a, b)
#define STRTOK(a, b) strtok(a, b)
#define STRCPY(a, b) strcpy(a, b)
#define STRNCPY(a, b, n) strncpy(a, b, n)
// SPRINTF — on Windows this is wsprintf (no size param). On Linux, use snprintf.
// Ditto calls SPRINTF(buf, fmt, ...) without size — we use sizeof(buf) as a reasonable default.
// This requires buf to be a stack array, not a pointer. Works for all Ditto usage.
#define SPRINTF(buf, ...) snprintf(buf, sizeof(buf), __VA_ARGS__)
#define ATOL(a) atol(a)
#define ATOI(a) atoi(a)
#define STRICMP(a, b) strcasecmp(a, b)
#define GETENV getenv

// ============================================================================
// Misc Windows macros that appear in Ditto code
// ============================================================================
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define LOBYTE(w) ((BYTE)(w))
#define HIBYTE(w) ((BYTE)(((WORD)(w) >> 8) & 0xFF))

// OutputDebugString — no-op
#define OutputDebugString(s) ((void)0)
#define OutputDebugStringA(s) ((void)0)

// AfxIsValidString — always true on Linux
#define AfxIsValidString(s) (true)

// Sleep
#include <unistd.h>
inline void Sleep(unsigned int ms) { usleep(ms * 1000); }

// GetTickCount
inline DWORD GetTickCount() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<DWORD>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
