// DatabaseUtilities_linux.cpp — Linux port of Ditto's database management functions
// Ported from src/DatabaseUtilities.cpp — preserves the exact same schema DDL
// so databases are compatible between Windows Ditto and Linux Qitto.

#include "StdAfx.h"
#include "DatabaseUtilities.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// ============================================================================
// Global database instance (replaces theApp.m_db on Windows)
// ============================================================================
static CppSQLite3DB g_db;

CppSQLite3DB& GetDittoDB()
{
    return g_db;
}

BOOL IsDatabaseOpen()
{
    return g_db.IsDatabaseOpen();
}

BOOL OpenDatabase(CString csDB)
{
    try
    {
        g_db.close();
        g_db.open(csDB);
        g_db.setBusyTimeout(5000); // 5s default timeout

        return TRUE;
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)
}

// ============================================================================
// ReOrderStickyClips — pure SQL, ported verbatim
// ============================================================================
void ReOrderStickyClips(int parentID, CppSQLite3DB& db)
{
    try
    {
        Log(StrF("Start of ReOrderStickyClips, ParentId %d", parentID));

        if (parentID == -1)
        {
            db.execDML("Update Main Set stickyClipOrder = -(2147483647) where bIsGroup = 1 AND stickyClipOrder = 0");
            db.execDML("Update Main Set stickyClipGroupOrder = -(2147483647) where bIsGroup = 1 AND stickyClipGroupOrder = 0");
        }

        CppSQLite3Query qGroup = db.execQueryEx("SELECT lID, mText FROM Main WHERE bIsGroup = 1 AND lParentID = %d", parentID);

        if (qGroup.eof() == false)
        {
            while (!qGroup.eof())
            {
                CString sql;
                if (parentID > -1)
                    sql = StrF("SELECT lID FROM Main WHERE stickyClipGroupOrder <> -(2147483647) AND lParentID = %d ORDER BY stickyClipGroupOrder DESC", parentID);
                else
                    sql = StrF("SELECT lID FROM Main WHERE stickyClipOrder <> -(2147483647) AND lParentID = %d ORDER BY stickyClipOrder DESC", parentID);

                CppSQLite3Query qSticky = db.execQueryEx(sql);

                int order = 1;

                if (qSticky.eof() == false)
                {
                    while (!qSticky.eof())
                    {
                        if (parentID > -1)
                            db.execDMLEx("Update Main Set stickyClipGroupOrder = %d where lID = %d", order, qSticky.getIntField("lID"));
                        else
                            db.execDMLEx("Update Main Set stickyClipOrder = %d where lID = %d", order, qSticky.getIntField("lID"));

                        qSticky.nextRow();
                        order--;
                    }
                }

                ReOrderStickyClips(qGroup.getIntField("lID"), db);

                qGroup.nextRow();
            }
        }

        Log(StrF("End of ReOrderStickyClips, ParentId %d", parentID));
    }
    CATCH_SQLITE_EXCEPTION
}

