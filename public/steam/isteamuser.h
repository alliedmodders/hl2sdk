//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: interface to user account information in Steam
//
//=============================================================================

#ifndef ISTEAMUSER_H
#define ISTEAMUSER_H
#ifdef _WIN32
#pragma once
#endif


// structure that contains client callback data
struct CallbackMsg_t
{
	HSteamUser m_hSteamUser;
	int m_iCallback;
	uint8 *m_pubParam;
	int m_cubParam;
};

// reference to a steam call, to filter results by
typedef int32 HSteamCall;

// C API bindings for GoldSRC, see isteamclient.h for details

extern "C"
{	
// C functions we export for the C API, maps to ISteamUser functions 
DLL_EXPORT void Steam_LogOn( HSteamUser hUser, HSteamPipe hSteamPipe, uint64 ulSteamID );
DLL_EXPORT void Steam_LogOff( HSteamUser hUser, HSteamPipe hSteamPipe );
DLL_EXPORT bool Steam_BLoggedOn( HSteamUser hUser, HSteamPipe hSteamPipe );
DLL_EXPORT bool Steam_BConnected( HSteamUser hUser, HSteamPipe hSteamPipe );
DLL_EXPORT bool Steam_BGetCallback( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall );
DLL_EXPORT void Steam_FreeLastCallback( HSteamPipe hSteamPipe );
DLL_EXPORT int Steam_GSGetSteamGameConnectToken( HSteamUser hUser, HSteamPipe hSteamPipe, void *pBlob, int cbBlobMax );
DLL_EXPORT int Steam_InitiateGameConnection( HSteamUser hUser, HSteamPipe hSteamPipe, void *pBlob, int cbMaxBlob, uint64 steamID, int nGameAppID, uint32 unIPServer, uint16 usPortServer, bool bSecure );
DLL_EXPORT void Steam_TerminateGameConnection( HSteamUser hUser, HSteamPipe hSteamPipe, uint32 unIPServer, uint16 usPortServer );


typedef bool (*PFNSteam_BGetCallback)( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall );
typedef void (*PFNSteam_FreeLastCallback)( HSteamPipe hSteamPipe );
}


//-----------------------------------------------------------------------------
// Purpose: types of VAC bans
//-----------------------------------------------------------------------------
enum EVACBan
{
	k_EVACBanGoldsrc,
	k_EVACBanSource,
	k_EVACBanDayOfDefeatSource,
};

enum ELogonState
{
	k_ELogonStateNotLoggedOn = 0,
	k_ELogonStateLoggingOn = 1,
	k_ELogonStateLoggingOff = 2,
	k_ELogonStateLoggedOn = 3
};

enum ERegistrySubTree
{
	k_ERegistrySubTreeNews = 0,
	k_ERegistrySubTreeApps = 1,
	k_ERegistrySubTreeSubscriptions = 2,
	k_ERegistrySubTreeGameServers = 3,
	k_ERegistrySubTreeFriends = 4,
	k_ERegistrySubTreeSystem = 5,
};


//-----------------------------------------------------------------------------
// Purpose: Functions for accessing and manipulating a steam account
//			associated with one client instance
//-----------------------------------------------------------------------------
class ISteamUser
{
public:
	// returns the HSteamUser this interface represents
	virtual HSteamUser GetHSteamUser() = 0;

	// steam account management functions
	virtual void LogOn( CSteamID steamID ) = 0;
	virtual void LogOff() = 0;
	virtual bool BLoggedOn() = 0;
	virtual ELogonState GetLogonState() = 0;
	virtual bool BConnected() = 0;
	virtual CSteamID GetSteamID() = 0;

	// account state

	// returns true if this account is VAC banned from the specified ban set
	virtual bool IsVACBanned( EVACBan eVACBan ) = 0;

	// returns true if the user needs to see the newly-banned message from the specified ban set
	virtual bool RequireShowVACBannedMessage( EVACBan eVACBan ) = 0;

	// tells the server that the user has seen the 'you have been banned' dialog
	virtual void AcknowledgeVACBanning( EVACBan eVACBan ) = 0;

	// registering/unregistration game launches functions
	// unclear as to where these should live
	// These are dead.
	virtual int NClientGameIDAdd( int nGameID ) = 0;
	virtual void RemoveClientGame( int nClientGameID )  = 0;
	virtual void SetClientGameServer( int nClientGameID, uint32 unIPServer, uint16 usPortServer ) = 0;

	// steam2 stuff
	virtual void SetSteam2Ticket( uint8 *pubTicket, int cubTicket ) = 0;
	virtual void AddServerNetAddress( uint32 unIP, uint16 unPort ) = 0;

	// email address setting
	virtual bool SetEmail( const char *pchEmail ) = 0;

	// logon cookie - this is obsolete
	virtual int Obsolete_GetSteamGameConnectToken( void *pBlob, int cbMaxBlob ) = 0;

