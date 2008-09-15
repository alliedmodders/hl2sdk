//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifdef _WIN32
#include <windows.h>
#endif
#include "vrad.h"
#include "lightmap.h"
#include "radial.h"
#include <bumpvects.h>
#include "tier1/utlvector.h"
#include "vmpi.h"
#include "anorms.h"
#include "map_utils.h"
#include "mathlib/halton.h"
#include "imagepacker.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlbuffer.h"
#include "bitmap/tgawriter.h"
#include "vtf/swizzler.h"
#include "quantize.h"
#include "bitmap/imageformat.h"

enum
{
	AMBIENT_ONLY = 0x1,
	NON_AMBIENT_ONLY = 0x2,
};

#define SMOOTHING_GROUP_HARD_EDGE	0xff000000

//==========================================================================//
// CNormalList.
//==========================================================================//

// This class keeps a list of unique normals and provides a fast 
class CNormalList
{
public:
	CNormalList();
	
	// Adds the normal if unique. Otherwise, returns the normal's index into m_Normals.
	int FindOrAddNormal( Vector const &vNormal );


public:
	
	CUtlVector<Vector>	m_Normals;


private:

	// This represents a grid from (-1,-1,-1) to (1,1,1).
	enum {NUM_SUBDIVS = 8};
	CUtlVector<int>	m_NormalGrid[NUM_SUBDIVS][NUM_SUBDIVS][NUM_SUBDIVS];
};


int g_iCurFace;
edgeshare_t	edgeshare[MAX_MAP_EDGES];

Vector	face_centroids[MAX_MAP_EDGES];

int vertexref[MAX_MAP_VERTS];
int *vertexface[MAX_MAP_VERTS];
faceneighbor_t faceneighbor[MAX_MAP_FACES];

static directlight_t *gSkyLight = NULL;
static directlight_t *gAmbient = NULL;

//==========================================================================//
// CNormalList implementation.
//==========================================================================//

CNormalList::CNormalList() : m_Normals( 128 )
{
	for( int i=0; i < sizeof(m_NormalGrid)/sizeof(m_NormalGrid[0][0][0]); i++ )
	{
		(&m_NormalGrid[0][0][0] + i)->SetGrowSize( 16 );
	}
}

int CNormalList::FindOrAddNormal( Vector const &vNormal )
{
	int gi[3];

	// See which grid element it's in.
	for( int iDim=0; iDim < 3; iDim++ )
	{
		gi[iDim] = (int)( ((vNormal[iDim] + 1.0f) * 0.5f) * NUM_SUBDIVS - 0.000001f );
		gi[iDim] = min( gi[iDim], NUM_SUBDIVS );
		gi[iDim] = max( gi[iDim], 0 );
	}

	// Look for a matching vector in there.
	CUtlVector<int> *pGridElement = &m_NormalGrid[gi[0]][gi[1]][gi[2]];
	for( int i=0; i < pGridElement->Size(); i++ )
	{
		int iNormal = pGridElement->Element(i);

		Vector *pVec = &m_Normals[iNormal];
		//if( pVec->DistToSqr(vNormal) < 0.00001f )
		if( *pVec == vNormal )
			return iNormal;
	}

	// Ok, add a new one.
	pGridElement->AddToTail( m_Normals.Size() );
	return m_Normals.AddToTail( vNormal );
}

// FIXME: HACK until the plane normals are made more happy
void GetBumpNormals( const float* sVect, const float* tVect, const Vector& flatNormal, 
					 const Vector& phongNormal, Vector bumpNormals[NUM_BUMP_VECTS] )
{
	Vector stmp( sVect[0], sVect[1], sVect[2] );
	Vector ttmp( tVect[0], tVect[1], tVect[2] );
	GetBumpNormals( stmp, ttmp, flatNormal, phongNormal, bumpNormals );
}

int EdgeVertex( dface_t *f, int edge )
{
	int k;

	if (edge < 0)
		edge += f->numedges;
	else if (edge >= f->numedges)
		edge = edge % f->numedges;

	k = dsurfedges[f->firstedge + edge];
	if (k < 0)
	{
		// Msg("(%d %d) ", dedges[-k].v[1], dedges[-k].v[0] );
		return dedges[-k].v[1];
	}
	else
	{
		// Msg("(%d %d) ", dedges[k].v[0], dedges[k].v[1] );
		return dedges[k].v[0];
	}
}


/*
  ============
  PairEdges
  ============
*/
void PairEdges (void)
{
	int		        i, j, k, n, m;
	dface_t	        *f;
	int             numneighbors;
    int             tmpneighbor[64];
	faceneighbor_t  *fn;

	// count number of faces that reference each vertex
	for (i=0, f = g_pFaces; i<numfaces ; i++, f++)
	{
        for (j=0 ; j<f->numedges ; j++)
        {
            // Store the count in vertexref
            vertexref[EdgeVertex(f,j)]++;
        }
	}

	// allocate room
	for (i = 0; i < numvertexes; i++)
	{
		// use the count from above to allocate a big enough array
		vertexface[i] = ( int* )calloc( vertexref[i], sizeof( vertexface[0] ) );
		// clear the temporary data
		vertexref[i] = 0;
	}

	// store a list of every face that uses a perticular vertex
	for (i=0, f = g_pFaces ; i<numfaces ; i++, f++)
	{
        for (j=0 ; j<f->numedges ; j++)
        {
            n = EdgeVertex(f,j);
            
            for (k = 0; k < vertexref[n]; k++)
            {
                if (vertexface[n][k] == i)
                    break;
            }
            if (k >= vertexref[n])
            {
                // add the face to the list
                vertexface[n][k] = i;
                vertexref[n]++;
            }
        }
	}

	// calc normals and set displacement surface flag
	for (i=0, f = g_pFaces; i<numfaces ; i++, f++)
	{
		fn = &faceneighbor[i];

		// get face normal
		VectorCopy( dplanes[f->planenum].normal, fn->facenormal );

		// set displacement surface flag
		fn->bHasDisp = false;
		if( ValidDispFace( f ) )
		{
			fn->bHasDisp = true;
		}
	}

	// find neighbors
	for (i=0, f = g_pFaces ; i<numfaces ; i++, f++)
	{
		numneighbors = 0;
		fn = &faceneighbor[i];

        // allocate room for vertex normals
        fn->normal = ( Vector* )calloc( f->numedges, sizeof( fn->normal[0] ) );
     		
        // look up all faces sharing vertices and add them to the list
        for (j=0 ; j<f->numedges ; j++)
        {
            n = EdgeVertex(f,j);
            
            for (k = 0; k < vertexref[n]; k++)
            {
                double	cos_normals_angle;
                Vector  *pNeighbornormal;
                
                // skip self
                if (vertexface[n][k] == i)
                    continue;

				// if this face doens't have a displacement -- don't consider displacement neighbors
				if( ( !fn->bHasDisp ) && ( faceneighbor[vertexface[n][k]].bHasDisp ) )
					continue;

                pNeighbornormal = &faceneighbor[vertexface[n][k]].facenormal;
                cos_normals_angle = DotProduct( *pNeighbornormal, fn->facenormal );
					
				// add normal if >= threshold or its a displacement surface (this is only if the original
				// face is a displacement)
				if ( fn->bHasDisp )
				{
					// Always smooth with and against a displacement surface.
					VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
				}
				else
				{
					// No smoothing - use of method (backwards compatibility).
					if ( ( f->smoothingGroups == 0 ) && ( g_pFaces[vertexface[n][k]].smoothingGroups == 0 ) )
					{
						if ( cos_normals_angle >= smoothing_threshold )
						{
							VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
						}
						else
						{
							// not considered a neighbor
							continue;
						}
					}
					else
					{
						unsigned int smoothingGroup = ( f->smoothingGroups & g_pFaces[vertexface[n][k]].smoothingGroups );

						// Hard edge.
						if ( ( smoothingGroup & SMOOTHING_GROUP_HARD_EDGE ) != 0 )
							continue;

						if ( smoothingGroup != 0 )
						{
							VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
						}
						else
						{
							// not considered a neighbor
							continue;
						}
					}
				}

				// look to see if we've already added this one
				for (m = 0; m < numneighbors; m++)
				{
					if (tmpneighbor[m] == vertexface[n][k])
						break;
				}
				
				if (m >= numneighbors)
				{
					// add to neighbor list
					tmpneighbor[m] = vertexface[n][k];
					numneighbors++;
					if ( numneighbors > ARRAYSIZE(tmpneighbor) )
					{
						Error("Stack overflow in neighbors\n");
					}
				}
            }
        }

        if (numneighbors)
        {
            // copy over neighbor list
            fn->numneighbors = numneighbors;
            fn->neighbor = ( int* )calloc( numneighbors, sizeof( fn->neighbor[0] ) );
            for (m = 0; m < numneighbors; m++)
            {
                fn->neighbor[m] = tmpneighbor[m];
            }
        }
        
		// fixup normals
        for (j = 0; j < f->numedges; j++)
        {
            VectorAdd( fn->normal[j], fn->facenormal, fn->normal[j] );
            VectorNormalize( fn->normal[j] );
        }
    }
}


void SaveVertexNormals( void )
{
	faceneighbor_t *fn;
	int i, j;
	dface_t *f;
	CNormalList normalList;

	g_numvertnormalindices = 0;

	for( i = 0 ;i<numfaces ; i++ )
	{
		fn = &faceneighbor[i];
		f = &g_pFaces[i];

		for( j = 0; j < f->numedges; j++ )
		{
			Vector vNormal; 
			if( fn->normal )
			{
				vNormal = fn->normal[j];
			}
			else
			{
				// original faces don't have normals
				vNormal.Init( 0, 0, 0 );
			}
			
			if( g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES )
			{
				Error( "g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES" );
			}
			
			g_vertnormalindices[g_numvertnormalindices] = (unsigned short)normalList.FindOrAddNormal( vNormal );
			g_numvertnormalindices++;
		}
	}

	if( normalList.m_Normals.Size() > MAX_MAP_VERTNORMALS )
	{
		Error( "g_numvertnormals > MAX_MAP_VERTNORMALS" );
	}

	// Copy the list of unique vert normals into g_vertnormals.
	g_numvertnormals = normalList.m_Normals.Size();
	memcpy( g_vertnormals, normalList.m_Normals.Base(), sizeof(g_vertnormals[0]) * normalList.m_Normals.Size() );
}

/*
  =================================================================

  LIGHTMAP SAMPLE GENERATION

  =================================================================
*/


//-----------------------------------------------------------------------------
// Purpose: Spits out an error message with information about a lightinfo_t.
// Input  : s - Error message string.
//			l - lightmap info struct.
//-----------------------------------------------------------------------------
void ErrorLightInfo(const char *s, lightinfo_t *l)
{
	texinfo_t *tex = &texinfo[l->face->texinfo];
	winding_t *w = WindingFromFace(&g_pFaces[l->facenum], l->modelorg);

	//
	// Show the face center and material name if possible.
	//
	if (w != NULL)
	{
		// Don't exit, we'll try to recover...
		Vector vecCenter;
		WindingCenter(w, vecCenter);
//		FreeWinding(w);

		Warning("%s at (%g, %g, %g)\n\tmaterial=%s\n", s, (double)vecCenter.x, (double)vecCenter.y, (double)vecCenter.z, TexDataStringTable_GetString( dtexdata[tex->texdata].nameStringTableID ) );
	}
	//
	// If not, just show the material name.
	//
	else
	{
		Warning("%s at (degenerate face)\n\tmaterial=%s\n", TexDataStringTable_GetString( dtexdata[tex->texdata].nameStringTableID ));
	}
}



void CalcFaceVectors(lightinfo_t *l)
{
	texinfo_t	*tex;
	int			i, j;

	tex = &texinfo[l->face->texinfo];
	
    // move into lightinfo_t
	for (i=0 ; i<2 ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			l->worldToLuxelSpace[i][j] = tex->lightmapVecsLuxelsPerWorldUnits[i][j];
		}
	}

	//Solve[ { x * w00 + y * w01 + z * w02 - s == 0, x * w10 + y * w11 + z * w12 - t == 0, A * x + B * y + C * z + D == 0 }, { x, y, z } ]
	//Rule(x,(  C*s*w11 - B*s*w12 + B*t*w02 - C*t*w01 + D*w02*w11 - D*w01*w12) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 )),
	//Rule(y,(  A*s*w12 - C*s*w10 + C*t*w00 - A*t*w02 + D*w00*w12 - D*w02*w10) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 )),
	//Rule(z,(  B*s*w10 - A*s*w11 + A*t*w01 - B*t*w00 + D*w01*w10 - D*w00*w11) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 ))))

	Vector luxelSpaceCross;

	luxelSpaceCross[0] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][1] * tex->lightmapVecsLuxelsPerWorldUnits[0][2] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][2] * tex->lightmapVecsLuxelsPerWorldUnits[0][1];
	luxelSpaceCross[1] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][2] * tex->lightmapVecsLuxelsPerWorldUnits[0][0] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][0] * tex->lightmapVecsLuxelsPerWorldUnits[0][2];
	luxelSpaceCross[2] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][0] * tex->lightmapVecsLuxelsPerWorldUnits[0][1] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][1] * tex->lightmapVecsLuxelsPerWorldUnits[0][0];

	float det = -DotProduct( l->facenormal, luxelSpaceCross );

	// invert the matrix
	l->luxelToWorldSpace[0][0]	= (l->facenormal[2] * l->worldToLuxelSpace[1][1] - l->facenormal[1] * l->worldToLuxelSpace[1][2]) / det;
	l->luxelToWorldSpace[1][0]	= (l->facenormal[1] * l->worldToLuxelSpace[0][2] - l->facenormal[2] * l->worldToLuxelSpace[0][1]) / det;
	l->luxelOrigin[0]			= -(l->facedist * luxelSpaceCross[0]) / det;
	l->luxelToWorldSpace[0][1]  = (l->facenormal[0] * l->worldToLuxelSpace[1][2] - l->facenormal[2] * l->worldToLuxelSpace[1][0]) / det; 
	l->luxelToWorldSpace[1][1]  = (l->facenormal[2] * l->worldToLuxelSpace[0][0] - l->facenormal[0] * l->worldToLuxelSpace[0][2]) / det; 
	l->luxelOrigin[1]			= -(l->facedist * luxelSpaceCross[1]) / det;
	l->luxelToWorldSpace[0][2]  = (l->facenormal[1] * l->worldToLuxelSpace[1][0] - l->facenormal[0] * l->worldToLuxelSpace[1][1]) / det; 
	l->luxelToWorldSpace[1][2]  = (l->facenormal[0] * l->worldToLuxelSpace[0][1] - l->facenormal[1] * l->worldToLuxelSpace[0][0]) / det; 
	l->luxelOrigin[2]			= -(l->facedist * luxelSpaceCross[2]) / det;

	// adjust for luxel offset
	VectorMA( l->luxelOrigin, -tex->lightmapVecsLuxelsPerWorldUnits[0][3], l->luxelToWorldSpace[0], l->luxelOrigin );
	VectorMA( l->luxelOrigin, -tex->lightmapVecsLuxelsPerWorldUnits[1][3], l->luxelToWorldSpace[1], l->luxelOrigin );

	// compensate for org'd bmodels
	VectorAdd (l->luxelOrigin, l->modelorg, l->luxelOrigin);
}



winding_t *LightmapCoordWindingForFace( lightinfo_t *l )
{
	int			i;
	winding_t	*w;

	w = WindingFromFace( l->face, l->modelorg );

	for (i = 0; i < w->numpoints; i++)
	{
		Vector2D coord;
		WorldToLuxelSpace( l, w->p[i], coord );
		w->p[i].x = coord.x;
		w->p[i].y = coord.y;
		w->p[i].z = 0;
	}

	return w;
}


