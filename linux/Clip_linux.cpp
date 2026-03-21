// Clip_linux.cpp — Linux port of Ditto's Clip CRUD methods
// Ported from src/Clip.cpp — all SQL logic preserved verbatim.
// Uses GetDittoDB() instead of theApp.m_db.

#include "StdAfx.h"
#include "Clip.h"
#include "DatabaseUtilities.h"
#include "Crc32Dynamic.h"
#include "sqlite/CppSQLite3.h"
#include "Misc.h"

// ============================================================================
// Static members
// ============================================================================
DWORD CClip::m_LastAddedCRC = 0;
int CClip::m_lastAddedID = -1;

// ============================================================================
// CClipFormat
// ============================================================================

CClipFormat::CClipFormat(CLIPFORMAT cfType, HGLOBAL hgData, int parentId)
    : m_cfType(cfType), m_hgData(hgData), m_autoDeleteData(true), m_dataId(-1), m_parentId(parentId)
{
}

// Copy constructor — shallow copy, NEVER auto-deletes.
// HGLOBAL ownership is managed explicitly by CClip::EmptyFormats / RemoveAll,
// not by CClipFormat destructors. This prevents double-free when QVector
// copies elements during reallocation.
CClipFormat::CClipFormat(const CClipFormat &other)
    : m_cfType(other.m_cfType), m_hgData(other.m_hgData),
      m_autoDeleteData(false), m_dataId(other.m_dataId), m_parentId(other.m_parentId)
{
}

CClipFormat& CClipFormat::operator=(const CClipFormat &other)
{
    if (this != &other) {
        m_cfType = other.m_cfType;
        m_hgData = other.m_hgData;
        m_autoDeleteData = false;
        m_dataId = other.m_dataId;
        m_parentId = other.m_parentId;
    }
    return *this;
}

CClipFormat::~CClipFormat()
{
    // On Linux, never auto-delete from destructor.
    // HGLOBAL lifetime is managed by CClip::EmptyFormats.
}

void CClipFormat::Clear()
{
    m_cfType = 0;
    Free();
    m_autoDeleteData = true;
    m_dataId = -1;
    m_parentId = -1;
}

void CClipFormat::Free()
{
    if (m_autoDeleteData && m_hgData)
    {
        GlobalFree(m_hgData);
    }
    m_hgData = 0;
}

// ============================================================================
// CClipFormats
// ============================================================================

CClipFormat* CClipFormats::FindFormat(UINT cfType)
{
    INT_PTR count = GetCount();
    for (int i = 0; i < count; i++)
    {
        if (ElementAt(i).m_cfType == cfType)
            return &ElementAt(i);
    }
    return NULL;
}

bool CClipFormats::RemoveFormat(CLIPFORMAT type)
{
    INT_PTR count = GetCount();
    for (int i = 0; i < count; i++)
    {
        if (ElementAt(i).m_cfType == type)
        {
            RemoveAt(i);
            return true;
        }
    }
    return false;
}

// ============================================================================
// CClip — Constructor / Destructor / Assignment
// ============================================================================

CClip::CClip() :
    m_id(-1),
    m_lTotalCopySize(0),
    m_parentId(-1),
    m_dontAutoDelete(0),
    m_shortCut(0),
    m_bIsGroup(FALSE),
    m_CRC(0),
    m_param1(0),
    m_clipOrder(0),
    m_clipGroupOrder(0),
    m_stickyClipOrder(INVALID_STICKY),
    m_stickyClipGroupOrder(INVALID_STICKY),
    m_globalShortCut(FALSE),
    m_moveToGroupShortCut(0),
    m_globalMoveToGroupShortCut(FALSE),
    m_copyReason(CopyReasonEnum::COPY_TO_UNKOWN),
    m_addToDbStickyEnum(AddToDbStickyEnum::NONE)
{
}

CClip::~CClip()
{
    EmptyFormats();
}

