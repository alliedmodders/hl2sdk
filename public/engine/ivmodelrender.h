//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef IVMODELRENDER_H
#define IVMODELRENDER_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib.h"
#include "istudiorender.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct mstudioanimdesc_t;
struct mstudioseqdesc_t;
struct model_t;
class IClientRenderable;
class Vector;
struct studiohdr_t;
class IMaterial;

FORWARD_DECLARE_HANDLE( LightCacheHandle_t ); 


//-----------------------------------------------------------------------------
// Model Rendering + instance data
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VENGINE_HUDMODEL_INTERFACE_VERSION	"VEngineModel012"

typedef unsigned short ModelInstanceHandle_t;

enum
{
	MODEL_INSTANCE_INVALID = (ModelInstanceHandle_t)~0
};

struct ModelRenderInfo_t
{
	int flags;
	IClientRenderable *pRenderable;
	ModelInstanceHandle_t instance;
	int entity_index;
	const model_t *pModel;
	Vector origin;
	QAngle angles; 
	int skin;
	int body;
	int hitboxset;
	const matrix3x4_t *pModelToWorld;
	const matrix3x4_t *pLightingOffset;
	const Vector *pLightingOrigin;

	ModelRenderInfo_t()
	{
		pModelToWorld = NULL;
		pLightingOffset = NULL;
		pLightingOrigin = NULL;
	}
};

// UNDONE: Move this to hud export code, subsume previous functions
abstract_class IVModelRender
{
public:
	virtual int		DrawModel(	int flags,
								IClientRenderable *pRenderable,
								ModelInstanceHandle_t instance,
								int entity_index, 
								const model_t *model, 
								Vector const& origin, 
								QAngle const& angles, 
								int skin,
								int body,
								int hitboxset,
								const matrix3x4_t *modelToWorld = NULL,
								const matrix3x4_t *pLightingOffset = NULL ) = 0;

	// This causes a material to be used when rendering the model instead 
	// of the materials the model was compiled with
	virtual void	ForcedMaterialOverride( IMaterial *newMaterial ) = 0;

	virtual void	SetViewTarget( const Vector& target ) = 0;
	virtual void	SetFlexWeights( int numweights, const float *weights ) = 0;
	virtual void	SetFlexWeights( int numweights, const float *weights, const float *delayedweights ) = 0;

	virtual matrix3x4_t* pBoneToWorld(int i) = 0;
	virtual matrix3x4_t* pBoneToWorldArray() = 0;
	
	// Creates, destroys instance data to be associated with the model
	virtual ModelInstanceHandle_t CreateInstance( IClientRenderable *pRenderable, LightCacheHandle_t *pCache = NULL ) = 0;
	virtual void DestroyInstance( ModelInstanceHandle_t handle ) = 0;

	// Associates a particular lighting condition with a model instance handle.
	// FIXME: This feature currently only works for static props. To make it work for entities, etc.,
	// we must clean up the lightcache handles as the model instances are removed.
	// At the moment, since only the static prop manager uses this, it cleans up all LightCacheHandles 
	// at level shutdown.
	virtual void SetStaticLighting( ModelInstanceHandle_t handle, LightCacheHandle_t* pHandle ) = 0;
	virtual LightCacheHandle_t GetStaticLighting( ModelInstanceHandle_t handle ) = 0;

	// moves an existing InstanceHandle to a nex Renderable to keep decals etc. Models must be the same
	virtual bool ChangeInstance( ModelInstanceHandle_t handle, IClientRenderable *pRenderable ) = 0;

	// Creates a decal on a model instance by doing a planar projection
	// along the ray. The material is the decal material, the radius is the
	// radius of the decal to create.
	virtual void AddDecal( ModelInstanceHandle_t handle, Ray_t const& ray, 
		Vector const& decalUp, int decalIndex, int body, bool noPokeThru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS ) = 0;

	// Removes all the decals on a model instance
	virtual void RemoveAllDecals( ModelInstanceHandle_t handle ) = 0;

	// Remove all decals from all models
	virtual void RemoveAllDecalsFromAllModels() = 0;

	// Shadow rendering
	virtual void DrawModelShadow( IClientRenderable *pRenderable, int body ) = 0;

	// This gets called when overbright, etc gets changed to recompute static prop lighting.
	virtual bool RecomputeStaticLighting( ModelInstanceHandle_t handle ) = 0;

	virtual void ReleaseAllStaticPropColorData( void ) = 0;
	virtual void RestoreAllStaticPropColorData( void ) = 0;

	// Extended version of drawmodel
	virtual int	DrawModelEx( ModelRenderInfo_t &pInfo ) = 0;

	// Shadow rendering extended version
	virtual void DrawModelShadowEx( IClientRenderable *pRenderable, int body, int skin ) = 0;

	virtual int	DrawModelExStaticProp( ModelRenderInfo_t &pInfo ) = 0;
};


#endif // IVMODELRENDER_H
