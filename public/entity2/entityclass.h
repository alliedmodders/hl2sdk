#ifndef ENTITYCLASS_H
#define ENTITYCLASS_H

#if _WIN32
#pragma once
#endif

#include "tier1/utlsymbollarge.h"

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
	uint m_flags; // 0x40
private:
	uint8 pad68[0xB4]; // 0x44
public:
	CEntityClass* m_pNext; // 0xF8
	CEntityIdentity* m_pFirstEntity; // 0x100
	ServerClass* m_pServerClass; // 0x108
};

#endif // ENTITYCLASS_H