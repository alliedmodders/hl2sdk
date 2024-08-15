//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ISERVER_H
#define ISERVER_H
#ifdef _WIN32
#pragma once
#endif

#include <inetmsghandler.h>
#include <edict.h>
#include <resourcefile/resourcetype.h>
#include <tier1/checksum_crc.h>
#include <engine/IEngineService.h>
#include <netadr.h>

class IGameSpawnGroupMgr;
struct EventServerAdvanceTick_t;
struct EventServerPollNetworking_t;
struct EventServerProcessNetworking_t;
struct EventServerSimulate_t;
struct EventServerPostSimulate_t;
struct server_state_t;
class IPrerequisite;
class CServerChangelevelState;
class ISource2WorldSession;
class INetworkGameClient;
class GameSessionConfiguration_t;
class KeyValues3;
class CSVCMsg_ServerInfo_t;
class CServerSideClientBase;
class CCLCMsg_SplitPlayerConnect_t;

typedef int ChallengeType_t;
typedef int PauseGroup_t;

abstract_class INetworkGameServer 
{
public:
	virtual	void	Init( const GameSessionConfiguration_t &, const char * ) = 0;

	virtual void	SetGameSpawnGroupMgr( IGameSpawnGroupMgr * ) = 0;
	virtual void	SetGameSessionManifest( HGameResourceManifest ) = 0;

	virtual void	RegisterLoadingSpawnGroups( CUtlVector<unsigned int> & ) = 0;

	virtual void	Shutdown( void ) = 0;
	virtual void	AddRef( void ) = 0;
	virtual void	Release ( void ) = 0;

	virtual CGlobalVars *GetGlobals(void) = 0;

	virtual bool	IsActive( void ) const = 0;	
	virtual bool	IsPaused( void ) const = 0;	

	virtual void	SetServerTick( int tick ) = 0;
	// returns game world tick
	virtual int		GetServerTick( void ) const = 0;

	// returns current client limit
	virtual int		GetMaxClients( void ) const = 0;

	virtual void	ServerAdvanceTick( const EventServerAdvanceTick_t & ) = 0;
	virtual void	ServerPollNetworking( const EventServerPollNetworking_t & ) = 0;
	virtual void	ServerProcessNetworking( const EventServerProcessNetworking_t & ) = 0;

	virtual void	ServerSimulate( const EventServerSimulate_t & ) = 0;
	virtual void	ServerPostSimulate( const EventServerPostSimulate_t & ) = 0;

	virtual void	LoadSpawnGroup( const SpawnGroupDesc_t & ) = 0;
	virtual void	AsyncUnloadSpawnGroup( unsigned int, /*ESpawnGroupUnloadOption*/ int ) = 0;
	virtual void	PrintSpawnGroupStatus( void ) const = 0;

	// returns the game time scale (multiplied in conjunction with host_timescale)
	virtual float	GetTimescale( void ) const = 0; 

	virtual bool	IsSaveRestoreAllowed( void ) const = 0;

	virtual void	SetMapName( const char *pszNewName ) = 0;
	// current map name (BSP)
	virtual const char *GetMapName( void ) const = 0;
	virtual const char *GetAddonName( void ) const = 0;

	virtual bool	IsBackgroundMap( void ) const = 0;

	// returns game world time (GetServerTick() * tick_interval)
	virtual float	GetTime( void ) const = 0;

	virtual bool	ActivateServer( void ) = 0;
	virtual bool	PrepareForAssetLoad( void ) = 0;

	virtual netadr_t GetServerNetworkAddress( void ) = 0;

	virtual SpawnGroupHandle_t FindSpawnGroupByName( const char *pszName ) = 0;
	virtual void	MakeSpawnGroupActive( SpawnGroupHandle_t ) = 0;
	virtual void	SynchronouslySpawnGroup( SpawnGroupHandle_t ) = 0;

	virtual void	SetServerState( server_state_t ) = 0;
	virtual void	SpawnServer( const char * ) = 0;

	virtual int 	GetSpawnGroupLoadingStatus( SpawnGroupHandle_t ) = 0;
	virtual void	SetSpawnGroupDescription( SpawnGroupHandle_t, const char * ) = 0;

	virtual CUtlVector<INetworkGameClient *> *StartChangeLevel( const char *, const char *pszLandmark, void * ) = 0;
	virtual void	FinishChangeLevel( CServerChangelevelState * ) = 0;
	virtual bool	IsChangelevelPending( void ) const = 0;

	virtual void	GetAllLoadingSpawnGroups( CUtlVector<SpawnGroupHandle_t> *pOut ) = 0;

	virtual void	PreserveSteamID( void ) = 0;

	virtual void	unk001() = 0;

	virtual void	ReserveServerForQueuedGame( const char *pszReason ) = 0;

	virtual void	unk101() = 0;
	virtual void	unk102() = 0;
	virtual void	unk103() = 0;

	virtual void	BroadcastPrintf( const char *pszFmt, ... ) FMTFUNCTION( 2, 3 ) = 0;

	virtual void	unk201() = 0;
	virtual void	unk202() = 0;
	
};

