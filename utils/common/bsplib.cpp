//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cmdlib.h"
#include "mathlib.h"
#include "bsplib.h"
#include "zip_utils.h"
#include "scriplib.h"
#include "UtlLinkedList.h"
#include "BSPTreeData.h"
#include "CModel.h"
#include "GameBSPFile.h"
#include "UtlBuffer.h"
#include "UtlRBTree.h"
#include "UtlSymbol.h"
#include "checksum_crc.h"
#include "tier0/dbg.h"
#include "lumpfiles.h"

//=============================================================================

// "-hdr" tells us to use the HDR fields (if present) on the light sources.  Also, tells us to write
// out the HDR lumps for lightmaps, ambient leaves, and lights sources.
bool g_bHDR = false;

uint32 g_LevelFlags = 0;

int			nummodels;
dmodel_t	dmodels[MAX_MAP_MODELS];

int			visdatasize;
byte		dvisdata[MAX_MAP_VISIBILITY];
dvis_t		*dvis = (dvis_t *)dvisdata;

CUtlVector<byte> dlightdataHDR;
CUtlVector<byte> dlightdataLDR;
CUtlVector<byte> *pdlightdata = &dlightdataLDR;

CUtlVector<char> dentdata;

int			numleafs;
dleaf_t		dleafs[MAX_MAP_LEAFS];

CUtlVector<CompressedLightCube> g_LeafAmbientLightingLDR;
CUtlVector<CompressedLightCube> g_LeafAmbientLightingHDR;
CUtlVector<CompressedLightCube> *g_pLeafAmbientLighting = NULL; // &g_LeafAmbientLightingLDR;

unsigned short  g_LeafMinDistToWater[MAX_MAP_LEAFS];

int			numplanes;
dplane_t	dplanes[MAX_MAP_PLANES];

int			numvertexes;
dvertex_t	dvertexes[MAX_MAP_VERTS];

int				g_numvertnormalindices;	// dfaces reference these. These index g_vertnormals.
unsigned short	g_vertnormalindices[MAX_MAP_VERTNORMALS];

int				g_numvertnormals;	
Vector			g_vertnormals[MAX_MAP_VERTNORMALS];

int			numnodes;
dnode_t		dnodes[MAX_MAP_NODES];

CUtlVector<texinfo_t> texinfo( MAX_MAP_TEXINFO );

int			numtexdata;
dtexdata_t	dtexdata[MAX_MAP_TEXDATA];

//
// displacement map bsp file info: dispinfo
//
CUtlVector<ddispinfo_t> g_dispinfo;
CUtlVector<CDispVert> g_DispVerts;
CUtlVector<CDispTri> g_DispTris;
CUtlVector<unsigned char> g_DispLightmapSamplePositions; // LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS

int         numorigfaces;
dface_t     dorigfaces[MAX_MAP_FACES];

int				g_numprimitives = 0;
dprimitive_t	g_primitives[MAX_MAP_PRIMITIVES];

int				g_numprimverts = 0;
dprimvert_t		g_primverts[MAX_MAP_PRIMVERTS];

int				g_numprimindices = 0;
unsigned short	g_primindices[MAX_MAP_PRIMINDICES];

int			numfaces;
dface_t		dfaces[MAX_MAP_FACES];

int			numfaces_hdr;
dface_t		dfaces_hdr[MAX_MAP_FACES];

int			numedges;
dedge_t		dedges[MAX_MAP_EDGES];

int			numleaffaces;
unsigned short		dleaffaces[MAX_MAP_LEAFFACES];

int			numleafbrushes;
unsigned short		dleafbrushes[MAX_MAP_LEAFBRUSHES];

int			numsurfedges;
int			dsurfedges[MAX_MAP_SURFEDGES];

int			numbrushes;
dbrush_t	dbrushes[MAX_MAP_BRUSHES];

int			numbrushsides;
dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

int			numareas;
darea_t		dareas[MAX_MAP_AREAS];

int			numareaportals;
dareaportal_t	dareaportals[MAX_MAP_AREAPORTALS];

int			numworldlightsLDR;
dworldlight_t dworldlightsLDR[MAX_MAP_WORLDLIGHTS];

int			numworldlightsHDR;
dworldlight_t dworldlightsHDR[MAX_MAP_WORLDLIGHTS];

int			*pNumworldlights = &numworldlightsLDR;
dworldlight_t *dworldlights = dworldlightsLDR;

int			numportals = 0;
dportal_t	dportals[MAX_MAP_PORTALS];

int			numclusters = 0;
dcluster_t	dclusters[MAX_MAP_CLUSTERS];

int			numleafwaterdata = 0;
dleafwaterdata_t dleafwaterdata[MAX_MAP_LEAFWATERDATA]; 

int			numportalverts = 0;
unsigned short	dportalverts[MAX_MAP_PORTALVERTS];

int					numclusterportals;
unsigned short		dclusterportals[MAX_MAP_PORTALS*2];

CUtlVector<CFaceMacroTextureInfo>	g_FaceMacroTextureInfos;

Vector				g_ClipPortalVerts[MAX_MAP_PORTALVERTS];
int					g_nClipPortalVerts;

dcubemapsample_t	g_CubemapSamples[MAX_MAP_CUBEMAPSAMPLES];
int					g_nCubemapSamples = 0;

int					g_nOverlayCount;
doverlay_t			g_Overlays[MAX_MAP_OVERLAYS];

int					g_nWaterOverlayCount;
dwateroverlay_t		g_WaterOverlays[MAX_MAP_WATEROVERLAYS];

CUtlVector<char>	g_TexDataStringData;
CUtlVector<int>		g_TexDataStringTable;

byte				*g_pPhysCollide = NULL;
int					g_PhysCollideSize = 0;
int					g_MapRevision = 0;

CUtlVector<doccluderdata_t>	g_OccluderData( 256, 256 );
CUtlVector<doccluderpolydata_t>	g_OccluderPolyData( 1024, 1024 );
CUtlVector<int>	g_OccluderVertexIndices( 2048, 2048 );

CUtlVector<dlightmappage_t>		g_dLightmapPages;
CUtlVector<dlightmappageinfo_t> g_dLightmapPageInfos;
 
static void AddLump (int lumpnum, void *data, int len, int version = 0 );

dheader_t	*header;
FileHandle_t wadfile;
dheader_t	outheader;

struct 
{
	void	*pLumps[ HEADER_LUMPS ];
	int		lumpParsed[ HEADER_LUMPS ];
	int		size[ HEADER_LUMPS ];
} g_Lumps;

//-----------------------------------------------------------------------------
// Game lump memory storage
//-----------------------------------------------------------------------------


// NOTE: This is not optimal at all; since I expect client lumps to
// not be accessed all that often.

struct GameLump_t
{
	GameLumpId_t	m_Id;
	unsigned short	m_Flags;
	unsigned short	m_Version;
	CUtlMemory< unsigned char >	m_Memory;
};

static CUtlLinkedList< GameLump_t, GameLumpHandle_t >	s_GameLumps;

//-----------------------------------------------------------------------------
// Purpose: // Singlegon instance
// Output : CPakFile&
//-----------------------------------------------------------------------------
IZip* GetPakFile( void )
{
	return zip_utils;
}

//-----------------------------------------------------------------------------
// Purpose: Set the sector alignment for all subsequent zip operations
//-----------------------------------------------------------------------------
void ForceAlignment( bool bAlign, unsigned int sectorSize )
{
	GetPakFile()->ForceAlignment( bAlign, sectorSize );
}

//-----------------------------------------------------------------------------
// Purpose: Store data back out to .bsp file
//-----------------------------------------------------------------------------
static void WritePakFileLump( void )
{
	CUtlBuffer buf( 0, 0 );

	GetPakFile()->SaveToBuffer( buf );

	unsigned int align = GetPakFile()->GetAlignment();
	if ( align )
	{
		// must repsect pak file alignment
		// pad up and ensure lump starts on same aligned boundary
		int filePos = g_pFileSystem->Tell(wadfile);
		int size = ((filePos + align - 1) & ~(align-1)) - filePos;

		if ( size )
		{
			char *pData = (char *)malloc( size );
			memset( pData, 0, size );
			SafeWrite(wadfile, pData, size );
			free( pData );
		}
	}

	// Now store final buffer out to file
	AddLump( LUMP_PAKFILE, buf.Base(), buf.TellPut() );
}

