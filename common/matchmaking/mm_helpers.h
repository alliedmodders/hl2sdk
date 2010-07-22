//===== Copyright © 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: common routines to operate on matchmaking sessions and members
// Assumptions: caller should include all required headers before including mm_helpers.h
//
//===========================================================================//

#ifndef __COMMON__MM_HELPERS_H_
#define __COMMON__MM_HELPERS_H_
#ifdef _WIN32
#pragma once
#endif

#include "tier1/keyvalues.h"
#include "tier1/fmtstr.h"


//
// Contains inline functions to deal with common tasks involving matchmaking and sessions
//

inline KeyValues * SessionMembersFindPlayer( KeyValues *pSessionSettings, XUID xuidPlayer, KeyValues **ppMachine = NULL )
{
	if ( ppMachine )
		*ppMachine = NULL;

	if ( !pSessionSettings )
		return NULL;

	KeyValues *pMembers = pSessionSettings->FindKey( "Members" );
	if ( !pMembers )
		return NULL;

	int numMachines = pMembers->GetInt( "numMachines" );
	for ( int k = 0; k < numMachines; ++ k )
	{
		KeyValues *pMachine = pMembers->FindKey( CFmtStr( "machine%d", k ) );
		if ( !pMachine )
			continue;

		int numPlayers = pMachine->GetInt( "numPlayers" );
		for ( int j = 0; j < numPlayers; ++ j )
		{
			KeyValues *pPlayer = pMachine->FindKey( CFmtStr( "player%d", j ) );
			if ( !pPlayer )
				continue;

			if ( pPlayer->GetUint64( "xuid" ) == xuidPlayer )
			{
				if ( ppMachine )
					*ppMachine = pMachine;

				return pPlayer;
			}
		}
	}

	return NULL;
}

inline XUID SessionMembersFindNonGuestXuid( XUID xuid )
{
#ifdef _X360
	if ( !g_pMatchFramework )
		return xuid;

	if ( !g_pMatchFramework->GetMatchSession() )
		return xuid;

	KeyValues *pMachine = NULL;
	KeyValues *pPlayer = SessionMembersFindPlayer( g_pMatchFramework->GetMatchSession()->GetSessionSettings(), xuid, &pMachine );
	if ( !pPlayer || !pMachine )
		return xuid;

	if ( !strchr( pPlayer->GetString( "name" ), '(' ) )
		return xuid;

	int numPlayers = pMachine->GetInt( "numPlayers" );
	for ( int k = 0; k < numPlayers; ++ k )
	{
		XUID xuidOtherPlayer = pMachine->GetUint64( CFmtStr( "player%d/xuid", k ) );
		if ( xuidOtherPlayer && !strchr( pMachine->GetString( CFmtStr( "player%d/xuid", k ) ), '(' ) )
			return xuidOtherPlayer;	// found a replacement that is not guest
	}
#endif

	return xuid;
}


#endif // __COMMON__MM_HELPERS_H_

