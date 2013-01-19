#include "global.h"
#include "ActorFrame.h"
#include "RageLog.h"
#include "XmlFile.h"
#include "ActorUtil.h"
#include "RageDisplay.h"
#include "ScreenDimensions.h"

#if defined(DEBUG)
#include "arch/Dialog/Dialog.h"
#endif

// lua start
LUA_REGISTER_CLASS( ActorFrame )
// lua end

/* Tricky: We need ActorFrames created in XML to auto delete their children.
 * We don't want classes that derive from ActorFrame to auto delete their 
 * children.  The name "ActorFrame" is widely used in XML, so we'll have
 * that string instead create an ActorFrameAutoDeleteChildren object.
 */
//REGISTER_ACTOR_CLASS( ActorFrame )
REGISTER_ACTOR_CLASS_WITH_NAME( ActorFrameAutoDeleteChildren, ActorFrame )


ActorFrame::ActorFrame()
{
	m_bPropagateCommands = false;
	m_bDeleteChildren = false;
	m_bDrawByZPosition = false;
	m_UpdateFunction.Unset();
	m_fUpdateRate = 1;
	m_fFOV = -1;
	m_fVanishX = SCREEN_CENTER_X;
	m_fVanishY = SCREEN_CENTER_Y;
	m_bOverrideLighting = false;
	m_bLighting = false;
}

ActorFrame::~ActorFrame()
{
	if( m_bDeleteChildren )
		DeleteAllChildren();
}

void ActorFrame::SetUpdateFunction( const LuaReference &UpdateFunction, bool bOnStack )
{
	if( !bOnStack )
	{
		m_UpdateFunction = UpdateFunction;
		return;
	}

	Lua *L = LUA->Get();
	m_UpdateFunction.SetFromStack( L );
	LUA->Release( L );
}

void ActorFrame::LoadFromNode( const CString& sDir, const XNode* pNode )
{
	Actor::LoadFromNode( sDir, pNode );

	pNode->GetAttrValue( "UpdateRate", m_fUpdateRate );
	pNode->GetAttrValue( "FOV", m_fFOV );
	
	CString s;
	if( pNode->GetAttrValue( "VanishX", s ) )
	{
		LuaHelpers::PrepareExpression( s );
		m_fVanishX = LuaHelpers::RunExpressionF( s );
	}
	if( pNode->GetAttrValue( "VanishY", s ) )
	{
		LuaHelpers::PrepareExpression( s );
		m_fVanishY = LuaHelpers::RunExpressionF( s );
	}

	m_bOverrideLighting = pNode->GetAttrValue( "Lighting", m_bLighting );
}

void ActorFrame::LoadChildrenFromNode( const CString& sDir, const XNode* pNode )
{
	// Shoudn't be calling this unless we're going to delete our children.
	ASSERT( m_bDeleteChildren );

	//
	// Load children
	//
	const XNode* pChildren = pNode->GetChild("children");
	if( pChildren )
	{
		FOREACH_CONST_Child( pChildren, pChild )
		{
			Actor* pChildActor = ActorUtil::LoadFromActorFile( sDir, pChild );
			if( pChildActor )
				AddChild( pChildActor );
		}
		SortByDrawOrder();
	}
}

void ActorFrame::AddChild( Actor* pActor )
{
#if _DEBUG
	// check that this Actor isn't already added.
	vector<Actor*>::iterator iter = find( m_SubActors.begin(), m_SubActors.end(), pActor );
	if( iter != m_SubActors.end() )
		Dialog::OK( ssprintf("Actor \"%s\" adds child \"%s\" more than once", m_sName.c_str(), pActor->m_sName.c_str()) );
#endif

	ASSERT( pActor );
	ASSERT( (void*)pActor != (void*)0xC0000005 );
	m_SubActors.push_back( pActor );
}

void ActorFrame::RemoveChild( Actor* pActor )
{
	vector<Actor*>::iterator iter = find( m_SubActors.begin(), m_SubActors.end(), pActor );
	if( iter != m_SubActors.end() )
		m_SubActors.erase( iter );
}

Actor* ActorFrame::GetChild( const CString &sName )
{
	FOREACH( Actor*, m_SubActors, a )
	{
		if( (*a)->GetName() == sName )
			return *a;
	}
	return NULL;
}

void ActorFrame::RemoveAllChildren()
{
	m_SubActors.clear();
}

void ActorFrame::MoveToTail( Actor* pActor )
{
	vector<Actor*>::iterator iter = find( m_SubActors.begin(), m_SubActors.end(), pActor );
	if( iter == m_SubActors.end() )	// didn't find
	{
		ASSERT(0);	// called with a pActor that doesn't exist
		return;
	}

	m_SubActors.erase( iter );
	m_SubActors.push_back( pActor );
}

void ActorFrame::MoveToHead( Actor* pActor )
{
	vector<Actor*>::iterator iter = find( m_SubActors.begin(), m_SubActors.end(), pActor );
	if( iter == m_SubActors.end() )	// didn't find
	{
		ASSERT(0);	// called with a pActor that doesn't exist
		return;
	}

	m_SubActors.erase( iter );
	m_SubActors.insert( m_SubActors.begin(), pActor );
}


