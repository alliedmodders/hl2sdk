//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISPCOLL_COMMON_H
#define DISPCOLL_COMMON_H
#pragma once

#include "trace.h"
#include "builddisp.h"
#include "terrainmod.h"
#include "tier1/utlrbtree.h"

FORWARD_DECLARE_HANDLE( memhandle_t );

#define DISPCOLL_TREETRI_SIZE		MAX_DISPTRIS
#define DISPCOLL_DIST_EPSILON		0.03125f
#define DISPCOLL_ROOTNODE_INDEX		0
#define DISPCOLL_INVALID_TRI		-1
#define DISPCOLL_INVALID_FRAC		-99999.9f
#define DISPCOLL_NORMAL_UNDEF		0xffff

extern double g_flDispCollSweepTimer;
extern double g_flDispCollIntersectTimer;
extern double g_flDispCollInCallTimer;

class CDispLeafLink;
class ITerrainMod;

struct RayDispOutput_t
{
	short	ndxVerts[4];	// 3 verts and a pad
	float	u, v;			// the u, v paramters (edgeU = v1 - v0, edgeV = v2 - v0)
	float	dist;			// intersection distance
};

// Assumptions:
//	Max patch is 17x17, therefore 9 bits needed to represent a triangle index
// 

//=============================================================================
//	Displacement Collision Triangle
class CDispCollTri
{

	struct index_t
	{
		union
		{
			struct
			{
				unsigned short uiVert:9;
				unsigned short uiMin:2;
				unsigned short uiMax:2;
			} m_Index;
			
			unsigned short m_IndexDummy;
		};
	};

	index_t				m_TriData[3];

public:
	unsigned short		m_ucSignBits:3;			// Plane test.
	unsigned short		m_ucPlaneType:3;		// Axial test?
	unsigned short		m_uiFlags:5;			// Uses 5-bits - maybe look into merging it with something?

	Vector				m_vecNormal;			// Triangle normal (plane normal).
	float				m_flDist;				// Triangle plane dist.

	// Creation.
	     CDispCollTri();
	void Init( void );
	void CalcPlane( CUtlVector<Vector> &m_aVerts );
	void FindMinMax( CUtlVector<Vector> &m_aVerts );

	// Triangle data.
	inline void SetVert( int iPos, int iVert )			{ Assert( ( iPos >= 0 ) && ( iPos < 3 ) ); Assert( ( iVert >= 0 ) && ( iVert < ( 1 << 9 ) ) ); m_TriData[iPos].m_Index.uiVert = iVert; }
	inline int  GetVert( int iPos )						{ Assert( ( iPos >= 0 ) && ( iPos < 3 ) ); return m_TriData[iPos].m_Index.uiVert; }
	inline void SetMin( int iAxis, int iMin )			{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); Assert( ( iMin >= 0 ) && ( iMin < 3 ) ); m_TriData[iAxis].m_Index.uiMin = iMin; }
	inline int  GetMin( int iAxis )						{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); return m_TriData[iAxis].m_Index.uiMin; }
	inline void SetMax( int iAxis, int iMax )			{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); Assert( ( iMax >= 0 ) && ( iMax < 3 ) ); m_TriData[iAxis].m_Index.uiMax = iMax; }
	inline int  GetMax( int iAxis )						{ Assert( ( iAxis >= 0 ) && ( iAxis < 3 ) ); return m_TriData[iAxis].m_Index.uiMax; }
};

//=============================================================================
//	AABB Node

#pragma pack(1)

#ifdef _XBOX

class CDispCollAABBVector
{
public:
	void Init()
	{
		x = y = z = 0;
	}

	void SetMaxs( const Vector &vecFrom )
	{
		x = ceil( vecFrom.x );
		y = ceil( vecFrom.y );
		z = ceil( vecFrom.z );
	}

	void SetMins( const Vector &vecFrom )
	{
		x = floor( vecFrom.x );
		y = floor( vecFrom.y );
		z = floor( vecFrom.z );
	}

