//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BSPLIB_H
#define BSPLIB_H

#ifdef _WIN32
#pragma once
#endif


#include "bspfile.h"
#include "utlvector.h"


class ISpatialQuery;
struct Ray_t;
class Vector2D;
struct portal_t;
class CUtlBuffer;

// this is only true in vrad
extern bool g_bHDR;

// default width/height of luxels in world units.
#define DEFAULT_LUXEL_SIZE ( 16.0f )

#define	SINGLE_BRUSH_MAP	(MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER*MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER)
#define	SINGLEMAP	(MAX_LIGHTMAP_DIM_INCLUDING_BORDER*MAX_LIGHTMAP_DIM_INCLUDING_BORDER)

struct entity_t
{
	Vector		origin;
	int			firstbrush;
	int			numbrushes;
	epair_t		*epairs;

	// only valid for func_areaportals
	int			areaportalnum;
	int			portalareas[2];
	portal_t	*m_pPortalsLeadingIntoAreas[2];	// portals leading into portalareas
};


extern	int			    nummodels;
extern	dmodel_t	    dmodels[MAX_MAP_MODELS];

extern	int			    visdatasize;
extern	byte		    dvisdata[MAX_MAP_VISIBILITY];
extern	dvis_t		    *dvis;

extern	CUtlVector<byte> dlightdataHDR;
extern	CUtlVector<byte> dlightdataLDR;
extern	CUtlVector<byte> *pdlightdata;
extern	CUtlVector<char> dentdata;

extern	int			    numleafs;
extern	dleaf_t			dleafs[MAX_MAP_LEAFS];
extern	CUtlVector<CompressedLightCube> *g_pLeafAmbientLighting;
extern	unsigned short  g_LeafMinDistToWater[MAX_MAP_LEAFS];

extern	int			    numplanes;
extern	dplane_t	    dplanes[MAX_MAP_PLANES];

extern	int			    numvertexes;
extern	dvertex_t	    dvertexes[MAX_MAP_VERTS];

extern	int				g_numvertnormalindices;	// dfaces reference these. These index g_vertnormals.
extern	unsigned short	g_vertnormalindices[MAX_MAP_VERTNORMALS];

extern	int				g_numvertnormals;	
extern	Vector			g_vertnormals[MAX_MAP_VERTNORMALS];

extern	int			    numnodes;
extern	dnode_t		    dnodes[MAX_MAP_NODES];

extern  CUtlVector<texinfo_t> texinfo;

extern	int			    numtexdata;
extern	dtexdata_t	    dtexdata[MAX_MAP_TEXDATA];

// displacement map .bsp file info
extern  CUtlVector<ddispinfo_t> g_dispinfo;
extern  CUtlVector<CDispVert> g_DispVerts;
extern  CUtlVector<CDispTri> g_DispTris;
extern  CUtlVector<unsigned char> g_DispLightmapSamplePositions; // LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS

extern  int             numorigfaces;
extern  dface_t         dorigfaces[MAX_MAP_FACES];

extern	int				g_numprimitives;
extern	dprimitive_t	g_primitives[MAX_MAP_PRIMITIVES];

extern	int				g_numprimverts;
extern	dprimvert_t		g_primverts[MAX_MAP_PRIMVERTS];

extern	int				g_numprimindices;
extern	unsigned short	g_primindices[MAX_MAP_PRIMINDICES];

extern	int			    numfaces;
extern	dface_t		    dfaces[MAX_MAP_FACES];

extern	int			    numfaces_hdr;
extern	dface_t		    dfaces_hdr[MAX_MAP_FACES];

extern	int			    numedges;
extern	dedge_t		    dedges[MAX_MAP_EDGES];

extern	int			    numleaffaces;
extern	unsigned short	dleaffaces[MAX_MAP_LEAFFACES];

extern	int			    numleafbrushes;
extern	unsigned short	dleafbrushes[MAX_MAP_LEAFBRUSHES];

extern	int			    numsurfedges;
extern	int			    dsurfedges[MAX_MAP_SURFEDGES];

extern	int			    numareas;
extern	darea_t		    dareas[MAX_MAP_AREAS];

extern	int			    numareaportals;
extern	dareaportal_t	dareaportals[MAX_MAP_AREAPORTALS];

extern	int			    numbrushes;
extern	dbrush_t	    dbrushes[MAX_MAP_BRUSHES];

extern	int			    numbrushsides;
extern	dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

extern  int			    *pNumworldlights;
extern  dworldlight_t   *dworldlights;

