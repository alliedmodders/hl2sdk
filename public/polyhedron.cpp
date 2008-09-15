//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "polyhedron.h"
#include <stdlib.h>
#include "utlvector.h"


struct GeneratePolyhedronFromPlanes_Point;
struct GeneratePolyhedronFromPlanes_PointLL;
struct GeneratePolyhedronFromPlanes_Line;
struct GeneratePolyhedronFromPlanes_LineLL;
struct GeneratePolyhedronFromPlanes_Polygon;
struct GeneratePolyhedronFromPlanes_PolygonLL;

struct GeneratePolyhedronFromPlanes_UnorderedPointLL;
struct GeneratePolyhedronFromPlanes_UnorderedLineLL;
struct GeneratePolyhedronFromPlanes_UnorderedPolygonLL;

Vector FindPointInPlanes( const float *pPlanes, int planeCount );
bool FindConvexShapeLooseAABB( const float *pInwardFacingPlanes, int iPlaneCount, Vector *pAABBMins, Vector *pAABBMaxs );
CPolyhedron *ClipLinkedGeometry( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory );
CPolyhedron *ConvertLinkedGeometryToPolyhedron( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, bool bUseTemporaryMemory );

#ifdef _DEBUG
//#define POLYHEDRON_EXTENSIVE_DEBUGGING //uncomment to enable extra checks that usually don't tell you much
#endif

//#define DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS //uncomment to dump every generated polyhedron to a numbered file

#ifdef DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS
#include "filesystem.h"
extern IFileSystem		*filesystem;
unsigned int g_iPolyhedronDumpCounter = 0; //increments so we write to different files
void DumpPolyhedronToGLView( const CPolyhedron *pPolyhedron, const char *pFilename );
void DumpPlaneToGlView( float *pPlane, float fGrayScale, const char *pszFileName );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPolyhedron_AllocByNew : public CPolyhedron
{
public:
	virtual void Release( void )
	{
		delete this;
	}

	virtual ~CPolyhedron_AllocByNew( void )
	{
		if( pVertices )
			delete []pVertices; //all memory for this struct is one big chunk starting at the vertices
	}
};

class CPolyhedron_TempMemory : public CPolyhedron
{
public:
#ifdef _DEBUG
	int iReferenceCount;
#endif

	virtual void Release( void )
	{
#ifdef _DEBUG
		--iReferenceCount;
#endif
	}

	CPolyhedron_TempMemory( void )
#ifdef _DEBUG
		: iReferenceCount( 0 )
#endif
	{ };
};


static CUtlVector<unsigned char> s_TempMemoryPolyhedron_Buffer;
static CPolyhedron_TempMemory s_TempMemoryPolyhedron;



struct GeneratePolyhedronFromPlanes_Point
{
	Vector ptPosition;
	GeneratePolyhedronFromPlanes_LineLL *pConnectedLines; //keep these in a clockwise order, circular linking
	float fPlaneDist; //used in plane cutting
	int iWorkData; //no specific use
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
	int iDebugFlags;
#endif
};

struct GeneratePolyhedronFromPlanes_Line
{
	GeneratePolyhedronFromPlanes_Point *pPoints[2]; //the 2 connecting points in no particular order
	GeneratePolyhedronFromPlanes_Polygon *pPolygons[2]; //viewing from the outside with the point connections going up, 0 is the left polygon, 1 is the right
	int iWorkData; //no specific use

	GeneratePolyhedronFromPlanes_LineLL *pPointLineLinks[2]; //rather than going into a point and searching for its link to this line, lets just cache it to eliminate searching
	GeneratePolyhedronFromPlanes_LineLL *pPolygonLineLinks[2]; //rather than going into a polygon and searching for its link to this line, lets just cache it to eliminate searching
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
	int iDebugFlags;
#endif
};

struct GeneratePolyhedronFromPlanes_LineLL
{
	GeneratePolyhedronFromPlanes_Line *pLine;
	int iReferenceIndex; //whatever is referencing the line should know which side of the line it's on (points and polygons), for polygons, it's which point to follow to continue going clockwise, which makes polygon 0 the one on the left side of an upward facing line vector, for points, it's the OTHER point's index
	GeneratePolyhedronFromPlanes_LineLL *pPrev;
	GeneratePolyhedronFromPlanes_LineLL *pNext;
};

struct GeneratePolyhedronFromPlanes_Polygon
{
	Vector vSurfaceNormal; 
	GeneratePolyhedronFromPlanes_LineLL *pLines; //keep these in a clockwise order, circular linking
	int iWorkData; //no specific use
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
	int iDebugFlags;
#endif
};

struct GeneratePolyhedronFromPlanes_UnorderedPolygonLL //an unordered collection polygons
{
	GeneratePolyhedronFromPlanes_Polygon *pPolygon;
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPrev;
};

struct GeneratePolyhedronFromPlanes_UnorderedLineLL //an unordered collection of lines
{
	GeneratePolyhedronFromPlanes_Line *pLine;
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pPrev;
};

struct GeneratePolyhedronFromPlanes_UnorderedPointLL //an unordered collection of points
{
	GeneratePolyhedronFromPlanes_Point *pPoint;
	GeneratePolyhedronFromPlanes_UnorderedPointLL *pNext;
	GeneratePolyhedronFromPlanes_UnorderedPointLL *pPrev;
};




