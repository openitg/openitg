#include "global.h"
#include "AutoActor.h"
#include "ThemeManager.h"
#include "ActorUtil.h"

void AutoActor::Unload()
{
	delete m_pActor;
	m_pActor=NULL;
}

void AutoActor::Load( const CString &sPath )
{
	Unload();
	m_pActor = ActorUtil::MakeActor( sPath );
}

void AutoActor::LoadFromNode( const CString &sDir, const XNode* pNode )
{
	Unload();

	m_pActor = ActorUtil::LoadFromActorFile( sDir, pNode );
}

void AutoActor::LoadAndSetName( const CString &sScreenName, const CString &sActorName )
{
	Load( THEME->GetPathG(sScreenName,sActorName) );
	m_pActor->SetName( sActorName );
}

AutoActor::AutoActor( const AutoActor &cpy )
{
        if( cpy.m_pActor == NULL )
                m_pActor = NULL;
        else
                m_pActor = cpy.m_pActor->Copy();
}

AutoActor &AutoActor::operator=( const AutoActor &cpy )
{
        Unload();

        if( cpy.m_pActor == NULL )
                m_pActor = NULL;
        else
                m_pActor = cpy.m_pActor->Copy();
        return *this;
}

/*
 * (c) 2003-2004 Chris Danford
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
