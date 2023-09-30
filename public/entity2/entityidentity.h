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

class CEntityIdentity;

struct ChangeAccessorFieldPathIndex_t
{
	int16 m_Value;
};

typedef CUtlStringToken WorldGroupId_t;

class CEntityInstance : public IHandleEntity
{
public:
	// MNetworkDisable
	CUtlSymbolLarge m_iszPrivateVScripts; // 0x8	
	// MNetworkEnable
	// MNetworkPriority "56"
	CEntityIdentity* m_pEntity; // 0x10
private:
	void* m_hPrivateScope; // 0x18 - CEntityPrivateScriptScope
	uint8 unknown[0x8]; // 0x20
public:
	// MNetworkEnable
	// MNetworkDisable
	CScriptComponent* m_CScriptComponent; // 0x28	
};


// Size: 0x78
class CEntityIdentity
{
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
	uint32 m_flags; // 0x30	
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