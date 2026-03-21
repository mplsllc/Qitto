
#if !defined(AFX_CP_GUI_H__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_)
#define AFX_CP_GUI_H__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_

#if !defined(LINUX_PORT) && _MSC_VER > 1000
#pragma once
#endif

#ifndef LINUX_PORT
#include "..\Shared/ArrayEx.h"
#endif
#include <vector>

#define VK_MOUSE_CLICK 0x01
#define VK_MOUSE_DOUBLE_CLICK 0x02
#define VK_MOUSE_RIGHT_CLICK 0x03
#define VK_MOUSE_MIDDLE_CLICK 0x04

#define UPDATE_AFTER_PASTE_SELECT_CLIP 0x1
#define UPDATE_CLIP_DESCRIPTION 0x2

#define REMOTE_CLIP_ADD_TO_CLIPBOARD 0x1
#define REMOTE_CLIP_MANUAL_SEND 0x2

#ifndef LINUX_PORT
//Handle foreign keyboards pressing ALT_GR (right alt), this simulates a control press
//http://compgroups.net/comp.os.programmer.win32/alt-gr-key-and-left-ctrl/2840252
#define CONTROL_PRESSED ((::GetKeyState(VK_CONTROL) & 0x8000) && (((GetKeyState(VK_RMENU) < 0) && (GetKeyState(VK_LCONTROL) < 0)) == FALSE))
#endif

#define ONE_MINUTE				60000
#define ONE_HOUR				3600000
#define ONE_DAY					86400000

class CopyReasonEnum
{
public:
	enum CopyReason
	{
		COPY_TO_UNKOWN,
		COPY_TO_GROUP,
		COPY_TO_BUFFER,
		COPY_FROM_TOOLTIP
	};
};

#define INVALID_STICKY	-(2147483647)

#define CATCH_SQLITE_EXCEPTION		\
	catch (CppSQLite3Exception& e)	\
    {								\
		Log(StrF(_T("SQLITE Exception %d - %s"), e.errorCode(), e.errorMessage()));	\
		ASSERT(FALSE);				\
    }								\

#define CATCH_SQLITE_EXCEPTION_AND_RETURN(bRet)		\
	catch (CppSQLite3Exception& e)	\
    {								\
		Log(StrF(_T("SQLITE Exception %d - %s"), e.errorCode(), e.errorMessage()));	\
		ASSERT(FALSE);				\
		return bRet;				\
    }								\

#ifdef LINUX_PORT
#define	FIX_CSTRING_PATH(csPath) \
{ \
	if(csPath.IsEmpty() == FALSE && csPath.GetAt(csPath.GetLength()-1) != '/' ) \
	csPath += "/"; \
}
#else
#define	FIX_PATH(strPath) \
{ \
	if (strPath[strlen(strPath)-1] != '\\' && strPath[strlen(strPath)-1] != '/') \
	strcat(strPath, "\\"); \
}

#define	FIX_CSTRING_PATH(csPath) \
{ \
	if(csPath.IsEmpty() == FALSE && csPath.GetAt(csPath.GetLength()-1) != '\\' && csPath.GetAt(csPath.GetLength()-1) != '/') \
	csPath += "\\"; \
}
#endif

#include "DatabaseUtilities.h"

#ifndef LINUX_PORT
CString GetIPAddress();
CString GetComputerName();
#endif

#ifdef LINUX_PORT
#define FUNC		__FUNCTION__
#define FUNCSIG		__PRETTY_FUNCTION__
#else
#define FUNC		__FUNCTION__
#define FUNCSIG		__FUNCSIG__
#endif

void AppendToFile(const TCHAR* fn, const TCHAR *msg);
#define Log(msg) log(msg, false, __FILE__, __LINE__)
void log(const TCHAR* msg, bool bFromSendRecieve = false, CString csFile = _T(""), long lLine = -1);
CString GetErrorString(int err);

#ifndef LINUX_PORT
double IdleSeconds();
#endif

#define LogSendRecieveInfo(cs) logsendrecieveinfo(cs, __FILE__, __LINE__);
void logsendrecieveinfo(CString cs, CString csFile = _T(""), long lLine = -1);

// Utility Functions
CString StrF(const TCHAR * pszFormat, ...);
BYTE GetEscapeChar( BYTE ch );
CString RemoveEscapes( const TCHAR* str );

#ifndef LINUX_PORT
CString GetWndText( HWND hWnd );
bool IsAppWnd( HWND hWnd );
#endif

// Global Memory Helper Functions
// On Linux, IsValid/NewGlobalP/NewGlobal/NewGlobalH are inlined in compat.h
// CopyToGlobal*/CompareGlobal* are in Misc_linux.cpp
#ifndef LINUX_PORT
BOOL IsValid(HGLOBAL hGlobal);
HGLOBAL NewGlobalP(LPVOID pBuf, SIZE_T nLen);
HGLOBAL NewGlobalH(HGLOBAL hSource, SIZE_T nLen);
HGLOBAL NewGlobal(SIZE_T nLen);
#endif
void CopyToGlobalHP(HGLOBAL hDest, LPVOID pBuf, SIZE_T ulBufLen);
void CopyToGlobalHH(HGLOBAL hDest, HGLOBAL hSource, SIZE_T ulBufLen);
int CompareGlobalHP(HGLOBAL hLeft, LPVOID pBuf, SIZE_T ulBufLen);
int CompareGlobalHH(HGLOBAL hLeft, HGLOBAL hRight, SIZE_T ulBufLen);