void ActorFrame::DrawPrimitives()
{
	ASSERT_M( !m_bClearZBuffer, "ClearZBuffer not supported on ActorFrames" );

	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPushMatrix();
		DISPLAY->LoadMenuPerspective( m_fFOV, m_fVanishX, m_fVanishY );
	}

	if( m_bOverrideLighting )
	{
		DISPLAY->SetLighting( m_bLighting );
		if( m_bLighting )
			DISPLAY->SetLightDirectional( 
				0, 
				RageColor(1,1,1,1), 
				RageColor(1,1,1,1),
				RageColor(1,1,1,1),
				RageVector3(0,0,1) );
	}



	// Don't set Actor-defined render states because we won't be drawing 
	// any geometry that belongs to this object.
	// Actor::DrawPrimitives();

	// draw all sub-ActorFrames while we're in the ActorFrame's local coordinate space
	if( m_bDrawByZPosition )
	{
		vector<Actor*> subs = m_SubActors;
		ActorUtil::SortByZPosition( subs );
		for( unsigned i=0; i<subs.size(); i++ )
			subs[i]->Draw();
	}
	else
	{
		for( unsigned i=0; i<m_SubActors.size(); i++ )
			m_SubActors[i]->Draw();
	}



	if( m_bOverrideLighting )
	{
		// TODO: pop state instead of turning lighting off
		DISPLAY->SetLightOff( 0 );
		DISPLAY->SetLighting( false );
	}

	if( m_fFOV != -1 )
	{
		DISPLAY->CameraPopMatrix();
	}
}

void ActorFrame::RunCommandsOnChildren( const LuaReference& cmds )
{
	for( unsigned i=0; i<m_SubActors.size(); i++ )
		m_SubActors[i]->RunCommands( cmds, this );
}

void ActorFrame::RunCommandsOnLeaves( const LuaReference& cmds, Actor* pParent )
{
	for( unsigned i=0; i<m_SubActors.size(); i++ )
		m_SubActors[i]->RunCommandsOnLeaves( cmds, this );
}

void ActorFrame::UpdateInternal( float fDeltaTime )
{
//	LOG->Trace( "ActorFrame::Update( %f )", fDeltaTime );

	fDeltaTime *= m_fUpdateRate;

	Actor::UpdateInternal( fDeltaTime );

	// update all sub-Actors
	for( vector<Actor*>::iterator it=m_SubActors.begin(); it!=m_SubActors.end(); it++ )
	{
		Actor *pActor = *it;
		pActor->Update(fDeltaTime);
	}

	// update Lua command, if any
	if( m_UpdateFunction.IsSet() )
	{
		Lua *L = LUA->Get();
		m_UpdateFunction.PushSelf( L );
		ASSERT( !lua_isnil(L, -1) );
		this->PushSelf( L );
		lua_pushnumber( L, fDeltaTime );
		CString sError;

		if( !LuaHelpers::RunScriptOnStack(L, sError, 2, 0) ) // 1 arg, 0 results
			LOG->Warn( "Error running m_UpdateFunction: %s", sError.c_str() );
		LUA->Release( L );
	}
	
}

#define PropagateActorFrameCommand( cmd ) \
	void ActorFrame::cmd()				\
	{									\
		Actor::cmd();					\
										\
		/* set all sub-Actors */		\
		for( unsigned i=0; i<m_SubActors.size(); i++ ) \
			m_SubActors[i]->cmd();		\
	}

#define PropagateActorFrameCommand1Param( cmd, type ) \
	void ActorFrame::cmd( type f )		\
	{									\
		Actor::cmd( f );				\
										\
		/* set all sub-Actors */		\
		for( unsigned i=0; i<m_SubActors.size(); i++ ) \
			m_SubActors[i]->cmd( f );	\
	}

PropagateActorFrameCommand( Reset )
PropagateActorFrameCommand( FinishTweening )
PropagateActorFrameCommand1Param( SetDiffuse,		RageColor )
PropagateActorFrameCommand1Param( SetZTestMode,		ZTestMode )
PropagateActorFrameCommand1Param( SetZWrite,		bool )
PropagateActorFrameCommand1Param( HurryTweening,	float )
PropagateActorFrameCommand1Param( SetDiffuseAlpha,	float )
PropagateActorFrameCommand1Param( SetBaseAlpha,		float )


float ActorFrame::GetTweenTimeLeft() const
{
	float m = Actor::GetTweenTimeLeft();

	for( unsigned i=0; i<m_SubActors.size(); i++ )
	{
		const Actor* pActor = m_SubActors[i];
		m = max(m, m_fHibernateSecondsLeft + pActor->GetTweenTimeLeft());
	}

	return m;

}

static bool CompareActorsByDrawOrder(const Actor *p1, const Actor *p2)
{
	return p1->GetDrawOrder() < p2->GetDrawOrder();
}

void ActorFrame::SortByDrawOrder()
{
	// Preserve ordering of Actors with equal DrawOrders.
	stable_sort( m_SubActors.begin(), m_SubActors.end(), CompareActorsByDrawOrder );
}

void ActorFrame::DeleteAllChildren()
{
	for( unsigned i=0; i<m_SubActors.size(); i++ )
		delete m_SubActors[i];
	m_SubActors.clear();
}

void ActorFrame::RunCommands( const LuaReference& cmds, Actor* pParent )
{
	if( m_bPropagateCommands )
		RunCommandsOnChildren( cmds );
	else
		Actor::RunCommands( cmds, pParent );
}

void ActorFrame::SetPropagateCommands( bool b )
{
	m_bPropagateCommands = b;
}

void ActorFrame::PlayCommand( const CString &sCommandName, Actor* pParent )
{
	// HACK: Don't propogate Init.  It gets called once for every Actor when the 
	// Actor is loaded, and we don't want to call it again.
	Actor::PlayCommand( sCommandName, pParent );

	if( sCommandName == "Init" )
		return;

	for( unsigned i=0; i<m_SubActors.size(); i++ ) 
	{
		Actor* pActor = m_SubActors[i];
		pActor->PlayCommand( sCommandName, this );
	}
}

void ActorFrame::SetDrawByZPosition( bool b )
{
	m_bDrawByZPosition = b;
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
