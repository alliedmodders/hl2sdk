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
#include "VRAD_DispColl.h"
#include "DispColl_Common.h"
#include "radial.h"
#include "CollisionUtils.h"
#include "tier0\dbg.h"

#define SAMPLE_BBOX_SLOP		5.0f
#define TRIEDGE_EPSILON			0.001f

float g_flMaxDispSampleSize = 512.0f;

static FileHandle_t pDispFile = FILESYSTEM_INVALID_HANDLE;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::CVRADDispColl()
{
	m_iParent = -1;
	m_nPointOffset = -1;

	m_flSampleRadius2 = 0.0f;
	m_flPatchSampleRadius2 = 0.0f;

	m_vecSampleBBox[0].Init();
	m_vecSampleBBox[1].Init();

	m_aVNodes.Purge();
	m_aLuxelCoords.Purge();
	m_aVertNormals.Purge();
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CVRADDispColl::~CVRADDispColl()
{
	m_aVNodes.Purge();
	m_aLuxelCoords.Purge();
	m_aVertNormals.Purge();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::Create( CCoreDispInfo *pDisp )
{
	// Base class create.
	if( !CDispCollTree::Create( pDisp ) )
		return false;

	// Allocate VRad specific memory.
	m_aVNodes.SetSize( Nodes_CalcCount( m_nPower ) );
	m_aLuxelCoords.SetSize( GetSize() );
	m_aVertNormals.SetSize( GetSize() );

	// VRad specific base surface data.
	CCoreDispSurface *pSurf = pDisp->GetSurface();
	m_iParent = pSurf->GetHandle();
	m_nPointOffset = pSurf->GetPointStartIndex();

	int nPointCount = GetParentPointCount();
	for( int iPoint = 0; iPoint < nPointCount; ++iPoint )
	{
		pSurf->GetPointNormal( iPoint, m_vecSurfPointNormals[iPoint] );
	}

	// VRad specific displacement surface data.
	for ( int iVert = 0; iVert < m_aVerts.Count(); ++iVert )
	{
		pDisp->GetNormal( iVert, m_aVertNormals[iVert] );
		pDisp->GetLuxelCoord( 0, iVert, m_aLuxelCoords[iVert] );
	}

	// Re-calculate the lightmap size (in uv) so that the luxels give
	// a better world-space uniform approx. due to the non-linear nature
	// of the displacement surface in uv-space
	dface_t *pFace = &g_pFaces[m_iParent];
	if( pFace )
	{
		CalcSampleRadius2AndBox( pFace );	
	}

	// Create the vrad specific node data - build for radiosity transfer,
	// Don't create for direct lighting only!	
	BuildVNodes();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::CalcSampleRadius2AndBox( dface_t *pFace )
{
	Vector boxMin, boxMax;
	GetSurfaceMinMax( boxMin, boxMax );

	Vector vecFaceNormal;
	int nAxis0, nAxis1;
	GetParentFaceNormal( vecFaceNormal );
	GetMinorAxes( vecFaceNormal, nAxis0, nAxis1 );

	float flWidth = boxMax[nAxis0] - boxMin[nAxis0];
	float flHeight = boxMax[nAxis1] - boxMin[nAxis1];
	flWidth /= pFace->m_LightmapTextureSizeInLuxels[0];
	flHeight /= pFace->m_LightmapTextureSizeInLuxels[1];

	// Calculate the sample radius squared.
	float flSampleRadius = sqrt( ( ( flWidth*flWidth) + ( flHeight*flHeight) ) ) * RADIALDIST2; 
	if ( flSampleRadius > g_flMaxDispSampleSize )
	{
		flSampleRadius = g_flMaxDispSampleSize;
	}
	m_flSampleRadius2 = flSampleRadius * flSampleRadius;

	// Calculate the sampling bounding box.
	m_vecSampleBBox[0] = boxMin;
	m_vecSampleBBox[1] = boxMax;
	for( int iAxis = 0; iAxis < 3; ++iAxis )
	{
		m_vecSampleBBox[0][iAxis] -= SAMPLE_BBOX_SLOP;
		m_vecSampleBBox[1][iAxis] += SAMPLE_BBOX_SLOP;
	}
	
	CalcPatchSampleRadius2();
}

//-----------------------------------------------------------------------------
// Purpose: Determine the optimal patch sampling radius (squared).
//-----------------------------------------------------------------------------
void CVRADDispColl::CalcPatchSampleRadius2( void )
{
	// Note: we currently have to choose between two not-so-great options because the
	// patches that are constructed for displacments are just too large if the displacment is large.
	// The real fix is to fix the disp patch subdivision code.
	if ( g_bLargeDispSampleRadius )
	{
		// Find the largest delta in x, y, or z.
		float flDistMax = 0.0f;
		for ( int iAxis = 0; iAxis < 3; ++iAxis )
		{
			float flDist = fabs( m_vecSampleBBox[1][iAxis] - m_vecSampleBBox[0][iAxis] );
			if ( flDist > flDistMax )
			{
				flDistMax = flDist;
			}
		}

		// Calculate the divisor based on the power of the displacement surface.
		int nPower = GetPower();
		float flScale = 1.0f / static_cast<float>( ( 1 << ( nPower + 1 ) ) );
		float flValue = flDistMax * flScale;
		float flPatchSampleRadius = sqrt( ( flValue*flValue ) + ( flValue*flValue ) ) * RADIALDIST2;

		// Squared.
		m_flPatchSampleRadius2 = ( flPatchSampleRadius * flPatchSampleRadius );
	}
	else
	{
		// maxchop - see vrad.h
		// kjb - FIXME: I'm reverting this to the pre 111277 version of the code, but this is still wrong.
		// maxchop has nothing do with the sample sizes on a face.  I'm guessing the above code is almost 
		// correct, but it's artifically clamping on large luxels sizes which is probably a bug, but these
		// next two lines throw the above answer away so why is it even doing it at all, and I can't find 
		// where chop is used by the displacement system ever.... This function needs help.
		float flPatchSampleRadius = sqrt( ( maxchop*16*maxchop*16 ) + ( maxchop*16*maxchop*16 ) ) * RADIALDIST2;
		m_flPatchSampleRadius2 = flPatchSampleRadius * flPatchSampleRadius;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the min/max of the displacement surface.
//-----------------------------------------------------------------------------
void CVRADDispColl::GetSurfaceMinMax( Vector &boxMin, Vector &boxMax )
{
	// Initialize the minimum and maximum box
	boxMin = m_aVerts[0];
	boxMax = m_aVerts[0];

	for( int i = 1; i < m_aVerts.Count(); i++ )
	{
		if( m_aVerts[i].x < boxMin.x ) { boxMin.x = m_aVerts[i].x; }
		if( m_aVerts[i].y < boxMin.y ) { boxMin.y = m_aVerts[i].y; }
		if( m_aVerts[i].z < boxMin.z ) { boxMin.z = m_aVerts[i].z; }

		if( m_aVerts[i].x > boxMax.x ) { boxMax.x = m_aVerts[i].x; }
		if( m_aVerts[i].y > boxMax.y ) { boxMax.y = m_aVerts[i].y; }
		if( m_aVerts[i].z > boxMax.z ) { boxMax.z = m_aVerts[i].z; }
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the minor projection axes based on the given normal.
//-----------------------------------------------------------------------------
void CVRADDispColl::GetMinorAxes( Vector const &vecNormal, int &nAxis0, int &nAxis1 )
{
	nAxis0 = 0;
	nAxis1 = 1;

	if( FloatMakePositive( vecNormal.x ) > FloatMakePositive( vecNormal.y ) )
	{
		if( FloatMakePositive( vecNormal.x ) > FloatMakePositive( vecNormal.z ) )
		{
			nAxis0 = 1;
			nAxis1 = 2;
		}
	}
	else
	{
		if( FloatMakePositive( vecNormal.y ) > FloatMakePositive( vecNormal.z ) )
		{
			nAxis0 = 0;
			nAxis1 = 2;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::BuildVNodes( void )
{
	// Get leaf indices (last level in tree)
	int iStart = Nodes_CalcCount( m_nPower - 1 );
	int iEnd = Nodes_CalcCount( m_nPower );

	int flWidth = GetWidth();

	for ( int iNode = iStart; iNode < iEnd; ++iNode )
	{
		// Get the current node
		CDispCollAABBNode *pNode = &m_aNodes[iNode];
		VNode_t *pVNode = &m_aVNodes[iNode];

		if ( !pNode )
			continue;

		// Cache locally the triangle verts and normals.
		Vector vecTriVerts[2][3];
		Vector vecTriVertNormals[2][3];
		for ( int iTri = 0; iTri < 2; ++iTri )	
		{
			for ( int iVert = 0; iVert < 3; iVert++ )
			{
				vecTriVerts[iTri][iVert] = m_aVerts[m_aTris[pNode->m_iTris[iTri]].GetVert(iVert)];
				vecTriVertNormals[iTri][iVert] = m_aVertNormals[m_aTris[pNode->m_iTris[iTri]].GetVert(iVert)];
			}
		}
		
		// Get the surface area of the triangles in the node.
		pVNode->patchArea = 0.0f;
		for ( int iTri = 0; iTri < 2; ++iTri )
		{
			Vector vecEdges[2], vecCross;			
			VectorSubtract( vecTriVerts[iTri][1], vecTriVerts[iTri][0], vecEdges[0] );
			VectorSubtract( vecTriVerts[iTri][2], vecTriVerts[iTri][0], vecEdges[1] );
			CrossProduct( vecEdges[0], vecEdges[1], vecCross );
			pVNode->patchArea += 0.5f * VectorLength( vecCross );
		}			

		// Get the patch origin (along the diagonal!).
		Vector vecEdgePoints[2];
		int nEdgePointCount = 0;
		int iEdges[2];

		for ( int iVert = 0; iVert < 3; ++iVert )
		{
			for ( int iVert2 = 0; iVert2 < 3; ++iVert2 )
			{
				if ( m_aTris[pNode->m_iTris[0]].GetVert( iVert ) == m_aTris[pNode->m_iTris[1]].GetVert( iVert2 ) )
				{
					iEdges[nEdgePointCount] = m_aTris[pNode->m_iTris[0]].GetVert( iVert );
					vecEdgePoints[nEdgePointCount] = vecTriVerts[0][iVert];
					nEdgePointCount++;
					break;
				}
			}
		}
		if ( nEdgePointCount != 2 )
			continue;

		pVNode->patchOrigin = ( vecEdgePoints[0] + vecEdgePoints[1] ) * 0.5f;
		
		Vector2D vecUV0, vecUV1;
		float flScale = 1.0f / ( float )( flWidth - 1 );
		vecUV0.x = ( iEdges[0] % flWidth ) * flScale;
		vecUV0.y = ( iEdges[0] / flWidth ) * flScale;		
		vecUV1.x = ( iEdges[1] % flWidth ) * flScale;
		vecUV1.y = ( iEdges[1] / flWidth ) * flScale;
		
		pVNode->patchOriginUV = ( vecUV0 + vecUV1 ) * 0.5f;
				
		// Get the averaged patch normal.
		pVNode->patchNormal.Init();
		for( int iVert = 0; iVert < 3; ++iVert )
		{
			VectorAdd( pVNode->patchNormal, vecTriVertNormals[0][iVert], pVNode->patchNormal );
			
			if( ( vecTriVerts[1][iVert] != vecTriVerts[0][0] ) && ( vecTriVerts[1][iVert] != vecTriVerts[0][1] ) && ( vecTriVerts[1][iVert] != vecTriVerts[0][2] ) )
			{
				VectorAdd( pVNode->patchNormal, vecTriVertNormals[1][iVert], pVNode->patchNormal );
			}
		}
		VectorNormalize( pVNode->patchNormal );
		
		// Copy the bounds.
		pVNode->patchBounds[0] = pNode->m_vecBox[0];
		pVNode->patchBounds[1] = pNode->m_vecBox[1];
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::BaseFacePlaneToDispUV( Vector const &vecPlanePt, Vector2D &dispUV )
{
	PointInQuadToBarycentric( m_vecSurfPoints[0], m_vecSurfPoints[3], m_vecSurfPoints[2], m_vecSurfPoints[1], vecPlanePt, dispUV );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfPoint( Vector2D const &dispUV, Vector &vecPoint, float flPushEps )
{
	// Check to see that the point is on the surface.
	if ( dispUV.x < 0.0f || dispUV.x > 1.0f || dispUV.y < 0.0f || dispUV.y > 1.0f )
		return;

	// Get the displacement power.
	int nWidth = ( ( 1 << m_nPower ) + 1 );
	int nHeight = nWidth;

	// Scale the U, V coordinates to the displacement grid size.
	float flU = dispUV.x * static_cast<float>( nWidth - 1.000001f );
	float flV = dispUV.y * static_cast<float>( nHeight - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	// Use this to get the triangle orientation.
	bool bOdd = ( ( ( nSnapV * nWidth ) + nSnapU ) % 2 == 1 );

	// Top Left to Bottom Right
	if( bOdd )
	{
		DispUVToSurf_TriTLToBR( vecPoint, flPushEps, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
	// Bottom Left to Top Right
	else
	{
		DispUVToSurf_TriBLToTR( vecPoint, flPushEps, flU, flV, nSnapU, nSnapV, nWidth, nHeight );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurf_TriTLToBR( Vector &vecPoint, float flPushEps, 
											float flU, float flV, int nSnapU, int nSnapV, 
											int nWidth, int nHeight )
{
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( ( flFracU + flFracV ) >= ( 1.0f + TRIEDGE_EPSILON ) )
	{
		int nIndices[3];
		nIndices[0] = nNextV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nNextU;	
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[0]] - m_aVerts[nIndices[1]];
		Vector edgeV = m_aVerts[nIndices[2]] - m_aVerts[nIndices[1]];
		vecPoint = m_aVerts[nIndices[1]] + edgeU * ( 1.0f - flFracU ) + edgeV * ( 1.0f - flFracV );

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
	else
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nSnapU;
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[2]] - m_aVerts[nIndices[0]];
		Vector edgeV = m_aVerts[nIndices[1]] - m_aVerts[nIndices[0]];
		vecPoint = m_aVerts[nIndices[0]] + edgeU * flFracU + edgeV * flFracV;

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeU, edgeV );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurf_TriBLToTR( Vector &vecPoint, float flPushEps, 
											float flU, float flV, int nSnapU, int nSnapV, 
											int nWidth, int nHeight )
{
	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	if( flFracU < flFracV )
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nSnapU;
		nIndices[2] = nNextV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[2]] - m_aVerts[nIndices[1]];
		Vector edgeV = m_aVerts[nIndices[0]] - m_aVerts[nIndices[1]];
		vecPoint =  m_aVerts[nIndices[1]] + edgeU * flFracU + edgeV * ( 1.0f - flFracV );

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
	else
	{
		int nIndices[3];
		nIndices[0] = nSnapV * nWidth + nSnapU;
		nIndices[1] = nNextV * nWidth + nNextU;
		nIndices[2] = nSnapV * nWidth + nNextU;

		Vector edgeU = m_aVerts[nIndices[0]] - m_aVerts[nIndices[2]];
		Vector edgeV = m_aVerts[nIndices[1]] - m_aVerts[nIndices[2]];
		vecPoint = m_aVerts[nIndices[2]] + edgeU * ( 1.0f - flFracU ) + edgeV * flFracV;

		if ( flPushEps != 0.0f )
		{
			Vector vecNormal;
			vecNormal = CrossProduct( edgeV, edgeU );
			VectorNormalize( vecNormal );
			vecPoint += ( vecNormal * flPushEps );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::DispUVToSurfNormal( Vector2D const &dispUV, Vector &vecNormal )
{
	// Check to see that the point is on the surface.
	if ( dispUV.x < 0.0f || dispUV.x > 1.0f || dispUV.y < 0.0f || dispUV.y > 1.0f )
		return;

	// Get the displacement power.
	int nWidth = ( ( 1 << m_nPower ) + 1 );
	int nHeight = nWidth;

	// Scale the U, V coordinates to the displacement grid size.
	float flU = dispUV.x * static_cast<float>( nWidth - 1.000001f );
	float flV = dispUV.y * static_cast<float>( nHeight - 1.000001f );

	// Find the base U, V.
	int nSnapU = static_cast<int>( flU );
	int nSnapV = static_cast<int>( flV );

	int nNextU = nSnapU + 1;
	int nNextV = nSnapV + 1;
	if ( nNextU == nWidth)	 { --nNextU; }
	if ( nNextV == nHeight ) { --nNextV; }

	float flFracU = flU - static_cast<float>( nSnapU );
	float flFracV = flV - static_cast<float>( nSnapV );

	// Get the four normals "around" the "spot"
	int iQuad[VRAD_QUAD_SIZE];
	iQuad[0] = ( nSnapV * nWidth ) + nSnapU;
	iQuad[1] = ( nNextV * nWidth ) + nSnapU;
	iQuad[2] = ( nNextV * nWidth ) + nNextU;
	iQuad[3] = ( nSnapV * nWidth ) + nNextU;

	// Find the blended normal (bi-linear).
	Vector vecTmpNormals[2], vecBlendedNormals[2], vecDispNormals[4];
	
	for ( int iVert = 0; iVert < VRAD_QUAD_SIZE; ++iVert )
	{
		GetVertNormal( iQuad[iVert], vecDispNormals[iVert] );
	}

	vecTmpNormals[0] = vecDispNormals[0] * ( 1.0f - flFracU );
	vecTmpNormals[1] = vecDispNormals[3] * flFracU;
	vecBlendedNormals[0] = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecBlendedNormals[0] );

	vecTmpNormals[0] = vecDispNormals[1] * ( 1.0f - flFracU );
	vecTmpNormals[1] = vecDispNormals[2] * flFracU;
	vecBlendedNormals[1] = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecBlendedNormals[1] );

	vecTmpNormals[0] = vecBlendedNormals[0] * ( 1.0f - flFracV );
	vecTmpNormals[1] = vecBlendedNormals[1] * flFracV;

	vecNormal = vecTmpNormals[0] + vecTmpNormals[1];
	VectorNormalize( vecNormal );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::ClosestBaseFaceData( Vector const &vecWorldPoint, Vector &vecPoint, 
										 Vector &vecNormal )
{
	float flMinDist = FLT_MAX;
	int iMin = -1;
	for ( int iPoint = 0; iPoint < VRAD_QUAD_SIZE; ++iPoint )
	{
		float flDist = ( vecWorldPoint - m_vecSurfPoints[iPoint] ).Length();
		if( flDist < flMinDist )
		{
			flMinDist = flDist;
			iMin = iPoint;
		}
	}

	int nWidth = GetWidth();
	int nHeight = GetHeight();

	switch( iMin )
	{
	case 0: { iMin = 0; break; }
	case 1: { iMin = ( nHeight - 1 ) * nWidth; break; }
	case 2: { iMin = ( nHeight * nWidth ) - 1; break; }
	case 3: { iMin = nWidth - 1; break; }
	default: { return; }
	}

	GetVert( iMin, vecPoint );
	GetVertNormal( iMin, vecNormal );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::InitPatch( int iPatch, int iParentPatch, bool bFirst )
{
	// Get the current patch
	patch_t *pPatch = &patches[iPatch];
	if ( pPatch )
	{
		// Clear the structure
		memset( pPatch, 0, sizeof( patch_t ) );

		// Initialize.
		pPatch->child1 = patches.InvalidIndex();
		pPatch->child2 = patches.InvalidIndex();
		pPatch->parent = iParentPatch;
		pPatch->ndxNextParent = patches.InvalidIndex();
		pPatch->ndxNextClusterChild = patches.InvalidIndex();
		pPatch->scale[0] = pPatch->scale[1] = 1.0f;
		pPatch->chop = 4;
		pPatch->sky = false;
		pPatch->winding = NULL;
		pPatch->plane = NULL;
		pPatch->origin.Init();
		pPatch->normal.Init();
		pPatch->area = 0.0f;
		pPatch->m_IterationKey = 0;

		// Get the parent patch if it exists.
		if ( iParentPatch != patches.InvalidIndex() )
		{
			patch_t *pParentPatch = &patches[iParentPatch];

			if( bFirst )
			{
				pParentPatch->child1 = iPatch;
			}
			else
			{
				pParentPatch->child2 = iPatch;
			}

			// Get data from parent.
			pPatch->faceNumber = pParentPatch->faceNumber;
			pPatch->face_mins = pParentPatch->face_mins;
			pPatch->face_maxs = pParentPatch->face_maxs;
			pPatch->reflectivity = pParentPatch->reflectivity;
			pPatch->normalMajorAxis = pParentPatch->normalMajorAxis;
		}
		else
		{
			// Set next pointers (add to lists)
			pPatch->ndxNext = facePatches.Element( m_iParent );
			facePatches[m_iParent] = iPatch;

			// Used data (for displacement surfaces)
			pPatch->faceNumber = m_iParent;

			// Set face mins/maxs -- calculated later (this is the face patch)
			pPatch->face_mins.Init( 99999.0f, 99999.0f, 99999.0f );
			pPatch->face_maxs.Init( -99999.0f, -99999.0f, -99999.0f );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakeParentPatch( int iPatch )
{
#if 0
	// Debugging!
	if ( !pDispFile )
	{
		pDispFile = g_pFileSystem->Open( "vraddisp.txt", "w" );
	}
#endif

	// Get the current patch
	patch_t *pPatch = &patches[iPatch];
	if ( pPatch )
	{
		int iStart = Nodes_CalcCount( m_nPower - 1 );
		int iEnd = Nodes_CalcCount( m_nPower );

		for ( int iNode = iStart; iNode < iEnd; ++iNode )
		{
			VNode_t *pVNode = &m_aVNodes[iNode];
			if ( pVNode )
			{
				VectorAdd( pPatch->normal, pVNode->patchNormal, pPatch->normal );
				pPatch->area += pVNode->patchArea;
				
				for ( int iAxis = 0; iAxis < 3; ++iAxis )
				{
					if( pPatch->face_mins[iAxis] > pVNode->patchBounds[0][iAxis] )
					{
						pPatch->face_mins[iAxis] = pVNode->patchBounds[0][iAxis];
					}
					
					if( pPatch->face_maxs[iAxis] < pVNode->patchBounds[1][iAxis] )
					{
						pPatch->face_maxs[iAxis] = pVNode->patchBounds[1][iAxis];
					}
				}
			}
		}
	
#if 0
		// Debugging!
		g_pFileSystem->FPrintf( pDispFile, "Parent Patch %d\n", iPatch );
		g_pFileSystem->FPrintf( pDispFile, "	Area: %lf\n", pPatch->area );
#endif

		// Set the patch bounds to face bounds (as this is the face patch)
		pPatch->mins = pPatch->face_mins;
		pPatch->maxs = pPatch->face_maxs;

		VectorNormalize( pPatch->normal );
		DispUVToSurfPoint( Vector2D( 0.5f, 0.5f ), pPatch->origin, 0.0f );		

		// Fill in the patch plane into given the normal and origin (have to alloc one - lame!)
		pPatch->plane = new dplane_t;
		if ( pPatch->plane )
		{
			pPatch->plane->normal = pPatch->normal;
			pPatch->plane->dist = pPatch->normal.Dot( pPatch->origin );
		}

		// Copy the patch origin to face_centroids for main patch
		VectorCopy( pPatch->origin, face_centroids[m_iParent] );
		VectorAdd( pPatch->origin, pPatch->normal, pPatch->origin );

		// Approximate patch winding - used for debugging!
		pPatch->winding = AllocWinding( 4 );
		if ( pPatch->winding )
		{
			pPatch->winding->numpoints = 4;
			for ( int iPoint = 0; iPoint < 4; ++iPoint )
			{
				GetParentPoint( iPoint, pPatch->winding->p[iPoint] );
			}
		}

		// Get base face normal (stab direction is base face normal)
		Vector vecFaceNormal;
		GetParentFaceNormal( vecFaceNormal );
		int nMajorAxis = 0;
		float flMajorValue = fabs( vecFaceNormal[0] );
		if( fabs( vecFaceNormal[1] ) > flMajorValue ) { nMajorAxis = 1; flMajorValue = fabs( vecFaceNormal[1] ); }
		if( fabs( vecFaceNormal[2] ) > flMajorValue ) { nMajorAxis = 2; }
		pPatch->normalMajorAxis = nMajorAxis;

		// get the base light for the face
		BaseLightForFace( &g_pFaces[m_iParent], pPatch->baselight, &pPatch->basearea, pPatch->reflectivity );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakeChildPatch( int iPatch )
{
	int	vNodeCount = 0;
	int	iVNodes[256];

	// Find all the nodes that reside behind all of the planes
	GetNodesInPatch( iPatch, iVNodes, vNodeCount );
	if( vNodeCount <= 0 )
		return false;

	// Accumulate data into current patch
	Vector2D uv( 0.0f, 0.0f );
	Vector2D uvBounds[2];
	uvBounds[0].Init( FLT_MAX, FLT_MAX );
	uvBounds[1].Init( -FLT_MAX, -FLT_MAX );

	patch_t *pPatch = &patches.Element( iPatch );
	if ( pPatch )
	{
		for ( int iNode = 0; iNode < vNodeCount; iNode++ )
		{
			VNode_t *pVNode = &m_aVNodes[iVNodes[iNode]];
			if ( pVNode )
			{
				VectorAdd( pPatch->normal, pVNode->patchNormal, pPatch->normal );
				pPatch->area += pVNode->patchArea;
				Vector2DAdd( uv, pVNode->patchOriginUV, uv );

				if ( uvBounds[0].x > pVNode->patchOriginUV.x ) { uvBounds[0].x = pVNode->patchOriginUV.x; }
				if ( uvBounds[0].y > pVNode->patchOriginUV.y ) { uvBounds[0].y = pVNode->patchOriginUV.y; }

				if ( uvBounds[1].x < pVNode->patchOriginUV.x ) { uvBounds[1].x = pVNode->patchOriginUV.x; }
				if ( uvBounds[1].y < pVNode->patchOriginUV.y ) { uvBounds[1].y = pVNode->patchOriginUV.y; }
			}
		}

		VectorNormalize( pPatch->normal );

		uv /= vNodeCount;
		DispUVToSurfPoint( uv, pPatch->origin, 1.0f );		

		// This value should be calculated based on the power of the displacement.
		for ( int i = 0; i < 2; i++ )
		{
			uvBounds[0][i] -= 0.120f;
			uvBounds[1][i] += 0.120f;
		}

		// Approximate patch winding - used for debugging!
		pPatch->winding = AllocWinding( 4 );
		if ( pPatch->winding )
		{
			pPatch->winding->numpoints = 4;
			DispUVToSurfPoint( uvBounds[0], pPatch->winding->p[0], 0.0f );
			DispUVToSurfPoint( Vector2D( uvBounds[0].x, uvBounds[1].y ), pPatch->winding->p[1], 0.0f );
			DispUVToSurfPoint( uvBounds[1], pPatch->winding->p[2], 0.0f );
			DispUVToSurfPoint( Vector2D( uvBounds[1].x, uvBounds[0].y ), pPatch->winding->p[3], 0.0f );
		}

		// Get the parent patch
		patch_t *pParentPatch = &patches.Element( pPatch->parent );
		if( pParentPatch )
		{
			// make sure the area is down by at least a little above half the
			// parent's area we will test at 30% (so we don't spin forever on 
			// weird patch center sampling problems
			float deltaArea = pParentPatch->area - pPatch->area;
			if( deltaArea < ( pParentPatch->area * 0.3 ) )
				return false;
		}

#if 0
		// debugging!
		g_pFileSystem->FPrintf( pDispFile, "Child Patch %d\n", iPatch );
		g_pFileSystem->FPrintf( pDispFile, "	Parent %d\n", pPatch->parent );
		g_pFileSystem->FPrintf( pDispFile, "	Area: %lf\n", pPatch->area );
#endif

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CVRADDispColl::MakePatch( int iPatch )
{
	patch_t *pPatch = &patches.Element( iPatch );
	if( !pPatch )
		return false;

	// Special case: the parent patch accumulates from all the child nodes
	if( pPatch->parent == patches.InvalidIndex() )
	{
		return MakeParentPatch( iPatch );
	}
	// or, accumulate the data from the child nodes that reside behind the defined planes
	else
	{
		return MakeChildPatch( iPatch );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CVRADDispColl::GetNodesInPatch( int iPatch, int *pVNodes, int &vNodeCount )
{
	// Get the current patch
	patch_t *pPatch = &patches.Element( iPatch );
	if ( !pPatch )
		return;

	// Get leaf indices (last level in tree)
	int iStart = Nodes_CalcCount( m_nPower - 1 );
	int iEnd = Nodes_CalcCount( m_nPower );

	for ( int iNode = iStart; iNode < iEnd; ++iNode )
	{
		VNode_t *pVNode = &m_aVNodes[iNode];
		if( !pVNode )
			continue;

		bool bInside = true;
		for ( int iAxis = 0; iAxis < 3 && bInside; ++iAxis )
		{
			for ( int iSide = -1; iSide < 2; iSide += 2 )
			{
				float flDist;
				if( iSide == -1 )
				{
					flDist = -pVNode->patchOrigin[iAxis] + pPatch->mins[iAxis];
				}
				else
				{
					flDist = pVNode->patchOrigin[iAxis] - pPatch->maxs[iAxis];
				}

				if( flDist > 0.0f )
				{
					bInside = false;
					break;
				}
			}
		}

		if( bInside )
		{
			pVNodes[vNodeCount] = iNode;
			vNodeCount++;
		}
	}
}