void WriteCoordWinding (FILE *out, lightinfo_t *l, winding_t *w, Vector& color )
{
	int			i;
	Vector		pos;

	fprintf (out, "%i\n", w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		LuxelSpaceToWorld( l, w->p[i][0], w->p[i][1], pos );
		fprintf (out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n",
				 pos[0],
				 pos[1],
				 pos[2],
				 color[ 0 ] / 256,
				 color[ 1 ] / 256,
				 color[ 2 ] / 256 );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DumpFaces( lightinfo_t *pLightInfo, int ndxFace )
{
	static	FileHandle_t out;

	// get face data
	faceneighbor_t *fn = &faceneighbor[ndxFace];
	Vector &centroid = face_centroids[ndxFace];

	// disable threading (not a multi-threadable function!)
	ThreadLock();
	
	if( !out )
	{
		// open the file
		out = g_pFileSystem->Open( "face.txt", "w" );
		if( !out )
			return;
	}
	
	//
	// write out face
	//
	for( int ndxEdge = 0; ndxEdge < pLightInfo->face->numedges; ndxEdge++ )
	{
//		int edge = dsurfedges[pLightInfo->face->firstedge+ndxEdge];

		Vector p1, p2;
		VectorAdd( dvertexes[EdgeVertex( pLightInfo->face, ndxEdge )].point, pLightInfo->modelorg, p1 );
		VectorAdd( dvertexes[EdgeVertex( pLightInfo->face, ndxEdge+1 )].point, pLightInfo->modelorg, p2 );
		
		Vector &n1 = fn->normal[ndxEdge];
		Vector &n2 = fn->normal[(ndxEdge+1)%pLightInfo->face->numedges];
		
		CmdLib_FPrintf( out, "3\n");
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", p1[0], p1[1], p1[2], n1[0] * 0.5 + 0.5, n1[1] * 0.5 + 0.5, n1[2] * 0.5 + 0.5 );
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", p2[0], p2[1], p2[2], n2[0] * 0.5 + 0.5, n2[1] * 0.5 + 0.5, n2[2] * 0.5 + 0.5 );
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", centroid[0] + pLightInfo->modelorg[0], 
					   centroid[1] + pLightInfo->modelorg[1], 
					   centroid[2] + pLightInfo->modelorg[2], 
					   fn->facenormal[0] * 0.5 + 0.5, 
					   fn->facenormal[1] * 0.5 + 0.5, 
					   fn->facenormal[2] * 0.5 + 0.5 );
		
	}
	
	// enable threading
	ThreadUnlock();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFacesamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// ratio of world area / lightmap area
	texinfo_t *pTex = &texinfo[pLightInfo->face->texinfo];
	pFaceLight->worldAreaPerLuxel = 1.0 / ( sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[0], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[0] ) ) * 
											sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[1], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[1] ) ) );

	//
	// quickly create samples and luxels (copy over samples)
	//
	pFaceLight->numsamples = width * height;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		return false;

	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	if( !pFaceLight->luxel )
		return false;

	sample_t *pSamples = pFaceLight->sample;
	Vector	 *pLuxels = pFaceLight->luxel;

	for( int t = 0; t < height; t++ )
	{
		for( int s = 0; s < width; s++ )
		{
			pSamples->s = s;
			pSamples->t = t;
			pSamples->coord[0] = s;
			pSamples->coord[1] = t;
			// unused but initialized anyway
			pSamples->mins[0] = s - 0.5;
			pSamples->mins[1] = t - 0.5;
			pSamples->maxs[0] = s + 0.5;
			pSamples->maxs[1] = t + 0.5;
			pSamples->area = pFaceLight->worldAreaPerLuxel;
			LuxelSpaceToWorld( pLightInfo, pSamples->coord[0], pSamples->coord[1], pSamples->pos );
			VectorCopy( pSamples->pos, *pLuxels );

			pSamples++;
			pLuxels++;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildSamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build samples for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFacesamplesAndLuxels_DoFast( pLightInfo, pFaceLight );
	}
	// build samples for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispSamplesAndLuxels_DoFast( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFacesamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// ratio of world area / lightmap area
	texinfo_t *pTex = &texinfo[pLightInfo->face->texinfo];
	pFaceLight->worldAreaPerLuxel = 1.0 / ( sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[0], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[0] ) ) * 
											sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[1], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[1] ) ) );

	// allocate a large number of samples for creation -- get copied later!
	char sampleData[sizeof(sample_t)*SINGLE_BRUSH_MAP*2];
	sample_t *samples = (sample_t*)sampleData; // use a char array to speed up the debug version.
	sample_t *pSamples = samples;

	// lightmap space winding
	winding_t *pLightmapWinding = LightmapCoordWindingForFace( pLightInfo ); 

	//
	// build vector pointing along the lightmap cutting planes
	//
	Vector sNorm( 1.0f, 0.0f, 0.0f );
	Vector tNorm( 0.0f, 1.0f, 0.0f );

	// sample center offset
	float sampleOffset = ( do_centersamples ) ? 0.5 : 1.0;

	//
	// clip the lightmap "spaced" winding by the lightmap cutting planes
	//
	winding_t *pWindingT1, *pWindingT2;
	winding_t *pWindingS1, *pWindingS2;
	float dist;

	for( int t = 0; t < height && pLightmapWinding; t++ )
	{
		dist = t + sampleOffset;
		
		// lop off a sample in the t dimension
		// hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
		ClipWindingEpsilon( pLightmapWinding, tNorm, dist, ON_EPSILON / 16.0f, &pWindingT1, &pWindingT2 );

		for( int s = 0; s < width && pWindingT2; s++ )
		{
			dist = s + sampleOffset;

			// lop off a sample in the s dimension, and put it in ws2
			// hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
			ClipWindingEpsilon( pWindingT2, sNorm, dist, ON_EPSILON / 16.0f, &pWindingS1, &pWindingS2 );

			//
			// s2 winding is a single sample worth of winding
			//
			if( pWindingS2 )
			{
				// save the s, t positions
				pSamples->s = s;
				pSamples->t = t;

				// get the lightmap space area of ws2 and convert to world area
				// and find the center (then convert it to 2D)
				Vector center;
				pSamples->area = WindingAreaAndBalancePoint(  pWindingS2, center ) * pFaceLight->worldAreaPerLuxel;
				pSamples->coord[0] = center.x; 
				pSamples->coord[1] = center.y;

				// find winding bounds (then convert it to 2D)
				Vector minbounds, maxbounds;
				WindingBounds( pWindingS2, minbounds, maxbounds );
				pSamples->mins[0] = minbounds.x; 
				pSamples->mins[1] = minbounds.y;
				pSamples->maxs[0] = maxbounds.x; 
				pSamples->maxs[1] = maxbounds.y;

				// convert from lightmap space to world space
				LuxelSpaceToWorld( pLightInfo, pSamples->coord[0], pSamples->coord[1], pSamples->pos );

				if (dumppatches || (do_extra && pSamples->area < pFaceLight->worldAreaPerLuxel - EQUAL_EPSILON))
				{
					//
					// convert the winding from lightmaps space to world for debug rendering and sub-sampling
					//
					Vector worldPos;
					for( int ndxPt = 0; ndxPt < pWindingS2->numpoints; ndxPt++ )
					{
						LuxelSpaceToWorld( pLightInfo, pWindingS2->p[ndxPt].x, pWindingS2->p[ndxPt].y, worldPos );
						VectorCopy( worldPos, pWindingS2->p[ndxPt] );
					}
					pSamples->w = pWindingS2;
				}
				else
				{
					// winding isn't needed, free it.
					pSamples->w = NULL;
					FreeWinding( pWindingS2 );
				}

				pSamples++;
			}

			//
			// if winding T2 still exists free it and set it equal S1 (the rest of the row minus the sample just created)
			//
			if( pWindingT2 )
			{
				FreeWinding( pWindingT2 );
			}

			// clip the rest of "s"
			pWindingT2 = pWindingS1;
		}

		//
		// if the original lightmap winding exists free it and set it equal to T1 (the rest of the winding not cut into samples) 
		//
		if( pLightmapWinding )
		{
			FreeWinding( pLightmapWinding );
		}

		if( pWindingT2 )
		{
			FreeWinding( pWindingT2 );
		}

		pLightmapWinding = pWindingT1;
	}

	//
	// copy over samples
	//
	pFaceLight->numsamples = pSamples - samples;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		return false;

	memcpy( pFaceLight->sample, samples, pFaceLight->numsamples * sizeof( *pFaceLight->sample ) );

	// supply a default sample normal (face normal - assumed flat)
	for( int ndxSample = 0; ndxSample < pFaceLight->numsamples; ndxSample++ )
	{
		Assert ( VectorLength ( pLightInfo->facenormal ) > 1.0e-20);
		pFaceLight->sample[ndxSample].normal = pLightInfo->facenormal;
	}

	// statistics - warning?!
	if( pFaceLight->numsamples == 0 )
	{
		Msg( "no samples %d\n", pLightInfo->face - g_pFaces );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Free any windings used by this facelight. It's currently assumed they're not needed again
//-----------------------------------------------------------------------------
void FreeSampleWindings( facelight_t *fl )
{
	int i;
	for (i = 0; i < fl->numsamples; i++)
	{
		if (fl->sample[i].w)
		{
			FreeWinding( fl->sample[i].w );
			fl->sample[i].w = NULL;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: build the sample data for each lightmapped primitive type
//-----------------------------------------------------------------------------
bool BuildSamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build samples for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFacesamples( pLightInfo, pFaceLight );
	}
	// build samples for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispSamples( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFaceLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// calcuate actual luxel points
	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	if( !pFaceLight->luxel )
		return false;

	for( int t = 0; t < height; t++ )
	{
		for( int s = 0; s < width; s++ )
		{
			LuxelSpaceToWorld( pLightInfo, s, t, pFaceLight->luxel[s+t*width] );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: build the luxels (find the luxel centers) for each lightmapped
//          primitive type
//-----------------------------------------------------------------------------
bool BuildLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build luxels for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFaceLuxels( pLightInfo, pFaceLight );
	}
	// build luxels for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispLuxels( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
// Purpose: for each face, find the center of each luxel; for each texture
//          aligned grid point, back project onto the plane and get the world
//          xyz value of the sample point
// NOTE: ndxFace = facenum
//-----------------------------------------------------------------------------
void CalcPoints( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// debugging!
	if( dumppatches ) 
	{
		DumpFaces( pLightInfo, ndxFace );
	}

	// quick and dirty!
	if( do_fast )
	{
		if( !BuildSamplesAndLuxels_DoFast( pLightInfo, pFaceLight, ndxFace ) )
		{
			Msg( "Face %d: (Fast)Error Building Samples and Luxels\n", ndxFace );
		}
		return;
	}

	// build the samples
	if( !BuildSamples( pLightInfo, pFaceLight, ndxFace ) )
	{
		Msg( "Face %d: Error Building Samples\n", ndxFace );
	}

	// build the luxels
	if( !BuildLuxels( pLightInfo, pFaceLight, ndxFace ) )
	{
		Msg( "Face %d: Error Building Luxels\n", ndxFace );
	}
}


//==============================================================

directlight_t	*activelights;
directlight_t	*freelights;

facelight_t		facelight[MAX_MAP_FACES];
int				numdlights;

/*
  ==================
  FindTargetEntity
  ==================
*/
entity_t *FindTargetEntity (char *target)
{
	int		i;
	char	*n;

	for (i=0 ; i<num_entities ; i++)
	{
		n = ValueForKey (&entities[i], "targetname");
		if (!strcmp (n, target))
			return &entities[i];
	}

	return NULL;
}



/*
  =============
  AllocDLight
  =============
*/

int GetVisCache( int lastoffset, int cluster, byte *pvs );
void SetDLightVis( directlight_t *dl, int cluster );
void MergeDLightVis( directlight_t *dl, int cluster );

directlight_t *AllocDLight( Vector& origin, bool bAddToList )
{
	directlight_t *dl;

	dl = ( directlight_t* )calloc(1, sizeof(directlight_t));
	dl->index = numdlights++;

	VectorCopy( origin, dl->light.origin );

	dl->light.cluster = ClusterFromPoint(dl->light.origin);
	SetDLightVis( dl, dl->light.cluster );

	dl->facenum = -1;

	if ( bAddToList )
	{
		dl->next = activelights;
		activelights = dl;
	}

	return dl;
}

void AddDLightToActiveList( directlight_t *dl )
{
	dl->next = activelights;
	activelights = dl;
}

void FreeDLights()
{
	gSkyLight = NULL;
	gAmbient = NULL;

	directlight_t *pNext;
	for( directlight_t *pCur=activelights; pCur; pCur=pNext )
	{
		pNext = pCur->next;
		free( pCur );
	}
	activelights = 0;
}


void SetDLightVis( directlight_t *dl, int cluster )
{
	if (dl->pvs == NULL)
	{
		dl->pvs = (byte *)calloc( 1, (dvis->numclusters / 8) + 1 );
	}

	GetVisCache( -1, cluster, dl->pvs );
}

void MergeDLightVis( directlight_t *dl, int cluster )
{
	if (dl->pvs == NULL)
	{
		SetDLightVis( dl, cluster );
	}
	else
	{
		byte		pvs[MAX_MAP_CLUSTERS/8];
		GetVisCache( -1, cluster, pvs );

		// merge both vis graphs
		for (int i = 0; i < (dvis->numclusters / 8) + 1; i++)
		{
			dl->pvs[i] |= pvs[i];
		}
	}
}


/*
  =============
  LightForKey
  =============
*/
int LightForKey (entity_t *ent, char *key, Vector& intensity )
{
	char *pLight;

	pLight = ValueForKey( ent, key );

	return LightForString( pLight, intensity );
}

int LightForString( char *pLight, Vector& intensity )
{
	double r, g, b, scaler;
	int argCnt;

	VectorFill( intensity, 0 );

	// scanf into doubles, then assign, so it is vec_t size independent
	r = g = b = scaler = 0;
	double r_hdr,g_hdr,b_hdr,scaler_hdr;
	argCnt = sscanf ( pLight, "%lf %lf %lf %lf %lf %lf %lf %lf", 
					  &r, &g, &b, &scaler, &r_hdr,&g_hdr,&b_hdr,&scaler_hdr );

	// This is a special case for HDR lights.  If we have a vector of [-1, -1, -1, 1], then we
	// need to fall back to the non-HDR lighting since the HDR lighting hasn't been defined
	// for this light source.
	if( ( argCnt == 3 && r == -1.0f && g == -1.0f && b == -1.0f ) ||
		( argCnt == 4 && r == -1.0f && g == -1.0f && b == -1.0f && scaler == 1.0f ) )
	{
		intensity.Init( -1.0f, -1.0f, -1.0f );
		return true;
	}

	if (argCnt==8) 											// 2 4-tuples
	{
		if (g_bHDR)
		{
			r=r_hdr;
			g=g_hdr;
			b=b_hdr;
			scaler=scaler_hdr;
		}
		argCnt=4;
	}

	intensity[0] = pow( r / 255.0, 2.2 ) * 255;				// convert to linear
	
	switch( argCnt)
	{
		case 1:
			// The R,G,B values are all equal.
			intensity[1] = intensity[2] = intensity[0]; 
			break;
			
		case 3:
		case 4:
			// Save the other two G,B values.
			intensity[1] = pow( g / 255.0, 2.2 ) * 255;
			intensity[2] = pow( b / 255.0, 2.2 ) * 255;
			
			// Did we also get an "intensity" scaler value too?
			if ( argCnt == 4 )
			{
				// Scale the normalized 0-255 R,G,B values by the intensity scaler
				VectorScale( intensity, scaler / 255.0, intensity );
			}
			break;

		default:
			printf("unknown light specifier type - %s\n",pLight);
			return false;
	}
	// scale up source lights by scaling factor
	VectorScale( intensity, lightscale, intensity );
	return true;
}

//-----------------------------------------------------------------------------
// Various parsing methods
//-----------------------------------------------------------------------------

static void ParseLightGeneric( entity_t *e, directlight_t *dl )
{
	entity_t		*e2;
	char	        *target;
	Vector	        dest;

	dl->light.style = (int)FloatForKey (e, "style");
	
	// get intenfsity
	if( g_bHDR )
	{
		if( LightForKey( e, "_lightHDR", dl->light.intensity ) == 0 ||
			( dl->light.intensity.x == -1.0f && 
			  dl->light.intensity.y == -1.0f && 
			  dl->light.intensity.z == -1.0f ) )
		{
			LightForKey( e, "_light", dl->light.intensity );
		}
	}
	else
	{
		LightForKey( e, "_light", dl->light.intensity );
	}
	
	// check angle, targets
	target = ValueForKey (e, "target");
	if (target[0])
	{	// point towards target
		e2 = FindTargetEntity (target);
		if (!e2)
			Warning("WARNING: light at (%i %i %i) has missing target\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
		else
		{
			GetVectorForKey (e2, "origin", dest);
			VectorSubtract (dest, dl->light.origin, dl->light.normal);
			VectorNormalize (dl->light.normal);
		}
	}
	else
	{	
		// point down angle
		Vector angles;
		GetVectorForKey( e, "angles", angles );
		float pitch = FloatForKey (e, "pitch");
		float angle = FloatForKey (e, "angle");
		SetupLightNormalFromProps( QAngle( angles.x, angles.y, angles.z ), angle, pitch, dl->light.normal );
	}
}

static void SetLightFalloffParams(entity_t *e, directlight_t *dl)
{
	float d50=FloatForKey(e,"_fifty_percent_distance");
	if (d50)
	{
		float d0=FloatForKey(e,"_zero_percent_distance");
		if (d0<d50)
		{
			Warning("light has _fifty_percent_distance of %f but no zero_percent_distance\n",d50);
			d0=2.0*d50;
		}
		float a=0,b=1,c=0;
		if (! SolveInverseQuadraticMonotonic(0,1.0,d50,2.0,d0,256.0,a,b,c))
		{
			Warning("can't solve quadratic for light %f %f\n",d50,d0);
		}
		// it it possible that the parameters couldn't be used because of enforing monoticity. If so, rescale so at
		// least the 50 percent value is right
//		printf("50 percent=%f 0 percent=%f\n",d50,d0);
// 		printf("a=%f b=%f c=%f\n",a,b,c);
		float v50=c+d50*(b+d50*a);
		float scale=2.0/v50;
		a*=scale;
		b*=scale;
		c*=scale;
// 		printf("scaled=%f a=%f b=%f c=%f\n",scale,a,b,c);
// 		for(float d=0;d<1000;d+=20)
// 			printf("at %f, %f\n",d,1.0/(c+d*(b+d*a)));
		dl->light.quadratic_attn=a;
		dl->light.linear_attn=b;
		dl->light.constant_attn=c;
	}
	else
	{
		dl->light.constant_attn = FloatForKey (e, "_constant_attn");
		dl->light.linear_attn = FloatForKey (e, "_linear_attn");
		dl->light.quadratic_attn = FloatForKey (e, "_quadratic_attn");
		
		dl->light.radius = FloatForKey (e, "_distance");
		
		// clamp values to >= 0
		if (dl->light.constant_attn < EQUAL_EPSILON)
			dl->light.constant_attn = 0;
		
		if (dl->light.linear_attn < EQUAL_EPSILON)
			dl->light.linear_attn = 0;
		
		if (dl->light.quadratic_attn < EQUAL_EPSILON)
			dl->light.quadratic_attn = 0;
		
		if (dl->light.constant_attn < EQUAL_EPSILON && dl->light.linear_attn < EQUAL_EPSILON && dl->light.quadratic_attn < EQUAL_EPSILON)
			dl->light.constant_attn = 1;
		
		// scale intensity for unit 100 distance
		float ratio = (dl->light.constant_attn + 100 * dl->light.linear_attn + 100 * 100 * dl->light.quadratic_attn);
		if (ratio > 0)
		{
			VectorScale( dl->light.intensity, ratio, dl->light.intensity );
		}
	}
}

static void ParseLightSpot( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, true );

	ParseLightGeneric( e, dl );

	dl->light.type = emit_spotlight;

	dl->light.stopdot = FloatForKey (e, "_inner_cone");
	if (!dl->light.stopdot)
		dl->light.stopdot = 10;

	dl->light.stopdot2 = FloatForKey (e, "_cone");
	if (!dl->light.stopdot2) 
		dl->light.stopdot2 = dl->light.stopdot;
	if (dl->light.stopdot2 < dl->light.stopdot)
		dl->light.stopdot2 = dl->light.stopdot;

	// This is a point light if stop dots are 180...
	if ((dl->light.stopdot == 180) && (dl->light.stopdot2 == 180))
	{
		dl->light.stopdot = dl->light.stopdot2 = 0;
		dl->light.type = emit_point;
		dl->light.exponent = 0;
	}
	else
	{
		// Clamp to 90, that's all DX8 can handle! 
		if (dl->light.stopdot > 90)
		{
			Warning("WARNING: light_spot at (%i %i %i) has inner angle larger than 90 degrees! Clamping to 90...\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
			dl->light.stopdot = 90;
		}

		if (dl->light.stopdot2 > 90)
		{
			Warning("WARNING: light_spot at (%i %i %i) has outer angle larger than 90 degrees! Clamping to 90...\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
			dl->light.stopdot2 = 90;
		}

		dl->light.stopdot2 = (float)cos(dl->light.stopdot2/180*M_PI);
		dl->light.stopdot = (float)cos(dl->light.stopdot/180*M_PI);
		dl->light.exponent = FloatForKey (e, "_exponent");
	}

	SetLightFalloffParams(e,dl);
}

// NOTE: This is just a heuristic.  It traces a finite number of rays to find sky
// NOTE: Full vis is necessary to make this 100% correct.
bool CanLeafTraceToSky( int iLeaf )
{
	// UNDONE: Really want a point inside the leaf here.  Center is a guess, may not be in the leaf
	// UNDONE: Clip this to each plane bounding the leaf to guarantee
	Vector center = vec3_origin;
	for ( int i = 0; i < 3; i++ )
	{
		center[i] = ( (float)(dleafs[iLeaf].mins[i] + dleafs[iLeaf].maxs[i]) ) * 0.5f;
	}

	Vector delta;
	for ( int j = 0; j < NUMVERTEXNORMALS; j++ )
	{
		// search back to see if we can hit a sky brush
		VectorScale( g_anorms[j], -MAX_TRACE_LENGTH, delta );
		VectorAdd( center, delta, delta );

		texinfo_t *tx = TestLine_Surface( 0, center, delta, 0 );

		if (tx == NULL || !(tx->flags & SURF_SKY))
			continue;	// no sky here
		return true;
	}

	return false;
}

void BuildVisForLightEnvironment( void )
{
	// Create the vis.
	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		dleafs[iLeaf].flags &= ~LEAF_FLAGS_SKY;
		unsigned int iFirstFace = dleafs[iLeaf].firstleafface;
		for ( int iLeafFace = 0; iLeafFace < dleafs[iLeaf].numleaffaces; ++iLeafFace )
		{
			unsigned int iFace = dleaffaces[iFirstFace+iLeafFace];
			
			texinfo_t &tex = texinfo[g_pFaces[iFace].texinfo];
			if ( tex.flags & SURF_SKY )
			{
				dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
				MergeDLightVis( gSkyLight, dleafs[iLeaf].cluster );
				MergeDLightVis( gAmbient, dleafs[iLeaf].cluster );
				break;
			}
		}
	}

	// Second pass to set flags on leaves that don't contain sky, but touch leaves that
	// contain sky.
	byte pvs[MAX_MAP_CLUSTERS / 8];

	int nLeafBytes = (numleafs >> 3) + 1;
	unsigned char *pLeafBits = (unsigned char *)stackalloc( nLeafBytes * sizeof(unsigned char) );
	memset( pLeafBits, 0, nLeafBytes );

	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		// If this leaf has light in it, then don't bother
		if ( dleafs[iLeaf].flags & LEAF_FLAGS_SKY )
			continue;

		// Don't bother with this leaf if it's solid
		if ( dleafs[iLeaf].contents & CONTENTS_SOLID )
			continue;

		// See what other leaves are visible from this leaf
		GetVisCache( -1, dleafs[iLeaf].cluster, pvs );

		// Now check out all other leaves
		for ( int iLeaf2 = 0; iLeaf2 < numleafs; ++iLeaf2 )
		{
			if ( iLeaf2 == iLeaf )
				continue;

			if ( !(dleafs[iLeaf2].flags & LEAF_FLAGS_SKY) )
				continue;

			// Can this leaf see into the leaf with the sky in it?
			if ( PVSCheck( pvs, dleafs[iLeaf2].cluster ) )
			{
				pLeafBits[ iLeaf >> 3 ] |= 1 << ( iLeaf & 0x7 );
				break;
			}
		}
	}

	// Must set the bits in a separate pass so as to not flood-fill LEAF_FLAGS_SKY everywhere
	// pLeafbits is a bit array of all leaves that need to be marked as seeing sky
	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		if ( dleafs[iLeaf].flags & LEAF_FLAGS_SKY )
			continue;

		// Don't bother with this leaf if it's solid
		if ( dleafs[iLeaf].contents & CONTENTS_SOLID )
			continue;

		if ( pLeafBits[ iLeaf >> 3 ] & (1 << ( iLeaf & 0x7 )) )
		{
			dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
		}
		else
		{
			// if radial vis was used on this leaf some of the portals leading
			// to sky may have been culled.  Try tracing to find sky.
			if ( dleafs[iLeaf].flags & LEAF_FLAGS_RADIAL )
			{
				if ( CanLeafTraceToSky(iLeaf) )
				{
					dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
				}
			}
		}
	}
}

static void ParseLightEnvironment( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, false );

	ParseLightGeneric( e, dl );

	if ( !gSkyLight )
	{
		// Sky light.
		gSkyLight = dl;
		dl->light.type = emit_skylight;

		// Sky ambient light.
		gAmbient = AllocDLight( dl->light.origin, false );
		gAmbient->light.type = emit_skyambient;
		if( g_bHDR && LightForKey( e, "_ambientHDR", gAmbient->light.intensity ) &&
			gAmbient->light.intensity.x != -1.0f && 
			gAmbient->light.intensity.y != -1.0f && 
			gAmbient->light.intensity.z != -1.0f )
		{
			// we have a valid HDR ambient light value
		}
		else if ( !LightForKey( e, "_ambient", gAmbient->light.intensity ) )
		{
			VectorScale( dl->light.intensity, 0.5, gAmbient->light.intensity );
		}
		
		BuildVisForLightEnvironment();

		// Add sky and sky ambient lights to the list.
		AddDLightToActiveList( gSkyLight );
		AddDLightToActiveList( gAmbient );
	}
}

static void ParseLightPoint( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, true );

	ParseLightGeneric( e, dl );

	dl->light.type = emit_point;

	SetLightFalloffParams(e,dl);
}

/*
  =============
  CreateDirectLights
  =============
*/
#define DIRECT_SCALE (100.0*100.0)
void CreateDirectLights (void)
{
	unsigned        i;
	patch_t	        *p = NULL;
	directlight_t	*dl = NULL;
	entity_t	    *e = NULL;
	char	        *name;
	Vector	        dest;

	numdlights = 0;

	FreeDLights();

	//
	// surfaces
	//
	unsigned int uiPatchCount = patches.Size();
	for (i=0; i< uiPatchCount; i++)
	{
		p = &patches.Element( i );

		// skip parent patches
		if (p->child1 != patches.InvalidIndex() )
			continue;

		if (p->basearea < 1e-6)
			continue;

		if( VectorAvg( p->baselight ) >= dlight_threshold )
		{
			dl = AllocDLight( p->origin, true );

			dl->light.type = emit_surface;
			VectorCopy (p->normal, dl->light.normal);
			Assert( VectorLength( p->normal ) > 1.0e-20 );
			// scale intensity by number of texture instances
			VectorScale( p->baselight, lightscale * p->area * p->scale[0] * p->scale[1] / p->basearea, dl->light.intensity );

			// scale to a range that results in actual light
			VectorScale( dl->light.intensity, DIRECT_SCALE, dl->light.intensity );
		}
	}
	
	//
	// entities
	//
	for (i=0 ; i<(unsigned)num_entities ; i++)
	{
		e = &entities[i];
		name = ValueForKey (e, "classname");
		if (strncmp (name, "light", 5))
			continue;

		// Light_dynamic is actually a real entity; not to be included here...
		if (!strcmp (name, "light_dynamic"))
			continue;

		if (!strcmp (name, "light_spot"))
		{
			ParseLightSpot( e, dl );
		}
		else if (!strcmp(name, "light_environment")) 
		{
			ParseLightEnvironment( e, dl );
		}
		else if (!strcmp(name, "light")) 
		{
			ParseLightPoint( e, dl );
		}
		else
		{
			qprintf( "unsupported light entity: \"%s\"\n", name );
		}
	}

	qprintf ("%i direct lights\n", numdlights);
	// exit(1);
}

/*
  =============
  ExportDirectLightsToWorldLights
  =============
*/

void ExportDirectLightsToWorldLights()
{
	directlight_t		*dl;

	// In case the level has already been VRADed.
	*pNumworldlights = 0;

	for (dl = activelights; dl != NULL; dl = dl->next )
	{
		dworldlight_t *wl = &dworldlights[(*pNumworldlights)++];

		if (*pNumworldlights > MAX_MAP_WORLDLIGHTS)
		{
			Error("too many lights %d / %d\n", *pNumworldlights, MAX_MAP_WORLDLIGHTS );
		}

		wl->cluster	= dl->light.cluster;
		wl->type	= dl->light.type;
		wl->style	= dl->light.style;
		VectorCopy( dl->light.origin, wl->origin );
		// FIXME: why does vrad want 0 to 255 and not 0 to 1??
		VectorScale( dl->light.intensity, (1.0 / 255.0), wl->intensity );
		VectorCopy( dl->light.normal, wl->normal );
		wl->stopdot	= dl->light.stopdot;
		wl->stopdot2 = dl->light.stopdot2;
		wl->exponent = dl->light.exponent;
		wl->radius = dl->light.radius;
		wl->constant_attn = dl->light.constant_attn;
		wl->linear_attn = dl->light.linear_attn;
		wl->quadratic_attn = dl->light.quadratic_attn;
		wl->flags = 0;
	}
}

/*
  =============
  GatherSampleLight
  =============
*/
#define NORMALFORMFACTOR	40.156979 // accumuated dot products for hemisphere

#define NSAMPLES_SUN_AREA_LIGHT 30							// number of samples to take for an
                                                            // non-point sun light

// returns dot product with normal and delta
// dl - light
// pos - position of sample
// normal - surface normal of sample
// out.dot[] - returned dot products with light vector and each normal
// out.falloff - amount of light falloff
bool GatherSampleLight( sampleLightOutput_t &out, directlight_t *dl, int facenum, 
						Vector const& pos, Vector *pNormals, int normalCount, int iThread,
						bool force_fast,
						int static_prop_index_to_ignore)
{
	float			dot, dot2;
	float			dist;
	Vector			delta;

	Assert( normalCount <= (NUM_BUMP_VECTS+1) );

	// skylights work fundamentally differently than normal lights
	if (dl->light.type == emit_skylight)
	{
		// make sure the angle is okay
		dot = -DotProduct( pNormals[0], dl->light.normal );
		if (dot <= EQUAL_EPSILON)
			return false;

		int nsamples=1;
		if (g_SunAngularExtent>0.0)
		{
			nsamples=NSAMPLES_SUN_AREA_LIGHT;
			if (do_fast || force_fast )
				nsamples/=4;
		}
		
		int nmisses=0;
		DirectionalSampler_t ldisp;

		for(int d=0;d<nsamples;d++)
		{
			// now, determine visibility of the skylight
			// search back to see if we can hit a sky brush
			VectorScale( dl->light.normal, -MAX_TRACE_LENGTH, delta );
			if (d)
			{
				// jitter light source location
				Vector ofs=ldisp.NextValue();
				ofs*=(MAX_TRACE_LENGTH*g_SunAngularExtent);
				delta+=ofs;
			}
			VectorAdd( pos, delta, delta );

			texinfo_t *tx = TestLine_Surface( 0, pos, delta, iThread, true, 
											  static_prop_index_to_ignore );

			if (tx == NULL || !(tx->flags & SURF_SKY))
				nmisses++;
		}
		if (nmisses==nsamples)
			return false;
		float see_percent=(nsamples-nmisses)*(1.0/nsamples);
		out.dot[0] = dot*see_percent;

		out.falloff = 1.0f;
		for ( int i = 1; i < normalCount; i++ )
		{
			out.dot[i] = -DotProduct( pNormals[i], dl->light.normal );
		}
		return true;
	} 
	else if (dl->light.type == emit_skyambient)
	{
		float sumdot=0;
		float ambient_intensity[NUM_BUMP_VECTS+1];
		int i, j;
		int possibleHitCount[NUM_BUMP_VECTS+1];
		float dots[NUM_BUMP_VECTS+1];

		for ( i = 0; i < normalCount; i++ )
		{
			ambient_intensity[i] = 0;
		}

		// count all of the directions that could have projected something on to the bump basis normal
		memset( possibleHitCount, 0, sizeof(possibleHitCount) );

		DirectionalSampler_t sampler;
		int nsky_samples=NUMVERTEXNORMALS;
		if (do_fast || force_fast )
			nsky_samples/=4;
		if (g_dofinal)
			nsky_samples*=16;

		for (j = 0; j < nsky_samples; j++)
		{
			// make sure the angle is okay
			Vector anorm=sampler.NextValue();
			dots[0] = -DotProduct( pNormals[0], anorm );
			if (dots[0] <= EQUAL_EPSILON)
				continue;

			sumdot+=dots[0];
			possibleHitCount[0]++;
			for ( i = 1; i < normalCount; i++ )
			{
				dots[i] = -DotProduct( pNormals[i], anorm );
				if ( dots[i] <= EQUAL_EPSILON )
				{
					dots[i] = 0;
				}
				else
				{
					possibleHitCount[i]++;
				}
			}
			// search back to see if we can hit a sky brush
			VectorScale( anorm, -MAX_TRACE_LENGTH, delta );
			VectorAdd( pos, delta, delta );

			texinfo_t *tx = TestLine_Surface( 0, pos, delta, iThread, true,
											  static_prop_index_to_ignore );

			if (tx == NULL || !(tx->flags & SURF_SKY))
				// if (!tx || tx->texdata != dl->texdata)
				continue;	// occluded

			for ( i = 0; i < normalCount; i++ )
			{
				ambient_intensity[i] += dots[i];
			}
		}

		out.falloff = 1.0f;
		for ( i = 0; i < normalCount; i++ )
		{
			// now scale out the missing parts of the hemisphere of this bump basis vector
			float factor = (float)possibleHitCount[i] / (float)possibleHitCount[0];
			out.dot[i] = ambient_intensity[i] / (factor * sumdot);
		}
		return true;
	}
	else
	{
		Vector	src;

		if (dl->facenum == -1)
		{
			VectorCopy( dl->light.origin, src );
		}
		else
		{
			src.Init( 0, 0, 0 );
		}

		VectorSubtract (src, pos, delta);
		dist = VectorNormalize (delta);
		dot = DotProduct (delta, pNormals[0]);
		if (dot <= EQUAL_EPSILON)
			return false;	// behind sample surface

		if (dist < 1.0)
			dist = 1.0;

		switch (dl->light.type)
		{
			case emit_point:
				out.falloff = 1.0 / (dl->light.constant_attn + dl->light.linear_attn * dist + dl->light.quadratic_attn * dist * dist);
				break;

			case emit_surface:
				dot2 = -DotProduct (delta, dl->light.normal);
				if (dot2 <= EQUAL_EPSILON)
					return false; // behind light surface
				out.falloff = dot2 / (dist * dist);
				break;

			case emit_spotlight:
				dot2 = -DotProduct (delta, dl->light.normal);
				if (dot2 <= dl->light.stopdot2)
					return false; // outside light cone

				out.falloff = dot2 / (dl->light.constant_attn + dl->light.linear_attn * dist + dl->light.quadratic_attn * dist * dist);
				if (dot2 <= dl->light.stopdot) // outside inner cone
				{
					if ((dl->light.exponent == 0.0f) || (dl->light.exponent == 1.0f))
						out.falloff *= (dot2 - dl->light.stopdot2) / (dl->light.stopdot - dl->light.stopdot2);
					else
						out.falloff *= pow((dot2 - dl->light.stopdot2) / (dl->light.stopdot - dl->light.stopdot2), dl->light.exponent);
				}
				break;

			default:
				Error ("Bad dl->light.type");
				return false;
		}

		if ( TestLine (pos, src, 0, iThread, static_prop_index_to_ignore) != CONTENTS_EMPTY )
			return false;	// occluded
	}
	out.dot[0] = dot;
	for ( int i = 1; i < normalCount; i++ )
	{
		out.dot[i] = DotProduct( pNormals[i], delta );
	}
	return true;
}



/*
  =============
  AddSampleToPatch

  Take the sample's collected light and
  add it back into the apropriate patch
  for the radiosity pass.
  =============
*/
void AddSampleToPatch (sample_t *s, Vector& light, int facenum)
{
	patch_t	*patch;
	Vector	mins, maxs;
	int		i;

	if (numbounce == 0)
		return;
	if( VectorAvg( light ) < 1)
		return;

	//
	// fixed the sample position and normal -- need to find the equiv pos, etc to set up 
	// patches
	//
	if( facePatches.Element( facenum ) == facePatches.InvalidIndex() )
		return;

	bool bDisp = ( g_pFaces[facenum].dispinfo != -1 );

	float radius = sqrt( s->area ) / 2.0;

	patch_t *pNextPatch = NULL;
	for( patch = &patches.Element( facePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != patches.InvalidIndex() )
		{
			pNextPatch = &patches.Element( patch->ndxNext );
		}

		if (patch->sky)
			continue;

		// skip patches with children
		if ( patch->child1 != patches.InvalidIndex() )
		 	continue;

		// see if the point is in this patch (roughly)
		if( !bDisp )
		{
			WindingBounds (patch->winding, mins, maxs);
		}
		else
		{
			mins = patch->mins;
			maxs = patch->maxs;
		}

		for (i=0 ; i<3 ; i++)
		{
			if (mins[i] > s->pos[i] + radius)
				goto nextpatch;
			if (maxs[i] < s->pos[i] - radius)
				goto nextpatch;
		}

		// add the sample to the patch
		patch->samplearea += s->area;
		VectorMA( patch->samplelight, s->area, light, patch->samplelight );

 nextpatch:;
	}
	// don't worry if some samples don't find a patch
}


void GetPhongNormal( int facenum, Vector const& spot, Vector& phongnormal )
{
	int	j;
	dface_t		*f = &g_pFaces[facenum];
//	dplane_t	*p = &dplanes[f->planenum];
	Vector		facenormal, vspot;

	VectorCopy( dplanes[f->planenum].normal, facenormal );
	VectorCopy( facenormal, phongnormal );

	if ( smoothing_threshold != 1 )
	{
		faceneighbor_t *fn = &faceneighbor[facenum];

		// Calculate modified point normal for surface
		// Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
		// Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
		// Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
		// Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

		for (j=0 ; j<f->numedges ; j++)
		{
			Vector	v1, v2;
			//int e = dsurfedges[f->firstedge + j];
			//int e1 = dsurfedges[f->firstedge + ((j+f->numedges-1)%f->numedges)];
			//int e2 = dsurfedges[f->firstedge + ((j+1)%f->numedges)];

			//edgeshare_t	*es = &edgeshare[abs(e)];
			//edgeshare_t	*es1 = &edgeshare[abs(e1)];
			//edgeshare_t	*es2 = &edgeshare[abs(e2)];
			// dface_t	*f2;
			float		a1, a2, aa, bb, ab;
			int			vert1, vert2;

			Vector& n1 = fn->normal[j];
			Vector& n2 = fn->normal[(j+1)%f->numedges];

			/*
			  if (VectorCompare( n1, fn->facenormal ) 
			  && VectorCompare( n2, fn->facenormal) )
			  continue;
			*/

			vert1 = EdgeVertex( f, j );
			vert2 = EdgeVertex( f, j+1 );

			Vector& p1 = dvertexes[vert1].point;
			Vector& p2 = dvertexes[vert2].point;

			// Build vectors from the middle of the face to the edge vertexes and the sample pos.
			VectorSubtract( p1, face_centroids[facenum], v1 );
			VectorSubtract( p2, face_centroids[facenum], v2 );
			VectorSubtract( spot, face_centroids[facenum], vspot );
			aa = DotProduct( v1, v1 );
			bb = DotProduct( v2, v2 );
			ab = DotProduct( v1, v2 );
			a1 = (bb * DotProduct( v1, vspot ) - ab * DotProduct( vspot, v2 )) / (aa * bb - ab * ab);
			a2 = (DotProduct( vspot, v2 ) - a1 * ab) / bb;

			// Test center to sample vector for inclusion between center to vertex vectors (Use dot product of vectors)
			if ( a1 >= 0.0 && a2 >= 0.0)
			{
				// calculate distance from edge to pos
				Vector	temp;
				float scale;
				
				// Interpolate between the center and edge normals based on sample position
				scale = 1.0 - a1 - a2;
				VectorScale( fn->facenormal, scale, phongnormal );
				VectorScale( n1, a1, temp );
				VectorAdd( phongnormal, temp, phongnormal );
				VectorScale( n2, a2, temp );
				VectorAdd( phongnormal, temp, phongnormal );
				Assert( VectorLength( phongnormal ) > 1.0e-20 );
				VectorNormalize( phongnormal );

				/*
				  if (a1 > 1 || a2 > 1 || a1 + a2 > 1)
				  {
				  Msg("\n%.2f %.2f\n", a1, a2 );
				  Msg("%.2f %.2f %.2f\n", v1[0], v1[1], v1[2] );
				  Msg("%.2f %.2f %.2f\n", v2[0], v2[1], v2[2] );
				  Msg("%.2f %.2f %.2f\n", vspot[0], vspot[1], vspot[2] );
				  exit(1);

				  a1 = 0;
				  }
				*/
				/*
				  phongnormal[0] = (((j + 1) & 4) != 0) * 255;
				  phongnormal[1] = (((j + 1) & 2) != 0) * 255;
				  phongnormal[2] = (((j + 1) & 1) != 0) * 255;
				*/
				return;
			}
		}
	}
}





int GetVisCache( int lastoffset, int cluster, byte *pvs )
{
	// get the PVS for the pos to limit the number of checks
    if ( !visdatasize )
    {       
        memset (pvs, 255, (dvis->numclusters+7)/8 );
        lastoffset = -1;
    }
    else 
    {
		if (cluster < 0)
		{
			// Error, point embedded in wall
			// sampled[0][1] = 255;
			memset (pvs, 255, (dvis->numclusters+7)/8 );
			lastoffset = -1;
		}
		else
		{
			int thisoffset = dvis->bitofs[ cluster ][DVIS_PVS];
			if ( thisoffset != lastoffset )
			{ 
				if ( thisoffset == -1 )
				{
					Error ("visofs == -1");
				}

				DecompressVis (&dvisdata[thisoffset], pvs);
			}
			lastoffset = thisoffset;
		}
    }
	return lastoffset;
}


void BuildPatchLights( int facenum );

void DumpSamples( int ndxFace, facelight_t *pFaceLight )
{
	ThreadLock();

	dface_t *pFace = &g_pFaces[ndxFace];
	if( pFace )
	{
		for( int ndxStyle = 0; ndxStyle < 4; ndxStyle++ )
		{
			if( pFace->styles[ndxStyle] != 255 )
			{
				for( int ndxSample = 0; ndxSample < pFaceLight->numsamples; ndxSample++ )
				{
					sample_t *pSample = &pFaceLight->sample[ndxSample];
					WriteWinding( pFileSamples[ndxStyle], pSample->w, pFaceLight->light[ndxStyle][0][ndxSample] );
					if( bDumpNormals )
					{
						WriteNormal( pFileSamples[ndxStyle], pSample->pos, pSample->normal, 15.0f, pSample->normal * 255.0f );
					}
				}
			}
		}
	}

	ThreadUnlock();
}


//-----------------------------------------------------------------------------
// Allocates light sample data
//-----------------------------------------------------------------------------
static inline void AllocateLightstyleSamples( facelight_t* fl, int styleIndex, int numnormals )
{
	for (int n = 0; n < numnormals; ++n)
	{
		fl->light[styleIndex][n] = ( Vector* )calloc(fl->numsamples, sizeof(Vector));
	}
}


//-----------------------------------------------------------------------------
// Used to find an existing lightstyle on a face
//-----------------------------------------------------------------------------
static inline int FindLightstyle( dface_t* f, int lightstyle )
{
 	for (int k = 0; k < MAXLIGHTMAPS; k++)
	{
		if (f->styles[k] == lightstyle)
			return k;
	}

	return -1;
}

static int FindOrAllocateLightstyleSamples( dface_t* f, facelight_t	*fl, int lightstyle, int numnormals )
{
	// Search the lightstyles associated with the face for a match
	int k;
 	for (k = 0; k < MAXLIGHTMAPS; k++)
	{
		if (f->styles[k] == lightstyle)
			break;

		// Found an empty entry, we can use it for a new lightstyle
		if (f->styles[k] == 255)
		{
			AllocateLightstyleSamples( fl, k, numnormals );
			f->styles[k] = lightstyle;
			break;
		}
	}

	// Check for overflow
	if (k >= MAXLIGHTMAPS)
		return -1;

	return k;
}


//-----------------------------------------------------------------------------
// Compute the illumination point + normal for the sample
//-----------------------------------------------------------------------------
static void ComputeIlluminationPointAndNormals( lightinfo_t const& l, 
												Vector const& samplePosition, Vector const& sampleNormal, SampleInfo_t* pInfo )
{
	// FIXME: move sample point off the surface a bit, this is done so that
	// light sampling will not be affected by a bug	where raycasts will
	// intersect with the face being lit. We really should just have that
	// logic in GatherSampleLight
	VectorAdd( samplePosition, l.facenormal, pInfo->m_Point );

	if( pInfo->m_IsDispFace )
    {
		// FIXME: un-work around the bug fix workaround above; displacements
		// correctly deal with self-shadowing issues
		pInfo->m_Point = samplePosition;
		Assert( VectorLength( sampleNormal ) > 1.0e-20 );
		pInfo->m_PointNormal[0] = sampleNormal;

		if( pInfo->m_NormalCount > 1 )
		{
			// use facenormal along with the smooth normal to build the three bump map vectors
			GetBumpNormals( pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[0], 
					        pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
					        sampleNormal, &pInfo->m_PointNormal[1] );
		}
	}
	else if (!l.isflat)
	{
		// If the face isn't flat, use a phong-based normal instead
		Vector vecSample = samplePosition - l.modelorg;
   		GetPhongNormal( pInfo->m_FaceNum, vecSample, pInfo->m_PointNormal[0] );

		if( pInfo->m_NormalCount > 1 )
		{
			// use facenormal along with the smooth normal to build the three bump map vectors
			GetBumpNormals( pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[0], 
							pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
							pInfo->m_PointNormal[0], &pInfo->m_PointNormal[1] );
		}
    }

	// Compute the cluster, used for a fast cull for visibility of lights
	// from the sample position 
	pInfo->m_Cluster = ClusterFromPoint( samplePosition );

	Assert( VectorLength( pInfo->m_PointNormal[0]) > 1.0e-20 );
}

//-----------------------------------------------------------------------------
// Iterates over all lights and computes lighting at a sample point
//-----------------------------------------------------------------------------
static void GatherSampleLightAtPoint( SampleInfo_t& info, int sampleIdx )
{
	sampleLightOutput_t out;

	// Iterate over all direct lights and add them to the particular sample
	for (directlight_t *dl = activelights; dl != NULL; dl = dl->next)
	{
		// is this lights cluster visible?
		if ( !PVSCheck( dl->pvs, info.m_Cluster ) )
			continue;

		// NOTE: Notice here that if the light is on the back side of the face
		// (tested by checking the dot product of the face normal and the light position)
		// we don't want it to contribute to *any* of the bumped lightmaps. It glows
		// in disturbing ways if we don't do this.
		if ( !GatherSampleLight( out, dl, info.m_FaceNum, info.m_Point, info.m_PointNormal, info.m_NormalCount, info.m_iThread ) )
			continue;

		// Figure out the lightstyle for this particular sample 
		int lightStyleIndex = FindOrAllocateLightstyleSamples( info.m_pFace, info.m_pFaceLight, 
															   dl->light.style, info.m_NormalCount );
		if (lightStyleIndex < 0)
		{
			if (info.m_WarnFace != info.m_FaceNum)
			{
				Warning ("\nWARNING: Too many light styles on a face (%.0f,%.0f,%.0f)\n", info.m_Point[0], info.m_Point[1], info.m_Point[2] );
				info.m_WarnFace = info.m_FaceNum;
			}
			continue;
		}

		// pLightmaps is an array of the lightmaps for each normal direction,
		// here's where the result of the sample gathering goes
		Vector** pLightmaps = info.m_pFaceLight->light[lightStyleIndex];

		// Incremental lighting only cares about lightstyle zero
		if( g_pIncremental && (dl->light.style == 0) )
		{
			g_pIncremental->AddLightToFace( dl->m_IncrementalID, info.m_FaceNum, sampleIdx, 
											info.m_LightmapSize, out.falloff * out.dot[0], info.m_iThread );
		}

		// Compute the contributions to each of the bumped lightmaps
		// The first sample is for non-bumped lighting.
		// The other sample are for bumpmapping.
		if (! (
				(out.dot[0]>=0)
				)
			)
		{
			// do again for debug
			GatherSampleLight( out, dl, info.m_FaceNum, info.m_Point, info.m_PointNormal, info.m_NormalCount, info.m_iThread );
		}

		VectorMA( pLightmaps[0][sampleIdx], out.falloff * out.dot[0], dl->light.intensity, pLightmaps[0][sampleIdx] );
		Assert( pLightmaps[0][sampleIdx].x >= 0 && pLightmaps[0][sampleIdx].y >= 0 && pLightmaps[0][sampleIdx].z >= 0 );
		Assert( pLightmaps[0][sampleIdx].x < 1e10 && pLightmaps[0][sampleIdx].y < 1e10 && pLightmaps[0][sampleIdx].z < 1e10 );

		for( int n = 1; n < info.m_NormalCount; ++n)
		{
			if (out.dot[n] > 0)
			{
				VectorMA( pLightmaps[n][sampleIdx], out.falloff * out.dot[n], dl->light.intensity, pLightmaps[n][sampleIdx] );
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Iterates over all lights and computes lighting at a sample point
//-----------------------------------------------------------------------------
static void ResampleLightAtPoint( SampleInfo_t& info, int lightStyleIndex, int flags, Vector* pLightmap )
{
	sampleLightOutput_t out;

	// Iterate over all direct lights and add them to the particular sample
	for (directlight_t *dl = activelights; dl != NULL; dl = dl->next)
	{
		if ((flags & AMBIENT_ONLY) && (dl->light.type != emit_skyambient))
			continue;

		if ((flags & NON_AMBIENT_ONLY) && (dl->light.type == emit_skyambient))
			continue;

		// Only add contributions that match the lightstyle 
		Assert( lightStyleIndex <= MAXLIGHTMAPS );
		Assert( info.m_pFace->styles[lightStyleIndex] != 255 );
		if (dl->light.style != info.m_pFace->styles[lightStyleIndex])
			continue;

		// is this lights cluster visible?
		if ( !PVSCheck( dl->pvs, info.m_Cluster ) )
			continue;

		// NOTE: Notice here that if the light is on the back side of the face
		// (tested by checking the dot product of the face normal and the light position)
		// we don't want it to contribute to *any* of the bumped lightmaps. It glows
		// in disturbing ways if we don't do this.
		if ( !GatherSampleLight( out, dl, info.m_FaceNum, info.m_Point, info.m_PointNormal, info.m_NormalCount, info.m_iThread ) )
			continue;

		// Compute the contributions to each of the bumped lightmaps
		// The first sample is for non-bumped lighting.
		// The other sample are for bumpmapping.
		VectorMA( pLightmap[0], out.falloff * out.dot[0], dl->light.intensity, pLightmap[0] );
		for( int n = 1; n < info.m_NormalCount; ++n)
		{
			if (out.dot[n] > 0)
			{
				VectorMA( pLightmap[n], out.falloff * out.dot[n], dl->light.intensity, pLightmap[n] );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Perform supersampling at a particular point
//-----------------------------------------------------------------------------
static int SupersampleLightAtPoint( lightinfo_t& l, SampleInfo_t& info, 
									int sampleIndex, int lightStyleIndex, Vector *pDirectLight )
{
	Vector superSamplePosition;
	Vector2D sampleLightOrigin;
	Vector2D superSampleLightCoord;

	// Check out the sample we're currently dealing with...
	sample_t& sample = info.m_pFaceLight->sample[sampleIndex];

	// Get the position of the original sample in lightmapspace
	WorldToLuxelSpace( &l, sample.pos, sampleLightOrigin );
 	// Msg("coord %f %f\n", coord[0], coord[1] );

	// Some parameters related to supersampling
	int subsample = 4;	// FIXME: make a parameter
	float cscale = 1.0 / subsample;
	float csshift = -((subsample - 1) * cscale) / 2.0;

	// Clear out the direct light values
	for (int i = 0; i < info.m_NormalCount; ++i )
		pDirectLight[i].Init( 0, 0, 0 );

	int subsampleCount = 0;
	for (int s = 0; s < subsample; ++s)
	{
		for (int t = 0; t < subsample; ++t)
		{
			// make sure the coordinate is inside of the sample's winding and when normalizing
			// below use the number of samples used, not just numsamples and some of them
			// will be skipped if they are not inside of the winding
			superSampleLightCoord[0] = sampleLightOrigin[0] + s * cscale + csshift;
			superSampleLightCoord[1] = sampleLightOrigin[1] + t * cscale + csshift;
			// Msg("subsample %f %f\n", superSampleLightCoord[0], superSampleLightCoord[1] );

			// Figure out where the supersample exists in the world, and make sure
			// it lies within the sample winding
			LuxelSpaceToWorld( &l, superSampleLightCoord[0], superSampleLightCoord[1], superSamplePosition );

			// A winding should exist only if the sample wasn't a uniform luxel, or if dumppatches is true.
			if ( sample.w )
			{
				if( !PointInWinding( superSamplePosition, sample.w ) )
					continue;
			}

			// Compute the super-sample illumination point and normal
			// We're assuming the flat normal is the same for all supersamples
			ComputeIlluminationPointAndNormals( l, superSamplePosition, sample.normal, &info );

			// Resample the non-ambient light at this point...
			ResampleLightAtPoint( info, lightStyleIndex, NON_AMBIENT_ONLY, pDirectLight );

			// Got another subsample
			++subsampleCount;
		}
	}

	return subsampleCount;
}


//-----------------------------------------------------------------------------
// Compute gradients of a lightmap
//-----------------------------------------------------------------------------
static void ComputeLightmapGradients( SampleInfo_t& info, bool const* pHasProcessedSample, 
									  float* pIntensity, float* gradient )
{
	int w = info.m_LightmapWidth;
	int h = info.m_LightmapHeight;
	facelight_t* fl = info.m_pFaceLight;

	for (int i=0 ; i<fl->numsamples ; i++)
	{
		// Don't supersample the same sample twice
		if (pHasProcessedSample[i])
			continue;

		gradient[i] = 0.0f;
		sample_t& sample = fl->sample[i];

		// Choose the maximum gradient of all bumped lightmap intensities
		for ( int n = 0; n < info.m_NormalCount; ++n )
		{
			int j = n * info.m_LightmapSize + sample.s + sample.t * w;

			if (sample.t > 0)
			{
				if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1-w] ) );
				gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-w] ) );
				if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1-w] ) );
			}
			if (sample.t < h-1)
			{
				if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1+w] ) );
				gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+w] ) );
				if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1+w] ) );
			}
			if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1] ) );
			if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1] ) );
		}
	}
}



