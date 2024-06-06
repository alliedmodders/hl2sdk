//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef CMODEL_H
#define CMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "trace.h"
#include "tier0/dbg.h"
#include "entityhandle.h"

struct edict_t;
struct model_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "bspflags.h"
//#include "mathlib/vector.h"

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

#include "vcollide.h"

enum RayType_t : uint8
{
	RAY_TYPE_LINE = 0,
	RAY_TYPE_SPHERE,
	RAY_TYPE_HULL,
	RAY_TYPE_CAPSULE,
	RAY_TYPE_MESH,
};

struct cmodel_t
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	int			headnode;

	vcollide_t	vcollisionData;
};

struct csurface_t
{
	const char	*name;
	short		surfaceProps;
	unsigned short	flags;		// BUGBUG: These are declared per surface, not per material, but this database is per-material now
};

//-----------------------------------------------------------------------------
// A ray...
//-----------------------------------------------------------------------------
struct Ray_t
{
	Ray_t() 																{ Init( Vector( 0.0f, 0.0f, 0.0f ) ); }
	Ray_t( const Vector& vStartOffset ) 									{ Init( vStartOffset ); }
	Ray_t( const Vector& vCenter, float flRadius ) 							{ Init( vCenter, flRadius ); }
	Ray_t( const Vector& vMins, const Vector& vMaxs ) 						{ Init( vMins, vMaxs ); }
	Ray_t( const Vector& vCenterA, const Vector& vCenterB, float flRadius ) { Init( vCenterA, vCenterB, flRadius ); }
	
	void Init( const Vector& vStartOffset )
	{
		m_Line.m_vStartOffset = vStartOffset;
		m_Line.m_flRadius = 0.0f;
		m_eType = RAY_TYPE_LINE;
	}
	
	void Init( const Vector& vCenter, float flRadius )
	{
		if ( flRadius > 0.0f )
		{
			m_Sphere.m_vCenter = vCenter;
			m_Sphere.m_flRadius = flRadius;
			m_eType = RAY_TYPE_SPHERE;
		}
		else
		{
			Init( vCenter );
		}
	}
	
	void Init( const Vector& vMins, const Vector& vMaxs )
	{
		if ( vMins != vMaxs )
		{
			m_Hull.m_vMins = vMins;
			m_Hull.m_vMaxs = vMaxs;
			m_eType = RAY_TYPE_HULL;
		}
		else
		{
			Init( vMins );
		}
	}
	
	void Init( const Vector& vCenterA, const Vector& vCenterB, float flRadius )
	{
		if ( vCenterA != vCenterB )
		{
			if ( flRadius > 0.0f )
			{
				m_Capsule.m_vCenter[0] = vCenterA;
				m_Capsule.m_vCenter[1] = vCenterB;
				m_Capsule.m_flRadius = flRadius;
				m_eType = RAY_TYPE_CAPSULE;
			}
			else
			{
				Init( vCenterA, vCenterB );
			}
		}
		else
		{
			Init( vCenterA, flRadius );
		}
	}
	
	struct Line_t
	{
		Vector m_vStartOffset;
		float m_flRadius;
	};
	
	struct Sphere_t
	{
		Vector m_vCenter;
		float m_flRadius;
	};
	
	struct Hull_t
	{
		Vector m_vMins;
		Vector m_vMaxs;
	};
	
	struct Capsule_t
	{
		Vector m_vCenter[2];
		float m_flRadius;
	};
	
	struct Mesh_t
	{
		Vector m_vMin;
		Vector m_vMax;
		Vector* m_pVertices;
		int m_nNumVertices;
	};
	
	union
	{
		Line_t 		m_Line;
		Sphere_t 	m_Sphere;
		Hull_t 		m_Hull;
		Capsule_t 	m_Capsule;
		Mesh_t 		m_Mesh;
	};
	
	RayType_t m_eType;
};


#endif // CMODEL_H

	
#include "gametrace.h"