extern Vector			g_ClipPortalVerts[MAX_MAP_PORTALVERTS];
extern int				g_nClipPortalVerts;

extern dcubemapsample_t	g_CubemapSamples[MAX_MAP_CUBEMAPSAMPLES];
extern int				g_nCubemapSamples;

extern int				g_nOverlayCount;
extern doverlay_t		g_Overlays[MAX_MAP_OVERLAYS];

extern int				g_nWaterOverlayCount;
extern dwateroverlay_t	g_WaterOverlays[MAX_MAP_WATEROVERLAYS];

extern CUtlVector<char>	g_TexDataStringData;
extern CUtlVector<int>	g_TexDataStringTable;

// JAY: portals in the BSP file
// These are the portal data structures (points to verts, leafs, planes)
extern	int					numportals;
extern	dportal_t			dportals[MAX_MAP_PORTALS];

// These are the per-cluster data (currently only the portal list)
extern	int					numclusters;
extern	dcluster_t			dclusters[MAX_MAP_CLUSTERS];

extern	int					numleafwaterdata;
extern	dleafwaterdata_t	dleafwaterdata[MAX_MAP_LEAFWATERDATA]; 

// This is a list of polygons.  Each set of portal verts define a portal polygon
// These numbers are indices into the map's vertex list
extern	int					numportalverts;
extern	unsigned short		dportalverts[MAX_MAP_PORTALVERTS];

// This is a list of portal indices.  Each portal is referenced twice (once by each cluster it separates)
extern	int					numclusterportals;
extern	unsigned short		dclusterportals[MAX_MAP_PORTALS*2];

extern CUtlVector<CFaceMacroTextureInfo>	g_FaceMacroTextureInfos;

extern CUtlVector<doccluderdata_t>	g_OccluderData;
extern CUtlVector<doccluderpolydata_t>	g_OccluderPolyData;
extern CUtlVector<int>	g_OccluderVertexIndices;

extern CUtlVector<dlightmappage_t>		g_dLightmapPages;
extern CUtlVector<dlightmappageinfo_t>	g_dLightmapPageInfos;

// level flags
extern uint32 g_LevelFlags;								// see LVLFLAGS_xxx in bspfile.h

// ---------------------------------------------------------------   portals

// physics collision data
extern	byte		*g_pPhysCollide;
extern	int			g_PhysCollideSize;

// Embedded pack/pak file
void				ClearPackFile( void );
void				AddFileToPack( const char *pRelativeName, const char *fullpath );
void				AddBufferToPack( const char *pRelativeName, void *data, int length, bool bTextMode );
bool				FileExistsInPack( const char *pRelativeName );
bool				ReadFileFromPack( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf );
void				RemoveFileFromPack( const char *pRelativeName );
int					GetNextFilename( int id, char *pBuffer, int bufferSize, int &fileSize );
void				ForceAlignment( bool bAlign, unsigned int sectorSize );

//-----------------------------------------------------------------------------
// Handle to a game lump
//-----------------------------------------------------------------------------
typedef unsigned short GameLumpHandle_t;


//-----------------------------------------------------------------------------
// Convert four-CC code to a handle	+ back
//-----------------------------------------------------------------------------
GameLumpHandle_t	GetGameLumpHandle( GameLumpId_t id );
GameLumpId_t		GetGameLumpId( GameLumpHandle_t handle );
int					GetGameLumpFlags( GameLumpHandle_t handle );
int					GetGameLumpVersion( GameLumpHandle_t handle );


//-----------------------------------------------------------------------------
// Game lump accessor methods 
//-----------------------------------------------------------------------------
void*	GetGameLump( GameLumpHandle_t handle );
int		GameLumpSize( GameLumpHandle_t handle );


//-----------------------------------------------------------------------------
// Game lump iteration methods 
//-----------------------------------------------------------------------------
GameLumpHandle_t	FirstGameLump();
GameLumpHandle_t	NextGameLump( GameLumpHandle_t handle );
GameLumpHandle_t	InvalidGameLump();


//-----------------------------------------------------------------------------
// Game lump creation/destruction method
//-----------------------------------------------------------------------------
GameLumpHandle_t	CreateGameLump( GameLumpId_t id, int size, int flags, int version );
void				DestroyGameLump( GameLumpHandle_t handle );
void				DestroyAllGameLumps();


//-----------------------------------------------------------------------------
// Helper for the bspzip tool
//-----------------------------------------------------------------------------
void ExtractZipFileFromBSP( char *pBSPFileName, char *pZipFileName );


