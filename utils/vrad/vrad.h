//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

//
// NOTE!!!
//
// VRAD has problems when linking to the release/multithreaded c libs, so 
// we link to debug/multithreadded c libs instead.
//
// Here's Charlie's notes on the matter:
//
// "I checked in a version of vrad built in release mode with Debug Multi-Threaded 
// standard libraries.  This will alleviate the red lighting bug until I (or someone) 
// has more time to look into this problem.  For those interested in \map\red 
// lighting bug is a map (redtest2.map) that consistently show the lighting problem 
// for future testing."
//
// NJS:
// I ran into this as well, and found it related to release / debug, and having nothing
// to do with multi-threading.  For example, you will get the same wrong results linking
// to Release Multi-threaded as Release Single-Threaded( with threads set to one of course)
// and the same correct results linking with the debug versions of the above.  
// Linking with Release vs. Debug makes a nearly 2x performance difference, though it's unclear
// as to whether this is because it's just that much faster, or the code generated on Release
// is skipping large operations erroniously.
//

#ifndef VRAD_H
#define VRAD_H
#pragma once


#include "commonmacros.h"
#include "worldsize.h"
#include "cmdlib.h"
#include "mathlib.h"
#include "bsplib.h"
#include "polylib.h"
#include "threads.h"
#include "builddisp.h"
#include "VRAD_DispColl.h"
#include "UtlMemory.h"
#include "UtlHash.h"
#include "UtlVector.h"
#include "iincremental.h"
#include "raytrace.h"


#ifdef _WIN32
#include <windows.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#pragma warning(disable: 4142 4028)
#include <io.h>
#pragma warning(default: 4142 4028)

#include <fcntl.h>
#include <direct.h>
#include <ctype.h>


// Can remove these options if they don't generate problems.
//#define SAMPLEHASH_USE_AREA_PATCHES	// Add patches to sample hash based on their AABB instead of as a single point.
#define SAMPLEHASH_QUERY_ONCE		// Big optimization - causes way less sample hash queries.


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

struct Ray_t;

#define		TRANSFER_EPSILON		0.0000001

struct directlight_t
{
	int		index;

	directlight_t *next;
	dworldlight_t light;

	byte	*pvs;		// accumulated domain of the light
	int		facenum;	// domain of attached lights
	int		texdata;	// texture source of traced lights

	Vector	snormal;
	Vector	tnormal;
	float	sscale;
	float	tscale;
	float	soffset;
	float	toffset;

	int		dorecalc; // position, vector, spot angle, etc.
	IncrementalLightID	m_IncrementalID;
};

struct bumplights_t
{
	Vector	light[NUM_BUMP_VECTS+1];
};


struct transfer_t
{
	int	patch;
	float	transfer;
};


#define	MAX_PATCHES	(4*65536)

struct patch_t
{
	winding_t	*winding;
	Vector		mins, maxs, face_mins, face_maxs;

	Vector		origin;				// adjusted off face by face normal

	dplane_t	*plane;				// plane (corrected for facing)
	
	unsigned short		m_IterationKey;	// Used to prevent touching the same patch multiple times in the same query.
										// See IncrementPatchIterationKey().
	
	// these are packed into one dword
	unsigned int normalMajorAxis : 2;	// the major axis of base face normal
	unsigned int sky : 1;
	unsigned int needsBumpmap : 1;
	unsigned int pad : 28;

	Vector		normal;				// adjusted for phong shading

	float		planeDist;			// Fixes up patch planes for brush models with an origin brush

	float		chop;				// smallest acceptable width of patch face
	float		luxscale;			// average luxels per world coord
	float		scale[2];			// Scaling of texture in s & t

	bumplights_t totallight;		// accumulated by radiosity
									// does NOT include light
									// accounted for by direct lighting
	Vector		baselight;			// emissivity only
	float		basearea;			// surface per area per baselight instance

	Vector		directlight;		// direct light value
	float		area;

	Vector		reflectivity;		// Average RGB of texture, modified by material type.

