//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================//
#ifdef _WIN32
#include <windows.h>
#endif
#include "vrad.h"
#include "Vector.h"
#include "UtlBuffer.h"
#include "UtlVector.h"
#include "GameBSPFile.h"
#include "BSPTreeData.h"
#include "VPhysics_Interface.h"
#include "Studio.h"
#include "Optimize.h"
#include "Bsplib.h"
#include "CModel.h"
#include "PhysDll.h"
#include "phyfile.h"
#include "collisionutils.h"
#include "pacifier.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/hardwareverts.h"

static void SetCurrentModel( studiohdr_t *pStudioHdr );

#define ALIGN_TO_POW2(x,y) (((x)+(y-1))&~(y-1))

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

// DON'T USE THIS FROM WITHIN A THREAD.  THERE IS A THREAD CONTEXT CREATED 
// INSIDE PropTested_t.  USE THAT INSTEAD.
IPhysicsCollision *s_pPhysCollision = NULL;

//-----------------------------------------------------------------------------
// Vrad's static prop manager
//-----------------------------------------------------------------------------

class CVradStaticPropMgr : public IVradStaticPropMgr, public ISpatialLeafEnumerator,
							public IBSPTreeDataEnumerator
{
public:
	// constructor, destructor
	CVradStaticPropMgr();
	virtual ~CVradStaticPropMgr();

	// methods of IStaticPropMgr
	void Init();
	void Shutdown();
	bool ClipRayToStaticProps( PropTested_t& propTested, Ray_t const& ray );
	bool ClipRayToStaticPropsInLeaf( PropTested_t& propTested, Ray_t const& ray, int leaf );
	void StartRayTest( PropTested_t& propTested );

	// ISpatialLeafEnumerator
	bool EnumerateLeaf( int leaf, int context );

	// IBSPTreeDataEnumerator
	bool FASTCALL EnumerateElement( int userId, int context );

	// iterate all the instanced static props and compute their vertex lighting
	void ComputeLighting( int iThread );

private:
	// Methods associated with unserializing static props
	void UnserializeModelDict( CUtlBuffer& buf );
	void UnserializeModels( CUtlBuffer& buf );
	void UnserializeStaticProps();

	// For raycasting against props
	void InsertPropIntoTree( int propIndex );
	void RemovePropFromTree( int propIndex );

	// Creates a collision model
	void CreateCollisionModel( char const* pModelName );

private:
	// Unique static prop models
	struct StaticPropDict_t
	{
		vcollide_t		m_loadedModel;
		CPhysCollide*	m_pModel;
		Vector			m_Mins;			// Bounding box is in local coordinates
		Vector			m_Maxs;
		studiohdr_t*	m_pStudioHdr;
		CUtlBuffer		m_VtxBuf;
	};

	struct MeshData_t
	{
		CUtlVector<Vector>	m_Verts;
		int					m_nLod;
	};

	// A static prop instance
	struct CStaticProp
	{
		Vector					m_Origin;
		QAngle					m_Angles;
		Vector					m_mins;
		Vector					m_maxs;
		Vector					m_LightingOrigin;
		int						m_ModelIdx;
		BSPTreeDataHandle_t		m_Handle;
		CUtlVector<MeshData_t>	m_MeshData;
		bool					m_bLightingOriginValid;
		int                     m_Flags;
	};

	// Enumeration context
	struct EnumContext_t
	{
		PropTested_t* m_pPropTested;
		Ray_t const* m_pRay;
	};

	// The list of all static props
	CUtlVector <StaticPropDict_t>	m_StaticPropDict;
	CUtlVector <CStaticProp>		m_StaticProps;

	IBSPTreeData*	m_pBSPTreeData;

	bool m_bIgnoreStaticPropTrace;

	void ComputeLighting( CStaticProp &prop, int iThread, int prop_index );
	void SerializeLighting();
	void AddPolysForRayTrace();
};


//-----------------------------------------------------------------------------
// Expose IVradStaticPropMgr to vrad
//-----------------------------------------------------------------------------

