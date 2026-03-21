// Clip.h: classes for manage clips
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
#define AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_

#if !defined(LINUX_PORT) && _MSC_VER > 1000
#pragma once
#endif

#ifndef LINUX_PORT
#include <afxole.h>
#include <afxtempl.h>
#include <memory>
#include "tinyxml\tinyxml.h"
#include "..\Shared\IClip.h"
#endif

#include "Misc.h"
#include <memory>

class CClip;

#ifndef LINUX_PORT
class CCopyThread;
#endif

typedef CArray<CLIPFORMAT, CLIPFORMAT> CClipTypes;

#ifndef LINUX_PORT
/*----------------------------------------------------------------------------*\
	COleDataObjectEx — Windows-only OLE clipboard access
\*----------------------------------------------------------------------------*/
class COleDataObjectEx : public COleDataObject
{
public:
	HGLOBAL GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);
	std::shared_ptr<CClipTypes> GetAvailableTypes();
};
#endif // !LINUX_PORT

/*----------------------------------------------------------------------------*\
	CClipFormat - holds the data of one clip format.
\*----------------------------------------------------------------------------*/
#ifdef LINUX_PORT
class CClipFormat
#else
class CClipFormat : public IClipFormat
#endif
{
public:
	CLIPFORMAT m_cfType;
    HGLOBAL m_hgData;
	bool m_autoDeleteData;
	int m_dataId;
	int m_parentId;

	CClipFormat(CLIPFORMAT cfType = 0, HGLOBAL hgData = 0, int parentId = -1);
#ifdef LINUX_PORT
	CClipFormat(const CClipFormat &other);
	CClipFormat& operator=(const CClipFormat &other);
#endif
	~CClipFormat();

	void Clear();
	virtual void Free();

	virtual CLIPFORMAT Type() { return m_cfType; }
	virtual HGLOBAL Data() { return m_hgData; }
	virtual void Type(CLIPFORMAT type) { m_cfType = type; }
	virtual void Data(HGLOBAL data) { m_hgData = data; }
	virtual void AutoDeleteData(bool autoDeleteData) { m_autoDeleteData = autoDeleteData; }
	virtual bool AutoDeleteData()	{ return m_autoDeleteData; }

	CStringA GetAsCStringA() {
		CStringA ret;
		if (m_hgData)
		{
			LPVOID data = GlobalLock(m_hgData);
			int size = (int)GlobalSize(m_hgData);
			if (data != NULL && size > 0)
			{
				ret = CStringA((char *)data, size-1);
			}
			GlobalUnlock(m_hgData);
		}
		return ret;
	}

	CString GetAsCString() {
		CString ret;
		if (m_hgData)
		{
			LPVOID data = GlobalLock(m_hgData);
			int size = (int)GlobalSize(m_hgData);
			if (data != NULL && size > 0)
			{
#ifdef LINUX_PORT
				// On Linux, data is UTF-8
				ret = CString((char *)data, size-1);
#else
				ret = CString((wchar_t *)data, ((size / (sizeof(wchar_t))) - 1));
#endif
			}
			GlobalUnlock(m_hgData);
		}
		return ret;
	}

#ifndef LINUX_PORT
	Gdiplus::Bitmap *CreateGdiplusBitmap();
#endif
};

/*----------------------------------------------------------------------------*\
	CClipFormats - holds an array of CClipFormat
\*----------------------------------------------------------------------------*/
#ifdef LINUX_PORT
class CClipFormats : public CArray<CClipFormat,CClipFormat&>
#else
class CClipFormats : public CArray<CClipFormat,CClipFormat&>, public IClipFormats
#endif
{
public:
	CClipFormat* FindFormat(UINT cfType);

	virtual int Size() { return (int)this->GetCount(); }
#ifndef LINUX_PORT
	virtual IClipFormat *GetAt(int nPos) { return &this->ElementAt(nPos); }
#endif
	virtual void DeleteAt(int nPos) { this->RemoveAt(nPos); }
	virtual void DeleteAll() { this->RemoveAll(); }
	virtual INT_PTR AddNew(CLIPFORMAT type, HGLOBAL data) {CClipFormat ft(type, data, -1); ft.m_autoDeleteData = false; return this->Add(ft); }
#ifndef LINUX_PORT
	virtual IClipFormat *FindFormatEx(CLIPFORMAT type)	{ return FindFormat((UINT)type); }
#endif
	virtual bool RemoveFormat(CLIPFORMAT type);
};


/*----------------------------------------------------------------------------*\
	CClip - holds multiple CClipFormats and clip statistics
	- provides static functions for manipulating a Clip as a single unit.
\*----------------------------------------------------------------------------*/
#ifdef LINUX_PORT

// AddToDbStickyEnum — controls sticky behavior when adding to DB
class AddToDbStickyEnum
{
public:
	enum AddToDbSticky
	{
		NONE,
		MAKE_TOP_STICKY,
		MAKE_LAST_STICKY,
		REPLACE_TOP_STICKY
	};
};