	operator Vector() const
	{
		return Vector( x, y, z );
	}

	short x, y, z;
};

#else

class CDispCollAABBVector : public Vector
{
public:
	void SetMaxs( const Vector &vecFrom ) { *((Vector *)this) = vecFrom; }
	void SetMins( const Vector &vecFrom ) { *((Vector *)this) = vecFrom; }
};

#endif

class CDispCollAABBNode
{
public:
	CDispCollAABBVector	m_vecBox[2];					// 0 - min, 1 - max
	short				m_iTris[2];

		 CDispCollAABBNode();
	void Init( void );	
	inline bool IsLeaf( void ) const				{ return ( ( m_iTris[0] != DISPCOLL_INVALID_TRI ) || ( m_iTris[1] != DISPCOLL_INVALID_TRI ) ); }
	void GenerateBox( CUtlVector <CDispCollTri> &m_aTris, CUtlVector<Vector> &m_aVerts );
};

#pragma pack()

//=============================================================================
//	Helper
class CDispCollHelper
{
public:

	float	m_flStartFrac;
	float	m_flEndFrac;
	Vector	m_vecImpactNormal;
	float	m_flImpactDist;
};

//=============================================================================
//	Cache
#pragma pack(1)
class CDispCollTriCache
{
public:
	unsigned short m_iCrossX[3];
	unsigned short m_iCrossY[3];
	unsigned short m_iCrossZ[3];
};
#pragma pack()

//=============================================================================
//
// Displacement Collision Tree Data
//
class CDispCollTree
{
public:

	// Creation/Destruction.
	CDispCollTree();
	~CDispCollTree();
	virtual bool Create( CCoreDispInfo *pDisp );

	// Raycasts.
	bool AABBTree_Ray( const Ray_t &ray, CBaseTrace *pTrace, bool bSide = true );
	bool AABBTree_Ray( const Ray_t &ray, RayDispOutput_t &output );

	// Hull Sweeps.
	bool AABBTree_SweepAABB( const Ray_t &ray, CBaseTrace *pTrace );

	// Hull Intersection.
	bool AABBTree_IntersectAABB( const Ray_t &ray );

	// Point/Box vs. Bounds.
	bool PointInBounds( Vector const &vecBoxCenter, Vector const &vecBoxMin, Vector const &vecBoxMax, bool bPoint );

	// Terrain modification - update the collision model.
	void ApplyTerrainMod( ITerrainMod *pMod );

	// Utility.
	inline void SetPower( int power )								{ m_nPower = power; }
	inline int GetPower( void )										{ return m_nPower; }

	inline int	GetFlags( void )									{ return m_nFlags; }
	inline void SetFlags( int nFlags )								{ m_nFlags = nFlags; }
	inline bool CheckFlags( int nFlags )							{ return ( ( nFlags & GetFlags() ) != 0 ) ? true : false; }

	inline int GetWidth( void )										{ return ( ( 1 << m_nPower ) + 1 ); }
	inline int GetHeight( void )									{ return ( ( 1 << m_nPower ) + 1 ); }
	inline int GetSize( void )										{ return ( ( 1 << m_nPower ) + 1 ) * ( ( 1 << m_nPower ) + 1 ); }
	inline int GetTriSize( void )									{ return ( ( 1 << m_nPower ) * ( 1 << m_nPower ) * 2 ); }

//	inline void SetTriFlags( short iTri, unsigned short nFlags )	{ m_aTris[iTri].m_uiFlags = nFlags; }

	inline void GetStabDirection( Vector &vecDir )					{ vecDir = m_vecStabDir; }

	inline void GetBounds( Vector &vecBoxMin, Vector &vecBoxMax )	{ vecBoxMin = m_vecBounds[0]; vecBoxMax = m_vecBounds[1]; }

	inline CDispLeafLink*&	GetLeafLinkHead()						{ return m_pLeafLinkHead; }

