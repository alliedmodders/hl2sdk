#ifndef ENTITYCOMPONENT_H
#define ENTITYCOMPONENT_H

#if _WIN32
#pragma once
#endif


#include <tier0/platform.h>
#include "tier1/utlsymbollarge.h"

class CEntityComponent
{
private:
	uint8 unknown[0x8]; // 0x0
};

class CScriptComponent : public CEntityComponent
{
private:
	uint8 unknown[0x28]; // 0x8
public:
	CUtlSymbolLarge m_scriptClassName; // 0x30
};

#endif // ENTITYCOMPONENT_H