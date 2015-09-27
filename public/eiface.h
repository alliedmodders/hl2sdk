//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef EIFACE_H
#define EIFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "icvar.h"
#include "edict.h"
#include "mathlib/vplane.h"
#include "iserverentity.h"
#include "engine/ivmodelinfo.h"
#include "soundflags.h"
#include "bitvec.h"
#include "tier1/bitbuf.h"
#include "tier1/utlstring.h"
#include <steam/steamclientpublic.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class	ServerClass;
class	CGameTrace;
typedef	CGameTrace trace_t;
struct	typedescription_t;
class	CSaveRestoreData;
struct	datamap_t;
struct	studiohdr_t;
class	CBaseEntity;
class	CRestore;
class	CSave;
class	variant_t;
struct	vcollide_t;
class	IRecipientFilter;
class	CBaseEntity;
class	ITraceFilter;
class	INetChannelInfo;
class	ISpatialPartition;
class IScratchPad3D;
class CStandardSendProxies;
class IAchievementMgr;
class CGamestatsData;
class CSteamID;
class ISPSharedMemory;
class CGamestatsData;
class CEngineHltvInfo_t;
class INetworkStringTable;
class CResourceManifestPrerequisite;
class CEntityLump;
class IPVS;
class IHLTVDirector;
struct SpawnGroupDesc_t;
class IClassnameForMapClassCallback;
struct Entity2Networkable_t;
class CCreateGameServerLoadInfo;
class INavListener;
class CNavData;
struct EconItemInfo_t;
struct EconControlPointInfo_t;
class CEntityHandle;
struct RenderDeviceInfo_t;
struct RenderMultisampleType_t;
class GameSessionConfiguration_t;
struct StringTableDef_t;
struct HostStateLoopModeType_t;
class ILoopModePrerequisiteRegistry;
struct URLArgument_t;
struct vis_info_t;
class IHLTVServer;

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

typedef uint32 SpawnGroupHandle_t;
typedef uint32 SwapChainHandle_t;

//-----------------------------------------------------------------------------
// defines
//-----------------------------------------------------------------------------

#ifdef _WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT /* */
#endif

#define INTERFACEVERSION_VENGINESERVER	"Source2EngineToServer001"

struct bbox_t
{
	Vector mins;
	Vector maxs;
};

struct CPlayerSlot
{
	CPlayerSlot( int slot )
	{
		_slot = slot;
	}
	
	int Get() const
	{
		return _slot;
	}
	
private:
	int _slot;
};

struct CEntityIndex
{
	CEntityIndex( int index )
	{
		_index = index;
	}
	
	int Get() const
	{
		return _index;
	}
	
	int _index;
};

struct CSplitScreenSlot
{
	CSplitScreenSlot( int index )
	{
		_index = index;
	}
	
	int Get() const
	{
		return _index;
	}
	