	inline int GetContents( void )									{ return m_nContents; }
	inline void SetSurfaceProps( int iProp, short nSurfProp )		{ Assert( ( iProp >= 0 ) && ( iProp < 2 ) ); m_nSurfaceProps[iProp] = nSurfProp; }
	inline short GetSurfaceProps( int iProp )						{ return m_nSurfaceProps[iProp]; }

	inline void SetCheckCount( int nDepth, int nCount )				{ Assert( ( nDepth >= 0 ) && ( nDepth < MAX_CHECK_COUNT_DEPTH ) ); m_nCheckCount[nDepth] = nCount; }
	inline int GetCheckCount( int nDepth ) const					{ Assert( ( nDepth >= 0 ) && ( nDepth < MAX_CHECK_COUNT_DEPTH ) ); return m_nCheckCount[nDepth]; }

	inline unsigned int GetMemorySize( void )						{ return m_nSize; }
	inline unsigned int GetCacheMemorySize( void )					{ return ( m_aTrisCache.Count() * sizeof(CDispCollTriCache) + m_aEdgePlanes.Count() * sizeof(Vector) ); }

	inline bool IsCached( void )									{ return m_aTrisCache.Count() == m_aTris.Count(); }

	void GetVirtualMeshList( struct virtualmeshlist_t *pList );
	void AABBTree_GetTrisInSphere( const Vector &center, float radius, unsigned short *pIndexOut, int indexMax, int *pIndexCount );

public:

	inline int Nodes_GetChild( int iNode, int nDirection );
	inline int Nodes_CalcCount( int nPower );
	inline int Nodes_GetParent( int iNode );
	inline int Nodes_GetLevel( int iNode );
	inline int Nodes_GetIndexFromComponents( int x, int y );

	void CheckCache();
	void Cache( void );
	void Uncache()	{ m_aTrisCache.Purge(); m_aEdgePlanes.Purge(); }

#ifdef ENGINE_DLL
	// Data manager methods
	static size_t EstimatedSize( CDispCollTree *pTree )
	{
		return pTree->GetCacheMemorySize();
	}

	static CDispCollTree *CreateResource( CDispCollTree *pTree )
	{
		// Created ahead of time
		return pTree;
	}

	bool GetData()
	{
		return IsCached();
	}

	size_t Size()
	{
		return GetCacheMemorySize();
	}

	void DestroyResource()
	{
		Uncache();
		m_hCache = NULL;
	}
#endif

protected:

	bool AABBTree_Create( CCoreDispInfo *pDisp );
	void AABBTree_CopyDispData( CCoreDispInfo *pDisp );
	void AABBTree_CreateLeafs( void );
	void AABBTree_GenerateBoxes( void );
	void AABBTree_CalcBounds( void );

	void AABBTree_BuildTreeTrisSweep_r( const Ray_t &ray, int iNode, CDispCollTri **ppTreeTris, unsigned short &nTriCount );
	void AABBTree_BuildTreeTrisIntersect_r( const Ray_t &ray, int iNode, CDispCollTri **ppTreeTris, unsigned short &nTriCount );
	void AABBTree_BuildTreeTrisInSphere_r( const Vector &center, float radius, int iNode, unsigned short *pIndexOut, unsigned short indexMax, unsigned short &nTriCount  );

	void AABBTree_TreeTrisRayTest_r( const Ray_t &ray, const Vector &vecInvDelta, int iNode, CBaseTrace *pTrace, bool bSide, CDispCollTri **pImpactTri );
	void AABBTree_TreeTrisRayBarycentricTest_r( const Ray_t &ray, const Vector &vecInvDelta, int iNode, RayDispOutput_t &output, CDispCollTri **pImpactTri );

	struct AABBTree_TreeTrisSweepTest_Args_t
	{
		AABBTree_TreeTrisSweepTest_Args_t( const Ray_t &ray, const Vector &vecInvDelta, const Vector &rayDir, CBaseTrace *pTrace )
			: ray( ray ), vecInvDelta( vecInvDelta ), rayDir( rayDir ), pTrace( pTrace ) {}
		const Ray_t &ray;
		const Vector &vecInvDelta;
		const Vector &rayDir;
		CBaseTrace *pTrace;
	};
	void AABBTree_TreeTrisSweepTest_r( const AABBTree_TreeTrisSweepTest_Args_t &args, int iNode );