	// persist per user data
	virtual bool SetRegistryString( ERegistrySubTree eRegistrySubTree, const char *pchKey, const char *pchValue ) = 0;
	virtual bool GetRegistryString( ERegistrySubTree eRegistrySubTree, const char *pchKey, char *pchValue, int cbValue ) = 0;
	virtual bool SetRegistryInt( ERegistrySubTree eRegistrySubTree, const char *pchKey, int iValue ) = 0;
	virtual bool GetRegistryInt( ERegistrySubTree eRegistrySubTree, const char *pchKey, int *piValue ) = 0;

	// notify of connection to game server
	virtual int InitiateGameConnection( void *pBlob, int cbMaxBlob, CSteamID steamID, int nGameAppID, uint32 unIPServer, uint16 usPortServer, bool bSecure ) = 0;
	// notify of disconnect
	virtual void TerminateGameConnection( uint32 unIPServer, uint16 usPortServer ) = 0;

	// controls where chat messages go to - puts the caller on top of the stack of chat destinations
	virtual void SetSelfAsPrimaryChatDestination() = 0;
    // returns true if the current caller is the one that should open new chat dialogs
	virtual bool IsPrimaryChatDestination() = 0;
};


// callbacks
enum {	k_iSteamUserCallbacks = 100 };

//-----------------------------------------------------------------------------
// Purpose: called when a logon attempt has succeeded
//-----------------------------------------------------------------------------
struct LogonSuccess_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 1 };
};


//-----------------------------------------------------------------------------
// Purpose: called when a logon attempt has failed
//-----------------------------------------------------------------------------
struct LogonFailure_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 2 };
	EResult m_eResult;
};


//-----------------------------------------------------------------------------
// Purpose: called when the user logs off
//-----------------------------------------------------------------------------
struct LoggedOff_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 3 };
	EResult m_eResult;
};


//-----------------------------------------------------------------------------
// Purpose: called when the client is trying to retry logon after being unintentionally logged off
//-----------------------------------------------------------------------------
struct BeginLogonRetry_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 4 };
};


//-----------------------------------------------------------------------------
// Purpose: called when the steam2 ticket has been set
//-----------------------------------------------------------------------------
struct Steam2TicketChanged_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 6 };
};


//-----------------------------------------------------------------------------
// Purpose: called when app news update is recieved
//-----------------------------------------------------------------------------
struct ClientAppNewsItemUpdate_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 10 };

	uint8 m_eNewsUpdateType;	// one of ENewsUpdateType
	uint32 m_uNewsID;			// unique news post ID
	uint32 m_uAppID;			// app ID this update applies to if it is of type k_EAppNews
};


//-----------------------------------------------------------------------------
// Purpose: steam news update
//-----------------------------------------------------------------------------
struct ClientSteamNewsItemUpdate_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 12 };

	uint8 m_eNewsUpdateType;	// one of ENewsUpdateType
	uint32 m_uNewsID;			// unique news post ID
	uint32 m_uHaveSubID;		// conditions to control if we display this update for type k_ESteamNews
	uint32 m_uNotHaveSubID;
	uint32 m_uHaveAppID;
	uint32 m_uNotHaveAppID;
	uint32 m_uHaveAppIDInstalled;
	uint32 m_uHavePlayedAppID;
};


//-----------------------------------------------------------------------------
// Purpose: connect to game server denied
//-----------------------------------------------------------------------------
struct ClientGameServerDeny_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 13 };

	uint32 m_uAppID;
	uint32 m_unGameServerIP;
	uint16 m_usGameServerPort;
	uint16 m_bSecure;
	uint32 m_uReason;
};


//-----------------------------------------------------------------------------
// Purpose: notifies the user that they are now the primary access point for chat messages
//-----------------------------------------------------------------------------
struct PrimaryChatDestinationSet_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 14 };
	uint8 m_bIsPrimary;
};


//-----------------------------------------------------------------------------
// Purpose: connect to game server denied
//-----------------------------------------------------------------------------
struct GSPolicyResponse_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 15 };
	uint8 m_bSecure;
};


//-----------------------------------------------------------------------------
// Purpose: steam cddb/bootstrapper update
//-----------------------------------------------------------------------------
struct ClientSteamNewsClientUpdate_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 16 };

	uint8 m_eNewsUpdateType;	// one of ENewsUpdateType
	uint8 m_bReloadCDDB; // if true there is a new CDDB available
	uint32 m_unCurrentBootstrapperVersion;
	uint32 m_unCurrentClientVersion;
};


//-----------------------------------------------------------------------------
// Purpose: called when the callback system for this client is in an error state (and has flushed pending callbacks)
//			When getting this message the client should disconnect from Steam, reset any stored Steam state and reconnect
//-----------------------------------------------------------------------------
struct CallbackPipeFailure_t
{
	enum { k_iCallback = k_iSteamUserCallbacks + 17 };
};


#define STEAMUSER_INTERFACE_VERSION "SteamUser004"

#endif // ISTEAMUSER_H