//-----------------------------------------------------------------------------
// ComputeLuxelIntensity...
//-----------------------------------------------------------------------------
static inline void ComputeLuxelIntensity( SampleInfo_t& info, int sampleIdx, 
										  Vector **ppLightSamples, float* pSampleIntensity )
{
	// Compute a separate intensity for each
	sample_t& sample = info.m_pFaceLight->sample[sampleIdx];
	int destIdx = sample.s + sample.t * info.m_LightmapWidth;
	for (int n = 0; n < info.m_NormalCount; ++n)
	{
		float intensity = ppLightSamples[n][sampleIdx][0] + ppLightSamples[n][sampleIdx][1] + ppLightSamples[n][sampleIdx][2];

		// convert to a linear perception space
		pSampleIntensity[n * info.m_LightmapSize + destIdx] = pow( intensity / 256.0, 1.0 / 2.2 );
	}
}

//-----------------------------------------------------------------------------
// Compute the maximum intensity based on all bumped lighting
//-----------------------------------------------------------------------------
static void ComputeSampleIntensities( SampleInfo_t& info, Vector **ppLightSamples, float* pSampleIntensity )
{
	for (int i=0; i<info.m_pFaceLight->numsamples; i++)
	{
		ComputeLuxelIntensity( info, i, ppLightSamples, pSampleIntensity );
	}
}


