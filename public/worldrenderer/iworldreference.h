#ifndef IWORLDREFERENCE_H
#define IWORLDREFERENCE_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include "worldschema.h"

#include <tier0/platform.h>

class ISceneWorld;
class Vector;

abstract_class IWorldReference
{
public:
	virtual uint32 AddRef() = 0;
	virtual uint32 Release() = 0;
	virtual bool IsWorldLoaded() const = 0;
	virtual bool IsErrorWorld() const = 0;
	virtual bool IsMarkedForDeletion() const = 0;
	virtual SpawnGroupHandle_t GetSpawnGroupHandle() const = 0;
	virtual bool GetWorldBounds( Vector &vecMins, Vector &vecMaxs ) const = 0;
	virtual void ClearComputeWorldOriginCallback() = 0;
	virtual bool HasWorldGeometry() const = 0;
	virtual void ForceBlockingLoad() = 0;
	virtual ISceneWorld *GetSceneWorld() const = 0;
	virtual WorldGroupId_t GetWorldGroupId() const = 0;
	virtual bool SetLayerVisiblity( const char *pLayerName, bool bVisible ) = 0;
	virtual void PrecacheAllWorldNodes( WorldNodeFlags_t flags, bool bDoNotDeleteManifest = false ) = 0;
	virtual bool GetCompileData( uint64 &nTimestamp, uint64 &nFingerprint ) const = 0;
};

#endif // IWORLDREFERENCE_H
