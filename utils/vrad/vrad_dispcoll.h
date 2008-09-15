//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VRAD_DISPCOLL_H
#define VRAD_DISPCOLL_H
#pragma once

#include <assert.h>
#include "DispColl_Common.h"

//=============================================================================
//
// VRAD specific collision
//
#define VRAD_QUAD_SIZE			4

class CVRADDispColl : public CDispCollTree
{
public:

	// Creation/Destruction Functions
	CVRADDispColl();
	~CVRADDispColl();

	bool Create( CCoreDispInfo *pDisp );

	// Operations Functions
	void BaseFacePlaneToDispUV( Vector const &vecPlanePt, Vector2D &dispUV );
	void DispUVToSurfPoint( Vector2D const &dispUV, Vector &vecPoint, float flPushEps );
	void DispUVToSurfNormal( Vector2D const &dispUV, Vector &vecNormal );

	void ClosestBaseFaceData( Vector const &vecWorldPoint, Vector &vecPoint, Vector &vecNormal );

	void InitPatch( int iPatch, int iParentPatch, bool bFirst );
	bool MakePatch( int iPatch );

	// Attrib Functions
	inline int GetParentIndex( void )									{ return m_iParent; }		
	inline void GetParentFaceNormal( Vector &vecNormal )				{ vecNormal = m_vecStabDir; }
	inline int GetPointOffset( void )									{ return m_nPointOffset; }
	inline int GetParentPointCount( void )								{ return VRAD_QUAD_SIZE; }
	inline void GetParentPoint( int iPoint, Vector &vecPoint )			{ Assert( ( iPoint >= 0 ) && ( iPoint < VRAD_QUAD_SIZE ) ); vecPoint = m_vecSurfPoints[iPoint]; }	
	inline void GetParentPointNormal( int iPoint, Vector &vecNormal )	{ Assert( ( iPoint >= 0 ) && ( iPoint < VRAD_QUAD_SIZE ) ); vecNormal = m_vecSurfPointNormals[iPoint]; }

	inline void GetVert( int iVert, Vector &vecVert )					{ Assert( ( iVert >= 0 ) && ( iVert < GetSize() ) ); vecVert = m_aVerts[iVert]; }
	inline void GetVertNormal( int iVert, Vector &vecNormal )			{ Assert( ( iVert >= 0 ) && ( iVert < GetSize() ) ); vecNormal = m_aVertNormals[iVert]; }
	inline Vector2D const& GetLuxelCoord( int iLuxel )					{ Assert( ( iLuxel >= 0 ) && ( iLuxel < GetSize() ) ); return m_aLuxelCoords[iLuxel]; }

	inline float GetSampleRadius2( void )								{ return m_flSampleRadius2; }
	inline float GetPatchSampleRadius2( void )							{ return m_flPatchSampleRadius2; }
	inline void GetSampleBBox( Vector &boxMin, Vector &boxMax )			{ boxMin = m_vecSampleBBox[0]; boxMax = m_vecSampleBBox[1]; }

public:

	static  float s_flMinChopLength;
	static	float s_flMaxChopLength;

	Vector	m_vecLightmapOrigin;						// the lightmap origin in world space (base face relative)
	Vector	m_vecWorldToLightmapSpace[2];				// u = ( world - lightorigin ) dot worldToLightmap[0]
	Vector	m_vecLightmapToWorldSpace[2];				// world = lightorigin + u * lightmapToWorld[0] 

protected:

	void CalcSampleRadius2AndBox( dface_t *pFace );
	void CalcPatchSampleRadius2( void );
	void BuildVNodes( void );

	void DispUVToSurf_TriTLToBR( Vector &vecPoint, float flPushEps, float flU, float flV, int nSnapU, int nSnapV, int nWidth, int nHeight );
	void DispUVToSurf_TriBLToTR( Vector &vecPoint, float flPushEps, float flU, float flV, int nSnapU, int nSnapV, int nWidth, int nHeight );

	bool MakeParentPatch( int iPatch );
	bool MakeChildPatch( int iPatch );
	void GetNodesInPatch( int iPatch, int *pVNodes, int &vNodeCount );

	// Utility.
	void GetSurfaceMinMax( Vector &boxMin, Vector &boxMax );
	void GetMinorAxes( Vector const &vecNormal, int &nAxis0, int &nAxis1 );

protected:

	struct VNode_t
	{
		Vector		patchOrigin;
		Vector2D	patchOriginUV;
		Vector		patchNormal;
		float		patchArea;
		Vector		patchBounds[2];
	};

	int						m_iParent;								// Parent index

	int						m_nPointOffset;

	float					m_flSampleRadius2;						// Sampling radius
	float					m_flPatchSampleRadius2;					// Patch sampling radius (max bound)
	Vector					m_vecSampleBBox[2];						// Sampling bounding box size

	Vector					m_vecSurfPointNormals[VRAD_QUAD_SIZE];	// Face point normals.

	CUtlVector<VNode_t>		m_aVNodes;								// Make an array parallel to the nodes array in the base class
																	// and store vrad specific node data
	CUtlVector<Vector2D>	m_aLuxelCoords;							// Lightmap coordinates.
	CUtlVector<Vector>		m_aVertNormals;							// Displacement vertex normals
};

#endif // VRAD_DISPCOLL_H