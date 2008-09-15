//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TERRAINMOD_H
#define TERRAINMOD_H
#ifdef _WIN32
#pragma once
#endif

#include "vector.h"

// Terrain Modification Types
enum TerrainModType
{
	TMod_Sphere = 0,			// sphere that pushes all vertices out along their normal.
	TMod_Suck,
	TMod_AABB
};

class CTerrainModParams
{
public:

	// Flags for m_Flags.
	enum
	{
		TMOD_SUCKTONORMAL	   = ( 1 << 0 ),	// For TMod_Suck, suck into m_Normal rather than on +Z.
		TMOD_STAYABOVEORIGINAL = ( 1 << 1 )		// For TMod_Suck, don't go below the original vert on Z.
	};

	CTerrainModParams() { m_Flags = 0; }		// people always forget to init this

	Vector		m_vCenter;
	Vector		m_vNormal;						// If TMod_Suck and TMOD_SUCKTONORMAL is set.
	int			m_Flags;						// Combination of TMOD_ flags.
	float		m_flRadius;
	Vector		m_vecMin;						// Bounding box.
	Vector		m_vecMax;
	float		m_flStrength;					// for TMod_Suck
	float		m_flMorphTime;					// time over which the morph takes place
};

class CSpeculativeTerrainModVert
{
public:
	Vector		m_vOriginal;		// vertex position before any mods
	Vector		m_vCurrent;			// current vertex position
	Vector		m_vNew;				// vertex position if the mod were applied
};

//-----------------------------------------------------------------------------
// Terrain modification interface
//-----------------------------------------------------------------------------
class ITerrainMod
{
public:

	//---------------------------------------------------------------------
	// Initialize the terrain modifier.
	//---------------------------------------------------------------------
	virtual void	Init( const CTerrainModParams &params ) = 0;

	//---------------------------------------------------------------------
	// Apply the terrain modifier to the surface.  The vertex should be
	// moved from its original position to the target position.
	// Return true if the position is modified.
	//---------------------------------------------------------------------
	virtual bool	ApplyMod( Vector &vecTargetPos, Vector const &vecOriginalPos ) = 0;

	//---------------------------------------------------------------------
	// Apply the terrain modifier to the surface.  The vertex should from 
	// its original position toward the target position bassed on the
	// morph time.
	// Return true if the posistion is modified.
	//---------------------------------------------------------------------
	virtual	bool	ApplyModAtMorphTime( Vector &vecTargetPos, const Vector&vecOriginalPos, 
		                                 float flCurrentTime, float flMorphTime ) = 0;

	//---------------------------------------------------------------------
	// Get the bounding box for things that this mod can affect (note that
	// it CAN move things outside of this bounding box).
	//---------------------------------------------------------------------
	virtual void	GetBBox( Vector &vecBBMin, Vector &vecBBMax ) = 0;
};

#endif // TERRAINMOD_H
