//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERNET_VARS_H
#define PLAYERNET_VARS_H
#ifdef _WIN32
#pragma once
#endif

#include "shared_classnames.h"

#define NUM_AUDIO_LOCAL_SOUNDS	8

// These structs are contained in each player's local data and shared by the client & server

struct fogparams_t
{
	DECLARE_CLASS_NOBASE( fogparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	CNetworkVar( bool, enable );
	CNetworkVar( bool, blend );
	CNetworkVector( dirPrimary );
	CNetworkColor32( colorPrimary );
	CNetworkColor32( colorSecondary );
	CNetworkVar( float, start );
	CNetworkVar( float, end );
	CNetworkVar( float, farz );

	CNetworkColor32( colorPrimaryLerpTo );
	CNetworkColor32( colorSecondaryLerpTo );
	CNetworkVar( float, startLerpTo );
	CNetworkVar( float, endLerpTo );
	CNetworkVar( float, lerptime );
	CNetworkVar( float, duration );
};

struct sky3dparams_t
{
	DECLARE_CLASS_NOBASE( sky3dparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	// 3d skybox camera data
	CNetworkVar( int, scale );
	CNetworkVector( origin );
	CNetworkVar( int, area );

	// 3d skybox fog data
	CNetworkVarEmbedded( fogparams_t, fog );
};

struct audioparams_t
{
	DECLARE_CLASS_NOBASE( audioparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	CNetworkArray( Vector, localSound, NUM_AUDIO_LOCAL_SOUNDS )
	CNetworkVar( int, soundscapeIndex );	// index of the current soundscape from soundscape.txt
	CNetworkVar( int, localBits );			// if bits 0,1,2,3 are set then position 0,1,2,3 are valid/used
	CNetworkHandle( CBaseEntity, ent );		// the entity setting the soundscape
};


#endif // PLAYERNET_VARS_H
