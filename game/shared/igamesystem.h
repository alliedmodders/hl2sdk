//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMESYSTEM_H
#define IGAMESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "network_connection.pb.h"
#include "iloopmode.h"

/*
* AMNOTE: To create your own gamesystem, you need to inherit from CBaseGameSystem or CAutoGameSystem,
* and define the events that you are interested in receiving via the GS_EVENT macro, example:
* 
* class TestGameSystem : CBaseGameSystem
* {
*     DECLARE_GAME_SYSTEM();
*     GS_EVENT( GameInit )
*     {
*         // In every event there's a ``msg`` argument provided which is a struct for this particular event
*         // refer to struct implementation to see what members are available in it, example usage:
*         msg->m_pConfig;
*         msg->m_pRegistry;
*     }
*     
*     // Or you can declare the needed events in your gamesystem, and define their body somewhere else
*     GS_EVENT( GameShutdown );
* };
* 
* GS_EVENT_MEMBER( TestGameSystem, GameShutdown )
* {
*     // The same ``msg`` argument would be available in here as well.
* }
*/

// Forward declaration
class GameSessionConfiguration_t;
class ILoopModePrerequisiteRegistry;
class IEntityResourceManifest;
class ISpawnGroupPrerequisiteRegistry;
class IEntityPrecacheConfiguration;
struct EngineLoopState_t;
struct EntitySpawnInfo_t;

#define DECLARE_GAME_SYSTEM() \
	virtual void YouForgot_DECLARE_GAME_SYSTEM_InYourClassDefinition() override {}; \
	struct YouForgot { } m_YouForgot;

#define GS_EVENT_MSG( name ) struct Event##name##_t
#define GS_EVENT_MSG_CHILD( name, parent ) struct Event##name##_t : Event##parent##_t

GS_EVENT_MSG( GameInit )
{
	const GameSessionConfiguration_t *m_pConfig;
	ILoopModePrerequisiteRegistry *m_pRegistry;
};

GS_EVENT_MSG( GamePostInit )
{
	const GameSessionConfiguration_t *m_pConfig;
	ILoopModePrerequisiteRegistry *m_pRegistry;
};

GS_EVENT_MSG( GameShutdown ) {};
GS_EVENT_MSG( GamePreShutdown ) {};

GS_EVENT_MSG( GameActivate )
{
	const EngineLoopState_t *m_pState;
	bool m_bBackgroundMap;
};

GS_EVENT_MSG( GameDeactivate )
{
	const EngineLoopState_t *m_pState;
	bool m_bBackgroundMap;
};

GS_EVENT_MSG( ClientFullySignedOn ) {};
GS_EVENT_MSG( ClientDisconnect )
{
	ENetworkDisconnectionReason m_ReasonCode;
};

GS_EVENT_MSG( ClientRestoreServerState ) {};

GS_EVENT_MSG( BuildGameSessionManifest )
{
	IEntityResourceManifest *m_pResourceManifest;
};

GS_EVENT_MSG( SpawnGroupPrecache )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
	int m_nEntityCount;
	const EntitySpawnInfo_t *m_pEntitiesToSpawn;
	ISpawnGroupPrerequisiteRegistry *m_pRegistry;
	IEntityResourceManifest *m_pManifest;
	IEntityPrecacheConfiguration *m_pConfig;
};

GS_EVENT_MSG( SpawnGroupUncache )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
};

GS_EVENT_MSG( PreSpawnGroupLoad )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
};

GS_EVENT_MSG( PostSpawnGroupLoad )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
	CUtlVector<CEntityHandle> m_EntityList;
};

GS_EVENT_MSG( PreSpawnGroupUnload )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
	CUtlVector<CEntityHandle> m_EntityList;
};

GS_EVENT_MSG( PostSpawnGroupUnload )
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
};

GS_EVENT_MSG( ActiveSpawnGroupChanged )
{
	SpawnGroupHandle_t m_SpawnGroupHandle;
	CUtlString m_SpawnGroupName;
	SpawnGroupHandle_t m_PreviousHandle;
};

GS_EVENT_MSG( ClientPostDataUpdate ) {};

GS_EVENT_MSG( ClientPreRender )
{
	float m_flFrameTime;
};
GS_EVENT_MSG( ClientPostRender ) {};

GS_EVENT_MSG( ClientPreEntityThink )
{
	bool m_bFirstTick;
	bool m_bLastTick;
};