class CClip
#else
class CClip : public IClip
#endif
{
public:
	CClip();
	~CClip();
	const CClip& operator=(const CClip &clip);

	static DWORD m_LastAddedCRC;
	static int m_lastAddedID;

	int m_id;
	CClipFormats m_Formats;
	CTime m_Time;
	CString m_Desc;
	ULONG m_lTotalCopySize;
	int m_parentId;
	int m_dontAutoDelete;
	int m_shortCut;
	BOOL m_bIsGroup;
	DWORD m_CRC;
	CString m_csQuickPaste;
	int m_param1;
	double m_clipOrder;
	double m_clipGroupOrder;
	double m_stickyClipOrder;
	double m_stickyClipGroupOrder;
	BOOL m_globalShortCut;
	CTime m_lastPasteDate;
	int m_moveToGroupShortCut;
	BOOL m_globalMoveToGroupShortCut;
	CopyReasonEnum::CopyReason m_copyReason;

	virtual CString Description() { return m_Desc; }
	virtual void Description(CString csValue) { m_Desc = csValue; }
	virtual CTime PasteTime() { return m_Time; }
	virtual int ID() { return m_id; }
	virtual int Parent() { return m_parentId; }
	virtual void Parent(int nParent) { m_parentId = nParent; }
	virtual int DontAutoDelete() { return m_dontAutoDelete; }
	virtual void DontAutoDelete(int Dont) { m_dontAutoDelete = Dont; }
	virtual CString QuickPaste() { return m_csQuickPaste; }
	virtual void QuickPaste(CString csValue) { m_csQuickPaste = csValue; }

	virtual void SetSaveToDbSticky(AddToDbStickyEnum::AddToDbSticky option) { m_addToDbStickyEnum = option; }

#ifndef LINUX_PORT
	virtual IClipFormats *Clips() { return (IClipFormats*)&m_Formats; }
#endif

	void Clear();
	void EmptyFormats();
	bool AddFormat(CLIPFORMAT cfType, void* pData, UINT nLen, bool setDesc = false);

#ifndef LINUX_PORT
	int LoadFromClipboard(CClipTypes* pClipTypes, bool checkClipboardIgnore = true, CString activeApp = _T(""), CString activeAppTitle = _T(""));
#endif

	bool SetDescFromText(HGLOBAL hgData, bool unicode);
	bool SetDescFromType();
	bool AddToDB(bool bCheckForDuplicates = true);
	bool ModifyMainTable();
	bool ModifyDescription();
	void MakeLatestOrder();
	void MakeLatestGroupOrder();
	void MakeLastOrder();
	void MakeLastGroupOrder();
	void MakeStickyTop(int parentId);
	void MakeStickyLast(int parentId);
	bool RemoveStickySetting(int parentId);
	BOOL LoadMainTable(int id);
	DWORD GenerateCRC();
	void MoveUp(int parentId);
	void MoveDown(int parentId);

#ifndef LINUX_PORT
	bool SaveFromEditWnd(BOOL bUpdateDesc);
#endif

	CStringW GetUnicodeTextFormat();
	CStringA GetCFTextTextFormat();
	CStringA GetRTFTextFormat();

	BOOL ContainsClipFormat(CLIPFORMAT clipFormat);

#ifndef LINUX_PORT
	BOOL WriteTextToFile(CString path, BOOL unicode, BOOL asci, BOOL rtf, BOOL forceUnicode = FALSE, BOOL utf8 = FALSE);
	BOOL WriteImageToFile(CString path);
	BOOL WriteTextToHtmlFile(CString path);
	BOOL SaveFormats(CString* unicode, CStringA* asci, CStringA* rtf, BOOL updateDescription, std::vector<BYTE>* cf_dibBytes = nullptr, std::vector<BYTE>* pngBytes = nullptr);
#endif

	// Allocates a Global containing the requested Clip's Format Data
	static HGLOBAL LoadFormat(int id, UINT cfType);
	// Fills "formats" with the Data of all Formats in the db for the given Clip ID
	bool LoadFormats(int id, bool bOnlyLoad_CF_TEXT = false, bool includeRichTextForTextOnly = false, int dataId = -1);
	// Fills "types" with all Types in the db for the given Clip ID
	static void LoadTypes(int id, CClipTypes& types);

	static double GetNewOrder(int parentId, int clipId);
	double GetNewLastOrder(int parentId, int clipId);
	static double GetNewTopSticky(int parentId, int clipId);
	static double GetNewLastSticky(int parentId, int clipId);
	static int GetExistingTopStickyClipId(int parentId);
	static bool RemoveStickySetting(int clipId, int parentId);

#ifndef LINUX_PORT
	bool AddFileDataToData(CString &errorMessage);
	Gdiplus::Bitmap *CreateGdiplusBitmap();
#endif

protected:
	bool AddToMainTable();
	bool AddToDataTable();
	int FindDuplicate();

	AddToDbStickyEnum::AddToDbSticky m_addToDbStickyEnum;
};


/*----------------------------------------------------------------------------*\
	CClipList
\*----------------------------------------------------------------------------*/

class CClipList : public CList<CClip*,CClip*>
{
public:
	~CClipList();
	int AddToDB( bool bLatestOrder = false);

	const CClipList& operator=(const CClipList &cliplist);
};

#endif // !defined(AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
