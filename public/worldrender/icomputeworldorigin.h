#ifndef ICOMPUTEWORLDORIGIN_H
#define ICOMPUTEWORLDORIGIN_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include <entity2/entityidentity.h>
#include <mathlib/mathlib.h>

class IWorld;

class IComputeWorldOriginCallback
{
public:
	virtual matrix3x4_t ComputeWorldOrigin(const char *pWorldName, SpawnGroupHandle_t hSpawnGroup, IWorld *pWorld) = 0;
};

#endif // ICOMPUTEWORLDORIGIN_H
