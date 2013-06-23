#include "global.h"
#include "MeterDisplay.h"
#include "RageUtil.h"
#include "GameState.h"
#include "song.h"
#include "ActorUtil.h"
#include "XmlFile.h"
#include "RageLog.h"

REGISTER_ACTOR_CLASS(MeterDisplay)
REGISTER_ACTOR_CLASS(SongMeterDisplay)

MeterDisplay::MeterDisplay()
{
}

void MeterDisplay::Load( CString sStreamPath, float fStreamWidth, CString sTipPath )
{
	m_fStreamWidth = fStreamWidth;

	m_sprStream.Load( sStreamPath );
	m_sprStream->SetZoomX( fStreamWidth / m_sprStream->GetUnzoomedWidth() );
	this->AddChild( m_sprStream );

	m_sprTip.Load( sTipPath );
	this->AddChild( m_sprTip );

	SetPercent( 0.5f );
}

void MeterDisplay::LoadFromNode( const CString& sDir, const XNode* pNode )
{
	LOG->Trace( "MeterDisplay::LoadFromNode(%s,node)", sDir.c_str() );

	if( !pNode->GetAttrValue( "StreamWidth", m_fStreamWidth ) )
		RageException::Throw( "MeterDisplay in " + sDir + " missing StreamWidth attribute" );

	{
		CString sStreamPath;
		if( !pNode->GetAttrValue( "StreamPath", sStreamPath ) )
			RageException::Throw( "MeterDisplay in " + sDir + " missing StreamPath attribute" );

		sStreamPath = sDir + sStreamPath;
		ActorUtil::ResolvePath( sStreamPath, sDir );

		m_sprStream.Load( sStreamPath );
		m_sprStream->SetZoomX( m_fStreamWidth / m_sprStream->GetUnzoomedWidth() );
		this->AddChild( m_sprStream );
	}

	const XNode* pChild = pNode->GetChild( "Tip" );
	if( pChild != NULL )
	{
		m_sprTip.LoadFromNode( sDir, pChild );
		this->AddChild( m_sprTip );
	}

	SetPercent( 0.5f );

	ActorFrame::LoadFromNode( sDir, pNode );
}

void MeterDisplay::SetPercent( float fPercent )
{
	ASSERT( fPercent >= 0 && fPercent <= 1 );

	m_sprStream->SetCropRight( 1-fPercent );

	m_sprTip->SetX( SCALE(fPercent, 0.f, 1.f, -m_fStreamWidth/2, m_fStreamWidth/2) );
}

void SongMeterDisplay::Update( float fDeltaTime )
{
	float fSongStartSeconds = GAMESTATE->m_pCurSong->m_Timing.GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fFirstBeat );
	float fSongEndSeconds = GAMESTATE->m_pCurSong->m_Timing.GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fLastBeat );
	float fPercentPositionSong = SCALE( GAMESTATE->m_fMusicSeconds, fSongStartSeconds, fSongEndSeconds, 0.0f, 1.0f );
	CLAMP( fPercentPositionSong, 0, 1 );
	
	SetPercent( fPercentPositionSong );

	MeterDisplay::Update( fDeltaTime );
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