void CClip::Clear()
{
    m_id = -1;
    m_Desc = "";
    m_CRC = 0;
    m_parentId = -1;
    m_lTotalCopySize = 0;
    m_dontAutoDelete = 0;
    m_shortCut = 0;
    m_bIsGroup = FALSE;
    m_csQuickPaste = "";
    m_param1 = 0;
    m_clipOrder = 0;
    m_clipGroupOrder = 0;
    m_stickyClipOrder = INVALID_STICKY;
    m_stickyClipGroupOrder = INVALID_STICKY;
    m_globalShortCut = FALSE;
    m_moveToGroupShortCut = 0;
    m_globalMoveToGroupShortCut = FALSE;
    m_copyReason = CopyReasonEnum::COPY_TO_UNKOWN;
    m_addToDbStickyEnum = AddToDbStickyEnum::NONE;
    EmptyFormats();
}

const CClip& CClip::operator=(const CClip &clip)
{
    m_id = clip.m_id;
    m_Formats.RemoveAll();
    // Shallow copy of formats (don't auto-delete in copy)
    INT_PTR count = clip.m_Formats.GetCount();
    for (int i = 0; i < count; i++)
    {
        CClipFormat cf;
        cf.m_cfType = clip.m_Formats[i].m_cfType;
        cf.m_hgData = clip.m_Formats[i].m_hgData;
        cf.m_autoDeleteData = false;
        cf.m_dataId = clip.m_Formats[i].m_dataId;
        cf.m_parentId = clip.m_Formats[i].m_parentId;
        m_Formats.Add(cf);
    }
    m_Time = clip.m_Time;
    m_Desc = clip.m_Desc;
    m_lTotalCopySize = clip.m_lTotalCopySize;
    m_parentId = clip.m_parentId;
    m_dontAutoDelete = clip.m_dontAutoDelete;
    m_shortCut = clip.m_shortCut;
    m_bIsGroup = clip.m_bIsGroup;
    m_CRC = clip.m_CRC;
    m_csQuickPaste = clip.m_csQuickPaste;
    m_clipOrder = clip.m_clipOrder;
    m_clipGroupOrder = clip.m_clipGroupOrder;
    m_stickyClipOrder = clip.m_stickyClipOrder;
    m_stickyClipGroupOrder = clip.m_stickyClipGroupOrder;
    m_globalShortCut = clip.m_globalShortCut;
    m_lastPasteDate = clip.m_lastPasteDate;
    m_moveToGroupShortCut = clip.m_moveToGroupShortCut;
    m_globalMoveToGroupShortCut = clip.m_globalMoveToGroupShortCut;
    m_copyReason = clip.m_copyReason;
    return *this;
}

void CClip::EmptyFormats()
{
    // Explicitly free all HGLOBALs since CClipFormat destructor doesn't on Linux
    for (INT_PTR i = 0; i < m_Formats.GetSize(); i++) {
        if (m_Formats.ElementAt(i).m_hgData) {
            GlobalFree(m_Formats.ElementAt(i).m_hgData);
            m_Formats.ElementAt(i).m_hgData = 0;
        }
    }
    m_Formats.RemoveAll();
}

bool CClip::AddFormat(CLIPFORMAT cfType, void* pData, UINT nLen, bool setDesc)
{
    HGLOBAL hGlobal = NewGlobalP(pData, nLen);
    if (!hGlobal)
        return false;

    CClipFormat cf(cfType, hGlobal, -1);
    cf.m_autoDeleteData = false; // never auto-delete on Linux
    m_Formats.Add(cf);

    m_lTotalCopySize += nLen;

    if (setDesc)
    {
        if (cfType == CF_UNICODETEXT || cfType == CF_TEXT)
            SetDescFromText(hGlobal, cfType == CF_UNICODETEXT);
    }

    return true;
}

// ============================================================================
// SetDescFromText / SetDescFromType
// ============================================================================

