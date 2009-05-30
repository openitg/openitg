#ifndef USER_PACK_MANAGER_H
#define USER_PACK_MANAGER_H
#include "global.h"

class UserPackManager
{
public:
	UserPackManager( const CString sTransferPath, const CString sSavePath );

	void			GetCurrentUserPacks( CStringArray &sAddTo );
	void			MergePacksToVFS();
	void			AddBlacklistedFolder( const CString sBlacklistedFolder );

	CString			GetUserTransferPath() { return m_sTransferPath; } /* /UserPacks part of /@mc1/UserPacks */
	CString			GetSavePath() { return m_sSavePath; } /* /UserPacks */

	bool			UnlinkAndRemovePack( const CString sPack );

	bool			IsPackAddable( const CString sPack, CString &sError );
	bool			IsPackTransferable( const CString sPack, CString &sError );
	bool			TransferPack( const CString sPack, const CString sDestZip, void(*OnUpdate)(float), CString &sError );

protected:
	CString			m_sTransferPath;
	CString			m_sSavePath;
	CStringArray	m_asBlacklistedFolders;

	CString			GetPackMountPoint( const CString sPack );
};

extern UserPackManager* UPACKMAN;
#endif /* USER_PACK_MANAGER_H */

/*
 * (c) 2009 "infamouspat"
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */