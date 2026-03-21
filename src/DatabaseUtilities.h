// DatabaseUtilites.h: interface for the CDatabaseUtilites class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)
#define AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_

#if !defined(LINUX_PORT) && _MSC_VER > 1000
#pragma once
#endif

#ifndef LINUX_PORT
#include "DittoPopupWindow.h"
#endif
#include "sqlite/CppSQLite3.h"

#define DEFAULT_DB_NAME "Ditto.db"
#define ERROR_OPENING_DATABASE	2

#ifndef LINUX_PORT
BOOL CreateBackup(CString csPath);
CString GetDBName();
CString GetDefaultDBName();
#endif

BOOL OpenDatabase(CString csDB);
BOOL IsDatabaseOpen();

#ifndef LINUX_PORT
BOOL CheckDBExists(CString csDBPath);
#endif

BOOL ValidDB(CString csPath, BOOL bUpgrade=TRUE);
BOOL CreateDB(CString csPath);

BOOL CompactDatabase();
BOOL RepairDatabase();

#ifndef LINUX_PORT
BOOL RemoveOldEntries(bool checkIdleTime);
BOOL DeleteNonUsedClips(bool fromAppWindow);
#endif

BOOL EnsureDirectory(CString csPath);

#ifndef LINUX_PORT
BOOL BackupDB(CString dbPath, CString backupPath);
BOOL RestoreDB(CString backupPath);
#endif

void ReOrderStickyClips(int parentID, CppSQLite3DB &db);

// Linux port: global database instance (replaces theApp.m_db)
#ifdef LINUX_PORT
CppSQLite3DB& GetDittoDB();
#endif

#endif // !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)