bool CClip::SetDescFromText(HGLOBAL hgData, bool unicode)
{
    if (!hgData) return false;

    LPVOID data = GlobalLock(hgData);
    int size = (int)GlobalSize(hgData);
    if (!data || size <= 0)
    {
        GlobalUnlock(hgData);
        return false;
    }

    if (unicode)
    {
#ifdef LINUX_PORT
        // On Linux, "unicode" text is still UTF-8 in our HGLOBAL
        m_Desc = CString((char*)data, size - 1);
#else
        m_Desc = CString((wchar_t*)data, (size / sizeof(wchar_t)) - 1);
#endif
    }
    else
    {
        m_Desc = CString((char*)data, size - 1);
    }

    GlobalUnlock(hgData);

    // Truncate description to reasonable size
    int bufLen = m_Desc.GetLength();
    if (bufLen > 200)
        m_Desc = m_Desc.Left(200);

    return true;
}

bool CClip::SetDescFromType()
{
    if (m_Formats.GetSize() <= 0)
        return false;

    CClipFormat *pCF = &m_Formats.ElementAt(0);
    m_Desc = GetFormatName(pCF->m_cfType);
    return true;
}

// ============================================================================
// GenerateCRC — ported from Clip.cpp:885
// ============================================================================

DWORD CClip::GenerateCRC()
{
    CClipFormat* pCF;
    DWORD dwCRC = 0xFFFFFFFF;

    CCrc32Dynamic *pCrc32 = new CCrc32Dynamic;
    if (pCrc32)
    {
        INT_PTR size = m_Formats.GetSize();
        for (int i = 0; i < size; i++)
        {
            pCF = &m_Formats.ElementAt(i);

            const unsigned char *Data = (const unsigned char *)GlobalLock(pCF->m_hgData);
            if (Data)
            {
                pCrc32->GenerateCrc32((const LPBYTE)Data, (DWORD)GlobalSize(pCF->m_hgData), dwCRC);
            }
            GlobalUnlock(pCF->m_hgData);
        }

        dwCRC = ~dwCRC;
        delete pCrc32;
    }

    return dwCRC;
}

// ============================================================================
// FindDuplicate — ported from Clip.cpp:854
// ============================================================================

int CClip::FindDuplicate()
{
    try
    {
        // Always check last added first
        if (m_CRC == m_LastAddedCRC && m_lastAddedID >= 0)
            return m_lastAddedID;

        // Then check database
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID FROM Main WHERE CRC = %d", m_CRC);
        if (q.eof() == false)
        {
            return q.getIntField("lID");
        }
    }
    CATCH_SQLITE_EXCEPTION

    return -1;
}

// ============================================================================
// AddToDB — ported from Clip.cpp:767
// ============================================================================

bool CClip::AddToDB(bool bCheckForDuplicates)
{
    bool bResult;
    try
    {
        m_Time = CTime::GetCurrentTime().GetTime();
        m_CRC = GenerateCRC();

        if (bCheckForDuplicates && m_parentId < 0)
        {
            int nID = FindDuplicate();
            if (nID >= 0)
            {
                MakeLatestOrder();
                MakeLatestGroupOrder();

                CString sql;
                sql.Format("UPDATE Main SET clipOrder = %f where lID = %d;",
                           m_clipOrder, nID);

                GetDittoDB().execDML(sql);

                if (m_parentId > -1)
                {
                    sql.Format("UPDATE Main SET clipGroupOrder = %f where lID = %d;",
                               m_clipGroupOrder, nID);
                    GetDittoDB().execDML(sql);
                }

                m_id = nID;

                Log(StrF("Found duplicate clip in db, Id: %d, crc: %d, NewOrder: %f",
                         nID, m_CRC, m_clipOrder));

                return true;
            }
        }
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    int removeStickySettingClipId = -1;

    if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_TOP_STICKY)
        m_stickyClipOrder = this->GetNewTopSticky(m_parentId, -1);
    else if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_LAST_STICKY)
        m_stickyClipOrder = this->GetNewLastSticky(m_parentId, -1);
    else if (m_addToDbStickyEnum == AddToDbStickyEnum::REPLACE_TOP_STICKY)
    {
        m_stickyClipOrder = this->GetNewTopSticky(m_parentId, -1);
        removeStickySettingClipId = GetExistingTopStickyClipId(m_parentId);
    }

    bResult = false;
    if (AddToMainTable())
    {
        bResult = AddToDataTable();
    }

    if (bResult)
    {
        if (removeStickySettingClipId > 0)
            RemoveStickySetting(removeStickySettingClipId, m_parentId);
    }

    return bResult;
}

