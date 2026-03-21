// Misc_linux.cpp — Linux implementations of Ditto's Misc utility functions
// IsValid, NewGlobalP, NewGlobal, NewGlobalH are inlines in compat.h.
// StrF, log, CopyToGlobal*, CompareGlobal*, GetFormatName/ID are here.

#include "StdAfx.h"
#include "Misc.h"
#include "sqlite/CppSQLite3.h"

#include <cstring>

// ============================================================================
// StrF — printf-style string formatter (used everywhere in Ditto)
// ============================================================================
CString StrF(const TCHAR * pszFormat, ...)
{
    CString str;
    va_list argList;
    va_start(argList, pszFormat);
    str.FormatV(pszFormat, argList);
    va_end(argList);
    return str;
}

// ============================================================================
// Logging helpers (log() with file/line signature used by Log macro)
// The Log macro in Misc.h calls log(msg, false, __FILE__, __LINE__)
// compat.h provides the simple Log(CString) overload.
// This provides the full-signature version for the macro.
// ============================================================================

void log(const TCHAR* msg, bool bFromSendRecieve, CString csFile, long lLine)
{
    (void)bFromSendRecieve;

    // Extract filename from path
    int slash = csFile.ReverseFind('/');
    if (slash >= 0)
        csFile = csFile.Right(csFile.GetLength() - slash - 1);

    fprintf(stderr, "[Qitto %s:%ld] %s\n", (const char*)csFile, lLine, msg);
}

void logsendrecieveinfo(CString cs, CString csFile, long lLine)
{
    log(cs, true, csFile, lLine);
}

void AppendToFile(const TCHAR* fn, const TCHAR *msg)
{
    FILE *f = fopen(fn, "a");
    if (f) {
        fprintf(f, "%s", msg);
        fclose(f);
    }
}

CString GetErrorString(int err)
{
    return StrF("Error %d: %s", err, strerror(err));
}

// ============================================================================
// Escape character handling
// ============================================================================

BYTE GetEscapeChar(BYTE ch)
{
    switch(ch)
    {
    case '\'':  return '\'';
    case '\"':  return '\"';
    case '\\':  return '\\';
    case 'a':   return '\a';
    case 'b':   return '\b';
    case 'f':   return '\f';
    case 'n':   return '\n';
    case 'r':   return '\r';
    case 't':   return '\t';
    case 'v':   return '\v';
    default:    return ch;
    }
}

CString RemoveEscapes(const TCHAR* str)
{
    CString ret;
    if (!str) return ret;

    int len = (int)strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == '\\' && i + 1 < len) {
            i++;
            char buf[2] = { (char)GetEscapeChar((BYTE)str[i]), 0 };
            ret += buf;
        } else {
            char buf[2] = { str[i], 0 };
            ret += buf;
        }
    }
    return ret;
}

// ============================================================================
// Global Memory Helpers — CopyToGlobal*, CompareGlobal*
// These call GlobalLock/Unlock from compat.h
// ============================================================================

void CopyToGlobalHP(HGLOBAL hDest, LPVOID pBuf, SIZE_T ulBufLen)
{
    LPVOID pvData = GlobalLock(hDest);
    if (pvData && pBuf)
        memcpy(pvData, pBuf, ulBufLen);
    GlobalUnlock(hDest);
}

void CopyToGlobalHH(HGLOBAL hDest, HGLOBAL hSource, SIZE_T ulBufLen)
{
    LPVOID pvData = GlobalLock(hSource);
    if (pvData)
        CopyToGlobalHP(hDest, pvData, ulBufLen);
    GlobalUnlock(hSource);
}

int CompareGlobalHP(HGLOBAL hLeft, LPVOID pBuf, SIZE_T ulBufLen)
{
    LPVOID pvData = GlobalLock(hLeft);
    int result = 0;
    if (pvData && pBuf)
        result = memcmp(pvData, pBuf, ulBufLen);
    GlobalUnlock(hLeft);
    return result;
}

int CompareGlobalHH(HGLOBAL hLeft, HGLOBAL hRight, SIZE_T ulBufLen)
{
    LPVOID pvData = GlobalLock(hRight);
    int result = 0;
    if (pvData)
        result = CompareGlobalHP(hLeft, pvData, ulBufLen);
    GlobalUnlock(hRight);
    return result;
}

// ============================================================================
// Clipboard format name ↔ ID mapping
// Must match Ditto's DB format strings exactly for schema compatibility.
// ============================================================================