//-----------------------------------------------------------------------------
// Purpose: Store data back out to .bsp file
//-----------------------------------------------------------------------------
static void WriteXZPPakFileLump( char* xzpFilename )
{
	unsigned int align = 512;  // HACK!
	if ( align )
	{
		// must repsect pak file alignment
		// pad up and ensure lump starts on same aligned boundary
		int filePos = g_pFileSystem->Tell(wadfile);
		int size = ((filePos + align - 1) & ~(align-1)) - filePos;

		if ( size )
		{
			char *pData = (char *)malloc( size );
			memset( pData, 0, size );
			SafeWrite(wadfile, pData, size );
			free( pData );
		}
	}

	FILE* hXZP = fopen(xzpFilename, "rb");
	fseek(hXZP,0,SEEK_END);
	int length = ftell(hXZP);
	fseek(hXZP,0,SEEK_SET);
	void* buffer = malloc( length );
	fread(buffer,1,length,hXZP);
	fclose(hXZP);

	// Now store final buffer out to file
	AddLump( LUMP_XZIPPAKFILE, buffer, length );
	free(buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Remove all entries
//-----------------------------------------------------------------------------
void ClearPackFile( void )
{
	GetPakFile()->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Add file from disk to .bsp PAK lump
// Input  : *relativename - 
//			*fullpath - 
//-----------------------------------------------------------------------------
void AddFileToPack( const char *relativename, const char *fullpath )
{
	GetPakFile()->AddFileToZip( relativename, fullpath );
}

//-----------------------------------------------------------------------------
// Purpose: Add buffer to .bsp PAK lump as named file
// Input  : *relativename - 
//			*data - 
//			length - 
//-----------------------------------------------------------------------------
void AddBufferToPack( const char *pRelativeName, void *data, int length, bool bTextMode )
{
	GetPakFile()->AddBufferToZip( pRelativeName, data, length, bTextMode );
}

//-----------------------------------------------------------------------------
// Purpose: Check if a file already exists in the pack file.
// Input  : *relativename - 
//-----------------------------------------------------------------------------
bool FileExistsInPack( const char *pRelativeName )
{
	return GetPakFile()->FileExistsInZip( pRelativeName );
}


//-----------------------------------------------------------------------------
// Read a file from the pack file
//-----------------------------------------------------------------------------
bool ReadFileFromPack( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf )
{
	return GetPakFile()->ReadFileFromZip( pRelativeName, bTextMode, buf );
}


//-----------------------------------------------------------------------------
// Purpose: Remove file from .bsp PAK lump
// Input  : *relativename - 
//-----------------------------------------------------------------------------
void RemoveFileFromPack( const char *relativename )
{
	GetPakFile()->RemoveFileFromZip( relativename );
}


//-----------------------------------------------------------------------------
// Purpose: Get next filename in directory
// Input  : id, -1 to start, returns next id, or -1 at list conclusion 
//-----------------------------------------------------------------------------
int GetNextFilename( int id, char *pBuffer, int bufferSize, int &fileSize )
{
	return GetPakFile()->GetNextFilename( id, pBuffer, bufferSize, fileSize );
}


//-----------------------------------------------------------------------------
// Convert four-CC code to a handle	+ back
//-----------------------------------------------------------------------------
GameLumpHandle_t GetGameLumpHandle( GameLumpId_t id )
{
	// NOTE: I'm also expecting game lump id's to be four-CC codes
	Assert( id > HEADER_LUMPS );

	for (int i = s_GameLumps.Count(); --i >= 0; )
	{
		if (s_GameLumps[i].m_Id == id)
			return i;
	}

	return InvalidGameLump();
}

GameLumpId_t GetGameLumpId( GameLumpHandle_t handle )
{
	return s_GameLumps[handle].m_Id;
}

int	GetGameLumpFlags( GameLumpHandle_t handle )
{
	return s_GameLumps[handle].m_Flags;
}

int	GetGameLumpVersion( GameLumpHandle_t handle )
{
	return s_GameLumps[handle].m_Version;
}


//-----------------------------------------------------------------------------
// Game lump accessor methods 
//-----------------------------------------------------------------------------

void*	GetGameLump( GameLumpHandle_t id )
{
	return s_GameLumps[id].m_Memory.Base();
}

int		GameLumpSize( GameLumpHandle_t id )
{
	return s_GameLumps[id].m_Memory.NumAllocated();
}


//-----------------------------------------------------------------------------
// Game lump iteration methods 
//-----------------------------------------------------------------------------

GameLumpHandle_t	FirstGameLump()
{
	return (s_GameLumps.Count()) ? 0 : InvalidGameLump();
}

GameLumpHandle_t	NextGameLump( GameLumpHandle_t handle )
{
	++handle;
	return (handle  < s_GameLumps.Count()) ? handle : InvalidGameLump();
}

GameLumpHandle_t	InvalidGameLump()
{
	return 0xFFFF;
}


//-----------------------------------------------------------------------------
// Game lump creation/destruction method
//-----------------------------------------------------------------------------

GameLumpHandle_t	CreateGameLump( GameLumpId_t id, int size, int flags, int version )
{
	Assert( GetGameLumpHandle(id) == InvalidGameLump() );
	GameLumpHandle_t handle = s_GameLumps.AddToTail();
	s_GameLumps[handle].m_Id = id;
	s_GameLumps[handle].m_Flags = flags;
	s_GameLumps[handle].m_Version = version;
	s_GameLumps[handle].m_Memory.EnsureCapacity( size );
	return handle;
}

void	DestroyGameLump( GameLumpHandle_t handle )
{
	s_GameLumps.Remove( handle );
}

void	DestroyAllGameLumps()
{
	s_GameLumps.RemoveAll();
}

//-----------------------------------------------------------------------------
// String table methods
//-----------------------------------------------------------------------------
const char *TexDataStringTable_GetString( int stringID )
{
	return &g_TexDataStringData[g_TexDataStringTable[stringID]];
}

int	TexDataStringTable_AddOrFindString( const char *pString )
{
	int i;
	// garymcthack: Make this use an RBTree!
	for( i = 0; i < g_TexDataStringTable.Count(); i++ )
	{
		if( stricmp( pString, &g_TexDataStringData[g_TexDataStringTable[i]] ) == 0 )
		{
			return i;
		}
	}

	int len = strlen( pString );
	int outOffset = g_TexDataStringData.AddMultipleToTail( len+1, pString );
	int outIndex = g_TexDataStringTable.AddToTail( outOffset );
	return outIndex;
}

//-----------------------------------------------------------------------------
// Compute file size and clump count
//-----------------------------------------------------------------------------

static void ComputeGameLumpSizeAndCount( int& size, int& clumpCount )
{
	// Figure out total size of the client lumps
	size = 0;
	clumpCount = 0;
	GameLumpHandle_t h;
	for( h = FirstGameLump(); h != InvalidGameLump(); h = NextGameLump( h ) )
	{
		++clumpCount;
		size += GameLumpSize( h );
	}

	// Add on headers
	size += sizeof( dgamelumpheader_t ) + clumpCount * sizeof( dgamelump_t );
}

//-----------------------------------------------------------------------------
// Adds all game lumps into one big block
//-----------------------------------------------------------------------------

static void AddGameLumps( )
{
	// Figure out total size of the client lumps
	int size, clumpCount;
	ComputeGameLumpSizeAndCount( size, clumpCount );

	// Set up the main lump dictionary entry
	g_Lumps.size[LUMP_GAME_LUMP] = 0;	// mark it written

	lump_t* lump = &header->lumps[LUMP_GAME_LUMP];
	
	lump->fileofs = LittleLong( g_pFileSystem->Tell(wadfile) );
	lump->filelen = LittleLong(size);

	// write header
	dgamelumpheader_t header;
	header.lumpCount = clumpCount;
	SafeWrite (wadfile, &header, sizeof(header));

	// write dictionary
	dgamelump_t dict;
	int offset = lump->fileofs + sizeof(header) + clumpCount * sizeof(dgamelump_t);
	GameLumpHandle_t h;
	for( h = FirstGameLump(); h != InvalidGameLump(); h = NextGameLump( h ) )
	{
		dict.id = GetGameLumpId(h);
		dict.version = GetGameLumpVersion(h);
		dict.flags = GetGameLumpFlags(h);
		dict.fileofs = offset;
		dict.filelen = GameLumpSize( h );
		SafeWrite (wadfile, &dict, sizeof(dict));

		offset += dict.filelen;
	}

	// write lumps..
	for( h = FirstGameLump(); h != InvalidGameLump(); h = NextGameLump( h ) )
	{
		SafeWrite (wadfile, GetGameLump(h), GameLumpSize(h));
	}

	// align to doubleword
	int totSize = (lump->filelen + 3) & ~0x3;
	totSize -= lump->filelen;
	if (totSize > 0)
	{
		char buf[3] = { 0, 0, 0 };
		SafeWrite (wadfile, buf, totSize);
	}
}


//-----------------------------------------------------------------------------
// Game lump file I/O
//-----------------------------------------------------------------------------
static void	ParseGameLump( dheader_t* pHeader )
{
	DestroyAllGameLumps();

	g_Lumps.lumpParsed[LUMP_GAME_LUMP] = 1; // mark it parsed

	int length = pHeader->lumps[LUMP_GAME_LUMP].filelen;
	int ofs = pHeader->lumps[LUMP_GAME_LUMP].fileofs;
	
	if (length > 0)
	{
		// Read dictionary...
		dgamelumpheader_t* pGameLumpHeader = (dgamelumpheader_t*)((byte *)pHeader + ofs);
		dgamelump_t* pGameLump = (dgamelump_t*)(pGameLumpHeader + 1);
		for (int i = 0; i < pGameLumpHeader->lumpCount; ++i )
		{
			int length = pGameLump[i].filelen;
			GameLumpHandle_t lump = CreateGameLump( pGameLump[i].id, length, pGameLump[i].flags,
				pGameLump[i].version );
			memcpy (GetGameLump(lump), (byte *)pHeader + pGameLump[i].fileofs, length);
		}
	}
}


//-----------------------------------------------------------------------------
// Adds the occluder lump...
//-----------------------------------------------------------------------------
static void AddOcclusionLump( )
{
	g_Lumps.size[LUMP_OCCLUSION] = 0;	// mark it written

	int nOccluderCount = g_OccluderData.Count();
	int nOccluderPolyDataCount = g_OccluderPolyData.Count();
	int nOccluderVertexIndices = g_OccluderVertexIndices.Count();

	int nLumpLength = nOccluderCount * sizeof(doccluderdata_t) +
		nOccluderPolyDataCount * sizeof(doccluderpolydata_t) +
		nOccluderVertexIndices * sizeof(int) +
		3 * sizeof(int);

	lump_t *lump = &header->lumps[LUMP_OCCLUSION];
	
	lump->fileofs = LittleLong( g_pFileSystem->Tell(wadfile) );
	lump->filelen = LittleLong( nLumpLength );
	lump->version = LittleLong( LUMP_OCCLUSION_VERSION );
	lump->fourCC[0] = ( char )0;
	lump->fourCC[1] = ( char )0;
	lump->fourCC[2] = ( char )0;
	lump->fourCC[3] = ( char )0;

	SafeWrite( wadfile, &nOccluderCount, 4 );
	SafeWrite( wadfile, g_OccluderData.Base(), nOccluderCount * sizeof(doccluderdata_t) );
	SafeWrite( wadfile, &nOccluderPolyDataCount, 4 );
	SafeWrite( wadfile, g_OccluderPolyData.Base(), nOccluderPolyDataCount * sizeof(doccluderpolydata_t) );
	SafeWrite( wadfile, &nOccluderVertexIndices, 4 );
	SafeWrite( wadfile, g_OccluderVertexIndices.Base(), nOccluderVertexIndices * sizeof(int) );
}


//-----------------------------------------------------------------------------
// Loads the occluder lump...
//-----------------------------------------------------------------------------
static void UnserializeOcclusionLumpV2( CUtlBuffer &buf )
{
	int nCount = buf.GetInt();
	if ( nCount )
	{
		g_OccluderData.SetCount( nCount );
		buf.Get( g_OccluderData.Base(), nCount * sizeof(g_OccluderData[0]) );
	}

	nCount = buf.GetInt();
	if ( nCount )
	{
		g_OccluderPolyData.SetCount( nCount );
		buf.Get( g_OccluderPolyData.Base(), nCount * sizeof(g_OccluderPolyData[0]) );
	}

	nCount = buf.GetInt();
	if ( nCount )
	{
		g_OccluderVertexIndices.SetCount( nCount );
		buf.Get( g_OccluderVertexIndices.Base(), nCount * sizeof(g_OccluderVertexIndices[0]) );
	}
}


static void LoadOcclusionLump()
{
	g_OccluderData.RemoveAll();
	g_OccluderPolyData.RemoveAll();
	g_OccluderVertexIndices.RemoveAll();

	int		length, ofs;

	g_Lumps.lumpParsed[LUMP_OCCLUSION] = 1; // mark it parsed

	length = header->lumps[LUMP_OCCLUSION].filelen;
	ofs = header->lumps[LUMP_OCCLUSION].fileofs;
	
	CUtlBuffer buf( (byte *)header + ofs, length, CUtlBuffer::READ_ONLY );
	switch (header->lumps[LUMP_OCCLUSION].version)
	{
	case 2:
		UnserializeOcclusionLumpV2( buf );
		break;

	case 0:
		break;

	default:
		Error("Unknown occlusion lump version!\n");
		break;
	}
}


/*
===============
CompressVis

===============
*/
int CompressVis (byte *vis, byte *dest)
{
	int		j;
	int		rep;
	int		visrow;
	byte	*dest_p;
	
	dest_p = dest;
//	visrow = (r_numvisleafs + 7)>>3;
	visrow = (dvis->numclusters + 7)>>3;
	
	for (j=0 ; j<visrow ; j++)
	{
		*dest_p++ = vis[j];
		if (vis[j])
			continue;

		rep = 1;
		for ( j++; j<visrow ; j++)
			if (vis[j] || rep == 255)
				break;
			else
				rep++;
		*dest_p++ = rep;
		j--;
	}
	
	return dest_p - dest;
}


/*
===================
DecompressVis
===================
*/
void DecompressVis (byte *in, byte *decompressed)
{
	int		c;
	byte	*out;
	int		row;

//	row = (r_numvisleafs+7)>>3;	
	row = (dvis->numclusters+7)>>3;	
	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		if (!c)
			Error ("DecompressVis: 0 repeat");
		in += 2;
		if ((out - decompressed) + c > row)
		{
			c = row - (out - decompressed);
			Warning( "warning: Vis decompression overrun\n" );
		}
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
}

//=============================================================================

int LumpVersion( int lump )
{
	return header->lumps[lump].version;
}

bool HasLump( int lump )
{
	return header->lumps[lump].filelen > 0;
}

int CopyLump (int lump, void *dest, int size, int forceVersion = -1)
{
	int		length, ofs;

	g_Lumps.lumpParsed[lump] = 1; // mark it parsed

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % size)
	{
		Error ("LoadBSPFile: odd size for lump %d", lump );
	}

	if ( forceVersion >= 0 && forceVersion != header->lumps[lump].version )
	{
		Error ("LoadBSPFile: old version for lump %d in map!", lump );
	}
	
	memcpy (dest, (byte *)header + ofs, length);

	return length / size;
}

template< class T >
void CopyLump ( int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	int		length, ofs;

	g_Lumps.lumpParsed[lump] = 1; // mark it parsed

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % sizeof(T))
	{
		Error ("LoadBSPFile: odd size for lump %d", lump );
	}

	if ( forceVersion >= 0 && forceVersion != header->lumps[lump].version )
	{
		Error ("LoadBSPFile: old version for lump %d in map!", lump );
	}

	dest.SetSize( length / sizeof(T) );
	memcpy( dest.Base(), (byte *)header + ofs, length );
}

template< class T >
void CopyOptionalLump( int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	int		length, ofs;

	g_Lumps.lumpParsed[lump] = 1; // mark it parsed

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % sizeof(T))
	{
		Error ("LoadBSPFile: odd size for lump %d", lump );
	}

	// not fatal if not present
	if ( length && forceVersion >= 0 && forceVersion != header->lumps[lump].version )
	{
		Error ("LoadBSPFile: old version for lump %d in map!", lump );
	}

	dest.SetSize( length / sizeof(T) );
	memcpy( dest.Base(), (byte *)header + ofs, length );
}