	Vector		samplelight;
	float		samplearea;		// for averaging direct light
	int			faceNumber;
	int			clusterNumber;

	int			parent;			// patch index of parent
	int			child1;			// patch index for children
	int			child2;

	int			ndxNext;					// next patch index in face
	int			ndxNextParent;				// next parent patch index in face
	int			ndxNextClusterChild;		// next terminal child index in cluster
//	struct		patch_s		*next;					// next in face
//	struct		patch_s		*nextparent;		    // next in face
//	struct		patch_s		*nextclusterchild;		// next terminal child in cluster

	int			numtransfers;
	transfer_t	*transfers;
};


extern CUtlVector<patch_t>	patches;
extern CUtlVector<int>		facePatches;		// constains all patches, children first
extern CUtlVector<int>		faceParents;		// contains only root patches, use next parent to iterate
extern CUtlVector<int>		clusterChildren;


struct sky_camera_t
{
	Vector origin;
	float world_to_sky;
	float sky_to_world;
	int area;
};

extern int num_sky_cameras;
extern sky_camera_t sky_cameras[MAX_MAP_AREAS];
extern int area_sky_cameras[MAX_MAP_AREAS];
void ProcessSkyCameras();

extern entity_t		*face_entity[MAX_MAP_FACES];
extern Vector		face_offset[MAX_MAP_FACES];		// for rotating bmodels
extern Vector		face_centroids[MAX_MAP_EDGES];
extern int			leafparents[MAX_MAP_LEAFS];
extern int			nodeparents[MAX_MAP_NODES];
extern float		lightscale;
extern float		dlight_threshold;
extern float		coring;
extern qboolean		dumppatches;
extern bool			bRed2Black;
extern bool			bDumpNormals;
extern float		maxchop;
extern bool			g_bXBox;
extern bool			g_bExportLightmaps;
extern FileHandle_t	pFileSamples[4];
extern qboolean		g_bLowPriority;
extern qboolean		do_fast;
extern bool			g_bInterrupt;		// Was used with background lighting in WC. Tells VRAD to stop lighting.
extern IIncremental *g_pIncremental;	// null if not doing incremental lighting
extern bool g_dofinal;										// highest quality
extern bool g_bLargeDispSampleRadius;
extern bool g_bStaticPropPolys;
extern bool g_bShowStaticPropNormals;

extern RayTracingEnvironment g_RtEnv;

#include "mpivrad.h"

void MakeShadowSplits (void);

//==============================================

void BuildVisMatrix (void);
void BuildClusterTable( void );
void AddDispsToClusterTable( void );
void FreeVisMatrix (void);
// qboolean CheckVisBit (unsigned int p1, unsigned int p2);
void TouchVMFFile (void);

//==============================================

extern  qboolean do_extra;
extern  qboolean do_fast;
extern  qboolean do_centersamples;
extern  int extrapasses;
extern	Vector ambient;
extern  float maxlight;
extern	unsigned numbounce;
extern  qboolean g_bLogHashData;
extern  bool	debug_extra;
extern	directlight_t	*activelights;
extern	directlight_t	*freelights;

// because of hdr having two face lumps (light styles can cause them to be different, among other
// things), we need to always access (r/w) face data though this pointer
extern dface_t *g_pFaces;


extern bool g_bMPIProps;

extern	byte	nodehit[MAX_MAP_NODES];
extern  float	gamma;
extern	float	indirect_sun;
extern	float	smoothing_threshold;
extern	int		dlight_map;

extern float	g_flMaxDispSampleSize;
extern float	g_SunAngularExtent;

extern char		source[MAX_PATH];

// Used by incremental lighting to trivial-reject faces.
// There is a bit in here for each face telling whether or not any of the
// active lights can see the face.
extern CUtlVector<byte> g_FacesVisibleToLights;

void MakeTnodes (dmodel_t *bm);
void PairEdges (void);

void SaveVertexNormals( void );

