#ifndef ENTITYINSTANCE_H
#define ENTITYINSTANCE_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlsymbollarge.h"
#include "entitycomponent.h"

class CEntityHandle;
class CEntityIdentity;

class CEntityInstance
{
	virtual void Schema_DynamicBinding(void**) = 0;

public:
	virtual ~CEntityInstance() = 0;
	// Note: this is implemented in game code (ehandle.h)
	CEntityHandle GetRefEHandle() const;

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

#endif // ENTITYINSTANCE_H
