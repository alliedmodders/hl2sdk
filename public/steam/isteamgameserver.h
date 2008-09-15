//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: interface to steam for game servers
//
//=============================================================================

#ifndef ISTEAMGAMESERVER_H
#define ISTEAMGAMESERVER_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: Functions for authenticating users via Steam to play on a game server
//-----------------------------------------------------------------------------
class ISteamGameServer
{
public:
	// connection functions
	virtual void LogOn() = 0;
	virtual void LogOff() = 0;
	virtual bool BLoggedOn() = 0;

	// user authentication functions
	virtual void GSSetSpawnCount( uint32 ucSpawn ) = 0;
	virtual bool GSGetSteam2GetEncryptionKeyToSendToNewClient( void *pvEncryptionKey, uint32 *pcbEncryptionKey, uint32 cbMaxEncryptionKey ) = 0;
	// the IP address and port should be in host order, i.e 127.0.0.1 == 0x7f000001
	virtual bool GSSendSteam2UserConnect(  uint32 unUserID, const void *pvRawKey, uint32 unKeyLen, uint32 unIPPublic, uint16 usPort, const void *pvCookie, uint32 cubCookie ) = 0; // Both Steam2 and Steam3 authentication
	// the IP address should be in host order, i.e 127.0.0.1 == 0x7f000001
	virtual bool GSSendSteam3UserConnect( CSteamID steamID, uint32 unIPPublic, const void *pvCookie, uint32 cubCookie ) = 0; // Steam3 only user auth
	virtual bool GSRemoveUserConnect( uint32 unUserID ) = 0;
	virtual bool GSSendUserDisconnect( CSteamID steamID, uint32 unUserID ) = 0;
	virtual bool GSSendUserStatusResponse( CSteamID steamID, int nSecondsConnected, int nSecondsSinceLast ) = 0;
	virtual bool Obsolete_GSSetStatus( int32 nAppIdServed, uint32 unServerFlags, int cPlayers, int cPlayersMax, int cBotPlayers, int unGamePort, 
		const char *pchServerName, const char *pchGameDir, const char *pchMapName, const char *pchVersion ) = 0;
	// Note that unGameIP is in host order
	virtual bool GSUpdateStatus( int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pchMapName ) = 0;
	virtual bool BSecure() = 0; 
	virtual CSteamID GetSteamID() = 0;
	virtual bool GSSetServerType( int32 nGameAppId, uint32 unServerFlags, uint32 unGameIP, uint32 unGamePort, const char *pchGameDir, const char *pchVersion ) = 0;
};

#define STEAMGAMESERVER_INTERFACE_VERSION "SteamGameServer002"

// game server flags
const uint32 k_unServerFlagNone			= 0x00;
const uint32 k_unServerFlagActive		= 0x01;
const uint32 k_unServerFlagSecure		= 0x02;
const uint32 k_unServerFlagDedicated	= 0x04;
const uint32 k_unServerFlagLinux		= 0x08;
const uint32 k_unServerFlagPassworded	= 0x10;


// callbacks
enum {	k_iSteamGameServerCallbacks = 200 };

// client has been approved to connect to this game server
struct GSClientApprove_t
{
	enum { k_iCallback = k_iSteamGameServerCallbacks + 1 };
	CSteamID m_SteamID;
};


// client has been denied to connection to this game server
struct GSClientDeny_t
{
	enum { k_iCallback = k_iSteamGameServerCallbacks + 2 };
	CSteamID m_SteamID;
	EDenyReason m_eDenyReason;
	char m_pchOptionalText[128];
};


// request the game server should kick the user
struct GSClientKick_t
{
	enum { k_iCallback = k_iSteamGameServerCallbacks + 3 };
	CSteamID m_SteamID;
	EDenyReason m_eDenyReason;
};

// client has been denied to connect to this game server because of a Steam2 auth failure
struct GSClientSteam2Deny_t
{
	enum { k_iCallback = k_iSteamGameServerCallbacks + 4 };
	uint32 m_UserID;
	uint32 m_eSteamError;
};


// client has been accepted by Steam2 to connect to this game server
struct GSClientSteam2Accept_t
{
	enum { k_iCallback = k_iSteamGameServerCallbacks + 5 };
	uint32 m_UserID;
	uint64 m_SteamID;
};


// C-API versions of the interface functions
DLL_EXPORT void *Steam_GetGSHandle( HSteamUser hUser, HSteamPipe hSteamPipe );
DLL_EXPORT bool Steam_GSSendSteam2UserConnect( void *phSteamHandle, uint32 unUserID, const void *pvRawKey, uint32 unKeyLen, uint32 unIPPublic, uint16 usPort, const void *pvCookie, uint32 cubCookie );
DLL_EXPORT bool Steam_GSSendSteam3UserConnect( void *phSteamHandle, uint64 ulSteamID, uint32 unIPPublic, const void *pvCookie, uint32 cubCookie );
DLL_EXPORT bool Steam_GSSendUserDisconnect( void *phSteamHandle, uint64 ulSteamID, uint32 unUserID );
DLL_EXPORT bool Steam_GSSendUserStatusResponse( void *phSteamHandle, uint64 ulSteamID, int nSecondsConnected, int nSecondsSinceLast );
DLL_EXPORT bool Steam_GSUpdateStatus( void *phSteamHandle, int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pchMapName );
DLL_EXPORT bool Steam_GSRemoveUserConnect( void *phSteamHandle, uint32 unUserID );
DLL_EXPORT void Steam_GSSetSpawnCount( void *phSteamHandle, uint32 ucSpawn );
DLL_EXPORT bool Steam_GSGetSteam2GetEncryptionKeyToSendToNewClient( void *phSteamHandle, void *pvEncryptionKey, uint32 *pcbEncryptionKey, uint32 cbMaxEncryptionKey );
DLL_EXPORT void Steam_GSLogOn( void *phSteamHandle );
DLL_EXPORT void Steam_GSLogOff( void *phSteamHandle );
DLL_EXPORT bool Steam_GSBLoggedOn( void *phSteamHandle );
DLL_EXPORT bool Steam_GSSetServerType( void *phSteamHandle, int32 nAppIdServed, uint32 unServerFlags, uint32 unGameIP, uint32 unGamePort, const char *pchGameDir, const char *pchVersion );
DLL_EXPORT bool Steam_GSBSecure( void *phSteamHandle );
DLL_EXPORT uint64 Steam_GSGetSteamID( void *phSteamHandle );

#endif // ISTEAMGAMESERVER_H