qboolean IsIncremental(char *filename);
int SaveIncremental(char *filename);
int PartialHead (void);
void BuildFacelights (int facenum, int threadnum);
void PrecompLightmapOffsets();
void FinalLightFace (int threadnum, int facenum);
void PvsForOrigin (Vector& org, byte *pvs);
void BuildPalettedLightmaps();
void CompressConstantLightmaps( int constantThreshold );
void ConvertRGBExp32ToRGBA8888( const ColorRGBExp32 *pSrc, unsigned char *pDst );

inline byte PVSCheck( const byte *pvs, int iCluster )
{
	if ( iCluster >= 0 )
	{
		return pvs[iCluster >> 3] & ( 1 << ( iCluster & 7 ) );
	}
	else
	{
		// PointInLeaf still returns -1 for valid points sometimes and rather than 
		// have black samples, we assume the sample is in the PVS.
		return 1;
	}
}

// returns contents at intersection point (or CONTENTS_EMPTY if no intersection)
int TestLine (Vector const& start, Vector const& stop, int node, int iThread,
			  int static_prop_index_to_ignore=-1 );

// returns surface flags at intersection (zero if no intersection or no surface properties)
texinfo_t *TestLine_Surface( int node, Vector const& start, Vector const& stop, int iThread, 
							 bool canRecurse = true, int static_prop_to_skip=-1 );

void BaseLightForFace( dface_t *f, Vector& light, float *parea, Vector& reflectivity );
void CreateDirectLights (void);
void GetPhongNormal( int facenum, Vector const& spot, Vector& phongnormal );
int LightForString( char *pLight, Vector& intensity );
void MakeTransfer( int ndxPatch1, int ndxPatch2, transfer_t *all_transfers );
void MakeScales( int ndxPatch, transfer_t *all_transfers );

// Run startup code like initialize mathlib.
void VRAD_Init();

// Load the BSP file and prepare to do the lighting.
// This is called after any command-line parameters have been set.
void VRAD_LoadBSP( char const *pFilename );

int VRAD_Main(int argc, char **argv);

// This performs an actual lighting pass.
// Returns true if the process was interrupted (with g_bInterrupt).
bool RadWorld_Go();

dleaf_t		*PointInLeaf (Vector const& point);
int			ClusterFromPoint( Vector const& point );
winding_t	*WindingFromFace (dface_t *f, Vector& origin );

void WriteWinding (FileHandle_t out, winding_t *w, Vector& color );
void WriteNormal( FileHandle_t out, Vector const &nPos, Vector const &nDir, 
				  float length, Vector const &color );

#ifdef STATIC_FOG
qboolean IsFog( dface_t * f );
#endif

#define CONTENTS_EMPTY	0
#define TEX_SPECIAL		(SURF_SKY|SURF_NOLIGHT)

//=============================================================================

// trace.cpp

bool AddDispCollTreesToWorld( void );
int PointLeafnum( Vector const &point );

//=============================================================================

// dispinfo.cpp

struct sampleLightOutput_t
{
	float		dot[NUM_BUMP_VECTS+1];
	float		falloff;
};

bool GatherSampleLight( sampleLightOutput_t &out, directlight_t *dl, int facenum, 
						Vector const& pos, Vector *pNormals, int normalCount, int iThread,
						bool force_fast = false, int static_prop_to_skip=-1 );

//-----------------------------------------------------------------------------
// VRad Displacements
//-----------------------------------------------------------------------------

struct facelight_t;
typedef struct radial_s radial_t;
struct lightinfo_t;

// NOTE: should probably come up with a bsptreetested_t struct or something,
//       see below (PropTested_t)
struct DispTested_t
{
	int	m_Enum;
	int	*m_pTested;
};

class IVRadDispMgr
{
public:
	// creation/destruction
	virtual void Init( void ) = 0;
	virtual void Shutdown( void ) = 0;

