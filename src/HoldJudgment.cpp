#include "global.h"
#include "HoldJudgment.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "ThemeMetric.h"

ThemeMetric<apActorCommands>	OK_COMMAND	("HoldJudgment","OKCommand");
ThemeMetric<apActorCommands>	NG_COMMAND	("HoldJudgment","NGCommand");
ThemeMetric<apActorCommands>	OK_ODD_COMMAND	("HoldJudgment","OKOddCommand");
ThemeMetric<apActorCommands>	NG_ODD_COMMAND	("HoldJudgment","NGOddCommand");
ThemeMetric<apActorCommands>	OK_EVEN_COMMAND	("HoldJudgment","OKEvenCommand");
ThemeMetric<apActorCommands>	NG_EVEN_COMMAND	("HoldJudgment","NGEvenCommand");


HoldJudgment::HoldJudgment()
{
	m_iCount = 0;

	m_sprJudgment.Load( THEME->GetPathG("HoldJudgment","label 1x2") );
	m_sprJudgment.StopAnimating();
	Reset();
	this->AddChild( &m_sprJudgment );
}

void HoldJudgment::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );
}

void HoldJudgment::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}

void HoldJudgment::Reset()
{
	m_sprJudgment.SetDiffuse( RageColor(1,1,1,0) );
	m_sprJudgment.SetXY( 0, 0 );
	m_sprJudgment.StopTweening();
	m_sprJudgment.SetEffectNone();
}

void HoldJudgment::SetHoldJudgment( HoldNoteScore hns )
{
	//LOG->Trace( "Judgment::SetJudgment()" );

	Reset();

	switch( hns )
	{
	case HNS_NONE:
		ASSERT(0);
	case HNS_OK:
		m_sprJudgment.SetState( 0 );
		m_sprJudgment.RunCommands( (m_iCount%2) ? OK_ODD_COMMAND : OK_EVEN_COMMAND );
		m_sprJudgment.RunCommands( OK_COMMAND );
		break;
	case HNS_NG:
		m_sprJudgment.SetState( 1 );
		m_sprJudgment.RunCommands( (m_iCount%2) ? NG_ODD_COMMAND : NG_EVEN_COMMAND );
		m_sprJudgment.RunCommands( NG_COMMAND );
		break;
	default:
		ASSERT(0);
	}

	m_iCount++;
}

/*
 * (c) 2001-2004 Chris Danford
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
