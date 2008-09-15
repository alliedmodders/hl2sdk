//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifdef _WIN32
#include <windows.h>
#endif
#include "vrad.h"
#include "UtlVector.h"
#include "cmodel.h"
#include "BSPTreeData.h"
#include "VRAD_DispColl.h"
#include "CollisionUtils.h"
#include "lightmap.h"
#include "Radial.h"
#include "CollisionUtils.h"
#include <bumpvects.h>
#include "UtlRBTree.h"
#include "tier0/fasttimer.h"
#include "disp_vrad.h"

class CBSPDispRayDistanceEnumerator;

//=============================================================================
//
// Displacement/Face List
//
class CBSPDispFaceListEnumerator : public ISpatialLeafEnumerator, public IBSPTreeDataEnumerator
{
public:

	//=========================================================================
	//
	// Construction/Deconstruction
	//
	CBSPDispFaceListEnumerator() {};
	virtual ~CBSPDispFaceListEnumerator()
	{
		m_DispList.Purge();
		m_FaceList.Purge();
	}

	// ISpatialLeafEnumerator
	bool EnumerateLeaf( int ndxLeaf, int context ); 

	// IBSPTreeDataEnumerator
	bool FASTCALL EnumerateElement( int userId, int context );

public:

	CUtlVector<CVRADDispColl*>	m_DispList;
	CUtlVector<int>				m_FaceList;
};


//=============================================================================
//
// RayEnumerator
//
class CBSPDispRayEnumerator : public ISpatialLeafEnumerator, public IBSPTreeDataEnumerator
{
public:
	// ISpatialLeafEnumerator
	bool EnumerateLeaf( int ndxLeaf, int context );

	// IBSPTreeDataEnumerator
	bool FASTCALL EnumerateElement( int userId, int context );
};

//=============================================================================
//
// VRad Displacement Manager
//
class CVRadDispMgr : public IVRadDispMgr
{
public:

	//=========================================================================
	//
	// Construction/Deconstruction
	//
	CVRadDispMgr();
	virtual ~CVRadDispMgr();

	// creation/destruction
	void Init( void );
	void Shutdown( void );

	// "CalcPoints"
	bool BuildDispSamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace );
	bool BuildDispLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace );
	bool BuildDispSamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace );

	// patching functions
	void MakePatches( void );
	void SubdividePatch( int ndxPatch );

	// pre "FinalLightFace"
	void InsertSamplesDataIntoHashTable( void );
	void InsertPatchSampleDataIntoHashTable( void );

	// "FinalLightFace"
	radial_t *BuildLuxelRadial( int ndxFace, int ndxStyle, bool bBump );
	bool SampleRadial( int ndxFace, radial_t *pRadial, Vector const &vPos, int ndxLxl, Vector *pLightSample, int sampleCount, bool bPatch );
	radial_t *BuildPatchRadial( int ndxFace, bool bBump );

	// utility
	void GetDispSurfNormal( int ndxFace, Vector &pt, Vector &ptNormal, bool bInside );
	void GetDispSurf( int ndxFace, CVRADDispColl **ppDispTree );

	// bsp tree functions
	bool ClipRayToDisp( DispTested_t &dispTested, Ray_t const &ray );
	bool ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, int ndxLeaf );
	void ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, int ndxLeaf,  
					float& dist, dface_t*& pFace, Vector2D& luxelCoord );
	void StartRayTest( DispTested_t &dispTested );

	// general timing -- should be moved!!
	void StartTimer( const char *name );
	void EndTimer( void );

	//=========================================================================
	//
	// Enumeration Methods
	//
	bool DispRay_EnumerateLeaf( int ndxLeaf, int context );
	bool DispRay_EnumerateElement( int userId, int context );
	bool DispRayDistance_EnumerateElement( int userId, CBSPDispRayDistanceEnumerator* pEnum );

	bool DispFaceList_EnumerateLeaf( int ndxLeaf, int context );
	bool DispFaceList_EnumerateElement( int userId, int context );

private:

	//=========================================================================
	//
	// BSP Tree Helpers
	//
	void InsertDispIntoTree( int ndxDisp );
	void RemoveDispFromTree( int ndxDisp );

	//=========================================================================
	//
	// Displacement Data Loader (from .bsp)
	//
	void UnserializeDisps( void );
	void DispBuilderInit( CCoreDispInfo *pBuilderDisp, dface_t *pFace, int ndxFace );

	//=========================================================================
	//
	//  Sampling Helpers
	//
	void RadialLuxelBuild( CVRADDispColl *pDispTree, radial_t *pRadial, int ndxStyle, bool bBump );
	void RadialLuxelAddSamples( int ndxFace, Vector const &luxelPt, Vector const &luxelNormal, float radius,
                                radial_t *pRadial, int ndxRadial, bool bBump, int lightStyle );

	void RadialPatchBuild( CVRADDispColl *pDispTree, radial_t *pRadial, bool bBump );
	void RadialLuxelAddPatch( int ndxFace, Vector const &luxelPt, 
											Vector const &luxelNormal,  float radius, 
											radial_t *pRadial, int ndxRadial, bool bBump,
											CUtlVector<patch_t*> &interestingPatches );

	bool IsNeighbor( int iDispFace, int iNeighborFace );

	void GetInterestingPatchesForLuxels( 
		int ndxFace,
		CUtlVector<patch_t*> &interestingPatches,
		float patchSampleRadius );

private:

	struct DispCollTree_t
	{
		CVRADDispColl			*m_pDispTree;
		BSPTreeDataHandle_t		m_Handle;
	};

	struct EnumContext_t
	{
		DispTested_t	*m_pDispTested;
		Ray_t const		*m_pRay;
	};

	CUtlVector<DispCollTree_t>	m_DispTrees;

	IBSPTreeData				*m_pBSPTreeData;

	CBSPDispRayEnumerator		m_EnumDispRay;
	CBSPDispFaceListEnumerator	m_EnumDispFaceList;

	int							sampleCount;
	Vector						*m_pSamplePos;

	CFastTimer					m_Timer;
};

//-----------------------------------------------------------------------------
// Purpose: expose IVRadDispMgr to vrad
//-----------------------------------------------------------------------------

static CVRadDispMgr	s_DispMgr;

IVRadDispMgr *StaticDispMgr( void )
{
	return &s_DispMgr;
}


//=============================================================================
//
// Displacement/Face List
//
// ISpatialLeafEnumerator
bool CBSPDispFaceListEnumerator::EnumerateLeaf( int ndxLeaf, int context ) 
{ 
	return s_DispMgr.DispFaceList_EnumerateLeaf( ndxLeaf, context );
}

// IBSPTreeDataEnumerator
bool FASTCALL CBSPDispFaceListEnumerator::EnumerateElement( int userId, int context )
{
	return s_DispMgr.DispFaceList_EnumerateElement( userId, context );
}


//=============================================================================
//
// RayEnumerator
//
bool CBSPDispRayEnumerator::EnumerateLeaf( int ndxLeaf, int context )
{
	return s_DispMgr.DispRay_EnumerateLeaf( ndxLeaf, context );
}

bool FASTCALL CBSPDispRayEnumerator::EnumerateElement( int userId, int context )
{
	return s_DispMgr.DispRay_EnumerateElement( userId, context );
}


//-----------------------------------------------------------------------------
// Here's an enumerator that we use for testing against disps in a leaf...
//-----------------------------------------------------------------------------

