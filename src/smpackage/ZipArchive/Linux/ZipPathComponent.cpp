////////////////////////////////////////////////////////////////////////////////
// $Workfile: ZipPathComponent.cpp $
// $Archive: /ZipArchive_Linux/ZipPathComponent.cpp $
// $Date: 2003/07/22 02:10:29 $ $Author: gmaynard $
////////////////////////////////////////////////////////////////////////////////
// This source file is part of the ZipArchive library source distribution and
// is Copyright 2000-2003 by Tadeusz Dracz (http://www.artpol-software.com/)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// For the licensing details see the file License.txt
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ZipPathComponent.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CZipPathComponent::~CZipPathComponent()
{

}

void CZipPathComponent::SetFullPath(LPCTSTR lpszFullPath)
{
	
	CZipString szTempPath(lpszFullPath);
	const CZipString szPrefix = _T("\\\\?\\unc\\");
	int i = -1, iLen = szPrefix.GetLength();
	if (iLen > szTempPath.GetLength())
		iLen = szTempPath.GetLength();
	CZipString szPossiblePrefix = szTempPath.Left(iLen);
	szPossiblePrefix.MakeLower(); // must perform case insensitive comparison
	while (++i < iLen && szPossiblePrefix[i] == szPrefix[i]); 
	if (i == 2 || i == 4 || i == 8) // unc path, unicode path or unc path meeting windows file name conventions
	{
		m_szPrefix = szTempPath.Left(i);
		szTempPath = szTempPath.Mid(i);		
	}
	else
		m_szPrefix.Empty();


	m_szDrive.Empty(); 
	m_szFileTitle.Empty();
	m_szDirectory.Empty();
	m_szFileExt.Empty();
	int p;
	for (p = szTempPath.GetLength() - 1; p >= 0; p--)
		if (szTempPath[p] == m_cSeparator)
			break;

	if (p != -1)
	{
		m_szDirectory = szTempPath.Left(p);
		if (p == szTempPath.GetLength() - 1 )
			return; // no filename present
		else 
			p++;
	}
	else 
		p = 0;

	// p points at the beginning of the filename
	m_szFileTitle = szTempPath.Mid(p);
	for (p = m_szFileTitle.GetLength() - 1; p >= 0; p--)
		if (m_szFileTitle[p] == _T('.'))
			break;

	if (p != -1)
	{
		m_szFileExt = m_szFileTitle.Mid(p+1);
		m_szFileTitle = m_szFileTitle.Left(p);
	}


	
}


CZipString CZipPathComponent::GetNoDrive() const
{
	CZipString szPath = m_szDirectory;
	CZipString szFileName = GetFileName();
	if (!szFileName.IsEmpty() && !szPath.IsEmpty())
		szPath += m_cSeparator;

	szPath += szFileName;
	return szPath;	
}