CLIPFORMAT GetFormatID(LPCTSTR cbName)
{
    if(STRCMP(cbName, "CF_TEXT") == 0)           return CF_TEXT;
    if(STRCMP(cbName, "CF_METAFILEPICT") == 0)   return CF_METAFILEPICT;
    if(STRCMP(cbName, "CF_SYLK") == 0)           return CF_SYLK;
    if(STRCMP(cbName, "CF_DIF") == 0)            return CF_DIF;
    if(STRCMP(cbName, "CF_TIFF") == 0)           return CF_TIFF;
    if(STRCMP(cbName, "CF_OEMTEXT") == 0)        return CF_OEMTEXT;
    if(STRCMP(cbName, "CF_DIB") == 0)            return CF_DIB;
    if(STRCMP(cbName, "CF_PALETTE") == 0)        return CF_PALETTE;
    if(STRCMP(cbName, "CF_PENDATA") == 0)        return CF_PENDATA;
    if(STRCMP(cbName, "CF_RIFF") == 0)           return CF_RIFF;
    if(STRCMP(cbName, "CF_WAVE") == 0)           return CF_WAVE;
    if(STRCMP(cbName, "CF_UNICODETEXT") == 0)    return CF_UNICODETEXT;
    if(STRCMP(cbName, "CF_ENHMETAFILE") == 0)    return CF_ENHMETAFILE;
    if(STRCMP(cbName, "CF_HDROP") == 0)          return CF_HDROP;
    if(STRCMP(cbName, "CF_LOCALE") == 0)         return CF_LOCALE;
    if(STRCMP(cbName, "CF_OWNERDISPLAY") == 0)   return CF_OWNERDISPLAY;
    if(STRCMP(cbName, "CF_DSPTEXT") == 0)        return CF_DSPTEXT;
    if(STRCMP(cbName, "CF_DSPBITMAP") == 0)       return CF_DSPBITMAP;
    if(STRCMP(cbName, "CF_DSPMETAFILEPICT") == 0) return CF_DSPMETAFILEPICT;
    if(STRCMP(cbName, "CF_DSPENHMETAFILE") == 0)  return CF_DSPENHMETAFILE;

    return RegisterClipboardFormat(cbName);
}

CString GetFormatName(CLIPFORMAT cbType)
{
    switch(cbType)
    {
    case CF_TEXT:           return "CF_TEXT";
    case CF_BITMAP:         return "CF_BITMAP";
    case CF_METAFILEPICT:   return "CF_METAFILEPICT";
    case CF_SYLK:           return "CF_SYLK";
    case CF_DIF:            return "CF_DIF";
    case CF_TIFF:           return "CF_TIFF";
    case CF_OEMTEXT:        return "CF_OEMTEXT";
    case CF_DIB:            return "CF_DIB";
    case CF_PALETTE:        return "CF_PALETTE";
    case CF_PENDATA:        return "CF_PENDATA";
    case CF_RIFF:           return "CF_RIFF";
    case CF_WAVE:           return "CF_WAVE";
    case CF_UNICODETEXT:    return "CF_UNICODETEXT";
    case CF_ENHMETAFILE:    return "CF_ENHMETAFILE";
    case CF_HDROP:          return "CF_HDROP";
    case CF_LOCALE:         return "CF_LOCALE";
    case CF_OWNERDISPLAY:   return "CF_OWNERDISPLAY";
    case CF_DSPTEXT:        return "CF_DSPTEXT";
    case CF_DSPBITMAP:      return "CF_DSPBITMAP";
    case CF_DSPMETAFILEPICT: return "CF_DSPMETAFILEPICT";
    case CF_DSPENHMETAFILE: return "CF_DSPENHMETAFILE";
    default:
        {
            TCHAR szFormat[256] = {0};
            GetClipboardFormatName(cbType, szFormat, 256);
            if (szFormat[0])
                return szFormat;
        }
        break;
    }

    return "UNKNOWN";
}

std::vector<CLIPFORMAT> GetSystemClipFormats()
{
    return {
        CF_TEXT, CF_UNICODETEXT, CF_DIB, CF_HDROP,
        CF_OEMTEXT, CF_LOCALE
    };
}

// ============================================================================
// RTF helpers (used by format aggregators)
// ============================================================================

void DeleteParamFromRTF(CStringA &text, CStringA find, bool searchForTrailingDigits)
{
    int pos = text.Find(find);
    while (pos >= 0) {
        int end = pos + find.GetLength();
        if (searchForTrailingDigits) {
            while (end < text.GetLength() &&
                   ((text.GetAt(end) >= '0' && text.GetAt(end) <= '9') || text.GetAt(end) == '-'))
                end++;
        }
        if (end < text.GetLength() && text.GetAt(end) == ' ')
            end++;

        text = CStringA(text.Left(pos) + text.Mid(end));
        pos = text.Find(find, pos);
    }
}

bool RemoveRTFSection(CStringA &str, CStringA section)
{
    int pos = str.Find(section);
    if (pos < 0)
        return false;

    int start = pos;
    for (int i = pos; i >= 0; i--) {
        if (str.GetAt(i) == '{') {
            start = i;
            break;
        }
    }

    int depth = 0;
    for (int i = start; i < str.GetLength(); i++) {
        if (str.GetAt(i) == '{') depth++;
        else if (str.GetAt(i) == '}') {
            depth--;
            if (depth == 0) {
                str = CStringA(str.Left(start) + str.Mid(i + 1));
                return true;
            }
        }
    }
    return false;
}

int WordCount(const CString& text)
{
    int count = 0;
    bool inWord = false;
    QByteArray utf8 = static_cast<const QString&>(text).toUtf8();
    for (int i = 0; i < utf8.size(); i++) {
        char c = utf8.at(i);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            inWord = false;
        } else if (!inWord) {
            inWord = true;
            count++;
        }
    }
    return count;
}