//-----------------------------------------------------------------------------
// Perform supersampling on a particular lightstyle
//-----------------------------------------------------------------------------
static void BuildSupersampleFaceLights( lightinfo_t& l, SampleInfo_t& info, int lightstyleIndex )
{
	Vector pAmbientLight[NUM_BUMP_VECTS+1];
	Vector pDirectLight[NUM_BUMP_VECTS+1];

	// This is used to make sure we don't supersample a light sample more than once
	int processedSampleSize = info.m_LightmapSize * sizeof(bool);
	bool* pHasProcessedSample = (bool*)stackalloc( processedSampleSize );
	memset( pHasProcessedSample, 0, processedSampleSize );

	// This is used to compute a simple gradient computation of the light samples
	// We're going to store the maximum intensity of all bumped samples at each sample location
	float* pGradient = (float*)stackalloc( info.m_pFaceLight->numsamples * sizeof(float) );
	float* pSampleIntensity = (float*)stackalloc( info.m_NormalCount * info.m_LightmapSize * sizeof(float) );

	// Compute the maximum intensity of all lighting associated with this lightstyle
	// for all bumped lighting
	Vector **ppLightSamples = info.m_pFaceLight->light[lightstyleIndex];
	ComputeSampleIntensities( info, ppLightSamples, pSampleIntensity );

	Vector *pVisualizePass = NULL;
	if (debug_extra)
	{
		int visualizationSize = info.m_pFaceLight->numsamples * sizeof(Vector);
		pVisualizePass = (Vector*)stackalloc( visualizationSize );
		memset( pVisualizePass, 0, visualizationSize ); 
	}

	// What's going on here is that we're looking for large lighting discontinuities
	// (large light intensity gradients) as a clue that we should probably be supersampling
	// in that area. Because the supersampling operation will cause lighting changes,
	// we've found that it's good to re-check the gradients again and see if any other
	// areas should be supersampled as a result of the previous pass. Keep going
	// until all the gradients are reasonable or until we hit a max number of passes
	bool do_anotherpass = true;
	int pass = 1;
	while (do_anotherpass && pass <= extrapasses)
	{
		// Look for lighting discontinuities to see what we should be supersampling
		ComputeLightmapGradients( info, pHasProcessedSample, pSampleIntensity, pGradient );

		do_anotherpass = false;

		// Now check all of the samples and supersample those which we have
		// marked as having high gradients
		for (int i=0 ; i<info.m_pFaceLight->numsamples; ++i)
		{
			// Don't supersample the same sample twice
			if (pHasProcessedSample[i])
				continue;

			// Don't supersample if the lighting is pretty uniform near the sample
			if (pGradient[i] < 0.0625)
				continue;

			// Joy! We're supersampling now, and we therefore must do another pass
			// Also, we need never bother with this sample again
			pHasProcessedSample[i] = true;
			do_anotherpass = true;

			if (debug_extra)
			{
				// Mark the little visualization bitmap with a color indicating
				// which pass it was updated on.
				pVisualizePass[i][0] = (pass & 1) * 255;
				pVisualizePass[i][1] = (pass & 2) * 128;
				pVisualizePass[i][2] = (pass & 4) * 64;
			}

			// Figure out the position + normal direction of the sample under consideration
			sample_t& sample = info.m_pFaceLight->sample[i];
			ComputeIlluminationPointAndNormals( l, sample.pos, sample.normal, &info );

			// Compute the ambient light for each bump direction vector
			for (int j = 0; j < info.m_NormalCount; ++j )
				pAmbientLight[j].Init( 0, 0, 0 );
			ResampleLightAtPoint( info, lightstyleIndex, AMBIENT_ONLY, pAmbientLight );

			// Supersample the non-ambient light for each bump direction vector
			int supersampleCount = SupersampleLightAtPoint( l, info, i, lightstyleIndex, pDirectLight );

			// Because of sampling problems, small area triangles may have no samples.
			// In this case, just use what we already have
			if (supersampleCount > 0)
			{
				// Add the ambient + directional terms togather, stick it back into the lightmap
				for (int n = 0; n < info.m_NormalCount; ++n)
				{
					if( supersampleCount > 0 )
					{
						VectorDivide( pDirectLight[n], supersampleCount, ppLightSamples[n][i] );
						ppLightSamples[n][i] += pAmbientLight[n];
					}
				}

				// Recompute the luxel intensity based on the supersampling
				ComputeLuxelIntensity( info, i, ppLightSamples, pSampleIntensity );
			}

		}

		// We've finished another pass
		pass++;
	}

	if (debug_extra)
	{
		// Copy colors representing which supersample pass the sample was messed with
		// into the actual lighting values so we can visualize it
		for (int i=0 ; i<info.m_pFaceLight->numsamples ; ++i)
		{
			for (int j = 0; j <info.m_NormalCount; ++j)
			{
				VectorCopy( pVisualizePass[i], ppLightSamples[j][i] ); 
			}
		}
	}
}


