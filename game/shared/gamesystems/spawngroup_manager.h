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
class CKeyValues3Context;
class CEntityKeyValues;
class ILoadingSpawnGroup;
class CGameResourceManifest;
class ISpawnGroupPrerequisiteRegistry;
class IWorld;
class IWorldReference;

abstract_class IGameResourceManifestLoadCompletionCallback
{
public:
	virtual void OnGameResourceManifestLoaded( HGameResourceManifest hManifest, int nResourceCount, ResourceHandle_t * pResourceHandles ) = 0;
};

abstract_class IComputeWorldOriginCallback
{
public:
	virtual matrix3x4_t ComputeWorldOrigin( const char *pWorldName, SpawnGroupHandle_t hSpawnGroup, IWorld * pWorld ) = 0;
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
	SpawnGroupDesc_t()
	 :  m_pWorldOffsetCallback(NULL),
	    m_hOwner(0),
	    m_iPriorityLoader(-2),
	    m_manifestLoadPriority(RESOURCE_MANIFEST_LOAD_PRIORITY_DEFAULT),
	    m_flTimeoutInterval(0.0f),
	    m_bCreateClientEntitiesOnLaterConnectingClients(false),
	    m_bDontSpawnEntities(false),
	    m_bBlockUntilLoaded(false),
	    m_bLoadStreamingData(true),
	    m_bCreateNewSceneWorld(false),
	    m_bManualCompletion(false),
	    m_bSetActivePostLoad(false),
	    m_bUnk(true),
	    m_bLevelTransition(false),
	    m_bUnk2(false)
	{
		SetIdentityMatrix(m_vecWorldOffset);
	}

	matrix3x4a_t m_vecWorldOffset;
	CUtlString m_sWorldName;
	CUtlString m_sWorldMountName;
	CUtlString m_sEntityLumpName;
	CUtlString m_sEntityFilterName;
	CUtlString m_sDescriptiveName;
	CUtlString m_sParentNameFixup;
	CUtlString m_sLocalNameFixup;
	IComputeWorldOriginCallback *m_pWorldOffsetCallback;
	CUtlString m_sWorldGroupname;
	SpawnGroupHandle_t m_hOwner;
	int m_iPriorityLoader;
	ResourceManifestLoadPriority_t m_manifestLoadPriority;
	float m_flTimeoutInterval;
	CUtlString m_sSaveFileName;

	bool m_bCreateClientEntitiesOnLaterConnectingClients;
	bool m_bDontSpawnEntities;
	bool m_bBlockUntilLoaded;
	bool m_bLoadStreamingData;
	bool m_bCreateNewSceneWorld;
	bool m_bManualCompletion;
	bool m_bSetActivePostLoad;
	bool m_bUnk;
	bool m_bLevelTransition;
	bool m_bUnk2;
};

struct SpawnGroupDescReceive_t
{
public:
	bool m_bIsInitialSpawnGroup;
	bool m_bCreateClientOnlyEntities;
	bool m_bIsSynchronousSpawn;

public:
	CCompressedResourceManifest *m_pCompressedResource;

public:
	int32 m_nTickCount;
	bool m_bManifestInComplete;

public:
	SpawnGroupHandle_t m_hSpawnGroupParentHandle;
	WorldGroupId_t m_hWorldGroupId;
};

enum CreateSpawnGroupType_t
{
	CREATE_SPAWN_GROUP_IMMEDIATELY = 0,
	CREATE_SPAWN_GROUP_ASYNCHRONOUSLY,
	CREATE_SPAWN_GROUP_ASYNCHRONOUSLY_CONFIRM_RESOURCES_LOADED,
};

enum ESpawnGroupUnloadOption
{
	kSGUO_None = 0,
	kSGUO_SaveEntities,
	kSGUO_MergedIntoOwner,
};

abstract_class ISpawnGroup
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
	virtual CUtlString Describe() const = 0;

public:
	virtual uint32 GetCreationTick() const = 0;
	virtual void RequestDeferredUnload(bool bAsync = true) = 0;
	virtual uint32 GetDestructionTick() const = 0;

public:
	virtual bool DontSpawnEntities() const = 0;
	virtual uint8 GetUnk2AndLastFlagBits() const = 0;
	virtual SpawnGroupHandle_t GetParentSpawnGroup() const = 0;
	virtual uint32 GetChildSpawnGroupCount() const = 0;
	virtual void GetSpawnGroupDesc(SpawnGroupDesc_t *pDesc) const = 0;

public:
	virtual void FlagManualCreation() = 0;
	virtual bool HasManualCreation() = 0;
	virtual void LoadAllGameResourceManifests(void *) = 0;
	virtual void AddSpawnGroupChild(SpawnGroupHandle_t hSpawnGroup) = 0;
	virtual bool HasSetActivePostLoad() const = 0;
	virtual bool HasUnk2() const = 0;
	virtual bool HasUnk() const = 0;
	virtual void UnkSetter(uint32 n) = 0;
	virtual bool HasLevelTransition() const = 0;
	virtual WorldGroupId_t GetWorldGroupId() const = 0;

	virtual matrix3x4_t ComputeWorldOrigin(const char *pWorldName, SpawnGroupHandle_t hSpawnGroup, IWorld *pWorld) = 0;
	virtual void Release() = 0;
	virtual void OnGameResourceManifestLoaded(HGameResourceManifest hManifest, int nResourceCount, ResourceHandle_t *pResourceHandles) = 0;
	virtual void Init(IResourceManifestRegistry *pResourceManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry) = 0;
	virtual void Shutdown() = 0;
	virtual bool GetLoadStatus() = 0;
	virtual void ForceBlockingLoad() = 0;
	virtual bool ShouldBlockUntilLoaded() const = 0;
	virtual void ServiceBlockingLoads() = 0;
	virtual bool GetEntityPrerequisites(HGameResourceManifest hManifest) = 0;
	virtual bool EntityPrerequisitesSatisfied() = 0;

