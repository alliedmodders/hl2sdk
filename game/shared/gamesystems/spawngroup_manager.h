#ifndef SPAWNGROUP_MANAGER_H
#define SPAWNGROUP_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "eiface.h"
#include "iserver.h"
#include "igamesystem.h"
#include "entity2/entitysystem.h"
#include "entity2/entityidentity.h"
#include "mathlib/mathlib.h"
#include "resourcefile/resourcetype.h"
#include "tier1/utlstring.h"
#include "tier1/utlscratchmemory.h"
#include "tier1/utlvector.h"

#define MAX_SPAWN_GROUP_WORLD_NAME_LENGTH 4096

class matrix3x4a_t;
class CKeyValues3Cluster;
class CEntityKeyValues;
class CGameResourceManifest;
class ISpawnGroupPrerequisiteRegistry;
class IComputeWorldOriginCallback;
class IWorldReference; // See worldrender/iworldreference.h

struct EventGameInit_t;

struct EventPreSpawnGroupLoad_t
{
	CUtlString m_SpawnGroupName;
	CUtlString m_EntityLumpName;
	SpawnGroupHandle_t m_SpawnGroupHandle;
};

struct EventServerPostEntityThink_t
{
	bool m_bFirstTick;
	bool m_bLastTick;
};

typedef int *SaveRestoreDataHandle_t;

enum SpawnGroupState_t
{
	SPAWN_GROUP_ALLOCATED = 0,
	SPAWN_GROUP_WORLD_LOADED = 1,
	SPAWN_GROUP_ENTITIES_ALLOCATED = 2,
	SPAWN_GROUP_GAMESYSTEMS_PRECACHE_START = 3,
	SPAWN_GROUP_GAMESYSTEMS_PRECACHE_END = 4,
	SPAWN_GROUP_ENTITY_PRECACHE_START = 5,
	SPAWN_GROUP_ENTITY_PRECACHE_END = 6,
	SPAWN_GROUP_READY_TO_SPAWN_ENTITIES = 7,
	SPAWN_GROUP_ENTITIES_SPAWNED = 8
};

struct SpawnGroupDesc_t
{
public:
	matrix3x4a_t m_vecWorldOffset;
	const char *m_pWorldName;
	const char *m_pWorldMountName;
	const char *m_pEntityLumpName;
	const char *m_pEntityFilterName;
	const char *m_pDescriptiveName;
	const char *m_pParentNameFixup;
	const char *m_pLocalNameFixup;
	IComputeWorldOriginCallback *m_pWorldOffsetCallback;
	WorldGroupId_t m_worldGroupId;
	SpawnGroupHandle_t m_hOwner;
	int m_iPriorityLoader;
	ResourceManifestLoadPriority_t m_manifestLoadPriority;
	float m_flTimeoutInterval;
	const char *m_pSaveFileName;
	bool m_bCreateClientEntitiesOnLaterConnectingClients;
	bool m_bDontSpawnEntities;
	bool m_bBlockUntilLoaded;
	bool m_bLoadStreamingData;
	bool m_bCreateNewSceneWorld;
	bool m_bManualCompletion;
	bool m_bSetActivePostLoad;
};

enum SpawnGroupFlags_t
{
	SPAWN_GROUP_LOAD_ENTITIES_FROM_SAVE = 0x1,
	SPAWN_GROUP_DONT_SPAWN_ENTITIES = 0x2,
	SPAWN_GROUP_SYNCHRONOUS_SPAWN = 0x4,
	SPAWN_GROUP_IS_INITIAL_SPAWN_GROUP = 0x8,
	SPAWN_GROUP_CREATE_CLIENT_ONLY_ENTITIES = 0x10,
	SPAWN_GROUP_SAVE_ENTITIES = 0x20,
	SPAWN_GROUP_BLOCK_UNTIL_LOADED = 0x40,
	SPAWN_GROUP_LOAD_STREAMING_DATA = 0x80,
	SPAWN_GROUP_CREATE_NEW_SCENE_WORLD = 0x100,
};