void InitLightinfo( lightinfo_t *pl, int facenum )
{
	dface_t		*f;

	f = &g_pFaces[facenum];

	memset (pl, 0, sizeof(*pl));
	pl->facenum = facenum;

    pl->face = f;

    //
    // rotate plane 
    //
	VectorCopy (dplanes[f->planenum].normal, pl->facenormal);
	pl->facedist = dplanes[f->planenum].dist;

	// get the origin offset for rotating bmodels
	VectorCopy (face_offset[facenum], pl->modelorg);

	CalcFaceVectors( pl );

	// figure out if the surface is flat
	pl->isflat = true;
	if (smoothing_threshold != 1)
	{
		faceneighbor_t *fn = &faceneighbor[facenum];

		for (int j=0 ; j<f->numedges ; j++)
		{
			float dot = DotProduct( pl->facenormal, fn->normal[j] );
			if (dot < 1.0 - EQUAL_EPSILON)
			{
				pl->isflat = false;
				break;
			}
		}
	}
}

static void InitSampleInfo( lightinfo_t const& l, int iThread, SampleInfo_t& info )
{
	info.m_LightmapWidth  = l.face->m_LightmapTextureSizeInLuxels[0]+1;
	info.m_LightmapHeight = l.face->m_LightmapTextureSizeInLuxels[1]+1;
	info.m_LightmapSize = info.m_LightmapWidth * info.m_LightmapHeight;

	// How many lightmaps are we going to need?
	info.m_pTexInfo = &texinfo[l.face->texinfo];
	info.m_NormalCount = (info.m_pTexInfo->flags & SURF_BUMPLIGHT) ? NUM_BUMP_VECTS + 1 : 1;
	info.m_FaceNum = l.facenum;
	info.m_pFace = l.face;
	info.m_pFaceLight = &facelight[info.m_FaceNum];
	info.m_IsDispFace = ValidDispFace( info.m_pFace );
	info.m_iThread = iThread;
	info.m_WarnFace = -1;

	// initialize normals if the surface is flat
	if (l.isflat)
	{
		VectorCopy( l.facenormal, info.m_PointNormal[0] );

		// use facenormal along with the smooth normal to build the three bump map vectors
		if( info.m_NormalCount > 1 )
		{
			GetBumpNormals( info.m_pTexInfo->textureVecsTexelsPerWorldUnits[0], 
							info.m_pTexInfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
							l.facenormal, &info.m_PointNormal[1] );
		}
	}
}


void BuildFacelightsOld(int facenum, int iThread);

/*
  =============
  BuildFacelights
  =============
*/
void BuildFacelights (int iThread, int facenum)
{
	int	i, j;

#ifdef TEST_AGAINST_OLD_LIGHTING
	BuildFacelightsOld( facenum, iThread );

	// Store off the old lightmaps....
	Vector		*pOldLight[MAXLIGHTMAPS][NUM_BUMP_VECTS+1];
	for ( i = 0; i < MAXLIGHTMAPS; ++i)
	{
		for ( j = 0; j < NUM_BUMP_VECTS+1; ++j)
		{
			pOldLight[i][j] = facelight[facenum].light[i][j];
		}
	}
#endif

	lightinfo_t	l;
	dface_t *f;
	facelight_t	*fl;
	SampleInfo_t sampleInfo;
	directlight_t *dl;
	Vector spot;

	if( g_bInterrupt )
		return;

	// FIXME: Is there a better way to do this? Like, in RunThreadsOn, for instance?
	// Don't pay this cost unless we have to; this is super perf-critical code.
	if (g_pIncremental)
	{
		// Both threads will be accessing this so it needs to be protected or else thread A
		// will load it in and thread B will increment it but its increment will be
		// overwritten by thread A when thread A writes it back.
		ThreadLock();
		++g_iCurFace;
		ThreadUnlock();
	}

    // some surfaces don't need lightmaps
	f = &g_pFaces[facenum];
	f->lightofs = -1;
	for (j=0 ; j<MAXLIGHTMAPS ; j++)
		f->styles[j] = 255;

	// Trivial-reject the whole face?	
	if( !( g_FacesVisibleToLights[facenum>>3] & (1 << (facenum & 7)) ) )
		return;

	// check for patches for this face.  If none it must be degenerate.  Ignore.
	if( facePatches.Element( facenum ) == facePatches.InvalidIndex() )
		return;

	fl = &facelight[facenum];

	if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
		return;		// non-lit texture

	InitLightinfo( &l, facenum );
	CalcPoints( &l, fl, facenum );
	InitSampleInfo( l, iThread, sampleInfo );

	// always allocate style 0 lightmap
	f->styles[0] = 0;
	AllocateLightstyleSamples( fl, 0, sampleInfo.m_NormalCount );

	// sample the lights at each sample location
	for (i=0 ; i<fl->numsamples ; i++)
	{
		// Figure out the position + normal direction of the sample under consideration
		sample_t& sample = fl->sample[i];
		ComputeIlluminationPointAndNormals( l, sample.pos, sample.normal, &sampleInfo );

		// Fix up the sample normal in the case of smooth faces...
		if (!l.isflat)
		{
			// The sample normal is now the phong normal
			sample.normal = sampleInfo.m_PointNormal[0];
		}

		// Iterate over all the lights and add their contribution to this spot
		GatherSampleLightAtPoint( sampleInfo, i );
	}

	// Tell the incremental light manager that we're done with this face.
	if( g_pIncremental )
	{
		for (dl = activelights; dl != NULL; dl = dl->next)
		{
			// Only deal with lightstyle 0 for incremental lighting
			if (dl->light.style == 0)
				g_pIncremental->FinishFace( dl->m_IncrementalID, facenum, iThread );
		}
	
		// Don't have to deal with patch lights (only direct lighting is used)
		// or supersampling
		return;
	}

	// get rid of the -extra functionality on displacement surfaces
	if (do_extra && !sampleInfo.m_IsDispFace)
	{
		// For each lightstyle, perform a supersampling pass
		for ( i = 0; i < MAXLIGHTMAPS; ++i )
		{
			// Stop when we run out of lightstyles
			if (f->styles[i] == 255)
				break;

			BuildSupersampleFaceLights( l, sampleInfo, i );
		}
	}

#ifdef TEST_AGAINST_OLD_LIGHTING
	for ( i = 1; i < MAXLIGHTMAPS; ++i)
	{
		for ( j = 0; j < NUM_BUMP_VECTS+1; ++j)
		{
			if (!pOldLight[i][j])
			{
				Assert( !fl->light[i][j] );
			}
			else
			{
				for (int s = 0; s < fl->numsamples; ++s)
				{
					Assert( VectorsAreEqual( pOldLight[i][j][s], fl->light[i][j][s], 1e-8 ) );
				}
			}
		}
	}
#endif

	if (!g_bUseMPI) 
	{
		//
		// This is done on the master node when MPI is used
		//
		BuildPatchLights( facenum );
	}

	if( dumppatches )
	{
		DumpSamples( facenum, fl );
	}

	FreeSampleWindings( fl );
}	

