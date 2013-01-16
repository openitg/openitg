#include "global.h"
#include "GraphDisplay.h"
#include "RageTextureManager.h"
#include "RageDisplay.h"
#include "StageStats.h"
#include "Foreach.h"
#include "song.h"

//#define DIVIDE_LINE_WIDTH			THEME->GetMetricI(m_sName,"TexturedBottomHalf")

GraphDisplay::GraphDisplay()
{
	m_pTexture = NULL;

	m_iFFCPoint = -1;
	m_iFECPoint = -1;
	m_iFGCPoint = -1;
	m_bColorize = false;
}


void GraphDisplay::Load( const CString &TexturePath, float fInitialHeight, const CString &sJustBarelyPath, const bool bColorize, 
	const CString sTextureFFC, const CString sTextureFEC, const CString sTextureFGC )
{
	m_Position = 1;
	memset( m_CurValues, 0, sizeof(m_CurValues) );
	memset( m_DestValues, 0, sizeof(m_DestValues) );
	memset( m_LastValues, 0, sizeof(m_LastValues) );

	Unload();
	m_pTexture = TEXTUREMAN->LoadTexture( TexturePath );
	m_size.x = (float) m_pTexture->GetSourceWidth();
	m_size.y = (float) m_pTexture->GetSourceHeight();
	m_bColorize = bColorize;

	if ( !IsAFile(sTextureFFC) || !IsAFile(sTextureFEC) || !IsAFile(sTextureFGC) )
		m_bColorize = false;

	if (m_bColorize)
	{
		m_pTextureFFC = TEXTUREMAN->LoadTexture( sTextureFFC );
		m_pTextureFEC = TEXTUREMAN->LoadTexture( sTextureFEC );
		m_pTextureFGC = TEXTUREMAN->LoadTexture( sTextureFGC );
	}

	for( int i = 0; i < VALUE_RESOLUTION; ++i )
		m_CurValues[i] = fInitialHeight;
	UpdateVerts();

	m_sprJustBarely.Load( sJustBarelyPath );
}

void GraphDisplay::Unload()
{
	if( m_pTexture != NULL )
		TEXTUREMAN->UnloadTexture( m_pTexture );

	if (m_bColorize)
	{
		TEXTUREMAN->UnloadTexture( m_pTextureFFC );
		TEXTUREMAN->UnloadTexture( m_pTextureFEC );
		TEXTUREMAN->UnloadTexture( m_pTextureFGC );
		m_pTextureFFC = NULL;
		m_pTextureFEC = NULL;
		m_pTextureFGC = NULL;
	}

	m_pTexture = NULL;

	FOREACH( AutoActor*, m_vpSongBoundaries, p )
		SAFE_DELETE( *p );
	m_vpSongBoundaries.clear();

	m_sprJustBarely.Unload();

	ActorFrame::RemoveAllChildren();
}