int CopyVariableLump( int lump, void **dest, int size, int forceVersion = -1 )
{
	int		length, ofs;

	g_Lumps.lumpParsed[lump] = 1;	// mark it parsed

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % size)
	{
		Error ("LoadBSPFile: odd lump size");
	}

	if ( forceVersion >= 0 && forceVersion != header->lumps[lump].version )
	{
		Error ("LoadBSPFile: old version map!");
	}

	*dest = malloc( length );
	memcpy (*dest, (byte *)header + ofs, length);

	return length / size;
}

void Lumps_Init( void )
{
	memset( &g_Lumps, 0, sizeof(g_Lumps) );
}

void Lumps_Parse( void )
{
	int i;

	for ( i = 0; i < HEADER_LUMPS; i++ )
	{
		if ( !g_Lumps.lumpParsed[i] && header->lumps[i].filelen )
		{
			g_Lumps.size[i] = CopyVariableLump( i, &g_Lumps.pLumps[i], 1 );
			Msg("Reading unknown lump #%d (%d bytes)\n", i, g_Lumps.size[i] );
		}
	}
}

void Lumps_Write( void )
{
	int i;

	for ( i = 0; i < HEADER_LUMPS; i++ )
	{
		if ( g_Lumps.size[i] )
		{
			Msg("Writing unknown lump #%d (%d bytes)\n", i, g_Lumps.size[i] );
			AddLump( i, g_Lumps.pLumps[i], g_Lumps.size[i] );
		}
		if ( g_Lumps.pLumps[i] )
		{
			free( g_Lumps.pLumps[i] );
			g_Lumps.pLumps[i] = NULL;
		}
	}
}


int LoadLeafs( void )
{
	switch( LumpVersion( LUMP_LEAFS ) )
	{
	case 0:
		{
			int length = header->lumps[LUMP_LEAFS].filelen;
			g_Lumps.lumpParsed[LUMP_LEAFS] = 1; // mark it parsed
			int size = sizeof( dleaf_version_0_t );
			if( length % size )
			{
				Error( "odd size for LUMP_LEAFS\n" );
			}
			int count = length / size;
			dleaf_version_0_t *pSrc = ( dleaf_version_0_t * )( ( byte * )header + header->lumps[LUMP_LEAFS].fileofs );
			dleaf_t *pDst = dleafs;
			Assert( g_pLeafAmbientLighting );
			g_pLeafAmbientLighting->SetCount( count );
			CompressedLightCube *pDstLeafAmbientLighting = &(*g_pLeafAmbientLighting)[0];
			int i;
			for( i = 0; i < count; i++ )
			{
				// pDst is a subset of pSrc;
				*pDst = *( ( dleaf_t * )( void * )pSrc );
				*pDstLeafAmbientLighting = pSrc->m_AmbientLighting;
				pDst++;
				pSrc++;
				pDstLeafAmbientLighting++;
			}
			return count;
		}		
		break;
	case 1:
		return CopyLump (LUMP_LEAFS, dleafs, sizeof(dleaf_t));
		break;
	default:
		Assert( 0 );
		Error( "Unknown LUMP_LEAFS version\n" );
		return 0;
		break;
	}
}

/*
=============
OpenBSPFile

Low level BSP opener for external parsing. Parses headers, but nothing else.
You must close the BSP, via CloseBSPFile().
=============
*/
void	OpenBSPFile (char *filename)
{
	int i;

	Lumps_Init();

	// load the file header
	LoadFile (filename, (void **)&header);

	// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version < MINBSPVERSION || header->version > BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, BSPVERSION);

	g_MapRevision = header->mapRevision;
}

/*
=============
CloseBSPFile
=============
*/
void	CloseBSPFile ( void )
{
	free (header);		
}