// ============================================================================
// AddToMainTable — ported from Clip.cpp:953
// ============================================================================

bool CClip::AddToMainTable()
{
    try
    {
        m_Desc.Replace("'", "''");
        m_csQuickPaste.Replace("'", "''");

        CString cs;
        cs.Format("INSERT into Main (lDate, mText, lShortCut, lDontAutoDelete, CRC, bIsGroup, lParentID, QuickPasteText, clipOrder, clipGroupOrder, globalShortCut, lastPasteDate, stickyClipOrder, stickyClipGroupOrder, MoveToGroupShortCut, GlobalMoveToGroupShortCut) "
                  "values(%lld, '%s', %d, %d, %d, %d, %d, '%s', %f, %f, %d, %lld, %f, %f, %d, %d);",
                  (long long)m_Time.GetTime(),
                  (const char*)m_Desc,
                  m_shortCut,
                  m_dontAutoDelete,
                  m_CRC,
                  m_bIsGroup,
                  m_parentId,
                  (const char*)m_csQuickPaste,
                  m_clipOrder,
                  m_clipGroupOrder,
                  m_globalShortCut,
                  (long long)CTime::GetCurrentTime().GetTime(),
                  m_stickyClipOrder,
                  m_stickyClipGroupOrder,
                  m_moveToGroupShortCut,
                  m_globalMoveToGroupShortCut);

        GetDittoDB().execDML(cs);
        m_id = (long)GetDittoDB().lastRowId();

        Log(StrF("Added clip to main table, Id: %d, ParentId: %d Desc: %.40s, Order: %f",
                 m_id, m_parentId, (const char*)m_Desc, m_clipOrder));

        m_LastAddedCRC = m_CRC;
        m_lastAddedID = m_id;
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    return true;
}

// ============================================================================
// ModifyMainTable — ported from Clip.cpp:994
// ============================================================================

bool CClip::ModifyMainTable()
{
    bool bRet = false;
    try
    {
        m_Desc.Replace("'", "''");
        m_csQuickPaste.Replace("'", "''");

        GetDittoDB().execDMLEx("UPDATE Main SET lShortCut = %d, "
            "mText = '%s', "
            "lParentID = %d, "
            "lDontAutoDelete = %d, "
            "QuickPasteText = '%s', "
            "clipOrder = %f, "
            "clipGroupOrder = %f, "
            "globalShortCut = %d, "
            "stickyClipOrder = %f, "
            "stickyClipGroupOrder = %f, "
            "MoveToGroupShortCut = %d, "
            "GlobalMoveToGroupShortCut = %d "
            "WHERE lID = %d;",
            m_shortCut,
            (const char*)m_Desc,
            m_parentId,
            m_dontAutoDelete,
            (const char*)m_csQuickPaste,
            m_clipOrder,
            m_clipGroupOrder,
            m_globalShortCut,
            m_stickyClipOrder,
            m_stickyClipGroupOrder,
            m_moveToGroupShortCut,
            m_globalMoveToGroupShortCut,
            m_id);

        bRet = true;
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    return bRet;
}

// ============================================================================
// ModifyDescription — ported from Clip.cpp:1036
// ============================================================================

bool CClip::ModifyDescription()
{
    bool bRet = false;
    try
    {
        m_Desc.Replace("'", "''");
        GetDittoDB().execDMLEx("UPDATE Main SET mText = '%s' WHERE lID = %d;",
                               (const char*)m_Desc, m_id);
        bRet = true;
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    return bRet;
}

// ============================================================================
// AddToDataTable — ported from Clip.cpp:1056
// ============================================================================

bool CClip::AddToDataTable()
{
    CClipFormat* pCF;

    try
    {
        CppSQLite3Statement stmt = GetDittoDB().compileStatement("insert into Data values (NULL, ?, ?, ?);");

        for (INT_PTR i = m_Formats.GetSize()-1; i >= 0; i--)
        {
            pCF = &m_Formats.ElementAt(i);

            CString formatName = GetFormatName(pCF->m_cfType);
            int clipSize = 0;

            stmt.bind(1, m_id);
            stmt.bind(2, formatName);

            const unsigned char *Data = (const unsigned char *)GlobalLock(pCF->m_hgData);
            if (Data)
            {
                clipSize = (int)GlobalSize(pCF->m_hgData);
                stmt.bind(3, Data, clipSize);
            }
            GlobalUnlock(pCF->m_hgData);

            stmt.execDML();
            stmt.reset();

            pCF->m_dataId = (long)GetDittoDB().lastRowId();

            Log(StrF("Added ClipData to DB, Id: %d, ParentId: %d Type: %s, size: %d",
                     pCF->m_dataId, m_id, (const char*)formatName, clipSize));
        }
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    return true;
}

// ============================================================================
// Order management — ported verbatim
// ============================================================================

double CClip::GetNewOrder(int parentId, int clipId)
{
    double maxOrder = 0;
    try
    {
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT MAX(clipOrder) FROM Main WHERE lParentID = %d", parentId);
        if (q.eof() == false)
            maxOrder = q.getFloatField(0);
    }
    CATCH_SQLITE_EXCEPTION

    return maxOrder + 1;
}

double CClip::GetNewLastOrder(int parentId, int clipId)
{
    double minOrder = 0;
    try
    {
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT MIN(clipOrder) FROM Main WHERE lParentID = %d", parentId);
        if (q.eof() == false)
            minOrder = q.getFloatField(0);
    }
    CATCH_SQLITE_EXCEPTION

    return minOrder - 1;
}

void CClip::MakeLatestOrder()
{
    m_clipOrder = GetNewOrder(m_parentId, m_id);
}

void CClip::MakeLatestGroupOrder()
{
    if (m_parentId > -1)
    {
        m_clipGroupOrder = GetNewOrder(m_parentId, m_id);
    }
}

void CClip::MakeLastOrder()
{
    m_clipOrder = GetNewLastOrder(m_parentId, m_id);
}

void CClip::MakeLastGroupOrder()
{
    if (m_parentId > -1)
    {
        m_clipGroupOrder = GetNewLastOrder(m_parentId, m_id);
    }
}

// ============================================================================
// Sticky clip management — ported verbatim
// ============================================================================

double CClip::GetNewTopSticky(int parentId, int clipId)
{
    double maxOrder = 0;
    try
    {
        CString field = (parentId > -1) ? "stickyClipGroupOrder" : "stickyClipOrder";
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT MAX(%s) FROM Main WHERE lParentID = %d AND %s <> %d",
            (const char*)field, parentId, (const char*)field, INVALID_STICKY);
        if (q.eof() == false)
            maxOrder = q.getFloatField(0);
    }
    CATCH_SQLITE_EXCEPTION

    return maxOrder + 1;
}

double CClip::GetNewLastSticky(int parentId, int clipId)
{
    double minOrder = 0;
    try
    {
        CString field = (parentId > -1) ? "stickyClipGroupOrder" : "stickyClipOrder";
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT MIN(%s) FROM Main WHERE lParentID = %d AND %s <> %d",
            (const char*)field, parentId, (const char*)field, INVALID_STICKY);
        if (q.eof() == false)
            minOrder = q.getFloatField(0);
    }
    CATCH_SQLITE_EXCEPTION

    return minOrder - 1;
}

int CClip::GetExistingTopStickyClipId(int parentId)
{
    try
    {
        CString field = (parentId > -1) ? "stickyClipGroupOrder" : "stickyClipOrder";
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID FROM Main WHERE lParentID = %d AND %s <> %d ORDER BY %s DESC LIMIT 1",
            parentId, (const char*)field, INVALID_STICKY, (const char*)field);
        if (q.eof() == false)
            return q.getIntField("lID");
    }
    CATCH_SQLITE_EXCEPTION

    return -1;
}

void CClip::MakeStickyTop(int parentId)
{
    m_stickyClipOrder = GetNewTopSticky(parentId, m_id);
    ModifyMainTable();
}

void CClip::MakeStickyLast(int parentId)
{
    m_stickyClipOrder = GetNewLastSticky(parentId, m_id);
    ModifyMainTable();
}

bool CClip::RemoveStickySetting(int parentId)
{
    m_stickyClipOrder = INVALID_STICKY;
    m_stickyClipGroupOrder = INVALID_STICKY;
    return ModifyMainTable();
}

bool CClip::RemoveStickySetting(int clipId, int parentId)
{
    try
    {
        GetDittoDB().execDMLEx("UPDATE Main SET stickyClipOrder = %d, stickyClipGroupOrder = %d WHERE lID = %d",
                               INVALID_STICKY, INVALID_STICKY, clipId);
        return true;
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)
}

// ============================================================================
// MoveUp / MoveDown — ported verbatim (swap clip orders)
// ============================================================================

void CClip::MoveUp(int parentId)
{
    try
    {
        if (parentId > -1 && m_stickyClipGroupOrder == INVALID_STICKY)
        {
            CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID, clipGroupOrder FROM Main Where lParentID = %d AND stickyClipGroupOrder == -(2147483647) AND clipGroupOrder > %f ORDER BY clipGroupOrder ASC LIMIT 1", parentId, m_clipGroupOrder);
            if (q.eof() == false)
            {
                int idAbove = q.getIntField("lID");
                double orderAbove = q.getFloatField("clipGroupOrder");
                GetDittoDB().execDMLEx("UPDATE Main SET clipGroupOrder = %f WHERE lID = %d", m_clipGroupOrder, idAbove);
                GetDittoDB().execDMLEx("UPDATE Main SET clipGroupOrder = %f WHERE lID = %d", orderAbove, m_id);
                m_clipGroupOrder = orderAbove;
            }
        }
        else if (parentId <= -1 && m_stickyClipOrder == INVALID_STICKY)
        {
            CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID, clipOrder FROM Main Where lParentID = %d AND stickyClipOrder == -(2147483647) AND clipOrder > %f ORDER BY clipOrder ASC LIMIT 1", parentId, m_clipOrder);
            if (q.eof() == false)
            {
                int idAbove = q.getIntField("lID");
                double orderAbove = q.getFloatField("clipOrder");
                GetDittoDB().execDMLEx("UPDATE Main SET clipOrder = %f WHERE lID = %d", m_clipOrder, idAbove);
                GetDittoDB().execDMLEx("UPDATE Main SET clipOrder = %f WHERE lID = %d", orderAbove, m_id);
                m_clipOrder = orderAbove;
            }
        }
    }
    CATCH_SQLITE_EXCEPTION
}

void CClip::MoveDown(int parentId)
{
    try
    {
        if (parentId > -1 && m_stickyClipGroupOrder == INVALID_STICKY)
        {
            CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID, clipGroupOrder FROM Main Where lParentID = %d AND stickyClipGroupOrder == -(2147483647) AND clipGroupOrder < %f ORDER BY clipGroupOrder DESC LIMIT 1", parentId, m_clipGroupOrder);
            if (q.eof() == false)
            {
                int idBelow = q.getIntField("lID");
                double orderBelow = q.getFloatField("clipGroupOrder");
                GetDittoDB().execDMLEx("UPDATE Main SET clipGroupOrder = %f WHERE lID = %d", m_clipGroupOrder, idBelow);
                GetDittoDB().execDMLEx("UPDATE Main SET clipGroupOrder = %f WHERE lID = %d", orderBelow, m_id);
                m_clipGroupOrder = orderBelow;
            }
        }
        else if (parentId <= -1 && m_stickyClipOrder == INVALID_STICKY)
        {
            CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT lID, clipOrder FROM Main Where lParentID = %d AND stickyClipOrder == -(2147483647) AND clipOrder < %f ORDER BY clipOrder DESC LIMIT 1", parentId, m_clipOrder);
            if (q.eof() == false)
            {
                int idBelow = q.getIntField("lID");
                double orderBelow = q.getFloatField("clipOrder");
                GetDittoDB().execDMLEx("UPDATE Main SET clipOrder = %f WHERE lID = %d", m_clipOrder, idBelow);
                GetDittoDB().execDMLEx("UPDATE Main SET clipOrder = %f WHERE lID = %d", orderBelow, m_id);
                m_clipOrder = orderBelow;
            }
        }
    }
    CATCH_SQLITE_EXCEPTION
}

// ============================================================================
// LoadMainTable — ported from Clip.cpp:1560
// ============================================================================

BOOL CClip::LoadMainTable(int id)
{
    bool bRet = false;
    try
    {
        CppSQLite3Query q = GetDittoDB().execQueryEx("SELECT * FROM Main WHERE lID = %d", id);

        if (q.eof() == false)
        {
            m_Time = q.getInt64Field("lDate");
            m_Desc = q.getStringField("mText");
            m_CRC = q.getIntField("CRC");
            m_parentId = q.getIntField("lParentID");
            m_dontAutoDelete = q.getIntField("lDontAutoDelete");
            m_shortCut = q.getIntField("lShortCut");
            m_bIsGroup = q.getIntField("bIsGroup");
            m_csQuickPaste = q.getStringField("QuickPasteText");
            m_clipOrder = q.getFloatField("clipOrder");
            m_clipGroupOrder = q.getFloatField("clipGroupOrder");
            m_globalShortCut = q.getIntField("globalShortCut");
            m_lastPasteDate = q.getInt64Field("lastPasteDate");
            m_stickyClipOrder = q.getFloatField("stickyClipOrder");
            m_stickyClipGroupOrder = q.getFloatField("stickyClipGroupOrder");
            m_moveToGroupShortCut = q.getIntField("MoveToGroupShortCut");
            m_globalMoveToGroupShortCut = q.getIntField("GlobalMoveToGroupShortCut");

            m_id = id;
            bRet = true;
        }
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

    return bRet;
}

// ============================================================================
// LoadFormat — ported from Clip.cpp:1599
// ============================================================================

HGLOBAL CClip::LoadFormat(int id, UINT cfType)
{
    HGLOBAL hGlobal = 0;
    try
    {
        CString csSQL;
        csSQL.Format("SELECT Data.ooData FROM Data "
                     "INNER JOIN Main ON Main.lID = Data.lParentID "
                     "WHERE Main.lID = %d "
                     "AND Data.strClipBoardFormat = '%s'",
                     id, (const char*)GetFormatName(cfType));

        CppSQLite3Query q = GetDittoDB().execQuery(csSQL);

        if (q.eof() == false)
        {
            int nDataLen = 0;
            const unsigned char *cData = q.getBlobField(0, nDataLen);
            if (cData != NULL)
            {
                hGlobal = NewGlobalP((LPVOID)cData, nDataLen);
            }
        }
    }
    CATCH_SQLITE_EXCEPTION

    return hGlobal;
}

// ============================================================================
// LoadFormats — ported from Clip.cpp:1633
// ============================================================================

bool CClip::LoadFormats(int id, bool bOnlyLoad_CF_TEXT, bool includeRichTextForTextOnly, int dataId)
{
    DWORD startTick = GetTickCount();
    CClipFormat cf;
    HGLOBAL hGlobal = 0;
    m_Formats.RemoveAll();

    try
    {
        CString textFilter = "";
        if (bOnlyLoad_CF_TEXT)
        {
            textFilter = "(strClipBoardFormat = 'CF_TEXT' OR strClipBoardFormat = 'CF_UNICODETEXT' OR strClipBoardFormat = 'CF_HDROP'";
            if (includeRichTextForTextOnly)
                textFilter = textFilter + " OR strClipBoardFormat = 'Rich Text Format') AND ";
            else
                textFilter = textFilter + ") AND ";
        }

        CString dataIdFilter = "";
        if (dataId >= 0)
            dataIdFilter.Format("AND lID = %d ", dataId);

        CString csSQL;
        csSQL.Format("SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data "
                     "WHERE %s lParentID = %d %s ORDER BY Data.lID desc",
                     (const char*)textFilter, id, (const char*)dataIdFilter);

        CppSQLite3Query q = GetDittoDB().execQuery(csSQL);

        while (q.eof() == false)
        {
            cf.m_dataId = q.getIntField("lID");
            cf.m_parentId = q.getIntField("lParentID");
            cf.m_cfType = GetFormatID(q.getStringField("strClipBoardFormat"));

            int nDataLen = 0;
            const unsigned char *cData = q.getBlobField("ooData", nDataLen);
            if (cData != NULL)
            {
                hGlobal = NewGlobalP((LPVOID)cData, nDataLen);
            }

            cf.m_hgData = hGlobal;
            m_Formats.Add(cf);

            // Don't auto-delete since m_Formats now owns it
            cf.m_hgData = 0;

            q.nextRow();
        }
    }
    CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

    DWORD endTick = GetTickCount();
    if ((endTick - startTick) > 150)
        Log(StrF("Paste Timing LoadFormats: %d, ClipId: %d", endTick - startTick, id));

    return m_Formats.GetSize() > 0;
}

// ============================================================================
// LoadTypes — ported from Clip.cpp:1720
// ============================================================================

void CClip::LoadTypes(int id, CClipTypes& types)
{
    types.RemoveAll();
    try
    {
        CString csSQL;
        csSQL.Format("SELECT strClipBoardFormat FROM Data WHERE lParentID = %d ORDER BY Data.lID desc", id);

        CppSQLite3Query q = GetDittoDB().execQuery(csSQL);

        while (q.eof() == false)
        {
            CLIPFORMAT cfType = GetFormatID(q.getStringField("strClipBoardFormat"));
            types.Add(cfType);
            q.nextRow();
        }
    }
    CATCH_SQLITE_EXCEPTION
}

// ============================================================================
// Format getters — ported verbatim
// ============================================================================

CStringW CClip::GetUnicodeTextFormat()
{
    CClipFormat *pFormat = m_Formats.FindFormat(CF_UNICODETEXT);
    if (pFormat)
        return pFormat->GetAsCString();
    return CStringW();
}

CStringA CClip::GetCFTextTextFormat()
{
    CClipFormat *pFormat = m_Formats.FindFormat(CF_TEXT);
    if (pFormat)
        return pFormat->GetAsCStringA();
    return CStringA();
}

CStringA CClip::GetRTFTextFormat()
{
    // RTF format ID varies — find by name in format list
    for (INT_PTR i = 0; i < m_Formats.GetSize(); i++)
    {
        CString name = GetFormatName(m_Formats.ElementAt(i).m_cfType);
        if (name.Find("Rich Text Format") >= 0)
            return m_Formats.ElementAt(i).GetAsCStringA();
    }
    return CStringA();
}

BOOL CClip::ContainsClipFormat(CLIPFORMAT clipFormat)
{
    return m_Formats.FindFormat(clipFormat) != NULL;
}

// ============================================================================
// CClipList
// ============================================================================

CClipList::~CClipList()
{
    for (auto it = this->begin(); it != this->end(); ++it)
        delete *it;
    this->clear();
}

int CClipList::AddToDB(bool bLatestOrder)
{
    int saved = 0;
    for (auto it = this->begin(); it != this->end(); ++it)
    {
        CClip *pClip = *it;
        if (bLatestOrder)
            pClip->MakeLatestOrder();
        if (pClip->AddToDB())
            saved++;
    }
    return saved;
}

const CClipList& CClipList::operator=(const CClipList &cliplist)
{
    for (auto it = this->begin(); it != this->end(); ++it)
        delete *it;
    this->clear();

    for (auto it = cliplist.begin(); it != cliplist.end(); ++it)
    {
        CClip *pNewClip = new CClip;
        *pNewClip = **it;
        this->push_back(pNewClip);
    }

    return *this;
}