//-----------------------------------------------------------------------------
// String table methods
//-----------------------------------------------------------------------------
const char *		TexDataStringTable_GetString( int stringID );
int					TexDataStringTable_AddOrFindString( const char *pString );

void DecompressVis (byte *in, byte *decompressed);
int CompressVis (byte *vis, byte *dest);

void	OpenBSPFile (char *filename);
void	CloseBSPFile (void);
void	LoadBSPFile (char *filename);
void	LoadBSPFile_FileSystemOnly (char *filename);
void	LoadBSPFileTexinfo (char *filename);	// just for qdata
void	WriteBSPFile (char *filename, char* xzpLumpFilename = NULL);
void	PrintBSPFileSizes (void);
void	PrintBSPPackDirectory(void);

void	WriteLumpToFile( char *filename, int lump );

//===============

extern	int			num_entities;
extern	entity_t	entities[MAX_MAP_ENTITIES];

void	ParseEntities (void);
void	UnparseEntities (void);

void 	SetKeyValue (entity_t *ent, const char *key, const char *value);
char 	*ValueForKey (entity_t *ent, char *key);
// will return "" if not present

int		IntForKey (entity_t *ent, char *key);
vec_t	FloatForKey (entity_t *ent, char *key);
void 	GetVectorForKey (entity_t *ent, char *key, Vector& vec);
void 	GetVector2DForKey (entity_t *ent, char *key, Vector2D& vec);
void 	GetAnglesForKey (entity_t *ent, char *key, QAngle& vec);

epair_t *ParseEpair (void);

void PrintEntity (entity_t *ent);


// Build a list of the face's vertices (index into dvertexes).
// points must be able to hold pFace->numedges indices.
void BuildFaceCalcWindingData( dface_t *pFace, int *points );

// Convert a tristrip to a trilist.
// Removes degenerates.
// Fills in pTriListIndices and pnTriListIndices.
// You must free pTriListIndices with delete[].
void TriStripToTriList( 
	unsigned short const *pTriStripIndices,
	int nTriStripIndices,
	unsigned short **pTriListIndices,
	int *pnTriListIndices );

// Calculates the lightmap coordinates at a given set of positions given the 
// lightmap basis information.
void CalcTextureCoordsAtPoints(
	float const texelsPerWorldUnits[2][4],
	int const subtractOffset[2],
	Vector const *pPoints,
	int const nPoints,
	Vector2D *pCoords );

// Figure out lightmap extents on all (lit) faces.
void UpdateAllFaceLightmapExtents();


//-----------------------------------------------------------------------------
// Gets at an interface for the tree for enumeration of leaves in volumes.
//-----------------------------------------------------------------------------
ISpatialQuery* ToolBSPTree();

class IBSPNodeEnumerator
{
public:
	// call back with a node and a context
	virtual bool EnumerateNode( int node, Ray_t const& ray, float f, int context ) = 0;

	// call back with a leaf and a context
	virtual bool EnumerateLeaf( int leaf, Ray_t const& ray, float start, float end, int context ) = 0;
};

//-----------------------------------------------------------------------------
// Enumerates nodes + leafs in front to back order...
//-----------------------------------------------------------------------------
bool EnumerateNodesAlongRay( Ray_t const& ray, IBSPNodeEnumerator* pEnum, int context );


//-----------------------------------------------------------------------------
// Helps us find all leaves associated with a particular cluster
//-----------------------------------------------------------------------------
struct clusterlist_t
{
	int				leafCount;
	CUtlVector<int> leafs;
};

extern CUtlVector<clusterlist_t> g_ClusterLeaves;

// Call this to build the mapping from cluster to leaves
void BuildClusterTable( );

void GetPlatformMapPath( const char *pMapPath, char *pPlatformMapPath, int dxlevel, int maxLength );

void SetHDRMode( bool bHDR );

// ----------------------------------------------------------------------------- //
// Helper accessors for the various structures.
// ----------------------------------------------------------------------------- //

inline ColorRGBExp32* dface_AvgLightColor( dface_t *pFace, int nLightStyleIndex ) 
{ 
	return (ColorRGBExp32*)&(*pdlightdata)[pFace->lightofs - (nLightStyleIndex+1) * 4];
}

inline const char* TexInfo_TexName( int iTexInto )
{
	return TexDataStringTable_GetString( dtexdata[texinfo[iTexInto].texdata].nameStringTableID );
}


#endif // BSPLIB_H