/*
=============
LoadBSPFile
=============
*/
void	LoadBSPFile (char *filename)
{
	OpenBSPFile( filename );

	nummodels = CopyLump (LUMP_MODELS, dmodels, sizeof(dmodel_t));
	numvertexes = CopyLump (LUMP_VERTEXES, dvertexes, sizeof(dvertex_t));
	numplanes = CopyLump (LUMP_PLANES, dplanes, sizeof(dplane_t));
	numleafs = LoadLeafs();
	numnodes = CopyLump (LUMP_NODES, dnodes, sizeof(dnode_t));
	CopyLump (LUMP_TEXINFO, texinfo);
	numtexdata = CopyLump (LUMP_TEXDATA, dtexdata, sizeof(dtexdata_t));
    
	CopyLump( LUMP_DISPINFO, g_dispinfo );
    CopyLump( LUMP_DISP_VERTS, g_DispVerts );
	CopyLump( LUMP_DISP_TRIS, g_DispTris );
    CopyLump( LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, g_DispLightmapSamplePositions );
	CopyLump( LUMP_FACE_MACRO_TEXTURE_INFO, g_FaceMacroTextureInfos );
	
	numfaces = CopyLump (LUMP_FACES, dfaces, sizeof(dface_t), LUMP_FACES_VERSION);
	if (HasLump( LUMP_FACES_HDR ))
		numfaces_hdr = CopyLump (LUMP_FACES_HDR, dfaces_hdr, sizeof(dface_t), LUMP_FACES_VERSION);
	else
		numfaces_hdr = 0;

	g_numprimitives = CopyLump (LUMP_PRIMITIVES, g_primitives, sizeof( dprimitive_t ) );
	g_numprimverts = CopyLump (LUMP_PRIMVERTS, g_primverts, sizeof( dprimvert_t ) );
	g_numprimindices = CopyLump (LUMP_PRIMINDICES, g_primindices, sizeof( unsigned short ) );
    numorigfaces = CopyLump( LUMP_ORIGINALFACES, dorigfaces, sizeof( dface_t ) );   // original faces
	numleaffaces = CopyLump (LUMP_LEAFFACES, dleaffaces, sizeof(dleaffaces[0]));
	numleafbrushes = CopyLump (LUMP_LEAFBRUSHES, dleafbrushes, sizeof(dleafbrushes[0]));
	numsurfedges = CopyLump (LUMP_SURFEDGES, dsurfedges, sizeof(dsurfedges[0]));
	numedges = CopyLump (LUMP_EDGES, dedges, sizeof(dedge_t));
	numbrushes = CopyLump (LUMP_BRUSHES, dbrushes, sizeof(dbrush_t));
	numbrushsides = CopyLump (LUMP_BRUSHSIDES, dbrushsides, sizeof(dbrushside_t));
	numareas = CopyLump (LUMP_AREAS, dareas, sizeof(darea_t));
	numareaportals = CopyLump (LUMP_AREAPORTALS, dareaportals, sizeof(dareaportal_t));

	visdatasize = CopyLump (LUMP_VISIBILITY, dvisdata, 1);
	CopyOptionalLump(LUMP_LIGHTING, dlightdataLDR, LUMP_LIGHTING_VERSION );
	CopyOptionalLump(LUMP_LIGHTING_HDR, dlightdataHDR, LUMP_LIGHTING_VERSION );

	CopyOptionalLump(LUMP_LEAF_AMBIENT_LIGHTING, g_LeafAmbientLightingLDR );
	CopyOptionalLump(LUMP_LEAF_AMBIENT_LIGHTING_HDR, g_LeafAmbientLightingHDR );

	CopyLump (LUMP_ENTITIES, dentdata);
	numworldlightsLDR = CopyLump (LUMP_WORLDLIGHTS, dworldlightsLDR, sizeof(dworldlight_t) );
	numworldlightsHDR = CopyLump (LUMP_WORLDLIGHTS_HDR, dworldlightsHDR, sizeof(dworldlight_t) );
	
	// not present on pc, not an error
	CopyOptionalLump( LUMP_LIGHTMAPPAGES, g_dLightmapPages, LUMP_LIGHTMAPPAGES_VERSION );
	CopyOptionalLump( LUMP_LIGHTMAPPAGEINFOS, g_dLightmapPageInfos, LUMP_LIGHTMAPPAGEINFOS_VERSION );

	numportals = CopyLump (LUMP_PORTALS, dportals, sizeof(dportal_t));
	numclusters = CopyLump (LUMP_CLUSTERS, dclusters, sizeof(dcluster_t));
	numleafwaterdata = CopyLump( LUMP_LEAFWATERDATA, dleafwaterdata, sizeof( dleafwaterdata_t ) );
	numportalverts = CopyLump (LUMP_PORTALVERTS, dportalverts, sizeof(unsigned short));
	numclusterportals = CopyLump (LUMP_CLUSTERPORTALS, dclusterportals, sizeof(unsigned short));
	g_PhysCollideSize = CopyVariableLump( LUMP_PHYSCOLLIDE, (void**)&g_pPhysCollide, 1 );

	g_numvertnormals = CopyLump (LUMP_VERTNORMALS, g_vertnormals, sizeof( g_vertnormals[0] ));
	g_numvertnormalindices = CopyLump (LUMP_VERTNORMALINDICES, g_vertnormalindices, sizeof( g_vertnormalindices[0] ));

	g_nClipPortalVerts = CopyLump( LUMP_CLIPPORTALVERTS, g_ClipPortalVerts, sizeof( g_ClipPortalVerts[0] ) );
	g_nCubemapSamples = CopyLump( LUMP_CUBEMAPS, g_CubemapSamples, sizeof( g_CubemapSamples[0] ) );	

	CopyLump( LUMP_TEXDATA_STRING_DATA, g_TexDataStringData );
	CopyLump( LUMP_TEXDATA_STRING_TABLE, g_TexDataStringTable );

	g_nOverlayCount = CopyLump( LUMP_OVERLAYS, g_Overlays, sizeof( g_Overlays[0] ) );
	g_nWaterOverlayCount = CopyLump( LUMP_WATEROVERLAYS, g_WaterOverlays, sizeof( g_WaterOverlays[0] ) );
	
	dflagslump_t flags_lump;
	
	if (HasLump( LUMP_MAP_FLAGS ))
		CopyLump ( LUMP_MAP_FLAGS, &flags_lump, sizeof( flags_lump ) );
	else
		memset(&flags_lump, 0, sizeof( flags_lump ) );			// default flags to 0

	g_LevelFlags = flags_lump.m_LevelFlags;

	LoadOcclusionLump();

	CopyLump( LUMP_LEAFMINDISTTOWATER, g_LeafMinDistToWater, sizeof( g_LeafMinDistToWater[0] ) );

	/*
	int crap;
	for( crap = 0; crap < g_nBSPStringTable; crap++ )
	{
		Msg( "stringtable %d", ( int )crap );
		Msg( " %d:",  ( int )g_BSPStringTable[crap] );
		puts( &g_BSPStringData[g_BSPStringTable[crap]] );
		puts( "\n" );
	}
	*/
		
	// Load PAK file lump into appropriate data structure
	{
		byte *pakbuffer = NULL;
		int paksize = CopyVariableLump( LUMP_PAKFILE, ( void ** )&pakbuffer, 1 );
		if ( paksize > 0 )
		{
			GetPakFile()->ParseFromBuffer( pakbuffer, paksize );
		}
		else
		{
			GetPakFile()->Reset();
		}

		free( pakbuffer );
	}

	ParseGameLump( header );

	// NOTE: Do NOT call CopyLump after Lumps_Parse() it parses all un-Copyied lumps
	Lumps_Parse();	// parse any additional lumps

	CloseBSPFile();	// everything has been copied out
}


/*
=============
LoadBSPFileFilesystemOnly
=============
*/
void	LoadBSPFile_FileSystemOnly (char *filename)
{
	int			i;

	Lumps_Init();
//
// load the file header
//
	LoadFile (filename, (void **)&header);

// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version < MINBSPVERSION || header->version > BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, BSPVERSION);

	// Load PAK file lump into appropriate data structure
	{
		byte *pakbuffer = NULL;
		int paksize = CopyVariableLump( LUMP_PAKFILE, ( void ** )&pakbuffer, 1 );
		if ( paksize > 0 )
		{
			GetPakFile()->ParseFromBuffer( pakbuffer, paksize );
		}
		else
		{
			GetPakFile()->Reset();
		}

		free( pakbuffer );
	}

	free (header);		// everything has been copied out
}

void ExtractZipFileFromBSP( char *pBSPFileName, char *pZipFileName )
{
	Lumps_Init();
//
// load the file header
//
	LoadFile ( pBSPFileName, (void **)&header);

// swap the header
	int i;
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
		Error ("%s is not a IBSP file", pBSPFileName);
	if (header->version < MINBSPVERSION || BSPVERSION > BSPVERSION)
		Error ("%s is version %i, not %i", pBSPFileName, header->version, BSPVERSION);

	byte *pakbuffer = NULL;
	int paksize = CopyVariableLump( LUMP_PAKFILE, ( void ** )&pakbuffer, 1 );
	if ( paksize > 0 )
	{
		byte *pakbuffer = NULL;
		int paksize = CopyVariableLump( LUMP_PAKFILE, ( void ** )&pakbuffer, 1 );

		FILE *fp;
		fp = fopen( pZipFileName, "wb" );
		if( !fp )
		{
			fprintf( stderr, "can't open %s\n", pZipFileName );
			return;
		}

		fwrite( pakbuffer, paksize, 1, fp );
		fclose( fp );
	}
	else
	{
		int paksize = CopyVariableLump( LUMP_XZIPPAKFILE, ( void ** )&pakbuffer, 1 );
		if ( paksize > 0 )
		{
			byte *pakbuffer = NULL;
			int paksize = CopyVariableLump( LUMP_XZIPPAKFILE, ( void ** )&pakbuffer, 1 );

			FILE *fp;
			fp = fopen( pZipFileName, "wb" );
			if( !fp )
			{
				fprintf( stderr, "can't open %s\n", pZipFileName );
				return;
			}

			fwrite( pakbuffer, paksize, 1, fp ); 
			fclose( fp );
		}
		else
		{
			fprintf( stderr, "zip file is zero length!\n" );
		}
	}
}

/*
=============
LoadBSPFileTexinfo

Only loads the texinfo lump, so qdata can scan for textures
=============
*/
void	LoadBSPFileTexinfo (char *filename)
{
	int			i;
	FILE		*f;
	int		length, ofs;

	header = (dheader_t*)malloc(sizeof(dheader_t));

	f = fopen (filename, "rb");
	fread (header, sizeof(dheader_t), 1, f);

// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
	{
		Error ("%s is not a IBSP file", filename);
	}
	if (header->version < MINBSPVERSION || header->version > BSPVERSION)
	{
		Error ("%s is version %i, not %i", filename, header->version, BSPVERSION);
	}

	length = header->lumps[LUMP_TEXINFO].filelen;
	ofs = header->lumps[LUMP_TEXINFO].fileofs;

	int nCount = length / sizeof(texinfo_t);

	texinfo.Purge();
	texinfo.AddMultipleToTail( nCount );

	fseek (f, ofs, SEEK_SET);
	fread (texinfo.Base(), length, 1, f);
	fclose (f);

	free (header);		// everything has been copied out
	
}


//============================================================================

static void AddLump (int lumpnum, void *data, int len, int version )
{
	lump_t *lump;

	g_Lumps.size[lumpnum] = 0;	// mark it written

	lump = &header->lumps[lumpnum];
	
	lump->fileofs = LittleLong( g_pFileSystem->Tell(wadfile) );
	lump->filelen = LittleLong(len);
	lump->version= LittleLong( version );
	lump->fourCC[0] = ( char )0;
	lump->fourCC[1] = ( char )0;
	lump->fourCC[2] = ( char )0;
	lump->fourCC[3] = ( char )0;
	SafeWrite (wadfile, data, (len+3)&~3);
}

