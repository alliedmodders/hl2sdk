//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef ECON_ITEM_SYSTEM_H
#define ECON_ITEM_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

class CEconItemSchema;

abstract_class IEconItemSystem
{
public:
	virtual CEconItemSchema *GetItemSchema() = 0;
};

#endif // ECON_ITEM_SYSTEM_H