	// "CalcPoints"
	virtual bool BuildDispSamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace ) = 0;
	virtual bool BuildDispLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace ) = 0;
	virtual bool BuildDispSamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace ) = 0;

	// patching functions
	virtual void MakePatches( void ) = 0;
	virtual void SubdividePatch( int ndxPatch ) = 0;

	// pre "FinalLightFace"
	virtual void InsertSamplesDataIntoHashTable( void ) = 0;
	virtual void InsertPatchSampleDataIntoHashTable( void ) = 0;

	// "FinalLightFace"
	virtual radial_t *BuildLuxelRadial( int ndxFace, int ndxStyle, bool bBump ) = 0;
	virtual bool SampleRadial( int ndxFace, radial_t *pRadial, Vector const &vPos, int ndxLxl, Vector *pLightSample, int sampleCount, bool bPatch ) = 0;
	virtual radial_t *BuildPatchRadial( int ndxFace, bool bBump ) = 0;

	// utility
	virtual	void GetDispSurfNormal( int ndxFace, Vector &pt, Vector &ptNormal, bool bInside ) = 0;
	virtual void GetDispSurf( int ndxFace, CVRADDispColl **ppDispTree ) = 0;

	// bsp tree functions
	virtual bool ClipRayToDisp( DispTested_t &dispTested, Ray_t const &ray ) = 0;
	virtual bool ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, int ndxLeaf ) = 0;
	virtual void ClipRayToDispInLeaf( DispTested_t &dispTested, Ray_t const &ray, 
				int ndxLeaf, float& dist, dface_t*& pFace, Vector2D& luxelCoord ) = 0;
	virtual void StartRayTest( DispTested_t &dispTested ) = 0;

	// general timing -- should be moved!!
	virtual void StartTimer( const char *name ) = 0;
	virtual void EndTimer( void ) = 0;
};

IVRadDispMgr *StaticDispMgr( void );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline bool ValidDispFace( dface_t *pFace )
{
	if( !pFace ) { return false; }
	if( pFace->dispinfo == -1 ) { return false; }
	if( pFace->numedges != 4 ) { return false; }

	return true;
}

#define SAMPLEHASH_VOXEL_SIZE			64.0f
typedef unsigned int SampleHandle_t;				// the upper 16 bits = facelight index (works because max face are 65536)
													// the lower 16 bits = sample index inside of facelight
struct sample_t;
struct SampleData_t
{
	unsigned short				x, y, z;
	CUtlVector<SampleHandle_t>	m_Samples;
};

struct PatchSampleData_t
{
	unsigned short				x, y, z;
	CUtlVector<int>				m_ndxPatches;
};

UtlHashHandle_t SampleData_AddSample( sample_t *pSample, SampleHandle_t sampleHandle );
void PatchSampleData_AddSample( patch_t *pPatch, int ndxPatch );
unsigned short IncrementPatchIterationKey();
void SampleData_Log( void );

extern CUtlHash<SampleData_t>		g_SampleHashTable;
extern CUtlHash<PatchSampleData_t>	g_PatchSampleHashTable;

extern int samplesAdded;
extern int patchSamplesAdded;

//-----------------------------------------------------------------------------
// Computes lighting for the detail props
//-----------------------------------------------------------------------------

void ComputeDetailPropLighting( int iThread );
void ComputeIndirectLightingAtPoint( Vector &position, Vector &normal, Vector &outColor, 
									 int iThread, bool force_fast = false );

//-----------------------------------------------------------------------------
// VRad static props
//-----------------------------------------------------------------------------
class IPhysicsCollision;
struct PropTested_t
{
	int m_Enum;
	int* m_pTested;
	IPhysicsCollision *pThreadedCollision;
};

class IVradStaticPropMgr
{
public:
	// methods of IStaticPropMgr
	virtual void Init() = 0;
	virtual void Shutdown() = 0;
	virtual bool ClipRayToStaticProps( PropTested_t& propTested, Ray_t const& ray ) = 0;
	virtual bool ClipRayToStaticPropsInLeaf( PropTested_t& propTested, Ray_t const& ray, int leaf ) = 0;
	virtual void StartRayTest( PropTested_t& propTested ) = 0;
	virtual void ComputeLighting( int iThread ) = 0;
	virtual void AddPolysForRayTrace() = 0;
};

IVradStaticPropMgr* StaticPropMgr();

#endif // VRAD_H
