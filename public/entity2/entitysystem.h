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

struct GameTime_t
{
public:
	GameTime_t( float value = 0.0f ) : m_Value( value ) {}

	float GetTime() const { return m_Value; }
	void SetTime( float value ) { m_Value = value; }

private:
	float m_Value;
};

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
	int m_unk001;
	GameTime_t m_flFireTime;
	EntityIOTargetType_t m_eTargetType;
	CUtlSymbolLarge m_iTarget;
	CUtlSymbolLarge m_iTargetInput;
	CEntityHandle m_pActivator;
	CEntityHandle m_pCaller;
	int m_iOutputID;
	CEntityHandle m_pEntTarget; // a pointer to the entity to target; overrides m_iTarget

	variant_t m_VariantValue; // variable-type parameter

	void *m_unk101;
	KeyValues3 m_KV3;
	KeyValues3::Data_t m_KV3Data;

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

class CEntitySystem : public IEntityResourceManifestBuilder
{
private:
	enum
	{
		MAX_ACCESSORS = 32,
	};

	struct CreationInfo_t
	{
		CEntityHandle m_hEnt;
		CEntityInstance* m_pEnt;
		const CEntityKeyValues* m_pKeyValues;

		// AMNOTE: Seems to be checked along side m_pKeyValues before its usage
		bool m_bEntityKeyValuesInitialized;
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

	typedef CUtlDelegate<bool( IParsingErrorListener *, CEntityInstance *, void *, const ComponentUnserializerFieldInfo_t *, const KeyValues3 * )> KeyUnserializerDelegate;

public:
	virtual						~CEntitySystem() = 0;

	// This function is called in CGameEntitySystem::ProcessEventQueue()
	virtual GameTime_t			unk_001( int ) = 0;

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
	IEntityResourceManifest *m_pCurrentManifest;

public:
	CConcreteEntityList m_EntityList;
	CUtlString m_sEntSystemName;

	CUtlMap<const char*, CEntityClass*, uint16, CDefFastCaselessStringLess> m_entClassesByCPPClassname;
	CUtlMap<const char*, CEntityClass*, uint16, CDefFastCaselessStringLess> m_entClassesByClassname;
	CUtlMap<const char*, CEntityComponentHelper*, uint16, CDefFastCaselessStringLess> m_entityComponentHelpers;
	CUtlMap<CUtlSymbolLarge, CUtlVector<CEntityHandle>*, uint16, CDefLess<CUtlSymbolLarge>> m_entityNames;

	CEventQueue m_EventQueue;
	CUtlVectorFixedGrowable<IEntityIONotify*, 2> m_entityIONotifiers;
	int m_nSuppressDormancyChangeCount;
	NetworkSerializationMode_t m_eNetworkSerializationMode;
	int m_nExecuteQueuedCreationDepth;
	int m_nExecuteQueuedDeletionDepth;
	int m_nSuppressDestroyImmediateCount;
	int m_nSuppressAutoDeletionExecutionCount;
	int m_nEntityKeyValuesAllocatorRefCount;
	float m_flChangeCallbackSpewThreshold;

	bool m_Unk1;
	bool m_Unk2;
	bool m_Unk3;
	bool m_bEnableAutoDeletionExecution;
	bool m_Unk4;
	bool m_Unk5;
	bool m_Unk6;

	CUtlVector<CreationInfo_t> m_queuedCreations;
	CUtlVector<PostDataUpdateInfo_t> m_queuedPostDataUpdates;
	CUtlVector<DestructionInfo_t> m_queuedDeletions;
	CUtlVector<DestructionInfo_t> m_queuedDeferredDeletions;
	CUtlVector<DestructionInfo_t> m_queuedDeallocations;
	CUtlVector<DormancyChangeInfo_t> m_queuedDormancyChanges;

	CUtlDelegate<void ( int, const EntitySpawnInfo_t* )> m_EntityPostSpawnCallback;
	INetworkFieldChangedEventQueue* m_pNetworkFieldChangedEventQueue;
	INetworkFieldScratchData* m_pNetworkFieldScratchData;
	IFieldChangeLimitSpew* m_pFieldChangeLimitSpew;
	CUtlHashtable<fieldtype_t, KeyUnserializerDelegate, MurmurHash2HashFunctor> m_DataDescKeyUnserializers;
	CUtlScratchMemoryPool m_ComponentUnserializerInfoAllocator;
	CKeyValues3Context m_EntityKeyValuesAllocator;
	CUtlSymbolTableLargeMT_CI m_Symbols;
	SpawnGroupHandle_t m_hActiveSpawnGroup;
	matrix3x4a_t m_vSpawnOriginOffset;
	IEntityDataInstantiator* m_Accessors[ MAX_ACCESSORS ];
	CUtlHashtable<CUtlString, void*> m_EntityMaterialAttributes;
};

class CGameEntitySystem : public CEntitySystem
{
	struct SpawnGroupEntityFilterInfo_t
	{
		ISpawnGroupEntityFilter* m_pFilter;
		SpawnGroupEntityFilterType_t m_nType;
	};

public:
	virtual ~CGameEntitySystem() = 0;

	void AddListenerEntity(IEntityListener* pListener);
	void RemoveListenerEntity(IEntityListener* pListener);

public:
	int m_iNetworkedEntCount;
	int m_iNonNetworkedSavedEntCount;
	CUtlDict<SpawnGroupEntityFilterInfo_t> m_spawnGroupEntityFilters;
	CUtlVector<IEntityListener*> m_entityListeners;
	IEntity2SaveRestore* m_pEntity2SaveRestore;
	IEntity2Networkables* m_pEntity2Networkables;
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