CPolyhedron *ClipPolyhedron( const CPolyhedron *pExistingPolyhedron, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	if( pExistingPolyhedron == NULL )
		return NULL;

	AssertMsg( (pExistingPolyhedron->iVertexCount >= 3) && (pExistingPolyhedron->iPolygonCount >= 2), "Polyhedron doesn't meet absolute minimum spec" );

	float *pUsefulPlanes = (float *)stackalloc( sizeof( float ) * 4 * iPlaneCount );
	int iUsefulPlaneCount = 0;
	Vector *pExistingVertices = pExistingPolyhedron->pVertices;

	//A large part of clipping will either eliminate the polyhedron entirely, or clip nothing at all, so lets just check for those first and throw away useless planes
	{
		int iLiveCount = 0;
		int iDeadCount = 0;
		const float fNegativeOnPlaneEpsilon = -fOnPlaneEpsilon;

		for( int i = 0; i != iPlaneCount; ++i )
		{
			Vector vNormal = *((Vector *)&pOutwardFacingPlanes[(i * 4) + 0]);
			float fPlaneDist = pOutwardFacingPlanes[(i * 4) + 3];

			for( int j = 0; j != pExistingPolyhedron->iVertexCount; ++j )
			{
				float fPointDist = vNormal.Dot( pExistingVertices[j] ) - fPlaneDist;
				
				if( fPointDist <= fNegativeOnPlaneEpsilon )
					++iLiveCount;
				else if( fPointDist > fOnPlaneEpsilon )
					++iDeadCount;
			}

			if( iLiveCount == 0 )
			{
				//all points are dead or on the plane, so the polyhedron is dead
				return NULL;
			}

			if( iDeadCount != 0 )
			{
				//at least one point died, this plane yields useful results
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 0] = vNormal.x;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 1] = vNormal.y;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 2] = vNormal.z;
				pUsefulPlanes[(iUsefulPlaneCount * 4) + 3] = fPlaneDist;
				++iUsefulPlaneCount;
			}
		}
	}

	if( iUsefulPlaneCount == 0 )
	{
		//testing shows that the polyhedron won't even be cut, clone the existing polyhedron and return that

		CPolyhedron *pReturn;
		unsigned char *pMemory;
		if( bUseTemporaryMemory )
		{
			AssertMsg( s_TempMemoryPolyhedron.iReferenceCount == 0, "Temporary polyhedron memory being rewritten before released" );
			pReturn = &s_TempMemoryPolyhedron;
#ifdef _DEBUG
			++s_TempMemoryPolyhedron.iReferenceCount;
#endif
			s_TempMemoryPolyhedron_Buffer.SetCount( (sizeof( Vector ) * pExistingPolyhedron->iVertexCount) +
													(sizeof( Polyhedron_IndexedLine_t ) * pExistingPolyhedron->iLineCount) +
													(sizeof( Polyhedron_IndexedLineReference_t ) * pExistingPolyhedron->iIndexCount) +
													(sizeof( Polyhedron_IndexedPolygon_t ) * pExistingPolyhedron->iPolygonCount) );
			pMemory = s_TempMemoryPolyhedron_Buffer.Base();
		}
		else
		{
			pReturn = new CPolyhedron_AllocByNew;
			pMemory = new unsigned char [ (sizeof( Vector ) * pExistingPolyhedron->iVertexCount) +
											(sizeof( Polyhedron_IndexedLine_t ) * pExistingPolyhedron->iLineCount) +
											(sizeof( Polyhedron_IndexedLineReference_t ) * pExistingPolyhedron->iIndexCount) +
											(sizeof( Polyhedron_IndexedPolygon_t ) * pExistingPolyhedron->iPolygonCount) ];
		}		

		pReturn->iVertexCount = pExistingPolyhedron->iVertexCount;
		pReturn->iLineCount = pExistingPolyhedron->iLineCount;
		pReturn->iIndexCount = pExistingPolyhedron->iIndexCount;
		pReturn->iPolygonCount = pExistingPolyhedron->iPolygonCount;
		
		pReturn->pVertices = (Vector *)pMemory;
		pReturn->pLines = (Polyhedron_IndexedLine_t *)(&pReturn->pVertices[pReturn->iVertexCount]);
		pReturn->pIndices = (Polyhedron_IndexedLineReference_t *)(&pReturn->pLines[pReturn->iLineCount]);
		pReturn->pPolygons = (Polyhedron_IndexedPolygon_t *)(&pReturn->pIndices[pReturn->iIndexCount]);

		memcpy( pReturn->pVertices, pExistingPolyhedron->pVertices, sizeof( Vector ) * pReturn->iVertexCount );
		memcpy( pReturn->pLines, pExistingPolyhedron->pLines, sizeof( Polyhedron_IndexedLine_t ) * pReturn->iLineCount );
		memcpy( pReturn->pIndices, pExistingPolyhedron->pIndices, sizeof( Polyhedron_IndexedLineReference_t ) * pReturn->iIndexCount );
		memcpy( pReturn->pPolygons, pExistingPolyhedron->pPolygons, sizeof( Polyhedron_IndexedPolygon_t ) * pReturn->iPolygonCount );

		return pReturn;
	}



	//convert the polyhedron to linked geometry
	GeneratePolyhedronFromPlanes_Point *pStartPoints = (GeneratePolyhedronFromPlanes_Point *)stackalloc( pExistingPolyhedron->iVertexCount * sizeof( GeneratePolyhedronFromPlanes_Point ) );
	GeneratePolyhedronFromPlanes_Line *pStartLines = (GeneratePolyhedronFromPlanes_Line *)stackalloc( pExistingPolyhedron->iLineCount * sizeof( GeneratePolyhedronFromPlanes_Line ) );
	GeneratePolyhedronFromPlanes_Polygon *pStartPolygons = (GeneratePolyhedronFromPlanes_Polygon *)stackalloc( pExistingPolyhedron->iPolygonCount * sizeof( GeneratePolyhedronFromPlanes_Polygon ) );

	GeneratePolyhedronFromPlanes_LineLL *pStartLineLinks = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( pExistingPolyhedron->iLineCount * 4 * sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
	
	int iCurrentLineLinkIndex = 0;

	//setup points
	for( int i = 0; i != pExistingPolyhedron->iVertexCount; ++i )
	{
		pStartPoints[i].ptPosition = pExistingPolyhedron->pVertices[i];
		pStartPoints[i].pConnectedLines = NULL; //we won't be circular linking until later
	}

	//setup lines and interlink to points (line links are not yet circularly linked, and are unordered)
	for( int i = 0; i != pExistingPolyhedron->iLineCount; ++i )
	{
		for( int j = 0; j != 2; ++j )
		{
			pStartLines[i].pPoints[j] = &pStartPoints[pExistingPolyhedron->pLines[i].iPointIndices[j]];

			GeneratePolyhedronFromPlanes_LineLL *pLineLink = &pStartLineLinks[iCurrentLineLinkIndex++];
			pStartLines[i].pPointLineLinks[j] = pLineLink;
			pLineLink->pLine = &pStartLines[i];
			pLineLink->iReferenceIndex = 1 - j;
			//pLineLink->pPrev = NULL;
			pLineLink->pNext = pStartLines[i].pPoints[j]->pConnectedLines;
			pStartLines[i].pPoints[j]->pConnectedLines = pLineLink;
		}
	}



	//setup polygons
	for( int i = 0; i != pExistingPolyhedron->iPolygonCount; ++i )
	{
		pStartPolygons[i].vSurfaceNormal = pExistingPolyhedron->pPolygons[i].polyNormal;
		Polyhedron_IndexedLineReference_t *pOffsetPolyhedronLines = &pExistingPolyhedron->pIndices[pExistingPolyhedron->pPolygons[i].iFirstIndex];

		
		GeneratePolyhedronFromPlanes_LineLL *pFirstLink = &pStartLineLinks[iCurrentLineLinkIndex];
		pStartPolygons[i].pLines = pFirstLink; //technically going to link to itself on first pass, then get linked properly immediately afterward
		for( int j = 0; j != pExistingPolyhedron->pPolygons[i].iIndexCount; ++j )
		{
			GeneratePolyhedronFromPlanes_LineLL *pLineLink = &pStartLineLinks[iCurrentLineLinkIndex++];
			pLineLink->pLine = &pStartLines[pOffsetPolyhedronLines[j].iLineIndex];
			pLineLink->iReferenceIndex = pOffsetPolyhedronLines[j].iEndPointIndex;
			
			pLineLink->pLine->pPolygons[pLineLink->iReferenceIndex] = &pStartPolygons[i];
			pLineLink->pLine->pPolygonLineLinks[pLineLink->iReferenceIndex] = pLineLink;			

			pLineLink->pPrev = pStartPolygons[i].pLines;
			pStartPolygons[i].pLines->pNext = pLineLink;
			pStartPolygons[i].pLines = pLineLink;
		}
		
		pFirstLink->pPrev = pStartPolygons[i].pLines;
		pStartPolygons[i].pLines->pNext = pFirstLink;
	}

	Assert( iCurrentLineLinkIndex == (pExistingPolyhedron->iLineCount * 4) );

	//go back to point line links so we can circularly link them as well as order them now that every point has all its line links
	for( int i = 0; i != pExistingPolyhedron->iVertexCount; ++i )
	{
		//interlink the points
		{
			GeneratePolyhedronFromPlanes_LineLL *pLastVisitedLink = pStartPoints[i].pConnectedLines;
			GeneratePolyhedronFromPlanes_LineLL *pCurrentLink = pLastVisitedLink;
			
			do
			{
				pCurrentLink->pPrev = pLastVisitedLink;
				pLastVisitedLink = pCurrentLink;
				pCurrentLink = pCurrentLink->pNext;
			} while( pCurrentLink );

			//circular link
			pLastVisitedLink->pNext = pStartPoints[i].pConnectedLines;
			pStartPoints[i].pConnectedLines->pPrev = pLastVisitedLink;
		}


		//fix ordering
		GeneratePolyhedronFromPlanes_LineLL *pFirstLink = pStartPoints[i].pConnectedLines;
		GeneratePolyhedronFromPlanes_LineLL *pWorkLink = pFirstLink;
		GeneratePolyhedronFromPlanes_LineLL *pSearchLink;
		GeneratePolyhedronFromPlanes_Polygon *pLookingForPolygon;
		Assert( pFirstLink->pNext != pFirstLink );
		do
		{
			pLookingForPolygon = pWorkLink->pLine->pPolygons[1 - pWorkLink->iReferenceIndex]; //grab pointer to left polygon
			pSearchLink = pWorkLink->pPrev;

			while( pSearchLink->pLine->pPolygons[pSearchLink->iReferenceIndex] != pLookingForPolygon )
				pSearchLink = pSearchLink->pPrev;

			Assert( pSearchLink->pLine->pPolygons[pSearchLink->iReferenceIndex] == pWorkLink->pLine->pPolygons[1 - pWorkLink->iReferenceIndex] );

			//pluck the search link from wherever it is
			pSearchLink->pPrev->pNext = pSearchLink->pNext;
			pSearchLink->pNext->pPrev = pSearchLink->pPrev;

			//insert the search link just before the work link			
			pSearchLink->pPrev = pWorkLink->pPrev;
			pSearchLink->pNext = pWorkLink;
			
			pSearchLink->pPrev->pNext = pSearchLink;
			pWorkLink->pPrev = pSearchLink;

			pWorkLink = pSearchLink;
		} while( pWorkLink != pFirstLink );
	}

	GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints = (GeneratePolyhedronFromPlanes_UnorderedPointLL *)stackalloc( pExistingPolyhedron->iVertexCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedPointLL ) );
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines = (GeneratePolyhedronFromPlanes_UnorderedLineLL *)stackalloc( pExistingPolyhedron->iLineCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedLineLL ) );
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons = (GeneratePolyhedronFromPlanes_UnorderedPolygonLL *)stackalloc( pExistingPolyhedron->iPolygonCount * sizeof( GeneratePolyhedronFromPlanes_UnorderedPolygonLL ) );

	//setup point collection
	{
		pPoints[0].pPrev = NULL;
		pPoints[0].pPoint = &pStartPoints[0];
		pPoints[0].pNext = &pPoints[1];
		int iLastPoint = pExistingPolyhedron->iVertexCount - 1;
		for( int i = 1; i != iLastPoint; ++i )
		{
			pPoints[i].pPrev = &pPoints[i - 1];
			pPoints[i].pPoint = &pStartPoints[i];
			pPoints[i].pNext = &pPoints[i + 1];
		}
		pPoints[iLastPoint].pPrev = &pPoints[iLastPoint - 1];
		pPoints[iLastPoint].pPoint = &pStartPoints[iLastPoint];
		pPoints[iLastPoint].pNext = NULL;
	}

	//setup line collection
	{
		pLines[0].pPrev = NULL;
		pLines[0].pLine = &pStartLines[0];
		pLines[0].pNext = &pLines[1];
		int iLastLine = pExistingPolyhedron->iLineCount - 1;
		for( int i = 1; i != iLastLine; ++i )
		{
			pLines[i].pPrev = &pLines[i - 1];
			pLines[i].pLine = &pStartLines[i];
			pLines[i].pNext = &pLines[i + 1];
		}
		pLines[iLastLine].pPrev = &pLines[iLastLine - 1];
		pLines[iLastLine].pLine = &pStartLines[iLastLine];
		pLines[iLastLine].pNext = NULL;
	}

	//setup polygon collection
	{
		pPolygons[0].pPrev = NULL;
		pPolygons[0].pPolygon = &pStartPolygons[0];
		pPolygons[0].pNext = &pPolygons[1];
		int iLastPolygon = pExistingPolyhedron->iPolygonCount - 1;
		for( int i = 1; i != iLastPolygon; ++i )
		{
			pPolygons[i].pPrev = &pPolygons[i - 1];
			pPolygons[i].pPolygon = &pStartPolygons[i];
			pPolygons[i].pNext = &pPolygons[i + 1];
		}
		pPolygons[iLastPolygon].pPrev = &pPolygons[iLastPolygon - 1];
		pPolygons[iLastPolygon].pPolygon = &pStartPolygons[iLastPolygon];
		pPolygons[iLastPolygon].pNext = NULL;
	}

	return ClipLinkedGeometry( pPolygons, pLines, pPoints, pUsefulPlanes, iUsefulPlaneCount, fOnPlaneEpsilon, bUseTemporaryMemory );
}



Vector FindPointInPlanes( const float *pPlanes, int planeCount )
{
	Vector point = vec3_origin;

	for ( int i = 0; i < planeCount; i++ )
	{
		float fD = DotProduct( *(Vector *)&pPlanes[i*4], point ) - pPlanes[i*4 + 3];
		if ( fD < 0 )
		{
			point -= fD * (*(Vector *)&pPlanes[i*4]);
		}
	}
	return point;
}