static CVradStaticPropMgr	g_StaticPropMgr;
IVradStaticPropMgr* StaticPropMgr()
{
	return &g_StaticPropMgr;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CVradStaticPropMgr::CVradStaticPropMgr()
{
	m_pBSPTreeData = CreateBSPTreeData();

	// set to ignore static prop traces
	m_bIgnoreStaticPropTrace = false;
}

CVradStaticPropMgr::~CVradStaticPropMgr()
{
	DestroyBSPTreeData( m_pBSPTreeData );
}


//-----------------------------------------------------------------------------
// Insert, remove a static prop from the tree for collision
//-----------------------------------------------------------------------------

void CVradStaticPropMgr::InsertPropIntoTree( int propIndex )
{
	CStaticProp& prop = m_StaticProps[propIndex];

	StaticPropDict_t& dict = m_StaticPropDict[prop.m_ModelIdx];

	// Compute the bbox of the prop
	if ( dict.m_pModel )
	{
		s_pPhysCollision->CollideGetAABB( prop.m_mins, prop.m_maxs, dict.m_pModel, prop.m_Origin, prop.m_Angles );
	}
	else
	{
		VectorAdd( dict.m_Mins, prop.m_Origin, prop.m_mins );
		VectorAdd( dict.m_Maxs, prop.m_Origin, prop.m_maxs );
	}

	// add the entity to the tree so we will collide against it
	prop.m_Handle = m_pBSPTreeData->Insert( propIndex, prop.m_mins, prop.m_maxs );
}

void CVradStaticPropMgr::RemovePropFromTree( int propIndex )
{
	// Release the tree handle
	if (m_StaticProps[propIndex].m_Handle != TREEDATA_INVALID_HANDLE)
	{
		m_pBSPTreeData->Remove( m_StaticProps[propIndex].m_Handle );
		m_StaticProps[propIndex].m_Handle = TREEDATA_INVALID_HANDLE;
	}
}


//-----------------------------------------------------------------------------
// Makes sure the studio model is a static prop
//-----------------------------------------------------------------------------

bool IsStaticProp( studiohdr_t* pHdr )
{
	if (!(pHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP))
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Load a file into a Utlbuf
//-----------------------------------------------------------------------------
static bool LoadFile( char const* pFileName, CUtlBuffer& buf )
{
	if ( !g_pFullFileSystem )
		return false;

	return g_pFullFileSystem->ReadFile( pFileName, NULL, buf );
}


//-----------------------------------------------------------------------------
// Constructs the file name from the model name
//-----------------------------------------------------------------------------
static char const* ConstructFileName( char const* pModelName )
{
	static char buf[1024];
	sprintf( buf, "%s%s", gamedir, pModelName );
	return buf;
}


//-----------------------------------------------------------------------------
// Computes a convex hull from a studio mesh
//-----------------------------------------------------------------------------
static CPhysConvex* ComputeConvexHull( mstudiomesh_t* pMesh )
{
	const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData();

	// Generate a list of all verts in the mesh
	Vector** ppVerts = (Vector**)_alloca(pMesh->numvertices * sizeof(Vector*) );
	for (int i = 0; i < pMesh->numvertices; ++i)
	{
		ppVerts[i] = vertData->Position(i);
	}

	// Generate a convex hull from the verts
	return s_pPhysCollision->ConvexFromVerts( ppVerts, pMesh->numvertices );
}


//-----------------------------------------------------------------------------
// Computes a convex hull from the studio model
//-----------------------------------------------------------------------------
CPhysCollide* ComputeConvexHull( studiohdr_t* pStudioHdr )
{
	CUtlVector<CPhysConvex*>	convexHulls;

	for (int body = 0; body < pStudioHdr->numbodyparts; ++body )
	{
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( body );
		for( int model = 0; model < pBodyPart->nummodels; ++model )
		{
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( model );
			for( int mesh = 0; mesh < pStudioModel->nummeshes; ++mesh )
			{
				// Make a convex hull for each mesh
				// NOTE: This won't work unless the model has been compiled
				// with $staticprop
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( mesh );
				convexHulls.AddToTail( ComputeConvexHull( pStudioMesh ) );
			}
		}
	}

	// Convert an array of convex elements to a compiled collision model
	// (this deletes the convex elements)
	return s_pPhysCollision->ConvertConvexToCollide( convexHulls.Base(), convexHulls.Size() );
}


//-----------------------------------------------------------------------------
// Load studio model vertex data from a file...
//-----------------------------------------------------------------------------

bool LoadStudioModel( char const* pModelName, CUtlBuffer& buf )
{
	// No luck, gotta build it	
	// Construct the file name...
	if (!LoadFile( pModelName, buf ))
	{
		Warning("Error! Unable to load model \"%s\"\n", pModelName );
		return false;
	}

	// Check that it's valid
	if (strncmp ((const char *) buf.PeekGet(), "IDST", 4) &&
		strncmp ((const char *) buf.PeekGet(), "IDAG", 4))
	{
		Warning("Error! Invalid model file \"%s\"\n", pModelName );
		return false;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.PeekGet();

	Studio_ConvertStudioHdrToNewVersion( pHdr );

	if (pHdr->version != STUDIO_VERSION)
	{
		Warning("Error! Invalid model version \"%s\"\n", pModelName );
		return false;
	}

	if (!IsStaticProp(pHdr))
	{
		Warning("Error! To use model \"%s\"\n"
			"      as a static prop, it must be compiled with $staticprop!\n", pModelName );
		return false;
	}

	// ensure reset
	pHdr->pVertexBase = NULL;
	pHdr->pIndexBase  = NULL;

	return true;
}

bool LoadStudioCollisionModel( char const* pModelName, CUtlBuffer& buf )
{
	char tmp[1024];
	Q_strncpy( tmp, pModelName, sizeof( tmp ) );
	Q_SetExtension( tmp, ".phy", sizeof( tmp ) );
	// No luck, gotta build it	
	if (!LoadFile( tmp, buf ))
	{
		// this is not an error, the model simply has no PHY file
		return false;
	}

	phyheader_t *header = (phyheader_t *)buf.PeekGet();

	if ( header->size != sizeof(*header) || header->solidCount <= 0 )
		return false;

	return true;
}

bool LoadVTXFile( char const* pModelName, const studiohdr_t *pStudioHdr, CUtlBuffer& buf )
{
	char	filename[MAX_PATH];

	// construct filename
	Q_StripExtension( pModelName, filename, sizeof( filename ) );
	strcat( filename, g_bXBox ? ".xbox.vtx" : ".dx80.vtx" );

	if ( !LoadFile( filename, buf ) )
	{
		Warning( "Error! Unable to load file \"%s\"\n", filename );
		return false;
	}

	OptimizedModel::FileHeader_t* pVtxHdr = (OptimizedModel::FileHeader_t *)buf.Base();

	// Check that it's valid
	if ( pVtxHdr->version != OPTIMIZED_MODEL_FILE_VERSION )
	{
		Warning( "Error! Invalid VTX file version: %d, expected %d \"%s\"\n", pVtxHdr->version, OPTIMIZED_MODEL_FILE_VERSION, filename );
		return false;
	}
	if ( pVtxHdr->checkSum != pStudioHdr->checksum )
	{
		Warning( "Error! Invalid VTX file checksum: %d, expected %d \"%s\"\n", pVtxHdr->checkSum, pStudioHdr->checksum, filename );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Gets a vertex position from a strip index
//-----------------------------------------------------------------------------
inline static Vector* PositionFromIndex( const mstudio_meshvertexdata_t *vertData, mstudiomesh_t* pMesh, OptimizedModel::StripGroupHeader_t* pStripGroup, int i )
{
	OptimizedModel::Vertex_t* pVert = pStripGroup->pVertex( i );
	return vertData->Position( pVert->origMeshVertID );
}


//-----------------------------------------------------------------------------
// Purpose: Writes a glview text file containing the collision surface in question
// Input  : *pCollide - 
//			*pFilename - 
//-----------------------------------------------------------------------------
void DumpCollideToGlView( vcollide_t *pCollide, const char *pFilename )
{
	if ( !pCollide )
		return;

	Msg("Writing %s...\n", pFilename );

	FILE *fp = fopen( pFilename, "w" );
	for (int i = 0; i < pCollide->solidCount; ++i)
	{
		Vector *outVerts;
		int vertCount = s_pPhysCollision->CreateDebugMesh( pCollide->solids[i], &outVerts );
		int triCount = vertCount / 3;
		int vert = 0;

		unsigned char r = (i & 1) * 64 + 64;
		unsigned char g = (i & 2) * 64 + 64;
		unsigned char b = (i & 4) * 64 + 64;

		float fr = r / 255.0f;
		float fg = g / 255.0f;
		float fb = b / 255.0f;

		for ( int i = 0; i < triCount; i++ )
		{
			fprintf( fp, "3\n" );
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
			fprintf( fp, "%6.3f %6.3f %6.3f %.2f %.3f %.3f\n", 
				outVerts[vert].x, outVerts[vert].y, outVerts[vert].z, fr, fg, fb );
			vert++;
		}
		s_pPhysCollision->DestroyDebugMesh( vertCount, outVerts );
	}
	fclose( fp );
}


//-----------------------------------------------------------------------------
// Creates a collision model (based on the render geometry!)
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::CreateCollisionModel( char const* pModelName )
{
	CUtlBuffer buf;
	CUtlBuffer bufvtx;
	CUtlBuffer bufphy;

	int i = m_StaticPropDict.AddToTail();
	m_StaticPropDict[i].m_pModel = NULL;
	m_StaticPropDict[i].m_pStudioHdr = NULL;

	if ( !LoadStudioModel( pModelName, buf ) )
	{
		VectorCopy( vec3_origin, m_StaticPropDict[i].m_Mins );
		VectorCopy( vec3_origin, m_StaticPropDict[i].m_Maxs );
		return;
	}

	studiohdr_t* pHdr = (studiohdr_t*)buf.Base();

	// necessary for vertex access
	SetCurrentModel( pHdr );

	VectorCopy( pHdr->hull_min, m_StaticPropDict[i].m_Mins );
	VectorCopy( pHdr->hull_max, m_StaticPropDict[i].m_Maxs );

	if ( LoadStudioCollisionModel( pModelName, bufphy ) )
	{
		phyheader_t header;
		bufphy.Get( &header, sizeof(header) );

		vcollide_t *pCollide = &m_StaticPropDict[i].m_loadedModel;
		s_pPhysCollision->VCollideLoad( pCollide, header.solidCount, (const char *)bufphy.PeekGet(), bufphy.TellPut() - bufphy.TellGet() );
		m_StaticPropDict[i].m_pModel = m_StaticPropDict[i].m_loadedModel.solids[0];

		/*
		static int propNum = 0;
		char tmp[128];
		sprintf( tmp, "staticprop%03d.txt", propNum );
		DumpCollideToGlView( pCollide, tmp );
		++propNum;
		*/
	}
	else
	{
		// mark this as unused
		m_StaticPropDict[i].m_loadedModel.solidCount = 0;

		// CPhysCollide* pPhys = CreatePhysCollide( pHdr, pVtxHdr );
		m_StaticPropDict[i].m_pModel = ComputeConvexHull( pHdr );
	}

	// clone it
	m_StaticPropDict[i].m_pStudioHdr = (studiohdr_t *)malloc( buf.Size() );
	memcpy( m_StaticPropDict[i].m_pStudioHdr, (studiohdr_t*)buf.Base(), buf.Size() );

	if ( !LoadVTXFile( pModelName, m_StaticPropDict[i].m_pStudioHdr, m_StaticPropDict[i].m_VtxBuf ) )
	{
		// failed, leave state identified as disabled
		m_StaticPropDict[i].m_VtxBuf.Purge();
	}
}


//-----------------------------------------------------------------------------
// Unserialize static prop model dictionary
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::UnserializeModelDict( CUtlBuffer& buf )
{
	int count = buf.GetInt();
	while ( --count >= 0 )
	{
		StaticPropDictLump_t lump;
		buf.Get( &lump, sizeof(StaticPropDictLump_t) );
		
		CreateCollisionModel( lump.m_Name );
	}
}

void CVradStaticPropMgr::UnserializeModels( CUtlBuffer& buf )
{
	int count = buf.GetInt();

	m_StaticProps.AddMultipleToTail(count);
	for ( int i = 0; i < count; ++i )				  
	{
		StaticPropLump_t lump;
		buf.Get( &lump, sizeof(StaticPropLump_t) );
		
		VectorCopy( lump.m_Origin, m_StaticProps[i].m_Origin );
		VectorCopy( lump.m_Angles, m_StaticProps[i].m_Angles );
		VectorCopy( lump.m_LightingOrigin, m_StaticProps[i].m_LightingOrigin );
		m_StaticProps[i].m_bLightingOriginValid = ( lump.m_Flags & STATIC_PROP_USE_LIGHTING_ORIGIN ) > 0;
		m_StaticProps[i].m_ModelIdx = lump.m_PropType;
		m_StaticProps[i].m_Handle = TREEDATA_INVALID_HANDLE;
		m_StaticProps[i].m_Flags = lump.m_Flags;

		// Add the prop to the tree for collision, but only if it isn't
		// marked as not casting a shadow
		if ((lump.m_Flags & STATIC_PROP_NO_SHADOW) == 0)
			InsertPropIntoTree( i );
	}
}

//-----------------------------------------------------------------------------
// Unserialize static props
//-----------------------------------------------------------------------------

void CVradStaticPropMgr::UnserializeStaticProps()
{
	// Unserialize static props, insert them into the appropriate leaves
	GameLumpHandle_t handle = GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	int size = GameLumpSize( handle );
	if (!size)
		return;

	if ( GetGameLumpVersion( handle ) != GAMELUMP_STATIC_PROPS_VERSION )
	{
		Error( "Cannot load the static props... encountered a stale map version. Re-vbsp the map." );
	}

	if ( GetGameLump( handle ) )
	{
		CUtlBuffer buf( GetGameLump(handle), size, CUtlBuffer::READ_ONLY );
		UnserializeModelDict( buf );

		// Skip the leaf list data
		int count = buf.GetInt();
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, count * sizeof(StaticPropLeafLump_t) );

		UnserializeModels( buf );
	}
}

//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------

void CVradStaticPropMgr::Init()
{
	CreateInterfaceFn physicsFactory = GetPhysicsFactory();
	if ( !physicsFactory )
		Error( "Unable to load vphysics DLL." );
		
	s_pPhysCollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
	if( !s_pPhysCollision )
	{
		Error( "Unable to get '%s' for physics interface.", VPHYSICS_COLLISION_INTERFACE_VERSION );
		return;
	}

	m_pBSPTreeData->Init( ToolBSPTree() );

	// Read in static props that have been compiled into the bsp file
	UnserializeStaticProps();
}

void CVradStaticPropMgr::Shutdown()
{
	// Remove all static props from the tree
	for (int i = m_StaticProps.Size(); --i >= 0; )
	{
		RemovePropFromTree( i );
	}

	// Remove all static prop model data
	for (int i = m_StaticPropDict.Size(); --i >= 0; )
	{
		studiohdr_t *pStudioHdr = m_StaticPropDict[i].m_pStudioHdr;
		if ( pStudioHdr )
		{
			if ( pStudioHdr->pVertexBase )
			{
				free( pStudioHdr->pVertexBase );
			}
			free( pStudioHdr );
		}
	}

	m_pBSPTreeData->Shutdown();

	m_StaticProps.Purge();
	m_StaticPropDict.Purge();
}

//-----------------------------------------------------------------------------
// Do the collision test
//-----------------------------------------------------------------------------

// IBSPTreeDataEnumerator
bool FASTCALL CVradStaticPropMgr::EnumerateElement( int userId, int context )
{
	CStaticProp& prop = m_StaticProps[userId];

	EnumContext_t* pCtx = (EnumContext_t*)context;

	// Don't test twice
	if (pCtx->m_pPropTested->m_pTested[ userId ] == pCtx->m_pPropTested->m_Enum )
		return true;

	pCtx->m_pPropTested->m_pTested[ userId ] = pCtx->m_pPropTested->m_Enum;

	StaticPropDict_t& dict = m_StaticPropDict[prop.m_ModelIdx];
	
	if ( !IsBoxIntersectingRay( prop.m_mins, prop.m_maxs, pCtx->m_pRay->m_Start, pCtx->m_pRay->m_Delta ) )
		return true;

	// If there is an invalid model file, it has a null entry here.
	if( !dict.m_pModel )
		return false;

	CGameTrace trace;
	pCtx->m_pPropTested->pThreadedCollision->TraceBox( *pCtx->m_pRay, dict.m_pModel, prop.m_Origin, prop.m_Angles, &trace );

	// False means stop iterating. Return false if we hit!
	return (trace.fraction == 1.0);
}


// ISpatialLeafEnumerator
bool CVradStaticPropMgr::EnumerateLeaf( int leaf, int context )
{
	return m_pBSPTreeData->EnumerateElementsInLeaf( leaf, this, context );
}

bool CVradStaticPropMgr::ClipRayToStaticProps( PropTested_t& propTested, Ray_t const& ray )
{
	if ( m_bIgnoreStaticPropTrace )
	{
		// as if the trace passes right through
		return false;
	}

	StartRayTest( propTested );

	EnumContext_t ctx;
	ctx.m_pRay = &ray;
	ctx.m_pPropTested = &propTested;

	// If it got through without a hit, it returns true
	return !m_pBSPTreeData->EnumerateLeavesAlongRay( ray, this, (int)&ctx );
}

bool CVradStaticPropMgr::ClipRayToStaticPropsInLeaf( PropTested_t& propTested, Ray_t const& ray, int leaf )
{
	if ( m_bIgnoreStaticPropTrace )
	{
		// as if the trace passes right through
		return false;
	}

	EnumContext_t ctx;
	ctx.m_pRay = &ray;
	ctx.m_pPropTested = &propTested;

	return !m_pBSPTreeData->EnumerateElementsInLeaf( leaf, this, (int)&ctx );
}

void CVradStaticPropMgr::StartRayTest( PropTested_t& propTested )
{
	if (m_StaticProps.Size() > 0)
	{
		if (propTested.m_pTested == 0)
		{
			propTested.m_pTested = new int[m_StaticProps.Size()];
			memset( propTested.m_pTested, 0, m_StaticProps.Size() * sizeof(int) );
			propTested.m_Enum = 0;
			propTested.pThreadedCollision = s_pPhysCollision->ThreadContextCreate();
		}
		++propTested.m_Enum;
	}
}

void ComputeLightmapColor( dface_t* pFace, Vector &color )
{
	texinfo_t* pTex = &texinfo[pFace->texinfo];
	if ( pTex->flags & SURF_SKY )
	{
		// sky ambient already accounted for in direct component
		return;
	}
}

bool PositionInSolid( Vector &position )
{
	int ndxLeaf = PointLeafnum( position );
	if ( dleafs[ndxLeaf].contents & CONTENTS_SOLID )
	{
		// position embedded in solid
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Trace from a vertex to each direct light source, accumulating its contribution.
//-----------------------------------------------------------------------------
void ComputeDirectLightingAtPoint( Vector &position, Vector &normal, Vector &outColor, int iThread,
								   int static_prop_id_to_skip=-1)
{
	sampleLightOutput_t	sampleOutput;

	outColor.Init();

	// Iterate over all direct lights and accumulate their contribution
	int cluster = ClusterFromPoint( position );
	for ( directlight_t *dl = activelights; dl != NULL; dl = dl->next )
	{
		if ( dl->light.style )
		{
			// skip lights with style
			continue;
		}

		// is this lights cluster visible?
		if ( !PVSCheck( dl->pvs, cluster ) )
			continue;

		// push the vertex towards the light to avoid surface acne
		Vector adjusted_pos = position;
		Vector fudge=dl->light.origin-position;
		VectorNormalize( fudge );
		fudge *= 1.0;
		adjusted_pos += fudge;

		if ( !GatherSampleLight(
				 sampleOutput, dl, -1, adjusted_pos, &normal, 1, iThread, true,
				 static_prop_id_to_skip ) )
			continue;
		
		VectorMA( outColor, sampleOutput.falloff * sampleOutput.dot[0], dl->light.intensity, outColor );
	}
}

// identifies a vertex embedded in solid
// lighting will be copied from nearest valid neighbor
struct badVertex_t
{
	int		m_ColorVertex;
	Vector	m_Position;
	Vector	m_Normal;
};

// a final colored vertex
struct colorVertex_t
{
	Vector	m_Color;
	Vector	m_Position;
	bool	m_bValid;
};

//-----------------------------------------------------------------------------
// Trace rays from each unique vertex, accumulating direct and indirect
// sources at each ray termination. Use the winding data to distribute the unique vertexes
// into the rendering layout.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::ComputeLighting( CStaticProp &prop, int iThread, int prop_index )
{
	Vector						samplePosition;
	Vector						sampleNormal;
	CUtlVector<colorVertex_t>	colorVerts; 
	CUtlVector<badVertex_t>		badVerts;

	StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];
	studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
	OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
	if ( !pStudioHdr || !pVtxHdr )
	{
		// must have model and its verts for lighting computation
		// game will fallback to fullbright
		return;
	}

	// for access to this model's vertexes
	SetCurrentModel( pStudioHdr );

	for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
	{
		OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
		mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

		for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
		{
			OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel( modelID );
			mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );

			// light all unique vertexes
			colorVerts.EnsureCount( pStudioModel->numvertices );
			memset( colorVerts.Base(), 0, colorVerts.Count() * sizeof(colorVertex_t) );

			int numVertexes = 0;
			for ( int meshID = 0; meshID < pStudioModel->nummeshes; ++meshID )
			{
				mstudiomesh_t *pStudioMesh = pStudioModel->pMesh( meshID );
				const mstudio_meshvertexdata_t *vertData = pStudioMesh->GetVertexData();
				for ( int vertexID = 0; vertexID < pStudioMesh->numvertices; ++vertexID )
				{
					// transform position and normal into world coordinate system
					matrix3x4_t	matrix;
					AngleMatrix( prop.m_Angles, prop.m_Origin, matrix );
					VectorTransform( *vertData->Position( vertexID ), matrix, samplePosition );
					AngleMatrix( prop.m_Angles, matrix );
					VectorTransform( *vertData->Normal( vertexID ), matrix, sampleNormal );

					if ( (! (prop.m_Flags & STATIC_PROP_NO_PER_VERTEX_LIGHTING ) ) &&
						 PositionInSolid( samplePosition ) )
					{
						// vertex is in solid, add to the bad list, and recover later
						badVertex_t badVertex;
						badVertex.m_ColorVertex = numVertexes;
						badVertex.m_Position = samplePosition;
						badVertex.m_Normal = sampleNormal;
						badVerts.AddToTail( badVertex );			
					}
					else
					{
						Vector direct_pos=samplePosition;
						int skip_prop=-1;

						Vector directColor(0,0,0);
						if (prop.m_Flags & STATIC_PROP_NO_PER_VERTEX_LIGHTING )
						{
							if (prop.m_bLightingOriginValid)
								VectorCopy( prop.m_LightingOrigin, direct_pos );
							else
								VectorCopy( prop.m_Origin, direct_pos );
							skip_prop = prop_index;
						}
						if ( prop.m_Flags & STATIC_PROP_NO_SELF_SHADOWING )
							skip_prop = prop_index;

						
						ComputeDirectLightingAtPoint( direct_pos,
													  sampleNormal, directColor, iThread,
													  skip_prop );
						Vector indirectColor(0,0,0);

						if (g_bShowStaticPropNormals)
						{
							directColor= sampleNormal;
							directColor += Vector(1.0,1.0,1.0);
							directColor *= 50.0;
						}
						else
							if (numbounce >= 1)
								ComputeIndirectLightingAtPoint( samplePosition, sampleNormal, 
																indirectColor, iThread, true );

						colorVerts[numVertexes].m_bValid = true;
						colorVerts[numVertexes].m_Position = samplePosition;
						VectorAdd( directColor, indirectColor, colorVerts[numVertexes].m_Color );
					}

					numVertexes++;
				}
			}

			// color in the bad vertexes
			// when entire model has no lighting origin and no valid neighbors
			// must punt, leave black coloring
			if ( badVerts.Count() && ( prop.m_bLightingOriginValid || badVerts.Count() != numVertexes ) )
			{
				for ( int nBadVertex = 0; nBadVertex < badVerts.Count(); nBadVertex++ )
				{		
					Vector bestPosition;
					if ( prop.m_bLightingOriginValid )
					{
						// use the specified lighting origin
						VectorCopy( prop.m_LightingOrigin, bestPosition );
					}
					else
					{
						// find the closest valid neighbor
						int best = 0;
						float closest = FLT_MAX;
						for ( int nColorVertex = 0; nColorVertex < numVertexes; nColorVertex++ )
						{
							if ( !colorVerts[nColorVertex].m_bValid )
							{
								// skip invalid neighbors
								continue;
							}
							Vector delta;
							VectorSubtract( colorVerts[nColorVertex].m_Position, badVerts[nBadVertex].m_Position, delta );
							float distance = VectorLength( delta );
							if ( distance < closest )
							{
								closest = distance;
								best    = nColorVertex;
							}
						}

						// use the best neighbor as the direction to crawl
						VectorCopy( colorVerts[best].m_Position, bestPosition );
					}

					// crawl toward best position
					// sudivide to determine a closer valid point to the bad vertex, and re-light
					Vector midPosition;
					int numIterations = 20;
					while ( --numIterations > 0 )
					{
						VectorAdd( bestPosition, badVerts[nBadVertex].m_Position, midPosition );
						VectorScale( midPosition, 0.5f, midPosition );
						if ( PositionInSolid( midPosition ) )
							break;
						bestPosition = midPosition;
					}

					// re-light from better position
					Vector directColor;
					ComputeDirectLightingAtPoint( bestPosition, badVerts[nBadVertex].m_Normal, directColor, iThread );

					Vector indirectColor;
					ComputeIndirectLightingAtPoint( bestPosition, badVerts[nBadVertex].m_Normal,
													indirectColor, iThread, true );

					// save results, not changing valid status
					// to ensure this offset position is not considered as a viable candidate
					colorVerts[badVerts[nBadVertex].m_ColorVertex].m_Position = bestPosition;
					VectorAdd( directColor, indirectColor, colorVerts[badVerts[nBadVertex].m_ColorVertex].m_Color );
				}
			}
			
			// discard bad verts
			badVerts.Purge();

			// distribute the lighting results
			for ( int nLod = 0; nLod < pVtxHdr->numLODs; nLod++ )
			{
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLod );

				for ( int nMesh = 0; nMesh < pStudioModel->nummeshes; ++nMesh )
				{
					mstudiomesh_t* pMesh = pStudioModel->pMesh( nMesh );
					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh( nMesh );

					for ( int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup )
					{
						OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup( nGroup );
						int nMeshIdx = prop.m_MeshData.AddToTail();
						prop.m_MeshData[nMeshIdx].m_Verts.AddMultipleToTail( pStripGroup->numVerts );
						prop.m_MeshData[nMeshIdx].m_nLod = nLod;

						for ( int nVertex = 0; nVertex < pStripGroup->numVerts; ++nVertex )
						{
							int nIndex = pMesh->vertexoffset + pStripGroup->pVertex( nVertex )->origMeshVertID;

							Assert( nIndex < pStudioModel->numvertices );
							prop.m_MeshData[nMeshIdx].m_Verts[nVertex] = colorVerts[nIndex].m_Color;
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Write the lighitng to bsp pak lump
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::SerializeLighting()
{
	char		filename[MAX_PATH];
	CUtlBuffer	utlBuf;

	// illuminate them all
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	char mapName[MAX_PATH];
	Q_FileBase( source, mapName, sizeof( mapName ) );

	int size;
	for (int i = 0; i < count; ++i)
	{
		sprintf( filename, "sp_%d.vhv", i );

		int totalVertexes = 0;
		for ( int j=0; j<m_StaticProps[i].m_MeshData.Count(); j++ )
		{
			totalVertexes += m_StaticProps[i].m_MeshData[j].m_Verts.Count();
		}

		// allocate a buffer with enough padding for alignment
		size = sizeof( HardwareVerts::FileHeader_t ) + 
				m_StaticProps[i].m_MeshData.Count()*sizeof(HardwareVerts::MeshHeader_t) +
				totalVertexes*4 + 2*512;
		utlBuf.EnsureCapacity( size );
		Q_memset( utlBuf.Base(), 0, size );

		HardwareVerts::FileHeader_t *pVhvHdr = (HardwareVerts::FileHeader_t *)utlBuf.Base();

		// align to start of vertex data
		unsigned char *pVertexData = (unsigned char *)(sizeof( HardwareVerts::FileHeader_t ) + m_StaticProps[i].m_MeshData.Count()*sizeof(HardwareVerts::MeshHeader_t));
		pVertexData = (unsigned char*)pVhvHdr + ALIGN_TO_POW2( (unsigned int)pVertexData, 512 );

		// construct header
		pVhvHdr->m_nVersion     = VHV_VERSION;
		pVhvHdr->m_nChecksum    = m_StaticPropDict[m_StaticProps[i].m_ModelIdx].m_pStudioHdr->checksum;
		pVhvHdr->m_nVertexFlags = VERTEX_COLOR;
		pVhvHdr->m_nVertexSize  = 4;
		pVhvHdr->m_nVertexes    = totalVertexes;
		pVhvHdr->m_nMeshes      = m_StaticProps[i].m_MeshData.Count();

		for (int n=0; n<pVhvHdr->m_nMeshes; n++)
		{
			// construct mesh dictionary
			HardwareVerts::MeshHeader_t *pMesh = pVhvHdr->pMesh( n );
			pMesh->m_nLod      = m_StaticProps[i].m_MeshData[n].m_nLod;
			pMesh->m_nVertexes = m_StaticProps[i].m_MeshData[n].m_Verts.Count();
			pMesh->m_nOffset   = (unsigned int)pVertexData - (unsigned int)pVhvHdr; 

			// construct vertexes
			for (int k=0; k<pMesh->m_nVertexes; k++)
			{
				Vector &vector = m_StaticProps[i].m_MeshData[n].m_Verts[k];

				ColorRGBExp32 rgbColor;
				VectorToColorRGBExp32( vector, rgbColor );
				unsigned char dstColor[4];
				ConvertRGBExp32ToRGBA8888( &rgbColor, dstColor );

				// b,g,r,a order
				pVertexData[0] = dstColor[2];
				pVertexData[1] = dstColor[1];
				pVertexData[2] = dstColor[0];
				pVertexData[3] = dstColor[3];
				pVertexData += 4;
			}
		}

		// align to end of file
		pVertexData = (unsigned char *)((unsigned int)pVertexData - (unsigned int)pVhvHdr);
		pVertexData = (unsigned char*)pVhvHdr + ALIGN_TO_POW2( (unsigned int)pVertexData, 512 );

		AddBufferToPack( filename, (void*)pVhvHdr, pVertexData - (unsigned char*)pVhvHdr, false );
	}
}

//-----------------------------------------------------------------------------
// Computes lighting for the static props.
// Must be after all other surface lighting has been computed for the indirect sampling.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::ComputeLighting( int iThread )
{
	// illuminate them all
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	StartPacifier( "Computing static prop lighting : " );

	// ensure any traces against us are ignored because we have no inherit lighting contribution
	m_bIgnoreStaticPropTrace = true;

	for (int i = 0; i < count; ++i)
	{
		UpdatePacifier( (float)i / (float)count );
		ComputeLighting( m_StaticProps[i], iThread, i );
	}

	// restore default
	m_bIgnoreStaticPropTrace = false;

	// save data to bsp
	SerializeLighting();

	EndPacifier( true );
}

//-----------------------------------------------------------------------------
// Adds all static prop polys to the ray trace store.
//-----------------------------------------------------------------------------
void CVradStaticPropMgr::AddPolysForRayTrace( void )
{
	int count = m_StaticProps.Count();
	if ( !count )
	{
		// nothing to do
		return;
	}

	for ( int nProp = 0; nProp < count; ++nProp )
	{
		CStaticProp &prop = m_StaticProps[nProp];
		if ( prop.m_Flags & STATIC_PROP_NO_SHADOW )
			continue;

		StaticPropDict_t &dict = m_StaticPropDict[prop.m_ModelIdx];
		studiohdr_t	*pStudioHdr = dict.m_pStudioHdr;
		OptimizedModel::FileHeader_t *pVtxHdr = (OptimizedModel::FileHeader_t *)dict.m_VtxBuf.Base();
		if ( !pStudioHdr || !pVtxHdr )
		{
			// must have model and its verts for decoding triangles
			return;
		}

		// for access to this model's vertexes
		SetCurrentModel( pStudioHdr );
	
		// meshes are deeply hierarchial, divided between three stores, follow the white rabbit
		// body parts -> models -> lod meshes -> strip groups -> strips
		// the vertices and indices are pooled, the trick is knowing the offset to determine your indexed base 
		for ( int bodyID = 0; bodyID < pStudioHdr->numbodyparts; ++bodyID )
		{
			OptimizedModel::BodyPartHeader_t* pVtxBodyPart = pVtxHdr->pBodyPart( bodyID );
			mstudiobodyparts_t *pBodyPart = pStudioHdr->pBodypart( bodyID );

			for ( int modelID = 0; modelID < pBodyPart->nummodels; ++modelID )
			{
				OptimizedModel::ModelHeader_t* pVtxModel = pVtxBodyPart->pModel( modelID );
				mstudiomodel_t *pStudioModel = pBodyPart->pModel( modelID );

				// assuming lod 0, could iterate if required
				int nLod = 0;
				OptimizedModel::ModelLODHeader_t *pVtxLOD = pVtxModel->pLOD( nLod );

				for ( int nMesh = 0; nMesh < pStudioModel->nummeshes; ++nMesh )
				{
					mstudiomesh_t* pMesh = pStudioModel->pMesh( nMesh );
					OptimizedModel::MeshHeader_t* pVtxMesh = pVtxLOD->pMesh( nMesh );
					const mstudio_meshvertexdata_t *vertData = pMesh->GetVertexData();

					for ( int nGroup = 0; nGroup < pVtxMesh->numStripGroups; ++nGroup )
					{
						OptimizedModel::StripGroupHeader_t* pStripGroup = pVtxMesh->pStripGroup( nGroup );

						int nStrip;
						for ( nStrip = 0; nStrip < pStripGroup->numStrips; nStrip++ )
						{
							OptimizedModel::StripHeader_t *pStrip = pStripGroup->pStrip( nStrip );

							if ( pStrip->flags & OptimizedModel::STRIP_IS_TRILIST )
							{
								for ( int i = 0; i < pStrip->numIndices; i += 3 )
								{
									int idx = pStrip->indexOffset + i;

									unsigned short i1 = *pStripGroup->pIndex( idx );
									unsigned short i2 = *pStripGroup->pIndex( idx + 1 );
									unsigned short i3 = *pStripGroup->pIndex( idx + 2 );

									int vertex1 = pStripGroup->pVertex( i1 )->origMeshVertID;
									int vertex2 = pStripGroup->pVertex( i2 )->origMeshVertID;
									int vertex3 = pStripGroup->pVertex( i3 )->origMeshVertID;

									// transform position into world coordinate system
									matrix3x4_t	matrix;
									AngleMatrix( prop.m_Angles, prop.m_Origin, matrix );

									Vector position1;
									Vector position2;
									Vector position3;
									VectorTransform( *vertData->Position( vertex1 ), matrix, position1 );
									VectorTransform( *vertData->Position( vertex2 ), matrix, position2 );
									VectorTransform( *vertData->Position( vertex3 ), matrix, position3 );
// 		printf( "\ngl 3\n" );
// 		printf( "gl %6.3f %6.3f %6.3f 1 0 0\n", XYZ(position1));
// 		printf( "gl %6.3f %6.3f %6.3f 0 1 0\n", XYZ(position2));
// 		printf( "gl %6.3f %6.3f %6.3f 0 0 1\n", XYZ(position3));
									g_RtEnv.AddTriangle( nProp,
														 position1, position2, position3,
														 Vector(0,0,0));
								}
							}
							else
							{
								// all tris expected to be discrete tri lists
								// must fixme if stripping ever occurs
								printf("unexpected strips found\n");
								Assert( 0 );
								return;
							}
						}
					}
				}
			}
		}
	}
}

static studiohdr_t *g_pActiveStudioHdr;
static void SetCurrentModel( studiohdr_t *pStudioHdr )
{
	// track the correct model
	g_pActiveStudioHdr = pStudioHdr;
}

const mstudio_modelvertexdata_t *mstudiomodel_t::GetVertexData()
{
	char				fileName[MAX_PATH];
	FileHandle_t		fileHandle;
	vertexFileHeader_t	*pVvdHdr;

	Assert( g_pActiveStudioHdr );

	if ( g_pActiveStudioHdr->pVertexBase )
	{
		vertexdata.pVertexData  = (byte *)g_pActiveStudioHdr->pVertexBase + ((vertexFileHeader_t *)g_pActiveStudioHdr->pVertexBase)->vertexDataStart;
		vertexdata.pTangentData = (byte *)g_pActiveStudioHdr->pVertexBase + ((vertexFileHeader_t *)g_pActiveStudioHdr->pVertexBase)->tangentDataStart;
		return &vertexdata;
	}

	// mandatory callback to make requested data resident
	// load and persist the vertex file
	strcpy( fileName, "models/" );	
	strcat( fileName, g_pActiveStudioHdr->pszName() );
	Q_StripExtension( fileName, fileName, sizeof( fileName ) );
	strcat( fileName, ".vvd" );

	// load the model
	fileHandle = g_pFileSystem->Open( fileName, "rb" );
	if ( !fileHandle )
	{
		Error( "Unable to load vertex data \"%s\"\n", fileName );
	}

	// Get the file size
	int size = g_pFileSystem->Size( fileHandle );
	if (size == 0)
	{
		g_pFileSystem->Close( fileHandle );
		Error( "Bad size for vertex data \"%s\"\n", fileName );
	}

	pVvdHdr = (vertexFileHeader_t *)malloc(size);
	g_pFileSystem->Read( pVvdHdr, size, fileHandle );
	g_pFileSystem->Close( fileHandle );

	// check header
	if (pVvdHdr->id != MODEL_VERTEX_FILE_ID)
	{
		Error("Error Vertex File %s id %d should be %d\n", fileName, pVvdHdr->id, MODEL_VERTEX_FILE_ID);
	}
	if (pVvdHdr->version != MODEL_VERTEX_FILE_VERSION)
	{
		Error("Error Vertex File %s version %d should be %d\n", fileName, pVvdHdr->version, MODEL_VERTEX_FILE_VERSION);
	}
	if (pVvdHdr->checksum != g_pActiveStudioHdr->checksum)
	{
		Error("Error Vertex File %s checksum %d should be %d\n", fileName, pVvdHdr->checksum, g_pActiveStudioHdr->checksum);
	}

	g_pActiveStudioHdr->pVertexBase = (void*)pVvdHdr; 

	vertexdata.pVertexData  = (byte *)pVvdHdr + pVvdHdr->vertexDataStart;
	vertexdata.pTangentData = (byte *)pVvdHdr + pVvdHdr->tangentDataStart;

	return &vertexdata;
}