GS_EVENT_MSG( ClientPreOutputParallelWithServer ) {};
GS_EVENT_MSG( ClientPostOutputParallelWithServer ) {};

GS_EVENT_MSG( ClientPreRenderAlt )
{
	float m_flFrameTime;
};

GS_EVENT_MSG( ClientPollNetworking ) {};

GS_EVENT_MSG( ClientUpdate )
{
	float m_flFrameTime;
	bool m_bFirstTick;
	bool m_bLastTick;
};

GS_EVENT_MSG_CHILD( ClientPreUpdate, ClientUpdate ) {};
GS_EVENT_MSG_CHILD( ClientPostUpdate, ClientUpdate ) {};

GS_EVENT_MSG( ServerPreEntityThink )
{
	bool m_bFirstTick;
	bool m_bLastTick;
};

GS_EVENT_MSG( ServerPostEntityThink )
{
	bool m_bFirstTick;
	bool m_bLastTick;
};

GS_EVENT_MSG( ServerPostPhysicsSimulate ) {};

GS_EVENT_MSG( ServerPreClientUpdate ) {};
GS_EVENT_MSG( ServerAdvanceTick ) {};
GS_EVENT_MSG( ClientAdvanceTick ) {};

GS_EVENT_MSG( Simulate )
{
	EngineLoopState_t m_LoopState;
	bool m_bFirstTick;
	bool m_bLastTick;
};

GS_EVENT_MSG_CHILD( ServerGamePostSimulate, Simulate ) {};
GS_EVENT_MSG_CHILD( ClientGamePostSimulate, Simulate ) {};

GS_EVENT_MSG( ServerPostAdvanceTick ) {};
GS_EVENT_MSG( ClientPostAdvanceTick ) {};

GS_EVENT_MSG( ServerBeginAsyncPostTickWork )
{
	// AMNOTE: Also is set on gpGlobals->m_unk301
	bool m_unk001;
};

GS_EVENT_MSG( ServerPreEndAsyncPostTickWork ) {};
GS_EVENT_MSG( ServerPostEndAsyncPostTickWork ) {};

GS_EVENT_MSG( ClientFrameSimulate ) {};
GS_EVENT_MSG( ClientPauseSimulate ) {};
GS_EVENT_MSG( ClientAdvanceNonRenderedFrame ) {};

GS_EVENT_MSG( FrameBoundary )
{
	float m_flFrameTime;
};

GS_EVENT_MSG_CHILD( GameFrameBoundary, FrameBoundary ) {};
GS_EVENT_MSG_CHILD( OutOfGameFrameBoundary, FrameBoundary ) {};

GS_EVENT_MSG( SaveGame )
{
	const CUtlVector<CEntityHandle> *m_pEntityList;
};

GS_EVENT_MSG( RestoreGame )
{
	const CUtlVector<CEntityHandle> *m_pEntityList;
};

GS_EVENT_MSG( NewLevelPlayerConnect )
{
	const CUtlString m_PreviousLevel;
};

GS_EVENT_MSG( DemoSkip )
{
	int m_PlaybackTick;
	int m_SkipToTick;
	bool m_InstantReplay;
};

GS_EVENT_MSG( PrePackEntities )
{
	CUtlVector<Entity2Networkable_t *> m_Entities;
};

#define GS_EVENT_IMPL( name ) virtual void On##name(const Event##name##_t* const msg) = 0;
#define GS_EVENT( name ) virtual void On##name(const Event##name##_t* const msg) override
#define GS_EVENT_MEMBER( gamesystem, name ) void gamesystem::On##name(const Event##name##_t* const msg)