bool FindConvexShapeLooseAABB( const float *pInwardFacingPlanes, int iPlaneCount, Vector *pAABBMins, Vector *pAABBMaxs ) //bounding box of the convex shape (subject to floating point error)
{
	//returns false if the AABB hasn't been set
	if( pAABBMins == NULL && pAABBMaxs == NULL ) //no use in actually finding out what it is
		return false;

	struct FindConvexShapeAABB_Polygon_t
	{
		float *verts;
		int iVertCount;
	};

	float *pMovedPlanes = (float *)stackalloc( iPlaneCount * 4 * sizeof( float ) );
	//Vector vPointInPlanes = FindPointInPlanes( pInwardFacingPlanes, iPlaneCount );

	for( int i = 0; i != iPlaneCount; ++i )
	{
		pMovedPlanes[(i * 4) + 0] = pInwardFacingPlanes[(i * 4) + 0];
		pMovedPlanes[(i * 4) + 1] = pInwardFacingPlanes[(i * 4) + 1];
		pMovedPlanes[(i * 4) + 2] = pInwardFacingPlanes[(i * 4) + 2];
		pMovedPlanes[(i * 4) + 3] = pInwardFacingPlanes[(i * 4) + 3] - 100.0f; //move planes out a lot to kill some imprecision problems
	}
	
	

	//vAABBMins = vAABBMaxs = FindPointInPlanes( pPlanes, iPlaneCount );
	float *vertsIn = NULL; //we'll be allocating a new buffer for this with each new polygon, and moving it off to the polygon array
	float *vertsOut = (float *)stackalloc( (iPlaneCount + 4) * (sizeof( float ) * 3) ); //each plane will initially have 4 points in its polygon representation, and each plane clip has the possibility to add 1 point to the polygon
	float *vertsSwap;

	FindConvexShapeAABB_Polygon_t *pPolygons = (FindConvexShapeAABB_Polygon_t *)stackalloc( iPlaneCount * sizeof( FindConvexShapeAABB_Polygon_t ) );
	int iPolyCount = 0;

	for ( int i = 0; i < iPlaneCount; i++ )
	{
		Vector *pPlaneNormal = (Vector *)&pInwardFacingPlanes[i*4];
		float fPlaneDist = pInwardFacingPlanes[(i*4) + 3];

		if( vertsIn == NULL )
			vertsIn = (float *)stackalloc( (iPlaneCount + 4) * (sizeof( float ) * 3) );

		// Build a big-ass poly in this plane
		int vertCount = PolyFromPlane( (Vector *)vertsIn, *pPlaneNormal, fPlaneDist, 100000.0f );

		//chop it by every other plane
		for( int j = 0; j < iPlaneCount; j++ )
		{
			// don't clip planes with themselves
			if ( i == j )
				continue;

			// Chop the polygon against this plane
			vertCount = ClipPolyToPlane( (Vector *)vertsIn, vertCount, (Vector *)vertsOut, *(Vector *)&pMovedPlanes[j*4], pMovedPlanes[(j*4) + 3], 0.0f );

			//swap the input and output arrays
			vertsSwap = vertsIn; vertsIn = vertsOut; vertsOut = vertsSwap;

			// Less than a poly left, something's wrong, don't bother with this polygon
			if ( vertCount < 3 )
				break;
		}

		if ( vertCount < 3 )
			continue; //not enough to work with

		pPolygons[iPolyCount].iVertCount = vertCount;
		pPolygons[iPolyCount].verts = vertsIn;
		vertsIn = NULL;
		++iPolyCount;
	}

	if( iPolyCount == 0 )
		return false;

	//initialize the AABB to the first point available
	Vector vAABBMins, vAABBMaxs;
	vAABBMins = vAABBMaxs = ((Vector *)pPolygons[0].verts)[0];

	if( pAABBMins && pAABBMaxs ) //they want the full box
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x < vAABBMins.x ) 
					vAABBMins.x = PolyVerts[j].x;
				if( PolyVerts[j].y < vAABBMins.y ) 
					vAABBMins.y = PolyVerts[j].y;
				if( PolyVerts[j].z < vAABBMins.z ) 
					vAABBMins.z = PolyVerts[j].z;

				if( PolyVerts[j].x > vAABBMaxs.x ) 
					vAABBMaxs.x = PolyVerts[j].x;
				if( PolyVerts[j].y > vAABBMaxs.y ) 
					vAABBMaxs.y = PolyVerts[j].y;
				if( PolyVerts[j].z > vAABBMaxs.z ) 
					vAABBMaxs.z = PolyVerts[j].z;
			}
		}
		*pAABBMins = vAABBMins;
		*pAABBMaxs = vAABBMaxs;
	}
	else if( pAABBMins ) //they only want the min
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x < vAABBMins.x ) 
					vAABBMins.x = PolyVerts[j].x;
				if( PolyVerts[j].y < vAABBMins.y ) 
					vAABBMins.y = PolyVerts[j].y;
				if( PolyVerts[j].z < vAABBMins.z ) 
					vAABBMins.z = PolyVerts[j].z;
			}
		}
		*pAABBMins = vAABBMins;
	}
	else //they only want the max
	{
		for( int i = 0; i != iPolyCount; ++i )
		{
			Vector *PolyVerts = (Vector *)pPolygons[i].verts;
			for( int j = 0; j != pPolygons[i].iVertCount; ++j )
			{
				if( PolyVerts[j].x > vAABBMaxs.x ) 
					vAABBMaxs.x = PolyVerts[j].x;
				if( PolyVerts[j].y > vAABBMaxs.y ) 
					vAABBMaxs.y = PolyVerts[j].y;
				if( PolyVerts[j].z > vAABBMaxs.z ) 
					vAABBMaxs.z = PolyVerts[j].z;
			}
		}
		*pAABBMaxs = vAABBMaxs;
	}

	return true;
}







CPolyhedron *ConvertLinkedGeometryToPolyhedron( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, bool bUseTemporaryMemory )
{
	Assert( (pPolygons != NULL) && (pLines != NULL) && (pPoints != NULL) );
	int iPolyCount = 0, iLineCount = 0, iPointCount = 0, iIndexCount = 0;

	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pPolygons;	
	do
	{
		++iPolyCount;
		GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pActivePolygonWalk->pPolygon->pLines;
		GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
		Assert( pLineWalk != NULL );
		
		do
		{
			++iIndexCount;
			pLineWalk = pLineWalk->pNext;
		} while( pLineWalk != pFirstLine );

		pActivePolygonWalk = pActivePolygonWalk->pNext;
	} while( pActivePolygonWalk );

	GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pLines;
	do
	{
		++iLineCount;
		pActiveLineWalk = pActiveLineWalk->pNext;
	} while( pActiveLineWalk );

	GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pPoints;
	do
	{
		++iPointCount;
		pActivePointWalk = pActivePointWalk->pNext;
	} while( pActivePointWalk );	
	
	CPolyhedron *pReturn;
	unsigned char *pMemory;
	if( bUseTemporaryMemory )
	{
		AssertMsg( s_TempMemoryPolyhedron.iReferenceCount == 0, "Temporary polyhedron memory being rewritten before released" );
		pReturn = &s_TempMemoryPolyhedron;
#ifdef _DEBUG
		++s_TempMemoryPolyhedron.iReferenceCount;
#endif
		s_TempMemoryPolyhedron_Buffer.SetCount( (sizeof( Vector ) * iPointCount) +
												(sizeof( Polyhedron_IndexedLine_t ) * iLineCount) +
												(sizeof( Polyhedron_IndexedLineReference_t ) * iIndexCount) +
												(sizeof( Polyhedron_IndexedPolygon_t ) * iPolyCount) );
		pMemory = s_TempMemoryPolyhedron_Buffer.Base();
	}
	else
	{
		pReturn = new CPolyhedron_AllocByNew;
		pMemory = new unsigned char [ (sizeof( Vector ) * iPointCount) +
										(sizeof( Polyhedron_IndexedLine_t ) * iLineCount) +
										(sizeof( Polyhedron_IndexedLineReference_t ) * iIndexCount) +
										(sizeof( Polyhedron_IndexedPolygon_t ) * iPolyCount) ];
	}

	pReturn->iVertexCount = iPointCount;
	pReturn->iLineCount = iLineCount;
	pReturn->iIndexCount = iIndexCount;
	pReturn->iPolygonCount = iPolyCount;

	Vector *pVertexArray = pReturn->pVertices = (Vector *)pMemory;
	Polyhedron_IndexedLine_t *pLineArray = pReturn->pLines = (Polyhedron_IndexedLine_t *)(&pVertexArray[iPointCount]);
	Polyhedron_IndexedLineReference_t *pIndexArray = pReturn->pIndices = (Polyhedron_IndexedLineReference_t *)(&pLineArray[iLineCount]);
	Polyhedron_IndexedPolygon_t *pPolyArray = pReturn->pPolygons = (Polyhedron_IndexedPolygon_t *)(&pIndexArray[iIndexCount]);

	//copy points
	pActivePointWalk = pPoints;
	for( int i = 0; i != iPointCount; ++i )
	{
		pVertexArray[i] = pActivePointWalk->pPoint->ptPosition;
		pActivePointWalk->pPoint->iWorkData = i; //storing array indices
		pActivePointWalk = pActivePointWalk->pNext;
	}

	//copy lines
	pActiveLineWalk = pLines;
	for( int i = 0; i != iLineCount; ++i )
	{
		pLineArray[i].iPointIndices[0] = (unsigned short)pActiveLineWalk->pLine->pPoints[0]->iWorkData;
		pLineArray[i].iPointIndices[1] = (unsigned short)pActiveLineWalk->pLine->pPoints[1]->iWorkData;

		pActiveLineWalk->pLine->iWorkData = i; //storing array indices

		pActiveLineWalk = pActiveLineWalk->pNext;
	}

	//copy polygons and indices at the same time
	pActivePolygonWalk = pPolygons;
	iIndexCount = 0;
	for( int i = 0; i != iPolyCount; ++i )
	{
		pPolyArray[i].polyNormal = pActivePolygonWalk->pPolygon->vSurfaceNormal;
		pPolyArray[i].iFirstIndex = iIndexCount;
		
		
		GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pActivePolygonWalk->pPolygon->pLines;
		GeneratePolyhedronFromPlanes_LineLL *pFirstLine = pLineWalk;
		do
		{
			//pIndexArray[iIndexCount] = pLineWalk->pLine->pPoints[pLineWalk->iReferenceIndex]->iWorkData; //startpoint of each line, iWorkData is the index of the vertex
			pIndexArray[iIndexCount].iLineIndex = pLineWalk->pLine->iWorkData;
			pIndexArray[iIndexCount].iEndPointIndex = pLineWalk->iReferenceIndex;
			
			++iIndexCount;
			pLineWalk = pLineWalk->pNext;
		} while( pLineWalk != pFirstLine );
		
		


		pPolyArray[i].iIndexCount = iIndexCount - pPolyArray[i].iFirstIndex;

		pActivePolygonWalk = pActivePolygonWalk->pNext;	
	}

#ifdef DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS
	/*if( szDumpFile )
	{
		DumpPolyhedronToGLView( pReturn, szDumpFile );
		pReturn->Release();
		return NULL;
	}
	else
	{*/
		char szCollisionFile[128];
		filesystem->CreateDirHierarchy( "PolyhedronDumps" );
		Q_snprintf( szCollisionFile, 128, "PolyhedronDumps/NewStyle_PolyhedronDump%i.txt", g_iPolyhedronDumpCounter );
		++g_iPolyhedronDumpCounter;

		filesystem->RemoveFile( szCollisionFile );
		DumpPolyhedronToGLView( pReturn, szCollisionFile );
		DumpPolyhedronToGLView( pReturn, "PolyhedronDumps/NewStyle_PolyhedronDump_All-Appended.txt" );
	//}
#endif

	return pReturn;
}