template< class T >
static void AddLump( int lumpnum, CUtlVector<T> &data, int version = 0 )
{
	AddLump( lumpnum, data.Base(), data.Count() * sizeof(T), version );
}


/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void	WriteBSPFile (char *filename, char* xzpLumpFilename )
{		
	if ( texinfo.Count() > MAX_MAP_TEXINFO )
	{
		Error( "Map has too many texinfos (has %d, can have at most %d)\n", texinfo.Count(), MAX_MAP_TEXINFO );
		return;
	}

	header = &outheader;
	memset (header, 0, sizeof(dheader_t));

	header->ident = LittleLong (IDBSPHEADER);
	header->version = LittleLong (BSPVERSION);
	header->mapRevision = LittleLong( g_MapRevision );

	wadfile = SafeOpenWrite (filename);
	SafeWrite (wadfile, header, sizeof(dheader_t));	// overwritten later

	AddLump (LUMP_PLANES, dplanes, numplanes*sizeof(dplane_t));
	AddLump (LUMP_LEAFS, dleafs, numleafs*sizeof(dleaf_t), LUMP_LEAFS_VERSION);
	// Make ambient lighting of zero so that the rest of the code can assume that this lump is here.
	if ( !g_bHDR && g_LeafAmbientLightingLDR.Count() == 0 )
	{
		g_LeafAmbientLightingLDR.SetCount( numleafs );
		memset( g_LeafAmbientLightingLDR.Base(), 0, g_LeafAmbientLightingLDR.Count()*sizeof(CompressedLightCube ));
	}
	AddLump (LUMP_LEAF_AMBIENT_LIGHTING, g_LeafAmbientLightingLDR, g_LeafAmbientLightingLDR.Count()*sizeof(CompressedLightCube));
	// Make ambient lighting of zero so that the rest of the code can assume that this lump is here.
	if ( g_bHDR && g_LeafAmbientLightingHDR.Count() == 0 )
	{
		g_LeafAmbientLightingHDR.SetCount( numleafs );
		memset( g_LeafAmbientLightingHDR.Base(), 0, g_LeafAmbientLightingHDR.Count()*sizeof(CompressedLightCube ));
	}
	AddLump (LUMP_LEAF_AMBIENT_LIGHTING_HDR, g_LeafAmbientLightingHDR, g_LeafAmbientLightingHDR.Count()*sizeof(CompressedLightCube));

	AddLump (LUMP_VERTEXES, dvertexes, numvertexes*sizeof(dvertex_t));
	AddLump (LUMP_NODES, dnodes, numnodes*sizeof(dnode_t));
	AddLump (LUMP_TEXINFO, texinfo);
	AddLump (LUMP_TEXDATA, dtexdata, numtexdata*sizeof(dtexdata_t));    

    AddLump (LUMP_DISPINFO, g_dispinfo );
    AddLump (LUMP_DISP_VERTS, g_DispVerts );
	AddLump (LUMP_DISP_TRIS, g_DispTris );
    AddLump (LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, g_DispLightmapSamplePositions );
	AddLump (LUMP_FACE_MACRO_TEXTURE_INFO, g_FaceMacroTextureInfos );
 
	AddLump (LUMP_PRIMITIVES, g_primitives, g_numprimitives * sizeof( dprimitive_t ) );
	AddLump (LUMP_PRIMVERTS, g_primverts, g_numprimverts * sizeof( dprimvert_t ) );
	AddLump (LUMP_PRIMINDICES, g_primindices, g_numprimindices * sizeof( unsigned short ) );
    AddLump (LUMP_FACES, dfaces, numfaces*sizeof(dface_t), LUMP_FACES_VERSION);
    if (numfaces_hdr)
		AddLump (LUMP_FACES_HDR, dfaces_hdr, numfaces_hdr*sizeof(dface_t), LUMP_FACES_VERSION);

	AddLump (LUMP_ORIGINALFACES, dorigfaces, numorigfaces*sizeof( dface_t ) );     // original faces lump
	AddLump (LUMP_BRUSHES, dbrushes, numbrushes*sizeof(dbrush_t));
	AddLump (LUMP_BRUSHSIDES, dbrushsides, numbrushsides*sizeof(dbrushside_t));
	AddLump (LUMP_LEAFFACES, dleaffaces, numleaffaces*sizeof(dleaffaces[0]));
	AddLump (LUMP_LEAFBRUSHES, dleafbrushes, numleafbrushes*sizeof(dleafbrushes[0]));
	AddLump (LUMP_SURFEDGES, dsurfedges, numsurfedges*sizeof(dsurfedges[0]));
	AddLump (LUMP_EDGES, dedges, numedges*sizeof(dedge_t));
	AddLump (LUMP_MODELS, dmodels, nummodels*sizeof(dmodel_t));
	AddLump (LUMP_AREAS, dareas, numareas*sizeof(darea_t));
	AddLump (LUMP_AREAPORTALS, dareaportals, numareaportals*sizeof(dareaportal_t));

	AddLump (LUMP_LIGHTING, dlightdataLDR, LUMP_LIGHTING_VERSION);
	AddLump (LUMP_LIGHTING_HDR, dlightdataHDR, LUMP_LIGHTING_VERSION);
	AddLump (LUMP_VISIBILITY, dvisdata, visdatasize);
	AddLump (LUMP_ENTITIES, dentdata);
	AddLump (LUMP_WORLDLIGHTS, dworldlightsLDR, numworldlightsLDR*sizeof(dworldlight_t));
	AddLump (LUMP_WORLDLIGHTS_HDR, dworldlightsHDR, numworldlightsHDR*sizeof(dworldlight_t));
	AddLump (LUMP_LEAFWATERDATA, dleafwaterdata, numleafwaterdata*sizeof(dleafwaterdata_t));

	AddOcclusionLump();

	dflagslump_t flags_lump;

	flags_lump.m_LevelFlags = g_LevelFlags;
	AddLump( LUMP_MAP_FLAGS, &flags_lump, sizeof( flags_lump) );

	// NOTE: This is just for debugging, so it is disabled in release maps
#if 0
	// add the vis portals to the BSP for visualization
	AddLump (LUMP_PORTALS, dportals, numportals*sizeof(dportal_t));
	AddLump (LUMP_CLUSTERS, dclusters, numclusters*sizeof(dcluster_t));
	AddLump (LUMP_PORTALVERTS, dportalverts, numportalverts*sizeof(unsigned short));
	AddLump (LUMP_CLUSTERPORTALS, dclusterportals, numclusterportals*sizeof(unsigned short));
#endif

	AddLump (LUMP_CLIPPORTALVERTS, g_ClipPortalVerts, g_nClipPortalVerts*sizeof(g_ClipPortalVerts[0]));
	AddLump (LUMP_CUBEMAPS, g_CubemapSamples, g_nCubemapSamples * sizeof( g_CubemapSamples[0] ) );
	AddLump (LUMP_TEXDATA_STRING_DATA, g_TexDataStringData );
	AddLump (LUMP_TEXDATA_STRING_TABLE, g_TexDataStringTable );
	AddLump (LUMP_OVERLAYS, g_Overlays, g_nOverlayCount * sizeof( g_Overlays[0] ) );
	AddLump (LUMP_WATEROVERLAYS, g_WaterOverlays, g_nWaterOverlayCount * sizeof( g_WaterOverlays[0] ) );

	if ( g_pPhysCollide )
	{
		AddLump (LUMP_PHYSCOLLIDE, g_pPhysCollide, g_PhysCollideSize);
	}

	AddLump (LUMP_VERTNORMALS, g_vertnormals, g_numvertnormals * sizeof( g_vertnormals[0] ) );
	AddLump (LUMP_VERTNORMALINDICES, g_vertnormalindices, g_numvertnormalindices * sizeof( g_vertnormalindices[0] ) );

	AddLump (LUMP_LEAFMINDISTTOWATER, g_LeafMinDistToWater, numleafs * sizeof( g_LeafMinDistToWater[0] ) );

	AddLump (LUMP_LIGHTMAPPAGES, g_dLightmapPages, LUMP_LIGHTMAPPAGES_VERSION);
	AddLump (LUMP_LIGHTMAPPAGEINFOS, g_dLightmapPageInfos, LUMP_LIGHTMAPPAGEINFOS_VERSION);

	AddGameLumps();

	/*
	{
		GetPakFile()->AddFileToPack( "cfg/config.cfg", "d:\\tf2\\tf2\\cfg\\config.cfg" );
	}
	*/

	// Write pakfile lump to disk
	if( xzpLumpFilename )
	{
		WriteXZPPakFileLump( xzpLumpFilename );
	}
	else
	{
		WritePakFileLump();
	}

	// NOTE: Do NOT call AddLump after Lumps_Write() it writes all un-Added lumps
	Lumps_Write();	// write any additional lumps

	g_pFileSystem->Seek (wadfile, 0, FILESYSTEM_SEEK_HEAD);
	SafeWrite (wadfile, header, sizeof(dheader_t));
	g_pFileSystem->Close (wadfile);
}

// Generate the next clear lump filename for the bsp file
bool GenerateNextLumpFileName( const char *bspfilename, char *lumpfilename, int buffsize )
{
	for (int i = 0; i < MAX_LUMPFILES; i++)
	{
		GenerateLumpFileName( bspfilename, lumpfilename, buffsize, i );
	
		if ( !g_pFileSystem->FileExists( lumpfilename ) )
			return true;
	}

	return false;
}

