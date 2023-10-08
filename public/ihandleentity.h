//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IHANDLEENTITY_H
#define IHANDLEENTITY_H
#ifdef _WIN32
#pragma once
#endif

// An IHandleEntity-derived class can go into an entity list and use ehandles.
class IHandleEntity
{
	virtual void Schema_DynamicBinding(void**) = 0;
public:
	virtual ~IHandleEntity() = 0;
};

#endif // IHANDLEENTITY_H