CPolyhedron *ClipLinkedGeometry( GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pPolygons, GeneratePolyhedronFromPlanes_UnorderedLineLL *pLines, GeneratePolyhedronFromPlanes_UnorderedPointLL *pPoints, const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	const float fNegativeOnPlaneEpsilon = -fOnPlaneEpsilon;

	//clear out line work variables
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pLines;
	do
	{
		pActiveLineWalk->pLine->iWorkData = 0;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
		pActiveLineWalk->pLine->iDebugFlags = 0;
#endif
		pActiveLineWalk = pActiveLineWalk->pNext;
	} while( pActiveLineWalk );

	//clear out polygon work variables
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pPolygons;
	do
	{
		pActivePolygonWalk->pPolygon->iWorkData = 0;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
		pActivePolygonWalk->pPolygon->iDebugFlags = 0;
#endif
		pActivePolygonWalk = pActivePolygonWalk->pNext;
	} while( pActivePolygonWalk );



	GeneratePolyhedronFromPlanes_UnorderedPointLL *pDeadPoints = NULL;
	GeneratePolyhedronFromPlanes_UnorderedPointLL *pOnPlanePoints = NULL;
	GeneratePolyhedronFromPlanes_UnorderedLineLL *pDeadLines = NULL;
	//GeneratePolyhedronFromPlanes_UnorderedPointLL *pPointLL_Pool = NULL; //unused pointers that can be reused, pPrev pointers unmaintained

#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
	/*char szDumpBase[128];
	char szDumpIterate[128];


	if( g_bOutputEveryPolyhedronClip )
	{
		Q_snprintf( szDumpBase, 128, "PolyhedronDumps/PerClips-%i-", g_iOutputEveryPolyhedronClip_Prefix );
		Q_snprintf( szDumpIterate, 128, "%sbasecube.txt", szDumpBase );
	
		ConvertLinkedGeometryToPolyhedron( pPolygons, pLines, pPoints, szDumpIterate );
	}*/
#endif




	for( int i = 0; i != iPlaneCount; ++i )
	{
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
		//clear out line work variables
		GeneratePolyhedronFromPlanes_UnorderedLineLL *pActiveLineWalk = pLines;
		do
		{
			pActiveLineWalk->pLine->iWorkData = 0;
			pActiveLineWalk = pActiveLineWalk->pNext;
		} while( pActiveLineWalk );
#endif

		
		/*if( g_bOutputEveryPolyhedronClip )
		{
			DumpPlaneToGlView( &pInwardFacingPlanes[i * 4], 1.0f, szDumpIterate );

			Q_snprintf( szDumpIterate, 128, "%s%i.txt", szDumpBase, i );

			DumpPlaneToGlView( &pInwardFacingPlanes[i * 4], 0.2f, szDumpIterate );
		}*/
		
		pDeadPoints = NULL; //TODO: Move these points into a reallocation pool
		pDeadLines = NULL; //TODO: Move these lines into a reallocation pool
		pOnPlanePoints = NULL; //TODO: Move these point pointers into a reallocation pool

		Vector vNormal = *((Vector *)&pOutwardFacingPlanes[(i * 4) + 0]);
		float fPlaneDist = pOutwardFacingPlanes[(i * 4) + 3];

		int iLivePointCount = 0; //if every point is dead or split, then we can just return NULL
		//find point distances from the plane
		GeneratePolyhedronFromPlanes_UnorderedPointLL *pActivePointWalk = pPoints;
		do
		{
			GeneratePolyhedronFromPlanes_Point *pPoint = pActivePointWalk->pPoint;
			float fPointDist = pPoint->fPlaneDist = vNormal.Dot( pPoint->ptPosition ) - fPlaneDist;
			if( fPointDist > fOnPlaneEpsilon )
			{
				pPoint->iWorkData = 2; //point is dead, bang bang

				//remove the point from active points and move it to dead points list
				GeneratePolyhedronFromPlanes_UnorderedPointLL *pThisPoint = pActivePointWalk;
				pActivePointWalk = pActivePointWalk->pNext;
				
				if( pThisPoint->pPrev )
					pThisPoint->pPrev->pNext = pActivePointWalk;
				else //at the head of the active point list
					pPoints = pActivePointWalk;

				if( pActivePointWalk )
					pActivePointWalk->pPrev = pThisPoint->pPrev;
				
				pThisPoint->pNext = pDeadPoints;
				if( pDeadPoints )
					pDeadPoints->pPrev = pThisPoint;

				pDeadPoints = pThisPoint;
				pThisPoint->pPrev = NULL;

				continue;
			}
			else if( fPointDist <= fNegativeOnPlaneEpsilon )
			{
				pPoint->iWorkData = 0; //point is in behind plane, not voted off the island....yet
				++iLivePointCount;
			}
			else
			{
				pPoint->iWorkData = 1; //point is on the plane, he's everyone's buddy
				
				//add to on-plane points so we can remove lines that exist wholly on the plane (because it's easier to just re-add it than it is to work around it
				GeneratePolyhedronFromPlanes_UnorderedPointLL *pNewLinkedListNode = (GeneratePolyhedronFromPlanes_UnorderedPointLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedPointLL ) );
				pNewLinkedListNode->pPoint = pPoint;
				pNewLinkedListNode->pPrev = NULL;
				pNewLinkedListNode->pNext = pOnPlanePoints;
				if( pOnPlanePoints )
					pOnPlanePoints->pPrev = pNewLinkedListNode;
				pOnPlanePoints = pNewLinkedListNode;
			}

			pActivePointWalk = pActivePointWalk->pNext;
		} while( pActivePointWalk );

		if( iLivePointCount == 0 )
			return NULL; //all the points either died or are on the plane, no polyhedron left at all

		if( pDeadPoints == NULL )
		{
			//if( g_bOutputEveryPolyhedronClip )
			//	ConvertLinkedGeometryToPolyhedron( pPolygons, pLines, pPoints, szDumpIterate );
			continue; //no cuts made
		}

		//scan for cut lines
		GeneratePolyhedronFromPlanes_UnorderedPointLL *pDeadPointWalk = pDeadPoints;
		do
		{
			AssertMsg( pDeadPointWalk->pPoint->iWorkData == 2, "Dead point not dead" );
			GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pDeadPointWalk->pPoint->pConnectedLines;
			bool bSkipWhileCheck = false;
			if( pLineWalk != NULL )
			{
				do
				{
					bSkipWhileCheck = false;
					GeneratePolyhedronFromPlanes_Line *pLine = pLineWalk->pLine;
					AssertMsg( (pLine->pPoints[1 - pLineWalk->iReferenceIndex] == pDeadPointWalk->pPoint) && (pLine->pPoints[pLineWalk->iReferenceIndex] != pDeadPointWalk->pPoint), "Line pointers corrupt or out of order" );

					int iReferenceIndex = pLineWalk->iReferenceIndex;
					if( pLine->pPoints[iReferenceIndex]->iWorkData == 0 )
					{
						int iDeadIndex = 1 - iReferenceIndex;
						//dead point connected to a live point, cut this line
						AssertMsg( pLine->pPoints[iDeadIndex]->iWorkData == 2, "Dead indexed point not dead" );

						pLine->iWorkData = 1;

						//have the polygon who goes clockwise into the dead point have it's line list point at this cut line first (makes it easier to relink polygons later)
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
						GeneratePolyhedronFromPlanes_LineLL *pOriginalPolyLineHead = pLine->pPolygons[iDeadIndex]->pLines;
#endif

						pLine->pPolygons[iDeadIndex]->pLines = pLine->pPolygonLineLinks[iDeadIndex];

#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
						pLine->pPolygons[iDeadIndex]->iDebugFlags |= (1<<2);
						
						{
							bool bFound = false;
							GeneratePolyhedronFromPlanes_LineLL *pTestLines = pLine->pPolygons[iDeadIndex]->pLines;
							GeneratePolyhedronFromPlanes_LineLL *pFirstTestLine = pTestLines;
							do
							{
								if( pTestLines == pOriginalPolyLineHead )
								{
									bFound = true;
									break;
								}
								pTestLines = pTestLines->pNext;
							} while( pTestLines != pFirstTestLine );

							AssertMsg( bFound, "Head polygon line link not found along polygon edge" );
						}
#endif
						//mark connected polygons as missing a side
						for( int j = 0; j != 2; ++j )
						{
							pLine->pPolygons[j]->iWorkData = 1;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
							pLine->pPolygons[j]->iDebugFlags |= (1<<0);
#endif
						}
					}
					else if( pLine->iWorkData != 2 ) //not already removed
					{
						//dead point connected to either another dead point or an on-plane point, this line is dead
						pLine->iWorkData = 2;

						//remove this line from connected points
						for( int j = 0; j != 2; ++j )
						{
							GeneratePolyhedronFromPlanes_Point *pPointToRemoveFrom = pLine->pPoints[j];
							GeneratePolyhedronFromPlanes_LineLL *pRemoveLine = pLine->pPointLineLinks[j];

							AssertMsg( (pRemoveLine->pLine->pPoints[0]->iWorkData != 0) && (pRemoveLine->pLine->pPoints[1]->iWorkData != 0), "Live line being removed from points" );

							//Assert( pRemoveLine != pRemoveLine->pNext );
							if( pRemoveLine != pRemoveLine->pNext )
							{
								pRemoveLine->pNext->pPrev = pRemoveLine->pPrev;
								pRemoveLine->pPrev->pNext = pRemoveLine->pNext;
								pPointToRemoveFrom->pConnectedLines = pRemoveLine->pNext;
							}
							else
							{
								pPointToRemoveFrom->pConnectedLines = NULL;
							}
							bSkipWhileCheck = true;
						}

						//remove this line from connected polygons
						for( int j = 0; j != 2; ++j )
						{
							GeneratePolyhedronFromPlanes_Polygon *pPolygonToRemoveFrom = pLine->pPolygons[j];
							GeneratePolyhedronFromPlanes_LineLL *pRemoveLine = pLine->pPolygonLineLinks[j];

							AssertMsg( (pRemoveLine->pLine->pPoints[0]->iWorkData != 0) && (pRemoveLine->pLine->pPoints[1]->iWorkData != 0), "Live line being removed from polygon" );

							pPolygonToRemoveFrom->iWorkData = 1; //mark the polygon as missing a side

#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
							pPolygonToRemoveFrom->iDebugFlags |= (1<<1);
							pLine->pPolygons[iReferenceIndex]->iDebugFlags &= ~(1<<2);
#endif						

							
							pRemoveLine->pNext->pPrev = pRemoveLine->pPrev;
							pRemoveLine->pPrev->pNext = pRemoveLine->pNext;

							pPolygonToRemoveFrom->pLines = pRemoveLine->pPrev;
							
							if( pPolygonToRemoveFrom->pLines == pPolygonToRemoveFrom->pLines->pNext )
							{
								//only one line left, kill the whole polygon
								GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pPolygons;
								do
								{
									if( pActivePolygonWalk->pPolygon == pPolygonToRemoveFrom )
									{
										//link over the polygon in the list
										if( pActivePolygonWalk->pPrev )
											pActivePolygonWalk->pPrev->pNext = pActivePolygonWalk->pNext;
										else //at the head of the polygon list
											pPolygons = pActivePolygonWalk->pNext;

										if( pActivePolygonWalk->pNext )
											pActivePolygonWalk->pNext->pPrev = pActivePolygonWalk->pPrev;

										//TODO: add this polygon's liberated pointers to dead pools for re-use
										break;
									}
									pActivePolygonWalk = pActivePolygonWalk->pNext;
								} while( pActivePolygonWalk );
							}
							
							//TODO: add this line's liberated pointers to dead pools for re-use
						}

						//TODO: add this line's liberated pointers to dead pools for re-use
					}
					pLineWalk = pLineWalk->pNext;
				} while( bSkipWhileCheck || ((pDeadPointWalk->pPoint->pConnectedLines != NULL) && (pLineWalk != pDeadPointWalk->pPoint->pConnectedLines)) );
			}

			pDeadPointWalk = pDeadPointWalk->pNext;
		} while( pDeadPointWalk );


		//scan for lines that exist entirely on the cutting plane and remove them (it's easier to remove and readd than to work around it)
		GeneratePolyhedronFromPlanes_UnorderedPointLL *pOnPlanePointWalk = pOnPlanePoints;
		while( pOnPlanePointWalk )
		{
			AssertMsg( pOnPlanePointWalk->pPoint->iWorkData == 1, "Point that's supposed to be on the cut plane, not on the cut plane" );
			GeneratePolyhedronFromPlanes_LineLL *pLineWalk = pOnPlanePointWalk->pPoint->pConnectedLines;
			bool bSkipWhileCheck = false; //in case the first line changes
			if( pLineWalk )
			{
				do
				{
					bSkipWhileCheck = false;
					GeneratePolyhedronFromPlanes_Line *pLine = pLineWalk->pLine;
					Assert( (pLine->pPoints[1 - pLineWalk->iReferenceIndex] == pOnPlanePointWalk->pPoint) && (pLine->pPoints[pLineWalk->iReferenceIndex] != pOnPlanePointWalk->pPoint) );

					int iReferenceIndex = pLineWalk->iReferenceIndex;
					
					if( (pLine->pPoints[iReferenceIndex]->iWorkData == 1) && (pLine->iWorkData != 2) )
					{
						//on-plane point connected to on-plane point, kill it
						pLine->iWorkData = 2;

						//remove this line from connected points
						for( int j = 0; j != 2; ++j )
						{
							GeneratePolyhedronFromPlanes_Point *pPointToRemoveFrom = pLine->pPoints[j];
							GeneratePolyhedronFromPlanes_LineLL *pRemoveLine = pLine->pPointLineLinks[j];

							AssertMsg( (pRemoveLine->pLine->pPoints[0]->iWorkData != 0) && (pRemoveLine->pLine->pPoints[1]->iWorkData != 0), "Live line about to be removed from points" );

							//Assert( pRemoveLine != pRemoveLine->pNext );
							if( pRemoveLine != pRemoveLine->pNext )
							{
								pRemoveLine->pNext->pPrev = pRemoveLine->pPrev;
								pRemoveLine->pPrev->pNext = pRemoveLine->pNext;
								pPointToRemoveFrom->pConnectedLines = pRemoveLine->pNext;
							}
							else
							{
								pPointToRemoveFrom->pConnectedLines = NULL;
							}
							bSkipWhileCheck = true;
						}

						//remove this line from connected polygons
						for( int j = 0; j != 2; ++j )
						{
							GeneratePolyhedronFromPlanes_Polygon *pPolygonToRemoveFrom = pLine->pPolygons[j];
							GeneratePolyhedronFromPlanes_LineLL *pRemoveLine = pLine->pPolygonLineLinks[j];

							AssertMsg( (pRemoveLine->pLine->pPoints[0]->iWorkData != 0) && (pRemoveLine->pLine->pPoints[1]->iWorkData != 0), "Live line being removed from polygon" );

							pPolygonToRemoveFrom->iWorkData = 1; //mark the polygon as missing a side

#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
							pPolygonToRemoveFrom->iDebugFlags |= (1<<3);
							pLine->pPolygons[iReferenceIndex]->iDebugFlags &= ~(1<<2);
#endif						


							pRemoveLine->pNext->pPrev = pRemoveLine->pPrev;
							pRemoveLine->pPrev->pNext = pRemoveLine->pNext;

							pPolygonToRemoveFrom->pLines = pRemoveLine->pPrev;

							if( pPolygonToRemoveFrom->pLines == pPolygonToRemoveFrom->pLines->pNext )
							{
								//only one line left, kill the whole polygon
								GeneratePolyhedronFromPlanes_UnorderedPolygonLL *pActivePolygonWalk = pPolygons;
								do
								{
									if( pActivePolygonWalk->pPolygon == pPolygonToRemoveFrom )
									{
										//link over the polygon in the list
										if( pActivePolygonWalk->pPrev )
											pActivePolygonWalk->pPrev->pNext = pActivePolygonWalk->pNext;
										else //at the head of the polygon list
											pPolygons = pActivePolygonWalk->pNext;

										if( pActivePolygonWalk->pNext )
											pActivePolygonWalk->pNext->pPrev = pActivePolygonWalk->pPrev;

										//TODO: add this polygon's liberated pointers to dead pools for re-use
										break;
									}
									pActivePolygonWalk = pActivePolygonWalk->pNext;
								} while( pActivePolygonWalk );
							}

							//TODO: add this line's liberated pointers to dead pools for re-use
						}

						//TODO: add this line's liberated pointers to dead pools for re-use
					}
					pLineWalk = pLineWalk->pNext;
				} while( bSkipWhileCheck || (pLineWalk != pOnPlanePointWalk->pPoint->pConnectedLines) );
			}

			pOnPlanePointWalk = pOnPlanePointWalk->pNext;
		}


		
		
		//remove dead lines from the beginning of the list
		while( pLines->pLine->iWorkData == 2 )
		{
			GeneratePolyhedronFromPlanes_UnorderedLineLL *pThisLine = pLines;
			pLines = pLines->pNext;

            pThisLine->pPrev = NULL;
			pThisLine->pNext = pDeadLines;
			if( pDeadLines )
				pDeadLines->pPrev = pThisLine;
			pDeadLines = pThisLine;
		}

		//go over all split lines and generate the new point that will replace the dead point
		pActiveLineWalk = pLines;
		do
		{
			GeneratePolyhedronFromPlanes_Line *pWorkLine = pActiveLineWalk->pLine;
			if( pWorkLine->iWorkData == 2 )
			{
				//dead line
				GeneratePolyhedronFromPlanes_UnorderedLineLL *pThisLine = pActiveLineWalk;
				pActiveLineWalk = pActiveLineWalk->pNext;

				pThisLine->pPrev->pNext = pActiveLineWalk;
				if( pActiveLineWalk )
					pActiveLineWalk->pPrev = pThisLine->pPrev;

				pThisLine->pPrev = NULL;
				pThisLine->pNext = pDeadLines;
				if( pDeadLines )
					pDeadLines->pPrev = pThisLine;
				pDeadLines = pThisLine;
				continue;
			}
			else if( pWorkLine->iWorkData == 1 )
			{
				int iDeadIndex = (pWorkLine->pPoints[0]->iWorkData == 2)?(0):(1);
				AssertMsg( pWorkLine->pPoints[iDeadIndex]->iWorkData == 2, "Dead indexed point not dead" );
#ifndef POLYHEDRON_EXTENSIVE_DEBUGGING
				pWorkLine->iWorkData = 0; //no longer cut
#endif

				GeneratePolyhedronFromPlanes_Point *pNewPoint = (GeneratePolyhedronFromPlanes_Point *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Point ) );
				{
					//before we forget, add this point to the active list
					pPoints->pPrev = (GeneratePolyhedronFromPlanes_UnorderedPointLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedPointLL ) );
					pPoints->pPrev->pNext = pPoints;
					pPoints = pPoints->pPrev;
					pPoints->pPrev = NULL;
					pPoints->pPoint = pNewPoint;
				}

				float fTotalDist = pWorkLine->pPoints[iDeadIndex]->fPlaneDist - pWorkLine->pPoints[1 - iDeadIndex]->fPlaneDist; //subtraction because the living index is known to be negative
				pNewPoint->ptPosition = (pWorkLine->pPoints[iDeadIndex]->ptPosition * ((-pWorkLine->pPoints[1 - iDeadIndex]->fPlaneDist)/fTotalDist)) + (pWorkLine->pPoints[1 - iDeadIndex]->ptPosition * ((pWorkLine->pPoints[iDeadIndex]->fPlaneDist)/fTotalDist));
				AssertMsg( fabs( vNormal.Dot( pNewPoint->ptPosition ) - fPlaneDist ) < 1.0f, "Generated split point is far from plane" );
				pNewPoint->iWorkData = 1;
				GeneratePolyhedronFromPlanes_LineLL *pNewLine = pNewPoint->pConnectedLines = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
				pNewLine->pLine = pWorkLine;
				pNewLine->pNext = pNewLine;
				pNewLine->pPrev = pNewLine;
				pNewLine->iReferenceIndex = 1 - iDeadIndex;

				pWorkLine->pPoints[iDeadIndex] = pNewPoint;
				pWorkLine->pPointLineLinks[iDeadIndex] = pNewLine;
				pNewPoint->pConnectedLines = pNewLine;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
				pNewPoint->fPlaneDist = 123456789.0f;
				pNewPoint->iDebugFlags = 8;
#endif
			}

			pActiveLineWalk = pActiveLineWalk->pNext;
		} while( pActiveLineWalk );

		//every polygon that needs a side added has been marked, find the first and then we can walk from one to the rest using their line connections
		pActivePolygonWalk = pPolygons;
		while( pActivePolygonWalk->pPolygon->iWorkData != 1 )
		{
			pActivePolygonWalk = pActivePolygonWalk->pNext;
		}
		Assert( pActivePolygonWalk->pPolygon->iWorkData == 1 );

		GeneratePolyhedronFromPlanes_Polygon *pNewPolygon = (GeneratePolyhedronFromPlanes_Polygon *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Polygon ) );
		{
			//before we forget, add this polygon to the active list
			pPolygons->pPrev = (GeneratePolyhedronFromPlanes_UnorderedPolygonLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedPolygonLL ) );
			pPolygons->pPrev->pNext = pPolygons;
			pPolygons = pPolygons->pPrev;
			pPolygons->pPrev = NULL;
			pPolygons->pPolygon = pNewPolygon;
		}
		pNewPolygon->iWorkData = 0;
		pNewPolygon->vSurfaceNormal = vNormal;
		pNewPolygon->pLines = NULL;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
		pNewPolygon->iDebugFlags = (1<<4);
