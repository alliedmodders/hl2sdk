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
#include <tier1/checksum_crc.h>
#include <engine/IEngineService.h>

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

typedef int HGameResourceManifest;

enum ESpawnGroupUnloadOption
{

};

abstract_class INetworkGameServer : public IConnectionlessPacketHandler
{
public:
	virtual	void	Init( const GameSessionConfiguration_t &, const char * ) = 0;
	virtual void	SetGameSpawnGroupMgr( IGameSpawnGroupMgr * ) = 0;
	virtual void	SetGameSessionManifest( HGameResourceManifest * ) = 0;
	virtual void	RegisterLoadingSpawnGroups( CUtlVector<unsigned int> & ) = 0;
	virtual void	Shutdown( void ) = 0;
	virtual void	AddRef( void ) = 0;
	virtual void	Release ( void ) = 0;
	virtual CGlobalVars *GetGlobals(void) = 0;
	virtual bool	IsActive( void ) const = 0;	
	virtual void	SetServerTick( int tick ) = 0;
	virtual int		GetServerTick( void ) const = 0;	// returns game world tick
	virtual void	SetFinalSimulationTickThisFrame( int ) = 0;
	virtual int		GetMaxClients( void ) const = 0; // returns current client limit
	virtual float	GetTickInterval( void ) const = 0; // tick interval in seconds
	virtual void	ServerAdvanceTick( const EventServerAdvanceTick_t & ) = 0;
	virtual void	ServerPollNetworking( const EventServerPollNetworking_t & ) = 0;
	virtual void	ServerProcessNetworking( const EventServerProcessNetworking_t & ) = 0;
	virtual void	ServerSimulate( const EventServerSimulate_t & ) = 0;
	virtual void	ServerPostSimulate( const EventServerPostSimulate_t & ) = 0;
	virtual void	LoadSpawnGroup( const SpawnGroupDesc_t & ) = 0;
	virtual void	AsyncUnloadSpawnGroup( unsigned int, ESpawnGroupUnloadOption ) = 0;
	virtual void	PrintSpawnGroupStatus( void ) const = 0;
	virtual float	GetTimescale( void ) const = 0; // returns the game time scale (multiplied in conjunction with host_timescale)
	virtual bool	IsSaveRestoreAllowed( void ) const = 0;
	virtual void	SetMapName( const char *pszNewName ) = 0;
	virtual const char *GetMapName( void ) const = 0; // current map name (BSP)
	virtual const char *GetAddonName( void ) const = 0;
	virtual bool	IsBackgroundMap( void ) const = 0;
	virtual float	GetTime( void ) const = 0;	// returns game world time
	virtual int		GetMapVersion( void ) const = 0;
	virtual void	ActivateServer( void ) = 0;
	virtual void	PrepareForAssetLoad( void ) = 0;
	virtual int		GetServerNetworkAddress( void ) = 0;
	virtual SpawnGroupHandle_t FindSpawnGroupByName( const char *pszName ) = 0;
	virtual void	MakeSpawnGroupActive( SpawnGroupHandle_t ) = 0;
	virtual void	SynchronouslySpawnGroup( SpawnGroupHandle_t ) = 0;
	virtual void	SetServerState( server_state_t ) = 0;
	virtual void	SpawnServer( const char * ) = 0;
	virtual void	GetSpawnGroupLoadingStatus( SpawnGroupHandle_t ) = 0;
	virtual void	SetSpawnGroupDescription( SpawnGroupHandle_t, const char * ) = 0;
	virtual CUtlVector<INetworkGameClient *> *StartChangeLevel( const char *, const char *pszLandmark, void * ) = 0;
	virtual void	FinishChangeLevel( CServerChangelevelState * ) = 0;
	virtual bool	IsChangelevelPending( void ) const = 0;
	virtual void	GetAllLoadingSpawnGroups( CUtlVector<SpawnGroupHandle_t> *pOut ) = 0;
	virtual void	PreserveSteamID( void ) = 0;
	virtual void	OnKickById( const CCommandContext &context, const CCommand &args) = 0;
	virtual void	OnKickByName( const CCommandContext &context, const CCommand &args) = 0;
};

abstract_class INetworkServerService : public IEngineService
{
public:
	virtual ~INetworkServerService() {}
	virtual INetworkGameServer	*GetIGameServer( void ) = 0;
	virtual bool	IsActiveInGame( void ) const = 0;
	virtual bool	unk001( void ) const = 0;
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

typedef INetworkGameServer IServer;


#endif // ISERVER_H