void GraphDisplay::LoadFromStageStats( const StageStats &ss, const PlayerStageStats &pss, const CString &sSongBoundaryPath )
{
	float fTotalStepSeconds = ss.GetTotalPossibleStepsSeconds();

	memcpy( m_LastValues, m_CurValues, sizeof(m_CurValues) );
	m_Position = 0;
	pss.GetLifeRecord( m_DestValues, VALUE_RESOLUTION, ss.GetTotalPossibleStepsSeconds() );
	for( unsigned i=0; i<ARRAYLEN(m_DestValues); i++ )
		CLAMP( m_DestValues[i], 0.f, 1.f );

	if ( m_bColorize )
	{
		if ( pss.bFlag_FFC && pss.fFullFantasticComboBegin > -1.0f )
			m_iFFCPoint = (int)(( pss.fFullFantasticComboBegin * VALUE_RESOLUTION ) / fTotalStepSeconds);
		if ( pss.bFlag_FEC && pss.fFullExcellentComboBegin > -1.0f )
			m_iFECPoint = (int)(( pss.fFullExcellentComboBegin * VALUE_RESOLUTION ) / fTotalStepSeconds);
		if ( pss.bFlag_FGC && pss.fFullGreatComboBegin > -1.0f )
			m_iFGCPoint = (int)(( pss.fFullGreatComboBegin * VALUE_RESOLUTION ) / fTotalStepSeconds);
		if ( pss.bFlag_PulsateEnd )
			m_iPulseStopPoint = (int)(( pss.fPulsatingComboEnd * VALUE_RESOLUTION ) / fTotalStepSeconds);
		else
			m_iPulseStopPoint = VALUE_RESOLUTION - 1;
	}

	UpdateVerts();

	//
	// Show song boundaries
	//
	float fSec = 0;
	FOREACH_CONST( Song*, ss.vpPossibleSongs, song )
	{
		if( song == ss.vpPossibleSongs.end()-1 )
			continue;
		fSec += (*song)->GetStepsSeconds();

		AutoActor *p = new AutoActor;
		m_vpSongBoundaries.push_back( p );
		p->Load( sSongBoundaryPath );
		float fX = SCALE( fSec, 0, fTotalStepSeconds, m_quadVertices.left, m_quadVertices.right );
		(*p)->SetX( fX );
		this->AddChild( *p );
	}

	if( !pss.bFailed && !pss.bGaveUp )
	{
		//
		// Search for the min life record to show "Just Barely!"
		//
		float fMinLifeSoFar = 1.0f;
		int iMinLifeSoFarAt = 0;

		int NumSlices = VALUE_RESOLUTION-1;
		for( int i = 0; i < NumSlices; ++i )
		{
			float fLife = m_DestValues[i];
			if( fLife < fMinLifeSoFar )
			{
				fMinLifeSoFar = fLife;
				iMinLifeSoFarAt = i;
			}
		}

		if( fMinLifeSoFar > 0.0f  &&  fMinLifeSoFar < 0.1f )
		{
			float fX = SCALE( float(iMinLifeSoFarAt), 0.0f, float(NumSlices), m_quadVertices.left, m_quadVertices.right );
			m_sprJustBarely->SetXY( fX, 0 );
		}
		else
		{
			m_sprJustBarely->SetHidden( true );
		}
		this->AddChild( m_sprJustBarely );
	}
}

