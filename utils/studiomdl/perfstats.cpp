//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <stdlib.h>
#include <tier0/dbg.h>
#include "interface.h"
#include "istudiorender.h"
#include "matsys.h"
#include "studio.h"
#include "optimize.h"
#include "cmdlib.h"
#include "studiomdl.h"

extern void MdlError( char const *pMsg, ... );

static CSysModule *g_pStudioRenderModule = NULL;
static IStudioRender *g_pStudioRender = NULL;
static void UpdateStudioRenderConfig( void );
static StudioRenderConfig_t s_StudioRenderConfig;

class CStudioDataCache : public CBaseAppSystem<IStudioDataCache>
{
public:
	bool VerifyHeaders( studiohdr_t *pStudioHdr );
	vertexFileHeader_t *CacheVertexData( studiohdr_t *pStudioHdr );
};

static CStudioDataCache	g_StudioDataCache;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CStudioDataCache, IStudioDataCache, STUDIO_DATA_CACHE_INTERFACE_VERSION, g_StudioDataCache );


/*
=================
VerifyHeaders

Minimal presence and header validation, no data loads
Return true if successful, false otherwise.
=================
*/
bool CStudioDataCache::VerifyHeaders( studiohdr_t *pStudioHdr )
{
	// default valid
	return true;
}

/*
=================
CacheVertexData

Cache model's specified dynamic data
=================
*/
vertexFileHeader_t *CStudioDataCache::CacheVertexData( studiohdr_t *pStudioHdr )
{
	// minimal implementation - return persisted data
	return (vertexFileHeader_t*)pStudioHdr->pVertexBase;
}

void InitStudioRender( void )
{
	if ( g_pStudioRenderModule )
		return;
	Assert( g_MatSysFactory );

	g_pStudioRenderModule = g_pFullFileSystem->LoadModule( "StudioRender.dll" );
	if( !g_pStudioRenderModule )
	{
		MdlError( "Can't load StudioRender.dll\n" );
	}
	CreateInterfaceFn studioRenderFactory = Sys_GetFactory( g_pStudioRenderModule );
	if (!studioRenderFactory )
	{
		MdlError( "Can't get factory for StudioRender.dll\n" );
	}
	g_pStudioRender = ( IStudioRender * )studioRenderFactory( STUDIO_RENDER_INTERFACE_VERSION, NULL );
	if (!g_pStudioRender)
	{
		MdlError( "Unable to init studio render system version %s\n", STUDIO_RENDER_INTERFACE_VERSION );
	}

	g_pStudioRender->Init( g_MatSysFactory, g_ShaderAPIFactory, g_ShaderAPIFactory, Sys_GetFactoryThis() );
	UpdateStudioRenderConfig();
}

static void UpdateStudioRenderConfig( void )
{
	memset( &s_StudioRenderConfig, 0, sizeof(s_StudioRenderConfig) );

	s_StudioRenderConfig.eyeGloss = true;
	s_StudioRenderConfig.bEyeMove = true;
	s_StudioRenderConfig.fEyeShiftX = 0.0f;
	s_StudioRenderConfig.fEyeShiftY = 0.0f;
	s_StudioRenderConfig.fEyeShiftZ = 0.0f;
	s_StudioRenderConfig.fEyeSize = 10.0f;
	s_StudioRenderConfig.bSoftwareSkin = false;
	s_StudioRenderConfig.bNoHardware = false;
	s_StudioRenderConfig.bNoSoftware = false;
	s_StudioRenderConfig.bTeeth = true;
	s_StudioRenderConfig.drawEntities = true;
	s_StudioRenderConfig.bFlex = true;
	s_StudioRenderConfig.bEyes = true;
	s_StudioRenderConfig.bWireframe = false;
	s_StudioRenderConfig.SetNormals( false );
	s_StudioRenderConfig.skin = 0;
	s_StudioRenderConfig.maxDecalsPerModel = 0;
	s_StudioRenderConfig.bWireframeDecals = false;
	s_StudioRenderConfig.fullbright = false;
	s_StudioRenderConfig.bSoftwareLighting = false;
	s_StudioRenderConfig.pConDPrintf = Warning;
	s_StudioRenderConfig.pConPrintf = Warning;
	s_StudioRenderConfig.bShowEnvCubemapOnly = false;
	g_pStudioRender->UpdateConfig( s_StudioRenderConfig );
}

void ShutdownStudioRender( void )
{
	if ( !g_pStudioRenderModule )
		return;

	if ( g_pStudioRender )
	{
		g_pStudioRender->Shutdown();
	}
	g_pStudioRender = NULL;

	g_pFullFileSystem->UnloadModule( g_pStudioRenderModule );
	g_pStudioRenderModule = NULL;
}