#endif

		GeneratePolyhedronFromPlanes_Polygon *pWorkPolygon = pActivePolygonWalk->pPolygon;

		//we're pointing at one polygon that needs a side added, we can walk from this polygon to each other polygon filling in the lines
		do
		{
			//during the cutting process we made sure that the head line link was going clockwise into the missing area
			GeneratePolyhedronFromPlanes_LineLL *pCutLines[2];
			pCutLines[0] = pWorkPolygon->pLines;
			pCutLines[1] = pCutLines[0]->pNext;

			GeneratePolyhedronFromPlanes_Line *pJoinLine = (GeneratePolyhedronFromPlanes_Line *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_Line ) );
			{
				//before we forget, add this line to the active list
				pLines->pPrev = (GeneratePolyhedronFromPlanes_UnorderedLineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_UnorderedLineLL ) );
				pLines->pPrev->pNext = pLines;
				pLines = pLines->pPrev;
				pLines->pPrev = NULL;
				pLines->pLine = pJoinLine;
			}
			pJoinLine->iWorkData = 0;
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
			pJoinLine->iWorkData = 1;
			pJoinLine->iDebugFlags = 0;
#endif
			
			pJoinLine->pPoints[0] = pCutLines[0]->pLine->pPoints[pCutLines[0]->iReferenceIndex];
			pJoinLine->pPoints[1] = pCutLines[1]->pLine->pPoints[1 - pCutLines[1]->iReferenceIndex];