void GraphDisplay::UpdateVerts()
{
	switch( m_HorizAlign )
	{
	case align_left:	m_quadVertices.left = 0;			m_quadVertices.right = m_size.x;		break;
	case align_center:	m_quadVertices.left = -m_size.x/2;	m_quadVertices.right = m_size.x/2;	break;
	case align_right:	m_quadVertices.left = -m_size.x;	m_quadVertices.right = 0;			break;
	default:		ASSERT( false );
	}

	switch( m_VertAlign )
	{
	case align_top:		m_quadVertices.top = 0;				m_quadVertices.bottom = m_size.y;	break;
	case align_middle:	m_quadVertices.top = -m_size.y/2;	m_quadVertices.bottom = m_size.y/2;	break;
	case align_bottom:	m_quadVertices.top = -m_size.y;		m_quadVertices.bottom = 0;			break;
	default:		ASSERT(0);
	}

	int NumSlices = VALUE_RESOLUTION-1;

	for( int i = 0; i < 4*NumSlices; ++i )
		m_Slices[i].c = RageColor(1,1,1,1);

	if (m_bColorize)
	{
		for( int i = 0; i < 4*NumSlices; ++i )
		{
			m_SlicesFFC[i].c = RageColor(1,1,1,1);
			m_SlicesFEC[i].c = RageColor(1,1,1,1);
			m_SlicesFGC[i].c = RageColor(1,1,1,1);
		}
	}

	// XXX: disgusting   p.s.: overlap  --infamouspat
	int iRangeFFC[2], iRangeFEC[2], iRangeFGC[2];
#define COMPARE_POINT(xx) (xx == -1 ? NumSlices : xx)
	if (m_bColorize)
	{
		iRangeFFC[0] = 0;
		iRangeFFC[1] = min( COMPARE_POINT(m_iFECPoint), min(COMPARE_POINT(m_iFGCPoint),COMPARE_POINT(m_iPulseStopPoint)) );

		iRangeFEC[0] = iRangeFFC[1];
		iRangeFEC[1] = min( COMPARE_POINT(m_iFGCPoint),COMPARE_POINT(m_iPulseStopPoint) );

		iRangeFGC[0] = iRangeFEC[1];
		iRangeFGC[1] = m_iPulseStopPoint;
	}
	else
	{
		iRangeFFC[0] = iRangeFFC[1] = 0;
		iRangeFEC[0] = iRangeFEC[1] = 0;
		iRangeFGC[0] = iRangeFGC[1] = 0;
	}
#undef COMPARE_POINT

	for( int i = 0; i < NumSlices; ++i )
	{
		const float Left = SCALE( float(i), 0.0f, float(NumSlices), m_quadVertices.left, m_quadVertices.right );
		const float Right = SCALE( float(i+1), 0.0f, float(NumSlices), m_quadVertices.left, m_quadVertices.right );
		const float LeftTop = SCALE( float(m_CurValues[i]), 0.0f, 1.0f, m_quadVertices.bottom, m_quadVertices.top );
		const float RightTop = SCALE( float(m_CurValues[i+1]), 0.0f, 1.0f, m_quadVertices.bottom, m_quadVertices.top );
		const float Zero = SCALE( 0.0f, 0.0f, 1.0f, m_quadVertices.bottom, m_quadVertices.top );

		// recall: m_iFFCPoint for the time being will always be 0
		if ( i >= iRangeFFC[0] && i < iRangeFFC[1] && m_bColorize)
		{
			m_SlicesFFC[i*4+0].p = RageVector3( Left,		LeftTop,	0 );	// top left
			m_SlicesFFC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFFC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFFC[i*4+3].p = RageVector3( Right,		RightTop,	0 );	// top right
			
			m_SlicesFEC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFEC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFEC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFEC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

			m_SlicesFGC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFGC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFGC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFGC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

			m_Slices[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_Slices[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_Slices[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_Slices[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

		}
		else if (i >= iRangeFEC[0] && i < iRangeFEC[1] && m_bColorize)
		{
			m_SlicesFFC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFFC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFFC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFFC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right
			
			m_SlicesFEC[i*4+0].p = RageVector3( Left,		LeftTop,	0 );	// top left
			m_SlicesFEC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFEC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFEC[i*4+3].p = RageVector3( Right,		RightTop,	0 );	// top right

			m_SlicesFGC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFGC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFGC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFGC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

			m_Slices[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_Slices[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_Slices[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_Slices[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

		}
		else if (i >= iRangeFGC[0] && i < iRangeFGC[1] && m_bColorize)
		{
			m_SlicesFFC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFFC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFFC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFFC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right
			
			m_SlicesFEC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_SlicesFEC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFEC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFEC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

			m_SlicesFGC[i*4+0].p = RageVector3( Left,		LeftTop,	0 );	// top left
			m_SlicesFGC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_SlicesFGC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_SlicesFGC[i*4+3].p = RageVector3( Right,		RightTop,	0 );	// top right

			m_Slices[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
			m_Slices[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_Slices[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_Slices[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

		}
		else 
		{
			if (m_bColorize)
			{
				m_SlicesFFC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
				m_SlicesFFC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
				m_SlicesFFC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
				m_SlicesFFC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right
			
				m_SlicesFEC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
				m_SlicesFEC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
				m_SlicesFEC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
				m_SlicesFEC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right

				m_SlicesFGC[i*4+0].p = RageVector3( Left,		Zero,	0 );	// top left
				m_SlicesFGC[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
				m_SlicesFGC[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
				m_SlicesFGC[i*4+3].p = RageVector3( Right,		Zero,	0 );	// top right
			}

			m_Slices[i*4+0].p = RageVector3( Left,		LeftTop,	0 );	// top left
			m_Slices[i*4+1].p = RageVector3( Left,		m_quadVertices.bottom,	0 );	// bottom left
			m_Slices[i*4+2].p = RageVector3( Right,		m_quadVertices.bottom,	0 );	// bottom right
			m_Slices[i*4+3].p = RageVector3( Right,		RightTop,	0 );	// top right
		}
	}

	const RectF *tex = m_pTexture->GetTextureCoordRect( 0 );
	for( unsigned i = 0; i < ARRAYLEN(m_Slices); ++i )
	{
		m_Slices[i].t = RageVector2( 
			SCALE( m_Slices[i].p.x, m_quadVertices.left, m_quadVertices.right, tex->left, tex->right ),
			SCALE( m_Slices[i].p.y, m_quadVertices.top, m_quadVertices.bottom, tex->top, tex->bottom )
			);
	}

	if (m_bColorize)
	{
		for( unsigned i = 0; i < ARRAYLEN(m_Slices); ++i )
		{
			m_SlicesFFC[i].t = RageVector2( 
				SCALE( m_SlicesFFC[i].p.x, m_quadVertices.left, m_quadVertices.right, tex->left, tex->right ),
				SCALE( m_SlicesFFC[i].p.y, m_quadVertices.top, m_quadVertices.bottom, tex->top, tex->bottom )
				);
		}
		for( unsigned i = 0; i < ARRAYLEN(m_Slices); ++i )
		{
			m_SlicesFEC[i].t = RageVector2( 
				SCALE( m_SlicesFEC[i].p.x, m_quadVertices.left, m_quadVertices.right, tex->left, tex->right ),
				SCALE( m_SlicesFEC[i].p.y, m_quadVertices.top, m_quadVertices.bottom, tex->top, tex->bottom )
				);
		}
		for( unsigned i = 0; i < ARRAYLEN(m_Slices); ++i )
		{
			m_SlicesFGC[i].t = RageVector2( 
				SCALE( m_SlicesFGC[i].p.x, m_quadVertices.left, m_quadVertices.right, tex->left, tex->right ),
				SCALE( m_SlicesFGC[i].p.y, m_quadVertices.top, m_quadVertices.bottom, tex->top, tex->bottom )
				);
		}
	}
}

void GraphDisplay::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	if( m_Position == 1 )
		return;

	m_Position = clamp( m_Position+fDeltaTime, 0, 1 );
	for( int i = 0; i < VALUE_RESOLUTION; ++i )
		m_CurValues[i] = m_DestValues[i]*m_Position + m_LastValues[i]*(1-m_Position);

	UpdateVerts();
}

void GraphDisplay::DrawPrimitives()
{
	Actor::SetGlobalRenderStates();	// set Actor-specified render states

	DISPLAY->ClearAllTextures();

	if (m_bColorize)
	{
		DISPLAY->SetTexture( 0, m_pTextureFFC );
		DISPLAY->DrawQuads( m_SlicesFFC, ARRAYLEN(m_SlicesFFC) );

		DISPLAY->SetTexture( 0, m_pTextureFEC );
		DISPLAY->DrawQuads( m_SlicesFEC, ARRAYLEN(m_SlicesFEC) );

		DISPLAY->SetTexture( 0, m_pTextureFGC );
		DISPLAY->DrawQuads( m_SlicesFGC, ARRAYLEN(m_SlicesFGC) );
	}

	DISPLAY->SetTexture( 0, m_pTexture );
	DISPLAY->DrawQuads( m_Slices, ARRAYLEN(m_Slices) );

	DISPLAY->SetTexture( 0, NULL );

	ActorFrame::DrawPrimitives();
}

/*
 * (c) 2003 Glenn Maynard
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
