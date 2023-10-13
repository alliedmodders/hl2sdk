#ifndef ENTITYSYSTEM_H
#define ENTITYSYSTEM_H

#include "tier0/platform.h"
#include "tier1/utlmemory.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "eiface.h"
#include "baseentity.h"
#include "entityhandle.h"
#include "concreteentitylist.h"
#include "entitydatainstantiator.h"

class CEntityKeyValues;
class IEntityResourceManifest;
class IEntityPrecacheConfiguration;
class IEntityResourceManifestBuilder;
class ISpawnGroupEntityFilter;

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

abstract_class IEntityResourceManifestBuilder
{
public:
	virtual void		BuildResourceManifest(EntityResourceManifestCreationCallback_t callback, void* pContext, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(const char* pManifestNameOrGroupName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, const CUtlVector<const CEntityKeyValues*, CUtlMemory<const CEntityKeyValues*, int> >* pEntityKeyValues, const char* pFilterName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, int nEntityKeyValueCount, const CEntityKeyValues** ppEntityKeyValues, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		UnknownFunc004() = 0; // Another BuildResourceManifest function in 2018, but it is quite different now
	virtual void		BuildResourceManifestForEntity(uint64 unknown1, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, uint64 unknown2) = 0;
	virtual void		InvokePrecacheCallback(void* hResource /* ResourceHandle_t */, const EntitySpawnInfo_t* const info, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, char* unk, void* callback /* SecondaryPrecacheMemberCallback_t */) = 0;
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

private:
	IEntityResourceManifest* m_pCurrentManifest;
public:
	CConcreteEntityList m_EntityList;
	// CConcreteEntityList seems to be correct but m_CallQueue supposedly starts at offset 2664, which is... impossible?
	// Based on CEntitySystem::CEntitySystem found via string "MaxNonNetworkableEntities"

private:
	uint8 pad2696[0xa88];
#ifdef PLATFORM_POSIX
	uint8 pad5392[0x30];
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