/*
  =============
  BuildFacelights
  =============
*/
void BuildFacelightsOld(int facenum, int iThread)
{
	int	i, j, k;
	int size;
	lightinfo_t	l;
	dface_t *f;
	facelight_t	*fl;
	directlight_t *dl;
	Vector spot;

	
	if( g_bInterrupt )
		return;

	// Both threads will be accessing this so it needs to be protected or else thread A
	// will load it in and thread B will increment it but its increment will be
	// overwritten by thread A when thread A writes it back.
	ThreadLock();
	++g_iCurFace;
	ThreadUnlock();


	// Trivial-reject the whole face?	
	if( !( g_FacesVisibleToLights[facenum>>3] & (1 << (facenum & 7)) ) )
		return;


	// check for patches for this face.  If none it must be degenerate.  Ignore.
	if( facePatches.Element( facenum ) == facePatches.InvalidIndex() )
		return;

	f = &g_pFaces[facenum];
	fl = &facelight[facenum];

    //
    // some surfaces don't need lightmaps
    //
	f->lightofs = -1;
	for (j=0 ; j<MAXLIGHTMAPS ; j++)
		f->styles[j] = 255;

	if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
		return;		// non-lit texture

	InitLightinfo( &l, facenum );

	CalcPoints( &l, fl, facenum );

	int lightmapwidth  = l.face->m_LightmapTextureSizeInLuxels[0]+1;
	int lightmapheight = l.face->m_LightmapTextureSizeInLuxels[1]+1;

	size = lightmapwidth*lightmapheight;
	if (size > SINGLEMAP)
		Error ("Bad lightmap size");


	// set up suface normal list
	texinfo_t *pTexinfo = &texinfo[f->texinfo];
	int needsBumpmap = pTexinfo->flags & SURF_BUMPLIGHT ? true : false;
	int numnormals = needsBumpmap ? NUM_BUMP_VECTS+1 : 1;
	Vector	pointnormal[ NUM_BUMP_VECTS + 1 ];

	// initialize normals if the surface is flat
	if (l.isflat)
	{
		VectorCopy( l.facenormal, pointnormal[0] );

		if( needsBumpmap )
		{
			// use facenormal along with the smooth normal to build the three bump map vectors
			GetBumpNormals( pTexinfo->textureVecsTexelsPerWorldUnits[0], 
							pTexinfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
							pointnormal[0], &pointnormal[1] );
		}
	}

	// always allocate style 0 bumpmap
	int n;
	f->styles[0] = 0; // Everyone gets the style zero map.
	for (n = 0; n < numnormals; n++)
	{
		fl->light[0][n] = ( Vector* )calloc(fl->numsamples, sizeof(Vector));
	}

	// sample the lights
	for (i=0 ; i<fl->numsamples ; i++)
	{
		// move sample point off the surface a bit
		VectorAdd( fl->sample[i].pos, l.facenormal, spot );

		// FIXME: only do this for faces that are parts of entities
        int cluster = ClusterFromPoint( spot );

	    if( ValidDispFace( f ) )
        {
//			spot = fl->sample[i].pos;
//			StaticDispMgr()->GetDispSurfNormal( facenum, spot, pointnormal[0], false );
//			fl->sample[i].pos = spot;
//			fl->sample[i].normal = pointnormal[0];

			// copy the world/lightmap space transform data
//			StaticDispMgr()->SetLightmapWorldSpaceTransformData( facenum, l );

			spot = fl->sample[i].pos;
			pointnormal[0] = fl->sample[i].normal;

			if( needsBumpmap )
			{
				// use facenormal along with the smooth normal to build the three bump map vectors
				GetBumpNormals( pTexinfo->textureVecsTexelsPerWorldUnits[0], 
					            pTexinfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
					            pointnormal[0], &pointnormal[1] );
			}
		}
		else if (!l.isflat)
		{
   			GetPhongNormal( facenum, fl->sample[i].pos, pointnormal[0] );
			fl->sample[i].normal = pointnormal[0];

			if( needsBumpmap )
			{
				// use facenormal along with the smooth normal to build the three bump map vectors
				GetBumpNormals( pTexinfo->textureVecsTexelsPerWorldUnits[0], 
								pTexinfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
								pointnormal[0], &pointnormal[1] );
			}
        }

		sampleLightOutput_t out;
		for (dl = activelights; dl != NULL; dl = dl->next)
		{
			// is this lights cluster visible?
			if ( !PVSCheck( dl->pvs, cluster ) )
				continue;

			if ( GatherSampleLight( out, dl, facenum, spot, pointnormal, numnormals, iThread ) )
			{
				for (k = 0; k < MAXLIGHTMAPS; k++)
				{
					if (f->styles[k] == dl->light.style)
						break;
					else if (f->styles[k] == 255)
					{
						for (n = 0; n < numnormals; n++)
						{
							fl->light[k][n] = ( Vector* )calloc(fl->numsamples, sizeof(Vector));
						}
						f->styles[k] = dl->light.style;
						break;
					}
				}
				if (k >= MAXLIGHTMAPS)
				{
					/*
					  Msg ("WARNING: Too many direct light styles on a face(%f,%f,%f)\n", 
					  fl->sample[i].pos[0], fl->sample[i].pos[1], fl->sample[i].pos[2] );
					*/
					continue;
				}

				// The first sample is for non-bumped lighting.
				// The other sample are for bumpmapping.
				float flContribution = out.falloff * out.dot[0];

				VectorMA( fl->light[k][0][i], flContribution, dl->light.intensity, fl->light[k][0][i] );
				
				if( g_pIncremental )
				{
					g_pIncremental->AddLightToFace(
						dl->m_IncrementalID,
						facenum,
						i,
						size,
						flContribution,
						iThread );
				}


				for( n = 1; n < numnormals; n++)
				{
					if (out.dot[n] > 0)
					{
						VectorMA( fl->light[k][n][i], out.falloff * out.dot[n], dl->light.intensity, fl->light[k][n][i] );
					}
				}
			}
		}
	}

	// Tell the incremental light manager that we're done with this face.
	if( g_pIncremental )
	{
		for (dl = activelights; dl != NULL; dl = dl->next)
			g_pIncremental->FinishFace( dl->m_IncrementalID, facenum, iThread );
	
		return;
	}

	bool bIsDisp = ValidDispFace( f );

	int h = l.face->m_LightmapTextureSizeInLuxels[1]+1;
	int w = l.face->m_LightmapTextureSizeInLuxels[0]+1;
	int do_anotherpass = do_extra;
	int	consider[SINGLEMAP];
	float sampled[SINGLEMAP];

	// get rid of the -extra functionality on displacement surfaces
	if( bIsDisp )
	{
		do_anotherpass = 0;
	}

	if (do_extra && !bIsDisp)
	{
		for (i=0 ; i<fl->numsamples ; i++)
		{
			consider[i] = true;
		}

		for (i=0 ; i<fl->numsamples ; i++)
		{
			// convert to a linear perception space
			sampled[fl->sample[i].s + fl->sample[i].t * w] = pow( (fl->light[0][0][i][0] + fl->light[0][0][i][1] + fl->light[0][0][i][2]) / 256.0, 1.0 / 2.2 );
		}
	}

	int pass = 1;
	Vector passcycle[SINGLEMAP*2];

	while (do_anotherpass && pass <= extrapasses)
	{
		if( pass == 1 )
		{
			for (i=0 ; i<fl->numsamples ; i++)
			{
				VectorFill( passcycle[i], 0 );
			}
		}		

		// really bad supersampling
		float	gradient[SINGLEMAP];
		int s, t;

		for (i=0 ; i<fl->numsamples ; i++)
		{
			if (!consider[i])
				continue;

			j = fl->sample[i].s + fl->sample[i].t * w;

			gradient[i] = 0.0;

			if (fl->sample[i].t > 0)
			{
				if (fl->sample[i].s > 0)   gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j-1-w] ) );
				gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j-w] ) );
				if (fl->sample[i].s < w-1) gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j+1-w] ) );
			}
			if (fl->sample[i].t < h-1)
			{
				if (fl->sample[i].s > 0)   gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j-1+w] ) );
				gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j+w] ) );
				if (fl->sample[i].s < w-1) gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j+1+w] ) );
			}
			if (fl->sample[i].s > 0)   gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j-1] ) );
			if (fl->sample[i].s < w-1) gradient[i] = max( gradient[i], fabs( sampled[j] - sampled[j+1] ) );
		}

		do_anotherpass = false;

		for (i=0 ; i<fl->numsamples ; i++)
		{
			if (!consider[i])
				continue;

			// FIXME: only do this for faces that are parts of entities
			int cluster = ClusterFromPoint( spot );

			if (gradient[i] > 0.0625)
			{
				consider[i] = false;
				do_anotherpass = true;
				
				// Vector pos, normal;
				Vector2D coord, coord2;

				// SamplePos( fl, i, pos );
				// SampleNormal( fl, i, normal );
				// WorldToCoord( &l, pos, coord );

				WorldToLuxelSpace( &l, fl->sample[i].pos, coord );

				// Msg("coord %f %f\n", coord[0], coord[1] );

				// clear previous light values
				VectorFill( fl->light[0][0][i], 0 );

				Vector spot2;
				int subsample = 4;	// FIXME: make a parameter

				float cscale = 1.0 / subsample;
				float csshift = -((subsample - 1) * cscale) / 2.0;

				VectorAdd( fl->sample[i].pos, l.facenormal, spot2 );

				Vector ambientLight( 0.0f, 0.0f, 0.0f );
				for (dl = activelights; dl != NULL; dl = dl->next)
				{
					if (dl->light.type != emit_skyambient)
						continue;

					// is this lights cluster visible?
					if ( !PVSCheck( dl->pvs, cluster ) )
						continue;

					sampleLightOutput_t out;
					if ( GatherSampleLight( out, dl, facenum, spot2, &l.facenormal, 1, iThread ) )
					{
						// accumulate the ambient light here and multiply by the number of "active" subsamples below!!
						VectorMA( ambientLight, ( out.falloff * out.dot[0] ), dl->light.intensity, ambientLight );
//							VectorMA( fl->light[0][0][i], falloff * dot * subsample * subsample, dl->light.intensity, fl->light[0][0][i] );
					}
				}

				unsigned int subsampleCount = 0;
				for (s = 0; s < subsample; s++)
				{
					for (t = 0; t < subsample; t++)
					{
						// coord2[0] = coord[0] + (rand() % 16) - 7.5;
						// coord2[1] = coord[1] + (rand() % 16) - 7.5;
						// FIXME: should be limited to sample area!!

						// make sure the coordinate is inside of the sample's winding and when normalizing
						// below use the number of samples used, not just numsamples and some of them
						// will be skipped if they are not inside of the winding

						coord2[0] = coord[0] + s * cscale + csshift;
						coord2[1] = coord[1] + t * cscale + csshift;

						// Msg("subsample %f %f\n", coord2[0], coord2[1] );

						LuxelSpaceToWorld( &l, coord2[0], coord2[1], spot2 );

						if( PointInWinding( spot2, fl->sample[i].w ) )
						{
							VectorAdd( spot2, l.facenormal, spot2 );
							
							for (dl = activelights; dl != NULL; dl = dl->next)
							{
								if (dl->light.type == emit_skyambient)
									continue;
								
								// is this lights cluster visible?
								if ( !PVSCheck( dl->pvs, cluster ) )
									continue;

								sampleLightOutput_t out;

								if ( GatherSampleLight( out, dl, facenum, spot2, &l.facenormal, 1, iThread ) )
								{
									VectorMA( fl->light[0][0][i], out.falloff * out.dot[0], dl->light.intensity, fl->light[0][0][i] );
								}
							}

							subsampleCount++;
						}
					}
				}

				if( subsampleCount > 0 )
				{
					VectorMA( fl->light[0][0][i], ( float )subsampleCount, ambientLight, fl->light[0][0][i] );
					VectorScale( fl->light[0][0][i], 1.0 / ( float )(subsampleCount), fl->light[0][0][i] );
//					VectorScale( fl->light[0][0][i], 1.0 / (subsample * subsample), fl->light[0][0][i] );
					sampled[fl->sample[i].s + fl->sample[i].t * w] = pow( (fl->light[0][0][i][0] + fl->light[0][0][i][1] + fl->light[0][0][i][2]) / 256.0, 1.0 / 2.2 );
				}

				passcycle[i][0] = (pass & 1) * 255;
				passcycle[i][1] = (pass & 2) * 128;
				passcycle[i][2] = (pass & 4) * 64;
			}
		}
		pass++;
	}

	for (i=0 ; i<fl->numsamples ; i++)
	{
		// VectorCopy( passcycle[i], fl->light[0][0][i] );
	}

//	BuildPatchLights( facenum );

	if( dumppatches )
	{
		DumpSamples( facenum, fl );
	}
}	