#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
			pJoinLine->iDebugFlags |= (1<<4);
#endif

			AssertMsg( (pJoinLine->pPoints[0]->iWorkData == 1) && (pJoinLine->pPoints[1]->iWorkData == 1), "Join points not on cutting plane" );

			//AssertMsg( (pCutLines[0]->pLine->iWorkData == 1) && (pCutLines[1]->pLine->iWorkData == 1), "Join Lines not cut" );
			
			pJoinLine->pPolygons[0] = pNewPolygon;
			pJoinLine->pPolygons[1] = pWorkPolygon;

			//now create all 4 links into the line
			GeneratePolyhedronFromPlanes_LineLL *pPointLinks[2];
			pPointLinks[0] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
			pPointLinks[1] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );

			GeneratePolyhedronFromPlanes_LineLL *pPolygonLinks[2];
			pPolygonLinks[0] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );
			pPolygonLinks[1] = (GeneratePolyhedronFromPlanes_LineLL *)stackalloc( sizeof( GeneratePolyhedronFromPlanes_LineLL ) );

			pPointLinks[0]->pLine = pPointLinks[1]->pLine = pPolygonLinks[0]->pLine = pPolygonLinks[1]->pLine = pJoinLine;
			
			pJoinLine->pPointLineLinks[0] = pPointLinks[0];
			pJoinLine->pPointLineLinks[1] = pPointLinks[1];
			pJoinLine->pPolygonLineLinks[0] = pPolygonLinks[0];
			pJoinLine->pPolygonLineLinks[1] = pPolygonLinks[1];
			
			pPointLinks[0]->iReferenceIndex = 1;
			pPointLinks[1]->iReferenceIndex = 0;

			GeneratePolyhedronFromPlanes_LineLL *pWorkLink;
			//insert before the link coming to cut line 0 from the new point
			pWorkLink = pCutLines[0]->pLine->pPointLineLinks[pCutLines[0]->iReferenceIndex];
			pPointLinks[0]->pNext = pWorkLink;
			pPointLinks[0]->pPrev = pWorkLink->pPrev;
			pWorkLink->pPrev->pNext = pPointLinks[0];
			pWorkLink->pPrev = pPointLinks[0];

			//insert after the link coming to cut line 1 from the new point
			pWorkLink = pCutLines[1]->pLine->pPointLineLinks[1 - pCutLines[1]->iReferenceIndex];
			pPointLinks[1]->pNext = pWorkLink->pNext;
			pPointLinks[1]->pPrev = pWorkLink;
			pWorkLink->pNext->pPrev = pPointLinks[1];
			pWorkLink->pNext = pPointLinks[1];


			pPolygonLinks[0]->iReferenceIndex = 0;
			pPolygonLinks[1]->iReferenceIndex = 1;

			//insert before the head line in the new polygon (effectively adding to the tail of the polygons lines)
			if( pNewPolygon->pLines == NULL )
			{
				//this is the first line being added to the polygon
				pNewPolygon->pLines = pPolygonLinks[0];
				pPolygonLinks[0]->pNext = pPolygonLinks[0];
				pPolygonLinks[0]->pPrev = pPolygonLinks[0];
			}
			else
			{
				pWorkLink = pNewPolygon->pLines;
				pPolygonLinks[0]->pNext = pWorkLink;
				pPolygonLinks[0]->pPrev = pWorkLink->pPrev;
				pWorkLink->pPrev->pNext = pPolygonLinks[0];
				pWorkLink->pPrev = pPolygonLinks[0];
			}

			//insert after the head line in the work polygon
			pWorkLink = pWorkPolygon->pLines;
			pPolygonLinks[1]->pNext = pWorkLink->pNext;
			pPolygonLinks[1]->pPrev = pWorkLink;
			pWorkLink->pNext->pPrev = pPolygonLinks[1];
			pWorkLink->pNext = pPolygonLinks[1];



			pWorkPolygon->iWorkData = 0; //no longer cut

			//walk to the next polygon polygon that needs work, general case should be the flip-side of cutline 0, but on-plane points might force us to skip a few, so we rotate around join point 0 (starting at cut line 0) until we find one that needs fixing
			GeneratePolyhedronFromPlanes_LineLL *pCutWalkLine = pCutLines[0]->pLine->pPointLineLinks[pCutLines[0]->iReferenceIndex];
			GeneratePolyhedronFromPlanes_LineLL *pFirstCutWalkLine = pCutWalkLine;
			do
			{
				if( pCutWalkLine->pLine->pPolygons[pCutWalkLine->iReferenceIndex]->iWorkData == 1 )
				{
					pWorkPolygon = pCutWalkLine->pLine->pPolygons[pCutWalkLine->iReferenceIndex];
					break;
				}
				pCutWalkLine = pCutWalkLine->pNext;
			} while( pCutWalkLine != pFirstCutWalkLine );

			
		} while( pWorkPolygon->iWorkData == 1 ); //when the work polygon isn't cut, then we're done