public:
	virtual bool LoadEntities() = 0;
	virtual void SaveRestoreMap(ILoadingSpawnGroup *pLoading, SpawnGroupHandle_t hSpawnGroup, const matrix3x4a_t &vecSpawnOffset) = 0;
	virtual void SaveRestoreMap2(ILoadingSpawnGroup *pLoading, SpawnGroupHandle_t hSpawnGroup, const matrix3x4a_t &vecSpawnOffset) = 0;
	virtual void SetParentSpawnGroupForChild(SpawnGroupHandle_t hSpawnGroup) = 0;

public:
	virtual void TransferOwnershipOfManifestsTo(ISpawnGroup *pTarget) = 0;

public:
	virtual IGameSpawnGroupMgr *GetSpawnGroupMgr() const = 0;
};

class CMapSpawnGroup
{
public:
	const char *GetWorldName() const
	{
		return m_pSpawnGroup->GetWorldName();
	}

	const char *GetEntityLumpName() const
	{
		return m_pSpawnGroup->GetEntityLumpName();
	}

	const char *GetEntityFilterName() const
	{
		return m_pSpawnGroup->GetEntityFilterName();
	}

	SpawnGroupHandle_t GetSpawnGroupHandle() const
	{
		return m_pSpawnGroup->GetHandle();
	}

	const matrix3x4a_t &GetWorldOffset() const
	{
		return m_pSpawnGroup->GetWorldOffset();
	}

	const char *GetParentNameFixup() const
	{
		return m_pSpawnGroup->GetParentNameFixup();
	}

	const char *GetLocalNameFixup() const
	{
		return m_pSpawnGroup->GetLocalNameFixup();
	}

	SpawnGroupHandle_t GetOwnerSpawnGroup() const
	{
		return m_pSpawnGroup->GetOwnerSpawnGroup();
	}

	ILoadingSpawnGroup *GetLoadingSpawnGroup() const
	{
		return m_pSpawnGroup->GetLoadingSpawnGroup();
	}

	void SetLoadingSpawnGroup(ILoadingSpawnGroup *pLoading)
	{
		m_pSpawnGroup->SetLoadingSpawnGroup(pLoading);
	}

	int GetCreationTick() const
	{
		return m_pSpawnGroup->GetCreationTick();
	}

	bool DontSpawnEntities() const
	{
		return m_pSpawnGroup->DontSpawnEntities();
	}

	void GetSpawnGroupDesc(SpawnGroupDesc_t *pDesc) const
	{
		m_pSpawnGroup->GetSpawnGroupDesc(pDesc);
	}

public:
	ISpawnGroup *GetSpawnGroup() const
	{
		return m_pSpawnGroup;
	}

private:
	ISpawnGroup *m_pSpawnGroup;
	bool m_bSpawnGroupPrecacheDispatched;
	bool m_bSpawnGroupLoadDispatched;
	IPVS *m_pPVS;
	// CUtlVector<SpawnGroupConnectionInfo_t> m_Connections;
};

abstract_class ILoadingSpawnGroup
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
	virtual CKeyValues3Context *GetEntityKeyValuesAllocator() = 0;
	virtual CEntityInstance *CreateEntityToSpawn(SpawnGroupHandle_t hSpawnGroup, const matrix3x4a_t &vecSpawnOffset, CreateSpawnGroupType_t createType, const CEntityKeyValues *pEntityKeyValues) = 0;
	virtual void SpawnEntities() = 0;
	virtual void Release() = 0;
};

abstract_class IGameSpawnGroupMgr
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

	// AMNOTE: Game System related methods
	virtual void YouForgot_DECLARE_GAME_SYSTEM_InYourClassDefinition() = 0;
	virtual void UnkSpawnGroupManagerGameSystemMember1() = 0;
	virtual void GameInit(const EventGameInit_t &msg) = 0;
	virtual void GameShutdown(const EventGameInit_t &msg) = 0;
	virtual void FrameBoundary(const EventGameInit_t &msg) = 0;
	virtual void PreSpawnGroupLoad(const EventPreSpawnGroupLoad_t &msg) = 0;
	virtual ~IGameSpawnGroupMgr() {}
};

// AMNOTE: Short stubs representing game class hierarchy
class CBaseSpawnGroup : public ISpawnGroup, public IComputeWorldOriginCallback, public IGameResourceManifestLoadCompletionCallback
{
};

class CLoadingSpawnGroup : public ILoadingSpawnGroup
{
};

class CSpawnGroupMgrGameSystem : public IGameSpawnGroupMgr, public IGameSystem
{
};

#endif // SPAWNGROUP_MANAGER_H
