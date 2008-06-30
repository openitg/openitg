#include "global.h"
#include "RageLog.h"
#include "TournamentManager.h"

TournamentManager* TOURNAMENT = NULL;

TournamentManager::TournamentManager()
{
	LOG->Debug( "TournamentManager::TournamentManager()" );
}

TournamentManager::~TournamentManager()
{
	LOG->Debug( "TournamentManager::~TournamentManager()" );
}

bool TournamentManager::IsTournamentMode()
{
	LOG->Debug( "TournamentManager::IsTournamentMode()" );

	return false;
}