	int _index;
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL and client DLL
//-----------------------------------------------------------------------------
abstract_class ISource2Engine : public IAppSystem
{
	// Is the game paused?
	virtual bool		IsPaused() = 0;
	
	// What is the game timescale multiplied with the host_timescale?
	virtual float		GetTimescale( void ) const = 0;
	
	virtual void		*FindOrCreateWorldSession( const char *, CResourceManifestPrerequisite * ) = 0;
	
	virtual void		UpdateAddonSearchPaths( bool, bool, const char* ) = 0;
	
	virtual CEntityLump	*GetEntityLumpForTemplate( const char *, bool, const char *, const char * ) = 0;
	
	virtual uint32		GetStatsAppID() const = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
abstract_class IVEngineServer2 : public ISource2Engine
{
public:
	virtual CreateInterfaceFn GetGameServerFactory() = 0;
	
	// Tell engine to change level ( "changelevel s1\n" or "changelevel2 s1 s2\n" )
	virtual void		ChangeLevel( const char *s1, const char *s2 ) = 0;
	
	// Ask engine whether the specified map is a valid map file (exists and has valid version number).
	virtual int			IsMapValid( const char *filename ) = 0;
	
	// Is this a dedicated server?
	virtual bool		IsDedicatedServer( void ) = 0;
	
	// Is this an HLTV relay?
	virtual bool		IsHLTVRelay( void ) = 0;
	
	// Is server only accepting local connections?
	virtual bool		IsServerLocalOnly( void ) = 0;
	
	// Add to the server/client lookup/precache table, the specified string is given a unique index
	// NOTE: The indices for PrecacheModel are 1 based
	//  a 0 returned from those methods indicates the model or sound was not correctly precached
	// However, generic and decal are 0 based
	// If preload is specified, the file is loaded into the server/client's cache memory before level startup, otherwise
	//  it'll only load when actually used (which can cause a disk i/o hitch if it occurs during play of a level).
	virtual int			PrecacheDecal( const char *name, bool preload = false ) = 0;
	virtual bool		IsDecalPrecached( const char *s ) const = 0;	
	virtual int			GetPrecachedDecalIndex ( const char *s ) const = 0;
	
	virtual int			PrecacheGeneric( const char *s, bool preload = false ) = 0;
	virtual bool		IsGenericPrecached( char const *s ) const = 0;
	
	// Returns the server assigned userid for this player.  Useful for logging frags, etc.  
	//  returns -1 if the edict couldn't be found in the list of players.
	virtual int			GetPlayerUserId( CPlayerSlot clientSlot ) = 0; 
	virtual const char	*GetPlayerNetworkIDString( CPlayerSlot clientSlot ) = 0;
	// Get stats info interface for a client netchannel
	virtual INetChannelInfo* GetPlayerNetInfo( CEntityIndex playerIndex ) = 0;
	
	virtual bool		IsUserIDInUse( int userID ) = 0;	// TERROR: used for transitioning
	virtual int			GetLoadingProgressForUserID( int userID ) = 0;	// TERROR: used for transitioning
	
	// This returns which entities, to the best of the server's knowledge, the client currently knows about.
	// This is really which entities were in the snapshot that this client last acked.
	// This returns a bit vector with one bit for each entity.
	//
	// USE WITH CARE. Whatever tick the client is really currently on is subject to timing and
	// ordering differences, so you should account for about a quarter-second discrepancy in here.
	// Also, this will return NULL if the client doesn't exist or if this client hasn't acked any frames yet.
	// 
	// iClientIndex is the CLIENT index, so if you use pPlayer->entindex(), subtract 1.
	virtual const CBitVec<MAX_EDICTS>* GetEntityTransmitBitsForClient( CEntityIndex iClientIndex ) = 0;
	
	// Given the current PVS(or PAS) and origin, determine which players should hear/receive the message
	virtual void		Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, CPlayerBitVec& playerbits ) = 0;
	
	// Issue a command to the command parser as if it was typed at the server console.	
	virtual void		ServerCommand( const char *str ) = 0;
	// Issue the specified command to the specified client (mimics that client typing the command at the console).
	virtual void		ClientCommand( CEntityIndex playerIndex, const char *szFmt, ... ) FMTFUNCTION( 3, 4 ) = 0;
	
	// Set the lightstyle to the specified value and network the change to any connected clients.  Note that val must not 
	//  change place in memory (use MAKE_STRING) for anything that's not compiled into your mod.
	virtual void		LightStyle( int style, const char *val ) = 0;

	// Project a static decal onto the specified entity / model (for level placed decals in the .bsp)
	virtual void		StaticDecal( const Vector &originInEntitySpace, int decalIndex, CEntityIndex entityIndex, int modelIndex, bool lowpriority ) = 0;
	
	// Print szMsg to the client console.
	virtual void		ClientPrintf( CEntityIndex playerIndex, const char *szMsg ) = 0;
	
	// SINGLE PLAYER/LISTEN SERVER ONLY (just matching the client .dll api for this)
	// Prints the formatted string to the notification area of the screen ( down the right hand edge
	//  numbered lines starting at position 0
	virtual void		Con_NPrintf( int pos, const char *fmt, ... ) = 0;
	// SINGLE PLAYER/LISTEN SERVER ONLY(just matching the client .dll api for this)
	// Similar to Con_NPrintf, but allows specifying custom text color and duration information
	virtual void		Con_NXPrintf( const struct con_nprint_s *info, const char *fmt, ... ) = 0;
	
	virtual bool IsLowViolence() = 0;
	virtual bool SetHLTVChatBan( int tvslot, bool bBanned ) = 0;
	virtual bool IsAnyClientLowViolence() = 0;
	
	// Change a specified player's "view entity" (i.e., use the view entity position/orientation for rendering the client view)
	virtual void		SetView( CEntityIndex playerIndex, CEntityIndex viewEntIndex ) = 0;
	
	// Get the current game directory (hl2, tf2, hl1, cstrike, etc.)
	virtual void        GetGameDir( char *szGetGameDir, int maxlength ) = 0;
	
	// Create a bot with the given name.  Player index is -1 if fake client can't be created
	virtual CEntityIndex	CreateFakeClient( const char *netname ) = 0;
	
	// Get a convar keyvalue for s specified client
	virtual const char	*GetClientConVarValue( CEntityIndex clientIndex, const char *name ) = 0;
	
	// Print a message to the server log file
	virtual void		LogPrint( const char *msg ) = 0;
	virtual bool		IsLogEnabled() = 0;
	
	virtual bool IsSplitScreenPlayer( CEntityIndex ent_num ) = 0;
	virtual edict_t *GetSplitScreenPlayerAttachToEdict( CEntityIndex ent_num ) = 0;
	virtual int	GetNumSplitScreenUsersAttachedToEdict( CEntityIndex ent_num ) = 0;
	virtual edict_t *GetSplitScreenPlayerForEdict( CEntityIndex ent_num, int nSlot ) = 0;
	
	// Ret types might be all wrong for these. Haven't researched yet.
	virtual bool	IsSpawnGroupLoadedOnClient( CEntityIndex ent_num, SpawnGroupHandle_t spawnGroup ) const = 0;
	virtual void	UnloadSpawnGroup( SpawnGroupHandle_t spawnGroup, /*ESpawnGroupUnloadOption*/ int) = 0;
	virtual void	LoadSpawnGroup( const SpawnGroupDesc_t & ) = 0;
	virtual void	SetSpawnGroupDescription( SpawnGroupHandle_t spawnGroup, const char *pszDescription ) = 0;
	virtual bool	IsSpawnGroupLoaded( SpawnGroupHandle_t spawnGroup ) const = 0;
	virtual bool	IsSpawnGroupLoading( SpawnGroupHandle_t spawnGroup ) const = 0;
	virtual void	MakeSpawnGroupActive( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual void	SynchronouslySpawnGroup( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual void	SynchronizeAndBlockUntilLoaded( SpawnGroupHandle_t spawnGroup ) = 0;
	
	virtual void SetTimescale( float flTimescale ) = 0;
	virtual CEntityIndex	GetViewEntity( CEntityIndex ent ) = 0;
	
	// Is the engine in Commentary mode?
	virtual int			IsInCommentaryMode( void ) = 0;
	
	virtual uint32		GetAppID() = 0;
	
	// Returns the SteamID of the specified player. It'll be NULL if the player hasn't authenticated yet.
	virtual const CSteamID	*GetClientSteamID( CEntityIndex clientIndex ) = 0;
	
	virtual void		ForceModelBounds( const char *s, const Vector &mins, const Vector &maxs ) = 0;
	
	// Marks the material (vmt file) for consistency checking.  If the client and server have different
	// contents for the file, the client's vmt can only use the VertexLitGeneric shader, and can only
	// contain $baseTexture and $bumpmap vars.
	virtual void		ForceSimpleMaterial( const char *s ) = 0;
	
	// Marks the filename for consistency checking.  This should be called after precaching the file.
	virtual void		ForceExactFile( const char *s ) = 0;
	
	// Methods to set/get a gamestats data container so client & server running in same process can send combined data
	virtual void SetGamestatsData( CGamestatsData *pGamestatsData ) = 0;
	virtual CGamestatsData *GetGamestatsData() = 0;
	
	// Send a client command keyvalues
	// keyvalues are deleted inside the function
	virtual void ClientCommandKeyValues( CEntityIndex client, KeyValues *pCommand ) = 0;
	
	// This makes the host run 1 tick per frame instead of checking the system timer to see how many ticks to run in a certain frame.
	// i.e. it does the same thing timedemo does.
	virtual void SetDedicatedServerBenchmarkMode( bool bBenchmarkMode ) = 0;
	
	// Returns true if this client has been fully authenticated by Steam
	virtual bool IsClientFullyAuthenticated( CPlayerSlot slot ) = 0;
	
	virtual CGlobalVars	*GetServerGlobals() = 0;
	
	// Sets a USERINFO client ConVar for a fakeclient
	virtual void		SetFakeClientConVarValue( CEntityIndex clientIndex, const char *cvar, const char *value ) = 0;
	
	virtual CSharedEdictChangeInfo* GetSharedEdictChangeInfo() = 0;
	
	virtual void SetAchievementMgr( IAchievementMgr *pAchievementMgr ) =0;
	virtual IAchievementMgr *GetAchievementMgr() = 0;
	
	// Fill in the player info structure for the specified player index (name, model, etc.)
	virtual bool GetPlayerInfo( CEntityIndex ent_num, google::protobuf::Message &info ) = 0;
	
	// Validate session
	virtual void HostValidateSession() = 0;
	
	// Returns the XUID of the specified player. It'll be NULL if the player hasn't connected yet.
	virtual uint64 GetClientXUID( CEntityIndex clientIndex ) = 0;
	
	//Finds or Creates a shared memory space, the returned pointer will automatically be AddRef()ed
	virtual ISPSharedMemory *GetSinglePlayerSharedMemorySpace( const char *szName, int ent_num = MAX_EDICTS ) = 0;
	
	virtual void				*GetPVSForSpawnGroup( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual SpawnGroupHandle_t	FindSpawnGroupByName( const char *szName ) = 0;
	
	// Returns the SteamID of the game server
	virtual CSteamID	GetGameServerSteamID() = 0;
	
	virtual int GetBuildVersion( void ) const = 0;
	
	virtual bool IsClientLowViolence( CEntityIndex clientIndex ) = 0;
	
	virtual void DisconnectClient( CEntityIndex clientIndex, /* ENetworkDisconnectionReason */ int reason ) = 0;
	
	virtual void GetAllSpawnGroupsWithPVS( CUtlVector<SpawnGroupHandle_t> *spawnGroups, CUtlVector<IPVS *> *pOut ) = 0;
	
	virtual void P2PGroupChanged() = 0;
};

abstract_class IServerGCLobby
{
public:
	virtual bool HasLobby() const = 0;
	virtual bool SteamIDAllowedToConnect( const CSteamID &steamId ) const = 0;
	virtual void UpdateServerDetails( void ) = 0;
	virtual bool ShouldHibernate() = 0;
	virtual bool SteamIDAllowedToP2PConnect( const CSteamID &steamId ) const = 0;
};

#define INTERFACEVERSION_SERVERGAMEDLL				"Source2Server001"

//-----------------------------------------------------------------------------
// Purpose: These are the interfaces that the game .dll exposes to the engine
//-----------------------------------------------------------------------------
abstract_class ISource2Server : public IAppSystem
{
public:
	virtual void			SetGlobals( CGlobalVars *pVars ) = 0;
	
	// Let the game .dll allocate it's own network/shared string tables
	virtual void			GameCreateNetworkStringTables( void ) = 0;
	
	virtual void			WriteSignonMessages( bf_write &bf ) = 0;
	
	virtual void			EnumSaveRestoreMapClasses( const void *, unsigned long, IClassnameForMapClassCallback * ) = 0;
	
	virtual void			PreWorldUpdate( bool simulating ) = 0;
	
	virtual CUtlVector<Entity2Networkable_t>	&GetEntity2Networkables( void ) const = 0;
	virtual bool			GetEntity2Networkable( CEntityIndex index, Entity2Networkable_t &out ) = 0;
	
	virtual void			ClearInstancedBaselineFromServerClasses( void ) = 0;
	
	virtual void			GetLevelsFromSaveFile( const char *pszFileName, CUtlVector<CCreateGameServerLoadInfo> &out, bool ) = 0;
	virtual void			ClearSaveDirectory( void ) = 0;
	
	virtual void			*GetEntityInfo() = 0;
	
	// Called to apply lobby settings to a dedicated server
	virtual void			ApplyGameSettings( KeyValues *pKV ) = 0;
	
	// The server should run physics/think on all edicts
	// One of these bools is 'simulating'... probably
	virtual void			GameFrame( bool, bool, bool ) = 0;
	
	virtual void			PreSaveGameLoaded( char const *pSaveName, bool bCurrentlyInGame ) = 0;

	// Returns true if the game DLL wants the server not to be made public.
	// Used by commentary system to hide multiplayer commentary servers from the master.
	virtual bool			ShouldHideFromMasterServer( bool ) = 0;
	
	virtual void			GetMatchmakingTags( char *buf, size_t bufSize ) = 0;

	virtual void			ServerHibernationUpdate( bool bHibernating ) = 0;
	
	virtual IServerGCLobby	*GetServerGCLobby() = 0;
	
	virtual void			GetMatchmakingGameData( char *buf, size_t bufSize ) = 0;
	
	// return true to disconnect client due to timeout (used to do stricter timeouts when the game is sure the client isn't loading a map)
	virtual bool			ShouldTimeoutClient( int nUserID, float flTimeSinceLastReceived ) = 0;
	
	virtual void			Status( void (*inputFunc) (const char *fmt, ...) ) = 0;
	
	virtual int				GetServerGameDLLFlags( void ) const = 0;
	
	// Get the list of cvars that require tags to show differently in the server browser
	virtual void			GetTaggedConVarList( KeyValues *pCvarTagList ) = 0;
	
	// Give the list of datatable classes to the engine.  The engine matches class names from here with
	//  edict_t::classname to figure out how to encode a class's data for networking
	virtual ServerClass*	GetAllServerClasses( void ) = 0;
	
	virtual const char		*GetLoadedMapName( void ) const = 0;
	
	virtual bool			IsPaused( void ) const = 0;
	
	virtual bool			GetNavMeshData( CNavData *pOut ) = 0;
	virtual void			SetNavMeshData( const CNavData &data ) = 0;
	virtual void			RegisterNavListener( INavListener *pListener ) = 0;
	virtual void			UnregisterNavListener( INavListener *pListener ) = 0;
	virtual void			*GetSpawnDebugInterface( void ) = 0;
	virtual void			GetAnimationActivityList( CUtlVector<CUtlString> &out ) = 0;
	virtual void			GetAnimationEventList( CUtlVector<CUtlString> &out ) = 0;
	virtual void			FilterPlayerCounts( int *, int *, int * ) = 0;

	// Called after the steam API has been activated post-level startup
	virtual void			GameServerSteamAPIActivated( void ) = 0;

	virtual void			GameServerSteamAPIDeactivated( void ) = 0;
	
	virtual void			OnHostNameChanged( const char *pszNewHostname ) = 0;
	virtual void			PreFatalShutdown( void ) const = 0;
	virtual void			UpdateWhenNotInGame( float ) = 0;
	virtual void			GetEconItemNamesForModel( const char *pszModel, bool, bool, CUtlVector<CUtlString> &out ) = 0;
	virtual void			GetEconItemNamesForCharacter( const char *pszCharacter, bool, bool, CUtlVector<CUtlString> &out ) = 0;
	virtual void			GetInfoForEconItems( const char *, const char *, CUtlVector<EconItemInfo_t> &out ) = 0;
	virtual void			GetDefaultScaleForModel( const char *pszModel ) = 0;
	virtual void			GetDefaultScaleForCharacter( const char *pszCharacter ) = 0;
	virtual void			GetDefaultControlPointAutoUpdates( const char *, CUtlVector<EconControlPointInfo_t> &out ) = 0;
	virtual void			GetCharacterNameForModel( const char *pszCharacter, char *pszOut, int size ) = 0;
	virtual void			GetDefaultModelNameForCharacter( const char *pszCharacter, char *pszOut, int size ) = 0;
	virtual void			GetCharacterList( CUtlVector<CUtlString> &out ) = 0;
	virtual void			GetDefaultChoreoDirForModel( const char *pszModel, char *pszOut, int size ) = 0;
	virtual void			OnStreamEntitiesFromFileCompleted( CUtlVector<CEntityHandle> &handles ) = 0;
};

//-----------------------------------------------------------------------------
// Just an interface version name for the random number interface
// See vstdlib/random.h for the interface definition
// NOTE: If you change this, also change VENGINE_CLIENT_RANDOM_INTERFACE_VERSION in cdll_int.h
//-----------------------------------------------------------------------------
#define VENGINE_SERVER_RANDOM_INTERFACE_VERSION	"VEngineRandom001"

#define INTERFACEVERSION_SERVERGAMEENTS			"Source2GameEntities001"
//-----------------------------------------------------------------------------
// Purpose: Interface to get at server entities
//-----------------------------------------------------------------------------
abstract_class ISource2GameEntities : public IAppSystem
{
public:
	virtual					~ISource2GameEntities()	{}
	
	// This sets a bit in pInfo for each edict in the list that wants to be transmitted to the 
	// client specified in pInfo.
	//
	// This is also where an entity can force other entities to be transmitted if it refers to them
	// with ehandles.
	virtual void			CheckTransmit( CCheckTransmitInfo **pInfo, int, CBitVec<16384> &, const Entity2Networkable_t **pNetworkables,
								const uint16 *pEntityIndicies, int nEntities ) = 0;
	
	// TERROR: Perform any PVS cleanup before a full update
	virtual void			PrepareForFullUpdate( CEntityIndex entIndex ) = 0;
	
	// Frees the entity attached to this edict
	virtual void			FreeContainingEntity( CEntityIndex entIndex ) = 0;
	
	virtual void			GetWorldspaceCenter( CEntityIndex entIndex, Vector *pOut ) const = 0;
	
	virtual bool			ShouldClientReceiveStringTableUserData( const INetworkStringTable *pStringTable, int iIndex, const CCheckTransmitInfo *pTransmitInfo ) = 0;
	
	virtual void			ResetChangeAccessorsSerialNumbersToZero() = 0;
};

#define INTERFACEVERSION_SERVERCONFIG			"Source2ServerConfig001"

abstract_class ISource2ServerConfig : public IAppSystem
{
public:
	// Returns string describing current .dll.  e.g., TeamFortress 2, Half-Life 2.  
	//  Hey, it's more descriptive than just the name of the game directory
	virtual const char *GetGameDescription( void ) = 0;
	
	virtual int			GetNetworkVersion( void ) = 0;
	virtual bool		ValidateNetworkVersion ( int version ) const = 0;
	
	// Get the simulation interval (must be compiled with identical values into both client and game .dll for MOD!!!)
	// Right now this is only requested at server startup time so it can't be changed on the fly, etc.
	virtual float			GetTickInterval( void ) const = 0;
	
	// Get server maxplayers and lower bound for same
	virtual void			GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers, bool &unknown ) const = 0;
	
	// Returns max splitscreen slot count ( 1 == no splits, 2 for 2-player split screen )
	virtual int		GetMaxSplitscreenPlayers( void ) = 0;
	
	// Return # of human slots, -1 if can't determine or don't care (engine will assume it's == maxplayers )
	virtual int				GetMaxHumanPlayers() = 0;
	
	virtual bool			ShouldNotifyLocalClientConnectionStateChanges() = 0;
	
	virtual bool			AllowPlayerToTakeOverBots() = 0;
	
	virtual void			OnClientFullyConnect( CEntityIndex index ) = 0;
	
	virtual void		GetHostStateLoopModeInfo( HostStateLoopModeType_t, CUtlString &, KeyValues ** ) = 0;
	
	virtual bool		AllowDedicatedServers( EUniverse universe ) const = 0;
	
	virtual void		GetConVarPrefixesToResetToDefaults( CUtlString &prefixes ) const = 0;
};

#define INTERFACEVERSION_SERVERGAMECLIENTS		"Source2GameClients001"

//-----------------------------------------------------------------------------
// Purpose: Player / Client related functions
//-----------------------------------------------------------------------------
abstract_class ISource2GameClients : public IAppSystem
{
public:
	// Client is connecting to server ( return false to reject the connection )
	//	You can specify a rejection message by writing it into reject
	virtual bool			ClientConnect( CEntityIndex index, uint64 xuid, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) = 0;
	
	virtual void			OnClientConnected( CEntityIndex index, int userId, const char *pszName, uint64 xuid, const char *pszNetworkID,
								const char *pszAddress, bool bFakePlayer ) = 0;
	
	// Client is connected and should be put in the game
	virtual void			ClientPutInServer( CEntityIndex index, char const *playername, bool bFakePlayer ) = 0;

	// Client is going active
	// If bLoadGame is true, don't spawn the player because its state is already setup.
	virtual void			ClientActive( CEntityIndex index, bool bLoadGame ) = 0;
	
	virtual void			ClientFullyConnect( CEntityIndex index, CPlayerSlot slot ) = 0;

	// Client is disconnecting from server
	virtual void			ClientDisconnect( CEntityIndex index, int userId, /* ENetworkDisconnectionReason */ int reason,
								const char *pszName, uint64 xuid, const char *pszNetworkID ) = 0;

	// Sets the client index for the client who typed the command into his/her console
	virtual void			SetCommandClient( CPlayerSlot slot) = 0;
	
	// The client has typed a command at the console
	virtual void			ClientCommand( CEntityIndex index, const CCommand &args ) = 0;

	// A player changed one/several replicated cvars (name etc)
	virtual void			ClientSettingsChanged( CEntityIndex index ) = 0;
	
	// Determine PVS origin and set PVS for the player/viewentity
	virtual void			ClientSetupVisibility( CEntityIndex viewentIndex, CEntityIndex clientindex, vis_info_t *visinfo ) = 0;
	
	// A block of CUserCmds has arrived from the user, decode them and buffer for execution during player simulation
	virtual float			ProcessUsercmds( CEntityIndex index, bf_read *buf, int numcmds, int totalcmds,
								int dropped_packets, bool ignore, bool paused ) = 0;
	
	// Let the game .dll do stuff after messages have been sent to all of the clients once the server frame is complete
	virtual void			PostClientMessagesSent_DEPRECIATED( void ) = 0;
	
	virtual bool			IsPlayerAlive( CEntityIndex index ) = 0;
	
	virtual int				GetPlayerScore( CEntityIndex index ) = 0;

	// Get the ear position for a specified client
	virtual void			ClientEarPosition( CEntityIndex index, Vector *pEarOrigin ) = 0;

	// Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
	//  can be added here
	virtual void			GetBugReportInfo( char *buf, int buflen ) = 0;

	// TERROR: A player sent a voice packet
	virtual void			ClientVoice( CEntityIndex index ) = 0;

	// A user has had their network id setup and validated 
	virtual void			NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) = 0;
	
	// The client has submitted a keyvalues command
	virtual void			ClientCommandKeyValues( CEntityIndex index, KeyValues *pKeyValues ) = 0;

	virtual bool			ClientCanPause( CEntityIndex index ) = 0;
	
	virtual void			HLTVClientFullyConnect( int index, const CSteamID &steamID ) = 0;
	
	virtual bool			CanHLTVClientConnect( int index, const CSteamID &steamID, int *pRejectReason ) = 0;
	
	virtual void			StartHLTVServer( CEntityIndex index ) = 0;
	
	virtual void			SendHLTVStatusMessage( IHLTVServer *, bool, bool, const char *, int, int, int ) = 0;
	
	virtual IHLTVDirector	*GetHLTVDirector( void ) = 0;
};

typedef IVEngineServer2 IVEngineServer;
typedef ISource2Server IServerGameDLL;
typedef ISource2GameEntities IServerGameEnts;
typedef ISource2GameClients IServerGameClients;

#endif // EIFACE_H
