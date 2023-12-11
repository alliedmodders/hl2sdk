#ifndef ENTITYSYSTEM_H
#define ENTITYSYSTEM_H

#include "tier0/platform.h"
#include "tier1/keyvalues3.h"
#include "tier1/utlmemory.h"
#include "tier1/utlsymbollarge.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "eiface.h"
#include "baseentity.h"
#include "entityhandle.h"
#include "concreteentitylist.h"
#include "entitydatainstantiator.h"

class CUtlScratchMemoryPool;

class CEntityKeyValues;
class IEntityResourceManifest;
class IEntityPrecacheConfiguration;
class IEntityResourceManifestBuilder;
class ISpawnGroupEntityFilter;
class IHandleEntity;

class CGameEntitySystem;
extern CGameEntitySystem* GameEntitySystem();

typedef void (*EntityResourceManifestCreationCallback_t)(IEntityResourceManifest *, void *);

enum SpawnGroupEntityFilterType_t
{
	SPAWN_GROUP_ENTITY_FILTER_FALLBACK = 0x0,
	SPAWN_GROUP_ENTITY_FILTER_MOD_SPECIFIC = 0x1,
};

enum ClearEntityDatabaseMode_t
{
	CED_NORMAL = 0x0,
	CED_NETWORKEDONLY_AND_DONTCLEARSTRINGPOOL = 0x1,
};

enum ActivateType_t
{
	ACTIVATE_TYPE_INITIAL_CREATION = 0x0,
	ACTIVATE_TYPE_DATAUPDATE_CREATION = 0x1,
	ACTIVATE_TYPE_ONRESTORE = 0x2,
};

enum DataUpdateType_t
{
	DATA_UPDATE_CREATED = 0x0,
	DATA_UPDATE_DATATABLE_CHANGED = 0x1,
};

enum EntityDormancyType_t
{
	ENTITY_NOT_DORMANT = 0x0,
	ENTITY_DORMANT = 0x1,
	ENTITY_SUSPENDED = 0x2,
};

// Entity notifications //

struct EntityNotification_t
{
	CEntityIdentity* m_pEntity;
};

struct EntityDormancyChange_t : EntityNotification_t
{
	EntityDormancyType_t m_nPrevDormancyType;
	EntityDormancyType_t m_nNewDormancyType;
};

struct EntitySpawnInfo_t : EntityNotification_t
{
	const CEntityKeyValues* m_pKeyValues;
	uint64 m_Unk1;
};

struct EntityActivation_t : EntityNotification_t
{
};

struct EntityDeletion_t : EntityNotification_t
{
};

struct PostDataUpdateInfo_t : EntityNotification_t
{
	DataUpdateType_t m_updateType;
};

struct CEntityPrecacheContext
{
	const CEntityKeyValues* m_pKeyValues;
	IEntityPrecacheConfiguration* m_pConfig;
	IEntityResourceManifest* m_pManifest;
};

// Resource data //

struct ResourceNameInfo_t
{
	CUtlSymbolLarge m_ResourceNameSymbol;
};

typedef const ResourceNameInfo_t *ResourceNameHandle_t;

typedef uint16 LoadingResourceIndex_t;

typedef char ResourceTypeIndex_t;

typedef uint32 ExtRefIndex_t;

struct ResourceBindingBase_t
{
	void* m_pData;
	ResourceNameHandle_t m_Name;
	uint16 m_nFlags;
	uint16 m_nReloadCounter;
	ResourceTypeIndex_t m_nTypeIndex;
	uint8 m_nPadding;
	LoadingResourceIndex_t m_nLoadingResource;
	CInterlockedInt m_nRefCount;
	ExtRefIndex_t m_nExtRefHandle;
};

typedef const ResourceBindingBase_t* ResourceHandle_t;

struct SecondaryPrecacheMemberCallback_t
{
	void (CEntityInstance::*pfnPrecache)(ResourceHandle_t hResource, const CEntityPrecacheContext* pContext);
};

class IEntityListener
{
public:
	virtual void OnEntityCreated(CEntityInstance* pEntity) {};
	virtual void OnEntitySpawned(CEntityInstance* pEntity) {};
	virtual void OnEntityDeleted(CEntityInstance* pEntity) {};
	virtual void OnEntityParentChanged(CEntityInstance* pEntity, CEntityInstance* pNewParent) {};
};

struct CEntityResourceManifestLock
{
	IEntityResourceManifestBuilder* m_pBuilder;
	matrix3x4a_t m_vSpawnGroupOffset;
	SpawnGroupHandle_t m_hSpawnGroup;
	IEntityPrecacheConfiguration* m_pConfig;
	IEntityResourceManifest* m_pManifest;
	bool m_bIsLocked;
	bool m_bPrecacheEnable;
};

// Manifest executor //