class CBSPDispRayDistanceEnumerator : public IBSPTreeDataEnumerator
{
public:
	CBSPDispRayDistanceEnumerator() : m_Distance(1.0f), m_pSurface(0) {}

	// IBSPTreeDataEnumerator
	bool FASTCALL EnumerateElement( int userId, int context )
	{
		return s_DispMgr.DispRayDistance_EnumerateElement( userId, this );
	}

	float m_Distance;
	dface_t* m_pSurface;
	DispTested_t	*m_pDispTested;
	Ray_t const		*m_pRay;
	Vector2D		m_LuxelCoord;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRadDispMgr::CVRadDispMgr()
{
	m_pBSPTreeData = CreateBSPTreeData();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRadDispMgr::~CVRadDispMgr()
{
	DestroyBSPTreeData( m_pBSPTreeData );
}


//-----------------------------------------------------------------------------
// Insert a displacement into the tree for collision
//-----------------------------------------------------------------------------
void CVRadDispMgr::InsertDispIntoTree( int ndxDisp )
{
	DispCollTree_t &dispTree = m_DispTrees[ndxDisp];
	CDispCollTree *pDispTree = dispTree.m_pDispTree;

	// get the bounding box of the tree
	Vector boxMin, boxMax;
	pDispTree->GetBounds( boxMin, boxMax );

	// add the displacement to the tree so we will collide against it
	dispTree.m_Handle = m_pBSPTreeData->Insert( ndxDisp, boxMin, boxMax );
}

	
//-----------------------------------------------------------------------------
// Remove a displacement from the tree for collision
//-----------------------------------------------------------------------------
void CVRadDispMgr::RemoveDispFromTree( int ndxDisp )
{
	// release the tree handle
	if( m_DispTrees[ndxDisp].m_Handle != TREEDATA_INVALID_HANDLE )
	{
		m_pBSPTreeData->Remove( m_DispTrees[ndxDisp].m_Handle );
		m_DispTrees[ndxDisp].m_Handle = TREEDATA_INVALID_HANDLE;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::Init( void )
{
	// initialize the bsp tree
	m_pBSPTreeData->Init( ToolBSPTree() );

	// read in displacements that have been compiled into the bsp file
	UnserializeDisps();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::Shutdown( void )
{
	// remove all displacements from the tree
	for( int ndxDisp = m_DispTrees.Size(); ndxDisp >= 0; ndxDisp-- )
	{
		RemoveDispFromTree( ndxDisp );
	}

	// shutdown the bsp tree
	m_pBSPTreeData->Shutdown();

	// purge the displacement collision tree list
	m_DispTrees.Purge();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::DispBuilderInit( CCoreDispInfo *pBuilderDisp, dface_t *pFace, int ndxFace )
{
	// get the .bsp displacement
	ddispinfo_t *pDisp = &g_dispinfo[pFace->dispinfo];
	if( !pDisp )
		return;

	//
	// initlialize the displacement base surface
	//
	CCoreDispSurface *pSurf = pBuilderDisp->GetSurface();
	pSurf->SetPointCount( 4 );
	pSurf->SetHandle( ndxFace );
	pSurf->SetContents( pDisp->contents ); 

	Vector pt[4];
	int ndxPt;
	for( ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		int eIndex = dsurfedges[pFace->firstedge+ndxPt];
		if( eIndex < 0 )
		{
			pSurf->SetPoint( ndxPt, dvertexes[dedges[-eIndex].v[1]].point );
		}
		else
		{
			pSurf->SetPoint( ndxPt, dvertexes[dedges[eIndex].v[0]].point );
		}

		VectorCopy( pSurf->GetPoint(ndxPt), pt[ndxPt] );
	}


	Vector2D lmCoords[4];
	CalcTextureCoordsAtPoints( 
		texinfo[pFace->texinfo].lightmapVecsLuxelsPerWorldUnits,
		pFace->m_LightmapTextureMinsInLuxels,
		pt,
		NUM_BUMP_VECTS + 1,
		lmCoords );

	for( ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		for( int j = 0; j < ( NUM_BUMP_VECTS + 1 ); ++j )
		{
			pSurf->SetLuxelCoord( j, ndxPt, lmCoords[ndxPt]	);
		}
	}

	//
	// calculate the displacement surface normal
	//
	Vector vFaceNormal;
	pSurf->GetNormal( vFaceNormal );
	for( ndxPt = 0; ndxPt < 4; ndxPt++ )
	{
		pSurf->SetPointNormal( ndxPt, vFaceNormal );
	}

	// set the surface initial point info
	pSurf->SetPointStart( pDisp->startPosition );
	pSurf->FindSurfPointStartIndex();
	pSurf->AdjustSurfPointData();

	pBuilderDisp->SetNeighborData( pDisp->m_EdgeNeighbors, pDisp->m_CornerNeighbors );
	
	CDispVert *pVerts = &g_DispVerts[ pDisp->m_iDispVertStart ];
	CDispTri *pTris = &g_DispTris[pDisp->m_iDispTriStart];

	//
	// initialize the displacement data
	//
	pBuilderDisp->InitDispInfo( 
		pDisp->power, 
		pDisp->minTess, 
		pDisp->smoothingAngle,
		pVerts,
		pTris );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::UnserializeDisps( void ) 
{
	// temporarily create the "builder" displacements
	CUtlVector<CCoreDispInfo*> builderDisps;
	for ( int iDisp = 0; iDisp < g_dispinfo.Count(); ++iDisp )
	{
		CCoreDispInfo *pDisp = new CCoreDispInfo;
		if ( !pDisp )
		{
			builderDisps.Purge();
			return;
		}

		int nIndex = builderDisps.AddToTail();
		pDisp->SetListIndex( nIndex );
		builderDisps[nIndex] = pDisp;
	}

	// Set them up as CDispUtilsHelpers.
	for ( int iDisp = 0; iDisp < g_dispinfo.Count(); ++iDisp )
	{
		builderDisps[iDisp]->SetDispUtilsHelperInfo( builderDisps.Base(), g_dispinfo.Count() );
	}

	//
	// find all faces with displacement data and initialize
	//
	for( int ndxFace = 0; ndxFace < numfaces; ndxFace++ )
	{
		dface_t *pFace = &g_pFaces[ndxFace];
		if( ValidDispFace( pFace ) )
		{
			DispBuilderInit( builderDisps[pFace->dispinfo], pFace, ndxFace );
		}
	}

	// generate the displacement surfaces
	for( int iDisp = 0; iDisp < g_dispinfo.Count(); ++iDisp )
	{
		builderDisps[iDisp]->Create();
	}

	// smooth edge normals
	SmoothNeighboringDispSurfNormals( builderDisps.Base(), g_dispinfo.Count() );

	//
	// create the displacement collision tree and add it to the bsp tree
	//
	CVRADDispColl *pDispTrees = new CVRADDispColl[g_dispinfo.Count()];
	if( !pDispTrees )
		return;

	m_DispTrees.AddMultipleToTail( g_dispinfo.Count() );

	for( int iDisp = 0; iDisp < g_dispinfo.Count(); iDisp++ )
	{
		pDispTrees[iDisp].Create( builderDisps[iDisp] );

		m_DispTrees[iDisp].m_pDispTree = &pDispTrees[iDisp];
		m_DispTrees[iDisp].m_Handle = TREEDATA_INVALID_HANDLE;

		InsertDispIntoTree( iDisp );
	}

	// free "builder" disps
	builderDisps.Purge();
}


//-----------------------------------------------------------------------------
// Purpose: create a set of patches for each displacement surface to transfer
//          bounced light around with
//-----------------------------------------------------------------------------
void CVRadDispMgr::MakePatches( void )
{
	//
	// for each displacement surface create a set of patches to transfer light
	//
	float totalArea = 0.0f;

	int treeCount = m_DispTrees.Size();
	for( int ndxTree = 0; ndxTree < treeCount; ndxTree++ )
	{
		// get the current displacement tree
		CVRADDispColl *pTree = m_DispTrees[ndxTree].m_pDispTree;
		if( !pTree )
			continue;

		// allocate a patch
		int ndxPatch = patches.AddToTail();
		patch_t *pPatch = &patches[ndxPatch];

		// create the "parent" patch
		pTree->InitPatch( ndxPatch, patches.InvalidIndex(), false );
		if( !pTree->MakePatch( ndxPatch ) )
		{
			Msg( "Removed patch %d\n", ndxPatch );
			patches.Remove( ndxPatch );
		}

		// accumulate total area for statistics
		totalArea += pPatch->area;
	}

	// print stats
	qprintf( "%i displacements\n", treeCount );
	qprintf( "%i square feet [%.2f square inches]\n", ( int )( totalArea / 144 ), totalArea );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::SubdividePatch( int ndxPatch )
{
#define MIN_PATCH_AREA	32*32		// 64*64 with a little slop!

	// get the current patch
	patch_t *pPatch = &patches.Element( ndxPatch );
	if( !pPatch )
		return;

	//
	// check area -- should I subdivide
	//
	if( pPatch->area < MIN_PATCH_AREA )
		return;

	// get the displacement tree;
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[pPatch->faceNumber].dispinfo];
    CVRADDispColl *pTree = dispTree.m_pDispTree;	
	if( !pTree )
		return;

	//
	// subdivide along the widest bounding axis
	//
	Vector patchExtents;
	VectorSubtract( pPatch->maxs, pPatch->mins, patchExtents );

	int ndxAxis;
	float widestExtents = -1.0f;
	int   widestAxis = -1;
	for( ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( ndxAxis == pPatch->normalMajorAxis )
			continue;

		if( patchExtents[ndxAxis] > widestExtents )
		{
			widestAxis = ndxAxis;
			widestExtents = patchExtents[ndxAxis];
		}
	}

	//
	// create patch children
	//
	int ndxChild1Patch = patches.AddToTail();
	pTree->InitPatch( ndxChild1Patch, ndxPatch, true );
	patch_t *pChild1 = &patches[ndxChild1Patch];
	pPatch = &patches[ndxPatch];

	//
	// create the split bounding volume for child1
	//
	for( ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		pChild1->mins[ndxAxis] = pPatch->mins[ndxAxis];
		if( ndxAxis != widestAxis )
		{
			pChild1->maxs[ndxAxis] = pPatch->maxs[ndxAxis];
		}
		else
		{
			pChild1->maxs[ndxAxis] = ( pPatch->maxs[ndxAxis] + pPatch->mins[ndxAxis] ) * 0.5f;
		}
	}

	if( !pTree->MakePatch( ndxChild1Patch ) )
	{
//		Msg( "Removed child patch %d\n", ndxChild1Patch );
		patches.Remove( ndxChild1Patch );
		ndxChild1Patch = patches.InvalidIndex();
		pPatch->child1 = patches.InvalidIndex();
	}

	int ndxChild2Patch = patches.AddToTail();
	pTree->InitPatch( ndxChild2Patch, ndxPatch, false );
	patch_t *pChild2 = &patches[ndxChild2Patch];
	pPatch = &patches[ndxPatch];

	//
	// create the split bounding volume for child2
	//
	for( ndxAxis = 0; ndxAxis < 3; ndxAxis++ )
	{
		if( ndxAxis != widestAxis )
		{
			pChild2->mins[ndxAxis] = pPatch->mins[ndxAxis];
		}
		else
		{
			pChild2->mins[ndxAxis] = ( pPatch->maxs[ndxAxis] + pPatch->mins[ndxAxis] ) * 0.5f;
		}
			
		pChild2->maxs[ndxAxis] = pPatch->maxs[ndxAxis];
	}

	if( !pTree->MakePatch( ndxChild2Patch ) )
	{
//		Msg( "Removed child patch %d\n", ndxChild2Patch );
		patches.Remove( ndxChild2Patch );
		ndxChild2Patch = patches.InvalidIndex();
		pPatch->child2 = patches.InvalidIndex();

		// also remove child1 patch if it exists (total subdivision or no subdivision)
		if ( ndxChild1Patch != patches.InvalidIndex() )
		{
//			Msg( "Removed child 1 also %d\n", ndxChild1Patch );
			patches.Remove( ndxChild1Patch );
			ndxChild1Patch = patches.InvalidIndex();
			pPatch->child1 = patches.InvalidIndex();
		}
	}

	// continue subdividing
	if( ndxChild1Patch != patches.InvalidIndex() )
	{
		SubdividePatch( ndxChild1Patch );
	}

	if( ndxChild2Patch != patches.InvalidIndex() )
	{
		SubdividePatch( ndxChild2Patch );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::StartRayTest( DispTested_t &dispTested )
{
	if( m_DispTrees.Size() > 0 )
	{
		if( dispTested.m_pTested == 0 )
		{
			dispTested.m_pTested = new int[m_DispTrees.Size()];
			memset( dispTested.m_pTested, 0, m_DispTrees.Size() * sizeof( int ) );
			dispTested.m_Enum = 0;
		}
		++dispTested.m_Enum;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::ClipRayToDisp( DispTested_t &dispTested, Ray_t const &ray )
{
	StartRayTest( dispTested );

	EnumContext_t ctx;
	ctx.m_pRay = &ray;
	ctx.m_pDispTested = &dispTested;

	// If it got through without a hit, it returns true
	return !m_pBSPTreeData->EnumerateLeavesAlongRay( ray, &m_EnumDispRay, ( int )&ctx );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, 
										int ndxLeaf )
{
	EnumContext_t ctx;
	ctx.m_pRay = &ray;
	ctx.m_pDispTested = &dispTested;

	return !m_pBSPTreeData->EnumerateElementsInLeaf( ndxLeaf, &m_EnumDispRay, ( int )&ctx );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, 
			int ndxLeaf, float& dist, dface_t*& pFace, Vector2D& luxelCoord )
{
	CBSPDispRayDistanceEnumerator rayTestEnum;
	rayTestEnum.m_pRay = &ray;
	rayTestEnum.m_pDispTested = &dispTested;

	m_pBSPTreeData->EnumerateElementsInLeaf( ndxLeaf, &rayTestEnum, 0 );

	dist = rayTestEnum.m_Distance;
	pFace = rayTestEnum.m_pSurface;
	if (pFace)
	{
		Vector2DCopy( rayTestEnum.m_LuxelCoord, luxelCoord );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::GetDispSurfNormal( int ndxFace, Vector &pt, Vector &ptNormal,
									  bool bInside )
{
	// get the displacement surface data
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	
	// find the parameterized displacement indices
	Vector2D uv;
	pDispTree->BaseFacePlaneToDispUV( pt, uv );

	if( bInside )
	{
		if( uv[0] < 0.0f || uv[0] > 1.0f ) { Msg( "Disp UV (%f) outside bounds!\n", uv[0] ); }
		if( uv[1] < 0.0f || uv[1] > 1.0f ) { Msg( "Disp UV (%f) outside bounds!\n", uv[1] ); }
	}

	if( uv[0] < 0.0f ) { uv[0] = 0.0f; }
	if( uv[0] > 1.0f ) { uv[0] = 1.0f; }
	if( uv[1] < 0.0f ) { uv[1] = 0.0f; }
	if( uv[1] > 1.0f ) { uv[1] = 1.0f; }

	// get the normal at "pt"
	pDispTree->DispUVToSurfNormal( uv, ptNormal );

	// get the new "pt"
	pDispTree->DispUVToSurfPoint( uv, pt, 1.0f );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::GetDispSurf( int ndxFace, CVRADDispColl **ppDispTree )
{
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
    *ppDispTree = dispTree.m_pDispTree;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::DispRay_EnumerateLeaf( int ndxLeaf, int context )
{
	return m_pBSPTreeData->EnumerateElementsInLeaf( ndxLeaf, &m_EnumDispRay, context );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::DispRay_EnumerateElement( int userId, int context )
{
	DispCollTree_t &dispTree = m_DispTrees[userId];
	EnumContext_t *pCtx = ( EnumContext_t* )context;

	// don't test twice (check tested value)
	if( pCtx->m_pDispTested->m_pTested[userId] == pCtx->m_pDispTested->m_Enum )
		return true;

	// set the tested value
	pCtx->m_pDispTested->m_pTested[userId] = pCtx->m_pDispTested->m_Enum;

	// false mean stop iterating -- return false if we hit! (NOTE: opposite return
	// result of the collision tree's ray test, thus the !)
	CBaseTrace trace;
	trace.fraction = 1.0f;
	return ( !dispTree.m_pDispTree->AABBTree_Ray( *pCtx->m_pRay, &trace, true ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CVRadDispMgr::DispRayDistance_EnumerateElement( int userId, CBSPDispRayDistanceEnumerator* pCtx )
{
	DispCollTree_t &dispTree = m_DispTrees[userId];

	// don't test twice (check tested value)
	if( pCtx->m_pDispTested->m_pTested[userId] == pCtx->m_pDispTested->m_Enum )
		return true;

	// set the tested value
	pCtx->m_pDispTested->m_pTested[userId] = pCtx->m_pDispTested->m_Enum;

	// Test the ray, if it's closer than previous tests, use it!
	RayDispOutput_t output;
	if (dispTree.m_pDispTree->AABBTree_Ray( *pCtx->m_pRay, output ))
	{
		if (output.dist < pCtx->m_Distance)
		{
			pCtx->m_Distance = output.dist;
			pCtx->m_pSurface = &g_pFaces[dispTree.m_pDispTree->GetParentIndex()];

			// Get the luxel coordinate
			ComputePointFromBarycentric( 
				dispTree.m_pDispTree->GetLuxelCoord(output.ndxVerts[0]),
				dispTree.m_pDispTree->GetLuxelCoord(output.ndxVerts[1]),
				dispTree.m_pDispTree->GetLuxelCoord(output.ndxVerts[2]),
				output.u, output.v, pCtx->m_LuxelCoord );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Test a ray against a particular dispinfo
//-----------------------------------------------------------------------------

/*
float CVRadDispMgr::ClipRayToDisp( Ray_t const &ray, int dispinfo )
{
	assert( m_DispTrees.IsValidIndex(dispinfo) );

	RayDispOutput_t output;
	if (!m_DispTrees[dispinfo].m_pDispTree->AABBTree_Ray( ray, output ))
		return 1.0f;
	return output.dist;
}
*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::DispFaceList_EnumerateLeaf( int ndxLeaf, int context )
{
	//
	// add the faces found in this leaf to the face list
	//
	dleaf_t *pLeaf = &dleafs[ndxLeaf];
	for( int ndxFace = 0; ndxFace < pLeaf->numleaffaces; ndxFace++ )
	{
		// get the current face index
		int ndxLeafFace = pLeaf->firstleafface + ndxFace;

		// check to see if the face already lives in the list
		int ndx;
		int size = m_EnumDispFaceList.m_FaceList.Size();
		for( ndx = 0; ndx < size; ndx++ )
		{
			if( m_EnumDispFaceList.m_FaceList[ndx] == ndxLeafFace )
				break;
		}

		if( ndx == size )
		{
			int ndxList = m_EnumDispFaceList.m_FaceList.AddToTail();
			m_EnumDispFaceList.m_FaceList[ndxList] = ndxLeafFace;
		}
	}

	return m_pBSPTreeData->EnumerateElementsInLeaf( ndxLeaf, &m_EnumDispFaceList, context );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::DispFaceList_EnumerateElement( int userId, int context )
{
	DispCollTree_t &dispTree = m_DispTrees[userId];
	CVRADDispColl  *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return false;

	// check to see if the displacement already lives in the list
	int ndx;
	int size = m_EnumDispFaceList.m_DispList.Size();
	for( ndx = 0; ndx < size; ndx++ )
	{
		if( m_EnumDispFaceList.m_DispList[ndx] == pDispTree )
			break;
	}

	if( ndx == size )
	{
		int ndxList = m_EnumDispFaceList.m_DispList.AddToTail();
		m_EnumDispFaceList.m_DispList[ndxList] = pDispTree;
	}

	return true;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void GetSampleLight( facelight_t *pFaceLight, int ndxStyle, bool bBumped, 
			                int ndxSample, Vector *pSampleLight )
{
//	SampleLight[0].Init( 20.0f, 10.0f, 10.0f );
//	return;

	// get sample from bumped lighting data
	if( bBumped )
	{
		for( int ndxBump = 0; ndxBump < ( NUM_BUMP_VECTS+1 ); ndxBump++ )
		{
			pSampleLight[ndxBump] = pFaceLight->light[ndxStyle][ndxBump][ndxSample];
		}
	}
	// just a generally lit surface
	else
	{
		pSampleLight[0] = pFaceLight->light[ndxStyle][0][ndxSample];
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void AddSampleLightToRadial( Vector const &samplePos, Vector const &sampleNormal,
							 Vector *pSampleLight, float sampleRadius2,
							 Vector const &luxelPos, Vector const &luxelNormal,
							 radial_t *pRadial, int ndxRadial, bool bBumped, 
							 bool bNeighborBumped ) 
{
	// check normals to see if sample contributes any light at all
	float angle = sampleNormal.Dot( luxelNormal );
	if ( angle < 0.15f )
		return;

	// calculate the light vector
	Vector vSegment = samplePos - luxelPos;

	// get the distance to the light
	float dist = vSegment.Length();
	float dist2 = dist * dist;

	// check to see if the light is within the sample distance
	if( dist2 > sampleRadius2 )
		return;

	//
	// leaving this garbage here until vrad stabalizes!!
	//
//	float influence = dist2 / ( sampleRadius2 * sampleRadius2 );
	float influence = 1.0f - ( dist2 / ( sampleRadius2 ) );
	influence *= angle;
#if 0
	float influence = 0.0f;
	if( dist2 < 1.0f )
	{
//		influence = 1.0f;
		influence = angle;
	}
	else
	{
//		influence = ( 1.0f / dist2 );
		influence = angle * ( 1.0f / dist2 );
	}
#endif

	if( bBumped )
	{
		if( bNeighborBumped )
		{
			for( int ndxBump = 0; ndxBump < ( NUM_BUMP_VECTS+1 ); ndxBump++ )
			{
				pRadial->light[ndxBump][ndxRadial] += pSampleLight[ndxBump] * influence;
			}
		}
		else
		{
			pRadial->light[0][ndxRadial] += pSampleLight[0] * influence;
			
			for( int ndxBump = 1; ndxBump < ( NUM_BUMP_VECTS+1 ); ndxBump++ )
			{
				pRadial->light[ndxBump][ndxRadial] += pSampleLight[0] * influence * OO_SQRT_3;
			}
		}
	}
	else
	{
		pRadial->light[0][ndxRadial] += pSampleLight[0] * influence;
	}

	// add the weight value
	pRadial->weight[ndxRadial] += influence;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::IsNeighbor( int iFace, int iNeighborFace )
{
	if ( iFace == iNeighborFace )
		return true;

	faceneighbor_t *pFaceNeighbor = &faceneighbor[iFace];
	for ( int iNeighbor = 0; iNeighbor < pFaceNeighbor->numneighbors; iNeighbor++ )
	{
		if ( pFaceNeighbor->neighbor[iNeighbor] == iNeighborFace )
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::RadialLuxelAddSamples( int ndxFace, Vector const &luxelPt, Vector const &luxelNormal, float radius,
									      radial_t *pRadial, int ndxRadial, bool bBump, int lightStyle )
{
	// calculate one over the voxel size
	float ooVoxelSize = 1.0f / SAMPLEHASH_VOXEL_SIZE;

	//
	// find voxel info
	//
	int voxelMin[3], voxelMax[3];
	for( int axis = 0; axis < 3; axis++ )
	{
		voxelMin[axis] = ( int )( ( luxelPt[axis] - radius ) * ooVoxelSize );
		voxelMax[axis] = ( int )( ( luxelPt[axis] + radius ) * ooVoxelSize ) + 1;
	}

	SampleData_t sampleData;	
	for( int ndxZ = voxelMin[2]; ndxZ < voxelMax[2] + 1; ndxZ++ )
	{
		for( int ndxY = voxelMin[1]; ndxY < voxelMax[1] + 1; ndxY++ )
		{
			for( int ndxX = voxelMin[0]; ndxX < voxelMax[0] + 1; ndxX++ )
			{
				sampleData.x = ndxX * 100;
				sampleData.y = ndxY * 10;
				sampleData.z = ndxZ;
				
				UtlHashHandle_t handle = g_SampleHashTable.Find( sampleData );
				if( handle != g_SampleHashTable.InvalidHandle() )
				{
					SampleData_t *pSampleData = &g_SampleHashTable.Element( handle );
					int count = pSampleData->m_Samples.Count();
					for( int ndx = 0; ndx < count; ndx++ )
					{
						SampleHandle_t sampleHandle = pSampleData->m_Samples.Element( ndx );
						int ndxSample = ( sampleHandle & 0x0000ffff );
						int ndxFaceLight = ( ( sampleHandle >> 16 ) & 0x0000ffff );

						facelight_t *pFaceLight = &facelight[ndxFaceLight];
						if( pFaceLight && IsNeighbor( ndxFace, ndxFaceLight ) )
						{
							//
							// check for similar lightstyles
							//
							dface_t	*pFace = &g_pFaces[ndxFaceLight];
							if( pFace )
							{
								int ndxNeighborStyle = -1;
								for( int ndxLightStyle = 0; ndxLightStyle < MAXLIGHTMAPS; ndxLightStyle++ )
								{
									if( pFace->styles[ndxLightStyle] == lightStyle )
									{
										ndxNeighborStyle = ndxLightStyle;
										break;
									}
								}
								if( ndxNeighborStyle == -1 )
									continue;
								
								// is this surface bumped???
								bool bNeighborBump = texinfo[pFace->texinfo].flags & SURF_BUMPLIGHT ? true : false;
								
								Vector sampleLight[NUM_BUMP_VECTS+1];
								GetSampleLight( pFaceLight, ndxNeighborStyle, bNeighborBump, ndxSample, sampleLight );
								AddSampleLightToRadial( pFaceLight->sample[ndxSample].pos, pFaceLight->sample[ndxSample].normal,
									                    sampleLight, radius*radius, luxelPt, luxelNormal, pRadial, ndxRadial,
									                    bBump, bNeighborBump );
							}
						}
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::RadialLuxelBuild( CVRADDispColl *pDispTree, radial_t *pRadial,
								     int ndxStyle, bool bBump )
{
	//
	// get data lighting data
	//
	int ndxFace = pDispTree->GetParentIndex();

	dface_t *pFace = &g_pFaces[ndxFace];
	facelight_t *pFaceLight = &facelight[ndxFace];

	// get the influence radius
	float radius2 = pDispTree->GetSampleRadius2();
	float radius = ( float )sqrt( radius2 );

	int radialSize = pRadial->w * pRadial->h;
	for( int ndxRadial = 0; ndxRadial < radialSize; ndxRadial++ )
	{
		RadialLuxelAddSamples( ndxFace, pFaceLight->luxel[ndxRadial], pFaceLight->luxelNormals[ndxRadial],
						       radius, pRadial, ndxRadial, bBump, pFace->styles[ndxStyle] );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
radial_t *CVRadDispMgr::BuildLuxelRadial( int ndxFace, int ndxStyle, bool bBump )
{
	// allocate the radial
	radial_t *pRadial = AllocateRadial( ndxFace );
	if( !pRadial )
		return NULL;

	//
	// step 1: get the displacement surface to be lit
	//
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return NULL;

	// step 2: build radial luxels
	RadialLuxelBuild( pDispTree, pRadial, ndxStyle, bBump );

	// step 3: return the built radial
	return pRadial;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::SampleRadial( int ndxFace, radial_t *pRadial, Vector const &vPos, int ndxLxl,
								 Vector *pLightSample, int sampleCount, bool bPatch )
{
	bool bGoodSample = true;
	for ( int count = 0; count < sampleCount; count++ )
	{
		pLightSample[count].Init();

		if ( pRadial->weight[ndxLxl] > 0.0f )
		{
			pLightSample[count] = pRadial->light[count][ndxLxl] * ( 1.0f / pRadial->weight[ndxLxl] );
		}
		else
		{
			// error, luxel has no samples (not for patches)
			if ( !bPatch )
			{
				// Yes, 2550 is correct!
				// pLightSample[count].Init( 2550.0f, 0.0f, 2550.0f );
				if( count == 0 )
					bGoodSample = false;
			}
		}
	}

	return bGoodSample;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void GetPatchLight( patch_t *pPatch, bool bBump, Vector *pPatchLight )
{
	VectorCopy( pPatch->totallight.light[0], pPatchLight[0] );

	if( bBump )
	{
		for( int ndxBump = 1; ndxBump < ( NUM_BUMP_VECTS + 1 ); ndxBump++ )
		{
			VectorCopy( pPatch->totallight.light[ndxBump], pPatchLight[ndxBump] );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void AddPatchLightToRadial( Vector const &patchOrigin, Vector const &patchNormal,
							Vector *pPatchLight, float patchRadius2,
							Vector const &luxelPos, Vector const &luxelNormal,
							radial_t *pRadial, int ndxRadial, bool bBump, 
							bool bNeighborBump ) 
{
	// calculate the light vector
	Vector vSegment = patchOrigin - luxelPos;

	// get the distance to the light
	float dist = vSegment.Length();
	float dist2 = dist * dist;

	// check to see if the light is within the sample distance
	if( dist2 > patchRadius2 )
		return;

	float influence = dist2 / ( patchRadius2 * patchRadius2 );

	if( bBump )
	{
		if( bNeighborBump )
		{
			for( int ndxBump = 0; ndxBump < ( NUM_BUMP_VECTS+1 ); ndxBump++ )
			{
				pRadial->light[ndxBump][ndxRadial] += pPatchLight[ndxBump] * influence;
			}
		}
		else
		{
			pRadial->light[0][ndxRadial] += pPatchLight[0] * influence;
			
			for( int ndxBump = 1; ndxBump < ( NUM_BUMP_VECTS+1 ); ndxBump++ )
			{
				pRadial->light[ndxBump][ndxRadial] += pPatchLight[0] * influence * OO_SQRT_3;
			}
		}
	}
	else
	{
		pRadial->light[0][ndxRadial] += pPatchLight[0] * influence;
	}

	// add the weight value
	pRadial->weight[ndxRadial] += influence;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float GetPatchRadius2( patch_t *pPatch )
{
	//
	// get minor axes
	//
	int minorAxes[2];
	int majorAxes = 0;
	if( pPatch->normal[majorAxes] < pPatch->normal[1] ) { majorAxes = 1; }
	if( pPatch->normal[majorAxes] < pPatch->normal[2] ) { majorAxes = 2; }

	switch( majorAxes )
	{
	case 0: { minorAxes[0] = 1; minorAxes[1] = 2; break; }
	case 1: { minorAxes[0] = 0; minorAxes[1] = 2; break; }
	case 2: { minorAxes[0] = 0; minorAxes[1] = 1; break; }
	}

	float dist0 = pPatch->maxs[minorAxes[0]] - pPatch->mins[minorAxes[0]];
	float dist1 = pPatch->maxs[minorAxes[1]] - pPatch->mins[minorAxes[1]];

	return ( ( ( dist0 * dist0 ) + ( dist1 * dist1 ) ) * RADIALDIST2 );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::RadialLuxelAddPatch( int ndxFace, Vector const &luxelPt, 
									    Vector const &luxelNormal,  float radius, 
									    radial_t *pRadial, int ndxRadial, bool bBump,
									    CUtlVector<patch_t*> &interestingPatches )
{
#ifdef SAMPLEHASH_QUERY_ONCE
	for ( int i=0; i < interestingPatches.Count(); i++ )
	{
		patch_t *pPatch = interestingPatches[i];	
		bool bNeighborBump = texinfo[g_pFaces[pPatch->faceNumber].texinfo].flags & SURF_BUMPLIGHT ? true : false;
		
		Vector patchLight[NUM_BUMP_VECTS+1];
		GetPatchLight( pPatch, bBump, patchLight );
		AddPatchLightToRadial( pPatch->origin, pPatch->normal, patchLight, radius*radius,
								luxelPt, luxelNormal, pRadial, ndxRadial, bBump, bNeighborBump );
	}
#else
	// calculate one over the voxel size
	float ooVoxelSize = 1.0f / SAMPLEHASH_VOXEL_SIZE;

	//
	// find voxel info
	//
	int voxelMin[3], voxelMax[3];
	for ( int axis = 0; axis < 3; axis++ )
	{
		voxelMin[axis] = ( int )( ( luxelPt[axis] - radius ) * ooVoxelSize );
		voxelMax[axis] = ( int )( ( luxelPt[axis] + radius ) * ooVoxelSize ) + 1;
	}

	unsigned short curIterationKey = IncrementPatchIterationKey();
	PatchSampleData_t patchData;	
	for ( int ndxZ = voxelMin[2]; ndxZ < voxelMax[2] + 1; ndxZ++ )
	{
		for ( int ndxY = voxelMin[1]; ndxY < voxelMax[1] + 1; ndxY++ )
		{
			for ( int ndxX = voxelMin[0]; ndxX < voxelMax[0] + 1; ndxX++ )
			{
				patchData.x = ndxX * 100;
				patchData.y = ndxY * 10;
				patchData.z = ndxZ;
				
				UtlHashHandle_t handle = g_PatchSampleHashTable.Find( patchData );
				if ( handle != g_PatchSampleHashTable.InvalidHandle() )
				{
					PatchSampleData_t *pPatchData = &g_PatchSampleHashTable.Element( handle );
					int count = pPatchData->m_ndxPatches.Count();
					for ( int ndx = 0; ndx < count; ndx++ )
					{
						int ndxPatch = pPatchData->m_ndxPatches.Element( ndx );
						patch_t *pPatch = &patches.Element( ndxPatch );
						if ( pPatch && pPatch->m_IterationKey != curIterationKey )
						{
							pPatch->m_IterationKey = curIterationKey;
							
							if ( IsNeighbor( ndxFace, pPatch->faceNumber ) )
							{									
								bool bNeighborBump = texinfo[g_pFaces[pPatch->faceNumber].texinfo].flags & SURF_BUMPLIGHT ? true : false;
								
								Vector patchLight[NUM_BUMP_VECTS+1];
								GetPatchLight( pPatch, bBump, patchLight );
								AddPatchLightToRadial( pPatch->origin, pPatch->normal, patchLight, radius*radius,
									                   luxelPt, luxelNormal, pRadial, ndxRadial, bBump, bNeighborBump );
							}
						}
					}
				}
			}
		}
	}
#endif
}


void CVRadDispMgr::GetInterestingPatchesForLuxels( 
	int ndxFace,
	CUtlVector<patch_t*> &interestingPatches,
	float patchSampleRadius )
{
	facelight_t *pFaceLight = &facelight[ndxFace]; 

	// Get the max bounds of all voxels that these luxels touch.
	Vector vLuxelMin( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector vLuxelMax( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	for ( int i=0; i < pFaceLight->numluxels; i++ )
	{
		VectorMin( pFaceLight->luxel[i], vLuxelMin, vLuxelMin );
		VectorMax( pFaceLight->luxel[i], vLuxelMax, vLuxelMax );
	}
		
	int allVoxelMin[3], allVoxelMax[3];
	for ( int axis = 0; axis < 3; axis++ )
	{
		allVoxelMin[axis] = ( int )( ( vLuxelMin[axis] - patchSampleRadius ) / SAMPLEHASH_VOXEL_SIZE );
		allVoxelMax[axis] = ( int )( ( vLuxelMax[axis] + patchSampleRadius ) / SAMPLEHASH_VOXEL_SIZE ) + 1;
	}
	int allVoxelSize[3] = { allVoxelMax[0] - allVoxelMin[0], allVoxelMax[1] - allVoxelMin[1], allVoxelMax[2] - allVoxelMin[2] };


	// Now figure out exactly which voxels these luxels touch.
	CUtlVector<unsigned char> voxelBits;
	voxelBits.SetSize( ((allVoxelSize[0] * allVoxelSize[1] * allVoxelSize[2]) + 7) / 8 );
	memset( voxelBits.Base(), 0, voxelBits.Count() );
	
	for ( int i=0; i < pFaceLight->numluxels; i++ )
	{
		int voxelMin[3], voxelMax[3];
		for ( int axis=0; axis < 3; axis++ )
		{
			voxelMin[axis] = ( int )( ( pFaceLight->luxel[i][axis] - patchSampleRadius ) / SAMPLEHASH_VOXEL_SIZE );
			voxelMax[axis] = ( int )( ( pFaceLight->luxel[i][axis] + patchSampleRadius ) / SAMPLEHASH_VOXEL_SIZE ) + 1;
		}

		for ( int x=voxelMin[0]; x < voxelMax[0]; x++ )
		{
			for	( int y=voxelMin[1]; y < voxelMax[1]; y++ )
			{
				for ( int z=voxelMin[2]; z < voxelMax[2]; z++ )
				{
					int iBit = (z - allVoxelMin[2])*(allVoxelSize[0]*allVoxelSize[1]) + 
						(y-allVoxelMin[1])*allVoxelSize[0] + 
						(x-allVoxelMin[0]);
					voxelBits[iBit>>3] |= (1 << (iBit & 7));
				}
			}
		}
	}
	
	
	// Now get the list of patches that touch those voxels.	
	unsigned short curIterationKey = IncrementPatchIterationKey();

	for ( int x=0; x < allVoxelSize[0]; x++ )
	{
		for ( int y=0; y < allVoxelSize[1]; y++ )
		{
			for ( int z=0; z < allVoxelSize[2]; z++ )
			{
				// Make sure this voxel has any luxels that care about it.
				int iBit = z*(allVoxelSize[0]*allVoxelSize[1]) + y*allVoxelSize[0] + x;
				unsigned char val = voxelBits[iBit>>3] & (1 << (iBit & 7));
				if ( !val )
					continue;
				
				PatchSampleData_t patchData;	
				patchData.x = (x + allVoxelMin[0]) * 100;
				patchData.y = (y + allVoxelMin[1]) * 10;
				patchData.z = (z + allVoxelMin[2]);
				
				UtlHashHandle_t handle = g_PatchSampleHashTable.Find( patchData );
				if ( handle != g_PatchSampleHashTable.InvalidHandle() )
				{
					PatchSampleData_t *pPatchData = &g_PatchSampleHashTable.Element( handle );
					
					// For all patches that touch this hash table element..
					for ( int ndx = 0; ndx < pPatchData->m_ndxPatches.Count(); ndx++ )
					{
						int ndxPatch = pPatchData->m_ndxPatches.Element( ndx );
						patch_t *pPatch = &patches.Element( ndxPatch );
						
						// If we haven't touched the patch already and it's a valid neighbor, then we want to use it.
						if ( pPatch && pPatch->m_IterationKey != curIterationKey )
						{
							pPatch->m_IterationKey = curIterationKey;
							
							if ( IsNeighbor( ndxFace, pPatch->faceNumber ) )
							{
								interestingPatches.AddToTail( pPatch );
							}
						}
					}
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::RadialPatchBuild( CVRADDispColl *pDispTree, radial_t *pRadial,
									 bool bBump )
{
	//
	// get data lighting data
	//
	int ndxFace = pDispTree->GetParentIndex();
	facelight_t *pFaceLight = &facelight[ndxFace];

	// get the influence radius
	float radius2 = pDispTree->GetPatchSampleRadius2();
	float radius = ( float )sqrt( radius2 );

	CUtlVector<patch_t*> interestingPatches;
#ifdef SAMPLEHASH_QUERY_ONCE
	GetInterestingPatchesForLuxels( ndxFace, interestingPatches, radius );
#endif

	int radialSize = pRadial->w * pRadial->h;
	for( int ndxRadial = 0; ndxRadial < radialSize; ndxRadial++ )
	{
		RadialLuxelAddPatch( 
			ndxFace, 
			pFaceLight->luxel[ndxRadial], 
			pFaceLight->luxelNormals[ndxRadial],
			radius, 
			pRadial, 
			ndxRadial, 
			bBump,
			interestingPatches );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
radial_t *CVRadDispMgr::BuildPatchRadial( int ndxFace, bool bBump )
{
	// allocate the radial
	radial_t *pRadial = AllocateRadial( ndxFace );
	if( !pRadial )
		return NULL;

	//
	// step 1: get the displacement surface to be lit
	//
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return NULL;

	// step 2: build radial of patch light
	RadialPatchBuild( pDispTree, pRadial, bBump );

	// step 3: return the built radial
	return pRadial;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool SampleInSolid( sample_t *pSample )
{
	int ndxLeaf = PointLeafnum( pSample->pos );
	return ( dleafs[ndxLeaf].contents == CONTENTS_SOLID );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::InsertSamplesDataIntoHashTable( void )
{
	int totalSamples = 0;
#if 0
	int totalSamplesInSolid = 0;
#endif

	for( int ndxFace = 0; ndxFace < numfaces; ndxFace++ )
	{
		dface_t *pFace = &g_pFaces[ndxFace];
		facelight_t *pFaceLight = &facelight[ndxFace];
		if( !pFace || !pFaceLight )
			continue;
		
		if( texinfo[pFace->texinfo].flags & TEX_SPECIAL )
			continue;

#if 0
		bool bDisp = ( pFace->dispinfo != -1 );
#endif
		//
		// for each sample
		//
		for( int ndxSample = 0; ndxSample < pFaceLight->numsamples; ndxSample++ )
		{
			sample_t *pSample = &pFaceLight->sample[ndxSample];
			if( pSample )
			{
#if 0
				if( bDisp )
				{
					// test sample to see if the displacement samples resides in solid
					if( SampleInSolid( pSample ) )
					{
						totalSamplesInSolid++;
						continue;
					}
				}
#endif

				// create the sample handle
				SampleHandle_t sampleHandle = ndxSample;
				sampleHandle |= ( ndxFace << 16 );
				
				SampleData_AddSample( pSample, sampleHandle );
			}

		}

		totalSamples += pFaceLight->numsamples;
	}

#if 0
	// not implemented yet!!!
	Msg( "%d samples in solid\n", totalSamplesInSolid );
#endif

	// log the distribution
	SampleData_Log();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::InsertPatchSampleDataIntoHashTable( void )
{
	// don't insert patch samples if we are not bouncing light
	if( numbounce <= 0 )
		return;

	int totalPatchSamples = 0;

	for( int ndxFace = 0; ndxFace < numfaces; ndxFace++ )
	{
		dface_t *pFace = &g_pFaces[ndxFace];
		facelight_t *pFaceLight = &facelight[ndxFace];
		if( !pFace || !pFaceLight )
			continue;
		
		if( texinfo[pFace->texinfo].flags & TEX_SPECIAL )
			continue;
		
		//
		// for each patch
		//
		patch_t *pNextPatch = NULL;
		if( facePatches.Element( ndxFace ) != facePatches.InvalidIndex() )
		{
			for( patch_t *pPatch = &patches.Element( facePatches.Element( ndxFace ) ); pPatch; pPatch = pNextPatch )
			{
				// next patch
				pNextPatch = NULL;
				if( pPatch->ndxNext != patches.InvalidIndex() )
				{
					pNextPatch = &patches.Element( pPatch->ndxNext );
				}
			
				// skip patches with children
				if( pPatch->child1 != patches.InvalidIndex() )
					continue;
			
				int ndxPatch = pPatch - patches.Base();
				PatchSampleData_AddSample( pPatch, ndxPatch );

				totalPatchSamples++;
			}
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::StartTimer( const char *name )
{
	Msg( name );
	m_Timer.Start();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRadDispMgr::EndTimer( void )
{
	m_Timer.End();
	CCycleCount duration = m_Timer.GetDuration();
	double seconds = duration.GetSeconds();

	Msg( "Done<%1.4lf sec>\n", seconds );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::BuildDispSamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// get the tree assosciated with the face
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return false;

	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

#if 0
	// leaving this here until the displacement common code comes around!!
	bool bUMajor = pDispTree->GetMajorAxisUV();
	if ( ( bUMajor && ( height > width ) ) || 
		 ( !bUMajor && ( width > height ) ) )
	{
		_asm int 3;
		width = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;
		height = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	}
#endif

	// calculate the steps in uv space
	float stepU = 1.0f / ( float )width;
	float stepV = 1.0f / ( float )height;
	float halfStepU = stepU * 0.5f;
	float halfStepV = stepV * 0.5f;
	
	//
	// build the winding points (used to generate world space winding and
	// calculate the area of the "sample")
	//
	int ndxU, ndxV;

	// NOTE: These allocations are necessary to avoid stack overflow
	// FIXME: Solve with storing global per-thread temp buffers if there's
	// a performance problem with this solution...
	bool bTempAllocationNecessary = ((height + 1) * (width + 1)) > SINGLE_BRUSH_MAP;

	Vector worldPointBuffer[SINGLE_BRUSH_MAP];
	sample_t sampleBuffer[SINGLE_BRUSH_MAP*2];

	Vector *pWorldPoints = worldPointBuffer;
	sample_t *pSamples = sampleBuffer;
	if (bTempAllocationNecessary)
	{
		pWorldPoints = (Vector*)malloc( SINGLEMAP * sizeof(Vector) );
		pSamples = (sample_t*)malloc( SINGLEMAP * sizeof(sample_t) );
	}

	for( ndxV = 0; ndxV < ( height + 1 ); ndxV++ )
	{
		for( ndxU = 0; ndxU < ( width + 1 ); ndxU++ )
		{
			int ndx = ( ndxV * ( width + 1 ) ) + ndxU;

			Vector2D uv( ndxU * stepU, ndxV * stepV );
			pDispTree->DispUVToSurfPoint( uv, pWorldPoints[ndx], 0.0f );
		}
	}


	for( ndxV = 0; ndxV < height; ndxV++ )
	{
		for( ndxU = 0; ndxU < width; ndxU++ )
		{
			// build the winding
			winding_t *pWinding = AllocWinding( 4 );
			if( pWinding )
			{
				pWinding->numpoints = 4;
				pWinding->p[0] = pWorldPoints[(ndxV*(width+1))+ndxU];
				pWinding->p[1] = pWorldPoints[((ndxV+1)*(width+1))+ndxU];
				pWinding->p[2] = pWorldPoints[((ndxV+1)*(width+1))+(ndxU+1)];
				pWinding->p[3] = pWorldPoints[(ndxV*(width+1))+(ndxU+1)];
				
				// calculate the area
				float area = WindingArea( pWinding );
				
				int ndxSample = ( ndxV * width ) + ndxU;
				pSamples[ndxSample].w = pWinding;
				pSamples[ndxSample].area = area;
			}
			else
			{
				Msg( "BuildDispSamples: WARNING - failed winding allocation\n" );
			}
		}
	}

	//
	// build the samples points (based on s, t and sampleoffset (center of samples);
	// generates world space position and normal)
	//
	for( ndxV = 0; ndxV < height; ndxV++ )
	{
		for( ndxU = 0; ndxU < width; ndxU++ )
		{
			int ndxSample = ( ndxV * width ) + ndxU;
			pSamples[ndxSample].s = ndxU;
			pSamples[ndxSample].t = ndxV;
			pSamples[ndxSample].coord[0] = ( ndxU * stepU ) + halfStepU;
			pSamples[ndxSample].coord[1] = ( ndxV * stepV ) + halfStepV;
			pDispTree->DispUVToSurfPoint( pSamples[ndxSample].coord, pSamples[ndxSample].pos, 1.0f );
			pDispTree->DispUVToSurfNormal( pSamples[ndxSample].coord, pSamples[ndxSample].normal );			
		}
	}

	//
	// copy over samples
	//
	pFaceLight->numsamples = width * height;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		goto buildDispSamplesError;

	memcpy( pFaceLight->sample, pSamples, pFaceLight->numsamples * sizeof( *pFaceLight->sample ) );

	// statistics - warning?!
	if( pFaceLight->numsamples == 0 )
	{
		Msg( "BuildDispSamples: WARNING - no samples %d\n", pLightInfo->face - g_pFaces );
	}
 
	if (bTempAllocationNecessary)
	{
		free( pWorldPoints );
		free( pSamples );
	}

	return true;

buildDispSamplesError:
	if (bTempAllocationNecessary)
	{
		free( pWorldPoints );
		free( pSamples );
	}

	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::BuildDispLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// get the tree assosciated with the face
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return false;

	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

#if 0
	// leaving this here until the displacement common code comes around!!
	bool bUMajor = pDispTree->GetMajorAxisUV();
	if ( ( bUMajor && ( height > width ) ) || 
		 ( !bUMajor && ( width > height ) ) )
	{
		_asm int 3;
		width = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;
		height = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	}
#endif

	// calcuate actual luxel points
	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	pFaceLight->luxelNormals = ( Vector* )calloc( pFaceLight->numluxels, sizeof( Vector ) );
	if( !pFaceLight->luxel || !pFaceLight->luxelNormals )
		return false;

	float stepU = 1.0f / ( float )( width - 1 );
	float stepV = 1.0f / ( float )( height - 1 );

	for( int ndxV = 0; ndxV < height; ndxV++ )
	{
		for( int ndxU = 0; ndxU < width; ndxU++ )
		{
			int ndxLuxel = ( ndxV * width ) + ndxU; 

			Vector2D uv( ndxU * stepU, ndxV * stepV );
			pDispTree->DispUVToSurfPoint( uv, pFaceLight->luxel[ndxLuxel], 1.0f );
			pDispTree->DispUVToSurfNormal( uv, pFaceLight->luxelNormals[ndxLuxel] );			
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRadDispMgr::BuildDispSamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// get the tree assosciated with the face
	DispCollTree_t &dispTree = m_DispTrees[g_pFaces[ndxFace].dispinfo];
	CVRADDispColl *pDispTree = dispTree.m_pDispTree;
	if( !pDispTree )
		return false;

	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// calcuate actual luxel points
	pFaceLight->numsamples = width * height;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		return false;

	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	pFaceLight->luxelNormals = ( Vector* )calloc( pFaceLight->numluxels, sizeof( Vector ) );
	if( !pFaceLight->luxel || !pFaceLight->luxelNormals )
		return false;

	float stepU = 1.0f / ( float )( width - 1 );
	float stepV = 1.0f / ( float )( height - 1 );
	float halfStepU = stepU * 0.5f;
	float halfStepV = stepV * 0.5f;

	for( int ndxV = 0; ndxV < height; ndxV++ )
	{
		for( int ndxU = 0; ndxU < width; ndxU++ )
		{
			int ndx = ( ndxV * width ) + ndxU;

			pFaceLight->sample[ndx].s = ndxU;
			pFaceLight->sample[ndx].t = ndxV;
			pFaceLight->sample[ndx].coord[0] = ( ndxU * stepU ) + halfStepU;
			pFaceLight->sample[ndx].coord[1] = ( ndxV * stepV ) + halfStepV;

			pDispTree->DispUVToSurfPoint( pFaceLight->sample[ndx].coord, pFaceLight->sample[ndx].pos, 1.0f );
			pDispTree->DispUVToSurfNormal( pFaceLight->sample[ndx].coord, pFaceLight->sample[ndx].normal );			

			pFaceLight->luxel[ndx] = pFaceLight->sample[ndx].pos;
			pFaceLight->luxelNormals[ndx] = pFaceLight->sample[ndx].normal;
		}
	}

	return true;
}