// ============================================================================
// ValidDB — schema migration, ported verbatim from DatabaseUtilities.cpp:287-486
// Upgrades old databases by adding missing columns/indexes/triggers.
// ============================================================================
BOOL ValidDB(CString csPath, BOOL bUpgrade)
{
    try
    {
        CppSQLite3DB db;
        db.open(csPath);

        // Verify core tables exist and are readable
        db.execQuery("SELECT lID, lDate, mText, lShortCut, lDontAutoDelete, "
                     "CRC, bIsGroup, lParentID, QuickPasteText FROM Main");
        db.execQuery("SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data");
        db.execQuery("SELECT lID, TypeText FROM Types");

        // Drop and recreate the main delete trigger
        try { db.execDML("DROP TRIGGER delete_data_trigger"); }
        catch (CppSQLite3Exception&) {}

        try { db.execDML("DROP TRIGGER delete_copy_buffer_trigger"); }
        catch (CppSQLite3Exception&) {}

        try {
            db.execDML("CREATE TRIGGER delete_data_trigger BEFORE DELETE ON Main FOR EACH ROW\n"
                       "BEGIN\n"
                       "INSERT INTO MainDeletes VALUES(old.lID, datetime('now'));\n"
                       "END\n");
        }
        catch (CppSQLite3Exception&) {}

        // Ensure CopyBuffers table exists
        try { db.execQuery("SELECT lID, lClipID, lCopyBuffer FROM CopyBuffers"); }
        catch (CppSQLite3Exception&) {
            db.execDML("CREATE TABLE CopyBuffers("
                       "lID INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "lClipID INTEGER,"
                       "lCopyBuffer INTEGER)");
        }

        // Ensure MainDeletes table exists
        try { db.execQuery("SELECT clipId FROM MainDeletes"); }
        catch (CppSQLite3Exception&) {
            db.execDML("CREATE TABLE MainDeletes("
                       "clipID INTEGER,"
                       "modifiedDate)");

            db.execDML("CREATE TRIGGER MainDeletes_delete_data_trigger BEFORE DELETE ON MainDeletes FOR EACH ROW\n"
                       "BEGIN\n"
                       "DELETE FROM CopyBuffers WHERE lClipID = old.clipID;\n"
                       "DELETE FROM Data WHERE lParentID = old.clipID;\n"
                       "END\n");
        }

        // Ensure indexes
        try {
            db.execDML("CREATE INDEX Main_ParentId on Main(lParentID DESC)");
            db.execDML("CREATE INDEX Main_IsGroup on Main(bIsGroup DESC)");
            db.execDML("CREATE INDEX Main_ShortCut on Main(lShortCut DESC)");
        }
        catch (CppSQLite3Exception&) {}

        // Ensure clipOrder columns
        try { db.execQuery("SELECT clipOrder, clipGroupOrder FROM Main"); }
        catch (CppSQLite3Exception&) {
            db.execDML("ALTER TABLE Main ADD clipOrder REAL");
            db.execDML("ALTER TABLE Main ADD clipGroupOrder REAL");
            db.execDML("Update Main set clipOrder = lDate, clipGroupOrder = lDate");
            db.execDML("CREATE INDEX Main_ClipOrder on Main(clipOrder DESC)");
            db.execDML("CREATE INDEX Main_ClipGroupOrder on Main(clipGroupOrder DESC)");
            try { db.execDML("DROP INDEX Main_Date"); } catch (CppSQLite3Exception&) {}
        }

        // Ensure globalShortCut column
        try { db.execQuery("SELECT globalShortCut FROM Main"); }
        catch (CppSQLite3Exception&) {
            db.execDML("ALTER TABLE Main ADD globalShortCut INTEGER");
        }

        // Ensure lastPasteDate column
        try { db.execQuery("SELECT lastPasteDate FROM Main"); }
        catch (CppSQLite3Exception&) {
            db.execDML("ALTER TABLE Main ADD lastPasteDate INTEGER");
            db.execDML("Update Main set lastPasteDate = lDate");
            db.execDMLEx("Update Main set lastPasteDate = %d where lastPasteDate <= 0",
                         (int)CTime::GetCurrentTime().GetTime());
        }

        // Ensure sticky columns
        try { db.execQuery("SELECT stickyClipOrder FROM Main"); }
        catch (CppSQLite3Exception&) {
            db.execDML("ALTER TABLE Main ADD stickyClipOrder REAL");
            db.execDML("ALTER TABLE Main ADD stickyClipGroupOrder REAL");
        }

        // Ensure sticky indexes and fix NULL values
        try {
            CppSQLite3Query q = db.execQuery("PRAGMA index_info(Main_NoGroup);");
            int count = 0;
            while (q.eof() == false) { count++; q.nextRow(); }

            if (count == 0) {
                db.execDML("Update Main set stickyClipOrder = -(2147483647) where stickyClipOrder IS NULL;");
                db.execDML("Update Main set stickyClipGroupOrder = -(2147483647) where stickyClipGroupOrder IS NULL;");
                db.execDML("Update Main set stickyClipOrder = -(2147483647) where stickyClipOrder = 0;");
                db.execDML("Update Main set stickyClipGroupOrder = -(2147483647) where stickyClipGroupOrder = 0;");

                db.execDML("CREATE INDEX Main_NoGroup ON Main(bIsGroup ASC, stickyClipOrder DESC, clipOrder DESC);");
                db.execDML("CREATE INDEX Main_InGroup ON Main(lParentId ASC, bIsGroup ASC, stickyClipGroupOrder DESC, clipGroupOrder DESC);");
                db.execDML("CREATE INDEX Data_ParentId_Format ON Data(lParentID COLLATE BINARY ASC, strClipBoardFormat COLLATE NOCASE ASC);");
            }
        }
        catch (CppSQLite3Exception&) {}

        // Ensure MoveToGroup columns
        try {
            db.execQuery("SELECT MoveToGroupShortCut FROM Main");
            db.execQuery("SELECT GlobalMoveToGroupShortCut FROM Main");
        }
        catch (CppSQLite3Exception&) {
            db.execDML("ALTER TABLE Main ADD MoveToGroupShortCut INTEGER");
            db.execDML("ALTER TABLE Main ADD GlobalMoveToGroupShortCut INTEGER");
        }

        // Drop old indexes, create new ones
        db.execDML("DROP INDEX IF EXISTS Main_NoGroup");
        db.execDML("DROP INDEX IF EXISTS Main_InGroup");
        db.execDML("DROP INDEX IF EXISTS Main_ShortCut");

        db.execDML("CREATE INDEX IF NOT EXISTS Main_TopLevelParentID ON Main(lParentId ASC, stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_TopLevel ON Main(stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_InGroup2 ON Main(lParentId ASC, stickyClipGroupOrder DESC, bIsGroup ASC, clipGroupOrder DESC);");

        db.execDML("CREATE INDEX IF NOT EXISTS Main_ShortCut2 on Main(lShortCut DESC, globalShortCut DESC)");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_MoveToGroup on Main(MoveToGroupShortCut DESC, GlobalMoveToGroupShortCut DESC)");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_CRC on Main(CRC ASC)");
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

    return TRUE;
}

