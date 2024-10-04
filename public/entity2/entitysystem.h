#ifndef ENTITYSYSTEM_H
#define ENTITYSYSTEM_H

#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include "tier1/generichash.h"
#include "tier1/keyvalues3.h"
#include "tier1/utlmemory.h"
#include "tier1/utlsymbollarge.h"
#include "tier1/utlvector.h"
#include "tier1/utldict.h"
#include "tier1/utlmap.h"
#include "tier1/utlhashtable.h"
#include "tier1/utldelegate.h"
#include "tier1/utlscratchmemory.h"
#include "tier1/utlstring.h"
#include "networksystem/inetworkserializer.h"
#include "vscript/ivscript.h"
#include "eiface.h"
#include "resourcefile/resourcetype.h"
#include "entityhandle.h"
#include "concreteentitylist.h"
#include "entitydatainstantiator.h"

class CKeyValues3Context;
class CEntityClass;
class CEntityComponentHelper;
class CEntityKeyValues;
class CGameEntitySystem;
class IEntityIONotify;
class INetworkFieldChangedEventQueue;
class INetworkFieldScratchData;
class IFieldChangeLimitSpew;
class IEntity2SaveRestore;
class IEntity2Networkables;
class IEntityResourceManifest;
class IEntityPrecacheConfiguration;
class IEntityResourceManifestBuilder;
class ISpawnGroupEntityFilter;
class IHandleEntity;
struct ComponentUnserializerFieldInfo_t;

extern CGameEntitySystem* GameEntitySystem();

typedef void (*EntityResourceManifestCreationCallback_t)(IEntityResourceManifest *, void *);

enum EntityIOTargetType_t
{
	ENTITY_IO_TARGET_INVALID = -1,
	ENTITY_IO_TARGET_CLASSNAME = 0,
	ENTITY_IO_TARGET_CLASSNAME_DERIVES_FROM = 1,
	ENTITY_IO_TARGET_ENTITYNAME = 2,
	ENTITY_IO_TARGET_CONTAINS_COMPONENT = 3,
	ENTITY_IO_TARGET_SPECIAL_ACTIVATOR = 4,
	ENTITY_IO_TARGET_SPECIAL_CALLER = 5,
	ENTITY_IO_TARGET_EHANDLE = 6,
	ENTITY_IO_TARGET_ENTITYNAME_OR_CLASSNAME = 7,
};

enum EntityIterType_t
{
	ENTITY_ITER_OVER_ACTIVE = 0x0,
	ENTITY_ITER_OVER_DORMANT = 0x1,
};

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

// Event queue //

struct EventQueuePrioritizedEvent_t
{
	float m_flFireTime;
	EntityIOTargetType_t m_eTargetType;
	CUtlSymbolLarge m_iTarget;
	CUtlSymbolLarge m_iTargetInput;
	CEntityHandle m_pActivator;
	CEntityHandle m_pCaller;
	int m_iOutputID;
	CEntityHandle m_pEntTarget; // a pointer to the entity to target; overrides m_iTarget

	variant_t m_VariantValue; // variable-type parameter

	EventQueuePrioritizedEvent_t *m_pNext;
	EventQueuePrioritizedEvent_t *m_pPrev;
};

