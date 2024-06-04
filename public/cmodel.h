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
	Ray_t() 
	{
		m_Line.m_vStartOffset.Init();
		m_Line.m_flRadius = 0.0f;
		m_eType = RAY_TYPE_LINE;
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
		Vector m_vMin;
		Vector m_vMax;
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