void WriteLumpToFile( char *filename, int lump )
{
	if ( !HasLump(lump) )
		return;

	char lumppre[MAX_PATH];	
	if ( !GenerateNextLumpFileName( filename, lumppre, MAX_PATH ) )
	{
		Warning( "Failed to find valid lump filename for bsp %s.\n", filename );
		return;
	}

	// Open the file
	FileHandle_t lumpfile = g_pFileSystem->Open(lumppre, "wb");
	if ( !lumpfile )
	{
		Error ("Error opening %s! (Check for write enable)\n",filename);
		return;
	}

	int ofs = header->lumps[lump].fileofs;
	int length = header->lumps[lump].filelen;

	// Write the header
	lumpfileheader_t lumpHeader;
	lumpHeader.lumpID = lump;
	lumpHeader.lumpVersion = LumpVersion(lump);
	lumpHeader.lumpLength = length;
	lumpHeader.mapRevision = LittleLong( g_MapRevision );
	lumpHeader.lumpOffset = sizeof(lumpfileheader_t);	// Lump starts after the header
	SafeWrite (lumpfile, &lumpHeader, sizeof(lumpfileheader_t));

	// Write the lump
	SafeWrite (lumpfile, (byte *)header + ofs, length);
}

//============================================================================
#define ENTRIES(a)		(sizeof(a)/sizeof(*(a)))
#define ENTRYSIZE(a)	(sizeof(*(a)))

int ArrayUsage( char *szItem, int items, int maxitems, int itemsize )
{
	float	percentage = maxitems ? items * 100.0 / maxitems : 0.0;

    Msg("%-17.17s %8i/%-8i %8i/%-8i (%4.1f%%) ", 
		   szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage );
	if ( percentage > 80.0 )
		Msg( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		Msg( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		Msg( "SIZE OVERFLOW!!!\n" );
	else
		Msg( "\n" );
	return items * itemsize;
}

int GlobUsage( char *szItem, int itemstorage, int maxstorage )
{
	float	percentage = maxstorage ? itemstorage * 100.0 / maxstorage : 0.0;
    Msg("%-17.17s     [variable]    %8i/%-8i (%4.1f%%) ", 
		   szItem, itemstorage, maxstorage, percentage );
	if ( percentage > 80.0 )
		Msg( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		Msg( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		Msg( "SIZE OVERFLOW!!!\n" );
	else
		Msg( "\n" );
	return itemstorage;
}

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void PrintBSPFileSizes (void)
{
	int	totalmemory = 0;

//	if (!num_entities)
//		ParseEntities ();

	Msg("\n");
	Msg( "%-17s %16s %16s %9s \n", "Object names", "Objects/Maxobjs", "Memory / Maxmem", "Fullness" );
	Msg( "%-17s %16s %16s %9s \n",  "------------", "---------------", "---------------", "--------" );

	totalmemory += ArrayUsage( "models",		nummodels,		ENTRIES(dmodels),		ENTRYSIZE(dmodels) );
	totalmemory += ArrayUsage( "brushes",		numbrushes,		ENTRIES(dbrushes),		ENTRYSIZE(dbrushes) );
	totalmemory += ArrayUsage( "brushsides",	numbrushsides,	ENTRIES(dbrushsides),	ENTRYSIZE(dbrushsides) );
	totalmemory += ArrayUsage( "planes",		numplanes,		ENTRIES(dplanes),		ENTRYSIZE(dplanes) );
	totalmemory += ArrayUsage( "vertexes",		numvertexes,	ENTRIES(dvertexes),		ENTRYSIZE(dvertexes) );
	totalmemory += ArrayUsage( "nodes",			numnodes,		ENTRIES(dnodes),		ENTRYSIZE(dnodes) );
	totalmemory += ArrayUsage( "texinfos",		texinfo.Count(),MAX_MAP_TEXINFO,		sizeof(texinfo_t) );
	totalmemory += ArrayUsage( "texdata",		numtexdata,		ENTRIES(dtexdata),		ENTRYSIZE(dtexdata) );
    
	totalmemory += ArrayUsage( "dispinfos",     g_dispinfo.Count(),			0,			sizeof( ddispinfo_t ) );
    totalmemory += ArrayUsage( "disp_verts",	g_DispVerts.Count(),		0,			sizeof( g_DispVerts[0] ) );
    totalmemory += ArrayUsage( "disp_tris",		g_DispTris.Count(),			0,			sizeof( g_DispTris[0] ) );
    totalmemory += ArrayUsage( "disp_lmsamples",g_DispLightmapSamplePositions.Count(),0,sizeof( g_DispLightmapSamplePositions[0] ) );
	
	totalmemory += ArrayUsage( "faces",			numfaces,		ENTRIES(dfaces),		ENTRYSIZE(dfaces) );
	totalmemory += ArrayUsage( "hdr faces",     numfaces_hdr,	ENTRIES(dfaces_hdr),	ENTRYSIZE(dfaces_hdr) );
    totalmemory += ArrayUsage( "origfaces",     numorigfaces,   ENTRIES(dorigfaces),    ENTRYSIZE(dorigfaces) );    // original faces
	totalmemory += ArrayUsage( "leaves",		numleafs,		ENTRIES(dleafs),		ENTRYSIZE(dleafs) );
	totalmemory += ArrayUsage( "leaffaces",		numleaffaces,	ENTRIES(dleaffaces),	ENTRYSIZE(dleaffaces) );
	totalmemory += ArrayUsage( "leafbrushes",	numleafbrushes,	ENTRIES(dleafbrushes),	ENTRYSIZE(dleafbrushes) );
	totalmemory += ArrayUsage( "areas",	numareas,	ENTRIES(dareas),	ENTRYSIZE(dareas) );
	totalmemory += ArrayUsage( "surfedges",		numsurfedges,	ENTRIES(dsurfedges),	ENTRYSIZE(dsurfedges) );
	totalmemory += ArrayUsage( "edges",			numedges,		ENTRIES(dedges),		ENTRYSIZE(dedges) );
	totalmemory += ArrayUsage( "LDR worldlights",	numworldlightsLDR,	ENTRIES(dworldlightsLDR),	ENTRYSIZE(dworldlightsLDR) );
	totalmemory += ArrayUsage( "HDR worldlights",	numworldlightsHDR,	ENTRIES(dworldlightsHDR),	ENTRYSIZE(dworldlightsHDR) );
	totalmemory += ArrayUsage( "waterstrips",	g_numprimitives,ENTRIES(g_primitives),	ENTRYSIZE(g_primitives) );
	totalmemory += ArrayUsage( "waterverts",	g_numprimverts,	ENTRIES(g_primverts),	ENTRYSIZE(g_primverts) );
	totalmemory += ArrayUsage( "waterindices",	g_numprimindices,ENTRIES(g_primindices),ENTRYSIZE(g_primindices) );
	totalmemory += ArrayUsage( "cubemapsamples", g_nCubemapSamples,ENTRIES(g_CubemapSamples),ENTRYSIZE(g_CubemapSamples) );
	totalmemory += ArrayUsage( "overlays",      g_nOverlayCount, ENTRIES(g_Overlays),   ENTRYSIZE(g_Overlays) );
	
	totalmemory += GlobUsage( "LDR lightdata",		dlightdataLDR.Count(),	0 );
	totalmemory += GlobUsage( "HDR lightdata",	dlightdataHDR.Count(),	0 );
	totalmemory += GlobUsage( "visdata",		visdatasize,	sizeof(dvisdata) );
	totalmemory += GlobUsage( "entdata",		dentdata.Count(), 384*1024 );	// goal is <384K

	totalmemory += ArrayUsage( "LDR leaf ambient lighting", g_LeafAmbientLightingLDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientLightingLDR[0] ) );
	totalmemory += ArrayUsage( "HDR leaf ambient lighting", g_LeafAmbientLightingHDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientLightingHDR[0] ) );

	totalmemory += ArrayUsage( "occluders",     g_OccluderData.Count(),	0, sizeof( g_OccluderData[0] ) );
    totalmemory += ArrayUsage( "occluder polygons",	g_OccluderPolyData.Count(),	0, sizeof( g_OccluderPolyData[0] ) );
    totalmemory += ArrayUsage( "occluder vert ind",g_OccluderVertexIndices.Count(),0, sizeof( g_OccluderVertexIndices[0] ) );

	GameLumpHandle_t h = GetGameLumpHandle( GAMELUMP_DETAIL_PROPS );
	if (h != InvalidGameLump())
		totalmemory += GlobUsage( "detail props",	1,	GameLumpSize(h) );
	h = GetGameLumpHandle( GAMELUMP_DETAIL_PROP_LIGHTING );
	if (h != InvalidGameLump())
		totalmemory += GlobUsage( "dtl prp lght",	1,	GameLumpSize(h) );
	h = GetGameLumpHandle( GAMELUMP_DETAIL_PROP_LIGHTING_HDR );
	if (h != InvalidGameLump())
		totalmemory += GlobUsage( "HDR dtl prp lght",	1,	GameLumpSize(h) );
	h = GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	if (h != InvalidGameLump())
		totalmemory += GlobUsage( "static props",	1,	GameLumpSize(h) );

	totalmemory += GlobUsage( "pakfile",		GetPakFile()->EstimateSize(), 0 );

	Msg( "\nLevel flags = %x\n", g_LevelFlags );

	Msg( "\nWin32 Specific Data:\n" );

	int totalWin32Specificmemory = 0;
	// HACKHACK: Set physics limit at 4MB, in reality this is totally dynamic
	totalWin32Specificmemory += GlobUsage( "physics",		g_PhysCollideSize, 4*1024*1024 );
	Msg( "==== Total Win32 BSP file data space used: %d bytes ====\n", totalmemory + totalWin32Specificmemory );

#ifdef _XBOX
	Msg( "\nXBox Specific Data:\n" );
	int totalXBoxSpecificmemory = 0;
	totalXBoxSpecificmemory += GlobUsage( "lightmap pages", g_dLightmapPages.Count()*sizeof(dlightmappage_t), 0 );
	totalXBoxSpecificmemory += GlobUsage( "lightmap infos", g_dLightmapPageInfos.Count()*sizeof(dlightmappageinfo_t), 0 );
	Msg( "==== Total XBox BSP file data space used: %d bytes ====\n", totalmemory + totalXBoxSpecificmemory );
#endif // _XBOX

	Msg( "\n" );

	int triangleCount = 0;

	for ( int i = 0; i < numfaces; i++ )
	{
		// face tris = numedges - 2
		triangleCount += dfaces[i].numedges - 2;
	}
	Msg("Total triangle count: %d\n", triangleCount );

	// UNDONE: 
	// areaportals, portals, texdata, clusters, worldlights, portalverts
}

/*
=============
PrintBSPPackDirectory

Dumps a list of files stored in the bsp pack.
=============
*/
void PrintBSPPackDirectory( void )
{
	GetPakFile()->PrintDirectory();	
}


//============================================

int			num_entities;
entity_t	entities[MAX_MAP_ENTITIES];

void StripTrailing (char *e)
{
	char	*s;

	s = e + strlen(e)-1;
	while (s >= e && *s <= 32)
	{
		*s = 0;
		s--;
	}
}

/*
=================
ParseEpair
=================
*/
epair_t *ParseEpair (void)
{
	epair_t	*e;

	e = (epair_t*)malloc (sizeof(epair_t));
	memset (e, 0, sizeof(epair_t));
	
	if (strlen(token) >= MAX_KEY-1)
		Error ("ParseEpar: token too long");
	e->key = copystring(token);
	GetToken (false);
	if (strlen(token) >= MAX_VALUE-1)
		Error ("ParseEpar: token too long");
	e->value = copystring(token);

	// strip trailing spaces
	StripTrailing (e->key);
	StripTrailing (e->value);

	return e;
}


/*
================
ParseEntity
================
*/
qboolean	ParseEntity (void)
{
	epair_t		*e;
	entity_t	*mapent;

	if (!GetToken (true))
		return false;

	if (strcmp (token, "{") )
		Error ("ParseEntity: { not found");
	
	if (num_entities == MAX_MAP_ENTITIES)
		Error ("num_entities == MAX_MAP_ENTITIES");

	mapent = &entities[num_entities];
	num_entities++;

	do
	{
		if (!GetToken (true))
			Error ("ParseEntity: EOF without closing brace");
		if (!strcmp (token, "}") )
			break;
		e = ParseEpair ();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while (1);
	
	return true;
}

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void ParseEntities (void)
{
	num_entities = 0;
	ParseFromMemory (dentdata.Base(), dentdata.Count());

	while (ParseEntity ())
	{
	}	
}


/*
================
UnparseEntities

Generates the dentdata string from all the entities
================
*/
void UnparseEntities (void)
{
	epair_t	*ep;
	char	line[2048];
	int		i;
	char	key[1024], value[1024];

	CUtlBuffer buffer(0,0,true);
	buffer.EnsureCapacity( 256 * 1024 );
	
	for (i=0 ; i<num_entities ; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;	// ent got removed
		
		buffer.PutString( "{\n" );
				
		for (ep = entities[i].epairs ; ep ; ep=ep->next)
		{
			strcpy (key, ep->key);
			StripTrailing (key);
			strcpy (value, ep->value);
			StripTrailing (value);
				
			sprintf(line, "\"%s\" \"%s\"\n", key, value);
			buffer.PutString( line );
		}
		buffer.PutString("}\n");
	}
	int entdatasize = buffer.TellPut()+1;

	dentdata.SetSize( entdatasize );
	memcpy( dentdata.Base(), buffer.Base(), entdatasize-1 );
	dentdata[entdatasize-1] = 0;
}

void PrintEntity (entity_t *ent)
{
	epair_t	*ep;
	
	Msg ("------- entity %p -------\n", ent);
	for (ep=ent->epairs ; ep ; ep=ep->next)
	{
		Msg ("%s = %s\n", ep->key, ep->value);
	}

}

void SetKeyValue(entity_t *ent, const char *key, const char *value)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
		{
			free (ep->value);
			ep->value = copystring(value);
			return;
		}
	ep = (epair_t*)malloc (sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = copystring(key);
	ep->value = copystring(value);
}

char 	*ValueForKey (entity_t *ent, char *key)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
			return ep->value;
	return "";
}

vec_t	FloatForKey (entity_t *ent, char *key)
{
	char	*k;
	
	k = ValueForKey (ent, key);
	return atof(k);
}

int		IntForKey (entity_t *ent, char *key)
{
	char	*k;
	
	k = ValueForKey (ent, key);
	return atol(k);
}

void 	GetVectorForKey (entity_t *ent, char *key, Vector& vec)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
}

void 	GetVector2DForKey (entity_t *ent, char *key, Vector2D& vec)
{
	char	*k;
	double	v1, v2;

	k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = 0;
	sscanf (k, "%lf %lf", &v1, &v2);
	vec[0] = v1;
	vec[1] = v2;
}

void 	GetAnglesForKey (entity_t *ent, char *key, QAngle& angle)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	angle[0] = v1;
	angle[1] = v2;
	angle[2] = v3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BuildFaceCalcWindingData( dface_t *pFace, int *points )
{
	for( int i = 0; i < pFace->numedges; i++ )
	{
		int eIndex = dsurfedges[pFace->firstedge+i];
		if( eIndex < 0 )
		{
			points[i] = dedges[-eIndex].v[1];
		}
		else
		{
			points[i] = dedges[eIndex].v[0];
		}
	}
}


void TriStripToTriList( 
	unsigned short const *pTriStripIndices,
	int nTriStripIndices,
	unsigned short **pTriListIndices,
	int *pnTriListIndices )
{
	int nMaxTriListIndices = (nTriStripIndices - 2) * 3;
	*pTriListIndices = new unsigned short[ nMaxTriListIndices ];
	*pnTriListIndices = 0;

	for( int i=0; i < nTriStripIndices - 2; i++ )
	{
		if( pTriStripIndices[i]   == pTriStripIndices[i+1] || 
			pTriStripIndices[i]   == pTriStripIndices[i+2] ||
			pTriStripIndices[i+1] == pTriStripIndices[i+2] )
		{
		}
		else
		{
			// Flip odd numbered tris..
			if( i & 1 )
			{
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+2];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+1];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i];
			}
			else
			{
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+1];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+2];
			}
		}
	}
}


