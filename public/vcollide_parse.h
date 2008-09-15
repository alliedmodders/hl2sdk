//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VCOLLIDE_PARSE_H
#define VCOLLIDE_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "vphysics_interface.h"

struct solid_t
{
	int		index;
	char	name[512];
	char	parent[512];
	char	surfaceprop[512];
	Vector	massCenterOverride;
	objectparams_t params;
};

struct fluid_t
{
	int index;
	char surfaceprop[512];

	fluidparams_t params;

	fluid_t() {}
	fluid_t( fluid_t const& src ) : params(src.params)
	{
		index = src.index;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Pass this into the parser to handle the keys that vphysics does not
// parse.
//-----------------------------------------------------------------------------
class IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue ) = 0;
	virtual void SetDefaults( void *pData ) = 0;
};


class IVPhysicsKeyParser
{
public:
	virtual ~IVPhysicsKeyParser() {}

	virtual const char *GetCurrentBlockName( void ) = 0;
	virtual bool		Finished( void ) = 0;
	virtual void		ParseSolid( solid_t *pSolid, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		ParseFluid( fluid_t *pFluid, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		ParseRagdollConstraint( constraint_ragdollparams_t *pConstraint, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		ParseSurfaceTable( int *table, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		ParseCustom( void *pCustom, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		ParseVehicle( vehicleparams_t *pVehicle, IVPhysicsKeyHandler *unknownKeyHandler ) = 0;
	virtual void		SkipBlock( void ) = 0;
};

#if defined( USE_PHX_FILES )
#include "compressed_vector.h"
struct solid_t;
struct fluid_t;

#pragma pack(1)
struct packedsolid_t
{
	float16 volume;
	float16 mass;
	byte index;
	byte name : 5;
	byte contents : 3;
	byte surfaceprop : 7;
	byte surfacePropIsName : 1;
	byte massCenterOverride;
	byte inertia;
	byte damping;
	byte rotdamping;
	byte drag;
};

struct packedfluid_t
{
	float		surfacePlaneDist;
	byte		index;
	byte		surfaceNormal;
	byte		currentVelocity; // velocity of the current in inches/second
	byte		damping;		// damping factor for buoyancy (tweak)
	//byte		torqueFactor;
	//byte		viscosityFactor;
	byte		contents;
};

struct packedragdollconstraint_t
{
	byte parent;
	byte child;
	char xmin;
	char xmax;
	byte xfriction;
	char ymin;
	char ymax;
	byte yfriction;
	char zmin;
	char zmax;
	byte zfriction;
};

struct packedcollisionrule_t
{
	byte index0 : 7;
	byte noselfCollisions : 1;
	byte index1;
};

struct packedanimatedfriction_t 
{
	byte minAnimatedFriction;
	byte maxAnimatedFriction;
	byte frictionTimeIn;
	byte frictionTimeOut;
	byte frictionTimeHold;
};

const int FBREAK_ISRAGDOLL = 0x01;
const int FBREAK_MOTIONDISABLED = 0x02;
const int FBREAK_DEBRIS = 0x04;
const int FBREAK_PLACEMENTISBONE = 0x08;
const int FBREAK_CLIENTSIDE = 0x10;
const int FBREAK_SERVERSIDE = 0x20;

struct packedbreakmodel_t
{
	byte name;
	byte flags;
	byte offset;
	byte health;
	byte fadetime;
	byte fademindist;
	byte fademaxdist;
	byte burstscale;
	byte placementName;
};

struct packedsectionheader_t
{
	byte type;
	byte count;
};

#pragma pack()

struct animatedfriction_t
{
	int m_iMinAnimatedFriction;
	int m_iMaxAnimatedFriction;
	float m_flFrictionTimeIn;
	float m_flFrictionTimeOut;
	float m_flFrictionTimeHold;
};

struct breakmodeldesc_t
{
	Vector offset;
	const char *pName;
	const char	*pPlacementName;
	float		health;
	float		fadetime;
	float		fademindist;
	float		fademaxdist;
	float		burstscale;
	int			nMPBreakMode;
	bool		isDebris;
	bool		isRagdoll;
	bool		motionDisabled;
	bool		placementIsBone;
	bool		hasOffset;
};

class CPackedPhysicsDescription
{
public:
	CPackedPhysicsDescription();

	virtual bool GetSolid( solid_t *pOut, int index );
	virtual int GetSolidContents( int index );
	virtual bool GetRagdollConstraint( constraint_ragdollparams_t *pOut, int index );
	virtual bool GetFluid( fluid_t *pOut, int index );
	virtual bool GetAnimatedFriction( animatedfriction_t *pOut, int index );
	virtual bool GetBreakModel( breakmodeldesc_t *pOut, int index );
	virtual bool GetMaterialTable( int *pTable, int tableSize );

	virtual const char *GetString( byte index );
	virtual float GetCoefficient( byte index, float defValue );
	virtual int GetInt( byte index, int defValue );
	virtual Vector GetVector( byte index, const Vector &defValue );


	int m_solidCount;
	packedsolid_t *m_pSolids;

	int m_constraintCount;
	packedragdollconstraint_t *m_pConstraints;

	int m_collisionRuleCount;
	packedcollisionrule_t *m_pCollisionRules;

	int m_animatedFrictionCount;
	packedanimatedfriction_t *m_pAnimatedFrictions;

	int m_fluidCount;
	packedfluid_t *m_pFluids;

	int m_breakModelCount;
	packedbreakmodel_t *m_pBreakModels;

	int m_floatCount;
	float *m_pFloats;

	int m_vectorCount;
	Vector *m_pVectors;

	int m_integerCount;
	int	*m_pIntegers;

	int		m_materialTableCount;
	char	*m_pMaterialTable;

	int		m_virtualTerrainCount;
	char	*m_pVirtualTerrain;

	int m_stringCount;
	char *m_pStringData;
	char *m_pStrings[256];
};

enum phyxsections_e
{
	SECTION_COEFFICIENTS = 0,
	SECTION_INTEGERS,
	SECTION_STRINGS,
	SECTION_VECTORS,
	SECTION_SOLIDS,
	SECTION_CONSTRAINTS,
	SECTION_COLLISIONRULES,
	SECTION_ANIMATEDFRICTION,
	SECTION_BREAKMODELS,
	SECTION_FLUIDS,
	SECTION_MATERIALTABLE,
	SECTION_VIRTUALTERRAIN,
};
#endif

#endif // VCOLLIDE_PARSE_H