#ifdef POLYHEDRON_EXTENSIVE_DEBUGGING
		pActivePolygonWalk = pPolygons;
		do
		{
			AssertMsg( pActivePolygonWalk->pPolygon->iWorkData == 0, "Some polygons not repaired after cut" );
			pActivePolygonWalk = pActivePolygonWalk->pNext;
		} while( pActivePolygonWalk );
#endif

		//if( g_bOutputEveryPolyhedronClip )
		//	ConvertLinkedGeometryToPolyhedron( pPolygons, pLines, pPoints, szDumpIterate );
	}

	return ConvertLinkedGeometryToPolyhedron( pPolygons, pLines, pPoints, bUseTemporaryMemory );
}



#define STARTPOINTTOLINELINKS(iPointNum, lineindex1, iOtherPointIndex1, lineindex2, iOtherPointIndex2, lineindex3, iOtherPointIndex3 )\
	StartingBoxPoints[iPointNum].pConnectedLines = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pLine = &StartingBoxLines[lineindex1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].iReferenceIndex = iOtherPointIndex1;\
	StartingBoxLines[lineindex1].pPointLineLinks[1 - iOtherPointIndex1] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 0].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pLine = &StartingBoxLines[lineindex2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].iReferenceIndex = iOtherPointIndex2;\
	StartingBoxLines[lineindex2].pPointLineLinks[1 - iOtherPointIndex2] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 1].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pLine = &StartingBoxLines[lineindex3];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].iReferenceIndex = iOtherPointIndex3;\
	StartingBoxLines[lineindex3].pPointLineLinks[1 - iOtherPointIndex3] = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 2];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pPrev = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 1];\
	StartingPoints_To_Lines_Links[(iPointNum * 3) + 2].pNext = &StartingPoints_To_Lines_Links[(iPointNum * 3) + 0];

#define STARTBOXCONNECTION( linenum, point1, point2, poly1, poly2 )\
	StartingBoxLines[linenum].pPoints[0] = &StartingBoxPoints[point1];\
	StartingBoxLines[linenum].pPoints[1] = &StartingBoxPoints[point2];\
	StartingBoxLines[linenum].pPolygons[0] = &StartingBoxPolygons[poly1];\
	StartingBoxLines[linenum].pPolygons[1] = &StartingBoxPolygons[poly2];

#define STARTPOLYGONTOLINELINKS( polynum, lineindex1, iThisPolyIndex1, lineindex2, iThisPolyIndex2, lineindex3, iThisPolyIndex3, lineindex4, iThisPolyIndex4 )\
	StartingBoxPolygons[polynum].pLines = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pLine = &StartingBoxLines[lineindex1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].iReferenceIndex = iThisPolyIndex1;\
	StartingBoxLines[lineindex1].pPolygonLineLinks[iThisPolyIndex1] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 0].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pLine = &StartingBoxLines[lineindex2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].iReferenceIndex = iThisPolyIndex2;\
	StartingBoxLines[lineindex2].pPolygonLineLinks[iThisPolyIndex2] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 1].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pLine = &StartingBoxLines[lineindex3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].iReferenceIndex = iThisPolyIndex3;\
	StartingBoxLines[lineindex3].pPolygonLineLinks[iThisPolyIndex3] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 1];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 2].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pLine = &StartingBoxLines[lineindex4];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].iReferenceIndex = iThisPolyIndex4;\
	StartingBoxLines[lineindex4].pPolygonLineLinks[iThisPolyIndex4] = &StartingPolygon_To_Lines_Links[(polynum * 4) + 3];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pPrev = &StartingPolygon_To_Lines_Links[(polynum * 4) + 2];\
	StartingPolygon_To_Lines_Links[(polynum * 4) + 3].pNext = &StartingPolygon_To_Lines_Links[(polynum * 4) + 0];


CPolyhedron *GeneratePolyhedronFromPlanes( const float *pOutwardFacingPlanes, int iPlaneCount, float fOnPlaneEpsilon, bool bUseTemporaryMemory )
{
	//this is version 2 of the polyhedron generator, version 1 made individual polygons and joined points together, some guesswork is involved and it therefore isn't a solid method
	//this version will start with a cube and hack away at it (retaining point connection information) to produce a polyhedron with no guesswork involved, this method should be rock solid
	
	//the polygon clipping functions we're going to use want inward facing planes
	float *pFlippedPlanes = (float *)stackalloc( (iPlaneCount * 4) * sizeof( float ) );
	for( int i = 0; i != iPlaneCount * 4; ++i )
	{
		pFlippedPlanes[i] = -pOutwardFacingPlanes[i];
	}

	//our first goal is to find the size of a cube big enough to encapsulate all points that will be in the final polyhedron
	Vector vAABBMins, vAABBMaxs;
	if( FindConvexShapeLooseAABB( pFlippedPlanes, iPlaneCount, &vAABBMins, &vAABBMaxs ) == false )
		return NULL; //no shape to work with apparently

	
	//grow the bounding box to a larger size since it's probably inaccurate a bit
	{
		Vector vGrow = (vAABBMaxs - vAABBMins) * 0.5f;
		vGrow.x += 100.0f;
		vGrow.y += 100.0f;
		vGrow.z += 100.0f;

		vAABBMaxs += vGrow;
		vAABBMins -= vGrow;
	}

	//generate our starting cube using the 2x AABB so we can start hacking away at it
	
	

	//create our starting box on the stack
	GeneratePolyhedronFromPlanes_Point StartingBoxPoints[8];
	GeneratePolyhedronFromPlanes_Line StartingBoxLines[12];
	GeneratePolyhedronFromPlanes_Polygon StartingBoxPolygons[6];
	GeneratePolyhedronFromPlanes_LineLL StartingPoints_To_Lines_Links[24]; //8 points, 3 lines per point
	GeneratePolyhedronFromPlanes_LineLL StartingPolygon_To_Lines_Links[24]; //6 polygons, 4 lines per poly
	
	GeneratePolyhedronFromPlanes_UnorderedPolygonLL StartingPolygonList[6]; //6 polygons
	GeneratePolyhedronFromPlanes_UnorderedLineLL StartingLineList[12]; //12 lines
	GeneratePolyhedronFromPlanes_UnorderedPointLL StartingPointList[8]; //8 points


	//I had to work all this out on a whiteboard if it seems completely unintuitive.
	{
		StartingBoxPoints[0].ptPosition.Init( vAABBMins.x, vAABBMins.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 0, 0, 1, 4, 1, 3, 0 );

		StartingBoxPoints[1].ptPosition.Init( vAABBMins.x, vAABBMaxs.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 1, 0, 0, 1, 1, 5, 1 );

		StartingBoxPoints[2].ptPosition.Init( vAABBMins.x, vAABBMins.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 2, 4, 0, 8, 1, 11, 0 );

		StartingBoxPoints[3].ptPosition.Init( vAABBMins.x, vAABBMaxs.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 3, 5, 0, 9, 1, 8, 0 );

		StartingBoxPoints[4].ptPosition.Init( vAABBMaxs.x, vAABBMins.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 4, 2, 0, 3, 1, 7, 1 );

		StartingBoxPoints[5].ptPosition.Init( vAABBMaxs.x, vAABBMaxs.y, vAABBMins.z );
		STARTPOINTTOLINELINKS( 5, 1, 0, 2, 1, 6, 1 );

		StartingBoxPoints[6].ptPosition.Init( vAABBMaxs.x, vAABBMins.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 6, 7, 0, 11, 1, 10, 0 );

		StartingBoxPoints[7].ptPosition.Init( vAABBMaxs.x, vAABBMaxs.y, vAABBMaxs.z );
		STARTPOINTTOLINELINKS( 7, 6, 0, 10, 1, 9, 0 );

		STARTBOXCONNECTION( 0, 0, 1, 0, 5 );
		STARTBOXCONNECTION( 1, 1, 5, 1, 5 );
		STARTBOXCONNECTION( 2, 5, 4, 2, 5 );
		STARTBOXCONNECTION( 3, 4, 0, 3, 5 );
		STARTBOXCONNECTION( 4, 0, 2, 3, 0 );
		STARTBOXCONNECTION( 5, 1, 3, 0, 1 );
		STARTBOXCONNECTION( 6, 5, 7, 1, 2 );
		STARTBOXCONNECTION( 7, 4, 6, 2, 3 );
		STARTBOXCONNECTION( 8, 2, 3, 4, 0 );
		STARTBOXCONNECTION( 9, 3, 7, 4, 1 );
		STARTBOXCONNECTION( 10, 7, 6, 4, 2 );
		STARTBOXCONNECTION( 11, 6, 2, 4, 3 );


		STARTBOXCONNECTION( 0, 0, 1, 5, 0 );
		STARTBOXCONNECTION( 1, 1, 5, 5, 1 );
		STARTBOXCONNECTION( 2, 5, 4, 5, 2 );
		STARTBOXCONNECTION( 3, 4, 0, 5, 3 );
		STARTBOXCONNECTION( 4, 0, 2, 0, 3 );
		STARTBOXCONNECTION( 5, 1, 3, 1, 0 );
		STARTBOXCONNECTION( 6, 5, 7, 2, 1 );
		STARTBOXCONNECTION( 7, 4, 6, 3, 2 );
		STARTBOXCONNECTION( 8, 2, 3, 0, 4 );
		STARTBOXCONNECTION( 9, 3, 7, 1, 4 );
		STARTBOXCONNECTION( 10, 7, 6, 2, 4 );
		STARTBOXCONNECTION( 11, 6, 2, 3, 4 );

		StartingBoxPolygons[0].vSurfaceNormal.Init( -1.0f, 0.0f, 0.0f );
		StartingBoxPolygons[1].vSurfaceNormal.Init( 0.0f, 1.0f, 0.0f );
		StartingBoxPolygons[2].vSurfaceNormal.Init( 1.0f, 0.0f, 0.0f );
		StartingBoxPolygons[3].vSurfaceNormal.Init( 0.0f, -1.0f, 0.0f );
		StartingBoxPolygons[4].vSurfaceNormal.Init( 0.0f, 0.0f, 1.0f );
		StartingBoxPolygons[5].vSurfaceNormal.Init( 0.0f, 0.0f, -1.0f );


		STARTPOLYGONTOLINELINKS( 0, 0, 1, 5, 1, 8, 0, 4, 0 );
		STARTPOLYGONTOLINELINKS( 1, 1, 1, 6, 1, 9, 0, 5, 0 );
		STARTPOLYGONTOLINELINKS( 2, 2, 1, 7, 1, 10, 0, 6, 0 );
		STARTPOLYGONTOLINELINKS( 3, 3, 1, 4, 1, 11, 0, 7, 0 );
		STARTPOLYGONTOLINELINKS( 4, 8, 1, 9, 1, 10, 1, 11, 1 );
		STARTPOLYGONTOLINELINKS( 5, 0, 0, 3, 0, 2, 0, 1, 0 );


		{
			StartingPolygonList[0].pPolygon = &StartingBoxPolygons[0];
			StartingPolygonList[0].pNext = &StartingPolygonList[1];
			StartingPolygonList[0].pPrev = NULL;

			StartingPolygonList[1].pPolygon = &StartingBoxPolygons[1];
			StartingPolygonList[1].pNext = &StartingPolygonList[2];
			StartingPolygonList[1].pPrev = &StartingPolygonList[0];

			StartingPolygonList[2].pPolygon = &StartingBoxPolygons[2];
			StartingPolygonList[2].pNext = &StartingPolygonList[3];
			StartingPolygonList[2].pPrev = &StartingPolygonList[1];

			StartingPolygonList[3].pPolygon = &StartingBoxPolygons[3];
			StartingPolygonList[3].pNext = &StartingPolygonList[4];
			StartingPolygonList[3].pPrev = &StartingPolygonList[2];

			StartingPolygonList[4].pPolygon = &StartingBoxPolygons[4];
			StartingPolygonList[4].pNext = &StartingPolygonList[5];
			StartingPolygonList[4].pPrev = &StartingPolygonList[3];

			StartingPolygonList[5].pPolygon = &StartingBoxPolygons[5];
			StartingPolygonList[5].pNext = NULL;
			StartingPolygonList[5].pPrev = &StartingPolygonList[4];
		}



		{
			StartingLineList[0].pLine = &StartingBoxLines[0];
			StartingLineList[0].pNext = &StartingLineList[1];
			StartingLineList[0].pPrev = NULL;

			StartingLineList[1].pLine = &StartingBoxLines[1];
			StartingLineList[1].pNext = &StartingLineList[2];
			StartingLineList[1].pPrev = &StartingLineList[0];

			StartingLineList[2].pLine = &StartingBoxLines[2];
			StartingLineList[2].pNext = &StartingLineList[3];
			StartingLineList[2].pPrev = &StartingLineList[1];

			StartingLineList[3].pLine = &StartingBoxLines[3];
			StartingLineList[3].pNext = &StartingLineList[4];
			StartingLineList[3].pPrev = &StartingLineList[2];

			StartingLineList[4].pLine = &StartingBoxLines[4];
			StartingLineList[4].pNext = &StartingLineList[5];
			StartingLineList[4].pPrev = &StartingLineList[3];

			StartingLineList[5].pLine = &StartingBoxLines[5];
			StartingLineList[5].pNext = &StartingLineList[6];
			StartingLineList[5].pPrev = &StartingLineList[4];

			StartingLineList[6].pLine = &StartingBoxLines[6];
			StartingLineList[6].pNext = &StartingLineList[7];
			StartingLineList[6].pPrev = &StartingLineList[5];

			StartingLineList[7].pLine = &StartingBoxLines[7];
			StartingLineList[7].pNext = &StartingLineList[8];
			StartingLineList[7].pPrev = &StartingLineList[6];

			StartingLineList[8].pLine = &StartingBoxLines[8];
			StartingLineList[8].pNext = &StartingLineList[9];
			StartingLineList[8].pPrev = &StartingLineList[7];

			StartingLineList[9].pLine = &StartingBoxLines[9];
			StartingLineList[9].pNext = &StartingLineList[10];
			StartingLineList[9].pPrev = &StartingLineList[8];

			StartingLineList[10].pLine = &StartingBoxLines[10];
			StartingLineList[10].pNext = &StartingLineList[11];
			StartingLineList[10].pPrev = &StartingLineList[9];

			StartingLineList[11].pLine = &StartingBoxLines[11];
			StartingLineList[11].pNext = NULL;
			StartingLineList[11].pPrev = &StartingLineList[10];
		}

		{
			StartingPointList[0].pPoint = &StartingBoxPoints[0];
			StartingPointList[0].pNext = &StartingPointList[1];
			StartingPointList[0].pPrev = NULL;

			StartingPointList[1].pPoint = &StartingBoxPoints[1];
			StartingPointList[1].pNext = &StartingPointList[2];
			StartingPointList[1].pPrev = &StartingPointList[0];

			StartingPointList[2].pPoint = &StartingBoxPoints[2];
			StartingPointList[2].pNext = &StartingPointList[3];
			StartingPointList[2].pPrev = &StartingPointList[1];

			StartingPointList[3].pPoint = &StartingBoxPoints[3];
			StartingPointList[3].pNext = &StartingPointList[4];
			StartingPointList[3].pPrev = &StartingPointList[2];

			StartingPointList[4].pPoint = &StartingBoxPoints[4];
			StartingPointList[4].pNext = &StartingPointList[5];
			StartingPointList[4].pPrev = &StartingPointList[3];

			StartingPointList[5].pPoint = &StartingBoxPoints[5];
			StartingPointList[5].pNext = &StartingPointList[6];
			StartingPointList[5].pPrev = &StartingPointList[4];

			StartingPointList[6].pPoint = &StartingBoxPoints[6];
			StartingPointList[6].pNext = &StartingPointList[7];
			StartingPointList[6].pPrev = &StartingPointList[5];

			StartingPointList[7].pPoint = &StartingBoxPoints[7];
			StartingPointList[7].pNext = NULL;
			StartingPointList[7].pPrev = &StartingPointList[6];
		}
	}

	return ClipLinkedGeometry( StartingPolygonList, StartingLineList, StartingPointList, pOutwardFacingPlanes, iPlaneCount, fOnPlaneEpsilon, bUseTemporaryMemory );
}













































