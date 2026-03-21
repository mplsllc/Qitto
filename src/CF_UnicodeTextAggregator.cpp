#ifdef LINUX_PORT
#include "linux/compat.h"
#else
#include "stdafx.h"
#endif
#include "CF_UnicodeTextAggregator.h"
#include "Misc.h"

CCF_UnicodeTextAggregator::CCF_UnicodeTextAggregator(CStringW csSeparator) :
	m_csSeparator(csSeparator)
{
}

CCF_UnicodeTextAggregator::~CCF_UnicodeTextAggregator(void)
{
}

bool CCF_UnicodeTextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
#ifndef LINUX_PORT
	if (cfType == CF_HDROP)
	{
		CString hDropFiles = _T("");
		HDROP drop = (HDROP)GlobalLock((HDROP)lpData);
		int nNumFiles = DragQueryFile(drop, -1, NULL, 0);
		TCHAR file[MAX_PATH];

		for (int nFile = 0; nFile < nNumFiles; nFile++)
		{
			if (DragQueryFile(drop, nFile, file, sizeof(file)) > 0)
			{
				hDropFiles += file;
				hDropFiles += _T("\r\n");
			}
		}

		if (hDropFiles != _T(""))
		{
			m_csNewText += hDropFiles;

			if (nPos != nCount - 1)
			{
				m_csNewText += m_csSeparator;
			}

			return true;
		}
		return false;
	}

	LPCWSTR pText = (LPCWSTR)lpData;
	if(pText == NULL)
	{
		return false;
	}

	int stringLen = nDataSize/sizeof(wchar_t);

	//Ensure it's null terminated
	if(pText[stringLen-1] != '\0')
	{
		int len = 0;
		for(len = 0; len < stringLen && pText[len] != '\0'; len++ )
		{
		}
		// if it is not null terminated, skip this item
		if(len >= stringLen)
			return false;
	}

	m_csNewText += pText;
#else
	// On Linux, "unicode" text is UTF-8 — same as narrow text
	if (cfType == CF_HDROP)
	{
		// File drop as text/uri-list — treat as text
	}

	LPCSTR pText = (LPCSTR)lpData;
	if(pText == NULL)
	{
		return false;
	}

	// Ensure null terminated
	if(pText[nDataSize-1] != '\0')
	{
		int len = 0;
		for(len = 0; len < nDataSize && pText[len] != '\0'; len++ )
		{
		}
		if(len >= nDataSize)
			return false;
	}

	m_csNewText += pText;
#endif

	if(nPos != nCount-1)
	{
		m_csNewText += m_csSeparator;
	}

	return true;
}

HGLOBAL CCF_UnicodeTextAggregator::GetHGlobal()
{
#ifdef LINUX_PORT
	// On Linux, unicode text is UTF-8 (same as narrow)
	long lLen = m_csNewText.GetLength();
	QByteArray utf8 = static_cast<const QString&>(m_csNewText).toUtf8();
	HGLOBAL hGlobal = NewGlobalP(utf8.constData(), utf8.size() + 1);
	return hGlobal;
#else
	long lLen = m_csNewText.GetLength() * sizeof(wchar_t);
	HGLOBAL hGlobal = NewGlobalP(m_csNewText.GetBuffer(lLen), lLen+sizeof(wchar_t));
	m_csNewText.ReleaseBuffer();
	return hGlobal;
#endif
}