abstract_class CNetworkGameServerBase : public INetworkGameServer, protected IConnectionlessPacketHandler
{
public:
	virtual ~CNetworkGameServerBase() = 0;
	
	virtual void	SetMaxClients( int nMaxClients ) = 0;
	
	virtual void	unk301() = 0;
	virtual bool	ProcessConnectionlessPacket( const ns_address *addr, bf_read *bf ) = 0; // process a connectionless packet


	virtual CPlayerUserId GetPlayerUserId( CPlayerSlot slot ) = 0;
	virtual const char *GetPlayerNetworkIDString( CPlayerSlot slot ) = 0;
	
	// Returns udp port of this server instance
	virtual int		GetUDPPort() = 0;
	// Returns hostname of this server instance
	virtual const char *GetName() = 0;

	// AMNOTE: arg names are speculative and might be incorrect!
	// Sums up across all the connected players.
	virtual void	GetNetStats( float &inflow, float &outflow ) = 0;

	virtual void	FillKV3ServerInfo( KeyValues3 *out ) = 0;

	virtual void	unk401() = 0;
	
	virtual bool	IsPausable( PauseGroup_t ) = 0;

	// Returns sv_password cvar value, if it's set to "none" nullptr would be returned!
	virtual const char *GetPassword() = 0;

	virtual void	unk501() = 0;
	virtual void	unk502() = 0;

	virtual void	FillServerInfo( CSVCMsg_ServerInfo_t *pServerInfo ) = 0;
	virtual void	UserInfoChanged( CPlayerSlot slot ) = 0;

	// 2nd arg is unused.
	virtual void	GetClassBaseline( void *, ServerClass *pClass, intp *pOut ) = 0;

	virtual void	unk601() = 0;

	virtual CServerSideClientBase *ConnectClient( const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer,
												  const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence ) = 0;
	virtual CServerSideClientBase *CreateNewClient( CPlayerSlot slot ) = 0;
	
	virtual bool	FinishCertificateCheck( const ns_address *pAddr, int socket, byte ) = 0;

	virtual ChallengeType_t	GetChallengeType( const ns_address &addr ) = 0;
	virtual bool	CheckChallenge( const ns_address &addr, const char *pszChallenge ) = 0;

	virtual void	CalculateCPUUsage() = 0;
};

abstract_class INetworkServerService : public IEngineService
{
public:
	virtual ~INetworkServerService() {}
	virtual CNetworkGameServerBase	*GetIGameServer( void ) = 0;
	virtual bool	IsActiveInGame( void ) const = 0;
	virtual bool	IsMultiplayer( void ) const = 0;
	virtual void	StartupServer( const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char * ) = 0;
	virtual void	SetGameSpawnGroupMgr( IGameSpawnGroupMgr *pMgr ) = 0;
	virtual void	AddServerPrerequisites( const GameSessionConfiguration_t &, const char *, ILoopModePrerequisiteRegistry *, bool ) = 0;
	//virtual void	SetServerSocket( int ) = 0;
	virtual bool	IsServerRunning( void ) const = 0;
	virtual void	DisconnectGameNow( /*ENetworkDisconnectionReason*/ int ) = 0;
	virtual void	PrintSpawnGroupStatus( void ) const = 0;
	virtual void	SetFinalSimulationTickThisFrame( int ) = 0;
	virtual void	*GetGameServer( void ) = 0;
	//virtual int		GetTickInterval( void ) const = 0;
	//virtual void	ProcessSocket( void ) = 0;
	virtual int		GetServerNetworkAddress( void ) = 0;
	virtual bool	GameLoadFailed( void ) const = 0;
	virtual void	SetGameLoadFailed( bool bFailed ) = 0;
	virtual void	SetGameLoadStarted( void ) = 0;
	virtual void	unk_18019F5B0( void ) = 0;
	virtual void	StartChangeLevel( void ) = 0;
	virtual void	PreserveSteamID( void ) = 0;
	virtual CRC32_t	GetServerSerializersCRC( void ) = 0;
	virtual void	*GetServerSerializersMsg( void ) = 0;
};

typedef CNetworkGameServerBase IServer;


#endif // ISERVER_H
