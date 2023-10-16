#ifndef ENTITYIDENTITY_H
#define ENTITYIDENTITY_H

#if _WIN32
#pragma once
#endif

#define MAX_ENTITIES_IN_LIST 512
#define MAX_ENTITY_LISTS 64 // 0x3F
#define MAX_TOTAL_ENTITIES MAX_ENTITIES_IN_LIST * MAX_ENTITY_LISTS // 0x8000

#include "eiface.h"
#include "entitycomponent.h"
#include "entityhandle.h"

class CEntityInstance;

struct ChangeAccessorFieldPathIndex_t
{
	int16 m_Value;
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
};

// Size: 0x78
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

public:
	CEntityInstance* m_pInstance; // 0x0
private:
	void* m_pClass; // 0x8 - CEntityClass
public:
	CEntityHandle m_EHandle; // 0x10
	int32 m_nameStringableIndex; // 0x14	
	CUtlSymbolLarge m_name; // 0x18
	CUtlSymbolLarge m_designerName; // 0x20
private:
	uint64 m_hPublicScope; // 0x28 - CEntityPublicScriptScope
public:
	EntityFlags_t m_flags; // 0x30	
private:
	SpawnGroupHandle_t m_hSpawnGroup; // 0x34
public:
	WorldGroupId_t m_worldGroupId; // 0x38
	uint32 m_fDataObjectTypes; // 0x3c	
	ChangeAccessorFieldPathIndex_t m_PathIndex; // 0x40
private:
	uint16 m_Padding; // 0x42
	void* m_pAttributes; // 0x48 - CUtlObjectAttributeTable<CEntityIdentity, CUtlStringToken>
	void* m_pRenderAttrs; // 0x50 - CRenderAttributesDoubleBuffered
public:
	CEntityIdentity* m_pPrev; // 0x58	
	CEntityIdentity* m_pNext; // 0x60	
	CEntityIdentity* m_pPrevByClass; // 0x68	
	CEntityIdentity* m_pNextByClass; // 0x70	
};

#endif // ENTITYIDENTITY_H