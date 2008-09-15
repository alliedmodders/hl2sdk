//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cmodel.h"
#include "dispcoll_common.h"
#include "collisionutils.h"
#include "vstdlib/strtools.h"
#include "tier0/vprof.h"
#include "tier1/fmtstr.h"
#include "tier1/utlhash.h"
#include "tier1/generichash.h"
#include "tier0/fasttimer.h"
#include "vphysics/virtualmesh.h"
#include "tier1/datamanager.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NEWRAYTEST 1

#if defined(_XBOX) && !defined(_RETAIL)
	inline void ValidateAABB( Vector *source, CDispCollAABBVector *dest )
	{
		if ( dest[0].operator Vector().x > source[0].x )
			__asm int 3;
		if ( dest[0].operator Vector().y > source[0].y )
			__asm int 3;
		if ( dest[0].operator Vector().z > source[0].z )
			__asm int 3;
		if ( dest[1].operator Vector().x < source[1].x )
			__asm int 3;
		if ( dest[1].operator Vector().y < source[1].y )
			__asm int 3;
		if ( dest[1].operator Vector().z < source[1].z )
			__asm int 3;
	}
#else
	#define ValidateAABB( source, dest ) ((void)0)
#endif

//=============================================================================
//	Cache

#ifdef ENGINE_DLL
CDataManager<CDispCollTree, CDispCollTree *, bool> g_DispCollTriCache( ( IsConsole() ) ? 192*1024 : 2048*1024 );
#endif


struct DispCollPlaneIndex_t
{
	Vector vecPlane;
	int index;
};

class CPlaneIndexHashFuncs
{
public:
	CPlaneIndexHashFuncs( int ) {}

	// Compare
	bool operator()( const DispCollPlaneIndex_t &lhs, const DispCollPlaneIndex_t &rhs ) const
	{
		return ( lhs.vecPlane == rhs.vecPlane || lhs.vecPlane == -rhs.vecPlane );
	}

	// Hash
	unsigned int operator()( const DispCollPlaneIndex_t &item ) const
	{
		return HashItem( item.vecPlane ) ^ HashItem( -item.vecPlane );
	}
};

CUtlHash<DispCollPlaneIndex_t, CPlaneIndexHashFuncs, CPlaneIndexHashFuncs> g_DispCollPlaneIndexHash( 512 );