#ifndef LINUX_PORT
BOOL EncryptString(CString &csString, UCHAR*& pOutput, int &nLenOutput);
BOOL DecryptString(UCHAR *pData, int nLenIn, UCHAR*& pOutput, int &nLenOutput);
int GetScreenWidth();
int GetScreenHeight();
#endif

std::vector<CLIPFORMAT> GetSystemClipFormats();
CLIPFORMAT GetFormatID(LPCTSTR cbName);
CString GetFormatName(CLIPFORMAT cbType);

#ifndef LINUX_PORT
BOOL PreTranslateGuiDll(MSG *pMsg);
CString GetFilePath(CString csFullPath);
CString GetFileName(CString csFileName);
BOOL EnsureWindowVisible(CRect *pcrRect);
CRect DefaultMonitorRect();
CRect MonitorRectFromRect(CRect rect);
CRect CenterRect(CRect startingRect);
CRect CenterRectFromRect(CRect startingRect, CRect outerRect);
__int64 GetLastWriteTime(const CString &csFile);
#endif

#ifndef LINUX_PORT
//Message to the main window to show icon or not
#define WM_USER 0x0400
#define WM_SHOW_TRAY_ICON		WM_USER + 200
#define WM_SETCONNECT			WM_USER + 201
#define WM_CV_IS_CONNECTED		WM_USER + 202
#define WM_CLOSE_APP			WM_USER + 204
#define WM_REFRESH_VIEW			WM_USER + 205
#define WM_CLIPBOARD_COPIED		WM_USER + 206
#define WM_ADD_TO_DATABASE_FROM_SOCKET		WM_USER + 207
#define WM_SEND_RECIEVE_ERROR	WM_USER + 208
#define WM_FOCUS_CHANGED		WM_USER + 209
#define WM_CV_GETCONNECT		WM_USER + 211
#define WM_EDIT_WND_CLOSING		WM_USER	+ 212
#define WM_SET_CONNECTED		WM_USER	+ 213
#define WM_LOAD_ClIP_ON_CLIPBOARD		WM_USER	+ 214
#define WM_RELOAD_CLIP_IN_UI	WM_USER	+ 217
#define WM_GLOBAL_CLIPS_CLOSED	WM_USER	+ 218
#define WM_OPTIONS_CLOSED	WM_USER	+ 219
#define WM_SHOW_OPTIONS	WM_USER	+ 220
#define WM_DELETE_CLIPS_CLOSED	WM_USER	+ 221
#define WM_OPEN_CLOSE_WINDOW WM_USER + 222
#define WM_SAVE_CLIPBOARD WM_USER + 223
#define WM_READD_TASKBAR_ICON WM_USER + 224
#define WM_REOPEN_DATABASE WM_USER + 225
#define WM_SHOW_MSG_WINDOW WM_USER + 226
#define WM_SHOW_DITTO_GROUP WM_USER + 227
#define WM_PLAIN_TEXT_PASTE WM_USER + 228
#define WM_SHOW_ERROR_MSG	WM_USER + 229
#define WM_RESTORE_DB	WM_USER + 230
#define WM_BACKUP_DB	WM_USER + 231
#define WM_REFRESH_FOOTER WM_USER + 232
#define WM_PASTE_CLIP WM_USER + 233
#define WM_EDIT_CLIP WM_USER + 234
#endif

#if !defined(_BITSET_)
#	include <bitset>
#endif

#ifndef LINUX_PORT
long NewGroupID(int parentID = 0, CString text = "");
BOOL DeleteAllIDs();
BOOL DeleteFormats(int parentID, ARRAY& formatIDs);

__inline BOOL FileExists(LPCTSTR pszFile)
{
	return (GetFileAttributes(pszFile) != 0xffffffff);
}

bool IsRunningLimited();
BOOL IsVista();

void DeleteDittoTempFiles(BOOL checkFileLastAccess);
void DeleteFolderFiles(CString csDir, BOOL checkFileLastAccess, CTimeSpan lastAccessOffset);

__int64 FileSize(const TCHAR *fileName);

int FindNoCaseAndInsert(CString& mainStr, CString& findStr, CString preInsert, CString postInsert, int linesPerRow);

void OnInitMenuPopupEx(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu, CWnd *pWnd);

CString InternetEncode(CString text);

CString GetProcessName(HWND hWnd, DWORD processId = 0);
#endif

void DeleteParamFromRTF(CStringA &test, CStringA find, bool searchForTrailingDigits);
bool RemoveRTFSection(CStringA &str, CStringA section);

#ifndef LINUX_PORT
CString NewGuidString();
CString FolderPath(int folderId);
CString TopLevelWindowText(DWORD pid);

BOOL DarkAppWindows10Setting();
DWORD Windows10AccentColor();
BOOL Windows10ColorTitleBar();

BOOL BackupDbPrompt(HWND hwnd);
BOOL RestoreDbPrompt(HWND hwnd);
#endif

int WordCount(const CString& text);

class VersionInfo
{
public:
	int Major;
	int Minor;
	int Revision;
	int Build;
};

#ifndef LINUX_PORT
VersionInfo GetRunningVersion();
CString GetVersionString(VersionInfo version);
#endif

#endif // !defined(AFX_CP_GUI_H__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_)