void BuildPatchLights( int facenum )
{
	int i, k;

	patch_t		*patch;

	dface_t	*f = &g_pFaces[facenum];
	facelight_t	*fl = &facelight[facenum];

	for( k = 0; k < MAXLIGHTMAPS; k++ )
	{
		if (f->styles[k] == 0)
			break;
	}

	if (k >= MAXLIGHTMAPS)
		return;

	for (i = 0; i < fl->numsamples; i++)
	{
		AddSampleToPatch( &fl->sample[i], fl->light[k][0][i], facenum);
	}

	// check for a valid face
	if( facePatches.Element( facenum ) == facePatches.InvalidIndex() )
		return;

	// push up sampled light to parents (children always exist first in the list)
	patch_t *pNextPatch;
	for( patch = &patches.Element( facePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != patches.InvalidIndex() )
		{
			pNextPatch = &patches.Element( patch->ndxNext );
		}

		// skip patches without parents
		if( patch->parent == patches.InvalidIndex() )
//		if (patch->parent == -1)
			continue;

		patch_t *parent = &patches.Element( patch->parent );

		parent->samplearea += patch->samplearea;
		VectorAdd( parent->samplelight, patch->samplelight, parent->samplelight );
	}

	// average up the direct light on each patch for radiosity
	if (numbounce > 0)
	{
		for( patch = &patches.Element( facePatches.Element( facenum ) ); patch; patch = pNextPatch )
		{
			// next patch
			pNextPatch = NULL;
			if( patch->ndxNext != patches.InvalidIndex() )
			{
				pNextPatch = &patches.Element( patch->ndxNext );
			}

			if (patch->samplearea)
			{ 
				float scale;
				Vector v;
				scale = 1.0 / patch->samplearea;

				VectorScale( patch->samplelight, scale, v );
				VectorAdd( patch->totallight.light[0], v, patch->totallight.light[0] );
				VectorAdd( patch->directlight, v, patch->directlight );
			}
		}
	}

	// pull totallight from children (children always exist first in the list)
	for( patch = &patches.Element( facePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != patches.InvalidIndex() )
		{
			pNextPatch = &patches.Element( patch->ndxNext );
		}

		if ( patch->child1 != patches.InvalidIndex() )
		{
			float s1, s2;
			patch_t *child1;
			patch_t *child2;

			child1 = &patches.Element( patch->child1 );
			child2 = &patches.Element( patch->child2 );

			s1 = child1->area / (child1->area + child2->area);
			s2 = child2->area / (child1->area + child2->area);

			VectorScale( child1->totallight.light[0], s1, patch->totallight.light[0] );
			VectorMA( patch->totallight.light[0], s2, child2->totallight.light[0], patch->totallight.light[0] );

			VectorCopy( patch->totallight.light[0], patch->directlight );
		}
	}

	bool needsBumpmap = false;
	if( texinfo[f->texinfo].flags & SURF_BUMPLIGHT )
	{
		needsBumpmap = true;
	}

	// add an ambient term if desired
	if (ambient[0] || ambient[1] || ambient[2])
	{
		for( int j=0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++ )
		{
			if ( f->styles[j] == 0 )
			{
				for (i = 0; i < fl->numsamples; i++)
				{
					VectorAdd( fl->light[j][0][i], ambient, fl->light[j][0][i] ); 
					if( needsBumpmap )
					{
						VectorAdd( fl->light[j][1][i], ambient, fl->light[j][1][i] ); 
						VectorAdd( fl->light[j][2][i], ambient, fl->light[j][2][i] ); 
					}
				}
				break;
			}
		}
	}

	// light from dlight_threshold and above is sent out, but the
	// texture itself should still be full bright

#if 0
	// if( VectorAvg( face_patches[facenum]->baselight ) >= dlight_threshold)	// Now all lighted surfaces glow
 {
	 for( j=0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++ )
	 {
		 if ( f->styles[j] == 0 )
		 {
			 // BUG: shouldn't this be done for all patches on the face?
			 for (i=0 ; i<fl->numsamples ; i++)
			 {
				 // garymctchange
				 VectorAdd( fl->light[j][0][i], face_patches[facenum]->baselight, fl->light[j][0][i] ); 
				 if( needsBumpmap )
				 {
					 for( bumpSample = 1; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
					 {
						 VectorAdd( fl->light[j][bumpSample][i], face_patches[facenum]->baselight, fl->light[j][bumpSample][i] ); 
					 }
				 }
			 }
			 break;
		 }
	 }
 }
#endif
}


/*
  =============
  PrecompLightmapOffsets
  =============
*/

void PrecompLightmapOffsets()
{
    int facenum;
    dface_t *f;
    int lightstyles;
    int lightdatasize = 0;
    
    // NOTE: We store avg face light data in this lump *before* the lightmap data itself
	// in *reverse order* of the way the lightstyles appear in the styles array.
    for( facenum = 0; facenum < numfaces; facenum++ )
    {
        f = &g_pFaces[facenum];

        if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
            continue;		// non-lit texture
        
        if ( dlight_map != 0 )
            f->styles[1] = 0;
        
        for (lightstyles=0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
        {
            if ( f->styles[lightstyles] == 255 )
                break;
        }
        
        if ( !lightstyles )
            continue;

        // Reserve room for the avg light color data
		lightdatasize += lightstyles * 4;

        f->lightofs = lightdatasize;

		bool needsBumpmap = false;
		if( texinfo[f->texinfo].flags & SURF_BUMPLIGHT )
		{
			needsBumpmap = true;
		}

		int nLuxels = (f->m_LightmapTextureSizeInLuxels[0]+1) * (f->m_LightmapTextureSizeInLuxels[1]+1);
		if( needsBumpmap )
		{
			lightdatasize += nLuxels * 4 * lightstyles * ( NUM_BUMP_VECTS + 1 );
		}
		else
		{
	        lightdatasize += nLuxels * 4 * lightstyles;
		}
    }

	// The incremental lighting code needs us to preserve the contents of dlightdata
	// since it only recomposites lighting for faces that have lights that touch them.
	if( g_pIncremental && pdlightdata->Count() )
		return;

	pdlightdata->SetSize( lightdatasize );
}
CUtlVector<CImagePacker>	g_ImagePackers;
CUtlVector<byte *>			g_rawLightmapPages;
char						g_currentMaterialName[256];


// Clamp the three values for bumped lighting such that we trade off directionality for brightness.
static void ColorClampBumped( Vector& color1, Vector& color2, Vector& color3 )
{
	Vector maxs;
	Vector *colors[3] = { &color1, &color2, &color3 };
	maxs[0] = VectorMaximum( color1 );
	maxs[1] = VectorMaximum( color2 );
	maxs[2] = VectorMaximum( color3 );

	// HACK!  Clean this up, and add some else statements
#define CONDITION(a,b,c) do { if( maxs[a] >= maxs[b] && maxs[b] >= maxs[c] ) { order[0] = a; order[1] = b; order[2] = c; } } while( 0 )
	
	int order[3];
	CONDITION(0,1,2);
	CONDITION(0,2,1);
	CONDITION(1,0,2);
	CONDITION(1,2,0);
	CONDITION(2,0,1);
	CONDITION(2,1,0);

	int i;
	for( i = 0; i < 3; i++ )
	{
		float max = VectorMaximum( *colors[order[i]] );
		if( max <= 1.0f )
		{
			continue;
		}
		// This channel is too bright. . take half of the amount that we are over and 
		// add it to the other two channel.
		float factorToRedist = ( max - 1.0f ) / max;
		Vector colorToRedist = factorToRedist * *colors[order[i]];
		*colors[order[i]] -= colorToRedist;
		colorToRedist *= 0.5f;
		*colors[order[(i+1)%3]] += colorToRedist;
		*colors[order[(i+2)%3]] += colorToRedist;
	}

	ColorClamp( color1 );
	ColorClamp( color2 );
	ColorClamp( color3 );
	
	if( color1[0] < 0.f ) color1[0] = 0.f;
	if( color1[1] < 0.f ) color1[1] = 0.f;
	if( color1[2] < 0.f ) color1[2] = 0.f;
	if( color2[0] < 0.f ) color2[0] = 0.f;
	if( color2[1] < 0.f ) color2[1] = 0.f;
	if( color2[2] < 0.f ) color2[2] = 0.f;
	if( color3[0] < 0.f ) color3[0] = 0.f;
	if( color3[1] < 0.f ) color3[1] = 0.f;
	if( color3[2] < 0.f ) color3[2] = 0.f;
}

static void LinearToBumpedLightmap(
	const float		*linearColor, 
	const float		*linearBumpColor1,
	const float		*linearBumpColor2, 
	const float		*linearBumpColor3,
	unsigned char	*ret, 
	unsigned char	*retBump1,
	unsigned char	*retBump2, 
	unsigned char	*retBump3 )
{
	const Vector &linearBump1 = *( ( const Vector * )linearBumpColor1 );
	const Vector &linearBump2 = *( ( const Vector * )linearBumpColor2 );
	const Vector &linearBump3 = *( ( const Vector * )linearBumpColor3 );

	Vector gammaGoal;
	// gammaGoal is premultiplied by 1/overbright, which we want
	gammaGoal[0] = LinearToVertexLight( linearColor[0] );
	gammaGoal[1] = LinearToVertexLight( linearColor[1] );
	gammaGoal[2] = LinearToVertexLight( linearColor[2] );
	Vector bumpAverage = linearBump1;
	bumpAverage += linearBump2;
	bumpAverage += linearBump3;
	bumpAverage *= ( 1.0f / 3.0f );
	
	Vector correctionScale;
	if( *( int * )&bumpAverage[0] != 0 && *( int * )&bumpAverage[1] != 0 && *( int * )&bumpAverage[2] != 0 )
	{
		// fast path when we know that we don't have to worry about divide by zero.
		VectorDivide( gammaGoal, bumpAverage, correctionScale );
//			correctionScale = gammaGoal / bumpSum;
	}
	else
	{
		correctionScale.Init( 0.0f, 0.0f, 0.0f );
		if( bumpAverage[0] != 0.0f )
		{
			correctionScale[0] = gammaGoal[0] / bumpAverage[0];
		}
		if( bumpAverage[1] != 0.0f )
		{
			correctionScale[1] = gammaGoal[1] / bumpAverage[1];
		}
		if( bumpAverage[2] != 0.0f )
		{
			correctionScale[2] = gammaGoal[2] / bumpAverage[2];
		}
	}
	Vector correctedBumpColor1;
	Vector correctedBumpColor2;
	Vector correctedBumpColor3;
	VectorMultiply( linearBump1, correctionScale, correctedBumpColor1 );
	VectorMultiply( linearBump2, correctionScale, correctedBumpColor2 );
	VectorMultiply( linearBump3, correctionScale, correctedBumpColor3 );

	Vector check = ( correctedBumpColor1 + correctedBumpColor2 + correctedBumpColor3 ) / 3.0f;

	ColorClampBumped( correctedBumpColor1, correctedBumpColor2, correctedBumpColor3 );

	ret[0] = RoundFloatToByte( gammaGoal[0] * 255.0f );
	ret[1] = RoundFloatToByte( gammaGoal[1] * 255.0f );
	ret[2] = RoundFloatToByte( gammaGoal[2] * 255.0f );
	retBump1[0] = RoundFloatToByte( correctedBumpColor1[0] * 255.0f );
	retBump1[1] = RoundFloatToByte( correctedBumpColor1[1] * 255.0f );
	retBump1[2] = RoundFloatToByte( correctedBumpColor1[2] * 255.0f );
	retBump2[0] = RoundFloatToByte( correctedBumpColor2[0] * 255.0f );
	retBump2[1] = RoundFloatToByte( correctedBumpColor2[1] * 255.0f );
	retBump2[2] = RoundFloatToByte( correctedBumpColor2[2] * 255.0f );
	retBump3[0] = RoundFloatToByte( correctedBumpColor3[0] * 255.0f );
	retBump3[1] = RoundFloatToByte( correctedBumpColor3[1] * 255.0f );
	retBump3[2] = RoundFloatToByte( correctedBumpColor3[2] * 255.0f );
}

//-----------------------------------------------------------------------------
// Convert a RGBExp32 to a RGBA8888
// This matches the engine's conversion, so the lighting result is consistent.
//-----------------------------------------------------------------------------
void ConvertRGBExp32ToRGBA8888( const ColorRGBExp32 *pSrc, unsigned char *pDst )
{
	Vector		linearColor;
	Vector		vertexColor;

	// convert from ColorRGBExp32 to linear space
	linearColor[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->r, ((ColorRGBExp32 *)pSrc)->exponent );
	linearColor[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->g, ((ColorRGBExp32 *)pSrc)->exponent );
	linearColor[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->b, ((ColorRGBExp32 *)pSrc)->exponent );

	// convert from linear space to lightmap space
	// cannot use mathlib routine directly because it doesn't match
	// the colorspace version found in the engine, which *is* the same sequence here
	vertexColor[0] = LinearToVertexLight( linearColor[0] );
	vertexColor[1] = LinearToVertexLight( linearColor[1] );
	vertexColor[2] = LinearToVertexLight( linearColor[2] );

	// this is really a color normalization with a floor
	ColorClamp( vertexColor );

	// final [0..255] scale
	pDst[0] = RoundFloatToByte( vertexColor[0] * 255.0f );
	pDst[1] = RoundFloatToByte( vertexColor[1] * 255.0f );
	pDst[2] = RoundFloatToByte( vertexColor[2] * 255.0f );
	pDst[3] = 255;
}

void SwizzleRect( const byte *pSource, byte *pTarget, int texWidth, int texHeight, int bytesPerPixel )
{
	int			x;
	int			y;
	int			n;
	byte		*pDst;
	const byte	*pSrc;

    // Initialize a swizzler. The swizzler lets us easily convert texel
    // addresses when the texture is swizzled.
    Swizzler swizzle( texWidth, texHeight, 0 );

    // Loop through and touch the texels. Note that SetU()/SetV() and
    // IncU()/IncV() are used to control texture addressed. This way,
    // the Get2D() function can be used to get the current address.
	// Initialize the V texture coordinate
    swizzle.SetV( 0 ); 

    for ( y = 0; y < texHeight; y++ )
    {
        swizzle.SetU( 0 ); 
		pSrc = pSource + y*texWidth*bytesPerPixel;
        for ( x = 0; x < texWidth; x++ )
        {
			pDst = pTarget + swizzle.Get2D()*bytesPerPixel;
			for ( n=0; n<bytesPerPixel; n++ )
			{
				pDst[n] = pSrc[n];
			}

            swizzle.IncU(); 
			pSrc += bytesPerPixel;
        }
        swizzle.IncV(); 
    }
}

//-----------------------------------------------------------------------------
// Quantize the 32b lightmap pages into a palettized equivalent ready for serialization
//-----------------------------------------------------------------------------
void PalettizeLightmaps( void )
{
	uint8	*pPixels;
	uint8	*pPalette3;
	uint8	*pPalette4;
	int		i;
	int		j;

	g_dLightmapPages.Purge();
	g_dLightmapPages.EnsureCount( g_rawLightmapPages.Count() );

	if ( g_bExportLightmaps )
	{
		// write out the raw 32bpp lightmap page
		for ( i=0; i<g_rawLightmapPages.Count(); ++i )
		{	
			char	buff[256];
			sprintf( buff, "lightmap_%2.2d.tga", i );

			CUtlBuffer outBuf;
			TGAWriter::WriteToBuffer( 
				g_rawLightmapPages[i], outBuf, MAX_LIGHTMAPPAGE_WIDTH, 
				MAX_LIGHTMAPPAGE_HEIGHT, IMAGE_FORMAT_RGBA8888, IMAGE_FORMAT_RGBA8888 );
			g_pFileSystem->WriteFile( buff, NULL, outBuf );			
		}
	}

	pPixels   = (uint8 *)malloc( MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT );
	pPalette3 = (uint8 *)malloc( 256*3 );
	pPalette4 = (uint8 *)malloc( 256*4 );

	for (i=0; i<g_rawLightmapPages.Count(); i++)
	{	
		// remap to 256x3 palette
		ColorQuantize( 
			g_rawLightmapPages[i], 
			MAX_LIGHTMAPPAGE_WIDTH, 
			MAX_LIGHTMAPPAGE_HEIGHT,
			QUANTFLAGS_NODITHER,
			256,
			pPixels,
			pPalette3,
			0 );

		// fixup 256x3 rgb palette to d3d 256x4 bgra
		for (j=0; j<256; j++)
		{
			pPalette4[j*4+0] = pPalette3[j*3+2];
			pPalette4[j*4+1] = pPalette3[j*3+1];
			pPalette4[j*4+2] = pPalette3[j*3+0];
			pPalette4[j*4+3] = 0xFF;
		}

		SwizzleRect( pPixels, g_dLightmapPages[i].data, MAX_LIGHTMAPPAGE_WIDTH, MAX_LIGHTMAPPAGE_HEIGHT, 1 );
		memcpy( g_dLightmapPages[i].palette, pPalette4, 256*4 );

		if ( g_bExportLightmaps && g_pFullFileSystem )
		{
			// write out the palettize page
			byte *pRaw4 = (byte *)malloc( MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT*4 );
			for (j=0; j<MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT; j++)
			{
				// index the palette, fixup to tga rgba order
				int color = pPixels[j]*4;
				pRaw4[j*4+0] = pPalette4[color+2];
				pRaw4[j*4+1] = pPalette4[color+1];
				pRaw4[j*4+2] = pPalette4[color+0];
				pRaw4[j*4+3] = pPalette4[color+3];
			}

			char	buff[256];
			sprintf(buff, "lightmap_%2.2d_256.tga", i);

			CUtlBuffer outBuf;
			TGAWriter::WriteToBuffer( 
				pRaw4, 
				outBuf, 
				MAX_LIGHTMAPPAGE_WIDTH, 
				MAX_LIGHTMAPPAGE_HEIGHT, 
				IMAGE_FORMAT_RGBA8888, 
				IMAGE_FORMAT_RGBA8888 );
			g_pFullFileSystem->WriteFile( buff, NULL, outBuf );

			free( pRaw4 );
		}
	}

	free( pPixels );
	free( pPalette3 );
	free( pPalette4 );
}


//-----------------------------------------------------------------------------
// Ensure the material names compare correctly by removing path or case disparity.
//-----------------------------------------------------------------------------
static int MaterialNameCompare(const char *pName1, const char *pName2)
{
	char	buff1[256];
	char	buff2[256];
	Assert( strlen(pName1) < sizeof(buff1) );
	Assert( strlen(pName2) < sizeof(buff2) );

	// normalize
	strcpy( buff1, pName1 );
	strlwr( buff1 );
	Q_FixSlashes(buff1, '/');

	// normalize
	strcpy( buff2, pName2 );
	strlwr( buff2 );
	Q_FixSlashes(buff2, '/');

	return strcmp(buff1, buff2);
}

//-----------------------------------------------------------------------------
// Allocate a surface's lightmap onto a page
// The materials must be presented in sorted order.
// Lightmaps are allocated onto succesive pages, and do not backfill.
//-----------------------------------------------------------------------------
static int AllocateLightmap( int width, int height, int offsetIntoLightmapPage[2], const char *pMaterialName )
{	
	// material change
	int			i;
	int			lightmapPage = -1;
	int			nPackCount;
	static int	imagePackerBase = 0;
	bool		bMaterialChange;

	bMaterialChange = false;
	nPackCount = g_ImagePackers.Count();
	if ( g_currentMaterialName[0] && MaterialNameCompare( g_currentMaterialName, pMaterialName ) )
	{
		// advance the lightmap page base to track the start of the active image packers
		// the closed out image packers are not suitable candidates for any further allocations
		if (nPackCount)
		{
			imagePackerBase = nPackCount-1;
		}
		bMaterialChange = true;
	}

	strcpy(g_currentMaterialName, pMaterialName);

	// Try to add it to any of the active image packers
	bool bAdded = false;
	for ( i = imagePackerBase; i < nPackCount; ++i )
	{
		bAdded = g_ImagePackers[i].AddBlock( width, height, &offsetIntoLightmapPage[0], &offsetIntoLightmapPage[1] );
		if ( bAdded )
		{
			lightmapPage = i;
			break;
		}
	}

	if ( !bAdded )
	{
		// allocate a new page
		lightmapPage = g_ImagePackers.AddToTail();
		g_ImagePackers[lightmapPage].Reset( MAX_LIGHTMAPPAGE_WIDTH, MAX_LIGHTMAPPAGE_HEIGHT );
	
		bAdded = g_ImagePackers[lightmapPage].AddBlock( width, height, &offsetIntoLightmapPage[0], &offsetIntoLightmapPage[1] );
		if ( !bAdded )
		{
			// couldn't add to new empty page, undo the new allocation
			g_ImagePackers.Remove( lightmapPage );
			return -1;
		}

		g_rawLightmapPages.AddToTail();
		g_rawLightmapPages[lightmapPage] = (byte *)malloc( MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT*4 );
		for (int n=0; n<MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT; n++)
		{
			if ( g_bExportLightmaps )
			{
				// debugging, fill empty areas with bright green
				((unsigned int *)g_rawLightmapPages[lightmapPage])[n] = 0xFF00FF00;
			}
			else
			{
				// fill empty areas with pure opaque black
				((unsigned int *)g_rawLightmapPages[lightmapPage])[n] = 0xFF000000;
			}
		}

		if (bMaterialChange && imagePackerBase == nPackCount-1)
		{
			// failed to add new material's lightmap to last open lightmap page, so forced to new page
			// then succeeded on new page
			// for integrity, ensure no further allocations end up on the old page
			imagePackerBase++;
		}
	}

	return lightmapPage;
}

//-----------------------------------------------------------------------------
// Rectangular blit from RGBExp32 lightmaps to final bumped format in lightmap page
//-----------------------------------------------------------------------------
static void BlitBumpedLightmapsToPage32bpp( byte *pSource0, byte *pSource1, byte *pSource2, byte *pSource3, byte *pTarget, int targetX, int targetY, int targetW, int targetH, int targetStride)
{
	int			x;
	int			y;
	byte		*pDst0;
	byte		*pDst1;
	byte		*pDst2;
	byte		*pDst3;
	byte		*pSrc0;
	byte		*pSrc1;
	byte		*pSrc2;
	byte		*pSrc3;
	float		linearColor[3];
	float		linearBumpColor1[3];
	float		linearBumpColor2[3];
	float		linearBumpColor3[3];
	byte		color[4];
	byte		bumpColor1[4];
	byte		bumpColor2[4];
	byte		bumpColor3[4];

	// add the ColorRGBExp32 bits to the page
	pSrc0 = pSource0;
	pSrc1 = pSource1;
	pSrc2 = pSource2;
	pSrc3 = pSource3;
	for ( y=0; y<targetH; ++y )
	{
		pDst0 = pTarget + ( (y + targetY)*targetStride + targetX + 0*targetW)*sizeof(ColorRGBExp32);
		pDst1 = pTarget + ( (y + targetY)*targetStride + targetX + 1*targetW)*sizeof(ColorRGBExp32);
		pDst2 = pTarget + ( (y + targetY)*targetStride + targetX + 2*targetW)*sizeof(ColorRGBExp32);
		pDst3 = pTarget + ( (y + targetY)*targetStride + targetX + 3*targetW)*sizeof(ColorRGBExp32);
		for ( x=0; x<targetW; ++x )
		{
			// convert from ColorRGBExp32 to linear space
			linearColor[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc0)->r, ((ColorRGBExp32 *)pSrc0)->exponent );
			linearColor[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc0)->g, ((ColorRGBExp32 *)pSrc0)->exponent );
			linearColor[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc0)->b, ((ColorRGBExp32 *)pSrc0)->exponent );

			linearBumpColor1[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc1)->r, ((ColorRGBExp32 *)pSrc1)->exponent );
			linearBumpColor1[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc1)->g, ((ColorRGBExp32 *)pSrc1)->exponent );
			linearBumpColor1[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc1)->b, ((ColorRGBExp32 *)pSrc1)->exponent );

			linearBumpColor2[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc2)->r, ((ColorRGBExp32 *)pSrc2)->exponent );
			linearBumpColor2[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc2)->g, ((ColorRGBExp32 *)pSrc2)->exponent );
			linearBumpColor2[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc2)->b, ((ColorRGBExp32 *)pSrc2)->exponent );

			linearBumpColor3[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc3)->r, ((ColorRGBExp32 *)pSrc3)->exponent );
			linearBumpColor3[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc3)->g, ((ColorRGBExp32 *)pSrc3)->exponent );
			linearBumpColor3[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc3)->b, ((ColorRGBExp32 *)pSrc3)->exponent );

			LinearToBumpedLightmap(
				linearColor, 
				linearBumpColor1,
				linearBumpColor2, 
				linearBumpColor3,
				color,
				bumpColor1,
				bumpColor2,
				bumpColor3 );

			pDst0[0] = color[0];
			pDst0[1] = color[1];
			pDst0[2] = color[2];
			pDst0[3] = 255;

			pDst1[0] = bumpColor1[0];
			pDst1[1] = bumpColor1[1];
			pDst1[2] = bumpColor1[2];
			pDst1[3] = 255;

			pDst2[0] = bumpColor2[0];
			pDst2[1] = bumpColor2[1];
			pDst2[2] = bumpColor2[2];
			pDst2[3] = 255;

			pDst3[0] = bumpColor3[0];
			pDst3[1] = bumpColor3[1];
			pDst3[2] = bumpColor3[2];
			pDst3[3] = 255;

			pSrc0 += sizeof( ColorRGBExp32 );
			pSrc1 += sizeof( ColorRGBExp32 );
			pSrc2 += sizeof( ColorRGBExp32 );
			pSrc3 += sizeof( ColorRGBExp32 );

			pDst0 += 4;
			pDst1 += 4;
			pDst2 += 4;
			pDst3 += 4;
		}
	}
}

//-----------------------------------------------------------------------------
// Rectangular blit from RGBExp32 lightmaps to final color format in lightmap page
//-----------------------------------------------------------------------------
static void BlitLightmapToPage32bpp( byte *pSource, byte *pTarget, int targetX, int	targetY, int targetW, int targetH, int targetStride)
{
	int			x;
	int			y;
	byte		*pDst;
	byte		*pSrc;

	// add the ColorRGBExp32 bits to the page
	pSrc = pSource;
	for ( y=0; y<targetH; ++y )
	{
		pDst = pTarget + ( (y + targetY)*targetStride + targetX )*4;
		for ( x=0; x<targetW; ++x )
		{
			ConvertRGBExp32ToRGBA8888( (ColorRGBExp32 *)pSrc, pDst );

			pSrc += sizeof( ColorRGBExp32 );
			pDst += 4;
		}
	}
}