abstract_class IEntityResourceManifestBuilder
{
public:
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, int nCount, const EntitySpawnInfo_t *pEntities, const matrix3x4a_t *vWorldOffset, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, int iCount, const CEntityKeyValues *pKeyValues, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, const CUtlVector<const CEntityKeyValues*>* pEntityKeyValues, const char* pFilterName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(const char* pManifestNameOrGroupName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(EntityResourceManifestCreationCallback_t callback, void* pContext, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifestForEntity(const char* pEntityDesignerName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, CUtlScratchMemoryPool* pKeyValuesMemoryPool) = 0;
	virtual void		InvokePrecacheCallback(ResourceHandle_t hResource, const EntitySpawnInfo_t* info, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, SecondaryPrecacheMemberCallback_t callback) = 0;
	virtual void		AddRefKeyValues(const CEntityKeyValues* pKeyValues) = 0;
	virtual void		ReleaseKeyValues(const CEntityKeyValues* pKeyValues) = 0;
	virtual void		LockResourceManifest(bool bLock, CEntityResourceManifestLock* const context) = 0;
};

// Size: 0x1510 | 0x1540 (from constructor)
class CEntitySystem : public IEntityResourceManifestBuilder
{
public:
	virtual				~CEntitySystem() = 0;
	virtual void		ClearEntityDatabase(ClearEntityDatabaseMode_t eMode) = 0;
	virtual void		FindEntityProcedural(const char* szName, CEntityInstance* pSearchingEntity, CEntityInstance* pActivator, CEntityInstance* pCaller) = 0;
	virtual void		OnEntityParentChanged(CEntityInstance* pEntity, CEntityInstance* pNewParent) = 0; // empty function
	virtual void		OnAddEntity(CEntityInstance* pEnt, CEntityHandle handle) = 0; // empty function
	virtual void		OnRemoveEntity(CEntityInstance* pEnt, CEntityHandle handle) = 0; // empty function
	virtual int			GetSpawnGroupWorldId(SpawnGroupHandle_t hSpawnGroup) = 0; // returns 0
	virtual void		Spawn(int nCount, const EntitySpawnInfo_t* pInfo) = 0;
	virtual void		Activate(int nCount, const EntityActivation_t* pActivates, ActivateType_t activateType) = 0;
	virtual void		PostDataUpdate(int nCount, const PostDataUpdateInfo_t *pInfo) = 0;
	virtual void		OnSetDormant(int nCount, const EntityDormancyChange_t* pInfo, bool bNotifyAddedToPVS) = 0;
	virtual void		UpdateOnRemove(int nCount, const EntityDeletion_t *pDeletion) = 0;

public:
	CBaseEntity* GetBaseEntity(CEntityIndex entnum);

	CBaseEntity* GetBaseEntity(const CEntityHandle& hEnt);

	void AddEntityKeyValuesAllocatorRef();
	void ReleaseEntityKeyValuesAllocatorRef();

	CKeyValues3Context* GetEntityKeyValuesAllocator();

private:
	IEntityResourceManifest* m_pCurrentManifest;
public:
	CConcreteEntityList m_EntityList;
	// CConcreteEntityList seems to be correct but m_CallQueue supposedly starts at offset 2664, which is... impossible?
	// Based on CEntitySystem::CEntitySystem found via string "MaxNonNetworkableEntities"

private:
	uint8 pad2696[0x150];
#ifdef PLATFORM_POSIX
	uint8 pad3032[0x10];
#endif

	int32 m_nEntityKeyValuesAllocatorRefCount; // 3032 | 3048

#ifdef PLATFORM_POSIX
	uint8 pad3052[0xFC];
#else
	uint8 pad3036[0xF4];
#endif

	CKeyValues3Context m_pEntityKeyValuesAllocator; // 3280 | 3304

#ifdef PLATFORM_POSIX
	uint8 pad4888[0x228];
#else
	uint8 pad4864[0x210];
#endif
};

// Size: 0x1580 | 0x15B0 (from constructor)
class CGameEntitySystem : public CEntitySystem
{
	struct SpawnGroupEntityFilterInfo_t
	{
		ISpawnGroupEntityFilter* m_pFilter;
		SpawnGroupEntityFilterType_t m_nType;
	};
	//typedef SpawnGroupEntityFilterInfo_t CUtlMap<char const*, SpawnGroupEntityFilterInfo_t, int, bool (*)(char const* const&, char const* const&)>::ElemType_t;

public:
	virtual				~CGameEntitySystem() = 0;

	void AddListenerEntity(IEntityListener* pListener);
	void RemoveListenerEntity(IEntityListener* pListener);

public:
	int m_iMaxNetworkedEntIndex; // 5392 | 5440
	int m_iNetworkedEntCount; // 5396 | 5444
	int m_iNonNetworkedSavedEntCount; // 5400 | 5448
	// int m_iNumEdicts; // This is no longer referenced in the server binary
	CUtlDict<SpawnGroupEntityFilterInfo_t> m_spawnGroupEntityFilters; // 5408 | 5456
	CUtlVector<IEntityListener*> m_entityListeners; // 5448 | 5496

private:
	uint8 pad5480[0x20];
};

#endif // ENTITYSYSTEM_H