void CalcTextureCoordsAtPoints(
	float const texelsPerWorldUnits[2][4],
	int const subtractOffset[2],
	Vector const *pPoints,
	int const nPoints,
	Vector2D *pCoords )
{
	for( int i=0; i < nPoints; i++ )
	{
		for( int iCoord=0; iCoord < 2; iCoord++ )
		{
			float *pDestCoord = &pCoords[i][iCoord];

			*pDestCoord = 0;
			for( int iDot=0; iDot < 3; iDot++ )
				*pDestCoord += pPoints[i][iDot] * texelsPerWorldUnits[iCoord][iDot];

			*pDestCoord += texelsPerWorldUnits[iCoord][3];
			*pDestCoord -= subtractOffset[iCoord];
		}
	}
}


/*
================
CalcFaceExtents

Fills in s->texmins[] and s->texsize[]
================
*/
void CalcFaceExtents(dface_t *s, int lightmapTextureMinsInLuxels[2], int lightmapTextureSizeInLuxels[2])
{
	vec_t	    mins[2], maxs[2], val=0;
	int		    i,j, e=0;
	dvertex_t	*v=NULL;
	texinfo_t	*tex=NULL;
	
	mins[0] = mins[1] = 1e24;
	maxs[0] = maxs[1] = -1e24;

	tex = &texinfo[s->texinfo];
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = dsurfedges[s->firstedge+i];
		if (e >= 0)
			v = dvertexes + dedges[e].v[0];
		else
			v = dvertexes + dedges[-e].v[1];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->point[0] * tex->lightmapVecsLuxelsPerWorldUnits[j][0] + 
				  v->point[1] * tex->lightmapVecsLuxelsPerWorldUnits[j][1] + 
				  v->point[2] * tex->lightmapVecsLuxelsPerWorldUnits[j][2] + 
				  tex->lightmapVecsLuxelsPerWorldUnits[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	int nMaxLightmapDim = (s->dispinfo == -1) ? MAX_LIGHTMAP_DIM_WITHOUT_BORDER : MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER;
	for (i=0 ; i<2 ; i++)
	{	
		mins[i] = ( float )floor( mins[i] );
		maxs[i] = ( float )ceil( maxs[i] );

		lightmapTextureMinsInLuxels[i] = ( int )mins[i];
		lightmapTextureSizeInLuxels[i] = ( int )( maxs[i] - mins[i] );
		if( lightmapTextureSizeInLuxels[i] > nMaxLightmapDim + 1 )
		{
			Vector point = vec3_origin;
			for (int j=0 ; j<s->numedges ; j++)
			{
				e = dsurfedges[s->firstedge+j];
				v = (e<0)?dvertexes + dedges[-e].v[1] : dvertexes + dedges[e].v[0];
				point += v->point;
				Warning( "Bad surface extents point: %f %f %f\n", v->point.x, v->point.y, v->point.z );
			}
			point *= 1.0f/s->numedges;
			Error( "Bad surface extents - surface is too big to have a lightmap\n\tmaterial %s around point (%.1f %.1f %.1f)\n\t(dimension: %d, %d>%d)\n", 
				TexDataStringTable_GetString( dtexdata[texinfo[s->texinfo].texdata].nameStringTableID ), 
				point.x, point.y, point.z,
				( int )i,
				( int )lightmapTextureSizeInLuxels[i],
				( int )( nMaxLightmapDim + 1 )
				);
		}
	}
}


void UpdateAllFaceLightmapExtents()
{
	for( int i=0; i < numfaces; i++ )
	{
		dface_t *pFace = &dfaces[i];

		if ( texinfo[pFace->texinfo].flags & (SURF_SKY|SURF_NOLIGHT) )
			continue;		// non-lit texture

		CalcFaceExtents( pFace, pFace->m_LightmapTextureMinsInLuxels, pFace->m_LightmapTextureSizeInLuxels );
	}
}


//-----------------------------------------------------------------------------
//
// Helper class to iterate over leaves, used by tools
//
//-----------------------------------------------------------------------------

#define TEST_EPSILON	(0.03125)


class CToolBSPTree : public ISpatialQuery
{
public:
	// Returns the number of leaves
	int LeafCount() const;

	// Enumerates the leaves along a ray, box, etc.
	bool EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context );
};


//-----------------------------------------------------------------------------
// Returns the number of leaves
//-----------------------------------------------------------------------------

int CToolBSPTree::LeafCount() const
{
	return numleafs;
}