class ISpawnGroup
{
public:
	virtual const char *GetWorldName() const = 0;
	virtual const char *GetEntityLumpName() const = 0;
	virtual const char *GetEntityFilterName() const = 0;
	virtual SpawnGroupHandle_t GetHandle() const = 0;
	virtual const matrix3x4a_t &GetWorldOffset() const = 0;
	virtual bool ShouldLoadEntitiesFromSave() const = 0;
	virtual const char *GetParentNameFixup() const = 0;
	virtual const char *GetLocalNameFixup() const = 0;
	virtual SpawnGroupHandle_t GetOwnerSpawnGroup() const = 0;
	virtual ILoadingSpawnGroup *GetLoadingSpawnGroup() const = 0;
	virtual void SetLoadingSpawnGroup(ILoadingSpawnGroup *pLoading) = 0;
	virtual IWorldReference *GetWorldReference() const = 0;
	virtual void Describe(CUtlString &sOutput) const = 0;

public: // CNetworkClientSpawnGroup/CNetworkServerSpawnGroup
	virtual uint64 GetCreationTick() const = 0;
	virtual void RequestDeferredUnload(bool bUnk) = 0; // Client-side needs.
	virtual uint64 GetDestructionTick() const = 0;

public:
	virtual uint64 UnkIsManualFlag1() const = 0;
	virtual bool DontSpawnEntities() const = 0;
	virtual SpawnGroupHandle_t GetParentSpawnGroup() const = 0;
	virtual uint64 GetChildSpawnGroupCount() const = 0;
	virtual void GetSpawnGroupDesc(SpawnGroupDesc_t *pDesc) const = 0;

public:
	virtual void UnkSetManualFlag2() = 0;
	virtual void UnkIsManualFlag2() = 0;
	virtual void UnkClientOrServerOnGameResourceManifestLoaded(CGameResourceManifest *pManifest, int nResourceCount, ResourceHandle_t *pResourceHandles) = 0;
	virtual void Unk(SpawnGroupHandle_t h) = 0;
	virtual void UnkIsManualFlag3() = 0;
	virtual void UnkIsManualFlag4() = 0;
	virtual void UnkIsManualFlag5() = 0;
	virtual void UnkSetter(uint64 n) = 0;
	virtual void UnkIsManualFlag6() = 0;
	virtual WorldGroupId_t GetWorldGroupId() const = 0;

	virtual void ComputeWorldOrigin(matrix3x4_t *retstrp) = 0;
	virtual void Release() = 0;
	virtual void OnGameResourceManifestLoaded(CGameResourceManifest *pManifest, int nResourceCount, ResourceHandle_t *pResourceHandles) = 0;
	virtual void Init(IResourceManifestRegistry *pResourceManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry) = 0;
	virtual void Shutdown() = 0;
	virtual bool GetLoadStatus() = 0;
	virtual void ForceBlockingLoad() = 0;
	virtual bool ShouldBlockUntilLoaded() const = 0;
	virtual void ServiceBlockingLoads() = 0;
	virtual bool GetEntityPrerequisites(CGameResourceManifest *pManifest) = 0;
	virtual bool EntityPrerequisitesSatisfied() = 0;

public: // CNetworkClientSpawnGroup/CNetworkServerSpawnGroup
	virtual bool LoadEntities() = 0;
	virtual uint64 Unk(uint64, int, uint64) = 0;
	virtual uint64 Unk2(uint64, int, uint64) = 0;
	virtual uint64 SetParentSpawnGroupForChild(SpawnGroupHandle_t h) = 0;

public:
	virtual void TransferOwnershipOfManifestsTo(ISpawnGroup *pTarget) = 0;

public: // CNetworkClientSpawnGroup/CNetworkServerSpawnGroup
	virtual IGameSpawnGroupMgr *GetSpawnGroupMgr() const = 0;
};

class CMapSpawnGroup
{
public:
	const char *GetWorldName() const
	{
		return this->m_pSpawnGroup->GetWorldName();
	}

	const char *GetEntityLumpName() const
	{
		return this->m_pSpawnGroup->GetEntityLumpName();
	}

	const char *GetEntityFilterName() const
	{
		return this->m_pSpawnGroup->GetEntityFilterName();
	}

	SpawnGroupHandle_t GetSpawnGroupHandle() const
	{
		return this->m_pSpawnGroup->GetHandle();
	}

	const matrix3x4a_t &GetWorldOffset() const
	{
		return this->m_pSpawnGroup->GetWorldOffset();
	}

	const char *GetParentNameFixup() const
	{
		return this->m_pSpawnGroup->GetParentNameFixup();
	}

	const char *GetLocalNameFixup() const
	{
		return this->m_pSpawnGroup->GetLocalNameFixup();
	}

	SpawnGroupHandle_t GetOwnerSpawnGroup() const
	{
		return this->m_pSpawnGroup->GetOwnerSpawnGroup();
	}

	ILoadingSpawnGroup *GetLoadingSpawnGroup() const
	{
		return this->m_pSpawnGroup->GetLoadingSpawnGroup();
	}

	void SetLoadingSpawnGroup(ILoadingSpawnGroup *pLoading)
	{
		this->m_pSpawnGroup->SetLoadingSpawnGroup(pLoading);
	}

	int GetCreationTick() const
	{
		return this->m_pSpawnGroup->GetCreationTick();
	}

	bool DontSpawnEntities() const
	{
		return this->m_pSpawnGroup->DontSpawnEntities();
	}