	bool AABBTree_SweepAABBBox( const Ray_t &ray, const Vector &rayDir, CBaseTrace *pTrace );
	void AABBTree_TreeTrisSweepTestBox_r( const Ray_t &ray, const Vector &rayDir, const Vector &vecMin, const Vector &vecMax, int iNode, CBaseTrace *pTrace );

	void AABBTree_TreeTrisIntersectBox_r( const Ray_t &ray, const Vector &vecMin, const Vector &vecMax, int iNode, bool bIntersect );

protected:

	void SweepAABBTriIntersect( const Ray_t &ray, const Vector &rayDir, int iTri, CDispCollTri *pTri, CBaseTrace *pTrace, bool bTestOutside );

	void Cache_Create( CDispCollTri *pTri, int iTri );		// Testing!
	bool Cache_EdgeCrossAxisX( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );
	bool Cache_EdgeCrossAxisY( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );
	bool Cache_EdgeCrossAxisZ( const Vector &vecEdge, const Vector &vecOnEdge, const Vector &vecOffEdge, CDispCollTri *pTri, unsigned short &iPlane );

	inline bool FacePlane( const Ray_t &ray, const Vector &rayDir, CDispCollTri *pTri );
	bool FORCEINLINE AxisPlanesXYZ( const Ray_t &ray, CDispCollTri *pTri );
	inline bool EdgeCrossAxisX( const Ray_t &ray, unsigned short iPlane );
	inline bool EdgeCrossAxisY( const Ray_t &ray, unsigned short iPlane );
	inline bool EdgeCrossAxisZ( const Ray_t &ray, unsigned short iPlane );

	bool ResolveRayPlaneIntersect( float flStart, float flEnd, const Vector &vecNormal, float flDist );
	template <int AXIS> bool FORCEINLINE TestOneAxisPlaneMin( const Ray_t &ray, CDispCollTri *pTri );
	template <int AXIS> bool FORCEINLINE TestOneAxisPlaneMax( const Ray_t &ray, CDispCollTri *pTri );
	template <int AXIS>	bool EdgeCrossAxis( const Ray_t &ray, unsigned short iPlane );

	// Utility
	inline void CalcClosestBoxPoint( const Vector &vecPlaneNormal, const Vector &vecBoxStart, const Vector &vecBoxExtents, Vector &vecBoxPoint );
	inline void CalcClosestExtents( const Vector &vecPlaneNormal, const Vector &vecBoxExtents, Vector &vecBoxPoint );
	int AddPlane( const Vector &vecNormal );
protected:

	enum { MAX_CHECK_COUNT_DEPTH = 2 };

	int								m_nPower;								// Size of the displacement ( 2^power + 1 )

	CDispLeafLink					*m_pLeafLinkHead;						// List that links it into the leaves.

	Vector							m_vecSurfPoints[4];						// Base surface points.
	int								m_nContents;								// The displacement surface "contents" (solid, etc...)
	short							m_nSurfaceProps[2];						// Surface properties (save off from texdata for impact responses)
	int								m_nFlags;

	// Collision data.
	Vector							m_vecStabDir;							// Direction to stab for this displacement surface (is the base face normal)
	Vector							m_vecBounds[2];							// Bounding box of the displacement surface and base face

	int								m_nCheckCount[MAX_CHECK_COUNT_DEPTH];	// Per frame collision flag (so we check only once)

	CUtlVector<Vector>				m_aVerts;								// Displacement verts.
	CUtlVector<CDispCollTri>		m_aTris;								// Displacement triangles.
	CUtlVector<CDispCollAABBNode>	m_aNodes;								// Nodes.
	
	// Cache
	CUtlVector<CDispCollTriCache>	m_aTrisCache;
	CUtlVector<Vector> m_aEdgePlanes;