class CEventQueue
{
public:
	CThreadMutex m_Mutex;
	EventQueuePrioritizedEvent_t m_Events;
	int m_iListCount;
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
	virtual void		BuildResourceManifest(SpawnGroupHandle_t hSpawnGroup, const CUtlVector<const CEntityKeyValues*>* pEntityKeyValues, const char* pFilterName /* Usually, "mapload" and "cs_respawn"/"respawn". See CSpawnGroupEntityFilterRegistrar ctors */, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(const char* pManifestNameOrGroupName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifest(EntityResourceManifestCreationCallback_t callback, void* pContext, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest) = 0;
	virtual void		BuildResourceManifestForEntity(const char* pEntityDesignerName, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, CKeyValues3Context* pEntityAllocator) = 0;
	virtual void		InvokePrecacheCallback(ResourceHandle_t hResource, const EntitySpawnInfo_t* info, IEntityPrecacheConfiguration* pConfig, IEntityResourceManifest* pResourceManifest, SecondaryPrecacheMemberCallback_t callback) = 0;
	virtual void		AddRefKeyValues(const CEntityKeyValues* pKeyValues) = 0;
	virtual void		ReleaseKeyValues(const CEntityKeyValues* pKeyValues) = 0;
	virtual void		LockResourceManifest(bool bLock, CEntityResourceManifestLock* const context) = 0;
};

// Size: 0x1510 | 0x1540 (from constructor)
class CEntitySystem : public IEntityResourceManifestBuilder
{
	enum
	{
		MAX_ACCESSORS = 32,
	};

	struct CreationInfo_t
	{
		CEntityHandle m_hEnt;
		CEntityInstance* m_pEnt;
		const CEntityKeyValues* m_pKeyValues;
		bool m_Unk1;
	};

	struct DestructionInfo_t
	{
		CEntityIdentity* m_pEnt;
	};

	struct DormancyChangeInfo_t
	{
		CEntityHandle m_hEnt;
		bool m_bInPVS;
	};

	struct MurmurHash2HashFunctor
	{ 
		unsigned int operator()( uint32 n ) const { return MurmurHash2( &n, sizeof( uint32 ), 0x3501A674 ); } 
	};

public:
	virtual						~CEntitySystem() = 0;
	virtual void				unk_001() = 0;
	virtual void				ClearEntityDatabase(ClearEntityDatabaseMode_t eMode) = 0;
	virtual CEntityInstance*	FindEntityProcedural(const char* szName, CEntityInstance* pSearchingEntity = nullptr, CEntityInstance* pActivator = nullptr, CEntityInstance* pCaller = nullptr) = 0;
	virtual void				OnEntityParentChanged(CEntityInstance* pEntity, CEntityInstance* pNewParent) = 0; // empty function
	virtual void				OnAddEntity(CEntityInstance* pEnt, CEntityHandle handle) = 0; // empty function
	virtual void				OnRemoveEntity(CEntityInstance* pEnt, CEntityHandle handle) = 0; // empty function
	virtual WorldGroupId_t		GetSpawnGroupWorldId(SpawnGroupHandle_t hSpawnGroup) = 0;
	virtual void				Spawn(int nCount, const EntitySpawnInfo_t* pInfo) = 0;
	virtual void				Activate(int nCount, const EntityActivation_t* pActivates, ActivateType_t activateType) = 0;
	virtual void				PostDataUpdate(int nCount, const PostDataUpdateInfo_t *pInfo) = 0;
	virtual void				OnSetDormant(int nCount, const EntityDormancyChange_t* pInfo, bool bNotifyAddedToPVS) = 0;
	virtual void				UpdateOnRemove(int nCount, const EntityDeletion_t *pDeletion) = 0;

public:
	CEntityIdentity *GetEntityIdentity( CEntityIndex entnum );
	CEntityIdentity *GetEntityIdentity( const CEntityHandle &hEnt );

	inline CEntityInstance *GetEntityInstance( CEntityIdentity *ident ) { return ident ? ident->m_pInstance : nullptr; }
	inline CEntityInstance *GetEntityInstance( CEntityIndex entnum ) { return GetEntityInstance( GetEntityIdentity( entnum ) ); }
	inline CEntityInstance *GetEntityInstance( const CEntityHandle &hEnt ) { return GetEntityInstance( GetEntityIdentity( hEnt ) ); }

	inline void AddEntityKeyValuesAllocatorRef() { ++m_nEntityKeyValuesAllocatorRefCount; }
	inline void ReleaseEntityKeyValuesAllocatorRef()
	{
		if (--m_nEntityKeyValuesAllocatorRefCount == 0) 
			m_EntityKeyValuesAllocator.Clear();
	}

	inline CKeyValues3Context* GetEntityKeyValuesAllocator() { return &m_EntityKeyValuesAllocator; }

	// Search for an entity class by its C++ name, case-insensitive
	CEntityClass* FindClassByName(const char* szClassName);
	
	// Search for an entity class by its designer name, case-insensitive
	CEntityClass* FindClassByDesignName(const char* szClassName);

	CEntityHandle FindFirstEntityHandleByName(const char* szName, WorldGroupId_t hWorldGroupId = WorldGroupId_t());

	CUtlSymbolLarge AllocPooledString(const char* pString);
	CUtlSymbolLarge FindPooledString(const char* pString);

private:
	IEntityResourceManifest* m_pCurrentManifest;
public:
	CConcreteEntityList m_EntityList;
	// CConcreteEntityList seems to be correct but m_CallQueue supposedly starts at offset 2664, which is... impossible?
	// Based on CEntitySystem::CEntitySystem found via string "MaxNonNetworkableEntities"

public:
	CUtlString m_sEntSystemName; // 2696
	CUtlMap<const char*, CEntityClass*, uint16, CDefFastCaselessStringLess> m_entClassesByCPPClassname; // 2704
	CUtlMap<const char*, CEntityClass*, uint16, CDefFastCaselessStringLess> m_entClassesByClassname; // 2736
	CUtlMap<const char*, CEntityComponentHelper*, uint16, CDefFastCaselessStringLess> m_entityComponentHelpers; // 2768
	CUtlMap<CUtlSymbolLarge, CUtlVector<CEntityHandle>*, uint16, CDefLess<CUtlSymbolLarge>> m_entityNames; // 2800
	CEventQueue m_EventQueue; // 2832
	CUtlVectorFixedGrowable<IEntityIONotify*, 2> m_entityIONotifiers; // 2968 | 2984
	int m_nSuppressDormancyChangeCount; // 3008 | 3024
	NetworkSerializationMode_t m_eNetworkSerializationMode; // 3012 | 3028
	int m_nExecuteQueuedCreationDepth; // 3016 | 3032
	int m_nExecuteQueuedDeletionDepth; // 3020 | 3036
	int m_nSuppressDestroyImmediateCount; // 3024 | 3040
	int m_nSuppressAutoDeletionExecutionCount; // 3028 | 3044
	int m_nEntityKeyValuesAllocatorRefCount; // 3032 | 3048
	float m_flChangeCallbackSpewThreshold; // 3036 | 3052
	bool m_Unk1; // 3040 | 3056
	bool m_Unk2; // 3041 | 3057
	bool m_Unk3; // 3042 | 3058
	bool m_bEnableAutoDeletionExecution; // 3043 | 3059
	bool m_Unk4; // 3044 | 3060
	bool m_Unk5; // 3045 | 3061
	bool m_Unk6; // 3046 | 3062
	CUtlVector<CreationInfo_t> m_queuedCreations; // 3048 | 3064
	CUtlVector<PostDataUpdateInfo_t> m_queuedPostDataUpdates; // 3072 | 3088
	CUtlVector<DestructionInfo_t> m_queuedDeletions; // 3096 | 3112
	CUtlVector<DestructionInfo_t> m_queuedDeferredDeletions; // 3120 | 3136
	CUtlVector<DestructionInfo_t> m_queuedDeallocations; // 3144 | 3160
	CUtlVector<DormancyChangeInfo_t> m_queuedDormancyChanges; // 3168 | 3184
	CUtlDelegate<void ( int, const EntitySpawnInfo_t* )> m_EntityPostSpawnCallback; // 3192 | 3208
	INetworkFieldChangedEventQueue* m_pNetworkFieldChangedEventQueue; // 3208 | 3232
	INetworkFieldScratchData* m_pNetworkFieldScratchData; // 3216 | 3240
	IFieldChangeLimitSpew* m_pFieldChangeLimitSpew; // 3224 | 3248
	CUtlHashtable<fieldtype_t, CUtlDelegate<bool ( IParsingErrorListener*, CEntityInstance*, void*, const ComponentUnserializerFieldInfo_t*, const KeyValues3* )>, MurmurHash2HashFunctor> m_DataDescKeyUnserializers; // 3232 | 3256
	CUtlScratchMemoryPool m_ComponentUnserializerInfoAllocator; // 3264 | 3288
	CKeyValues3Context m_EntityKeyValuesAllocator; // 3280 | 3304
	CUtlSymbolTableLargeMT_CI m_Symbols; // 4864 | 4888
	SpawnGroupHandle_t m_hActiveSpawnGroup; // 5048 | 5088
	matrix3x4a_t m_vSpawnOriginOffset; // 5056 | 5104
	IEntityDataInstantiator* m_Accessors[ MAX_ACCESSORS ]; // 5104 | 5152
	CUtlHashtable<CUtlString, void*> m_EntityMaterialAttributes; // 5360 | 5408
};

// Size: 0x1570 | 0x15A0 (from constructor)
class CGameEntitySystem : public CEntitySystem
{
	struct SpawnGroupEntityFilterInfo_t
	{
		ISpawnGroupEntityFilter* m_pFilter;
		SpawnGroupEntityFilterType_t m_nType;
	};
	//typedef SpawnGroupEntityFilterInfo_t CUtlMap<char const*, SpawnGroupEntityFilterInfo_t, int, bool (*)(char const* const&, char const* const&)>::ElemType_t;

public:
	virtual ~CGameEntitySystem() = 0;

	void AddListenerEntity(IEntityListener* pListener);
	void RemoveListenerEntity(IEntityListener* pListener);

public:
	int m_iMaxNetworkedEntIndex; // 5392 | 5440
	int m_iNetworkedEntCount; // 5396 | 5444
	int m_iNonNetworkedSavedEntCount; // 5400 | 5448
	// int m_iNumEdicts; // This is no longer referenced in the server binary
	CUtlDict<SpawnGroupEntityFilterInfo_t> m_spawnGroupEntityFilters; // 5408 | 5456
	CUtlVector<IEntityListener*> m_entityListeners; // 5448 | 5496
	IEntity2SaveRestore* m_pEntity2SaveRestore; // 5472 | 5520
	IEntity2Networkables* m_pEntity2Networkables; // 5480 | 5528
};

abstract_class IEntityFindFilter
{
public:
	virtual bool ShouldFindEntity(CEntityInstance* pEnt) = 0;
};

class EntityInstanceIter_t
{
public:
	EntityInstanceIter_t(IEntityFindFilter* pFilter = nullptr, EntityIterType_t eIterType = ENTITY_ITER_OVER_ACTIVE);

	CEntityInstance* First();
	CEntityInstance* Next();

	inline void SetWorldGroupId(WorldGroupId_t hWorldGroupId) { m_hWorldGroupId = hWorldGroupId; }

private:
	CEntityIdentity*	m_pCurrentEnt;
	IEntityFindFilter*	m_pFilter;
	EntityIterType_t	m_eIterType;
	WorldGroupId_t		m_hWorldGroupId;
};

class EntityInstanceByNameIter_t
{
public:
	EntityInstanceByNameIter_t(const char* szName, CEntityInstance* pSearchingEntity = nullptr, CEntityInstance* pActivator = nullptr, CEntityInstance* pCaller = nullptr, IEntityFindFilter* pFilter = nullptr, EntityIterType_t eIterType = ENTITY_ITER_OVER_ACTIVE);

	CEntityInstance* First();
	CEntityInstance* Next();

	inline void SetWorldGroupId(WorldGroupId_t hWorldGroupId) { m_hWorldGroupId = hWorldGroupId; }

private:
	CEntityIdentity*			m_pCurrentEnt;
	IEntityFindFilter*			m_pFilter;
	EntityIterType_t			m_eIterType;
	WorldGroupId_t				m_hWorldGroupId;
	const char*					m_pszEntityName;
	CUtlVector<CEntityHandle>*	m_pEntityHandles;
	int							m_nCurEntHandle;
	int							m_nNumEntHandles;
	CEntityInstance*			m_pProceduralEnt;
};

class EntityInstanceByClassIter_t
{
public:
	EntityInstanceByClassIter_t(const char* szClassName, IEntityFindFilter* pFilter = nullptr, EntityIterType_t eIterType = ENTITY_ITER_OVER_ACTIVE);
	EntityInstanceByClassIter_t(CEntityInstance* pStart, const char* szClassName, IEntityFindFilter* pFilter = nullptr, EntityIterType_t eIterType = ENTITY_ITER_OVER_ACTIVE);

	CEntityInstance* First();
	CEntityInstance* Next();

	inline void SetWorldGroupId(WorldGroupId_t hWorldGroupId) { m_hWorldGroupId = hWorldGroupId; }

private:
	CEntityIdentity*	m_pCurrentEnt;
	IEntityFindFilter*	m_pFilter;
	EntityIterType_t	m_eIterType;
	WorldGroupId_t		m_hWorldGroupId;
	const char*			m_pszClassName;
	CEntityClass*		m_pEntityClass;
};

#endif // ENTITYSYSTEM_H