	void GetSpawnGroupDesc(SpawnGroupDesc_t *pDesc) const
	{
		this->m_pSpawnGroup->GetSpawnGroupDesc(pDesc);
	}

public:
	ISpawnGroup *GetSpawnGroup() const
	{
		return this->m_pSpawnGroup;
	}

private:
	ISpawnGroup *m_pSpawnGroup;
	bool m_bSpawnGroupPrecacheDispatched;
	bool m_bSpawnGroupLoadDispatched;
	IPVS *m_pPVS;
	// CUtlVector<SpawnGroupConnectionInfo_t> m_Connections;
};

class ILoadingSpawnGroup
{
public:
	virtual int EntityCount() const = 0;
	virtual const EntitySpawnInfo_t *GetEntities() const = 0;
	virtual bool ShouldLoadEntitiesFromSave() const = 0;
	virtual const CUtlBuffer *GetSaveRestoreFileData() const = 0;
	virtual void SetLevelTransitionPreviousMap(const char *pLevelTransitionMap, const char *pLandmarkName) = 0;
	virtual bool IsLevelTransition() const = 0;
	virtual const char *GetLevelTransitionPreviousMap() const = 0;
	virtual const char *GetLevelTransitionLandmarkName() const = 0;
	virtual CKeyValues3Cluster *GetEntityKeyValuesAllocator() = 0;
	virtual CEntityInstance *CreateEntityToSpawn(SpawnGroupHandle_t hSpawnGroup, const matrix3x4a_t *const vecSpawnOffset, int createType, const CEntityKeyValues *pEntityKeyValues) = 0;
	virtual void SpawnEntities() = 0;
	virtual void Release() = 0;
};

class IGameSpawnGroupMgr
{
public:
	virtual void AllocateSpawnGroup(SpawnGroupHandle_t h, ISpawnGroup *pSpawnGroup) = 0;
	virtual void ReleaseSpawnGroup(SpawnGroupHandle_t h) = 0;
	virtual void SpawnGroupInit(SpawnGroupHandle_t handle, IEntityResourceManifest *pManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry) = 0;
	virtual ILoadingSpawnGroup *CreateLoadingSpawnGroup(SpawnGroupHandle_t handle, bool bSynchronouslySpawnEntities, bool bConfirmResourcesLoaded, const CUtlVector<const CEntityKeyValues *> *pKeyValues) = 0;
	virtual ILoadingSpawnGroup *CreateLoadingSpawnGroupForSaveFile(SpawnGroupHandle_t handle, const void *pSaveData, size_t nSaveDataSize) = 0;
	virtual void SpawnGroupSpawnEntities(SpawnGroupHandle_t handle) = 0;
	virtual void SpawnGroupShutdown(SpawnGroupHandle_t handle) = 0;
	virtual void SetActiveSpawnGroup(SpawnGroupHandle_t handle) = 0;
	virtual void UnloadSpawnGroup(SpawnGroupHandle_t hSpawnGroup, bool bSaveEntities) = 0;
	virtual SpawnGroupHandle_t GetParentSpawnGroup(SpawnGroupHandle_t hSpawnGroup) = 0;
	virtual int GetChildSpawnGroupCount(SpawnGroupHandle_t handle) = 0;
	virtual SaveRestoreDataHandle_t SaveGame_Start(const char *pSaveName, const char *pOldLevel, const char *pszLandmarkName) = 0;
	virtual bool SaveGame_Finalize(SaveRestoreDataHandle_t hSaveRestore) = 0;
	virtual bool FrameUpdatePostEntityThink(const EventServerPostEntityThink_t &msg) = 0;
	virtual const char *SaveGame_GetLastSaveFile(SaveRestoreDataHandle_t hSaveRestore) = 0;
	virtual bool IsGameReadyToSave() = 0;
	virtual unsigned long UnkGetGlobal1() = 0;
	virtual void * const UnkGetGameEntitySystemSync1() = 0;
	virtual void UnkRelease1() = 0;
	virtual void UnkRelease2() = 0;
	virtual void YouForgot_DECLARE_GAME_SYSTEM_InYourClassDefinition() = 0;
	virtual void UnkSpawnGroupManagerGameSystemMember1() = 0;
	virtual void GameInit(const EventGameInit_t &msg) = 0;
	virtual void GameShutdown(const EventGameInit_t &msg) = 0;
	virtual void FrameBoundary(const EventGameInit_t &msg) = 0;
	virtual void PreSpawnGroupLoad(const EventPreSpawnGroupLoad_t &msg) = 0;
	virtual ~IGameSpawnGroupMgr() = 0;
};

class CSpawnGroupMgrGameSystem : public IGameSpawnGroupMgr, public IGameSystem
{
};

class CLoadingSpawnGroup : public ILoadingSpawnGroup
{
};

#endif // SPAWNGROUP_MANAGER_H