// ============================================================================
// CreateDB — schema DDL, ported verbatim from DatabaseUtilities.cpp:693-773
// This creates the exact same schema as Windows Ditto.
// ============================================================================
BOOL CreateDB(CString csFile)
{
    try
    {
        CppSQLite3DB db;
        db.open(csFile);

        db.execDML("PRAGMA auto_vacuum = 1");

        db.execDML("CREATE TABLE Main("
                   "lID INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "lDate INTEGER, "
                   "mText TEXT, "
                   "lShortCut INTEGER, "
                   "lDontAutoDelete INTEGER, "
                   "CRC INTEGER, "
                   "bIsGroup INTEGER, "
                   "lParentID INTEGER, "
                   "QuickPasteText TEXT, "
                   "clipOrder REAL, "
                   "clipGroupOrder REAL, "
                   "globalShortCut INTEGER, "
                   "lastPasteDate INTEGER, "
                   "stickyClipOrder REAL, "
                   "stickyClipGroupOrder REAL, "
                   "MoveToGroupShortCut INTEGER, "
                   "GlobalMoveToGroupShortCut INTEGER);");

        db.execDML("CREATE TABLE Data("
                   "lID INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "lParentID INTEGER, "
                   "strClipBoardFormat TEXT, "
                   "ooData BLOB);");

        db.execDML("CREATE TABLE Types("
                   "lID INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "TypeText TEXT);");

        db.execDML("CREATE UNIQUE INDEX Main_ID on Main(lID ASC)");
        db.execDML("CREATE UNIQUE INDEX Data_ID on Data(lID ASC)");
        db.execDML("CREATE INDEX Main_ClipOrder on Main(clipOrder DESC)");
        db.execDML("CREATE INDEX Main_ClipGroupOrder on Main(clipGroupOrder DESC)");
        db.execDML("CREATE INDEX Main_ParentId on Main(lParentID DESC)");
        db.execDML("CREATE INDEX Main_IsGroup on Main(bIsGroup DESC)");

        db.execDML("CREATE TRIGGER delete_data_trigger BEFORE DELETE ON Main FOR EACH ROW\n"
                   "BEGIN\n"
                   "INSERT INTO MainDeletes VALUES(old.lID, datetime('now'));\n"
                   "END\n");

        db.execDML("CREATE TABLE CopyBuffers("
                   "lID INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "lClipID INTEGER, "
                   "lCopyBuffer INTEGER)");

        db.execDML("CREATE TABLE MainDeletes("
                   "clipID INTEGER,"
                   "modifiedDate)");

        db.execDML("CREATE TRIGGER MainDeletes_delete_data_trigger BEFORE DELETE ON MainDeletes FOR EACH ROW\n"
                   "BEGIN\n"
                   "DELETE FROM CopyBuffers WHERE lClipID = old.clipID;\n"
                   "DELETE FROM Data WHERE lParentID = old.clipID;\n"
                   "END\n");

        db.execDML("CREATE INDEX Data_ParentId_Format ON Data(lParentID COLLATE BINARY ASC, strClipBoardFormat COLLATE NOCASE ASC);");

        db.execDML("CREATE INDEX IF NOT EXISTS Main_TopLevelParentID ON Main(lParentId ASC, stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_TopLevel ON Main(stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_InGroup2 ON Main(lParentId ASC, stickyClipGroupOrder DESC, bIsGroup ASC, clipGroupOrder DESC);");

        db.execDML("CREATE INDEX IF NOT EXISTS Main_ShortCut2 on Main(lShortCut DESC, globalShortCut DESC)");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_MoveToGroup on Main(MoveToGroupShortCut DESC, GlobalMoveToGroupShortCut DESC)");
        db.execDML("CREATE INDEX IF NOT EXISTS Main_CRC on Main(CRC ASC)");

        db.close();
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

    return TRUE;
}

// ============================================================================
// CompactDatabase / RepairDatabase — stubs (original was already commented out)
// ============================================================================
BOOL CompactDatabase()
{
    try
    {
        if (IsDatabaseOpen())
            g_db.execDML("VACUUM");
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

    return TRUE;
}

BOOL RepairDatabase()
{
    try
    {
        if (IsDatabaseOpen())
            g_db.execDML("PRAGMA integrity_check");
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

    return TRUE;
}

// ============================================================================
// EnsureDirectory — create directory if it doesn't exist
// ============================================================================
BOOL EnsureDirectory(CString csPath)
{
    QDir dir(static_cast<const QString&>(csPath));
    if (!dir.exists())
        return dir.mkpath(".") ? TRUE : FALSE;
    return TRUE;
}