#ifdef DEBUG_DUMP_POLYHEDRONS_TO_NUMBERED_GLVIEWS
void DumpPolyhedronToGLView( const CPolyhedron *pPolyhedron, const char *pFilename )
{
	if ( !pPolyhedron )
		return;

	printf("Writing %s...\n", pFilename );
	
	FileHandle_t fp = filesystem->Open( pFilename, "ab" );

	//randomizing an array of colors to help spot shared/unshared vertices
	Vector *pColors = (Vector *)stackalloc( sizeof( Vector ) * pPolyhedron->iVertexCount );	
	int counter;
	for( counter = 0; counter != pPolyhedron->iVertexCount; ++counter )
	{
		pColors[counter].Init( rand()/32768.0f, rand()/32768.0f, rand()/32768.0f );
	}



	for ( counter = 0; counter != pPolyhedron->iPolygonCount; ++counter )
	{
		filesystem->FPrintf( fp, "%i\n", pPolyhedron->pPolygons[counter].iIndexCount );
		int counter2;
		float fIntensity = 1.0f;
		float fIntensityDelta = -1.0f/((float)pPolyhedron->pPolygons[counter].iIndexCount);
		for( counter2 = 0; counter2 != pPolyhedron->pPolygons[counter].iIndexCount; ++counter2 )
		{
			Polyhedron_IndexedLineReference_t *pLineReference = &pPolyhedron->pIndices[pPolyhedron->pPolygons[counter].iFirstIndex + counter2];
			
			Vector *pVertex = &pPolyhedron->pVertices[pPolyhedron->pLines[pLineReference->iLineIndex].iPointIndices[pLineReference->iEndPointIndex]];
			Vector *pColor = &pColors[pPolyhedron->pLines[pLineReference->iLineIndex].iPointIndices[pLineReference->iEndPointIndex]];
			//filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n",pVertex->x, pVertex->y, pVertex->z, fIntensity, 0.0f, 0.0f );
			filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n",pVertex->x, pVertex->y, pVertex->z, pColor->x, pColor->y, pColor->z );
			fIntensity += fIntensityDelta;
		}
	}

	for( counter = 0; counter != pPolyhedron->iVertexCount; ++counter )
	{
		Vector vMins = pPolyhedron->pVertices[counter] - Vector(0.3f, 0.3f, 0.3f );
		Vector vMaxs = pPolyhedron->pVertices[counter] + Vector(0.3f, 0.3f, 0.3f );
		Vector *pColor = &pColors[counter];
		//x min side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		//x max side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );


		//y min side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );



		//y max side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );



		//z min side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMins.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );



		//z max side
		filesystem->FPrintf( fp, "4\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );

		filesystem->FPrintf( fp, "4\n" );	
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMaxs.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMaxs.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vMins.x, vMins.y, vMaxs.z, 1.0f - pColor->x, 1.0f - pColor->y, 1.0f - pColor->z );
	}

	filesystem->Close( fp );
}


void DumpPlaneToGlView( float *pPlane, float fGrayScale, const char *pszFileName )
{
	FileHandle_t fp = filesystem->Open( pszFileName, "ab" );

	
	Vector vPlaneVerts[4];

	PolyFromPlane( vPlaneVerts, *(Vector *)(pPlane), pPlane[3], 100000.0f );

	filesystem->FPrintf( fp, "4\n" );

	filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[0].x, vPlaneVerts[0].y, vPlaneVerts[0].z, fGrayScale, fGrayScale, fGrayScale );
	filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[1].x, vPlaneVerts[1].y, vPlaneVerts[1].z, fGrayScale, fGrayScale, fGrayScale );
	filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[2].x, vPlaneVerts[2].y, vPlaneVerts[2].z, fGrayScale, fGrayScale, fGrayScale );
	filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", vPlaneVerts[3].x, vPlaneVerts[3].y, vPlaneVerts[3].z, fGrayScale, fGrayScale, fGrayScale );

	filesystem->Close( fp );
}
#endif


