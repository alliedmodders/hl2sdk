#ifndef IWORLD_H
#define IWORLD_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include "worldschema.h"

#include <tier0/platform.h>
#include <tier1/utlvector.h>
#include <mathlib/camera.h>

class ISceneWorld;
class Vector;
class CEntityKeyValues;

abstract_class IWorld
{
public:
	// Loading
	virtual void Init( ISceneWorld *pScene, bool b, int n ) = 0;
	virtual void CreateAndDispatchLoadRequests( const Vector &vEye, int n ) = 0;
	virtual void Shutdown() = 0;

	// Reflection
	virtual int GetNumNodes() const = 0;
	virtual const WorldBuilderParams_t *GetBuilderParams() const = 0;
	virtual bool IsAncestor( int nNodeInQuestion, int nPotentialAncestor ) const = 0;
	virtual const NodeData_t *GetNodeData( int n ) = 0;
	virtual AABB_t GetNodeBounds( int n ) const = 0;
	virtual float GetNodeMinDistance( int n ) const = 0;

	// Transform
	virtual void SetWorldTransform( const matrix3x4_t &mat ) = 0;
	virtual const matrix3x4_t &GetWorldTransform() const = 0;

	// Sun
	virtual void SetLoadSun( bool bEnable ) = 0;
	virtual bool GetLoadSun() const = 0;

	// Players
	virtual bool IsFullyLoadedForPlayer( WorldGroupId_t hWorldGroupId, CSplitScreenSlot nSlot ) const = 0;
	virtual void ClearOutstandingLoadRequests() = 0;

	// Precache
	virtual void PrecacheAllWorldNodes( WorldNodeFlags_t flags, int iCacheNodes = 1, bool bDoNotDeleteManifest = false ) = 0;

	// Entities and lighting
	virtual const CUtlVector< const CEntityKeyValues * > *GetEntityList( const char *pSearchLump = NULL ) const = 0; // Can be return NULL value.
	virtual void FindEntitiesByTargetname( const char *pTargetname, const char *pSearchLump, CUtlVector< const CEntityKeyValues * > &res ) const = 0;
	virtual bool HasLightmaps() const = 0;
	virtual bool HasBakedLighting() const = 0;
	virtual const BakedLightingInfo_t *GetBakedLightingInfo() const = 0;
	virtual void FindEntitiesByClassname( const char *pClassname, const char *pSearchLump, CUtlVector< const CEntityKeyValues * > &res ) const = 0;
};

#endif // IWORLD_H
