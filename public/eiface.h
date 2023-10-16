//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
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

#include "tier1/convar.h"
#include "icvar.h"
#include "edict.h"
#include "mathlib/vplane.h"
#include "iserverentity.h"
#include "engine/ivmodelinfo.h"
#include "soundflags.h"
#include "bitvec.h"
#include "tier1/bitbuf.h"
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"
#include "tier1/bufferstring.h"
#include <steam/steamclientpublic.h>
#include "playerslot.h"
#include <iloopmode.h>

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
class ILoopModePrerequisiteRegistry;
struct URLArgument_t;
struct vis_info_t;
class IHLTVServer;
class CCompressedResourceManifest;
class ILoadingSpawnGroup;
class IToolGameSimulationAPI;

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

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

class CPlayerUserId
{
public:
	CPlayerUserId( int index )
	{
		_index = index;
	}

	int Get() const
	{
		return _index;
	}

	bool operator==( const CPlayerUserId &other ) const { return other._index == _index; }
	bool operator!=( const CPlayerUserId &other ) const { return other._index != _index; }

private:
	short _index;
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL and client DLL
//-----------------------------------------------------------------------------
abstract_class ISource2Engine : public IAppSystem
{
public:
	// Is the game paused?
	virtual bool		IsPaused() = 0;
	
	// What is the game timescale multiplied with the host_timescale?
	virtual float		GetTimescale( void ) const = 0;
	
	virtual void		*FindOrCreateWorldSession( const char *pszWorldName, CResourceManifestPrerequisite * ) = 0;
	
	virtual CEntityLump	*GetEntityLumpForTemplate( const char *, bool, const char *, const char * ) = 0;
	
	virtual uint32		GetStatsAppID() const = 0;
	
	virtual void		*UnknownFunc1(const char *pszFilename, void *pUnknown1, void *pUnknown2, void *pUnknown3) = 0;
	virtual void		UnknownFunc2() = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
abstract_class IVEngineServer2 : public ISource2Engine
{
public:
	virtual EUniverse	GetSteamUniverse() const = 0;
	
	virtual void		unk001() = 0;
	virtual void		unk002() = 0;
	virtual void		unk003() = 0;
	virtual void		unk004() = 0;
	virtual void		unk005() = 0;
	virtual void		unk006() = 0;


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

	virtual void		UnknownFunc3() = 0;
	virtual void		UnknownFunc4() = 0;
	
	virtual int			PrecacheGeneric( const char *s, bool preload = false ) = 0;
	virtual bool		IsGenericPrecached( char const *s ) const = 0;
	
	// Returns the server assigned userid for this player.  Useful for logging frags, etc.  
	//  returns -1 if the edict couldn't be found in the list of players.
	virtual CPlayerUserId GetPlayerUserId( CPlayerSlot nSlot ) = 0;
	virtual const char	*GetPlayerNetworkIDString( CPlayerSlot nSlot ) = 0;
	// Get stats info interface for a client netchannel
	virtual INetChannelInfo* GetPlayerNetInfo( CPlayerSlot nSlot ) = 0;
	
	virtual bool		IsUserIDInUse( int userID ) = 0;	// TERROR: used for transitioning
	virtual int			GetLoadingProgressForUserID( int userID ) = 0;	// TERROR: used for transitioning
	
	// Given the current PVS(or PAS) and origin, determine which players should hear/receive the message
	virtual void		Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, CPlayerBitVec& playerbits ) = 0;
	
	// Issue a command to the command parser as if it was typed at the server console.
	virtual void		ServerCommand( const char *str ) = 0;
	// Issue the specified command to the specified client (mimics that client typing the command at the console).
	virtual void		ClientCommand( CPlayerSlot nSlot, const char *szFmt, ... ) FMTFUNCTION( 3, 4 ) = 0;
	
	// Set the lightstyle to the specified value and network the change to any connected clients.  Note that val must not 
	//  change place in memory (use MAKE_STRING) for anything that's not compiled into your mod.
	virtual void		LightStyle( int style, const char *val ) = 0;
	
	// Print szMsg to the client console.
	virtual void		ClientPrintf( CPlayerSlot nSlot, const char *szMsg ) = 0;
	
	virtual bool		IsLowViolence() = 0;
	virtual bool		SetHLTVChatBan( int tvslot, bool bBanned ) = 0;
	virtual bool		IsAnyClientLowViolence() = 0;
	
	// Get the current game directory (hl2, tf2, hl1, cstrike, etc.)
	virtual void        GetGameDir( CBufferString &gameDir ) = 0;
	
	// Create a bot with the given name.  Player index is -1 if fake client can't be created
	virtual CPlayerSlot	CreateFakeClient( const char *netname ) = 0;
	
	// Get a convar keyvalue for s specified client
	virtual const char	*GetClientConVarValue( CPlayerSlot nSlot, const char *name ) = 0;
	
	// Print a message to the server log file
	virtual void		LogPrint( const char *msg ) = 0;
	virtual bool		IsLogEnabled() = 0;
	
	virtual bool IsSplitScreenPlayer( CPlayerSlot nSlot ) = 0;
	virtual edict_t *GetSplitScreenPlayerAttachToEdict( CPlayerSlot nSlot ) = 0;
	virtual int	GetNumSplitScreenUsersAttachedToEdict( CPlayerSlot nSlot ) = 0;
	virtual edict_t *GetSplitScreenPlayerForEdict( CPlayerSlot nSlot, int nSplitScreenSlot ) = 0;
	
	// Ret types might be all wrong for these. Haven't researched yet.
	virtual void	UnloadSpawnGroup( SpawnGroupHandle_t spawnGroup, /*ESpawnGroupUnloadOption*/ int) = 0;
	virtual void	SetSpawnGroupDescription( SpawnGroupHandle_t spawnGroup, const char *pszDescription ) = 0;
	virtual bool	IsSpawnGroupLoaded( SpawnGroupHandle_t spawnGroup ) const = 0;
	virtual bool	IsSpawnGroupLoading( SpawnGroupHandle_t spawnGroup ) const = 0;
	virtual void	MakeSpawnGroupActive( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual void	SynchronouslySpawnGroup( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual void	SynchronizeAndBlockUntilLoaded( SpawnGroupHandle_t spawnGroup ) = 0;
	
	virtual void SetTimescale( float flTimescale ) = 0;

	virtual uint32		GetAppID() = 0;
	
	// Returns the SteamID of the specified player. It'll be NULL if the player hasn't authenticated yet.
	virtual const CSteamID	*GetClientSteamID( CPlayerSlot nSlot ) = 0;
	
	// Methods to set/get a gamestats data container so client & server running in same process can send combined data
	virtual void SetGamestatsData( CGamestatsData *pGamestatsData ) = 0;
	virtual CGamestatsData *GetGamestatsData() = 0;
	
	// Send a client command keyvalues
	// keyvalues are deleted inside the function
	virtual void ClientCommandKeyValues( CPlayerSlot nSlot, KeyValues *pCommand ) = 0;
	
	// This makes the host run 1 tick per frame instead of checking the system timer to see how many ticks to run in a certain frame.
	// i.e. it does the same thing timedemo does.
	virtual void SetDedicatedServerBenchmarkMode( bool bBenchmarkMode ) = 0;
	
	// Returns true if this client has been fully authenticated by Steam
	virtual bool IsClientFullyAuthenticated( CPlayerSlot nSlot ) = 0;
	
	virtual CGlobalVars	*GetServerGlobals() = 0;
	
	// Sets a USERINFO client ConVar for a fakeclient
	virtual void		SetFakeClientConVarValue( CPlayerSlot nSlot, const char *cvar, const char *value ) = 0;
	
	virtual CSharedEdictChangeInfo* GetSharedEdictChangeInfo() = 0;
	
	virtual void SetAchievementMgr( IAchievementMgr *pAchievementMgr ) =0;
	virtual IAchievementMgr *GetAchievementMgr() = 0;
	
	// Fill in the player info structure for the specified player index (name, model, etc.)
	virtual bool GetPlayerInfo( CPlayerSlot nSlot, google::protobuf::Message &info ) = 0;
	
	// Returns the XUID of the specified player. It'll be NULL if the player hasn't connected yet.
	virtual uint64 GetClientXUID( CPlayerSlot nSlot ) = 0;
	
	virtual void				*GetPVSForSpawnGroup( SpawnGroupHandle_t spawnGroup ) = 0;
	virtual SpawnGroupHandle_t	FindSpawnGroupByName( const char *szName ) = 0;
	
	// Returns the SteamID of the game server
	virtual CSteamID	GetGameServerSteamID() = 0;
	
	virtual int GetBuildVersion( void ) const = 0;
	
	virtual bool IsClientLowViolence( CPlayerSlot nSlot ) = 0;
	
	// Kicks the slot with the specified NetworkDisconnectionReason
	virtual void DisconnectClient( CPlayerSlot nSlot, /* ENetworkDisconnectionReason */ int reason ) = 0;

#if 0 // Don't really match the binary
	virtual void GetAllSpawnGroupsWithPVS( CUtlVector<SpawnGroupHandle_t> *spawnGroups, CUtlVector<IPVS *> *pOut ) = 0;
	
	virtual void P2PGroupChanged() = 0;
#endif

	virtual void unk101() = 0;
	virtual void unk102() = 0;
	virtual void unk103() = 0;
	virtual void unk104() = 0;
	virtual void unk105() = 0;
	virtual void unk106() = 0;
	virtual void unk107() = 0;

	virtual void OnKickClient( const CCommandContext &context, const CCommand &cmd ) = 0;

	// Kicks and bans the slot.
    // Note that the internal reason is never displayed to the user.
	// ENetworkDisconnectionReason reason is ignored, client is always kicked with ENetworkDisconnectionReason::NETWORK_DISCONNECT_KICKBANADDED
	//
	// AM TODO: add header ref for ENetworkDisconnectReason from proto header
    virtual void BanClient( CPlayerSlot nSlot, const char *szInternalReason, /*ENetworkDisconnectionReason*/ char reason ) = 0;

	virtual void unk200() = 0;
	virtual void unk201() = 0;
	virtual void unk202() = 0;
	virtual void unk203() = 0;
	virtual void unk204() = 0;
	virtual void unk205() = 0;
	virtual void unk206() = 0;
	virtual void unk207() = 0;
	virtual void unk208() = 0;

	virtual void SetClientUpdateRate( CPlayerSlot nSlot, float flUpdateRate ) = 0;

	virtual void unk300() = 0;
	virtual void unk301() = 0;
};

abstract_class IServerGCLobby
{
public:
	virtual bool HasLobby() const = 0;
	virtual bool SteamIDAllowedToConnect( const CSteamID &steamId ) const = 0;
	virtual void UpdateServerDetails( void ) = 0;
	virtual bool ShouldHibernate() = 0;
	virtual bool SteamIDAllowedToP2PConnect( const CSteamID &steamId ) const = 0;
	virtual bool LobbyAllowsCheats( void ) const = 0;
};

#define INTERFACEVERSION_SERVERGAMEDLL				"Source2Server001"

//-----------------------------------------------------------------------------
// Purpose: These are the interfaces that the game .dll exposes to the engine
//-----------------------------------------------------------------------------
abstract_class ISource2Server : public IAppSystem
{
public:
	virtual bool			Unknown0() const = 0;

	virtual void			SetGlobals( CGlobalVars *pGlobals ) = 0;
	
	// Let the game .dll allocate it's own network/shared string tables
	virtual void			GameCreateNetworkStringTables( void ) = 0;
	
	virtual void			WriteSignonMessages( const bf_write &buf ) = 0;
	
	virtual void			PreWorldUpdate( bool simulating ) = 0;
	
	virtual CUtlMap<int, Entity2Networkable_t>	*GetEntity2Networkables( void ) const = 0;
	
	virtual void			*GetEntityInfo() = 0;
	
	// Called to apply lobby settings to a dedicated server
	virtual void			ApplyGameSettings( KeyValues *pKV ) = 0;
	
	// The server should run physics/think on all edicts
	// One of these bools is 'simulating'... probably
	virtual void			GameFrame( bool simulating, bool bFirstTick, bool bLastTick ) = 0;
	
	// Returns true if the game DLL wants the server not to be made public.
	// Used by commentary system to hide multiplayer commentary servers from the master.
	virtual bool			ShouldHideFromMasterServer( bool bServerHasPassword ) = 0;
	
	virtual void			GetMatchmakingTags( char *buf, size_t bufSize ) = 0;

	virtual void			ServerHibernationUpdate( bool bHibernating ) = 0;
	
	virtual IServerGCLobby	*GetServerGCLobby() = 0;
	
	virtual void			GetMatchmakingGameData( CBufferString &buf ) = 0;
	
	// return true to disconnect client due to timeout (used to do stricter timeouts when the game is sure the client isn't loading a map)
	virtual bool			ShouldTimeoutClient( int nUserID, float flTimeSinceLastReceived ) = 0;
	
	virtual void			PrintStatus( CEntityIndex nPlayerEntityIndex, CBufferString &output ) = 0;
	
	virtual int				GetServerGameDLLFlags( void ) const = 0;
	
	// Get the list of cvars that require tags to show differently in the server browser
	virtual void			GetTaggedConVarList( KeyValues *pCvarTagList ) = 0;
	
	// Give the list of datatable classes to the engine.  The engine matches class names from here with
	//  edict_t::classname to figure out how to encode a class's data for networking
	virtual ServerClass		*GetAllServerClasses( void ) = 0;
	
	virtual const char		*GetActiveWorldName( void ) const = 0;
	
	virtual bool			IsPaused( void ) const = 0;
	
	virtual bool			GetNavMeshData( CNavData *pNavMeshData ) = 0;
	virtual void			SetNavMeshData( const CNavData *navMeshData ) = 0;
	virtual void			RegisterNavListener( INavListener *pNavListener ) = 0;
	virtual void			UnregisterNavListener( INavListener *pNavListener ) = 0;
	virtual void			*GetSpawnDebugInterface( void ) = 0;
	virtual void			*Unknown1( void ) = 0;
	virtual IToolGameSimulationAPI *GetToolGameSimulationAPI( void ) = 0;
	virtual void			GetAnimationActivityList( CUtlVector<CUtlString> &activityList ) = 0;
	virtual void			GetAnimationEventList( CUtlVector<CUtlString> &eventList ) = 0;
	virtual void			FilterPlayerCounts( int *pInOutHumans, int *pInOutHumansSlots, int *pInOutBots ) = 0;

	// Called after the steam API has been activated post-level startup
	virtual void			GameServerSteamAPIActivated( void ) = 0;

	virtual void			GameServerSteamAPIDeactivated( void ) = 0;
	
	virtual void			OnHostNameChanged( const char *pHostname ) = 0;
	virtual void			PreFatalShutdown( void ) const = 0;
	virtual void			UpdateWhenNotInGame( float flFrameTime ) = 0;
	
	virtual void			GetEconItemNamesForModel( const char *pModelName, bool bExcludeItemSets, bool bExcludeIndividualItems, CUtlVector<CUtlString> &econItemNames ) = 0;
	virtual void			GetEconItemNamesForCharacter( const char *pCharacterName, bool bExcludeItemSets, bool bExcludeIndividualItems, CUtlVector<CUtlString> &econItemNames ) = 0;
	virtual void			GetEconItemsInfoForModel( const char *pModelName, const char *pEconItemName, bool bExcludeItemSets, bool bExcludeIndividualItems, bool bExcludeStockItemSet, CUtlVector<EconItemInfo_t> &econInfo ) = 0;
	virtual void			GetEconItemsInfoForCharacter( const char *pCharacterName, const char *pEconItemName, bool bExcludeItemSets, bool bExcludeIndividualItems, bool bExcludeStockItemSet, CUtlVector<EconItemInfo_t> &econInfo ) = 0;
	
	virtual void			GetDefaultScaleForModel( const char *pModelName, bool bCheckLoadoutScale ) = 0;
	virtual void			GetDefaultScaleForCharacter( const char *pCharacterName, bool bCheckLoadoutScale ) = 0;
	virtual void			GetDefaultControlPointAutoUpdates( const char *pParticleSystemName, CUtlVector<EconControlPointInfo_t> &autoUpdates ) = 0;
	virtual void			GetCharacterNameForModel( const char *pModelName, bool bCheckItemModifiers, CUtlString &characterName ) = 0;
	virtual void			GetModelNameForCharacter( const char *pCharacterNamel, int nIndex, CBufferString &modelName ) = 0;  
	virtual void			GetCharacterList( CUtlVector<CUtlString> &characterNames ) = 0;
	virtual void			GetDefaultChoreoDirForModel( const char *pModelName, CBufferString &defaultVCDDir ) = 0;
	
	virtual void			*GetEconItemSystem( void ) = 0;
	
	virtual void			ServerConVarChanged( const char *pVarName, const char *pValue ) = 0;
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
	
	// Get the simulation interval (must be compiled with identical values into both client and game .dll for MOD!!!)
	// Right now this is only requested at server startup time so it can't be changed on the fly, etc.
	virtual float			GetTickInterval( void ) const = 0;
	
	// Get server maxplayers and lower bound for same
	virtual void			GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers, bool &bIsMultiplayer ) const = 0;
	
	// Returns max splitscreen slot count ( 1 == no splits, 2 for 2-player split screen )
	virtual int		GetMaxSplitscreenPlayers( void ) = 0;
	
	// Return # of human slots, -1 if can't determine or don't care (engine will assume it's == maxplayers )
	virtual int				GetMaxHumanPlayers() = 0;
	
	virtual bool			ShouldNotifyLocalClientConnectionStateChanges() = 0;
	
	virtual bool			AllowPlayerToTakeOverBots() = 0;
	
	virtual void			OnClientFullyConnect( CEntityIndex nEntityIndex ) = 0;
	
	virtual void		GetHostStateLoopModeInfo( HostStateLoopModeType_t type, CUtlString &loopModeName, KeyValues **ppLoopModeOptions ) = 0;
	
	virtual bool		AllowDedicatedServers( EUniverse universe ) const = 0;
	
	virtual void		GetConVarPrefixesToResetToDefaults( CUtlString &sSemicolonDelimitedPrefixList ) const = 0;

	virtual bool		AllowSaveRestore() = 0;
};

#define INTERFACEVERSION_SERVERGAMECLIENTS		"Source2GameClients001"

//-----------------------------------------------------------------------------
// Purpose: Player / Client related functions
//-----------------------------------------------------------------------------
abstract_class ISource2GameClients : public IAppSystem
{
public:
	virtual void			OnClientConnected( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer ) = 0;
	
	// Called when the client attempts to connect (doesn't get called for bots)
	// returning false would reject the connection with the pRejectReason message
	virtual bool			ClientConnect( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason );

	// Client is connected and should be put in the game
	// type values could be:
	// 0 - player
	// 1 - fake player (bot)
	// 2 - unknown
	virtual void			ClientPutInServer( CPlayerSlot slot, char const *pszName, int type, uint64 xuid ) = 0;

	// Client is going active
	// If bLoadGame is true, don't spawn the player because its state is already setup.
	virtual void			ClientActive( CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid ) = 0;
	
	virtual void			ClientFullyConnect( CPlayerSlot slot ) = 0;

	// Client is disconnecting from server
	virtual void			ClientDisconnect( CPlayerSlot slot, /* ENetworkDisconnectionReason */ int reason,
								const char *pszName, uint64 xuid, const char *pszNetworkID ) = 0;

	// Sets the client index for the client who typed the command into his/her console
	// virtual void			SetCommandClient( CPlayerSlot slot) = 0;
	
	// The client has typed a command at the console
	virtual void			ClientCommand( CPlayerSlot slot, const CCommand &args ) = 0;

	// A player changed one/several replicated cvars (name etc)
	virtual void			ClientSettingsChanged( CPlayerSlot slot ) = 0;
	
	// Determine PVS origin and set PVS for the player/viewentity
	virtual void			ClientSetupVisibility( CPlayerSlot slot, vis_info_t *visinfo ) = 0;
	
	// A block of CUserCmds has arrived from the user, decode them and buffer for execution during player simulation
	virtual float			ProcessUsercmds( CPlayerSlot slot, bf_read *buf, int numcmds, bool ignore, bool paused ) = 0;
	
	virtual bool			IsPlayerSlotOccupied( CPlayerSlot slot ) = 0;
	
	virtual bool			IsPlayerAlive( CPlayerSlot slot) = 0;
	
	virtual int				GetPlayerScore( CPlayerSlot slot ) = 0;

	// Get the ear position for a specified client
	virtual void			ClientEarPosition( CPlayerSlot slot, Vector *pEarOrigin ) = 0;

	// Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
	//  can be added here
	virtual void			GetBugReportInfo( CBufferString &buf ) = 0;

	// TERROR: A player sent a voice packet
	virtual void			ClientVoice( CPlayerSlot slot ) = 0;

	// A user has had their network id setup and validated 
	virtual void			NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) = 0;
	
	// The client has submitted a keyvalues command
	virtual void			ClientCommandKeyValues( CPlayerSlot slot, KeyValues *pKeyValues ) = 0;

	virtual bool			ClientCanPause( CPlayerSlot slot ) = 0;
	
	virtual void			HLTVClientFullyConnect( int index, const CSteamID &steamID ) = 0;
	
	virtual bool			CanHLTVClientConnect( int index, const CSteamID &steamID, int *pRejectReason ) = 0;
	
	virtual void			StartHLTVServer( CEntityIndex index ) = 0;
	
	virtual void			SendHLTVStatusMessage( IHLTVServer *, bool, bool, const char *, int, int, int ) = 0;
	
	virtual IHLTVDirector	*GetHLTVDirector( void ) = 0;

	virtual void			unk002( CPlayerSlot slot ) = 0;
	virtual void			unk003( CPlayerSlot slot ) = 0;

	// Something NetMessage related
	virtual void			unk004() = 0;

	// Something pawn related
	virtual void			unk005() = 0;
	virtual void			unk006() = 0;

	virtual void			unk007() = 0;
	virtual void			unk008() = 0;
};

typedef IVEngineServer2 IVEngineServer;
typedef ISource2Server IServerGameDLL;
typedef ISource2GameEntities IServerGameEnts;
typedef ISource2GameClients IServerGameClients;

#endif // EIFACE_H
