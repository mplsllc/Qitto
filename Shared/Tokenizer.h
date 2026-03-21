#pragma once
#include "ArrayEx.h"

#ifdef LINUX_PORT

// Linux-compatible CTokenizer — uses CString (which wraps QString)
// The original uses CArrayEx<TCHAR> for delimiter matching with binary search.
// On Linux, TCHAR is char, and we use simple string search instead.
class CTokenizer
{
public:
	CString m_cs;
	CString m_delim;
	int m_nCurPos;

	CTokenizer(const CString& cs, const CString& csDelim)
		: m_cs(cs), m_delim(csDelim), m_nCurPos(0) {}

	void SetDelimiters(const CString& csDelim) { m_delim = csDelim; }

	bool IsDelim(char c) const {
		QByteArray d = static_cast<const QString&>(m_delim).toUtf8();
		return d.contains(c);
	}

	bool Next(CString& cs)
	{
		cs = CString();
		QByteArray utf8 = static_cast<const QString&>(m_cs).toUtf8();
		int len = utf8.size();

		while (m_nCurPos < len && IsDelim(utf8.at(m_nCurPos)))
			++m_nCurPos;

		if (m_nCurPos >= len)
			return false;

		int nStartPos = m_nCurPos;

		while (m_nCurPos < len && !IsDelim(utf8.at(m_nCurPos)))
			++m_nCurPos;

		cs = CString(utf8.mid(nStartPos, m_nCurPos - nStartPos));
		return true;
	}

	CString Tail()
	{
		QByteArray utf8 = static_cast<const QString&>(m_cs).toUtf8();
		int len = utf8.size();
		int nCurPos = m_nCurPos;

		while (nCurPos < len && IsDelim(utf8.at(nCurPos)))
			++nCurPos;

		if (nCurPos < len)
			return CString(utf8.mid(nCurPos));

		return CString();
	}
};

#else

// Original Windows CTokenizer
class CTokenizer
{
public:
	CString m_cs;
	CArrayEx < TCHAR > m_delim;
	int m_nCurPos;

	CTokenizer(const CString& cs, const CString& csDelim) : m_cs(cs), m_nCurPos(0)
	{
		SetDelimiters(csDelim);
	}

	void SetDelimiters(const CString& csDelim)
	{
		for (int i = 0; i < csDelim.GetLength(); ++i)
		{
			m_delim.Add(csDelim[i]);
		}

		m_delim.SortAscending();
	}

	bool Next(CString& cs)
	{
		cs.Empty();
		int len = m_cs.GetLength();

		while (m_nCurPos < len && m_delim.Find(m_cs[m_nCurPos]))
		{
			++m_nCurPos;
		}

		if (m_nCurPos >= len)
		{
			return false;
		}

		int nStartPos = m_nCurPos;

		while (m_nCurPos < len && !m_delim.Find(m_cs[m_nCurPos]))
		{
			++m_nCurPos;
		}

		cs = m_cs.Mid(nStartPos, m_nCurPos - nStartPos);

		return true;
	}

	CString Tail()
	{
		int len = m_cs.GetLength();
		int nCurPos = m_nCurPos;

		while (nCurPos < len && m_delim.Find(m_cs[nCurPos]))
		{
			++nCurPos;
		}

		CString csResult;
		if (nCurPos < len)
		{
			csResult = m_cs.Mid(nCurPos);
		}

		return csResult;
	}
};

#endif // LINUX_PORT
