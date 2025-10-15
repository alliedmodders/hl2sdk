#ifndef ENTITYIDENTITY_H
#define ENTITYIDENTITY_H

#if _WIN32
#pragma once
#endif

#define MAX_ENTITIES_IN_LIST 512
#define MAX_ENTITY_LISTS 64 // 0x3F
#define MAX_TOTAL_ENTITIES MAX_ENTITIES_IN_LIST * MAX_ENTITY_LISTS // 0x8000

#include "tier1/utlstring.h"
#include "tier1/utlsymbollarge.h"
#include "entity2/entitycomponent.h"
#include "entityhandle.h"

class CEntityClass;
class CEntityInstance;

struct ChangeAccessorFieldPathIndex_t
{
	ChangeAccessorFieldPathIndex_t() { m_Value = -1; }
	ChangeAccessorFieldPathIndex_t( int32 value ) { m_Value = value; }
	
	int32 m_Value;
};

typedef uint32 SpawnGroupHandle_t;
typedef CUtlStringToken WorldGroupId_t;

class CEntityIndex
{
public:
	CEntityIndex( int index )
	{
		_index = index;
	}

	int Get() const
	{
		return _index;
	}

	bool operator==( const CEntityIndex &other ) const { return other._index == _index; }
	bool operator!=( const CEntityIndex &other ) const { return other._index != _index; }
	
private:
	int _index;
};

enum EntityFlags_t : uint32
{
	EF_IS_INVALID_EHANDLE = 0x1,
	EF_SPAWN_IN_PROGRESS = 0x2,
	EF_IN_STAGING_LIST = 0x4,
	EF_IN_POST_DATA_UPDATE = 0x8,
	EF_DELETE_IN_PROGRESS = 0x10,
	EF_IN_STASIS = 0x20,
	EF_IS_ISOLATED_ALLOCATION_NETWORKABLE = 0x40,
	EF_IS_DORMANT = 0x80,
	EF_IS_PRE_SPAWN = 0x100,
	EF_MARKED_FOR_DELETE = 0x200,
	EF_IS_CONSTRUCTION_IN_PROGRESS = 0x400,
	EF_IS_ISOLATED_ALLOCATION = 0x800,
	EF_HAS_BEEN_UNSERIALIZED = 0x1000,
	EF_IS_SUSPENDED = 0x2000,
	EF_IS_ANONYMOUS_ALLOCATION = 0x4000,
	EF_SUSPEND_OUTSIDE_PVS = 0x8000,
};

class CEntityIdentity
{
public:
	inline CEntityHandle GetRefEHandle() const
	{
		CEntityHandle handle = m_EHandle;
		handle.m_Parts.m_Serial -= (m_flags & EF_IS_INVALID_EHANDLE);

		return handle;
	}
	
	inline const char *GetClassname() const
	{
		return m_designerName.String();
	}

	inline CEntityIndex GetEntityIndex() const
	{
		return m_EHandle.GetEntryIndex();
	}

	inline SpawnGroupHandle_t GetSpawnGroup() const
	{
		return m_hSpawnGroup;
	}

	bool NameMatches( const char* pszNameOrWildcard ) const;
	bool ClassMatches( const char* pszClassOrWildcard ) const;

public:
	CEntityInstance* m_pInstance;
	CEntityClass* m_pClass;
	CEntityHandle m_EHandle;
	int32 m_nameStringableIndex;
	CUtlSymbolLarge m_name;
	CUtlSymbolLarge m_designerName;
private:
	uint64 m_hPublicScope; // CEntityPublicScriptScope
public:
	EntityFlags_t m_flags;
private:
	SpawnGroupHandle_t m_hSpawnGroup;
public:
	WorldGroupId_t m_worldGroupId;
	uint32 m_fDataObjectTypes;
	ChangeAccessorFieldPathIndex_t m_PathIndex;
private:
	void* m_pAttributes; // CUtlObjectAttributeTable<CEntityIdentity, CUtlStringToken>
public:
	CEntityIdentity* m_pPrev;
	CEntityIdentity* m_pNext;
	CEntityIdentity* m_pPrevByClass;
	CEntityIdentity* m_pNextByClass;
};

#endif // ENTITYIDENTITY_H