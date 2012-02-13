#ifndef USER_PACK_MANAGER_H
#define USER_PACK_MANAGER_H
#include "global.h"

/* path on the memory card where packs are checked for */
static const CString USER_PACK_TRANSFER_PATH = "UserPacks/";

/* path on the VFS where transferred packs end up */
static const CString USER_PACK_SAVE_PATH = "UserPacks/";

class UserPackManager
{
public:
	void GetUserPacks( vector<CString> &sAddTo );

	void MountAll();
	bool IsPackMountable( const CString &sPack, CString &sError );
	bool IsPackTransferable( const CString &sPack, const CString &sPath, CString &sError );

	/* XXX: duped from RageUtil so we don't pull in that header. */
	bool TransferPack( const CString &sPack, const CString &sDest,
		bool (*CopyFn)(uint64_t, uint64_t), CString &sError );
	bool Remove( const CString &sPack );

protected:
	CString GetPackMountPoint( const CString &sPack );
};

extern UserPackManager*	UPACKMAN;	// global and, uh, "accessable" from anywhere in our program

#endif /* USER_PACK_MANAGER_H */

/*
 * (c) 2009 Pat Mac ("infamouspat")
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