//-----------------------------------------------------------------------------
// Game systems are singleton objects in the client + server codebase responsible for
// various tasks
// The order in which the server systems appear in this list are the
// order in which they are initialized and updated. They are shut down in
// reverse order from which they are initialized.
//-----------------------------------------------------------------------------
abstract_class IGameSystem
{
public:
	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() = 0;								// 0
	virtual void PostInit() = 0;							// 1
	virtual void Shutdown() = 0;							// 2

	// Game init, shutdown
	GS_EVENT_IMPL( GameInit )								// 3
	GS_EVENT_IMPL( GameShutdown )							// 4
	GS_EVENT_IMPL( GamePostInit )							// 5
	GS_EVENT_IMPL( GamePreShutdown )						// 6

	GS_EVENT_IMPL( BuildGameSessionManifest )				// 7

	GS_EVENT_IMPL( GameActivate )							// 8

	GS_EVENT_IMPL( ClientFullySignedOn )					// 9
	GS_EVENT_IMPL( ClientDisconnect )						// 10

	GS_EVENT_IMPL( ClientRestoreServerState )				// 11

	GS_EVENT_IMPL( GameDeactivate )							// 12

	GS_EVENT_IMPL( SpawnGroupPrecache )						// 13
	GS_EVENT_IMPL( SpawnGroupUncache )						// 14
	GS_EVENT_IMPL( PreSpawnGroupLoad )						// 15
	GS_EVENT_IMPL( PostSpawnGroupLoad )						// 16
	GS_EVENT_IMPL( PreSpawnGroupUnload )					// 17
	GS_EVENT_IMPL( PostSpawnGroupUnload )					// 18
	GS_EVENT_IMPL( ActiveSpawnGroupChanged )				// 19
	GS_EVENT_IMPL( ClientPostDataUpdate )					// 20

	// Called before rendering
	GS_EVENT_IMPL( ClientPreRender )						// 21

	GS_EVENT_IMPL( ClientPreEntityThink )					// 22

	GS_EVENT_IMPL( ClientPreOutputParallelWithServer )		// 23
	GS_EVENT_IMPL( ClientPostOutputParallelWithServer )		// 24
	GS_EVENT_IMPL( ClientPreRenderAlt )						// 25

	GS_EVENT_IMPL( ClientPollNetworking )					// 26

	// Gets called each frame
	GS_EVENT_IMPL( ClientPreUpdate )						// 27
	GS_EVENT_IMPL( ClientPostUpdate )						// 28

	// AMNOTE: Client event that's called at the end of 11th ClientFrameStage_t
	virtual void unk_101( const void *const msg ) = 0;		// 29

	// Called after rendering
	GS_EVENT_IMPL( ClientPostRender )						// 30

	// Called each frame before entities think
	GS_EVENT_IMPL( ServerPreEntityThink )					// 31
	// called after entities think
	GS_EVENT_IMPL( ServerPostEntityThink )					// 32
	GS_EVENT_IMPL( ServerPostPhysicsSimulate )				// 33
	GS_EVENT_IMPL( ServerPreClientUpdate )					// 34
	GS_EVENT_IMPL( ServerAdvanceTick )						// 35
	GS_EVENT_IMPL( ClientAdvanceTick )						// 36
	GS_EVENT_IMPL( ServerGamePostSimulate )					// 37
	GS_EVENT_IMPL( ClientGamePostSimulate )					// 38
	GS_EVENT_IMPL( ServerPostAdvanceTick )					// 39
	GS_EVENT_IMPL( ClientPostAdvanceTick )					// 40
	GS_EVENT_IMPL( ServerBeginAsyncPostTickWork )			// 41
	GS_EVENT_IMPL( ServerPreEndAsyncPostTickWork )			// 42
	GS_EVENT_IMPL( ServerPostEndAsyncPostTickWork )			// 43
	GS_EVENT_IMPL( ClientFrameSimulate )					// 44
	GS_EVENT_IMPL( ClientPauseSimulate )					// 45
	GS_EVENT_IMPL( ClientAdvanceNonRenderedFrame )			// 46

	GS_EVENT_IMPL( GameFrameBoundary )						// 47
	GS_EVENT_IMPL( OutOfGameFrameBoundary )					// 48

	GS_EVENT_IMPL( SaveGame )								// 49
	GS_EVENT_IMPL( RestoreGame )							// 50

	// AMNOTE: Called only when gpGlobals->maxplayer == 1 on player_connect_full
	GS_EVENT_IMPL( NewLevelPlayerConnect )					// 51

	// AMNOTE: CSpawnGroupMgrGameSystem related
	virtual void unk_201( const void *const msg ) = 0;		// 52
	virtual void unk_202( const void *const msg ) = 0;		// 53

	virtual void unk_203( const void *const msg ) = 0;		// 54
	virtual void unk_204( const void *const msg ) = 0;		// 55

	// Same as to demo_skip event
	GS_EVENT_IMPL( DemoSkip )								// 56

	GS_EVENT_IMPL( PrePackEntities )						// 57

	virtual const char* GetName() const = 0;				// 58
	virtual void SetGameSystemGlobalPtrs(void* pValue) = 0;	// 59
	virtual void SetName(const char* pName) = 0;			// 60
	virtual bool DoesGameSystemReallocate() = 0;			// 61
	virtual ~IGameSystem() {}
	virtual void YouForgot_DECLARE_GAME_SYSTEM_InYourClassDefinition() = 0;
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseGameSystem : public IGameSystem
{
public:
	CBaseGameSystem(const char* pszInitName = "unnamed")
	 :  m_pName(pszInitName)
	{
	}

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() override { return true; }
	virtual void PostInit() override {}
	virtual void Shutdown() override {}

	// Game init, shutdown
	GS_EVENT( GameInit ) {}
	GS_EVENT( GameShutdown ) {}
	GS_EVENT( GamePostInit ) {}
	GS_EVENT( GamePreShutdown ) {}

	GS_EVENT( BuildGameSessionManifest ) {}

	GS_EVENT( GameActivate ) {}

	GS_EVENT( ClientFullySignedOn ) {}
	GS_EVENT( ClientDisconnect ) {}

	GS_EVENT( ClientRestoreServerState ) {}

	GS_EVENT( GameDeactivate ) {}

	GS_EVENT( SpawnGroupPrecache ) {}
	GS_EVENT( SpawnGroupUncache ) {}
	GS_EVENT( PreSpawnGroupLoad ) {}
	GS_EVENT( PostSpawnGroupLoad ) {}
	GS_EVENT( PreSpawnGroupUnload ) {}
	GS_EVENT( PostSpawnGroupUnload ) {}
	GS_EVENT( ActiveSpawnGroupChanged ) {}

	GS_EVENT( ClientPostDataUpdate ) {}

	// Called before rendering
	GS_EVENT( ClientPreRender ) {}

	GS_EVENT( ClientPreEntityThink ) {}

	GS_EVENT( ClientPreOutputParallelWithServer ) {}
	GS_EVENT( ClientPostOutputParallelWithServer ) {}
	GS_EVENT( ClientPreRenderAlt ) {}

	GS_EVENT( ClientPollNetworking ) {}

	// Gets called each frame
	GS_EVENT( ClientPreUpdate ) {}
	GS_EVENT( ClientPostUpdate ) {}

	virtual void unk_101( const void *const msg ) override {}

	// Called after rendering
	GS_EVENT( ClientPostRender ) {}

	// Called each frame before entities think
	GS_EVENT( ServerPreEntityThink ) {}
	// called after entities think
	GS_EVENT( ServerPostEntityThink ) {}
	GS_EVENT( ServerPostPhysicsSimulate ) {}
	GS_EVENT( ServerPreClientUpdate ) {}
	GS_EVENT( ServerAdvanceTick ) {}
	GS_EVENT( ClientAdvanceTick ) {}
	GS_EVENT( ServerGamePostSimulate ) {}
	GS_EVENT( ClientGamePostSimulate ) {}
	GS_EVENT( ServerPostAdvanceTick ) {}
	GS_EVENT( ClientPostAdvanceTick ) {}
	GS_EVENT( ServerBeginAsyncPostTickWork ) {}
	GS_EVENT( ServerPreEndAsyncPostTickWork ) {}
	GS_EVENT( ServerPostEndAsyncPostTickWork ) {}
	GS_EVENT( ClientFrameSimulate ) {}
	GS_EVENT( ClientPauseSimulate ) {}
	GS_EVENT( ClientAdvanceNonRenderedFrame ) {}

	GS_EVENT( GameFrameBoundary ) {}
	GS_EVENT( OutOfGameFrameBoundary ) {}

	GS_EVENT( SaveGame ) {}
	GS_EVENT( RestoreGame ) {}

	GS_EVENT( NewLevelPlayerConnect ) {}

	virtual void unk_201( const void *const msg ) override {}
	virtual void unk_202( const void *const msg ) override {}
	virtual void unk_203( const void *const msg ) override {}
	virtual void unk_204( const void *const msg ) override {}

	GS_EVENT( DemoSkip ) {}

	GS_EVENT( PrePackEntities ) {}

	virtual const char* GetName() const override { return m_pName; }
	virtual void SetGameSystemGlobalPtrs(void* pValue) override {}
	virtual void SetName(const char* pName) override { m_pName = pName; }
	virtual bool DoesGameSystemReallocate() override { return false; }
	virtual ~CBaseGameSystem() {}

private:
	const char* m_pName;
};

class CAutoGameSystem : public CBaseGameSystem
{
protected:
	virtual ~CAutoGameSystem() {};
};

#endif // IGAMESYSTEM_H