	CDispCollHelper					m_Helper;

	unsigned int					m_nSize;

#ifdef ENGINE_DLL
	memhandle_t						m_hCache;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: get the child node index given the current node index and direction
//          of the child (1 of 4)
//   Input: iNode - current node index
//          nDirection - direction of the child ( [0...3] - SW, SE, NW, NE )
//  Output: int - the index of the child node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetChild( int iNode, int nDirection )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_aNodes.Count() );

    // ( node index * 4 ) + ( direction + 1 )
    return ( ( iNode << 2 ) + ( nDirection + 1 ) );	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_CalcCount( int nPower )
{ 
	Assert( nPower >= 1 );
	Assert( nPower <= 4 );

	return ( ( 1 << ( ( nPower + 1 ) << 1 ) ) / 3 ); 
}

//-----------------------------------------------------------------------------
// Purpose: get the parent node index given the current node
//   Input: iNode - current node index
//  Output: int - the index of the parent node
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetParent( int iNode )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_aNodes.Count() );

	// ( node index - 1 ) / 4
	return ( ( iNode - 1 ) >> 2 );
}

//-----------------------------------------------------------------------------
// Purpose:
// TODO: should make this a function - not a hardcoded set of statements!!!
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetLevel( int iNode )
{
	// node range [0...m_NodeCount)
	Assert( iNode >= 0 );
	Assert( iNode < m_aNodes.Count() );

	// level = 2^n + 1
	if ( iNode == 0 )  { return 1; }
	if ( iNode < 5 )   { return 2; }
	if ( iNode < 21 )  { return 3; }
	if ( iNode < 85 )  { return 4; }
	if ( iNode < 341 ) { return 5; }

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline int CDispCollTree::Nodes_GetIndexFromComponents( int x, int y )
{
	int nIndex = 0;

	// Interleave bits from the x and y values to create the index
	int iShift;
	for( iShift = 0; x != 0; iShift += 2, x >>= 1 )
	{
		nIndex |= ( x & 1 ) << iShift;
	}

	for( iShift = 1; y != 0; iShift += 2, y >>= 1 )
	{
		nIndex |= ( y & 1 ) << iShift;
	}

	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTree::CalcClosestBoxPoint( const Vector &vecPlaneNormal, const Vector &vecBoxStart, 
											    const Vector &vecBoxExtents, Vector &vecBoxPoint )
{
	vecBoxPoint = vecBoxStart;
	( vecPlaneNormal[0] < 0.0f ) ? vecBoxPoint[0] += vecBoxExtents[0] : vecBoxPoint[0] -= vecBoxExtents[0];
	( vecPlaneNormal[1] < 0.0f ) ? vecBoxPoint[1] += vecBoxExtents[1] : vecBoxPoint[1] -= vecBoxExtents[1];
	( vecPlaneNormal[2] < 0.0f ) ? vecBoxPoint[2] += vecBoxExtents[2] : vecBoxPoint[2] -= vecBoxExtents[2];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CDispCollTree::CalcClosestExtents( const Vector &vecPlaneNormal, const Vector &vecBoxExtents, 
											   Vector &vecBoxPoint )
{
	( vecPlaneNormal[0] < 0.0f ) ? vecBoxPoint[0] = vecBoxExtents[0] : vecBoxPoint[0] = -vecBoxExtents[0];
	( vecPlaneNormal[1] < 0.0f ) ? vecBoxPoint[1] = vecBoxExtents[1] : vecBoxPoint[1] = -vecBoxExtents[1];
	( vecPlaneNormal[2] < 0.0f ) ? vecBoxPoint[2] = vecBoxExtents[2] : vecBoxPoint[2] = -vecBoxExtents[2];
}

//=============================================================================
// Global Helper Functions
CDispCollTree *DispCollTrees_Alloc( int count );
void DispCollTrees_Free( CDispCollTree *pTrees );

#endif // DISPCOLL_COMMON_H