//=============================================================================
//	Displacement Collision Triangle

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDispCollTri::CDispCollTri()
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::Init( void )
{
	m_vecNormal.Init();
	m_flDist = 0.0f;
	m_TriData[0].m_IndexDummy = m_TriData[1].m_IndexDummy = m_TriData[2].m_IndexDummy = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::CalcPlane( CUtlVector<Vector> &m_aVerts )
{
	Vector vecEdges[2];
	vecEdges[0] = m_aVerts[GetVert( 1 )] - m_aVerts[GetVert( 0 )];
	vecEdges[1] = m_aVerts[GetVert( 2 )] - m_aVerts[GetVert( 0 )];
	
	m_vecNormal = vecEdges[1].Cross( vecEdges[0] );
	VectorNormalize( m_vecNormal );
	m_flDist = m_vecNormal.Dot( m_aVerts[GetVert( 0 )] );

	// Calculate the signbits for the plane - fast test.
	m_ucSignBits = 0;
	m_ucPlaneType = PLANE_ANYZ;
	for ( int iAxis = 0; iAxis < 3 ; ++iAxis )
	{
		if ( m_vecNormal[iAxis] < 0.0f )
		{
			m_ucSignBits |= 1 << iAxis;
		}

		if ( m_vecNormal[iAxis] == 1.0f )
		{
			m_ucPlaneType = iAxis;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMin( float v1, float v2, float v3, int &iMin )
{
	float flMin = v1; 
	iMin = 0;
	if( v2 < flMin ) { flMin = v2; iMin = 1; }
	if( v3 < flMin ) { flMin = v3; iMin = 2; }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void FindMax( float v1, float v2, float v3, int &iMax )
{
	float flMax = v1;
	iMax = 0;
	if( v2 > flMax ) { flMax = v2; iMax = 1; }
	if( v3 > flMax ) { flMax = v3; iMax = 2; }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTri::FindMinMax( CUtlVector<Vector> &m_aVerts )
{
	int iMin, iMax;
	FindMin( m_aVerts[GetVert(0)].x, m_aVerts[GetVert(1)].x, m_aVerts[GetVert(2)].x, iMin );
	FindMax( m_aVerts[GetVert(0)].x, m_aVerts[GetVert(1)].x, m_aVerts[GetVert(2)].x, iMax );
	SetMin( 0, iMin );
	SetMax( 0, iMax );

	FindMin( m_aVerts[GetVert(0)].y, m_aVerts[GetVert(1)].y, m_aVerts[GetVert(2)].y, iMin );
	FindMax( m_aVerts[GetVert(0)].y, m_aVerts[GetVert(1)].y, m_aVerts[GetVert(2)].y, iMax );
	SetMin( 1, iMin );
	SetMax( 1, iMax );

	FindMin( m_aVerts[GetVert(0)].z, m_aVerts[GetVert(1)].z, m_aVerts[GetVert(2)].z, iMin );
	FindMax( m_aVerts[GetVert(0)].z, m_aVerts[GetVert(1)].z, m_aVerts[GetVert(2)].z, iMax );
	SetMin( 2, iMin );
	SetMax( 2, iMax );
}

//=============================================================================
//	AABB Node

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDispCollAABBNode::CDispCollAABBNode()
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollAABBNode::Init( void )
{
	m_vecBox[0].Init();
	m_vecBox[1].Init();
	m_iTris[0] = m_iTris[1] = DISPCOLL_INVALID_TRI;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollAABBNode::GenerateBox( CUtlVector<CDispCollTri> &m_aTris, CUtlVector<Vector> &m_aVerts )
{
	Vector vecBox[2];
	vecBox[0].Init( FLT_MAX, FLT_MAX, FLT_MAX );
	vecBox[1].Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	for ( int iTri = 0; iTri < 2; ++iTri )
	{
		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			VectorMin( m_aVerts[m_aTris[m_iTris[iTri]].GetVert( iVert )], vecBox[0], vecBox[0] );
			VectorMax( m_aVerts[m_aTris[m_iTris[iTri]].GetVert( iVert )], vecBox[1], vecBox[1] );
		}
	}

	m_vecBox[0].SetMins( vecBox[0] );
	m_vecBox[1].SetMaxs( vecBox[1] );

	ValidateAABB( vecBox, m_vecBox );
}

//-----------------------------------------------------------------------------
// Purpose: Create the AABB tree.
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Create( CCoreDispInfo *pDisp )
{
	// Copy the flags.
	m_nFlags = pDisp->GetSurface()->GetFlags();

	// Copy necessary displacement data.
	AABBTree_CopyDispData( pDisp );
	
	// Setup/create the leaf nodes first so the recusion can use this data to stop.
	AABBTree_CreateLeafs();

	// Generate bounding boxes.
	AABBTree_GenerateBoxes();

	// Create the bounding box of the displacement surface + the base face.
	AABBTree_CalcBounds();

	// Successful.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CopyDispData( CCoreDispInfo *pDisp )
{
	// Displacement size.
	m_nPower = pDisp->GetPower();

	// Displacement base surface data.
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_nContents = pSurf->GetContents();
	pSurf->GetNormal( m_vecStabDir );
	for ( int iPoint = 0; iPoint < 4; iPoint++ )
	{
		pSurf->GetPoint( iPoint, m_vecSurfPoints[iPoint] );
	}

	// Allocate collision tree data.
	{
	MEM_ALLOC_CREDIT();
	m_aVerts.SetSize( GetSize() );
	}

	{
	MEM_ALLOC_CREDIT();
	m_aTris.SetSize( GetTriSize() );
	}

	{
	MEM_ALLOC_CREDIT();
	m_aNodes.SetSize( Nodes_CalcCount( m_nPower ) );
	}

	// Setup size.
	m_nSize = sizeof( this );
	m_nSize += sizeof( Vector ) * GetSize();
	m_nSize += sizeof( CDispCollTri ) * GetTriSize();
	m_nSize += sizeof( CDispCollAABBNode ) * Nodes_CalcCount( m_nPower );
	m_nSize += sizeof( CDispCollTri* ) * DISPCOLL_TREETRI_SIZE;

	// Copy vertex data.
	for ( int iVert = 0; iVert < m_aVerts.Count(); iVert++ )
	{
		pDisp->GetVert( iVert, m_aVerts[iVert] );
	}

	// Copy and setup triangle data.
	unsigned short iVerts[3];
	for ( int iTri = 0; iTri < m_aTris.Count(); ++iTri )
	{
		pDisp->GetTriIndices( iTri, iVerts[0], iVerts[1], iVerts[2] );
		m_aTris[iTri].SetVert( 0, iVerts[0] );
		m_aTris[iTri].SetVert( 1, iVerts[1] );
		m_aTris[iTri].SetVert( 2, iVerts[2] );
		m_aTris[iTri].m_uiFlags = pDisp->GetTriTagValue( iTri );

		// Calculate the surface props and set flags.
		float flTotalAlpha = 0.0f;
		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			flTotalAlpha += pDisp->GetAlpha( m_aTris[iTri].GetVert( iVert ) );
		}

		if ( flTotalAlpha > DISP_ALPHA_PROP_DELTA )
		{
			m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFPROP2;
		}
		else
		{
			m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFPROP1;
		}

		// Add the displacement surface flag.
		m_aTris[iTri].m_uiFlags |= DISPSURF_FLAG_SURFACE;

		// Calculate the plane normal and the min max.
		m_aTris[iTri].CalcPlane( m_aVerts );
		m_aTris[iTri].FindMinMax( m_aVerts );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CreateLeafs( void )
{
	// Find the bottom leftmost node.
	int iMinNode = 0;
	for ( int iPower = 0; iPower < m_nPower; ++iPower )
	{
		iMinNode = Nodes_GetChild( iMinNode, 0 );
	}

	// Get the width and height of the displacement.
	int nWidth = GetWidth() - 1;
	int nHeight = GetHeight() - 1;

	for ( int iHgt = 0; iHgt < nHeight; ++iHgt )
	{
		for ( int iWid = 0; iWid < nWidth; ++iWid )
		{
			int iNode = Nodes_GetIndexFromComponents( iWid, iHgt );
			iNode += iMinNode;

			int iIndex = iHgt * nWidth + iWid;
			int iTri = iIndex * 2;

			m_aNodes[iNode].m_iTris[0] = iTri;
			m_aNodes[iNode].m_iTris[1] = iTri + 1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_GenerateBoxes( void )
{
	Vector vecBox[2];
	for ( int iNode = ( m_aNodes.Count() - 1 ); iNode >= 0; --iNode )
	{
		// Leaf?
		if ( m_aNodes[iNode].IsLeaf() )
		{
			m_aNodes[iNode].GenerateBox( m_aTris, m_aVerts );
		}
		else
		{
			// Get bounds from children.

			vecBox[0].Init( FLT_MAX, FLT_MAX, FLT_MAX );
			vecBox[1].Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );
			
			for ( int iChild = 0; iChild < 4; ++iChild )
			{
				int iChildNode = Nodes_GetChild( iNode, iChild );
				
				VectorMin( m_aNodes[iChildNode].m_vecBox[0], vecBox[0], vecBox[0] );
				VectorMax( m_aNodes[iChildNode].m_vecBox[1], vecBox[1], vecBox[1] );
			}
			m_aNodes[iNode].m_vecBox[0].SetMins( vecBox[0] );
			m_aNodes[iNode].m_vecBox[1].SetMaxs( vecBox[1] );

			ValidateAABB( vecBox, m_aNodes[iNode].m_vecBox );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_CalcBounds( void )
{
	// Check data.
	if ( ( m_aVerts.Count() == 0 ) || ( m_aNodes.Count() == 0 ) )
		return;

	m_vecBounds[0] = m_aNodes[0].m_vecBox[0];
	m_vecBounds[1] = m_aNodes[0].m_vecBox[1];
	
	// Add surface points to bounds.
	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		VectorMin( m_vecSurfPoints[iPoint], m_vecBounds[0], m_vecBounds[0] );
		VectorMax( m_vecSurfPoints[iPoint], m_vecBounds[1], m_vecBounds[1] );
	}

	// Bloat a little.
	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		m_vecBounds[0][iAxis] -= 1.0f;
		m_vecBounds[1][iAxis] += 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CDispCollTree::CheckCache()
{
#ifdef ENGINE_DLL
	if ( !g_DispCollTriCache.GetResource_NoLock( m_hCache ) )
	{
		Cache();
		m_hCache = g_DispCollTriCache.CreateResource( this );
		//Msg( "Adding 0x%x to cache (actual %d) [%d, %d --> %.2f] %d total, %d unique\n", this, GetCacheMemorySize(), GetTriSize(), m_aEdgePlanes.Count(), (float)m_aEdgePlanes.Count()/(float)GetTriSize(), totals, uniques );
	}
#else
	Cache();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::Cache( void )
{
	if ( m_aTrisCache.Count() == GetTriSize() )
	{
		return;
	}

	VPROF( "CDispCollTree::Cache" );

	// Alloc.
//	int nSize = sizeof( CDispCollTriCache ) * GetTriSize();
	int nTriCount = GetTriSize();
	{
	MEM_ALLOC_CREDIT();
	m_aTrisCache.SetSize( nTriCount );
	}

	for ( int iTri = 0; iTri < nTriCount; ++iTri )
	{
		Cache_Create( &m_aTris[iTri], iTri );
	}

	g_DispCollPlaneIndexHash.Purge();
}

#ifdef NEWRAYTEST
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, RayDispOutput_t &output )
{
	VPROF( "DispRayTest" );

	// Check for ray test.
	if ( CheckFlags( CCoreDispInfo::SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;

	// Pre-calc the inverse delta for perf.
	CDispCollTri *pImpactTri = NULL;

	Vector vecInvDelta;
	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		if ( ray.m_Delta[iAxis] != 0.0f )
		{
			vecInvDelta[iAxis] = 1.0f / ray.m_Delta[iAxis];
		}
		else
		{
			vecInvDelta[iAxis] = FLT_MAX;
		}
	}
	
	if ( IsBoxIntersectingRay( m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[0], 
		                       m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[1], 
		                       ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON ) )
	{
		AABBTree_TreeTrisRayBarycentricTest_r( ray, vecInvDelta, DISPCOLL_ROOTNODE_INDEX, output, &pImpactTri );
	}

	if ( pImpactTri )
	{
		// Collision.
		output.ndxVerts[0] = pImpactTri->GetVert( 0 );
		output.ndxVerts[1] = pImpactTri->GetVert( 2 );
		output.ndxVerts[2] = pImpactTri->GetVert( 1 );

		Assert( (output.u <= 1.0f ) && ( output.v <= 1.0f ) );
		Assert( (output.u >= 0.0f ) && ( output.v >= 0.0f ) );

		return true;
	}

	// No collision.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_TreeTrisRayBarycentricTest_r( const Ray_t &ray, const Vector &vecInvDelta, int iNode, RayDispOutput_t &output, CDispCollTri **pImpactTri )
{
	if ( m_aNodes[iNode].IsLeaf() )
	{
		float flU, flV, flT;

		CDispCollTri *pTri0 = &m_aTris[m_aNodes[iNode].m_iTris[0]];
		CDispCollTri *pTri1 = &m_aTris[m_aNodes[iNode].m_iTris[1]];
		if ( ComputeIntersectionBarycentricCoordinates( ray, m_aVerts[pTri0->GetVert( 0 )], m_aVerts[pTri0->GetVert( 2 )], m_aVerts[pTri0->GetVert( 1 )], flU, flV, &flT ) )
		{
			// Make sure it's inside the range
			if ( ( flU >= 0.0f ) && ( flV >= 0.0f ) && ( ( flU + flV ) <= 1.0f ) )
			{
				if( ( flT > 0.0f ) && ( flT < output.dist ) )
				{
					(*pImpactTri) = pTri0;
					output.u = flU;
					output.v = flV;
					output.dist = flT;
				}
			}
		}

		if ( ComputeIntersectionBarycentricCoordinates( ray, m_aVerts[pTri1->GetVert( 0 )], m_aVerts[pTri1->GetVert( 2 )], m_aVerts[pTri1->GetVert( 1 )], flU, flV, &flT ) )
		{
			// Make sure it's inside the range
			if ( ( flU >= 0.0f ) && ( flV >= 0.0f ) && ( ( flU + flV ) <= 1.0f ) )
			{
				if( ( flT > 0.0f ) && ( flT < output.dist ) )
				{
					(*pImpactTri) = pTri1;
					output.u = flU;
					output.v = flV;
					output.dist = flT;
				}
			}
		}
	}
	else
	{
		int iChildNode[4];
		iChildNode[0] = Nodes_GetChild( iNode, 0 );
		iChildNode[1] = Nodes_GetChild( iNode, 1 );
		iChildNode[2] = Nodes_GetChild( iNode, 2 );
		iChildNode[3] = Nodes_GetChild( iNode, 3 );

		bool bIntersectChild[4];
		bIntersectChild[0] = IsBoxIntersectingRay( m_aNodes[iChildNode[0]].m_vecBox[0], m_aNodes[iChildNode[0]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[1] = IsBoxIntersectingRay( m_aNodes[iChildNode[1]].m_vecBox[0], m_aNodes[iChildNode[1]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[2] = IsBoxIntersectingRay( m_aNodes[iChildNode[2]].m_vecBox[0], m_aNodes[iChildNode[2]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[3] = IsBoxIntersectingRay( m_aNodes[iChildNode[3]].m_vecBox[0], m_aNodes[iChildNode[3]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );

		if ( bIntersectChild[0] ) { AABBTree_TreeTrisRayBarycentricTest_r( ray, vecInvDelta, iChildNode[0], output, pImpactTri ); }
		if ( bIntersectChild[1] ) { AABBTree_TreeTrisRayBarycentricTest_r( ray, vecInvDelta, iChildNode[1], output, pImpactTri ); }
		if ( bIntersectChild[2] ) { AABBTree_TreeTrisRayBarycentricTest_r( ray, vecInvDelta, iChildNode[2], output, pImpactTri ); }
		if ( bIntersectChild[3] ) { AABBTree_TreeTrisRayBarycentricTest_r( ray, vecInvDelta, iChildNode[3], output, pImpactTri ); }
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, RayDispOutput_t &output )
{
	// Check for ray test.
	if ( CheckFlags( SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;

	// Build the triangle list.
	unsigned short nTreeTriCount = 0;
	CDispCollTri *aTreeTris[DISPCOLL_TREETRI_SIZE];
	AABBTree_BuildTreeTrisSweep_r( ray, DISPCOLL_ROOTNODE_INDEX, &aTreeTris[0], nTreeTriCount );
	if ( nTreeTriCount == 0 )
		return false;

	// Init output data.
	float flMinT = 1.0f;
	float flMinU = 0.0f;
	float flMinV = 0.0f;

	CDispCollTri *pImpactTri = NULL;
	for( int iTri = 0; iTri < nTreeTriCount; ++iTri )
	{
		float flU, flV, flT;

		CDispCollTri *pTri = aTreeTris[iTri];
		if ( ComputeIntersectionBarycentricCoordinates( ray, m_aVerts[pTri->GetVert( 0 )], m_aVerts[pTri->GetVert( 2 )], m_aVerts[pTri->GetVert( 1 )], flU, flV, &flT ) )
		{
			// Make sure it's inside the range
			if ( ( flU < 0.0f ) || ( flV < 0.0f ) || ( ( flU + flV ) > 1.0f ) )
				continue;

			if( ( flT > 0.0f ) && ( flT < flMinT ) )
			{
				pImpactTri = pTri;
				flMinT = flT;
				flMinU = flU;
				flMinV = flV;
			}
		}
	}

	// Collision.
	if( pImpactTri )
	{
		output.ndxVerts[0] = pImpactTri->GetVert( 0 );
		output.ndxVerts[1] = pImpactTri->GetVert( 1 );
		output.ndxVerts[2] = pImpactTri->GetVert( 2 );
		output.u = flMinU;
		output.v = flMinV;
		output.dist = flMinT;

		Assert( (output.u <= 1.0f ) && ( output.v <= 1.0f ) );
		Assert( (output.u >= 0.0f ) && ( output.v >= 0.0f ) );

		return true;
	}

	// No collision.
	return false;
}
#endif

#ifdef NEWRAYTEST
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, CBaseTrace *pTrace, bool bSide )
{
//	VPROF_BUDGET( "DispRayTraces", VPROF_BUDGETGROUP_DISP_RAYTRACES );

	// Check for ray test.
	if ( CheckFlags( CCoreDispInfo::SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;

	// Pre-calc the inverse delta for perf.
	CDispCollTri *pImpactTri = NULL;

	Vector vecInvDelta;
	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		if ( ray.m_Delta[iAxis] != 0.0f )
		{
			vecInvDelta[iAxis] = 1.0f / ray.m_Delta[iAxis];
		}
		else
		{
			vecInvDelta[iAxis] = FLT_MAX;
		}
	}

	if ( IsBoxIntersectingRay( m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[0], 
		                       m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[1], 
		                       ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON ) )
	{
		AABBTree_TreeTrisRayTest_r( ray, vecInvDelta, DISPCOLL_ROOTNODE_INDEX, pTrace, bSide, &pImpactTri );
	}

	if ( pImpactTri )
	{
		// Collision.
		VectorCopy( pImpactTri->m_vecNormal, pTrace->plane.normal );
		pTrace->plane.dist = pImpactTri->m_flDist;
		pTrace->dispFlags = pImpactTri->m_uiFlags;
		return true;
	}
	
	// No collision.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_TreeTrisRayTest_r( const Ray_t &ray, const Vector &vecInvDelta, int iNode, CBaseTrace *pTrace, bool bSide, CDispCollTri **pImpactTri )
{
	if ( m_aNodes[iNode].IsLeaf() )
	{
		CDispCollTri *pTri0 = &m_aTris[m_aNodes[iNode].m_iTris[0]];
		CDispCollTri *pTri1 = &m_aTris[m_aNodes[iNode].m_iTris[1]];
		float flFrac = IntersectRayWithTriangle( ray, m_aVerts[pTri0->GetVert( 0 )], m_aVerts[pTri0->GetVert( 2 )], m_aVerts[pTri0->GetVert( 1 )], bSide );
		if( ( flFrac >= 0.0f ) && ( flFrac < pTrace->fraction ) )
		{
			pTrace->fraction = flFrac;
			(*pImpactTri) = pTri0;
		}
		
		flFrac = IntersectRayWithTriangle( ray, m_aVerts[pTri1->GetVert( 0 )], m_aVerts[pTri1->GetVert( 2 )], m_aVerts[pTri1->GetVert( 1 )], bSide );
		if( ( flFrac >= 0.0f ) && ( flFrac < pTrace->fraction ) )
		{
			pTrace->fraction = flFrac;
			(*pImpactTri) = pTri1;
		}
	}
	else
	{
		int iChildNode[4];
		iChildNode[0] = Nodes_GetChild( iNode, 0 );
		iChildNode[1] = Nodes_GetChild( iNode, 1 );
		iChildNode[2] = Nodes_GetChild( iNode, 2 );
		iChildNode[3] = Nodes_GetChild( iNode, 3 );
		
		bool bIntersectChild[4];
		bIntersectChild[0] = IsBoxIntersectingRay( m_aNodes[iChildNode[0]].m_vecBox[0], m_aNodes[iChildNode[0]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[1] = IsBoxIntersectingRay( m_aNodes[iChildNode[1]].m_vecBox[0], m_aNodes[iChildNode[1]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[2] = IsBoxIntersectingRay( m_aNodes[iChildNode[2]].m_vecBox[0], m_aNodes[iChildNode[2]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[3] = IsBoxIntersectingRay( m_aNodes[iChildNode[3]].m_vecBox[0], m_aNodes[iChildNode[3]].m_vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );

		if ( bIntersectChild[0] ) { AABBTree_TreeTrisRayTest_r( ray, vecInvDelta, iChildNode[0], pTrace, bSide, pImpactTri ); }
		if ( bIntersectChild[1] ) { AABBTree_TreeTrisRayTest_r( ray, vecInvDelta, iChildNode[1], pTrace, bSide, pImpactTri ); }
		if ( bIntersectChild[2] ) { AABBTree_TreeTrisRayTest_r( ray, vecInvDelta, iChildNode[2], pTrace, bSide, pImpactTri ); }
		if ( bIntersectChild[3] ) { AABBTree_TreeTrisRayTest_r( ray, vecInvDelta, iChildNode[3], pTrace, bSide, pImpactTri ); }
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_Ray( const Ray_t &ray, CBaseTrace *pTrace, bool bSide )
{
	// Check for ray test.
	if ( CheckFlags( CCoreDispInfo::SURF_NORAY_COLL ) )
		return false;

	// Check for opacity.
	if ( !( m_nContents & MASK_OPAQUE ) )
		return false;	

	// Build the triangle list.
	unsigned short nTreeTriCount = 0;
	CDispCollTri *aTreeTris[DISPCOLL_TREETRI_SIZE];
	AABBTree_BuildTreeTrisSweep_r( ray, DISPCOLL_ROOTNODE_INDEX, &aTreeTris[0], nTreeTriCount );
	if ( nTreeTriCount == 0 )
		return false;

	CDispCollTri *pImpactTri = NULL;
	for( int iTri = 0; iTri < nTreeTriCount; ++iTri )
	{
		CDispCollTri *pTri = aTreeTris[iTri];
		float flFrac = IntersectRayWithTriangle( ray, m_aVerts[pTri->GetVert( 0 )], m_aVerts[pTri->GetVert( 2 )], m_aVerts[pTri->GetVert( 1 )], bSide );
		if( flFrac < 0.0f )
			continue;			
		
		if( flFrac < pTrace->fraction )
		{
			pTrace->fraction = flFrac;
			pImpactTri = pTri;
		}
	}

	if ( pImpactTri )
	{
		pTrace->plane.normal = pImpactTri->m_vecNormal;
		pTrace->plane.dist = pImpactTri->m_flDist;
		pTrace->dispFlags = pImpactTri->m_uiFlags;
		return true;
	}

	// No collision.
	return false;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_BuildTreeTrisSweep_r( const Ray_t &ray, int iNode, CDispCollTri **ppTreeTris, unsigned short &nTriCount )
{
	bool bIntersect = IsBoxIntersectingRay( m_aNodes[iNode].m_vecBox[0], m_aNodes[iNode].m_vecBox[1], ray, DISPCOLL_DIST_EPSILON );
	if ( bIntersect )
	{
		if ( m_aNodes[iNode].IsLeaf() )
		{
			if ( nTriCount <= (DISPCOLL_TREETRI_SIZE-2) )
			{
				ppTreeTris[nTriCount] = &m_aTris[m_aNodes[iNode].m_iTris[0]];
				ppTreeTris[nTriCount+1] = &m_aTris[m_aNodes[iNode].m_iTris[1]];
				nTriCount += 2;
			}
			else
			{
				// If we get here, then it generated more triangles than there should even be in a displacement.
				Assert( false );
			}
		}
		else
		{
			AABBTree_BuildTreeTrisSweep_r( ray, Nodes_GetChild( iNode, 0 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisSweep_r( ray, Nodes_GetChild( iNode, 1 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisSweep_r( ray, Nodes_GetChild( iNode, 2 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisSweep_r( ray, Nodes_GetChild( iNode, 3 ), ppTreeTris, nTriCount );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_BuildTreeTrisIntersect_r( const Ray_t &ray, int iNode, CDispCollTri **ppTreeTris, unsigned short &nTriCount )
{
	bool bIntersect = false;
	if ( ray.m_IsRay )
	{
		bIntersect = IsPointInBox( ray.m_Start, m_aNodes[iNode].m_vecBox[0], m_aNodes[iNode].m_vecBox[1] );
	}
	else
	{
		// Box.
		Vector vecMin, vecMax;
		VectorSubtract( m_aNodes[iNode].m_vecBox[0], ray.m_Extents, vecMin );
		VectorAdd( m_aNodes[iNode].m_vecBox[1], ray.m_Extents, vecMax );
		bIntersect = IsPointInBox( ray.m_Start, vecMin, vecMax );
	}

	if ( bIntersect )
	{
		if ( m_aNodes[iNode].IsLeaf() )
		{
			if ( nTriCount < DISPCOLL_TREETRI_SIZE )
			{
				ppTreeTris[nTriCount] = &m_aTris[m_aNodes[iNode].m_iTris[0]];
				ppTreeTris[nTriCount+1] = &m_aTris[m_aNodes[iNode].m_iTris[1]];
				nTriCount += 2;
			}
		}
		else
		{
			AABBTree_BuildTreeTrisIntersect_r( ray, Nodes_GetChild( iNode, 0 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisIntersect_r( ray, Nodes_GetChild( iNode, 1 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisIntersect_r( ray, Nodes_GetChild( iNode, 2 ), ppTreeTris, nTriCount );
			AABBTree_BuildTreeTrisIntersect_r( ray, Nodes_GetChild( iNode, 3 ), ppTreeTris, nTriCount );
		}
	}
}

void CDispCollTree::AABBTree_GetTrisInSphere( const Vector &center, float radius, unsigned short *pIndexOut, int indexMax, int *pIndexCount )
{
	unsigned short triCount = 0;
	AABBTree_BuildTreeTrisInSphere_r( center, radius, DISPCOLL_ROOTNODE_INDEX, pIndexOut, indexMax, triCount );
	*pIndexCount = triCount;
}

void CDispCollTree::AABBTree_BuildTreeTrisInSphere_r( const Vector &center, float radius, int iNode, unsigned short *pIndexOut, unsigned short indexMax, unsigned short &nTriCount  )
{
	if ( IsBoxIntersectingSphere( m_aNodes[iNode].m_vecBox[0],m_aNodes[iNode].m_vecBox[1], center, radius ) )
	{
		if ( m_aNodes[iNode].IsLeaf() )
		{
			if ( (nTriCount+2) <= indexMax )
			{
				pIndexOut[nTriCount] = m_aNodes[iNode].m_iTris[0];
				pIndexOut[nTriCount+1] = m_aNodes[iNode].m_iTris[1];
				nTriCount += 2;
			}
		}
		else
		{
			AABBTree_BuildTreeTrisInSphere_r( center, radius, Nodes_GetChild( iNode, 0 ), pIndexOut, indexMax, nTriCount );
			AABBTree_BuildTreeTrisInSphere_r( center, radius, Nodes_GetChild( iNode, 1 ), pIndexOut, indexMax, nTriCount );
			AABBTree_BuildTreeTrisInSphere_r( center, radius, Nodes_GetChild( iNode, 2 ), pIndexOut, indexMax, nTriCount );
			AABBTree_BuildTreeTrisInSphere_r( center, radius, Nodes_GetChild( iNode, 3 ), pIndexOut, indexMax, nTriCount );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_IntersectAABB( const Ray_t &ray )
{
	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;
	
	cplane_t plane;

	// Build the triangle list.
	unsigned short nTreeTriCount = 0;
	CDispCollTri *aTreeTris[DISPCOLL_TREETRI_SIZE];
	AABBTree_BuildTreeTrisIntersect_r( ray, DISPCOLL_ROOTNODE_INDEX, &aTreeTris[0], nTreeTriCount );
	if ( nTreeTriCount == 0 )
		return false;

	// Test the axial-aligned box against the triangle list.
	for ( int iTri = 0; iTri < nTreeTriCount; ++iTri )
	{
		CDispCollTri *pTri = aTreeTris[iTri];

		VectorCopy( pTri->m_vecNormal, plane.normal );
		plane.dist = pTri->m_flDist;
		plane.signbits = pTri->m_ucSignBits;
		plane.type = pTri->m_ucPlaneType;

		if ( IsBoxIntersectingTriangle( ray.m_Start, ray.m_Extents,
			                            m_aVerts[pTri->GetVert( 0 )],
			                            m_aVerts[pTri->GetVert( 2 )],
			                            m_aVerts[pTri->GetVert( 1 )],
										plane, 0.0f ) )
			return true;
	}

	// no collision
	return false; 
}

#ifdef NEWRAYTEST
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline float Invert( float f )
{
	return ( f != 0.0 ) ? 1.0f / f : FLT_MAX;
}

bool CDispCollTree::AABBTree_SweepAABB( const Ray_t &ray, CBaseTrace *pTrace )
{
//	VPROF_BUDGET( "DispHullTraces", VPROF_BUDGETGROUP_DISP_HULLTRACES );

	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;

	// Test ray against the triangles in the list.
	Vector rayDir;
	VectorCopy( ray.m_Delta, rayDir );
	float flRayLength = VectorNormalize( rayDir );
	float flExtentLength = ray.m_Extents.Length();
	if ( flRayLength < ( flExtentLength * 0.5f ) )
	{
		return AABBTree_SweepAABBBox( ray, rayDir, pTrace );
	}

	// Save fraction.
	float flFrac = pTrace->fraction;
	
	// Calculate the inverse ray delta.
	Vector vecInvDelta;
	
	vecInvDelta[0] = Invert( ray.m_Delta[0] );
	vecInvDelta[1] = Invert( ray.m_Delta[1] );
	vecInvDelta[2] = Invert( ray.m_Delta[2] );
		
	const CDispCollAABBVector *vecNodeBox = m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox;
	Vector vecBox[2];
	VectorSubtract( vecNodeBox[0], ray.m_Extents, vecBox[0] );
	VectorAdd( vecNodeBox[1], ray.m_Extents, vecBox[1] );
	if( IsBoxIntersectingRay( vecBox[0], vecBox[1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON ) )
	{
		AABBTree_TreeTrisSweepTest_r( AABBTree_TreeTrisSweepTest_Args_t( ray, vecInvDelta, rayDir, pTrace ), DISPCOLL_ROOTNODE_INDEX );
	}

	// Collision.
	if ( pTrace->fraction < flFrac )
		return true;
	
	// No collision.
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::AABBTree_TreeTrisSweepTest_r( const AABBTree_TreeTrisSweepTest_Args_t &args, int iNode )
{
	const CDispCollAABBNode &node = m_aNodes[iNode];
	const Ray_t &ray = args.ray;
	if ( node.IsLeaf() )
	{
		int iTri0 = node.m_iTris[0];
		int iTri1 = node.m_iTris[1];
		CDispCollTri *pTri0 = &m_aTris[iTri0];
		CDispCollTri *pTri1 = &m_aTris[iTri1];
		const Vector &rayDir = args.rayDir;
		CBaseTrace *pTrace = args.pTrace;

		SweepAABBTriIntersect( ray, rayDir, iTri0, pTri0, pTrace, false );
		SweepAABBTriIntersect( ray, rayDir, iTri1, pTri1, pTrace, false );
	}
	else
	{
		int iChildNode[4];
		iChildNode[0] = Nodes_GetChild( iNode, 0 );
		iChildNode[1] = Nodes_GetChild( iNode, 1 );
		iChildNode[2] = Nodes_GetChild( iNode, 2 );
		iChildNode[3] = Nodes_GetChild( iNode, 3 );

		Vector vecBox[4][2];

		const CDispCollAABBNode &node0 = m_aNodes[iChildNode[0]];
		VectorSubtract( node0.m_vecBox[0], ray.m_Extents, vecBox[0][0] );
		VectorAdd( node0.m_vecBox[1], ray.m_Extents, vecBox[0][1] );

		const CDispCollAABBNode &node1 = m_aNodes[iChildNode[1]];
		VectorSubtract( node1.m_vecBox[0], ray.m_Extents, vecBox[1][0] );
		VectorAdd( node1.m_vecBox[1], ray.m_Extents, vecBox[1][1] );

		const CDispCollAABBNode &node2 = m_aNodes[iChildNode[2]];
		VectorSubtract( node2.m_vecBox[0], ray.m_Extents, vecBox[2][0] );
		VectorAdd( node2.m_vecBox[1], ray.m_Extents, vecBox[2][1] );

		const CDispCollAABBNode &node3 = m_aNodes[iChildNode[3]];
		VectorSubtract( node3.m_vecBox[0], ray.m_Extents, vecBox[3][0] );
		VectorAdd( node3.m_vecBox[1], ray.m_Extents, vecBox[3][1] );

		bool bIntersectChild[4];
		const Vector &vecInvDelta = args.vecInvDelta;
		bIntersectChild[0] = IsBoxIntersectingRay( vecBox[0][0], vecBox[0][1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[1] = IsBoxIntersectingRay( vecBox[1][0], vecBox[1][1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[2] = IsBoxIntersectingRay( vecBox[2][0], vecBox[2][1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );
		bIntersectChild[3] = IsBoxIntersectingRay( vecBox[3][0], vecBox[3][1], ray.m_Start, ray.m_Delta, vecInvDelta, DISPCOLL_DIST_EPSILON );

		if ( bIntersectChild[0] ) { AABBTree_TreeTrisSweepTest_r( args, iChildNode[0] ); }
		if ( bIntersectChild[1] ) { AABBTree_TreeTrisSweepTest_r( args, iChildNode[1] ); }
		if ( bIntersectChild[2] ) { AABBTree_TreeTrisSweepTest_r( args, iChildNode[2] ); }
		if ( bIntersectChild[3] ) { AABBTree_TreeTrisSweepTest_r( args, iChildNode[3] ); }
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_SweepAABBBox( const Ray_t &ray, const Vector &rayDir, CBaseTrace *pTrace )
{
	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;

	// Save fraction.
	float flFrac = pTrace->fraction; 

	// Create the box.
	Vector vecMin, vecMax;
	for ( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		if ( ray.m_Delta[iAxis] < 0.0f )
		{
			vecMin[iAxis] = ray.m_Start[iAxis] - ray.m_Extents[iAxis];
			vecMax[iAxis] = ray.m_Start[iAxis] + ray.m_Extents[iAxis];
			vecMin[iAxis] += ray.m_Delta[iAxis];
		}
		else
		{
			vecMin[iAxis] = ray.m_Start[iAxis] - ray.m_Extents[iAxis];
			vecMax[iAxis] = ray.m_Start[iAxis] + ray.m_Extents[iAxis];
			vecMax[iAxis] += ray.m_Delta[iAxis];
		}
	}

	if ( IsBoxIntersectingBox( m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[0], m_aNodes[DISPCOLL_ROOTNODE_INDEX].m_vecBox[1], vecMin, vecMax ) )
	{
		AABBTree_TreeTrisSweepTestBox_r( ray, rayDir, vecMin, vecMax, DISPCOLL_ROOTNODE_INDEX, pTrace );
	}

	// Collision.
	if ( pTrace->fraction < flFrac )
		return true;
	
	// No collision.
	return false;
}

void CDispCollTree::AABBTree_TreeTrisSweepTestBox_r( const Ray_t &ray, const Vector &rayDir, const Vector &vecMin, const Vector &vecMax,
													 int iNode, CBaseTrace *pTrace )
{
	if ( m_aNodes[iNode].IsLeaf() )
	{
		int iTri0 = m_aNodes[iNode].m_iTris[0];
		int iTri1 = m_aNodes[iNode].m_iTris[1];

		CDispCollTri *pTri0 = &m_aTris[m_aNodes[iNode].m_iTris[0]];
		CDispCollTri *pTri1 = &m_aTris[m_aNodes[iNode].m_iTris[1]];

		SweepAABBTriIntersect( ray, rayDir, iTri0, pTri0, pTrace, false );
		SweepAABBTriIntersect( ray, rayDir, iTri1, pTri1, pTrace, false );
	}
	else
	{
		int iChildNode[4];
		iChildNode[0] = Nodes_GetChild( iNode, 0 );
		iChildNode[1] = Nodes_GetChild( iNode, 1 );
		iChildNode[2] = Nodes_GetChild( iNode, 2 );
		iChildNode[3] = Nodes_GetChild( iNode, 3 );

		bool bIntersectChild[4];
		bIntersectChild[0] = IsBoxIntersectingBox( m_aNodes[iChildNode[0]].m_vecBox[0], m_aNodes[iChildNode[0]].m_vecBox[1], vecMin, vecMax );
		bIntersectChild[1] = IsBoxIntersectingBox( m_aNodes[iChildNode[1]].m_vecBox[0], m_aNodes[iChildNode[1]].m_vecBox[1], vecMin, vecMax );
		bIntersectChild[2] = IsBoxIntersectingBox( m_aNodes[iChildNode[2]].m_vecBox[0], m_aNodes[iChildNode[2]].m_vecBox[1], vecMin, vecMax );
		bIntersectChild[3] = IsBoxIntersectingBox( m_aNodes[iChildNode[3]].m_vecBox[0], m_aNodes[iChildNode[3]].m_vecBox[1], vecMin, vecMax );

		if ( bIntersectChild[0] ) { AABBTree_TreeTrisSweepTestBox_r( ray, rayDir, vecMin, vecMax, iChildNode[0], pTrace ); }
		if ( bIntersectChild[1] ) { AABBTree_TreeTrisSweepTestBox_r( ray, rayDir, vecMin, vecMax, iChildNode[1], pTrace ); }
		if ( bIntersectChild[2] ) { AABBTree_TreeTrisSweepTestBox_r( ray, rayDir, vecMin, vecMax, iChildNode[2], pTrace ); }
		if ( bIntersectChild[3] ) { AABBTree_TreeTrisSweepTestBox_r( ray, rayDir, vecMin, vecMax, iChildNode[3], pTrace ); }
	}
}
#else
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDispCollTree::AABBTree_SweepAABB( const Ray_t &ray, CBaseTrace *pTrace )
{
//	VPROF_BUDGET( "DispHullTraces", VPROF_BUDGETGROUP_DISP_HULLTRACES );

	// Check for hull test.
	if ( CheckFlags( CCoreDispInfo::SURF_NOHULL_COLL ) )
		return false;

	// Build the triangle list.
	unsigned short nTreeTriCount = 0;
	CDispCollTri *aTreeTris[DISPCOLL_TREETRI_SIZE];
	AABBTree_BuildTreeTrisSweep_r( ray, DISPCOLL_ROOTNODE_INDEX, &aTreeTris[0], nTreeTriCount );
	if ( !nTreeTriCount )
		return false;

	// Save min fraction.
	float flMinFrac = pTrace->fraction;

	// Test ray against the triangles in the list.
	Vector rayDir = ray.m_Delta;
	VectorNormalize( rayDir );

	for ( int iTri = 0; iTri < nTreeTriCount; ++iTri )
	{
		CDispCollTri *pTri = aTreeTris[iTri];
		SweepAABBTriIntersect( ray, rayDir, iTri, pTri, pTrace, false );
	}

	// Collision.
	if ( pTrace->fraction < flMinFrac )
		return true;

	// No collision.
	return false;
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CDispCollTree::ResolveRayPlaneIntersect( float flStart, float flEnd, const Vector &vecNormal, float flDist )
{
	if( ( flStart > 0.0f ) && ( flEnd > 0.0f ) ) 
		return false; 

	if( ( flStart < 0.0f ) && ( flEnd < 0.0f ) ) 
		return true; 

	float flDenom = flStart - flEnd;
	bool bDenomIsZero = ( flDenom == 0.0f );
	if( ( flStart >= 0.0f ) && ( flEnd <= 0.0f ) )
	{
		// Find t - the parametric distance along the trace line.
		float t = ( !bDenomIsZero ) ? ( flStart - DISPCOLL_DIST_EPSILON ) / flDenom : 0.0f;
		if( t > m_Helper.m_flStartFrac )
		{
			m_Helper.m_flStartFrac = t;
			VectorCopy( vecNormal, m_Helper.m_vecImpactNormal );
			m_Helper.m_flImpactDist = flDist;
		}
	}
	else
	{
		// Find t - the parametric distance along the trace line.
		float t = ( !bDenomIsZero ) ? ( flStart + DISPCOLL_DIST_EPSILON ) / flDenom : 0.0f;
		if( t < m_Helper.m_flEndFrac )
		{
			m_Helper.m_flEndFrac = t;
		}	
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CDispCollTree::FacePlane( const Ray_t &ray, const Vector &rayDir, CDispCollTri *pTri )
{
	// Calculate the closest point on box to plane (get extents in that direction).
	Vector vecExtent;
	CalcClosestExtents( pTri->m_vecNormal, ray.m_Extents, vecExtent );

	float flExpandDist = pTri->m_flDist - pTri->m_vecNormal.Dot( vecExtent );

	float flStart = pTri->m_vecNormal.Dot( ray.m_Start ) - flExpandDist;
	float flEnd = pTri->m_vecNormal.Dot( ( ray.m_Start + ray.m_Delta ) ) - flExpandDist;

	return ResolveRayPlaneIntersect( flStart, flEnd, pTri->m_vecNormal, pTri->m_flDist );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FORCEINLINE CDispCollTree::AxisPlanesXYZ( const Ray_t &ray, CDispCollTri *pTri )
{
	static const TableVector g_ImpactNormalVecs[2][3] = 
	{
		{
			{ -1, 0, 0 },
			{ 0, -1, 0 },
			{ 0, 0, -1 },
		},

		{
			{ 1, 0, 0 },
			{ 0, 1, 0 },
			{ 0, 0, 1 },
		}
	};

	Vector vecImpactNormal;
	float flDist, flExpDist, flStart, flEnd;
	
	int iAxis;
	for ( iAxis = 2; iAxis >= 0; --iAxis )
	{
		const float rayStart = ray.m_Start[iAxis];
		const float rayExtent = ray.m_Extents[iAxis];
		const float rayDelta = ray.m_Delta[iAxis];

		// Min
		flDist = m_aVerts[pTri->GetVert(pTri->GetMin(iAxis))][iAxis];
		flExpDist = flDist - rayExtent;
		flStart = flExpDist - rayStart;
		flEnd = flStart - rayDelta;

		if ( !ResolveRayPlaneIntersect( flStart, flEnd, g_ImpactNormalVecs[0][iAxis], flDist ) )
			return false;

		// Max
		flDist = m_aVerts[pTri->GetVert(pTri->GetMax(iAxis))][iAxis];
		flExpDist = flDist + rayExtent;
		flStart = rayStart - flExpDist;
		flEnd = flStart + rayDelta;

		if ( !ResolveRayPlaneIntersect( flStart, flEnd, g_ImpactNormalVecs[1][iAxis], flDist ) )
			return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Testing!
//-----------------------------------------------------------------------------
void CDispCollTree::Cache_Create( CDispCollTri *pTri, int iTri )
{
	MEM_ALLOC_CREDIT();
	Vector *pVerts[3];
	pVerts[0] = &m_aVerts[pTri->GetVert( 0 )];
	pVerts[1] = &m_aVerts[pTri->GetVert( 1 )];
	pVerts[2] = &m_aVerts[pTri->GetVert( 2 )];

	CDispCollTriCache *pCache = &m_aTrisCache[iTri];

	Vector vecEdge;

	// Edge 1
	VectorSubtract( *pVerts[1], *pVerts[0], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossX[0] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossY[0] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[0], *pVerts[2], pTri, pCache->m_iCrossZ[0] );

	// Edge 2
	VectorSubtract( *pVerts[2], *pVerts[1], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossX[1] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossY[1] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[1], *pVerts[0], pTri, pCache->m_iCrossZ[1] );

	// Edge 3
	VectorSubtract( *pVerts[0], *pVerts[2], vecEdge );
	Cache_EdgeCrossAxisX( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossX[2] );
	Cache_EdgeCrossAxisY( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossY[2] );
	Cache_EdgeCrossAxisZ( vecEdge, *pVerts[2], *pVerts[1], pTri, pCache->m_iCrossZ[2] );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CDispCollTree::AddPlane( const Vector &vecNormal )
{
	UtlHashHandle_t handle;
	DispCollPlaneIndex_t planeIndex;
	bool bDidInsert;

	planeIndex.vecPlane = vecNormal;
	planeIndex.index = m_aEdgePlanes.Count();

	handle = g_DispCollPlaneIndexHash.Insert( planeIndex, &bDidInsert );

	if ( !bDidInsert )
	{
		DispCollPlaneIndex_t &existingEntry = g_DispCollPlaneIndexHash[handle];
		if ( existingEntry.vecPlane == vecNormal )
		{
			return existingEntry.index;
		}
		else
		{
			return ( existingEntry.index | 0x8000 );
		}
	}

	return m_aEdgePlanes.AddToTail( vecNormal );
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: The plane distance get stored in the normal x position since it isn't
//       used.
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisX( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisX = ( 0.0, edgeZ, -edgeY )
	Vector vecNormal( 0.0f, vecEdge.z, -vecEdge.y );
	VectorNormalize( vecNormal );

	// Check for zero length normals.
	if( ( vecNormal.y == 0.0f ) || ( vecNormal.z == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.y * vecOnEdge.y ) + ( vecNormal.z * vecOnEdge.z );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.y * vecOffEdge.y ) + ( vecNormal.z * vecOffEdge.z );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing - triangle should be behind the plane.
		vecNormal.x = -flDist;
		vecNormal.y = -vecNormal.y;
		vecNormal.z = -vecNormal.z;
	}
	else
	{
		vecNormal.x = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// NOTE: The plane distance get stored in the normal y position since it isn't
//       used.
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisY( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisY = ( -edgeZ, 0.0, edgeX )
	Vector vecNormal( -vecEdge.z, 0.0f, vecEdge.x );
	VectorNormalize( vecNormal );

	// Check for zero length normals
	if( ( vecNormal.x == 0.0f ) || ( vecNormal.z == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.x * vecOnEdge.x ) + ( vecNormal.z * vecOnEdge.z );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.x * vecOffEdge.x ) + ( vecNormal.z * vecOffEdge.z );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing if necessay - triangle should be behind the plane.
		vecNormal.x = -vecNormal.x;
		vecNormal.y = -flDist;
		vecNormal.z = -vecNormal.z;
	}
	else
	{
		vecNormal.y = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Cache_EdgeCrossAxisZ( const Vector &vecEdge, const Vector &vecOnEdge,
										  const Vector &vecOffEdge, CDispCollTri *pTri,
										  unsigned short &iPlane )
{
	// Calculate the normal - edge x axisY = ( edgeY, -edgeX, 0.0 )
	Vector vecNormal( vecEdge.y, -vecEdge.x, 0.0f );
	VectorNormalize( vecNormal );

	// Check for zero length normals
	if( ( vecNormal.x == 0.0f ) || ( vecNormal.y == 0.0f ) )
	{
		iPlane = DISPCOLL_NORMAL_UNDEF;
		return false;
	}

//	if ( pTri->m_vecNormal.Dot( vecNormal ) )
//	{
//		iPlane = DISPCOLL_NORMAL_UNDEF;
//		return false;
//	}

	// Finish the plane definition - get distance.
	float flDist = ( vecNormal.x * vecOnEdge.x ) + ( vecNormal.y * vecOnEdge.y );

	// Special case the point off edge in plane
	float flOffDist = ( vecNormal.x * vecOffEdge.x ) + ( vecNormal.y * vecOffEdge.y );
	if ( !( FloatMakePositive( flOffDist - flDist ) < DISPCOLL_DIST_EPSILON ) && ( flOffDist > flDist ) )
	{
		// Adjust plane facing if necessay - triangle should be behind the plane.
		vecNormal.x = -vecNormal.x;
		vecNormal.y = -vecNormal.y;
		vecNormal.z = -flDist;
	}
	else
	{
		vecNormal.z = flDist;
	}

	// Add edge plane to edge plane list.
	iPlane = static_cast<unsigned short>( AddPlane( vecNormal ) );

	// Created the cached edge.
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template <int AXIS>
bool CDispCollTree::EdgeCrossAxis( const Ray_t &ray, unsigned short iPlane )
{
	if ( iPlane == DISPCOLL_NORMAL_UNDEF )
		return true;

	// Get the edge plane.
	Vector vecNormal;
	if ( ( iPlane & 0x8000 ) != 0 )
	{
		VectorCopy( m_aEdgePlanes[(iPlane&0x7fff)], vecNormal );
		vecNormal.Negate();
	}
	else
	{
		VectorCopy( m_aEdgePlanes[iPlane], vecNormal );
	}

	const int OTHER_AXIS1 = ( AXIS + 1 ) % 3;
	const int OTHER_AXIS2 = ( AXIS + 2 ) % 3;

	// Get the pland distance are "fix" the normal.
	float flDist = vecNormal[AXIS];
	vecNormal[AXIS] = 0.0f;

	// Calculate the closest point on box to plane (get extents in that direction).
	Vector vecExtent;
	//vecExtent[AXIS] = 0.0f;
	vecExtent[OTHER_AXIS1] = ( vecNormal[OTHER_AXIS1] < 0.0f ) ? ray.m_Extents[OTHER_AXIS1] : -ray.m_Extents[OTHER_AXIS1];
	vecExtent[OTHER_AXIS2] = ( vecNormal[OTHER_AXIS2] < 0.0f ) ? ray.m_Extents[OTHER_AXIS2] : -ray.m_Extents[OTHER_AXIS2];

	// Expand the plane by the extents of the box to reduce the swept box/triangle
	// test to a ray/extruded triangle test (one of the triangles extruded planes
	// was just calculated above).
	Vector vecEnd;
	vecEnd[AXIS] = 0;
	vecEnd[OTHER_AXIS1] = ray.m_Start[OTHER_AXIS1] + ray.m_Delta[OTHER_AXIS1];
	vecEnd[OTHER_AXIS2] = ray.m_Start[OTHER_AXIS2] + ray.m_Delta[OTHER_AXIS2];

	float flExpandDist 	= flDist - ( ( vecNormal[OTHER_AXIS1] * vecExtent[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * vecExtent[OTHER_AXIS2] ) );
	float flStart 		= ( vecNormal[OTHER_AXIS1] * ray.m_Start[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * ray.m_Start[OTHER_AXIS2] ) - flExpandDist;
	float flEnd 		= ( vecNormal[OTHER_AXIS1] * vecEnd[OTHER_AXIS1] ) + ( vecNormal[OTHER_AXIS2] * vecEnd[OTHER_AXIS2] ) - flExpandDist;

	return ResolveRayPlaneIntersect( flStart, flEnd, vecNormal, flDist );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisX( const Ray_t &ray, unsigned short iPlane )
{
	return EdgeCrossAxis<0>( ray, iPlane );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisY( const Ray_t &ray, unsigned short iPlane )
{
	return EdgeCrossAxis<1>( ray, iPlane );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CDispCollTree::EdgeCrossAxisZ( const Ray_t &ray, unsigned short iPlane )
{
	return EdgeCrossAxis<2>( ray, iPlane );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDispCollTree::SweepAABBTriIntersect( const Ray_t &ray, const Vector &rayDir, int iTri, CDispCollTri *pTri, CBaseTrace *pTrace, bool bTestOutside )
{
#if 0
	// Do we test to make sure we aren't in solid?
	if( bTestOutside )
	{
	}
#endif

	// Init test data.
	m_Helper.m_flEndFrac = 1.0f;
	m_Helper.m_flStartFrac = DISPCOLL_INVALID_FRAC;

	// Make sure objects are traveling toward one another.
	float flAngle = pTri->m_vecNormal.Dot( rayDir );
	if( flAngle > DISPCOLL_DIST_EPSILON )
		return;

	// Test against the axis planes.
	if ( !AxisPlanesXYZ( ray, pTri ) )
	{
		return;
	}

	//
	// There are 9 edge tests - edges 1, 2, 3 cross with the box edges (symmetry) 1, 2, 3.  However, the box
	// is axis-aligned resulting in axially directional edges -- thus each test is edges 1, 2, and 3 vs. 
	// axial planes x, y, and z
	//
	// There are potentially 9 more tests with edges, the edge's edges and the direction of motion!
	// NOTE: I don't think these tests are necessary for a manifold surface.
	//

	CheckCache();

	CDispCollTriCache *pCache = &m_aTrisCache[iTri];

	// Edges 1-3, interleaved - axis tests are 2d tests
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[0] ) ) { return; }
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[1] ) ) { return; }
	if ( !EdgeCrossAxisX( ray, pCache->m_iCrossX[2] ) ) { return; }

	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[0] ) ) { return; }
	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[1] ) ) { return; }
	if ( !EdgeCrossAxisY( ray, pCache->m_iCrossY[2] ) ) { return; }

	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[0] ) ) { return; }
	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[1] ) ) { return; }
	if ( !EdgeCrossAxisZ( ray, pCache->m_iCrossZ[2] ) ) { return; }

	// Test against the triangle face plane.
	if ( !FacePlane( ray, rayDir, pTri ) )
		return;

	if ( ( m_Helper.m_flStartFrac < m_Helper.m_flEndFrac ) || ( FloatMakePositive( m_Helper.m_flStartFrac - m_Helper.m_flEndFrac ) < 0.001f ) )
	{
		if ( ( m_Helper.m_flStartFrac != DISPCOLL_INVALID_FRAC ) && ( m_Helper.m_flStartFrac < pTrace->fraction ) )
		{
			// Clamp -- shouldn't really ever be here!???
			if ( m_Helper.m_flStartFrac < 0.0f )
			{
				m_Helper.m_flStartFrac = 0.0f;
			}
			
			pTrace->fraction = m_Helper.m_flStartFrac;
			VectorCopy( m_Helper.m_vecImpactNormal, pTrace->plane.normal );
			pTrace->plane.dist = m_Helper.m_flImpactDist;
			pTrace->dispFlags = pTri->m_uiFlags;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CDispCollTree::CDispCollTree()
{
	m_nPower = 0;
	m_nFlags = 0;
	m_pLeafLinkHead = NULL;

	for ( int iPoint = 0; iPoint < 4; ++iPoint )
	{
		m_vecSurfPoints[iPoint].Init();
	}
	m_nContents = -1;
	m_nSurfaceProps[0] = 0;
	m_nSurfaceProps[1] = 0;

	m_vecStabDir.Init();
	m_vecBounds[0].Init( FLT_MAX, FLT_MAX, FLT_MAX );
	m_vecBounds[1].Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	for ( int iCount = 0; iCount < MAX_CHECK_COUNT_DEPTH; ++iCount )
	{
		m_nCheckCount[iCount] = 0;
	}

	m_aVerts.Purge();
	m_aTris.Purge();
	m_aNodes.Purge();
	m_aEdgePlanes.Purge();
#ifdef ENGINE_DLL
	m_hCache = INVALID_MEMHANDLE;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: deconstructor
//-----------------------------------------------------------------------------
CDispCollTree::~CDispCollTree()
{	
#ifdef ENGINE_DLL
	if ( m_hCache != INVALID_MEMHANDLE )
		g_DispCollTriCache.DestroyResource( m_hCache );
#endif
	m_aVerts.Purge();
	m_aTris.Purge();
	m_aNodes.Purge();
	m_aEdgePlanes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::Create( CCoreDispInfo *pDisp )
{
	// Create the AABB Tree.
	return AABBTree_Create( pDisp );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CDispCollTree::PointInBounds( const Vector &vecBoxCenter, const Vector &vecBoxMin, 
								   const Vector &vecBoxMax, bool bPoint )
{
	// Point test inside bounds.
	if( bPoint )
	{
		return IsPointInBox( vecBoxCenter, m_vecBounds[0], m_vecBounds[1] );
	}
	
	// Box test inside bounds
	Vector vecExtents;
	VectorSubtract( vecBoxMax, vecBoxMin, vecExtents );
	vecExtents *= 0.5f;

	Vector vecExpandBounds[2];
	vecExpandBounds[0] = m_vecBounds[0] - vecExtents;
	vecExpandBounds[1] = m_vecBounds[1] + vecExtents;

	return IsPointInBox( vecBoxCenter, vecExpandBounds[0], vecExpandBounds[1] );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDispCollTree::ApplyTerrainMod( ITerrainMod *pMod )
{
#if 0
	int nVertCount = GetSize();
	for ( int iVert = 0; iVert < nVertCount; ++iVert )
	{
		pMod->ApplyMod( m_aVerts[iVert].m_vecPos, m_aVerts[iVert].m_vecOrigPos );
		pMod->ApplyMod( m_aVerts[iVert].m_vecPos, m_aVerts[iVert].m_vecOrigPos );
	}

	// Setup/create the leaf nodes first so the recusion can use this data to stop.
	AABBTree_CreateLeafs();

	// Generate bounding boxes.
	AABBTree_GenerateBoxes();

	// Create the bounding box of the displacement surface + the base face.
	AABBTree_CalcBounds();
#endif
}

void CDispCollTree::GetVirtualMeshList( virtualmeshlist_t *pList )
{
	int i;
	int triangleCount = GetTriSize();
	pList->indexCount = triangleCount * 3;
	pList->triangleCount = triangleCount;
	pList->vertexCount = m_aVerts.Count();
	pList->pVerts = m_aVerts.Base();
	int index = 0;
	for ( i = 0 ; i < triangleCount; i++ )
	{
		pList->indices[index+0] = m_aTris[i].GetVert(0);
		pList->indices[index+1] = m_aTris[i].GetVert(1);
		pList->indices[index+2] = m_aTris[i].GetVert(2);
		index += 3;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CDispCollTree *DispCollTrees_Alloc( int count )
{
	CDispCollTree *pTrees = NULL;
	pTrees = new CDispCollTree[count];
	if( !pTrees )
		return NULL;

	return pTrees;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DispCollTrees_Free( CDispCollTree *pTrees )
{
	if( pTrees )
	{
		delete [] pTrees;
		pTrees = NULL;
	}
}
