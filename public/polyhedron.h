//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef POLYHEDRON_H_
#define	POLYHEDRON_H_

#ifdef _WIN32
#pragma once
#endif

#include "mathlib.h"



struct Polyhedron_IndexedLine_t
{
	unsigned short iPointIndices[2];
};

struct Polyhedron_IndexedLineReference_t
{
	unsigned short iLineIndex;
	unsigned char iEndPointIndex; //since two polygons reference any one line, one needs to traverse the line backwards, this flags that behavior
};

struct Polyhedron_IndexedPolygon_t
{
	int iFirstIndex;
	int iIndexCount;
	Vector polyNormal;
};

class CPolyhedron //made into a class because it's going virtual to support distinctions between temp and permanent versions
{
public:
	Vector *pVertices;
	int iVertexCount;

	Polyhedron_IndexedLine_t *pLines;
	int iLineCount;
	
	Polyhedron_IndexedLineReference_t *pIndices;
	int iIndexCount;

	Polyhedron_IndexedPolygon_t *pPolygons;
	int iPolygonCount;

	virtual ~CPolyhedron( void ) {};
	virtual void Release( void ) = 0;
};

CPolyhedron *GeneratePolyhedronFromPlanes( const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory = false ); //be sure to polyhedron->Release()
CPolyhedron *ClipPolyhedron( const CPolyhedron *pExistingPolyhedron, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory = false ); //this does NOT modify/delete the existing polyhedron



#endif //#ifndef POLYHEDRON_H_

