#ifndef ENTITYCLASS_H
#define ENTITYCLASS_H

#if _WIN32
#pragma once
#endif

#include "tier1/utlsymbollarge.h"
#include "tier1/utlvector.h"
#include "entity2/entitycomponent.h"
#include "entityhandle.h"
#include "networksystem/iflattenedserializers.h"

#define FENTCLASS_NON_NETWORKABLE		(1 << 0) // If the EntityClass is non-networkable
#define FENTCLASS_ALIAS					(1 << 1) // If the EntityClass is an alias
#define FENTCLASS_NO_SPAWNGROUP			(1 << 2) // Don't use spawngroups when creating entity
#define FENTCLASS_FORCE_EHANDLE			(1 << 3) // Forces m_requiredEHandle on created entities
#define FENTCLASS_UNK004				(1 << 4)
#define FENTCLASS_SUSPEND_OUTSIDE_PVS	(1 << 5) // Suspend entities outside of PVS
#define FENTCLASS_ANONYMOUS				(1 << 6) // If the EntityClass is anonymous
#define FENTCLASS_UNK007				(1 << 7)
#define FENTCLASS_UNK008				(1 << 8)
#define FENTCLASS_UNK009				(1 << 9)
#define FENTCLASS_FORCE_WORLDGROUPID	(1 << 10) // Forces worldgroupid to be 1 on created entities

class CSchemaClassInfo;
class CEntityClass;
class CEntityIdentity;
class CEntitySharedPulseSignature;
class ServerClass;
struct EntInput_t;
struct EntOutput_t;
struct datamap_t;

struct EntClassComponentOverride_t
{
	const char* pszBaseComponent;
	const char* pszOverrideComponent;
};

class CEntityClassInfo
{
public:
	const char* m_pszClassname;
	const char* m_pszCPPClassname;
	const char* m_pszDescription;
	CEntityClass *m_pClass;
	CEntityClassInfo *m_pBaseClassInfo;
	CSchemaClassInfo* m_pSchemaBinding;
	datamap_t* m_pDataDescMap;
	datamap_t* m_pPredDescMap;
};

// Size: 0x118
class CEntityClass
{
	struct ComponentOffsets_t
	{
		uint16 m_nOffset;
	};

	struct ComponentHelper_t
	{
		size_t m_nOffset;
		CEntityComponentHelper* m_pComponentHelper;
	};
	
	struct ClassInputInfo_t
	{
		CUtlSymbolLarge m_sName;
		EntInput_t* m_pInput;
	};
	
	struct ClassOutputInfo_t
	{
		CUtlSymbolLarge m_sName;
		EntOutput_t* m_pOutput;
	};
	
public:
	inline CSchemaClassInfo *GetSchemaBinding() const
	{
		return m_pClassInfo->m_pSchemaBinding;
	}

	inline datamap_t *GetDataDescMap() const
	{
		return m_pClassInfo->m_pDataDescMap;
	}
	
public:
	void* m_pScriptDesc; // 0x0
	EntInput_t* m_pInputs; // 0x8
	EntOutput_t* m_pOutputs; // 0x10
	int m_nInputCount; // 0x18
	int m_nOutputCount; // 0x1c
	CEntitySharedPulseSignature* m_pSharedPulseSignature; // 0x20
	EntClassComponentOverride_t* m_pComponentOverrides; // 0x28
	CEntityClassInfo* m_pClassInfo; // 0x30
	CEntityClassInfo* m_pBaseClassInfo; // 0x38
	CUtlSymbolLarge m_designerName; // 0x40

	// Uses FENTCLASS_* flags
	uint m_flags; // 0x48

	// Special class group?
	int m_Unk1; // 0x4c
	
	uint m_nAllHelpersFlags; // 0x50

	CUtlVector<ComponentOffsets_t> m_ComponentOffsets; // 0x58
	CUtlVector<ComponentHelper_t> m_AllHelpers; // 0x70
	
	ComponentUnserializerClassInfo_t m_componentUnserializerClassInfo; // 0x88
	
	FlattenedSerializerDesc_t m_flattenedSerializer; // 0xb8

	CUtlVector<ClassInputInfo_t> m_classInputInfos; // 0xc8
	CUtlVector<ClassOutputInfo_t> m_classOutputInfos; // 0xe0
	
	CEntityHandle m_requiredEHandle; // 0xf8

	CEntityClass* m_pNext; // 0x100
	CEntityIdentity* m_pFirstEntity; // 0x108
	ServerClass* m_pServerClass; // 0x110
};

#endif // ENTITYCLASS_H