//-----------------------------------------------------------------------------
// Prepare and drive the allocation of the surface's lightmap
//-----------------------------------------------------------------------------
static void RegisterLightmappedSurface( dface_t *pFace )
{
	int			lightmapWidth;
	int			lightmapHeight;
	int			allocationWidth;
	int			allocationHeight;
	int			offsetIntoLightmapPage[2];
	const char	*pMaterialName;
	int			lightmapPage;
	bool		bBump;

	pMaterialName = TexDataStringTable_GetString( dtexdata[texinfo[pFace->texinfo].texdata].nameStringTableID );

	lightmapWidth  = pFace->m_LightmapTextureSizeInLuxels[0] + 1;
	lightmapHeight = pFace->m_LightmapTextureSizeInLuxels[1] + 1;
	
	allocationWidth  = lightmapWidth;
	allocationHeight = lightmapHeight;
	bBump            = ( texinfo[pFace->texinfo].flags & SURF_BUMPLIGHT ) > 0;
	if ( bBump )
	{
		// Allocate all bumped lightmaps next to each other so that we can just 
		// increment the s texcoord by pSurf->bumpSTexCoordOffset to render the next
		// of the three lightmaps
		allocationWidth *= NUM_BUMP_VECTS+1;
	}

	// register this surface's lightmap
	lightmapPage = AllocateLightmap( allocationWidth, allocationHeight,	offsetIntoLightmapPage,	pMaterialName );

//	Warning( "%s: page %d (%dx%d)\n", pMaterialName, lightmapPage, allocationWidth, allocationHeight );

	if ( lightmapPage >= 0 )
	{
		// rectlinear transfer each page
		if (!bBump)
		{
			BlitLightmapToPage32bpp(
				pdlightdata->Base() + pFace->lightofs,
				g_rawLightmapPages[lightmapPage],
				offsetIntoLightmapPage[0],
				offsetIntoLightmapPage[1],
				lightmapWidth,
				lightmapHeight,
				MAX_LIGHTMAPPAGE_WIDTH );
		}
		else
		{
			int bumpMapSize = lightmapWidth*lightmapHeight*sizeof(ColorRGBExp32);
			BlitBumpedLightmapsToPage32bpp(
				pdlightdata->Base() + pFace->lightofs + 0*bumpMapSize,
				pdlightdata->Base() + pFace->lightofs + 1*bumpMapSize,
				pdlightdata->Base() + pFace->lightofs + 2*bumpMapSize,
				pdlightdata->Base() + pFace->lightofs + 3*bumpMapSize,
				g_rawLightmapPages[lightmapPage],
				offsetIntoLightmapPage[0],
				offsetIntoLightmapPage[1],
				lightmapWidth,
				lightmapHeight,
				MAX_LIGHTMAPPAGE_WIDTH );
		}

		// build a new lightmap page info
		// lightstyles are xbox deprecated, only need single average light color for static style 0
		int pageInfo = g_dLightmapPageInfos.AddToTail();
		g_dLightmapPageInfos[pageInfo].page      = lightmapPage;
		g_dLightmapPageInfos[pageInfo].offset[0] = offsetIntoLightmapPage[0];
		g_dLightmapPageInfos[pageInfo].offset[1] = offsetIntoLightmapPage[1];
		g_dLightmapPageInfos[pageInfo].avgColor  = *dface_AvgLightColor( pFace, 0 );

		// hijack lightofs, to index into page infos instead
		pFace->lightofs = pageInfo;
	}
	else
	{
		Error( "AllocateLightmap: (%s) lightmap (%dx%d) too big to fit in page (%dx%d)\n", 
			pMaterialName, allocationWidth, allocationHeight, MAX_LIGHTMAPPAGE_WIDTH, MAX_LIGHTMAPPAGE_HEIGHT );

		// mark as unallocated
		pFace->lightofs = -1;
	}
}

//-----------------------------------------------------------------------------
// Comparison function for inserting surfaces to achieve desired sort order
//-----------------------------------------------------------------------------
static bool LightmapLess( const int &surfID1, const int &surfID2 )
{
	dface_t *pFace1 = &g_pFaces[surfID1];
	dface_t *pFace2 = &g_pFaces[surfID2];

	bool hasLightmap1 = (texinfo[pFace1->texinfo].flags & SURF_NOLIGHT) == 0;
	bool hasLightmap2 = (texinfo[pFace2->texinfo].flags & SURF_NOLIGHT) == 0;

	// We want lightmapped surfaces to show up first
	if (hasLightmap1 != hasLightmap2)
		return hasLightmap1 > hasLightmap2;
	
	// sort by name
	const char *pName1 = TexDataStringTable_GetString( dtexdata[texinfo[pFace1->texinfo].texdata].nameStringTableID );
	const char *pName2 = TexDataStringTable_GetString( dtexdata[texinfo[pFace2->texinfo].texdata].nameStringTableID );
	int sort = MaterialNameCompare( pName1, pName2 );
	if (sort)
		return sort < 0;

	// then sort by lightmap area for better packing... (big areas first)
	// must account for bumped surfaces to ensure proper descending sequences of areas
	int width  = g_pFaces[surfID1].m_LightmapTextureSizeInLuxels[0]+1;
	int height = g_pFaces[surfID1].m_LightmapTextureSizeInLuxels[1]+1;
	if ( texinfo[pFace1->texinfo].flags & SURF_BUMPLIGHT )
	{
		width *= ( NUM_BUMP_VECTS+1 );
	}
	int area1 = width * height;

	width  = g_pFaces[surfID2].m_LightmapTextureSizeInLuxels[0]+1;
	height = g_pFaces[surfID2].m_LightmapTextureSizeInLuxels[1]+1;
	if ( texinfo[pFace2->texinfo].flags & SURF_BUMPLIGHT )
	{
		width *= ( NUM_BUMP_VECTS+1 );
	}
	int area2 = width * height;

	return area2 < area1;
}

//-----------------------------------------------------------------------------
// Iterate surfaces generating cooked lightmap pages for palettizing
// Each page gets its own palette.
//-----------------------------------------------------------------------------
void BuildPalettedLightmaps( void )
{
	int		i;
	int		j;

	g_rawLightmapPages.Purge();
	g_currentMaterialName[0] = '\0';

	g_dLightmapPageInfos.Purge();

	CUtlRBTree< int, int >	surfaces( 0, numfaces, LightmapLess );
	for ( i = 0; i < numfaces; i++ )
	{
		surfaces.Insert( i );
	}

	// surfaces must be iterated in order to achieve proper lightmap allocation
	for ( i = surfaces.FirstInorder(); i != surfaces.InvalidIndex(); i = surfaces.NextInorder(i) )
	{
		dface_t *pFace = &g_pFaces[surfaces[i]];

		bool hasLightmap = (texinfo[pFace->texinfo].flags & SURF_NOLIGHT) == 0;
		if ( hasLightmap && pFace->lightofs != -1 )
		{
			RegisterLightmappedSurface( pFace );
		}
		else
		{
			pFace->lightofs = -1;
		}

		// remove unsupported light styles
		for (j=0; j<MAXLIGHTMAPS; j++)
			pFace->styles[j] = 255;
	}

	// convert cooked 32b lightmap pages into their palettized versions
	PalettizeLightmaps();

	for (i=0; i<g_rawLightmapPages.Count(); i++)
		free( g_rawLightmapPages[i] );
	
	g_rawLightmapPages.Purge();
	g_ImagePackers.Purge();
	
	// no longer require raw rgb light data
	// ensure light data doesn't get serialized
	pdlightdata->Purge();
}

// a temporary struct that holds a x/y/z/u/v for recursive subdivision luxel rasterization
struct facevert_t
{
	Vector pos;
	Vector2D luxel;
};

// diff these two color channels
inline int ColorDelta( int c0, int c1 )
{
	return abs(c0-c1);
}

// holds the lightmap data for a face for intermediate calcs
struct lightmapdata_t
{
	bool bBump;
	ColorRGBExp32 avgColor;
	ColorRGBExp32 bumpColor[NUM_BUMP_VECTS];
	int		width; // width including *4 for bumps
	int		height;
	int		threshold;
	ColorRGBExp32 *pLuxels;

	void Init( ColorRGBExp32 *_pLuxels, bool _bBump, int _width, int _height, int _threshold )
	{
		pLuxels = _pLuxels;
		bBump = _bBump;
		width = _width;
		height = _height;
		threshold = _threshold;
	}

	// Get the color of the luxel at s/t
	void GetLuxelAtCoords( ColorRGBExp32 *pDest, float s, float t, int bumpIndex = 0 ) const
	{
		s += 0.5f;
		t += 0.5f;
		int u = (int)s;
		int v = (int)t;
#if _DEBUG
		int w = bBump ? (width/4) : width;
		Assert( u >= 0 && u <= (w-1) );
		Assert( v >= 0 && v <= (height-1) );
#endif
		int offset = u + v * width;
		if ( bBump )
		{
			offset += bumpIndex * (width/4);
		}
		if ( offset > width * height )
			offset = width * height;
		*pDest = pLuxels[offset];
	}

	// Is this color within threshold of the average color for this lightmap
	bool IsAvgColor( ColorRGBExp32 &color ) const
	{
		byte color0[4], color1[4];
		ConvertRGBExp32ToRGBA8888( &avgColor, color0 );
		ConvertRGBExp32ToRGBA8888( &color, color1 );
		int delta = ColorDelta( color0[0], color1[0] );
		delta += ColorDelta( color0[1], color1[1] );
		delta += ColorDelta( color0[2], color1[2] );
		return delta > threshold ? false : true;
	}

	bool IsAvgColor( float s, float t ) const
	{
#if 0
		GetLuxelAtCoords( &tmp, s, t );
		return IsAvgColor( tmp );
#else
		// billinear sample luxel at s,t
		int u = s;
		int v = t;
		float fracU = fmod(s,1) + 0.5f;
		float fracV = fmod(t,1) + 0.5f;
		if ( fracU > 1 )
		{
			fracU -= 1.0f;
			u++;
		}
		if ( fracV > 1 )
		{
			fracV -= 1.0f;
			v++;
		}
		int w = bBump ? (width/4) : width;
		Vector luxel[4];
		for ( int i = 0; i < 4; i++ )
		{
			int sampU = u + ((i&1)?1:0);
			int sampV = v + ((i&2)?1:0);
			sampU = clamp(sampU, 0, w-1);
			sampV = clamp(sampV, 0, height-1);
			int offset = sampU + sampV*width;
			luxel[i][0] = TexLightToLinear( pLuxels[offset].r, pLuxels[offset].exponent );
			luxel[i][1] = TexLightToLinear( pLuxels[offset].g, pLuxels[offset].exponent );
			luxel[i][2] = TexLightToLinear( pLuxels[offset].b, pLuxels[offset].exponent );
		}
		luxel[0] *= (1.0f - fracU) * (1.0f - fracV);
		luxel[1] *= fracU * (1.0f - fracV);
		luxel[2] *= (1.0f - fracU) * fracV;
		luxel[3] *= fracU * fracV;
		Vector out = luxel[0] + luxel[1] + luxel[2] + luxel[3];

		Vector gammaOut;
		for ( int i = 0; i < 3; i++ )
		{
			gammaOut[i] = LinearToVertexLight( out[i] );
		}
		ColorClamp(gammaOut);
		byte avg[4];
		ConvertRGBExp32ToRGBA8888( &avgColor, avg );
		int delta = 0;
		for ( int i = 0; i < 3; i++ )
		{
			int c = RoundFloatToByte( gammaOut[i] * 255.0f );
			delta += ColorDelta( c, avg[i] );
		}
		return delta > threshold ? false : true;
#endif
	}
};

// Returns true if this triangle has constant lighting over its surface
// This is evaluated by recursively subdividing the triangle until it is less than 1 luxel along each edge
// then the entire triangle is used to point-sample a luxel which is compared to the average for constant-ness
bool IsConstantTriangle_r( const lightmapdata_t &data, const facevert_t &v0, const facevert_t &v1, const facevert_t &v2 )
{
	float edge[3];
	edge[0] = (v1.luxel - v0.luxel).Length();
	edge[1] = (v2.luxel - v1.luxel).Length();
	edge[2] = (v0.luxel - v2.luxel).Length();

	int maxIndex = 0;
	for ( int i = 1; i < 3; i++ )
	{
		if ( edge[i] > edge[maxIndex] )
		{
			maxIndex = i;
		}
	}
	if ( edge[maxIndex] < 0.5f )
	{
		// smaller than 1 luxel
		return data.IsAvgColor( v0.luxel[0], v0.luxel[1] );
#if 0
		ColorRGBExp32 tmp0, tmp1, tmp2;
		data.GetLuxelAtCoords( &tmp0, v0.luxel[0], v0.luxel[1] );
		data.GetLuxelAtCoords( &tmp1, v1.luxel[0], v1.luxel[1] );
		data.GetLuxelAtCoords( &tmp2, v2.luxel[0], v2.luxel[1] );
		return data.IsAvgColor( tmp0 ) && data.IsAvgColor( tmp1 ) && data.IsAvgColor( tmp2 );
#endif
	}
	// subdivide largest edge and recurse
	switch( maxIndex )
	{
	case 0: // v1, v0
		{
			facevert_t tmp;
			tmp.pos = 0.5f * v1.pos + 0.5f * v0.pos;
			tmp.luxel = 0.5f * v1.luxel + 0.5f * v0.luxel;
			if ( !IsConstantTriangle_r( data, tmp, v1, v2 ))
				return false;
			return IsConstantTriangle_r( data, v0, tmp, v2 );
		}
		break;
	case 1: // v2, v1
		{
			facevert_t tmp;
			tmp.pos = 0.5f * v2.pos + 0.5f * v1.pos;
			tmp.luxel = 0.5f * v2.luxel + 0.5f * v1.luxel;
			if ( !IsConstantTriangle_r( data, v0, v1, tmp ))
				return false;
			return IsConstantTriangle_r( data, v0, tmp, v2 );
		}
		break;
	default: // v0, v2
	case 2:
		{
			facevert_t tmp;
			tmp.pos = 0.5f * v2.pos + 0.5f * v0.pos;
			tmp.luxel = 0.5f * v2.luxel + 0.5f * v0.luxel;
			if ( !IsConstantTriangle_r( data, v0, v1, tmp ))
				return false;
			return IsConstantTriangle_r( data, tmp, v1, v2 );
		}
		break;
	}
}

// Check to see if this face has constant lighting by recursive rasterization into the lightmap
bool IsConstantColor( dface_t *pFace, lightmapdata_t &data )
{
	int vertIndex[256];
	facevert_t v[256];
	int i;
	Assert( pFace->numedges < 256 );
	texinfo_t *pTexInfo = &texinfo[ pFace->texinfo ];
	BuildFaceCalcWindingData( pFace, vertIndex );
#if _DEBUG
	int wLimit = data.bBump ? (data.width/4) : data.width;
	int hLimit = data.height;
#endif
	for ( i = 0; i < pFace->numedges; i++ )
	{
		v[i].pos = dvertexes[vertIndex[i]].point;

		CalcTextureCoordsAtPoints( pTexInfo->lightmapVecsLuxelsPerWorldUnits, pFace->m_LightmapTextureMinsInLuxels, &v[i].pos, 1, &v[i].luxel );
		if ( v[i].luxel[0] < 0 )
		{
			if ( v[i].luxel[0] > -1e-3f ) // clamp away a little bit of error
				v[i].luxel[0] = 0;
		}
		if ( v[i].luxel[1] < 0 )
		{
			if ( v[i].luxel[1] > -1e-3f ) // clamp away a little bit of error
				v[i].luxel[1] = 0;
		}
		Assert( v[i].luxel[0] >= 0 && v[i].luxel[0] < wLimit );
		Assert( v[i].luxel[1] >= 0 && v[i].luxel[1] < hLimit );
	}

	// initialize the average color to be the color under the first vertex
	data.GetLuxelAtCoords( &data.avgColor, v[0].luxel[0], v[0].luxel[1] );
	if ( data.bBump )	
	{
		for ( i = 0; i < NUM_BUMP_VECTS; i++ )
		{
			data.GetLuxelAtCoords( &data.bumpColor[i], v[0].luxel[0], v[0].luxel[1], i+1 );
		}
	}

	if ( pFace->GetNumPrims() )
	{
		// use the stored indices
		dprimitive_t *pPrim = &g_primitives[pFace->firstPrimID];
		Assert(pPrim->vertCount == 0);
		Assert(pPrim->indexCount == (pFace->numedges-2)*3);
		unsigned short *pIndices = &g_primindices[pPrim->firstIndex];
		int index = 0;
		for ( i = 0; i < pFace->numedges-2; i++ )
		{
			if ( !IsConstantTriangle_r( data, v[pIndices[index]], v[pIndices[index+1]], v[pIndices[index+2]] ) )
				return false;
			index += 3;
		}
	}
	else
	{
		// this surface is a fan
		int index = 0;
		for ( i = 0; i < pFace->numedges-2; i++ )
		{
			if ( !IsConstantTriangle_r( data, v[0], v[i+1], v[i+2] ) )
				return false;
			index += 3;
		}
	}

	return true;
}

// Iterate the faces and remove any lightmaps that are constant across their renderable luxels
void CompressConstantLightmaps( int threshold )
{
	lightmapdata_t mapData;

	for ( int i = 0; i < numfaces; i++ )
	{
		dface_t *pFace = &g_pFaces[i];
		bool hasLightmap = (texinfo[pFace->texinfo].flags & SURF_NOLIGHT) == 0;
		if ( !hasLightmap || pFace->lightofs == -1 )
			continue;

		int lightmapWidth  = pFace->m_LightmapTextureSizeInLuxels[0] + 1;
		int lightmapHeight = pFace->m_LightmapTextureSizeInLuxels[1] + 1;
		bool bBump = ( texinfo[pFace->texinfo].flags & SURF_BUMPLIGHT ) > 0;
		if ( bBump )
		{
			lightmapWidth *= (NUM_BUMP_VECTS+1);
		}

		ColorRGBExp32 *pLightdata = reinterpret_cast<ColorRGBExp32 *>(pdlightdata->Base() + pFace->lightofs);

		mapData.Init( pLightdata, bBump, lightmapWidth, lightmapHeight, threshold );
		if ( IsConstantColor( pFace, mapData ) )
		{
			pLightdata[0] = mapData.avgColor;
			pFace->m_LightmapTextureSizeInLuxels[0] = 0;
			pFace->m_LightmapTextureSizeInLuxels[1] = 0;
			if ( bBump )
			{
				for ( int i = 0; i < NUM_BUMP_VECTS; i++ )
				{
					pLightdata[i+1] = mapData.bumpColor[i];
				}
			}
		}
	}
}

