//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ISTEAMCLIENT_H
#define ISTEAMCLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include "steamtypes.h"
#include "steamclientpublic.h"

// handle to a communication pipe to the Steam client
typedef int32 HSteamPipe;
// handle to single instance of a steam user
typedef int32 HSteamUser;

#ifndef DLL_EXPORT
#define DLL_EXPORT 
#endif

// interface predec
class ISteamUser;
class ISteamGameServer;
class ISteamFriends;
class ISteamBilling;
class ISteamUtils;
class IVAC;


//-----------------------------------------------------------------------------
// Purpose: Interface to creating a new steam instance, or to
//			connect to an existing steam instance, whether it's in a
//			different process or is local
//-----------------------------------------------------------------------------
class ISteamClient
{
public:
	// Creates a communication pipe to the Steam client
	virtual HSteamPipe CreateSteamPipe() = 0;

	// Releases a previously created communications pipe
	virtual bool BReleaseSteamPipe( HSteamPipe hSteamPipe ) = 0;

	// creates a global instance of a steam user, so that other processes can share it
	// used by the steam UI, to share it's account info/connection with any games it launches
	// fails (returns NULL) if an existing instance already exists
	virtual HSteamUser CreateGlobalUser( HSteamPipe *phSteamPipe ) = 0;

	// connects to an existing global user, failing if none exists
	// used by the game to coordinate with the steamUI
	virtual HSteamUser ConnectToGlobalUser( HSteamPipe hSteamPipe ) = 0;

	// used by game servers, create a steam user that won't be shared with anyone else
	virtual HSteamUser CreateLocalUser( HSteamPipe *phSteamPipe ) = 0;

	// removes an allocated user
	virtual void ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

	// retrieves the ISteamUser interface associated with the handle
	virtual ISteamUser *GetISteamUser( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// retrieves the IVac interface associated with the handle
	// there is normally only one instance of VAC running, but using this connects it to the right user/account
	virtual IVAC *GetIVAC( HSteamUser hSteamUser ) = 0;

	// retrieves the ISteamGameServer interface associated with the handle
	virtual ISteamGameServer *GetISteamGameServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// set the local IP and Port to bind to
	// this must be set before CreateLocalUser()
	virtual void SetLocalIPBinding( uint32 unIP, uint16 usPort ) = 0; 

	// returns the name of a universe
	virtual const char *GetUniverseName( EUniverse eUniverse ) = 0;

	// returns the ISteamFriends interface
	virtual ISteamFriends *GetISteamFriends( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamUtils interface
	virtual ISteamUtils *GetISteamUtils( HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamBilling interface
	virtual ISteamBilling *GetISteamBilling( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;
};

#define STEAMCLIENT_INTERFACE_VERSION		"SteamClient006"


// For GoldSRC we need a C API as the C++ ABI changed from the GoldSRC compiler 
// (GCC 2.95.3) to the Source/Steam3 one (GCC 3.4.1)
// C functions we export for the C API, maps to ISteamClient functions 
DLL_EXPORT HSteamPipe Steam_CreateSteamPipe();
DLL_EXPORT bool Steam_BReleaseSteamPipe( HSteamPipe hSteamPipe );
DLL_EXPORT HSteamUser Steam_CreateLocalUser( HSteamPipe *phSteamPipe );
DLL_EXPORT HSteamUser Steam_CreateGlobalUser( HSteamPipe *phSteamPipe );
DLL_EXPORT HSteamUser Steam_ConnectToGlobalUser( HSteamPipe hSteamPipe );
DLL_EXPORT void Steam_ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser );
DLL_EXPORT void Steam_SetLocalIPBinding( uint32 unIP, uint16 usLocalPort );


#endif // ISTEAMCLIENT_H
