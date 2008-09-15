//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "terrainmodmgr.h"
#include "terrainmodmgr_shared.h"
#include "gameinterface.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void TerrainMod_Add( TerrainModType type, const CTerrainModParams &params )
{
	if ( IsXbox() )
	{
		return;
	}

	// Move players out of the way so they don't get stuck on the terrain.
	float playerStartHeights[MAX_PLAYERS];

	int i;
	int nPlayers = min( MAX_PLAYERS, gpGlobals->maxClients );
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;

		playerStartHeights[i] = pPlayer->GetAbsOrigin().z;

		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer, pPlayer->GetAbsOrigin(),
			pPlayer->GetAbsOrigin() + Vector( 0, 0, params.m_flRadius*2 ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
	
	// Apply the mod.
	engine->ApplyTerrainMod( type, params );
	
	// Move players back down.
	for( i=0; i < nPlayers; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i+1 );
	
		if( !pPlayer )
			continue;
		
		// Cast a ray upwards to see if we can move the player out of the way.
		trace_t trace;
		UTIL_TraceEntity( 
			pPlayer,
			pPlayer->GetAbsOrigin(),
			Vector( pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, playerStartHeights[i] ),
			MASK_SOLID,
			&trace );

		pPlayer->SetLocalOrigin( trace.endpos );
	}
}