void SpewPerfStats( studiohdr_t *pStudioHdr, const char *pFilename )
{
	char							fileName[260];
	vertexFileHeader_t				*pNewVvdHdr;
	vertexFileHeader_t				*pVvdHdr;
	OptimizedModel::FileHeader_t	*pVtxHdr;
	DrawModelInfo_t					drawModelInfo;
	studiohwdata_t					studioHWData;
	int								vvdSize;
	const char						*prefix[] = {".dx80.vtx", ".dx90.vtx", ".sw.vtx", ".xbox.vtx"};

	if (!pStudioHdr->numbodyparts)
	{
		// no stats on these
		return;
	}

	// Need to load up StudioRender.dll to spew perf stats.
	InitStudioRender();

	// persist the vvd data
	Q_StripExtension( pFilename, fileName, sizeof( fileName ) );
	strcat( fileName, ".vvd" );

	if (FileExists( fileName ))
	{
		vvdSize = LoadFile( fileName, (void**)&pVvdHdr );
	}
	else
	{
		MdlError( "Could not open '%s'\n", fileName );
	}

	// validate header
	if (pVvdHdr->id != MODEL_VERTEX_FILE_ID)
	{
		MdlError( "Bad id for '%s' (got %d expected %d)\n", fileName, pVvdHdr->id, MODEL_VERTEX_FILE_ID);
	}
	if (pVvdHdr->version != MODEL_VERTEX_FILE_VERSION)
	{
		MdlError( "Bad version for '%s' (got %d expected %d)\n", fileName, pVvdHdr->version, MODEL_VERTEX_FILE_VERSION);
	}
	if (pVvdHdr->checksum != pStudioHdr->checksum)
	{
		MdlError( "Bad checksum for '%s' (got %d expected %d)\n", fileName, pVvdHdr->checksum, pStudioHdr->checksum);
	}

	if (pVvdHdr->numFixups)
	{
		// need to perform mesh relocation fixups
		// allocate a new copy
		pNewVvdHdr = (vertexFileHeader_t *)malloc( vvdSize );
		if (!pNewVvdHdr)
		{
			MdlError( "Error allocating %d bytes for Vertex File '%s'\n", vvdSize, fileName );
		}

		Studio_LoadVertexes( pVvdHdr, pNewVvdHdr, 0, true );

		// discard original
		free( pVvdHdr );

		pVvdHdr = pNewVvdHdr;
	}
	
	// iterate all ???.vtx files
	for (int j=0; j<sizeof(prefix)/sizeof(prefix[0]); j++)
	{
		// make vtx filename
		Q_StripExtension( pFilename, fileName, sizeof( fileName ) );
		strcat( fileName, prefix[j] );

		printf( "\n" );
		printf( "Performance Stats: %s\n", fileName );
		printf( "------------------\n" );

		// persist the vtx data
		if (FileExists(fileName))
		{
			LoadFile( fileName, (void**)&pVtxHdr );
		}
		else
		{
			MdlError( "Could not open '%s'\n", fileName );
		}

		// validate header
		if (pVtxHdr->version != OPTIMIZED_MODEL_FILE_VERSION)
		{
			MdlError( "Bad version for '%s' (got %d expected %d)\n", fileName, pVtxHdr->version, OPTIMIZED_MODEL_FILE_VERSION );
		}
		if (pVtxHdr->checkSum != pStudioHdr->checksum)
		{
			MdlError( "Bad checksum for '%s' (got %d expected %d)\n", fileName, pVtxHdr->checkSum, pStudioHdr->checksum );
		}

		// studio render will request these through cache interface
		pStudioHdr->pVertexBase = (void *)pVvdHdr;
		pStudioHdr->pIndexBase  = (void *)pVtxHdr;

		g_pStudioRender->LoadModel( pStudioHdr, pVtxHdr, &studioHWData );
		memset( &drawModelInfo, 0, sizeof( DrawModelInfo_t ) );
		drawModelInfo.m_pStudioHdr = pStudioHdr;
		drawModelInfo.m_pHardwareData = &studioHWData;	
		int i;
		for( i = studioHWData.m_RootLOD; i < studioHWData.m_NumLODs; i++ )
		{
			CUtlBuffer statsOutput( 0, 0, true /* text */ );
			printf( "LOD: %d\n", i );
			drawModelInfo.m_Lod = i;
			g_pStudioRender->GetPerfStats( drawModelInfo, &statsOutput );
			printf( "\tactual tris: %d\n", ( int )drawModelInfo.m_ActualTriCount );
			printf( "\ttexture memory bytes: %d\n", ( int )drawModelInfo.m_TextureMemoryBytes );
			printf( ( char * )statsOutput.Base() );
		}
		g_pStudioRender->UnloadModel( &studioHWData );
		free(pVtxHdr);
	}

	if (pVvdHdr)
		free(pVvdHdr);
}

