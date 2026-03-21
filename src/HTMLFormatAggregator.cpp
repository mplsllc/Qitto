#ifndef LINUX_PORT
#include "stdafx.h"
#endif
#include "HTMLFormatAggregator.h"
#include "Misc.h"
#include "../Shared/Tokenizer.h"

CHTMLFormatAggregator::CHTMLFormatAggregator(CStringA csSepator) :
	m_csSeparator(csSepator)
{
	m_csSeparator.Replace("\r\n", "<br>");
}

CHTMLFormatAggregator::~CHTMLFormatAggregator(void)
{
}

bool CHTMLFormatAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	LPSTR pText = (LPSTR)lpData;
	if(pText == NULL)
	{
		return false;
	}

	//Ensure it's null terminated
	if(pText[nDataSize-1] != '\0')
	{
		pText[nDataSize-1] = '\0';
	}

	CHTMFormatStruct HtmlData;
	if(HtmlData.GetData(pText))
	{
		m_csNewText += HtmlData.GetFragment();

		if(m_csSourceURL.IsEmpty())
			m_csSourceURL = HtmlData.GetURL();
		if(m_csVersion.IsEmpty())
			m_csVersion = HtmlData.GetVersion();

		if(nPos != nCount-1)
		{
			m_csNewText += m_csSeparator;
		}
	}

	return true;
}

HGLOBAL CHTMLFormatAggregator::GetHGlobal()
{
	CHTMFormatStruct HtmlData;
	HtmlData.SetFragment(m_csNewText);
	HtmlData.SetURL(m_csSourceURL);
	HtmlData.SetVersion(m_csVersion);

	CStringA csHtmlFormat;
	HtmlData.Serialize(csHtmlFormat);

	long lLen = csHtmlFormat.GetLength();
	HGLOBAL hGlobal = NewGlobalP(csHtmlFormat.GetBuffer(lLen), lLen+sizeof(char));
	csHtmlFormat.ReleaseBuffer();

	return hGlobal;
}

bool CHTMFormatStruct::GetData(LPCSTR HTML)
{
	CTokenizer Tokenizer(HTML, "\r\n");
	CString Token;
	while(Tokenizer.Next(Token))
	{
		CTokenizer ItemTokenizer(Token, ":");
		CString csParam;
		ItemTokenizer.Next(csParam);
		CString csValue = ItemTokenizer.Tail();

		if(csParam == "Version")
		{
			m_csVersion = csValue;
		}
		else if(csParam == "StartHTML")
		{
			m_lStartHTML = ATOI(csValue);
		}
		else if(csParam == "EndHTML")
		{
			m_lEndHTML = ATOI(csValue);
		}
		else if(csParam == "StartFragment")
		{
			m_lStartFragment = ATOI(csValue);
		}
		else if(csParam == "EndFragment")
		{
			m_lEndFragment = ATOI(csValue);
		}
		else if(csParam == "SourceURL")
		{
			m_csSourceURL = csValue;
			break;
		}
		else if(csParam.Left(5) == "<html")
		{
			break;
		}
	}

	if(m_lStartFragment >= 0 && m_lEndFragment >= 0 && m_lStartFragment < m_lEndFragment)
	{
		m_csFragment = Tokenizer.m_cs.Mid(m_lStartFragment, m_lEndFragment-m_lStartFragment);
#ifdef LINUX_PORT
		m_csFragment = CStringA(static_cast<const QString&>(m_csFragment).trimmed().toUtf8());
#else
		m_csFragment = m_csFragment.Trim();
#endif
	}

	if(m_csFragment.IsEmpty())
	{
		return false;
	}

	return true;
}

bool CHTMFormatStruct::Serialize(CStringA &csHTMLFormat)
{
	CStringA csVersionText("Version:");
	CStringA csStartHTMLText("StartHTML:");
	CStringA csEndHTMLText("EndHTML:");
	CStringA csStartFragmentText("StartFragment:");
	CStringA csEndFragmentText("EndFragment:");
	CStringA csSourceURLText("SourceURL:");
	CStringA csStartFragmentMarkerText("<!--StartFragment-->");
	CStringA csEndFragmentMarkerText("<!--EndFragment-->");
	CStringA csStartHTML("<html><body>");
	CStringA csEndHTML("</body>\r\n</html>");
	long lNumberCharacters = 8;

	//+2 is for the line feeds

	long lCurrentPos = csVersionText.GetLength() + m_csVersion.GetLength() + 2 +
						csStartHTMLText.GetLength() + lNumberCharacters + 2 +
						csEndHTMLText.GetLength() + lNumberCharacters + 2 +
						csStartFragmentText.GetLength() + lNumberCharacters + 2 +
						csEndFragmentText.GetLength() + lNumberCharacters + 2 +
						csSourceURLText.GetLength() + m_csSourceURL.GetLength() + 2;

	m_lStartHTML = lCurrentPos;

	lCurrentPos += csStartHTMLText.GetLength() + 2 +
					csStartFragmentMarkerText.GetLength() + 2;
	m_lStartFragment = lCurrentPos;

	lCurrentPos += m_csFragment.GetLength();
	m_lEndFragment = lCurrentPos;

	lCurrentPos += csEndFragmentMarkerText.GetLength() + 2 +
					csEndHTML.GetLength();
	m_lEndHTML = lCurrentPos;


	csHTMLFormat = csVersionText + m_csVersion + "\r\n";

	CStringA csFormat;
	csFormat.Format("%s%08d\r\n", (const char*)csStartHTMLText, m_lStartHTML);
	csHTMLFormat += csFormat;

	csFormat.Format("%s%08d\r\n", (const char*)csEndHTMLText, m_lEndHTML);
	csHTMLFormat += csFormat;

	csFormat.Format("%s%08d\r\n", (const char*)csStartFragmentText, m_lStartFragment);
	csHTMLFormat += csFormat;

	csFormat.Format("%s%08d\r\n", (const char*)csEndFragmentText, m_lEndFragment);
	csHTMLFormat += csFormat;

	csFormat.Format("%s%s\r\n", (const char*)csSourceURLText, (const char*)m_csSourceURL);
	csHTMLFormat += csFormat;

	csFormat.Format("%s\r\n%s", (const char*)csStartHTML, (const char*)csStartFragmentMarkerText);
	csHTMLFormat += csFormat;

	csHTMLFormat += m_csFragment;

	csFormat.Format("%s\r\n%s", (const char*)csEndFragmentMarkerText, (const char*)csEndHTML);
	csHTMLFormat += csFormat;

	return true;
}
