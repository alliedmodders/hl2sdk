#ifndef IWORLDRENDERERMGR_H
#define IWORLDRENDERERMGR_H

#ifdef COMPILER_MSVC
#pragma once
#endif

#include "worldschema.h"

#include <appframework/IAppSystem.h>
#include <tier0/platform.h>

struct CreateWorldInfo_t;
class CThreadRWLock;
class IWorldReference;
class CWorldVisibility;
class IWorldVPKOverrideManager;

abstract_class IWorldLoadUnloadCallback
{
public:
	virtual void OnWorldCreate( const char *pWorldName, IWorldReference *pWorldRef ) = 0;
	virtual void OnWorldDestroy( const char *pWorldName, IWorldReference *pWorldRef ) = 0;
};

abstract_class IWorldRendererMgr : public IAppSystem
{
public:
	virtual IWorldReference *CreateWorld( CreateWorldInfo_t &info ) = 0;
	virtual IWorldReference *GetWorldReference( int iIndex ) const = 0;
	virtual IWorldReference *FindWorldReferenceAndAddRef( SpawnGroupHandle_t hSpawnGroup ) = 0;
	virtual uint32 GetWorldCount( CThreadRWLock *lock = NULL ) = 0;
	virtual uint32 GetPendingWorldCount() = 0;
	virtual const char *GetWorldName( int iIndex ) const = 0;
	virtual IWorld *GetGeometryWorld( int iIndex ) = 0;
	virtual IWorld *GetGeomentryFromReference( IWorldReference *pWorldRef ) const = 0;
	virtual CWorldVisibility *GetWorldVisibility( int iIndex ) const = 0;

	// World group id
	virtual int FindLoadedWorldIndex( WorldGroupId_t hWorldGroupId, const char *pWorldName, CThreadRWLock *lock = NULL ) const = 0;
	virtual void UpdateObjectsForRendering( WorldGroupId_t hWorldGroupId, int nSkipFlags, const Vector &vCameraPos, float flLODScale, float flFarPlane, float flElapsedTime = 0.0f, const Vector *pCameraPos2 = NULL ) = 0;
	virtual bool IsFullyLoadedForPlayer( WorldGroupId_t hWorldGroupId, CSplitScreenSlot nSlot ) const = 0;
	virtual bool GetBoundsForWorld( WorldGroupId_t hWorldGroupId, Vector &vecMins, Vector &vecMaxs ) const = 0;
	virtual void UnkPutUtlContainer( WorldGroupId_t hWorldGroupId, void * ) = 0;
	virtual void FindEntitiesByTargetname( WorldGroupId_t hWorldGroupId, const char *pTargetname, const char *pSearchLump, CUtlVector< const CEntityKeyValues * > &res ) const = 0;

	// World reference
	virtual const CUtlVector< const CEntityKeyValues * > *GetEntityList( IWorldReference *pWorldRef, const char *pSearchLump = NULL ) const = 0;
	virtual bool LoadEntityLump( IWorldReference *pWorldRef, const char *pLumpName ) = 0;
	virtual bool IsEntityLumpLoaded( IWorldReference *pWorldRef, const char *pLumpName ) const = 0;
	virtual void UnloadEntityLump( IWorldReference *pWorldRef, const char *pLumpName ) = 0;

	virtual void UnkMarkGeometryWorld( int nCompareFlags, SpawnGroupHandle_t h ) = 0;
	virtual void ServiceWorldRequests() = 0;
	virtual IWorld *FindWorld( WorldGroupId_t hWorldGroupId, const char *pWorldName ) = 0;

	// Load/Unload callback
	virtual void AddWorldLoadHandler( IWorldLoadUnloadCallback *pCallback ) = 0;
	virtual void RemoveWorldLoadHandler( IWorldLoadUnloadCallback *pCallback ) = 0;

	virtual int GetDeletedWorldCount() = 0;

	// VPK manager
	virtual void InstallWorldVPKOverrideManager( IWorldVPKOverrideManager *pVPKMgr ) = 0;
	virtual void RemoveWorldVPKOverrideManager( IWorldVPKOverrideManager *pVPKMgr ) = 0;

	virtual int UnkGetCount() = 0;
	virtual const char *UnkGetStringElm( int i ) const = 0;
	virtual const char *UnkGetString2Elm( int i ) const = 0;
};

#endif // IWORLDRENDERERMGR_H