//-----------------------------------------------------------------------------
// Enumerates the leaves at a point
//-----------------------------------------------------------------------------

bool CToolBSPTree::EnumerateLeavesAtPoint( Vector const& pt, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	int node = 0;
	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if (DotProduct( pPlane->normal, pt ) <= pPlane->dist)
		{
			node = pNode->children[1];
		}
		else
		{
			node = pNode->children[0];
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}


//-----------------------------------------------------------------------------
// Enumerates the leaves in a box
//-----------------------------------------------------------------------------

static bool EnumerateLeavesInBox_R( int node, Vector const& mins, 
				Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context )
{
	Vector cornermin, cornermax;

	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		// Arbitrary split plane here
		for (int i = 0; i < 3; ++i)
		{
			if (pPlane->normal[i] >= 0)
			{
				cornermin[i] = mins[i];
				cornermax[i] = maxs[i];
			}
			else
			{
				cornermin[i] = maxs[i];
				cornermax[i] = mins[i];
			}
		}

		if ( (DotProduct( pPlane->normal, cornermax ) - pPlane->dist) <= -TEST_EPSILON )
		{
			node = pNode->children[1];
		}
		else if ( (DotProduct( pPlane->normal, cornermin ) - pPlane->dist) >= TEST_EPSILON )
		{
			node = pNode->children[0];
		}
		else
		{
			if (!EnumerateLeavesInBox_R( pNode->children[0], mins, maxs, pEnum, context ))
			{
				return false;
			}

			return EnumerateLeavesInBox_R( pNode->children[1], mins, maxs, pEnum, context );
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, 
									ISpatialLeafEnumerator* pEnum, int context )
{
	return EnumerateLeavesInBox_R( 0, mins, maxs, pEnum, context );
}

//-----------------------------------------------------------------------------
// Enumerate leaves within a sphere
//-----------------------------------------------------------------------------

static bool EnumerateLeavesInSphere_R( int node, Vector const& origin, 
				float radius, ISpatialLeafEnumerator* pEnum, int context )
{
	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if (DotProduct( pPlane->normal, origin ) + radius - pPlane->dist <= -TEST_EPSILON )
		{
			node = pNode->children[1];
		}
		else if (DotProduct( pPlane->normal, origin ) - radius - pPlane->dist >= TEST_EPSILON )
		{
			node = pNode->children[0];
		}
		else
		{
			if (!EnumerateLeavesInSphere_R( pNode->children[0], 
					origin, radius, pEnum, context ))
			{
				return false;
			}

			return EnumerateLeavesInSphere_R( pNode->children[1],
				origin, radius, pEnum, context );
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context )
{
	return EnumerateLeavesInSphere_R( 0, center, radius, pEnum, context );
}


//-----------------------------------------------------------------------------
// Enumerate leaves along a ray
//-----------------------------------------------------------------------------

static bool EnumerateLeavesAlongRay_R( int node, Ray_t const& ray, 
	Vector const& start, Vector const& end, ISpatialLeafEnumerator* pEnum, int context )
{
	float front,back;

	while (node >= 0)
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if ( pPlane->type <= PLANE_Z )
		{
			front = start[pPlane->type] - pPlane->dist;
			back = end[pPlane->type] - pPlane->dist;
		}
		else
		{
			front = DotProduct(start, pPlane->normal) - pPlane->dist;
			back = DotProduct(end, pPlane->normal) - pPlane->dist;
		}

		if (front <= -TEST_EPSILON && back <= -TEST_EPSILON)
		{
			node = pNode->children[1];
		}
		else if (front >= TEST_EPSILON && back >= TEST_EPSILON)
		{
			node = pNode->children[0];
		}
		else
		{
			// test the front side first
			bool side = front < 0;

			// Compute intersection point based on the original ray
			float splitfrac;
			float denom = DotProduct( ray.m_Delta, pPlane->normal );
			if ( denom == 0.0f )
			{
				splitfrac = 1.0f;
			}
			else
			{
				splitfrac = (	pPlane->dist - DotProduct( ray.m_Start, pPlane->normal ) ) / denom;
				if (splitfrac < 0)
					splitfrac = 0;
				else if (splitfrac > 1)
					splitfrac = 1;
			}

			// Compute the split point
			Vector split;
			VectorMA( ray.m_Start, splitfrac, ray.m_Delta, split );

			bool r = EnumerateLeavesAlongRay_R (pNode->children[side], ray, start, split, pEnum, context );
			if (!r)
				return r;
			return EnumerateLeavesAlongRay_R (pNode->children[!side], ray, split, end, pEnum, context);
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context )
{
	if (!ray.m_IsSwept)
	{
		Vector mins, maxs;
		VectorAdd( ray.m_Start, ray.m_Extents, maxs );
		VectorSubtract( ray.m_Start, ray.m_Extents, mins );

		return EnumerateLeavesInBox_R( 0, mins, maxs, pEnum, context );
	}

	// FIXME: Extruded ray not implemented yet
	Assert( ray.m_IsRay );

	Vector end;
	VectorAdd( ray.m_Start, ray.m_Delta, end );
	return EnumerateLeavesAlongRay_R( 0, ray, ray.m_Start, end, pEnum, context );
}


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

ISpatialQuery* ToolBSPTree()
{
	static CToolBSPTree s_ToolBSPTree;
	return &s_ToolBSPTree;
}



//-----------------------------------------------------------------------------
// Enumerates nodes in front to back order...
//-----------------------------------------------------------------------------

// FIXME: Do we want this in the IBSPTree interface?

static bool EnumerateNodesAlongRay_R( int node, Ray_t const& ray, float start, float end,
	IBSPNodeEnumerator* pEnum, int context )
{
	float front, back;
	float startDotN, deltaDotN;

	while (node >= 0)
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if ( pPlane->type <= PLANE_Z )
		{
			startDotN = ray.m_Start[pPlane->type];
			deltaDotN = ray.m_Delta[pPlane->type];
		}
		else
		{
			startDotN = DotProduct( ray.m_Start, pPlane->normal );
			deltaDotN = DotProduct( ray.m_Delta, pPlane->normal );
		}

		front = startDotN + start * deltaDotN - pPlane->dist;
		back = startDotN + end * deltaDotN - pPlane->dist;

		if (front <= -TEST_EPSILON && back <= -TEST_EPSILON)
		{
			node = pNode->children[1];
		}
		else if (front >= TEST_EPSILON && back >= TEST_EPSILON)
		{
			node = pNode->children[0];
		}
		else
		{
			// test the front side first
			bool side = front < 0;

			// Compute intersection point based on the original ray
			float splitfrac;
			if ( deltaDotN == 0.0f )
			{
				splitfrac = 1.0f;
			}
			else
			{
				splitfrac = ( pPlane->dist - startDotN ) / deltaDotN;
				if (splitfrac < 0.0f)
					splitfrac = 0.0f;
				else if (splitfrac > 1.0f)
					splitfrac = 1.0f;
			}

			bool r = EnumerateNodesAlongRay_R (pNode->children[side], ray, start, splitfrac, pEnum, context );
			if (!r)
				return r;

			// Visit the node...
			if (!pEnum->EnumerateNode( node, ray, splitfrac, context ))
				return false;

			return EnumerateNodesAlongRay_R (pNode->children[!side], ray, splitfrac, end, pEnum, context);
		}
	}

	// Visit the leaf...
	return pEnum->EnumerateLeaf( - node - 1, ray, start, end, context );
}


bool EnumerateNodesAlongRay( Ray_t const& ray, IBSPNodeEnumerator* pEnum, int context )
{
	Vector end;
	VectorAdd( ray.m_Start, ray.m_Delta, end );
	return EnumerateNodesAlongRay_R( 0, ray, 0.0f, 1.0f, pEnum, context );
}


//-----------------------------------------------------------------------------
// Helps us find all leaves associated with a particular cluster
//-----------------------------------------------------------------------------
CUtlVector<clusterlist_t> g_ClusterLeaves;

void BuildClusterTable( void )
{
	int i, j;
	int leafCount;
	int	leafList[MAX_MAP_LEAFS];

	g_ClusterLeaves.SetCount( dvis->numclusters );
	for ( i = 0; i < dvis->numclusters; i++ )
	{
		leafCount = 0;
		for ( j = 0; j < numleafs; j++ )
		{
			if ( dleafs[j].cluster == i )
			{
				leafList[ leafCount ] = j;
				leafCount++;
			}
		}

		g_ClusterLeaves[i].leafCount = leafCount;
		if ( leafCount )
		{
			g_ClusterLeaves[i].leafs.SetCount( leafCount );
			memcpy( g_ClusterLeaves[i].leafs.Base(), leafList, sizeof(int) * leafCount );
		}
	}
}

// There's a version of this in host.cpp!!!  Make sure that they match.
void GetPlatformMapPath( const char *pMapPath, char *pPlatformMapPath, int dxlevel, int maxLength )
{
	Q_StripExtension( pMapPath, pPlatformMapPath, maxLength );

//	if( dxlevel <= 60 )
//	{
//		Q_strncat( pPlatformMapPath, "_dx60", maxLength, COPY_ALL_CHARACTERS );
//	}

	Q_strncat( pPlatformMapPath, ".bsp", maxLength, COPY_ALL_CHARACTERS );
}

void SetHDRMode( bool bHDR )
{
	g_bHDR = bHDR;
	if( bHDR )
	{
		pdlightdata = &dlightdataHDR;		
		g_pLeafAmbientLighting = &g_LeafAmbientLightingHDR;
		pNumworldlights = &numworldlightsHDR;
		dworldlights = dworldlightsHDR;
#ifdef VRAD
		extern void VRadDetailProps_SetHDRMode( bool bHDR );
		VRadDetailProps_SetHDRMode( bHDR );
#endif
	}
	else
	{
		pdlightdata = &dlightdataLDR;		
		g_pLeafAmbientLighting = &g_LeafAmbientLightingLDR;
		pNumworldlights = &numworldlightsLDR;
		dworldlights = dworldlightsLDR;
#ifdef VRAD
		extern void VRadDetailProps_SetHDRMode( bool bHDR );
		VRadDetailProps_SetHDRMode( bHDR );
#endif
	}
}
