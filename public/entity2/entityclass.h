#ifndef ENTITYCLASS_H
#define ENTITYCLASS_H

#if _WIN32
#pragma once
#endif

#include "tier1/utlsymbollarge.h"

#define FENTCLASS_NETWORKABLE		(1 << 0) // If the EntityClass is networkable
#define FENTCLASS_ALIAS				(1 << 1) // If the EntityClass is an alias
#define FENTCLASS_NO_SPAWNGROUP		(1 << 2) // Don't use spawngroups when creating entity
#define FENTCLASS_FORCE_EHANDLE		(1 << 3) // Forces m_requiredEHandle on created entities
#define FENTCLASS_UNK004			(1 << 4)
#define FENTCLASS_UNK005			(1 << 5)
#define FENTCLASS_ANONYMOUS			(1 << 6) // If the EntityClass is anonymous

class CEntityClassInfo;
class CEntityIdentity;
class ServerClass;
struct EntInput_t;
struct EntOutput_t;
struct EntClassComponentOverride_t;

// Size: 0x110
class CEntityClass
{
public:
	void* m_pScriptDesc; // 0x0
	EntInput_t* m_pInputs; // 0x8
	EntOutput_t* m_pOutputs; // 0x10
	int m_nInputCount; // 0x18
	int m_nOutputCount; // 0x1C
	EntClassComponentOverride_t* m_pComponentOverrides; // 0x20
	CEntityClassInfo* m_pClassInfo; // 0x28
	CEntityClassInfo* m_pBaseClassInfo; // 0x30
	CUtlSymbolLarge m_designerName; // 0x38

	// Uses FENTCLASS_* flags
	uint m_flags; // 0x40
private:
	uint8 pad68[0xB4]; // 0x44
public:
	CEntityClass* m_pNext; // 0xF8
	CEntityIdentity* m_pFirstEntity; // 0x100
	ServerClass* m_pServerClass; // 0x108
};

#endif // ENTITYCLASS_H