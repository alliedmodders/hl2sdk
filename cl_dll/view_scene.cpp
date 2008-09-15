//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Responsible for drawing the scene
//
// $NoKeywords: $
//===========================================================================//


#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "model_types.h"
#include "clientsideeffects.h"
#include "particlemgr.h"
#include "viewrender.h"
#include "iclientmode.h"
#include "voice_status.h"
#include "glow_overlay.h"
#include "materialsystem/imesh.h"
#include "materialsystem/ITexture.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "materialsystem/IColorCorrection.h"
#include "DetailObjectSystem.h"
#include "tier0/vprof.h"
#include "datacache/imdlcache.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "vstdlib/icommandline.h"

#include "view_scene.h"
#include "particles_ez.h"
#include "engine/IStaticPropMgr.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "c_pixel_visibility.h"
#include "ClientEffectPrecacheSystem.h"
#include "c_rope.h"
#include "c_effects.h"
#include "smoke_fog_overlay.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "datacache/imdlcache.h"
#include "ScreenSpaceEffects.h"

#if defined( HL2_CLIENT_DLL ) || defined( CSTRIKE_DLL )
#define USE_MONITORS
#endif

// GR
#include "rendertexture.h"
#ifdef _XBOX
#include "tier0/mem.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse );

static ConVar cl_drawmonitors( "cl_drawmonitors", "1" );
static ConVar cl_overdraw_test( "cl_overdraw_test", "0", FCVAR_CHEAT | FCVAR_NEVER_AS_STRING );
static ConVar r_eyewaterepsilon( "r_eyewaterepsilon", "7.0f", FCVAR_CHEAT );

static void SetClearColorToFogColor( unsigned char ucFogColor[3] )
{
	materials->GetFogColor( ucFogColor );
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
	{
		float scale = LinearToGammaFullRange( materials->GetToneMappingScaleLinear().x );
		ucFogColor[0] *= scale;
		ucFogColor[1] *= scale;
		ucFogColor[2] *= scale;
	}
	materials->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
}

// Used to verify frame syncing.
void GenerateOverdrawForTesting()
{
	if ( !cl_overdraw_test.GetInt() )
		return;

	for ( int i=0; i < 40; i++ )
	{
		g_SmokeFogOverlayAlpha = 20 / 255.0;
		DrawSmokeFogOverlay();
	}
	g_SmokeFogOverlayAlpha = 0;
}


//-----------------------------------------------------------------------------
// Convars related to controlling rendering
//-----------------------------------------------------------------------------
static ConVar cl_maxrenderable_dist("cl_maxrenderable_dist", "3000", FCVAR_CHEAT, "Max distance from the camera at which things will be rendered" );

ConVar r_updaterefracttexture( "r_updaterefracttexture", "1" );

ConVar r_entityclips( "r_entityclips", "1" ); //FIXME: Nvidia drivers before 81.94 on cards that support user clip planes will have problems with this, require driver update? Detect and disable?

// Matches the version in the engine
static ConVar r_drawopaqueworld( "r_drawopaqueworld", "1", FCVAR_CHEAT );
static ConVar r_drawtranslucentworld( "r_drawtranslucentworld", "1", FCVAR_CHEAT );
static ConVar r_3dsky( "r_3dsky","1", 0, "Enable the rendering of 3d sky boxes" );
static ConVar r_skybox( "r_skybox","1", FCVAR_CHEAT, "Enable the rendering of sky boxes" );
ConVar r_drawviewmodel( "r_drawviewmodel","1", FCVAR_CHEAT );
static ConVar r_drawtranslucentrenderables( "r_drawtranslucentrenderables", "1", FCVAR_CHEAT );
static ConVar r_drawopaquerenderables( "r_drawopaquerenderables", "1", FCVAR_CHEAT );

static ConVar r_flashlightdrawdepth( "r_flashlightdrawdepth", "0" );

ConVar r_DrawDetailProps( "r_DrawDetailProps", "1", FCVAR_CHEAT, "0=Off, 1=Normal, 2=Wireframe" );

ConVar mat_bloomscale( "mat_bloomscale", "1" );

//-----------------------------------------------------------------------------
// Convars related to fog color
//-----------------------------------------------------------------------------
static ConVar fog_override( "fog_override", "0", FCVAR_CHEAT );
// set any of these to use the maps fog
static ConVar fog_start( "fog_start", "-1" );
static ConVar fog_end( "fog_end", "-1" );
static ConVar fog_color( "fog_color", "-1 -1 -1" );
static ConVar fog_enable( "fog_enable", "1" );
static ConVar fog_startskybox( "fog_startskybox", "-1" );
static ConVar fog_endskybox( "fog_endskybox", "-1" );
static ConVar fog_colorskybox( "fog_colorskybox", "-1 -1 -1" );
static ConVar fog_enableskybox( "fog_enableskybox", "1" );


//-----------------------------------------------------------------------------
// Water-related convars
//-----------------------------------------------------------------------------
static ConVar r_debugcheapwater( "r_debugcheapwater", "0", FCVAR_CHEAT );
static ConVar r_waterforceexpensive( "r_waterforceexpensive", "0" );
static ConVar r_waterforcereflectentities( "r_waterforcereflectentities", "0" );
static ConVar r_WaterDrawRefraction( "r_WaterDrawRefraction", "1", 0, "Enable water refraction" );
static ConVar r_WaterDrawReflection( "r_WaterDrawReflection", "1", 0, "Enable water reflection" );
static ConVar r_ForceWaterLeaf( "r_ForceWaterLeaf", "1", 0, "Enable for optimization to water - considers view in leaf under water for purposes of culling" );
static ConVar mat_drawwater( "mat_drawwater", "1", FCVAR_CHEAT );
static ConVar mat_clipz( "mat_clipz", "1" );


//-----------------------------------------------------------------------------
// debugging
//-----------------------------------------------------------------------------
static ConVar r_visocclusion( "r_visocclusion", "0", FCVAR_CHEAT );
// (the engine owns this cvar).
ConVar mat_wireframe( "mat_wireframe", "0", FCVAR_CHEAT );

// sv_cheats is replicated and lives in the engine.  re-declaring it here without FCVAR_REPLICATED
// breaks the linkage, and declaring it here with FCVAR_REPLICATED wants it to be declared in the server also.
const ConVar *sv_cheats = NULL;



//-----------------------------------------------------------------------------
// debugging overlays
//-----------------------------------------------------------------------------
static ConVar cl_drawmaterial( "cl_drawmaterial", "", FCVAR_CHEAT, "Draw a particular material over the frame" );
static ConVar cl_drawshadowtexture( "cl_drawshadowtexture", "0", FCVAR_CHEAT );
static ConVar mat_showwatertextures( "mat_showwatertextures", "0", FCVAR_CHEAT );
static ConVar mat_wateroverlaysize( "mat_wateroverlaysize", "128" );
static ConVar mat_showframebuffertexture( "mat_showframebuffertexture", "0", FCVAR_CHEAT );
static ConVar mat_framebuffercopyoverlaysize( "mat_framebuffercopyoverlaysize", "128" );
static ConVar mat_showcamerarendertarget( "mat_showcamerarendertarget", "0", FCVAR_CHEAT );
static ConVar mat_camerarendertargetoverlaysize( "mat_camerarendertargetoverlaysize", "128", FCVAR_CHEAT );
static ConVar mat_hsv( "mat_hsv", "0" );
static ConVar mat_yuv( "mat_yuv", "0" );

#ifdef _XBOX
static ConVar mat_viewTextureEnable("mat_viewTextureEnable", "", 0, "Enable view texture");
static ConVar mat_viewTextureName("mat_viewTextureName", "", 0, "Set the view texture name");
static ConVar mat_viewTextureScale("mat_viewTextureScale", "1", 0, "Set the view texture scale");
#endif

//-----------------------------------------------------------------------------
// Other convars
//-----------------------------------------------------------------------------

ConVar r_DoCovertTransitions("r_DoCovertTransitions", "1", FCVAR_NEVER_AS_STRING );				// internally used by game code and engine to choose when to allow LOD transitions.
ConVar r_TransitionSensitivity("r_TransitionSensitivity", "6", 0, "Controls when LODs are changed. Lower numbers cause more overt LOD transitions.");

static ConVar r_screenfademinsize( "r_screenfademinsize", "0" );
static ConVar r_screenfademaxsize( "r_screenfademaxsize", "0" );


//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
static Vector g_vecCurrentRenderOrigin(0,0,0);
static QAngle g_vecCurrentRenderAngles(0,0,0);
static Vector g_vecCurrentVForward(0,0,0), g_vecCurrentVRight(0,0,0), g_vecCurrentVUp(0,0,0);
static VMatrix g_matCurrentCamInverse;
static bool s_bCanAccessCurrentView = false;
IntroData_t *g_pIntroData = NULL;
static bool	g_bRenderingView = false;			// For debugging...
static int g_CurrentViewID = VIEW_NONE;
bool g_bRenderingScreenshot = false;
int g_viewscene_refractUpdateFrame = 0;

// mapmaker controlled autoexposure
bool g_bUseCustomAutoExposureMin = false;
bool g_bUseCustomAutoExposureMax = false;
bool g_bUseCustomBloomScale = false;
float g_flCustomAutoExposureMin = 0;
float g_flCustomAutoExposureMax = 0;
float g_flCustomBloomScale = 0.0f;
float g_flCustomBloomScaleMinimum = 0.0f;
//-----------------------------------------------------------------------------
// Precache of necessary materials
//-----------------------------------------------------------------------------

#ifdef HL2_CLIENT_DLL
CLIENTEFFECT_REGISTER_BEGIN( PrecacheViewRender )
	CLIENTEFFECT_MATERIAL( "scripted/intro_screenspaceeffect" )
	CLIENTEFFECT_REGISTER_END()
#endif

#ifndef _XBOX
CLIENTEFFECT_REGISTER_BEGIN( PrecachePostProcessingEffects )
	CLIENTEFFECT_MATERIAL( "dev/downsample" )
	CLIENTEFFECT_MATERIAL( "dev/downsample_non_hdr" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery_and_add_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/no_pixel_write" )
	CLIENTEFFECT_MATERIAL( "dev/lumcompare" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx" )
	CLIENTEFFECT_MATERIAL( "dev/blurfilterx_nohdr" )
	CLIENTEFFECT_MATERIAL( "dev/floattoscreen_combine" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb_vanilla" )
	CLIENTEFFECT_MATERIAL( "dev/copyfullframefb" )
	CLIENTEFFECT_MATERIAL( "dev/blurfiltery" )
	CLIENTEFFECT_REGISTER_END_CONDITIONAL( engine->GetDXSupportLevel() >= 90)
#endif

//-----------------------------------------------------------------------------
// Accessors to return the current view being rendered
//-----------------------------------------------------------------------------
	const Vector &CurrentViewOrigin()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderOrigin;
}

const QAngle &CurrentViewAngles()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentRenderAngles;
}

const Vector &CurrentViewForward()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVForward;
}

const Vector &CurrentViewRight()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVRight;
}

const Vector &CurrentViewUp()
{
	Assert( s_bCanAccessCurrentView );
	return g_vecCurrentVUp;
}

const VMatrix &CurrentWorldToViewMatrix()
{
	Assert( s_bCanAccessCurrentView );
	return g_matCurrentCamInverse;
}


//-----------------------------------------------------------------------------
// Methods to set the current view/guard access to view parameters
//-----------------------------------------------------------------------------
void AllowCurrentViewAccess( bool allow )
{
	s_bCanAccessCurrentView = allow;
}

bool IsCurrentViewAccessAllowed()
{
	return s_bCanAccessCurrentView;
}

void SetupCurrentView( const Vector &vecOrigin, const QAngle &angles, view_id_t viewID )
{
	// Store off view origin and angles
	g_vecCurrentRenderOrigin = vecOrigin;
	g_vecCurrentRenderAngles = angles;

	// Compute the world->main camera transform
	ComputeCameraVariables( vecOrigin, angles, 
		&g_vecCurrentVForward, &g_vecCurrentVRight, &g_vecCurrentVUp, &g_matCurrentCamInverse );

	g_CurrentViewID = viewID;
	s_bCanAccessCurrentView = true;

	// Cache off fade distances
	float flScreenFadeMinSize, flScreenFadeMaxSize;
	view->GetScreenFadeDistances( &flScreenFadeMinSize, &flScreenFadeMaxSize );
	modelinfo->SetViewScreenFadeRange( flScreenFadeMinSize, flScreenFadeMaxSize );
}

view_id_t CurrentViewID()
{
	return ( view_id_t )g_CurrentViewID;
}

void FinishCurrentView()
{
	s_bCanAccessCurrentView = false;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CViewRender::CViewRender()
{
	m_AnglesHistoryCounter = 0;
	memset(m_AnglesHistory, 0, sizeof(m_AnglesHistory));
	m_flCheapWaterStartDistance = 0.0f;
	m_flCheapWaterEndDistance = 0.1f;
	m_BaseDrawFlags = m_DrawFlags = 0;
	m_bOverrideVisData		= false;
	m_iForceViewLeaf		= -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CViewRender::ShouldDrawEntities( void )
{
	return ( !m_pDrawEntities || (m_pDrawEntities->GetInt() != 0) );
}


//-----------------------------------------------------------------------------
// Purpose: Check all conditions which would prevent drawing the view model
// Input  : drawViewmodel - 
//			*viewmodel - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawViewModel( bool bDrawViewmodel )
{
	if ( !bDrawViewmodel )
		return false;

	if ( !r_drawviewmodel.GetBool() )
		return false;

	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
		return false;

	if ( !ShouldDrawEntities() )
		return false;

	if ( render->GetViewEntity() > gpGlobals->maxClients )
		return false;

	return true;
}


void CViewRender::DrawRenderablesInList( CUtlVector< IClientRenderable * > &list )
{
	int nCount = list.Count();
	for( int i=0; i < nCount; ++i )
	{
		IClientUnknown *pUnk = list[i]->GetIClientUnknown();
		Assert( pUnk );
		C_BaseEntity *ent = pUnk->GetBaseEntity();
		Assert( ent );

		// Non-view models wanting to render in view model list...
		if ( ent->ShouldDraw() )
		{
			ent->DrawModel( STUDIO_RENDER );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Actually draw the view model
// Input  : drawViewModel - 
//-----------------------------------------------------------------------------
void CViewRender::DrawViewModels( const CViewSetup &view, bool drawViewmodel )
{
	VPROF( "CViewRender::DrawViewModel" );

	if ( !ShouldDrawViewModel( drawViewmodel ) )
		return;

	// Restore the matrices
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();

	// Set up for drawing the view model
	render->SetProjectionMatrix( view.fovViewmodel, view.zNearViewmodel, view.zFarViewmodel );

	const bool bUseDepthHack = true;

	// FIXME: Add code to read the current depth range
	float depthmin = 0.0f;
	float depthmax = 1.0f;

	// HACK HACK:  Munge the depth range to prevent view model from poking into walls, etc.
	// Force clipped down range
	if( bUseDepthHack )
		materials->DepthRange( 0.0f, 0.1f );
	
	CUtlVector< IClientRenderable * > opaqueViewModelList( 32 );
	CUtlVector< IClientRenderable * > translucentViewModelList( 32 );

	ClientLeafSystem()->CollateViewModelRenderables( opaqueViewModelList, translucentViewModelList );
	DrawRenderablesInList( opaqueViewModelList );
	DrawRenderablesInList( translucentViewModelList );

	// Reset the depth range to the original values
	if( bUseDepthHack )
		materials->DepthRange( depthmin, depthmax );

	// Restore the matrices
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnt - 
// Output : int
//-----------------------------------------------------------------------------
VPlane* CViewRender::GetFrustum()
{
	// The frustum is only valid while in a RenderView call.
	Assert(g_bRenderingView || g_bRenderingCameraView || g_bRenderingScreenshot);	
	return m_Frustum;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::ShouldDrawBrushModels( void )
{
	if ( m_pDrawBrushModels && !m_pDrawBrushModels->GetInt() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Sort entities in a back-to-front ordering
//-----------------------------------------------------------------------------
void SortEntities( CRenderList::CEntry *pEntities, int nEntities )
{
	// Don't sort if we only have 1 entity
	if ( nEntities <= 1 )
		return;

	float dists[CRenderList::MAX_GROUP_ENTITIES];

	const Vector &vecRenderOrigin = CurrentViewOrigin();
	const Vector &vecRenderForward = CurrentViewForward();

	// First get a distance for each entity.
	int i;
	for( i=0; i < nEntities; i++ )
	{
		IClientRenderable *pRenderable = pEntities[i].m_pRenderable;

		// Compute the center of the object (needed for translucent brush models)
		Vector boxcenter;
		Vector mins,maxs;
		pRenderable->GetRenderBounds( mins, maxs );
		VectorAdd( mins, maxs, boxcenter );
		VectorMA( pRenderable->GetRenderOrigin(), 0.5f, boxcenter, boxcenter );

		// Compute distance...
		Vector delta;
		VectorSubtract( boxcenter, vecRenderOrigin, delta );
		dists[i] = DotProduct( delta, vecRenderForward );
	}

	// H-sort.
	int stepSize = 4;
	while( stepSize )
	{
		int end = nEntities - stepSize;
		for( i=0; i < end; i += stepSize )
		{
			if( dists[i] > dists[i+stepSize] )
			{
				swap( pEntities[i], pEntities[i+stepSize] );
				swap( dists[i], dists[i+stepSize] );

				if( i == 0 )
				{
					i = -stepSize;
				}
				else
				{
					i -= stepSize << 1;
				}
			}
		}

		stepSize >>= 1;
	}
}


void CViewRender::SetupRenderList( const CViewSetup *pView, ClientWorldListInfo_t& info, CRenderList &renderList )
{
	VPROF( "CViewRender::SetupRenderList" );

	// Clear the list.
	int i;
	for( i=0; i < RENDER_GROUP_COUNT; i++ )
	{
		renderList.m_RenderGroupCounts[i] = 0;
	}

	// Precache information used commonly in CollateRenderables
	SetupRenderInfo_t setupInfo;
	setupInfo.m_nRenderFrame = m_BuildRenderableListsNumber;
	setupInfo.m_nDetailBuildFrame = m_BuildWorldListsNumber;
	setupInfo.m_pRenderList = &renderList;
	setupInfo.m_bDrawDetailObjects = g_pClientMode->ShouldDrawDetailObjects() && r_DrawDetailProps.GetInt();

	if (pView)
	{
		setupInfo.m_vecRenderOrigin = pView->origin;
		setupInfo.m_flRenderDistSq = cl_maxrenderable_dist.GetFloat();
		setupInfo.m_flRenderDistSq  *= setupInfo.m_flRenderDistSq;
	}
	else
	{
		setupInfo.m_flRenderDistSq = 0.0f;
	}

	// Now collate the entities in the leaves.
	if( ShouldDrawEntities() )
	{
		IClientLeafSystem *pClientLeafSystem = ClientLeafSystem();
		for( i=0; i < info.m_LeafCount; i++ )
		{
			int nTranslucent = renderList.m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY];

			// Add renderables from this leaf...
			pClientLeafSystem->CollateRenderablesInLeaf( info.m_pLeafList[i], i, setupInfo );

			int nNewTranslucent = renderList.m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - nTranslucent;
			if( nNewTranslucent )
			{
				// Sort the new translucent entities.
				SortEntities( &renderList.m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY][nTranslucent], nNewTranslucent );
			}
		}
	}
}

static void OverlayWaterTexture( IMaterial *pMaterial, int xOffset, int yOffset, bool bFlip )
{
#ifndef _XBOX
	float xBaseOffset = 0;
	float yBaseOffset = 0;
#else
	// screen safe
	float xBaseOffset = 32;
	float yBaseOffset = 32;
#endif
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	float fFlip0 = bFlip ? 1.0f : 0.0f;
	float fFlip1 = bFlip ? 0.0f : 1.0f;
	if( !IsErrorMaterial( pMaterial ) )
	{
		materials->Bind( pMaterial );
		IMesh* pMesh = materials->GetDynamicMesh( true );

		float w = mat_wateroverlaysize.GetFloat();
		float h = mat_wateroverlaysize.GetFloat();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( xBaseOffset + xOffset * w, yBaseOffset + yOffset * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, fFlip1 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + ( xOffset + 1 ) * w, yBaseOffset + yOffset * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, fFlip1 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + ( xOffset + 1 ) * w, yBaseOffset + ( yOffset + 1 ) * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, fFlip0 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( xBaseOffset + xOffset * w, yBaseOffset + ( yOffset + 1 ) * h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, fFlip0 + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}

static void OverlayWaterTextures( void )
{
	OverlayWaterTexture( materials->FindMaterial( "debug/debugreflect", NULL ), 0, 0, false );
	OverlayWaterTexture( materials->FindMaterial( "debug/debugrefract", NULL ), 0, 1, true );
}

void OverlayCameraRenderTarget( const char *pszMaterialName, float flX, float flY, float w, float h )
{
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	IMaterial *pMaterial;
	pMaterial = materials->FindMaterial( pszMaterialName, TEXTURE_GROUP_OTHER, true );
	if( !IsErrorMaterial( pMaterial ) )
	{
		materials->Bind( pMaterial );
		IMesh* pMesh = materials->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( flX, flY, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX+w, flY, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX+w, flY+h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( flX, flY+h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}


static void OverlayFrameBufferTexture( int nFrameBufferIndex )
{
	float offsetS = ( 0.5f / 256.0f );
	float offsetT = ( 0.5f / 256.0f );
	IMaterial *pMaterial;
	char buf[MAX_PATH];
	Q_snprintf( buf, MAX_PATH, "debug/debugfbtexture%d", nFrameBufferIndex );
	pMaterial = materials->FindMaterial( buf, TEXTURE_GROUP_OTHER, true );
	if( !IsErrorMaterial( pMaterial ) )
	{
		materials->Bind( pMaterial );
		IMesh* pMesh = materials->GetDynamicMesh( true );

		float w = mat_framebuffercopyoverlaysize.GetFloat();
		float h = mat_framebuffercopyoverlaysize.GetFloat();

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( w * nFrameBufferIndex, 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * ( nFrameBufferIndex + 1 ), 0.0f, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 0.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * ( nFrameBufferIndex + 1 ), h, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( w * nFrameBufferIndex, h, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f + offsetS, 1.0f + offsetT );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}



void UpdateClientRenderableInPVSStatus()
{
	// Vis for this view should already be setup at this point.

	// For each client-only entity, notify it if it's newly coming into the PVS.
	CUtlLinkedList<CClientEntityList::CPVSNotifyInfo,unsigned short> &theList = ClientEntityList().GetPVSNotifiers();
	FOR_EACH_LL( theList, i )
	{
		CClientEntityList::CPVSNotifyInfo *pInfo = &theList[i];

		if ( pInfo->m_InPVSStatus & INPVS_YES )
		{
			// Ok, this entity already thinks it's in the PVS. No need to notify it.
			// We need to set the INPVS_YES_THISFRAME flag if it's in this frame at all, so we 
			// don't tell the entity it's not in the PVS anymore at the end of the frame.
			if ( !( pInfo->m_InPVSStatus & INPVS_THISFRAME ) )
			{
				if ( g_pClientLeafSystem->IsRenderableInPVS( pInfo->m_pRenderable ) )
				{
					pInfo->m_InPVSStatus |= INPVS_THISFRAME;
				}
			}
		}
		else
		{
			// This entity doesn't think it's in the PVS yet. If it is now in the PVS, let it know.
			if ( g_pClientLeafSystem->IsRenderableInPVS( pInfo->m_pRenderable ) )
			{
				pInfo->m_pNotify->OnPVSStatusChanged( true );
				pInfo->m_InPVSStatus |= INPVS_YES;
				pInfo->m_InPVSStatus |= INPVS_THISFRAME;
			}
		}
	}	
}

//-----------------------------------------------------------------------------
// Debugging aid to display a texture
//-----------------------------------------------------------------------------
#ifdef _XBOX
static void OverlayShowTexture( const char* textureName, float scale )
{
	bool			foundVar;
	IMaterial		*pMaterial;
	IMaterialVar	*BaseTextureVar;
	ITexture		*pTex;
	float			x, y, w, h;

	// screen safe
	x = 32;
	y = 32;

	pMaterial = materials->FindMaterial( "___debug", TEXTURE_GROUP_OTHER, true );
	BaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );
	if (!foundVar)
		return;

	if ( textureName && textureName[0] )
	{
		pTex = materials->FindTexture( textureName, TEXTURE_GROUP_OTHER, false );
		BaseTextureVar->SetTextureValue( pTex );

		w = pTex->GetActualWidth() * scale;
		h = pTex->GetActualHeight() * scale;
	}
	else
	{
		w = h = 64.0f * scale;
	}

	materials->Bind( pMaterial );
	IMesh* pMesh = materials->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );
	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x+w, y, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x+w, y+h, 0.0f );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( x, x+h, 0.0f );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}
#endif

extern ConVar cl_leveloverview;
//-----------------------------------------------------------------------------
// Purpose: Builds lists of things to render in the world, called once per view
//-----------------------------------------------------------------------------
void CViewRender::BuildWorldRenderLists( const CViewSetup *pView, 
	ClientWorldListInfo_t& info, bool bUpdateLightmaps, bool bDrawEntities, int iForceViewLeaf )
{
	VPROF_BUDGET( "BuildWorldRenderLists", VPROF_BUDGETGROUP_WORLD_RENDERING );
	
	// Server entities already know which ones are in the PVS, but client-only entities don't.
	// We need to know which client-only entities are in the PVS at this point so the shadow
	// manager can project/occlude their shadows correctly.
	UpdateClientRenderableInPVSStatus();

	++m_BuildWorldListsNumber;
	// Override vis data if specified this render, otherwise use default behavior with NULL
	VisOverrideData_t* pVisData = (m_bOverrideVisData)?(&m_VisData):(NULL);
	render->BuildWorldLists_VisOverride( &info, bUpdateLightmaps, iForceViewLeaf, pVisData );

	// Return to default area portal behavior by using the CurrentView as the starting area
	m_bOverrideVisData		= false;
	m_iForceViewLeaf		= -1;

	if ( bDrawEntities )
	{
		// Now that we have the list of all leaves, regenerate shadows cast
		g_pClientShadowMgr->ComputeShadowTextures( pView, info.m_LeafCount, info.m_pLeafList );

		// Compute the prop opacity based on the view position and fov zoom scale
		float flFactor = 1.0f;
		C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
		if ( pLocal )
		{
			flFactor = pLocal->GetFOVDistanceAdjustFactor();
		}

		if ( cl_leveloverview.GetFloat() > 0 )
		{
			// disable prop fading
			flFactor = -1;
		}

		// When zoomed in, tweak the opacity to stay visible from further away
		staticpropmgr->ComputePropOpacity( CurrentViewOrigin(), flFactor );

		// Build a list of detail props to render
		DetailObjectSystem()->BuildDetailObjectRenderLists();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Computes the actual world list info based on the render flags
//-----------------------------------------------------------------------------
ClientWorldListInfo_t *CViewRender::ComputeActualWorldListInfo( const ClientWorldListInfo_t& info, int nDrawFlags, ClientWorldListInfo_t& tmpInfo )
{
	// Drawing everything? Just return the world list info as-is 
	int nWaterDrawFlags = nDrawFlags & (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER);
	if ( nWaterDrawFlags == (DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER) )
		return const_cast<ClientWorldListInfo_t*>(&info);

	tmpInfo.m_ViewFogVolume = info.m_ViewFogVolume;
	tmpInfo.m_LeafCount = 0;

	// Not drawing anything? Then don't bother with renderable lists
	if ( nWaterDrawFlags == 0 )
		return &tmpInfo;
		
	// Create a sub-list based on the actual leaves being rendered
	bool bRenderingUnderwater = (nWaterDrawFlags & DF_RENDER_UNDERWATER) != 0;
	for ( int i = 0; i < info.m_LeafCount; ++i )
	{
		bool bLeafIsUnderwater = ( info.m_pLeafFogVolume[i] != -1 );
		if ( bRenderingUnderwater == bLeafIsUnderwater )
		{
			tmpInfo.m_pLeafList[ tmpInfo.m_LeafCount ] = info.m_pLeafList[ i ];
			tmpInfo.m_pLeafFogVolume[ tmpInfo.m_LeafCount ] = info.m_pLeafFogVolume[ i ];
			tmpInfo.m_pActualLeafIndex[ tmpInfo.m_LeafCount ] = i;
			++tmpInfo.m_LeafCount;
		}
	}

	return &tmpInfo;
}


//-----------------------------------------------------------------------------
// Purpose: Builds render lists for renderables. Called once for refraction, once for over water
//-----------------------------------------------------------------------------
void CViewRender::BuildRenderableRenderLists( const CViewSetup *pView, ClientWorldListInfo_t& info, CRenderList &renderList )
{
	MDLCACHE_CRITICAL_SECTION();
	++m_BuildRenderableListsNumber;

	// For better sorting, find out the leaf *nearest* to the camera
	// and render translucent objects as if they are in that leaf.
	if( ShouldDrawEntities() )
	{
		ClientLeafSystem()->ComputeTranslucentRenderLeaf( 
			info.m_LeafCount, info.m_pLeafList, info.m_pLeafFogVolume, m_BuildRenderableListsNumber );
	}
	
	SetupRenderList( pView, info, renderList );
}


//-----------------------------------------------------------------------------
// Computes draw flags for the engine to build its world surface lists
//-----------------------------------------------------------------------------
static inline unsigned long BuildDrawFlags( bool bDrawSkybox, bool bDrawUnderWater, bool bDrawAboveWater, bool bDrawWaterSurface, bool bClipSkybox )
{
	unsigned long nEngineFlags = 0;

	if ( bDrawSkybox )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_SKYBOX;
	}

	if ( bDrawAboveWater )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYABOVEWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( bDrawUnderWater )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_STRICTLYUNDERWATER;
		nEngineFlags |= DRAWWORLDLISTS_DRAW_INTERSECTSWATER;
	}

	if ( bDrawWaterSurface )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_WATERSURFACE;
	}

	if( bClipSkybox )
	{
		nEngineFlags |= DRAWWORLDLISTS_DRAW_CLIPSKYBOX;
	}

	return nEngineFlags;
}

void CViewRender::DrawWorld( ClientWorldListInfo_t& info, CRenderList &renderList, int flags, float waterZAdjust )
{
	VPROF_INCREMENT_COUNTER( "RenderWorld", 1 );
	VPROF_BUDGET( "DrawWorld", VPROF_BUDGETGROUP_WORLD_RENDERING );
	if( !r_drawopaqueworld.GetBool() )
	{
		return;
	}

	bool drawSkybox = (flags & DF_DRAWSKYBOX) != 0;
	bool drawUnderWater = (flags & DF_RENDER_UNDERWATER) != 0;
	bool drawAboveWater = (flags & DF_RENDER_ABOVEWATER) != 0;
	bool drawWaterSurface = (flags & DF_RENDER_WATER) != 0;
	bool bClipSkybox = (flags & DF_CLIP_SKYBOX ) != 0;

	unsigned long engineFlags = BuildDrawFlags( drawSkybox, drawUnderWater, drawAboveWater, drawWaterSurface, bClipSkybox );
	if ( flags & DF_MAINTAINWORLDLISTS )
	{
		engineFlags |= DRAWWORLDLISTS_MAINTAIN_RENDER_LISTS;
	}

	int oldDrawFlags = m_DrawFlags;
	m_DrawFlags |= flags;

	render->DrawWorldLists( engineFlags, waterZAdjust );

	m_DrawFlags = oldDrawFlags;
}

static inline void DrawOpaqueRenderable( IClientRenderable *pEnt, bool twoPass )
{
	float color[3];

	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );


	int flags = STUDIO_RENDER;
	if (twoPass)
	{
		flags |= STUDIO_TWOPASS;
	}

	float *pRenderClipPlane = NULL;

	if( !materials->UsingFastClipping() && //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
		r_entityclips.GetBool() ) //checking here reduces the r_entityclips.GetBool() check from 2 times to 1
			pRenderClipPlane = pEnt->GetRenderClipPlane();

	if( pRenderClipPlane )	
		materials->PushCustomClipPlane( pRenderClipPlane );	

	pEnt->DrawModel( flags );
	
	if( pRenderClipPlane )	
		materials->PopCustomClipPlane();
}


static inline void DrawTranslucentRenderable( IClientRenderable *pEnt, bool twoPass )
{
	// Determine blending amount and tell engine
	float blend = (float)( pEnt->GetFxBlend() / 255.0f );

	// Totally gone
	if ( blend <= 0.0f )
		return;

	// Tell engine
	render->SetBlend( blend );

	float color[3];
	pEnt->GetColorModulation( color );
	render->SetColorModulation(	color );

	int flags = STUDIO_RENDER | STUDIO_TRANSPARENCY;
	if (twoPass)
		flags |= STUDIO_TWOPASS;

#if 0
	Vector mins, maxs;
	pEnt->GetRenderBounds( mins, maxs );
	debugoverlay->AddBoxOverlay( pEnt->GetRenderOrigin(), mins, maxs, pEnt->GetRenderAngles(), 255, 255, 255, 64, 0.01 );
	if ( pEnt->GetModel() )
	{
		const char *pName = modelinfo->GetModelName( pEnt->GetModel() );
		if ( Q_stricmp( pName, "models/props_c17/tv_monitor01_screen.mdl" ) )
		{
			debugoverlay->AddTextOverlay( pEnt->GetRenderOrigin(), 0.01, pName );
		}
	}
#endif

	

	float *pRenderClipPlane = NULL;

	if( !materials->UsingFastClipping() && //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
		r_entityclips.GetBool() ) //checking here reduces the r_entityclips.GetBool() check from 2 times to 1
			pRenderClipPlane = pEnt->GetRenderClipPlane();

	if( pRenderClipPlane )	
		materials->PushCustomClipPlane( pRenderClipPlane );	

	pEnt->DrawModel( flags );

	if( pRenderClipPlane )	
		materials->PopCustomClipPlane();
}


//-----------------------------------------------------------------------------
// Draws all opaque renderables in leaves that were rendered
//-----------------------------------------------------------------------------
void CViewRender::DrawOpaqueRenderables( ClientWorldListInfo_t& info, CRenderList &renderList )
{
	VPROF("CViewRender::DrawOpaqueRenderables");

	if( !r_drawopaquerenderables.GetBool() )
		return;
	
	if( !ShouldDrawEntities() )
		return;

	render->SetBlend( 1 );
	
	// Iterate over all leaves that were visible, and draw opaque things in them.	
	RopeManager()->ResetRenderCache();

	// Iterate over all leaves that were visible, and draw opaque things in them.
	int i;

	// first do the brush models
	CRenderList::CEntry *pEntities = renderList.m_RenderGroups[RENDER_GROUP_OPAQUE_BRUSH];
	int nOpaque = renderList.m_RenderGroupCounts[RENDER_GROUP_OPAQUE_BRUSH];
	for( i=0; i < nOpaque; ++i )
	{
		Assert(pEntities[i].m_TwoPass==0);
		DrawOpaqueRenderable( pEntities[i].m_pRenderable, false );
	}

	// now the static props
	pEntities = renderList.m_RenderGroups[RENDER_GROUP_OPAQUE_STATIC];
	nOpaque = renderList.m_RenderGroupCounts[RENDER_GROUP_OPAQUE_STATIC];
	if ( nOpaque )
	{
		nOpaque = min( nOpaque, 512 );

		IClientRenderable *pStatics[512];
		for( i=0; i < nOpaque; ++i )
		{
			pStatics[i] = pEntities[i].m_pRenderable;
		}
		staticpropmgr->DrawStaticProps( pStatics, nOpaque );
	}


	// now the other opaque entities
	pEntities = renderList.m_RenderGroups[RENDER_GROUP_OPAQUE_ENTITY];
	nOpaque = renderList.m_RenderGroupCounts[RENDER_GROUP_OPAQUE_ENTITY];

	for( i=0; i < nOpaque; ++i )
	{
		DrawOpaqueRenderable( pEntities[i].m_pRenderable, (pEntities[i].m_TwoPass != 0) );
	}

	RopeManager()->DrawRenderCache();
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CViewRender::DrawTranslucentWorldInLeaves( int iCurLeafIndex, int iFinalLeafIndex, ClientWorldListInfo_t &info, int nDrawFlags )
{
	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	for( ; iCurLeafIndex >= iFinalLeafIndex; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( nActualLeafIndex, nDrawFlags ) )
		{
			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( nActualLeafIndex, nDrawFlags );
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent world + detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CViewRender::DrawTranslucentWorldAndDetailPropsInLeaves( int iCurLeafIndex, int iFinalLeafIndex,
	ClientWorldListInfo_t &info, int nDrawFlags, int &nDetailLeafCount, LeafIndex_t* pDetailLeafList )
{
	VPROF_BUDGET( "CViewRender::DrawTranslucentWorldAndDetailPropsInLeaves", VPROF_BUDGETGROUP_WORLD_RENDERING );
	for( ; iCurLeafIndex >= iFinalLeafIndex; iCurLeafIndex-- )
	{
		int nActualLeafIndex = info.m_pActualLeafIndex ? info.m_pActualLeafIndex[ iCurLeafIndex ] : iCurLeafIndex;
		Assert( nActualLeafIndex != INVALID_LEAF_INDEX );
		if ( render->LeafContainsTranslucentSurfaces( nActualLeafIndex, nDrawFlags ) )
		{
			// First draw any queued-up detail props from previously visited leaves
			DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), nDetailLeafCount, pDetailLeafList );
			nDetailLeafCount = 0;

			// Now draw the surfaces in this leaf
			render->DrawTranslucentSurfaces( nActualLeafIndex, nDrawFlags );
		}

		// Queue up detail props that existed in this leaf
		if ( ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( info.m_pLeafList[iCurLeafIndex], m_BuildWorldListsNumber ) )
		{
			pDetailLeafList[nDetailLeafCount] = info.m_pLeafList[iCurLeafIndex];
			++nDetailLeafCount;
		}
	}
}


//-----------------------------------------------------------------------------
// Renders all translucent entities in the render list
//-----------------------------------------------------------------------------
void CViewRender::DrawTranslucentRenderablesNoWorld( CRenderList &renderList, bool bInSkybox )
{
	VPROF( "CViewRender::DrawTranslucentRenderablesNoWorld" );

	if ( !ShouldDrawEntities() || !r_drawtranslucentrenderables.GetBool() )
		return;

	// Draw the particle singletons.
	DrawParticleSingletons( bInSkybox );
	
	CRenderList::CEntry *pEntities = renderList.m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
	int iCurTranslucentEntity = renderList.m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - 1;

	while( iCurTranslucentEntity >= 0 )
	{
		IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
		if ( pRenderable->UsesFrameBufferTexture() )
		{
			UpdateRefractTexture();
		}
		DrawTranslucentRenderable( pRenderable, (pEntities[iCurTranslucentEntity].m_TwoPass != 0) );
		--iCurTranslucentEntity;
	}

	// Reset the blend state.
	render->SetBlend( 1 );
}

//-----------------------------------------------------------------------------
// Renders all translucent world, entities, and detail objects in a particular set of leaves
//-----------------------------------------------------------------------------
void CViewRender::DrawTranslucentRenderables( ClientWorldListInfo_t& info, CRenderList &renderList, 
	int nFlags, bool bInSkybox )
{
	if ( !r_drawtranslucentworld.GetBool() )
	{
		DrawTranslucentRenderablesNoWorld( renderList, bInSkybox );
		return;
	}

	VPROF( "CViewRender::DrawTranslucentRenderables" );
	int iPrevLeaf = info.m_LeafCount - 1;
	int nDetailLeafCount = 0;
	LeafIndex_t *pDetailLeafList = (LeafIndex_t*)stackalloc( info.m_LeafCount * sizeof(LeafIndex_t) );

	bool bDrawUnderWater = (nFlags & DF_RENDER_UNDERWATER) != 0;
	bool bDrawAboveWater = (nFlags & DF_RENDER_ABOVEWATER) != 0;
	bool bDrawWater = (nFlags & DF_RENDER_WATER) != 0;
	bool bClipSkybox = (nFlags & DF_CLIP_SKYBOX ) != 0;
	unsigned long nDrawFlags = BuildDrawFlags( false, bDrawUnderWater, bDrawAboveWater, bDrawWater, bClipSkybox );

	DetailObjectSystem()->BeginTranslucentDetailRendering();

	if ( ShouldDrawEntities() && r_drawtranslucentrenderables.GetBool() )
	{
		// Draw the particle singletons.
		DrawParticleSingletons( bInSkybox );
		
		CRenderList::CEntry *pEntities = renderList.m_RenderGroups[RENDER_GROUP_TRANSLUCENT_ENTITY];
		int iCurTranslucentEntity = renderList.m_RenderGroupCounts[RENDER_GROUP_TRANSLUCENT_ENTITY] - 1;

		bool bRenderingWaterRenderTargets = nFlags & ( DF_RENDER_REFRACTION | DF_RENDER_REFLECTION );

		while( iCurTranslucentEntity >= 0 )
		{
			// Seek the current leaf up to our current translucent-entity leaf.
			int iThisLeaf = pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf;

			// First draw the translucent parts of the world up to and including those in this leaf
			DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, iThisLeaf, info, nDrawFlags, nDetailLeafCount, pDetailLeafList );

			// We're traversing the leaf list backwards to get the appropriate sort ordering (back to front)
			iPrevLeaf = iThisLeaf - 1;

			// Draw all the translucent entities with this leaf.
			int nLeaf = info.m_pLeafList[iThisLeaf];

			bool bDrawDetailProps = ClientLeafSystem()->ShouldDrawDetailObjectsInLeaf( nLeaf, m_BuildWorldListsNumber );
			if ( bDrawDetailProps )
			{
				// Draw detail props up to but not including this leaf
				Assert( nDetailLeafCount > 0 ); 
				--nDetailLeafCount;
				Assert( pDetailLeafList[nDetailLeafCount] == nLeaf );
				DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), nDetailLeafCount, pDetailLeafList );

				// Draw translucent renderables in the leaf interspersed with detail props
				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					MDLCACHE_CRITICAL_SECTION();
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;

					// Draw any detail props in this leaf that's farther than the entity
					const Vector &vecRenderOrigin = pRenderable->GetRenderOrigin();
					DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( 
						CurrentViewOrigin(), CurrentViewForward(), nLeaf, &vecRenderOrigin );

					if ( pRenderable->UsesFrameBufferTexture() )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}
						UpdateRefractTexture();
					}

					// Then draw the translucent renderable
					DrawTranslucentRenderable( pRenderable, (pEntities[iCurTranslucentEntity].m_TwoPass != 0) );
				}

				// Draw all remaining props in this leaf
				DetailObjectSystem()->RenderTranslucentDetailObjectsInLeaf( CurrentViewOrigin(), CurrentViewForward(), nLeaf, NULL );
			}
			else
			{
				// Draw queued up detail props (we know that the list of detail leaves won't include this leaf, since ShouldDrawDetailObjectsInLeaf is false)
				// Therefore no fixup on nDetailLeafCount is required as in the above section
				DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), nDetailLeafCount, pDetailLeafList );

				for( ;pEntities[iCurTranslucentEntity].m_iWorldListInfoLeaf == iThisLeaf && iCurTranslucentEntity >= 0; --iCurTranslucentEntity )
				{
					
					MDLCACHE_CRITICAL_SECTION();
					IClientRenderable *pRenderable = pEntities[iCurTranslucentEntity].m_pRenderable;
					if ( pRenderable->UsesFrameBufferTexture() )
					{
						if( bRenderingWaterRenderTargets )
						{
							continue;
						}
						UpdateRefractTexture();
					}
					DrawTranslucentRenderable( pRenderable, (pEntities[iCurTranslucentEntity].m_TwoPass != 0) );
					
				}
			}

			nDetailLeafCount = 0;
		}
	}

	// Draw the rest of the surfaces in world leaves
	DrawTranslucentWorldAndDetailPropsInLeaves( iPrevLeaf, 0, info, nDrawFlags, nDetailLeafCount, pDetailLeafList );

	// Draw any queued-up detail props from previously visited leaves
	DetailObjectSystem()->RenderTranslucentDetailObjects( CurrentViewOrigin(), CurrentViewForward(), nDetailLeafCount, pDetailLeafList );

	// Reset the blend state.
	render->SetBlend( 1 );
}


//-----------------------------------------------------------------------------
// Renders the shadow texture to screen...
//-----------------------------------------------------------------------------
static void RenderMaterial( const char *pMaterialName )
{
	// So it's not in the very top left
	float x = 100.0f, y = 100.0f;
	// float x = 0.0f, y = 0.0f;

	IMaterial *pMaterial = materials->FindMaterial( pMaterialName, TEXTURE_GROUP_OTHER, false );
	if ( !IsErrorMaterial( pMaterial ) )
	{
		materials->Bind( pMaterial );
		IMesh* pMesh = materials->GetDynamicMesh( true );

		CMeshBuilder meshBuilder;
		meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

		meshBuilder.Position3f( x, y, 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x + pMaterial->GetMappingWidth(), y, 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x + pMaterial->GetMappingWidth(), y + pMaterial->GetMappingHeight(), 0.0f );
		meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( x, y + pMaterial->GetMappingHeight(), 0.0f );
		meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.End();
		pMesh->Draw();
	}
}


//-----------------------------------------------------------------------------
// Draws the screen effect
//-----------------------------------------------------------------------------
static void DrawScreenEffectMaterial( IMaterial *pMaterial, int x, int y, int w, int h )
{
	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
	ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );

	materials->DrawScreenSpaceRectangle( pMaterial, x, y, w, h,
										 actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
										 pTexture->GetActualWidth(), pTexture->GetActualHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenSpaceEffects( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenSpaceEffects()");

	// FIXME: Screen-space effects are busted in the editor
	if ( engine->IsHammerRunning() )
		return;

	g_pScreenSpaceEffects->RenderEffects( x, y, w, h );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the screen space effect material (can't be done during rendering)
//-----------------------------------------------------------------------------
void CViewRender::SetScreenOverlayMaterial( IMaterial *pMaterial )
{
	m_ScreenOverlayMaterial.Init( pMaterial );
}

IMaterial *CViewRender::GetScreenOverlayMaterial( )
{
	return m_ScreenOverlayMaterial;
}


//-----------------------------------------------------------------------------
// Purpose: Performs screen space effects, if any
//-----------------------------------------------------------------------------
void CViewRender::PerformScreenOverlay( int x, int y, int w, int h )
{
	VPROF("CViewRender::PerformScreenOverlay()");

	if (m_ScreenOverlayMaterial)
	{
		if ( m_ScreenOverlayMaterial->NeedsFullFrameBufferTexture() )
		{
			DrawScreenEffectMaterial( m_ScreenOverlayMaterial, x, y, w, h );
		}
		else if ( m_ScreenOverlayMaterial->NeedsPowerOfTwoFrameBufferTexture() )
		{
			// First copy the FB off to the offscreen texture
			UpdateRefractTexture( x, y, w, h, true );

			// Now draw the entire screen using the material...
			ITexture *pTexture = GetPowerOfTwoFrameBufferTexture( );
			int sw = pTexture->GetActualWidth();
			int sh = pTexture->GetActualHeight();
			materials->DrawScreenSpaceRectangle( m_ScreenOverlayMaterial, x, y, w, h,
												 0, 0, sw-1, sh-1, sw, sh );
		}
		else
		{
			byte color[4] = { 255, 255, 255, 255 };
			render->ViewDrawFade( color, m_ScreenOverlayMaterial );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders world and all entities, etc.
//-----------------------------------------------------------------------------
#include "tier0/memdbgoff.h"

void CViewRender::DrawWorldAndEntities( bool bDrawSkybox, const CViewSetup &view, int nClearFlags )
{
	VPROF("CViewRender::DrawWorldAndEntities");
	MDLCACHE_CRITICAL_SECTION();

	ClientWorldListInfo_t info;	
#ifndef _XBOX
	CRenderList renderList;
#else
	void *ptr = MemAllocScratch(sizeof(CRenderList));
	CRenderList *renderList = new (ptr) CRenderList;
#endif

	EnableWorldFog();

	int nFlags = DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_RENDER_WATER;
	if (bDrawSkybox)
	{
		nFlags |= DF_DRAWSKYBOX;
	}

	render->Push3DView( view, nClearFlags, false, NULL, m_Frustum );

	BuildWorldRenderLists( &view, info, true, true, m_iForceViewLeaf );
#ifndef _XBOX
	BuildRenderableRenderLists( &view, info, renderList );
#else
	BuildRenderableRenderLists( &view, info, *renderList );
#endif

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	m_DrawFlags = m_BaseDrawFlags;
#ifndef _XBOX
	DrawWorld( info, renderList, nFlags, 0.0f );
	DrawOpaqueRenderables( info, renderList );
	DrawTranslucentRenderables( info, renderList, nFlags, false );
#else
	DrawWorld( info, *renderList, nFlags, 0.0f );
	DrawOpaqueRenderables( info, *renderList );
	DrawTranslucentRenderables( info, *renderList, nFlags, false );
	MemFreeScratch();
#endif

	m_DrawFlags = 0;

	ParticleMgr()->DrawBeforeViewModelEffects();

	render->PopView( m_Frustum );
}

#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Computes us some geometry to render the frustum planes
//-----------------------------------------------------------------------------
void CViewRender::ComputeFrustumRenderGeometry( Vector pRenderPoint[8] )
{
	Vector viewPoint = CurrentViewOrigin();

	// Find lines along each of the plane intersections.
	// We know these lines are perpendicular to both plane normals,
	// so we can take the cross product to find them.
	static int edgeIdx[4][2] =
	{
		{ 0, 2 }, { 0, 3 }, { 1, 3 }, { 1, 2 }
	};

	int i;
	Vector edges[4];
	for ( i = 0; i < 4; ++i)
	{
		CrossProduct( GetFrustum()[edgeIdx[i][0]].m_Normal,
			GetFrustum()[edgeIdx[i][1]].m_Normal, edges[i] );
		VectorNormalize( edges[i] );
	}

	// Figure out four near points by intersection lines with the near plane
	// Figure out four far points by intersection with lines against far plane
	for (i = 0; i < 4; ++i)
	{
		float t = (GetFrustum()[4].m_Dist - DotProduct(GetFrustum()[4].m_Normal, viewPoint)) /
			DotProduct(GetFrustum()[4].m_Normal, edges[i]);
		VectorMA( viewPoint, t, edges[i], pRenderPoint[i] );

		/*
		t = (m_FrustumPlanes[5][3] - DotProduct(GetFrustum()[5], viewPoint)) /
			DotProduct(GetFrustum()[5], edges[i]);
		VectorMA( viewPoint, t, edges[i], pRenderPoint[i + 4] );
		*/
		if (t < 0)
		{
			edges[i] *= -1;
		}

		VectorMA( pRenderPoint[i], 200.0, edges[i], pRenderPoint[i + 4] );
	}
}


//-----------------------------------------------------------------------------
// renders the frustum
//-----------------------------------------------------------------------------
void CViewRender::RenderFrustum( )
{
	static int indices[] = 
	{
		0, 1, 1, 2, 2, 3, 3, 0,	// near square
		4, 5, 5, 6, 6, 7, 7, 4,	// far square
		0, 4, 1, 5, 2, 6, 3, 7	// connections between them
	};

	int numIndices = sizeof(indices) / sizeof(int);

	Vector vecFrustumRenderPoint[8];
	ComputeFrustumRenderGeometry( vecFrustumRenderPoint );

	int i;
	for ( i = 0; i < numIndices; i += 2 )
	{
		debugoverlay->AddLineOverlay( vecFrustumRenderPoint[indices[i]],
			vecFrustumRenderPoint[indices[i+1]], 0, 0, 255, 255, 1.0f );
	}
}


//-----------------------------------------------------------------------------
// Draws all the debugging info
//-----------------------------------------------------------------------------
void CViewRender::Draw3DDebuggingInfo( const CViewSetup &view )
{
	VPROF("CViewRender::Draw3DDebuggingInfo");

	// Draw 3d overlays
	render->Draw3DDebugOverlays();

	// Draw the line file used for debugging leaks
	render->DrawLineFile();
	
	// Draw client side effects
	// NOTE: These are not sorted against the rest of the frame
	clienteffects->DrawEffects( gpGlobals->frametime );	

	// Mark the frame as locked down for client fx additions
	SetFXCreationAllowed( false );
}


//-----------------------------------------------------------------------------
// Draws all the debugging info
//-----------------------------------------------------------------------------
void CViewRender::Draw2DDebuggingInfo( const CViewSetup &view )
{
	if ( IsXbox() && IsRetail() )
		return;

	// HDRFIXME: Assert NULL rendertarget
	if ( mat_yuv.GetInt() && (engine->GetDXSupportLevel() >= 80) )
	{
		IMaterial *pMaterial;
		pMaterial = materials->FindMaterial( "debug/yuv", TEXTURE_GROUP_OTHER, true );
		if( !IsErrorMaterial( pMaterial ) )
		{
			DrawScreenEffectMaterial( pMaterial, view.x, view.y, view.width, view.height );
		}
	}

#ifndef _XBOX
	if ( mat_hsv.GetInt() && (engine->GetDXSupportLevel() >= 90) )
	{
		IMaterial *pMaterial;
		pMaterial = materials->FindMaterial( "debug/hsv", TEXTURE_GROUP_OTHER, true );
		if( !IsErrorMaterial( pMaterial ) )
		{
			DrawScreenEffectMaterial( pMaterial, view.x, view.y, view.width, view.height );
		}
	}
#endif

	// Draw debugging lightmaps
	render->DrawLightmaps();

	if ( cl_drawshadowtexture.GetInt() )
	{
		g_pClientShadowMgr->RenderShadowTexture( 256, 256 );
	}

	const char *pDrawMaterial = cl_drawmaterial.GetString();
	if ( pDrawMaterial && pDrawMaterial[0] )
	{
		RenderMaterial( pDrawMaterial ); 
	}

	if ( mat_showwatertextures.GetBool() )
	{
		OverlayWaterTextures();
	}

	if ( mat_showcamerarendertarget.GetBool() )
	{
		float w = mat_wateroverlaysize.GetFloat();
		float h = mat_wateroverlaysize.GetFloat();
		OverlayCameraRenderTarget( "debug/debugcamerarendertarget", 0, 0, w, h );
	}

	if ( mat_showframebuffertexture.GetBool() )
	{
		// HDRFIXME: Get rid of these rendertarget sets assuming that the assert at the top of this function is true.
		materials->PushRenderTargetAndViewport( NULL );
		OverlayFrameBufferTexture( 0 );
		OverlayFrameBufferTexture( 1 );
		materials->PopRenderTargetAndViewport( );
	}

#ifdef _XBOX
	if ( mat_viewTextureEnable.GetBool() )
	{
		OverlayShowTexture( mat_viewTextureName.GetString(), mat_viewTextureScale.GetFloat() );
	}
#endif

	if( r_flashlightdrawdepth.GetBool() )
	{
		shadowmgr->DrawFlashlightDepthTexture( );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the min/max fade distances
//-----------------------------------------------------------------------------
void CViewRender::GetScreenFadeDistances( float *min, float *max )
{
	if ( min )
	{
		*min = r_screenfademinsize.GetFloat();
	}

	if ( max )
	{
		*max = r_screenfademaxsize.GetFloat();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Renders world and all entities, etc.
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene( bool bDrew3dSkybox, bool bSkyboxVisible, const CViewSetup &view, 
								 int nClearFlags, view_id_t viewID, bool bDrawViewModel, int baseDrawFlags )
{
	VPROF( "CViewRender::ViewDrawScene" );

	m_BaseDrawFlags = baseDrawFlags;
	m_DrawFlags = 0;

	SetupCurrentView( view.origin, view.angles, viewID );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view
	unsigned int visFlags;
	SetupVis( view, visFlags );

	if ( !bDrew3dSkybox && 
		!bSkyboxVisible && ( visFlags & IVRenderView::VIEW_SETUP_VIS_EX_RETURN_FLAGS_USES_RADIAL_VIS ) )
	{
		// This covers the case where we don't see a 3dskybox, yet radial vis is clipping
		// the far plane.  Need to clear to fog color in this case.
		nClearFlags |= VIEW_CLEAR_COLOR;
		unsigned char ucFogColor[3];
		SetClearColorToFogColor( ucFogColor );
	}

	bool drawSkybox = r_skybox.GetBool();
	if ( bDrew3dSkybox || !bSkyboxVisible )
	{
		drawSkybox = false;
	}

	ParticleMgr()->IncrementFrameCode();

	WaterDrawWorldAndEntities( drawSkybox, view, nClearFlags );

	// Disable fog for the rest of the stuff
	DisableFog();

	// UNDONE: Don't do this with masked brush models, they should probably be in a separate list
	// render->DrawMaskEntities()
	
	// Here are the overlays...

	// This is an overlay that goes over everything else

	CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );

	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// Draw rain..
	DrawPrecipitation();

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	Draw3DDebuggingInfo( view );

	// Invoke post-render methods
	IGameSystem::PostRenderAllSystems();

	FinishCurrentView();

	m_DrawFlags = 0;
}

void CheckAndTransitionColor( float flPercent, float *pColor, float *pLerpToColor )
{
	if ( pLerpToColor[0] != pColor[0] || pLerpToColor[1] != pColor[1] || pLerpToColor[2] != pColor[2] )
	{
		float flDestColor[3];

		flDestColor[0] = pLerpToColor[0];
		flDestColor[1] = pLerpToColor[1];
		flDestColor[2] = pLerpToColor[2];

		pColor[0] = FLerp( pColor[0], flDestColor[0], flPercent );
		pColor[1] = FLerp( pColor[1], flDestColor[1], flPercent );
		pColor[2] = FLerp( pColor[2], flDestColor[2], flPercent );
	}
	else
	{
		pColor[0] = pLerpToColor[0];
		pColor[1] = pLerpToColor[1];
		pColor[2] = pLerpToColor[2];
	}
}

static void GetFogColorTransition( CPlayerLocalData	*local, float *pColorPrimary, float *pColorSecondary )
{
	if ( local == NULL )
		return;

	if ( local->m_fog.lerptime >= gpGlobals->curtime )
	{
		float flPercent = 1.0f - (( local->m_fog.lerptime - gpGlobals->curtime ) / local->m_fog.duration );

		float flPrimaryColorLerp[3] = { local->m_fog.colorPrimaryLerpTo.GetR(), local->m_fog.colorPrimaryLerpTo.GetG(), local->m_fog.colorPrimaryLerpTo.GetB() };
		float flSecondaryColorLerp[3] = { local->m_fog.colorSecondaryLerpTo.GetR(), local->m_fog.colorSecondaryLerpTo.GetG(), local->m_fog.colorSecondaryLerpTo.GetB() };

		CheckAndTransitionColor( flPercent, pColorPrimary, flPrimaryColorLerp );
		CheckAndTransitionColor( flPercent, pColorSecondary, flSecondaryColorLerp );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
static void GetFogColor( float *pColor )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	const char *fogColorString = fog_color.GetString();
	if( fog_override.GetInt() && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor+1, pColor+2 );
	}
	else
	{
		float flPrimaryColor[3] = { local->m_fog.colorPrimary.GetR(), local->m_fog.colorPrimary.GetG(), local->m_fog.colorPrimary.GetB() };
		float flSecondaryColor[3] = { local->m_fog.colorSecondary.GetR(), local->m_fog.colorSecondary.GetG(), local->m_fog.colorSecondary.GetB() };

		GetFogColorTransition( local, flPrimaryColor, flSecondaryColor );

		if( local->m_fog.blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			AngleVectors(pbp->GetAbsAngles(), &forward);
			
			Vector vNormalized = local->m_fog.dirPrimary;
			VectorNormalize( vNormalized );
			local->m_fog.dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( local->m_fog.dirPrimary ) + 0.5;

			// FIXME: convert to linear colorspace
			pColor[0] = flPrimaryColor[0] * flBlendFactor + flSecondaryColor[0] * ( 1 - flBlendFactor );
			pColor[1] = flPrimaryColor[1] * flBlendFactor + flSecondaryColor[1] * ( 1 - flBlendFactor );
			pColor[2] = flPrimaryColor[2] * flBlendFactor + flSecondaryColor[2] * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = flPrimaryColor[0];
			pColor[1] = flPrimaryColor[1];
			pColor[2] = flPrimaryColor[2];
		}
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


static float GetFogStart( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_start.GetFloat() == -1.0f )
		{
			return local->m_fog.start;
		}
		else
		{
			return fog_start.GetFloat();
		}
	}
	else
	{
		if ( local->m_fog.lerptime > gpGlobals->curtime )
		{
			if ( local->m_fog.start != local->m_fog.startLerpTo )
			{
				if ( local->m_fog.lerptime > gpGlobals->curtime )
				{
					float flPercent = 1.0f - (( local->m_fog.lerptime - gpGlobals->curtime ) / local->m_fog.duration );

					return FLerp( local->m_fog.start, local->m_fog.startLerpTo, flPercent );
				}
				else
				{
					if ( local->m_fog.start != local->m_fog.startLerpTo )
					{
						local->m_fog.start = local->m_fog.startLerpTo;
					}
				}
			}
		}

		return local->m_fog.start;
	}
}

static float GetFogEnd( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_end.GetFloat() == -1.0f )
		{
			return local->m_fog.end;
		}
		else
		{
			return fog_end.GetFloat();
		}
	}
	else
	{
		if ( local->m_fog.lerptime > gpGlobals->curtime )
		{
			if ( local->m_fog.end != local->m_fog.endLerpTo )
			{
				if ( local->m_fog.lerptime > gpGlobals->curtime )
				{
					float flPercent = 1.0f - (( local->m_fog.lerptime - gpGlobals->curtime ) / local->m_fog.duration );

					return FLerp( local->m_fog.end, local->m_fog.endLerpTo, flPercent );
				}
				else
				{
					if ( local->m_fog.end != local->m_fog.endLerpTo )
					{
						local->m_fog.end = local->m_fog.endLerpTo;
					}
				}
			}
		}

		return local->m_fog.end;
	}
}

static bool GetFogEnable( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return false;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if ( cl_leveloverview.GetFloat() > 0 )
		return false;

	// Ask the clientmode
	if ( g_pClientMode->ShouldDrawFog() == false )
		return false;

	if( fog_override.GetInt() )
	{
		if( fog_enable.GetInt() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return local->m_fog.enable != false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the skybox fog color to use in rendering the current frame.
//-----------------------------------------------------------------------------
static void GetSkyboxFogColor( float *pColor )
{			   
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	const char *fogColorString = fog_colorskybox.GetString();
	if( fog_override.GetInt() && fogColorString )
	{
		sscanf( fogColorString, "%f%f%f", pColor, pColor+1, pColor+2 );
	}
	else
	{
		if( local->m_skybox3d.fog.blend )
		{
			//
			// Blend between two fog colors based on viewing angle.
			// The secondary fog color is at 180 degrees to the primary fog color.
			//
			Vector forward;
			AngleVectors( pbp->GetAbsAngles(), &forward );
			
			Vector vNormalized = local->m_fog.dirPrimary;
			VectorNormalize( vNormalized );
			local->m_fog.dirPrimary = vNormalized;

			float flBlendFactor = 0.5 * forward.Dot( local->m_fog.dirPrimary ) + 0.5;
						 
			// FIXME: convert to linear colorspace
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetR() * ( 1 - flBlendFactor );
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetG() * ( 1 - flBlendFactor );
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB() * flBlendFactor + local->m_skybox3d.fog.colorSecondary.GetB() * ( 1 - flBlendFactor );
		}
		else
		{
			pColor[0] = local->m_skybox3d.fog.colorPrimary.GetR();
			pColor[1] = local->m_skybox3d.fog.colorPrimary.GetG();
			pColor[2] = local->m_skybox3d.fog.colorPrimary.GetB();
		}
	}

	VectorScale( pColor, 1.0f / 255.0f, pColor );
}


static float GetSkyboxFogStart( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_startskybox.GetFloat() == -1.0f )
		{
			return local->m_skybox3d.fog.start;
		}
		else
		{
			return fog_startskybox.GetFloat();
		}
	}
	else
	{
		return local->m_skybox3d.fog.start;
	}
}

static float GetSkyboxFogEnd( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return 0.0f;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_endskybox.GetFloat() == -1.0f )
		{
			return local->m_skybox3d.fog.end;
		}
		else
		{
			return fog_endskybox.GetFloat();
		}
	}
	else
	{
		return local->m_skybox3d.fog.end;
	}
}

static bool GetSkyboxFogEnable( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return false;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( fog_override.GetInt() )
	{
		if( fog_enableskybox.GetInt() )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return !!local->m_skybox3d.fog.enable;
	}
}

void CViewRender::EnableWorldFog( void )
{
	VPROF("CViewRender::EnableWorldFog");
	if( GetFogEnable() )
	{
		float fogColor[3];
		GetFogColor( fogColor );
		materials->FogMode( MATERIAL_FOG_LINEAR );
		materials->FogColor3fv( fogColor );
		materials->FogStart( GetFogStart() );
		materials->FogEnd( GetFogEnd() );
	}
	else
	{
		materials->FogMode( MATERIAL_FOG_NONE );
	}
}

void CViewRender::Enable3dSkyboxFog( void )
{
	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();
	if( !pbp )
	{
		return;
	}
	CPlayerLocalData	*local		= &pbp->m_Local;

	if( GetSkyboxFogEnable() )
	{
		float fogColor[3];
		GetSkyboxFogColor( fogColor );
		float scale = 1.0f;
		if ( local->m_skybox3d.scale > 0.0f )
		{
			scale = 1.0f / local->m_skybox3d.scale;
		}
		materials->FogMode( MATERIAL_FOG_LINEAR );
		materials->FogColor3fv( fogColor );
		materials->FogStart( GetSkyboxFogStart() * scale );
		materials->FogEnd( GetSkyboxFogEnd() * scale );
	}
	else
	{
		materials->FogMode( MATERIAL_FOG_NONE );
	}
}

void CViewRender::DisableFog( void )
{
	VPROF("CViewRander::DisableFog()");

	materials->FogMode( MATERIAL_FOG_NONE );
}

#include "tier0/memdbgoff.h"

void CViewRender::Draw3dSkyboxworld( const CViewSetup &view, int &nClearFlags, 
									bool &bDrew3dSkybox, bool &bSkyboxVisible )
{
	VPROF_BUDGET( "CViewRender::Draw3dSkyboxworld", "3D Skybox" );

	bDrew3dSkybox = false;

	// The skybox might not be visible from here
	bSkyboxVisible = engine->IsSkyboxVisibleFromPoint( view.origin );

	if ( !bSkyboxVisible && r_3dsky.GetInt() != 2 )
	{
		return;
	}

	// render the 3D skybox
	if ( !r_3dsky.GetInt() )
	{
		return;
	}

	C_BasePlayer *pbp = C_BasePlayer::GetLocalPlayer();

	// No local player object yet...
	if ( !pbp )
		return;

	CPlayerLocalData* local = &pbp->m_Local;
	if ( local->m_skybox3d.area == 255 )
		return;

	unsigned char **areabits = render->GetAreaBits();
	unsigned char *savebits;
	unsigned char tmpbits[ 32 ];
	savebits = *areabits;
	memset( tmpbits, 0, sizeof(tmpbits) );
	
	// set the sky area bit
	tmpbits[local->m_skybox3d.area>>3] |= 1 << (local->m_skybox3d.area&7);

	*areabits = tmpbits;
	CViewSetup skyView = view;
	skyView.zNear = 0.5;
	skyView.zFar = 18000;

	// scale origin by sky scale
	if ( local->m_skybox3d.scale > 0 )
	{
		float scale = 1.0f / local->m_skybox3d.scale;
		VectorScale( skyView.origin, scale, skyView.origin );
	}
	Enable3dSkyboxFog();
	VectorAdd( skyView.origin, local->m_skybox3d.origin, skyView.origin );
	
	skyView.m_vUnreflectedOrigin = skyView.origin;

	// BUGBUG: Fix this!!!  We shouldn't need to call setup vis for the sky if we're connecting
	// the areas.  We'd have to mark all the clusters in the skybox area in the PVS of any 
	// cluster with sky.  Then we could just connect the areas to do our vis.
	//m_bOverrideVisOrigin could hose us here, so call direct
	render->ViewSetupVis( false, 1, &local->m_skybox3d.origin.Get() );
	render->Push3DView( skyView, nClearFlags, false, NULL, m_Frustum );

	// At this point, we've cleared everything we need to clear
	// The next path will need to clear depth, though.
	nClearFlags &= ~(VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH | VIEW_CLEAR_FULL_TARGET);
	nClearFlags |= VIEW_CLEAR_DEPTH;

	// Store off view origin and angles
	SetupCurrentView( skyView.origin, skyView.angles, VIEW_3DSKY );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	ClientWorldListInfo_t info;
#ifndef _XBOX
	CRenderList renderList;
	CRenderList *pRenderList = &renderList;
#else
	void *ptr = MemAllocScratch(sizeof(CRenderList));
	CRenderList *pRenderList = new (ptr) CRenderList;
#endif

	BuildWorldRenderLists( NULL, info, true, true, m_iForceViewLeaf );
	BuildRenderableRenderLists( NULL, info, *pRenderList );

	int flags = DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER | DF_RENDER_WATER;
	if( r_skybox.GetBool() )
	{
		flags |= DF_DRAWSKYBOX;
	}
	
	int oldDrawFlags = m_DrawFlags;
	m_DrawFlags |= flags;
	DrawWorld( info, *pRenderList, flags, 0.0f );

	// Iterate over all leaves and render objects in those leaves
	DrawOpaqueRenderables( info, *pRenderList );

	// Iterate over all leaves and render objects in those leaves
	DrawTranslucentRenderables( info, *pRenderList, flags, true );

	DisableFog();

	CGlowOverlay::UpdateSkyOverlays( skyView.zFar, view.m_bCacheFullSceneState );
	
	PixelVisibility_EndCurrentView();

	// restore old area bits
	*areabits = savebits;

	// Invoke post-render methods
	IGameSystem::PostRenderAllSystems();

	FinishCurrentView();

	m_DrawFlags = oldDrawFlags;

#ifdef _XBOX
	MemFreeScratch();
#endif

	render->PopView( m_Frustum );
	bDrew3dSkybox = true;
}

#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::SetupVis( const CViewSetup& view, unsigned int &visFlags )
{
	VPROF( "CViewRender::SetupVis" );

	if ( m_bOverrideVisOrigin )
	{
		// Pass array or vis origins to merge
		render->ViewSetupVisEx( m_bForceNoVis, m_nNumVisOrigins, m_rgVisOrigins, visFlags );
	}
	else
	{
		// Use render origin as vis origin by default
		render->ViewSetupVisEx( m_bForceNoVis, 1, &view.origin, visFlags );
	}
}

#ifndef _XBOX

// hdr parameters
ConVar mat_hdr_level( "mat_hdr_level", "2" );
ConVar mat_bloomamount_rate( "mat_bloomamount_rate", "0.05f" );
static ConVar debug_postproc( "mat_debug_postprocessing_effects", "0" );
static ConVar split_postproc( "mat_debug_process_halfscreen", "0" );
static ConVar mat_dynamic_tonemapping( "mat_dynamic_tonemapping", "0" );
static ConVar mat_show_ab_hdr( "mat_show_ab_hdr", "0" );
static ConVar mat_tonemapping_occlusion_use_stencil( "mat_tonemapping_occlusion_use_stencil", "1" );
ConVar mat_debug_autoexposure("mat_debug_autoexposure","0");
static ConVar mat_autoexposure_max( "mat_autoexposure_max", "2" );
static ConVar mat_autoexposure_min( "mat_autoexposure_min", "0.5" );
static ConVar mat_show_histogram( "mat_show_histogram", "0" );
ConVar mat_hdr_tonemapscale( "mat_hdr_tonemapscale", "1.0", FCVAR_CHEAT );
ConVar mat_force_bloom("mat_force_bloom","0");
ConVar mat_disable_bloom("mat_disable_bloom","0");
ConVar mat_debug_bloom("mat_debug_bloom","0");

// fudge factor to make non-hdr bloom more closely match hdr bloom. Because of auto-exposure, high
// bloomscales don't blow out as much in hdr. this factor was derived by comparing images in a
// reference scene.
ConVar mat_non_hdr_bloom_scalefactor("mat_non_hdr_bloom_scalefactor",".3");


enum PostProcessingCondition {
    PPP_ALWAYS,
	PPP_IF_COND_VAR,
	PPP_IF_NOT_COND_VAR
};

struct PostProcessingPass {
	PostProcessingCondition ppp_test;
	ConVar const *cvar_to_test;
	char const *material_name;								// terminate list with null
	char const *dest_rendering_target;
	char const *src_rendering_target;						// can be null. needed for source scaling
	int xdest_scale,ydest_scale;							// allows scaling down
	int xsrc_scale,ysrc_scale;								// allows scaling down
	CMaterialReference m_mat_ref;							// so we don't have to keep searching
};

#define PPP_PROCESS_PARTIAL_SRC(srcmatname,dest_rt_name,src_tname,scale) \
	{PPP_ALWAYS,0,srcmatname,dest_rt_name,src_tname,1,1,scale,scale}
#define PPP_PROCESS_PARTIAL_DEST(srcmatname,dest_rt_name,src_tname,scale) \
	{PPP_ALWAYS,0,srcmatname,dest_rt_name,src_tname,scale,scale,1,1}
#define PPP_PROCESS_PARTIAL_SRC_PARTIAL_DEST(srcmatname,dest_rt_name,src_tname,srcscale,destscale) \
	{PPP_ALWAYS,0,srcmatname,dest_rt_name,src_tname,destscale,destscale,srcscale,srcscale}
#define PPP_END 	{PPP_ALWAYS,0,NULL,NULL,0,0,0,0,0}
#define PPP_PROCESS(srcmatname,dest_rt_name) {PPP_ALWAYS,0,srcmatname,dest_rt_name,0,1,1,1,1}
#define PPP_PROCESS_IF_CVAR(cvarptr,srcmatname,dest_rt_name) \
	{PPP_IF_COND_VAR,cvarptr,srcmatname,dest_rt_name,0,1,1,1,1}
#define PPP_PROCESS_IF_NOT_CVAR(cvarptr,srcmatname,dest_rt_name) \
	{PPP_IF_NOT_COND_VAR,cvarptr,srcmatname,dest_rt_name,0,1,1,1,1}
#define PPP_PROCESS_IF_NOT_CVAR_SRCTEXTURE(cvarptr,srcmatname,src_tname,dest_rt_name) \
	{PPP_IF_NOT_COND_VAR,cvarptr,srcmatname,dest_rt_name,src_tname,1,1,1,1}
#define PPP_PROCESS_IF_CVAR_SRCTEXTURE(cvarptr,srcmatname,src_txtrname,dest_rt_name) \
	{PPP_IF_COND_VAR,cvarptr,srcmatname,dest_rt_name,src_txtrname,1,1,1,1}
#define PPP_PROCESS_SRCTEXTURE(srcmatname,src_tname,dest_rt_name) \
	{PPP_ALWAYS,0,srcmatname,dest_rt_name,src_tname,1,1,1,1}

struct ClipBox
{
	int m_minx, m_miny;
	int m_maxx, m_maxy;
};

static void DrawClippedScreenSpaceRectangle( 
	IMaterial *pMaterial,
	int destx, int desty,
	int width, int height,
	float src_texture_x0, float src_texture_y0,			// which texel you want to appear at
	// destx/y
	float src_texture_x1, float src_texture_y1,			// which texel you want to appear at
	// destx+width-1, desty+height-1
	int src_texture_width, int src_texture_height,		// needed for fixup
	ClipBox const *clipbox
	)
{
	if (clipbox)
	{
		if ((destx>clipbox->m_maxx) || (desty>clipbox->m_maxy))
			return;
		if ((destx+width-1<clipbox->m_minx) || (desty+height-1<clipbox->m_miny))
			return;
		// left clip
		if (destx<clipbox->m_minx)
		{
			src_texture_x0=FLerp(src_texture_x0,src_texture_x1,destx,destx+width-1,clipbox->m_minx);
			width-=(clipbox->m_minx-destx);
			destx=clipbox->m_minx;
		}
		// top clip
		if (desty<clipbox->m_miny)
		{
			src_texture_y0=FLerp(src_texture_y0,src_texture_y1,desty,desty+height-1,clipbox->m_miny);
			height-=(clipbox->m_miny-desty);
			desty=clipbox->m_miny;
		}
		// right clip
		if (destx+width-1>clipbox->m_maxx)
		{
			src_texture_x1=FLerp(src_texture_x0,src_texture_x1,destx,destx+width-1,clipbox->m_maxx);
			width=clipbox->m_maxx-destx;
		}
		// bottom clip
		if (desty+height-1>clipbox->m_maxy)
		{
			src_texture_y1=FLerp(src_texture_y0,src_texture_y1,desty,desty+height-1,clipbox->m_maxy);
			height=clipbox->m_maxy-desty;
		}
	}
	materials->DrawScreenSpaceRectangle(pMaterial,destx,desty,width,height,src_texture_x0,
										src_texture_y0,src_texture_x1,src_texture_y1,
										src_texture_width,src_texture_height);
}

void ApplyPostProcessingPasses(PostProcessingPass *pass_list, // table of effects to apply
							   ClipBox const *clipbox=0,	// clipping box for these effects
							   ClipBox *dest_coords_out=0)	// receives dest coords of last blit
{
	ITexture *pSaveRenderTarget = materials->GetRenderTarget();
	int pcount=0;
	if ( debug_postproc.GetInt() ) 
	{
		materials->SetRenderTarget(NULL);
		int dest_width,dest_height;
		materials->GetRenderTargetDimensions( dest_width, dest_height );
		materials->Viewport( 0, 0, dest_width, dest_height );
		materials->ClearColor3ub(255,0,0);
//		materials->ClearBuffers(true,true);
	}
	while(pass_list->material_name)
	{
		bool do_it=true;

		switch(pass_list->ppp_test)
		{
			case PPP_IF_COND_VAR:
				do_it=(pass_list->cvar_to_test)->GetBool();
				break;
			case PPP_IF_NOT_COND_VAR:
				do_it=! ((pass_list->cvar_to_test)->GetBool());
				break;
		}
		if ((pass_list->dest_rendering_target==0) && (debug_postproc.GetInt()))
			do_it=0;
		if (do_it)
		{
			ClipBox const *cb=0;
			if (pass_list->dest_rendering_target==0)
			{
				cb=clipbox;
			}

			IMaterial *src_mat=pass_list->m_mat_ref;
			if (! src_mat)
			{
				src_mat=materials->FindMaterial(pass_list->material_name,
													   TEXTURE_GROUP_OTHER,true);
				if (src_mat)
				{
					pass_list->m_mat_ref.Init(src_mat);
				}
			}
			if (pass_list->dest_rendering_target)
			{
				ITexture *dest_rt=materials->FindTexture(pass_list->dest_rendering_target,
														 TEXTURE_GROUP_RENDER_TARGET);
				materials->SetRenderTarget( dest_rt);
			}
			else
			{
				materials->SetRenderTarget( NULL );
			}
			int dest_width,dest_height;
			materials->GetRenderTargetDimensions( dest_width, dest_height );
			materials->Viewport( 0, 0, dest_width, dest_height );
			dest_width/=pass_list->xdest_scale;
			dest_height/=pass_list->ydest_scale;

			if (pass_list->src_rendering_target)
			{
				ITexture *src_rt=materials->FindTexture(pass_list->src_rendering_target,
														TEXTURE_GROUP_RENDER_TARGET);
				int src_width=src_rt->GetActualWidth();
				int src_height=src_rt->GetActualHeight();
				int ssrc_width=(src_width-1)/pass_list->xsrc_scale;
				int ssrc_height=(src_height-1)/pass_list->ysrc_scale;
				DrawClippedScreenSpaceRectangle(
					src_mat,0,0,dest_width,dest_height,
					0,0,ssrc_width,ssrc_height,src_width,src_height,cb);
				if ((pass_list->dest_rendering_target) && debug_postproc.GetInt())
				{
					materials->SetRenderTarget(NULL);
					int row=pcount/2;
					int col=pcount %2;
					int vdest_width,vdest_height;
					materials->GetRenderTargetDimensions( vdest_width, vdest_height );
					materials->Viewport( 0, 0, vdest_width, vdest_height );
					materials->DrawScreenSpaceRectangle(
						src_mat,col*400,200+row*300,dest_width,dest_height,
						0,0,ssrc_width,ssrc_height,src_width,src_height);
				}
			}
			else
			{
				// just draw the whole source
				if ((pass_list->dest_rendering_target==0) && split_postproc.GetInt())
				{
					DrawClippedScreenSpaceRectangle(src_mat,0,0,dest_width/2,dest_height,
													0,0,.5,1,1,1,cb);
				}
				else
				{
					DrawClippedScreenSpaceRectangle(src_mat,0,0,dest_width,dest_height,
													0,0,1,1,1,1,cb);
				}
				if ((pass_list->dest_rendering_target) && debug_postproc.GetInt())
				{
					materials->SetRenderTarget(NULL);
					int row=pcount/4;
					int col=pcount %4;
					int dest_width,dest_height;
					materials->GetRenderTargetDimensions( dest_width, dest_height );
					materials->Viewport( 0, 0, dest_width, dest_height );
					DrawClippedScreenSpaceRectangle(src_mat,10+col*220,10+row*220,
													200,200,
													0,0,1,1,1,1,cb);
				}	
			}
			if (dest_coords_out)
			{
				dest_coords_out->m_minx=0;
				dest_coords_out->m_maxx=dest_width-1;
				dest_coords_out->m_miny=0;
				dest_coords_out->m_maxy=dest_height-1;
			}
		}
		pass_list++;
		pcount++;
	}
	materials->SetRenderTarget(pSaveRenderTarget);
}

PostProcessingPass HDRFinal_Float[] =
{
 	PPP_PROCESS_SRCTEXTURE("dev/downsample","_rt_FullFrameFB", "_rt_SmallFB0"),
 	PPP_PROCESS("dev/blurfilterx","_rt_SmallFB1"),
 	PPP_PROCESS("dev/blurfiltery","_rt_SmallFB0"),
 	PPP_PROCESS("dev/floattoscreen_combine",NULL),
	PPP_END
};

PostProcessingPass HDRFinal_Float_NoBloom[] =
{
	PPP_PROCESS_SRCTEXTURE("dev/copyfullframefb", "_rt_FullFrameFB",NULL),
	PPP_END
};

PostProcessingPass HDRSimulate_NonHDR[]={
	PPP_PROCESS("dev/copyfullframefb_vanilla",NULL),
	PPP_END
};

static bool IsSplitScreenHDR(void)
{
	return ( mat_show_ab_hdr.GetInt() &&
			 (g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE) );
}

static void SetRenderTargetAndViewPort(ITexture *rt)
{
	materials->SetRenderTarget(rt);
	materials->Viewport(0,0,rt->GetActualWidth(),rt->GetActualHeight());
}

#define FILTER_KERNEL_SLOP 20

// Note carefully about the downsampling: the first downsampling samples from the full rendertarget
// down to a temp. When doing this sampling, the texture source clamping will take care of the out
// of bounds sampling done because of the filter kernels's width. However, on any of the subsequent
// sampling operations, we will be sampling from a partially filled render target. So, texture
// coordinate clamping cannot help us here. So, we need to always render a few more pixels to the
// destination than we actually intend to, so as to replicate the border pixels so that garbage
// pixels do not get sucked into the sampling. To deal with this, we always add FILTER_KERNEL_SLOP
// to our widths/heights if there is room for them in the destination.
static void DrawScreenSpaceRectangleWithSlop( 
	ITexture *dest_rt,
	IMaterial *pMaterial,
	int destx, int desty,
	int width, int height,
	float src_texture_x0, float src_texture_y0,			// which texel you want to appear at
	// destx/y
	float src_texture_x1, float src_texture_y1,			// which texel you want to appear at
	// destx+width-1, desty+height-1
	int src_texture_width, int src_texture_height		// needed for fixup
	)
{
	// add slop
	int slopwidth=width+FILTER_KERNEL_SLOP; //min(dest_rt->GetActualWidth()-destx,width+FILTER_KERNEL_SLOP);
	int slopheight=height+FILTER_KERNEL_SLOP; //min(dest_rt->GetActualHeight()-desty,height+FILTER_KERNEL_SLOP);
	// adjust coordinates for slop
	src_texture_x1=FLerp(src_texture_x0,src_texture_x1,destx,destx+width-1,destx+slopwidth-1);
	src_texture_y1=FLerp(src_texture_y0,src_texture_y1,desty,desty+height-1,desty+slopheight-1);
	width=slopwidth;
	height=slopheight;
	materials->DrawScreenSpaceRectangle(pMaterial,destx,desty,width,height,
										src_texture_x0,src_texture_y0,
										src_texture_x1,src_texture_y1,
										src_texture_width,src_texture_height);

}



enum Histogram_entry_state_t
{
	HESTATE_INITIAL=0,
	HESTATE_FIRST_QUERY_IN_FLIGHT,
	HESTATE_QUERY_IN_FLIGHT,
	HESTATE_QUERY_DONE,
};


ConVar mat_exposure_center_region_x( "mat_exposure_center_region_x","0.75", FCVAR_CHEAT );
ConVar mat_exposure_center_region_y( "mat_exposure_center_region_y","0.80", FCVAR_CHEAT );
ConVar mat_exposure_center_region_x_flashlight( "mat_exposure_center_region_x_flashlight","0.33", FCVAR_CHEAT );
ConVar mat_exposure_center_region_y_flashlight( "mat_exposure_center_region_y_flashlight","0.33", FCVAR_CHEAT );

#define N_LUMINANCE_RANGES 31
#define MAX_QUERIES_PER_FRAME 1

class CHistogram_entry_t
{
public:
	Histogram_entry_state_t m_state;
	OcclusionQueryObjectHandle_t m_occ_handle;				// the occlusion query handle
	int m_frame_queued;										// when this query was last queued
	int m_npixels;										   // # of pixels this histogram represents
	int m_npixels_in_range;
	float m_min_lum, m_max_lum;					 // the luminance range this entry was queried with
	float m_minx,m_miny,m_maxx,m_maxy;					// range is 0..1 in fractions of the screen

	bool ContainsValidData( void )
	{
		return ( m_state==HESTATE_QUERY_DONE ) || ( m_state==HESTATE_QUERY_IN_FLIGHT );
		}

	void IssueQuery( int frm_num )
	{
		if ( ! m_occ_handle)
			m_occ_handle=materials->CreateOcclusionQueryObject();
		int xl,yl,dest_width,dest_height;
		materials->GetViewport( xl,yl,dest_width,dest_height);
		// first, set stencil bits where the colors match
		IMaterial *test_mat=materials->FindMaterial(
			"dev/lumcompare",
			TEXTURE_GROUP_OTHER,true);
		IMaterialVar *pMinVar = test_mat->FindVar( "$C0_X", NULL );
		pMinVar->SetFloatValue( m_min_lum);
		IMaterialVar *pMaxVar = test_mat->FindVar( "$C0_Y", NULL );
		if (m_max_lum==1.0)
			pMaxVar->SetFloatValue( 10000.0);				// count all pixels >1.0 as 1.0
		else
			pMaxVar->SetFloatValue( m_max_lum);
		int scrx_min=FLerp(xl,xl+dest_width-1,0,1,m_minx);
		int scrx_max=FLerp(xl,xl+dest_width-1,0,1,m_maxx);
		int scry_min=FLerp(yl,yl+dest_height-1,0,1,m_miny);
		int scry_max=FLerp(yl,yl+dest_height-1,0,1,m_maxy);

		float exposure_width_scale, exposure_height_scale;

		// now, shrink region of interest if the flashlight is on
		C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
		if ( pLocal->IsEffectActive( EF_DIMLIGHT ) )		// check flashlight
		{
			exposure_width_scale=
				(0.5*(1.0-mat_exposure_center_region_x_flashlight.GetFloat()));
			exposure_height_scale=
				(0.5*(1.0-mat_exposure_center_region_y_flashlight.GetFloat()));
		}
		else
		{
			exposure_width_scale=
				(0.5*(1.0-mat_exposure_center_region_x.GetFloat()));
			exposure_height_scale=
				(0.5*(1.0-mat_exposure_center_region_y.GetFloat()));
		}
		int skip_edgex=(1+scrx_max-scrx_min)*exposure_width_scale;
		int skip_edgey=(1+scry_max-scry_min)*exposure_height_scale;

		// now, do luminance compare
		float tscale=1.0;
		if (g_pMaterialSystemHardwareConfig->GetHDRType()==HDR_TYPE_FLOAT)
		{
			tscale=materials->GetToneMappingScaleLinear().x;
		}
		IMaterialVar *use_t_scale = test_mat->FindVar( "$C0_Z", NULL );
		use_t_scale->SetFloatValue( tscale);
		

		m_npixels=(1+scrx_max-scrx_min)*(1+scry_max-scry_min);
		if (mat_tonemapping_occlusion_use_stencil.GetInt())
		{
			materials->SetStencilWriteMask(1);
			materials->ClearStencilBufferRectangle(scrx_min,scry_min,scrx_max,scry_max,0);
			materials->SetStencilEnable(true);
			materials->SetStencilPassOperation(STENCILOPERATION_REPLACE);
			materials->SetStencilCompareFunction(STENCILCOMPARISONFUNCTION_ALWAYS);
			materials->SetStencilFailOperation(STENCILOPERATION_KEEP);
			materials->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			materials->SetStencilReferenceValue(1);
		}
		else
			materials->BeginOcclusionQueryDrawing(m_occ_handle);
		scrx_min+=skip_edgex;
		scry_min+=skip_edgey;
		scrx_max-=skip_edgex;
		scry_max-=skip_edgey;
		materials->DrawScreenSpaceRectangle(test_mat,
											scrx_min,scry_min,
											1+scrx_max-scrx_min,
											1+scry_max-scry_min,
											scrx_min,scry_min,
											scrx_max, scry_max,
											dest_width,dest_height);
		
		if (mat_tonemapping_occlusion_use_stencil.GetInt())
		{
			// now, start counting how many pixels had their stencil bit set via an occlusion query
			materials->BeginOcclusionQueryDrawing(m_occ_handle);
			// now, issue an occlusion query using stencil as the mask
			materials->SetStencilEnable(true);
			materials->SetStencilTestMask(1);
			materials->SetStencilPassOperation(STENCILOPERATION_KEEP);
			materials->SetStencilCompareFunction(STENCILCOMPARISONFUNCTION_EQUAL);
			materials->SetStencilFailOperation(STENCILOPERATION_KEEP);
			materials->SetStencilZFailOperation(STENCILOPERATION_KEEP);
			materials->SetStencilReferenceValue(1);
			IMaterial *stest_mat=materials->FindMaterial(
				"dev/no_pixel_write",
				TEXTURE_GROUP_OTHER,true);
			materials->DrawScreenSpaceRectangle(stest_mat,
												scrx_min,scry_min,
												1+scrx_max-scrx_min,
												1+scry_max-scry_min,
												scrx_min,scry_min,
												scrx_max, scry_max,
												dest_width,dest_height);
			materials->SetStencilEnable(false);
		}
		materials->EndOcclusionQueryDrawing(m_occ_handle);
		if (m_state==HESTATE_INITIAL)
			m_state=HESTATE_FIRST_QUERY_IN_FLIGHT;
		else
			m_state=HESTATE_QUERY_IN_FLIGHT;
		m_frame_queued=frm_num;
	}

};
			

#define HISTOGRAM_BAR_SIZE 200

class CLuminanceHistogramSystem
{
	CHistogram_entry_t CurHistogram[N_LUMINANCE_RANGES];
	int cur_query_frame;
public:
	float GetAverageLuminance( void )
	{
		float total=0;
		int total_pixels=0;
		float scale_value=1.0;
		if (CurHistogram[N_LUMINANCE_RANGES-1].ContainsValidData())
		{
			scale_value=CurHistogram[N_LUMINANCE_RANGES-1].m_npixels*
				(1.0/CurHistogram[N_LUMINANCE_RANGES-1].m_npixels_in_range);
			if (mat_debug_autoexposure.GetInt())
				Warning("scale value=%f\n",scale_value);
		}
		else
			return 0.5;

		for(int i=0;i<NELEMS( CurHistogram )-1;i++)
		{
			if ( CurHistogram[i].ContainsValidData() )
			{
				total+=scale_value*CurHistogram[i].m_npixels_in_range*
					AVG( CurHistogram[i].m_min_lum,CurHistogram[i].m_max_lum );
				total_pixels+=CurHistogram[i].m_npixels;
			}
			else
				return 0.5;						 // always return 0.5 until we've queried a whole frame
		}
		if (total_pixels > 0)
			return total*(1.0/total_pixels);
		else
			return 0.5;
	}

	void Update(void)
	{
		// find which histogram entries should have something done this frame
		int n_queries_issued_this_frame=0;
		cur_query_frame++;

		for(int i=0;i<NELEMS( CurHistogram );i++)
		{
			switch( CurHistogram[i].m_state )
			{
			case HESTATE_INITIAL:
				if ( n_queries_issued_this_frame<MAX_QUERIES_PER_FRAME )
				{
					CurHistogram[i].IssueQuery(cur_query_frame);
					n_queries_issued_this_frame++;
				}
				break;

			case HESTATE_FIRST_QUERY_IN_FLIGHT:
			case HESTATE_QUERY_IN_FLIGHT:
				if ( cur_query_frame>CurHistogram[i].m_frame_queued+2 )
				{
					int np= materials->OcclusionQuery_GetNumPixelsRendered(
						CurHistogram[i].m_occ_handle);
					if (np!=-1) 						// -1=query not finished. wait until
						// next time
					{
						CurHistogram[i].m_npixels_in_range=np;
						// 						    if (mat_debug_autoexposure.GetInt())
						// 								Warning("min=%f max=%f np = %d\n",CurHistogram[i].m_min_lum,CurHistogram[i].m_max_lum,np);
						CurHistogram[i].m_state=HESTATE_QUERY_DONE;
					}
				}
				break;
			}
		}
		// now, issue queries for the oldest finished queries we have
		while( n_queries_issued_this_frame<MAX_QUERIES_PER_FRAME )
		{
			int oldest_so_far=-1;
			for(int i=0;i<NELEMS( CurHistogram );i++)
				if ( (CurHistogram[i].m_state==HESTATE_QUERY_DONE ) &&
					( (oldest_so_far==-1) || 
					(CurHistogram[i].m_frame_queued<
					CurHistogram[oldest_so_far].m_frame_queued) ) )
					oldest_so_far=i;
			if ( oldest_so_far==-1 )								// nothing to do
				break;
			CurHistogram[oldest_so_far].IssueQuery(cur_query_frame);
			n_queries_issued_this_frame++;
		}
	}

	void DisplayHistogram(void)
	{
		int xp=10;
		for(int l=0;l<N_LUMINANCE_RANGES-1;l++)
		{
			int np=0;
			CHistogram_entry_t &e=CurHistogram[l];
			if (e.ContainsValidData())
				np+=e.m_npixels_in_range;
			int width=500*(e.m_max_lum-e.m_min_lum);
			if (np)
			{
				int height=max(1,min(HISTOGRAM_BAR_SIZE,np/6000));
				materials->Viewport(xp,HISTOGRAM_BAR_SIZE-height,width,height);
				materials->ClearColor3ub(255,0,0);
				materials->ClearBuffers(true,true);
			}
			xp+=width+2;
		}
	}

	CLuminanceHistogramSystem(void)
	{
		cur_query_frame=0;
		for(int bucket=0;bucket<N_LUMINANCE_RANGES;bucket++)
		{
			int idx=bucket;
			CHistogram_entry_t &e=CurHistogram[idx];
			e.m_state=HESTATE_INITIAL;
			e.m_minx=0;
			e.m_maxx=1;
			e.m_miny=0;
			e.m_maxy=1;
			if (bucket!=N_LUMINANCE_RANGES-1)				// last bucket is special
			{
				// use a logarithmic ramp for high range in the low range
				e.m_min_lum=-.01+exp(FLerp(log(.01),log(.01+1),0,N_LUMINANCE_RANGES-1,bucket));
				e.m_max_lum=-.01+exp(FLerp(log(.01),log(.01+1),0,N_LUMINANCE_RANGES-1,bucket+1));
			}
			else
			{
				// the last bucket is used as a test to determine the return range for occlusion
				// queries to use as a scale factor. some boards (nvidia) have their occlusion
				// query return values larger when using AA.
				e.m_min_lum=0;
				e.m_max_lum=100000.0;
			}
		}
	}
};

static CLuminanceHistogramSystem g_HDR_HistogramSystem;

static float s_MovingAverageToneMapScale[10];
static int s_nInAverage;

void ResetToneMapping(float value)
{
	s_nInAverage=0;
	materials->ResetToneMappingScale(value);
}

static void SetToneMapScale(float newvalue, float minvalue, float maxvalue)
{
	mat_hdr_tonemapscale.SetValue(newvalue);
#if 1
	if (s_nInAverage<NELEMS(s_MovingAverageToneMapScale))
		s_MovingAverageToneMapScale[s_nInAverage++]=newvalue;
	else
	{
		// scroll, losing oldest
		for(int i=0;i<NELEMS(s_MovingAverageToneMapScale)-1;i++)
			s_MovingAverageToneMapScale[i]=s_MovingAverageToneMapScale[i+1];
		s_MovingAverageToneMapScale[NELEMS(s_MovingAverageToneMapScale)-1]=newvalue;
	}
	// now, use the average of the last tonemap calculations as our goal scale
	if (s_nInAverage==NELEMS(s_MovingAverageToneMapScale))	// got full buffer yet?
	{
		float avg=0.;
		float sumweights=0;
		int sample_pt=NELEMS(s_MovingAverageToneMapScale)/2;
		for(int i=0;i<NELEMS(s_MovingAverageToneMapScale);i++)
		{
			float weight=abs(i-sample_pt)*(1.0/(NELEMS(s_MovingAverageToneMapScale)/2));
			sumweights+=weight;
			avg+=weight*s_MovingAverageToneMapScale[i];
		}
		avg*=(1.0/sumweights);
		avg=min(maxvalue,max(minvalue,avg));
		mat_hdr_tonemapscale.SetValue(avg);
	}
#endif
}

static void DoPostProcessingEffects( int x, int y, int w, int h )
{
	bool bloom_enabled = (mat_hdr_level.GetInt() >= 1);
	if ( !engine->MapHasHDRLighting() )
		bloom_enabled = false;
	if ( mat_force_bloom.GetInt() )
		bloom_enabled = true;
	if ( mat_disable_bloom.GetInt() )
		bloom_enabled = false;
	if ( building_cubemaps.GetBool() )
		bloom_enabled = false;

	HDRType_t hdrType = g_pMaterialSystemHardwareConfig->GetHDRType();

	float bloom_coeff=0.0;
	if (bloom_enabled)
	{
		static float currentBloomAmount = 1.0f;
		float rate = mat_bloomamount_rate.GetFloat();

		// Use the appropriate bloom scale settings.  Mapmakers's overrides the convar settings.
		float flCurrentBloomScale = 1.0f;
		if ( g_bUseCustomBloomScale )
		{
			flCurrentBloomScale = g_flCustomBloomScale;
		}
		else
		{
			flCurrentBloomScale = mat_bloomscale.GetFloat();
		}

		currentBloomAmount = flCurrentBloomScale * rate + ( 1.0f - rate ) * currentBloomAmount;
		bloom_coeff=currentBloomAmount;
	}

	if ( hdrType == HDR_TYPE_NONE )
		bloom_coeff *= mat_non_hdr_bloom_scalefactor.GetFloat();

	// Use the appropriate autoexposure min / max settings.
	// Mapmaker's overrides the convar settings.
	float flAutoExposureMin;
	float flAutoExposureMax;
	if ( g_bUseCustomAutoExposureMin )
	{
		flAutoExposureMin = g_flCustomAutoExposureMin;
	}
	else
	{
		flAutoExposureMin = mat_autoexposure_min.GetFloat();
	}
	if ( g_bUseCustomAutoExposureMax )
	{
		flAutoExposureMax = g_flCustomAutoExposureMax;
	}
	else
	{
		flAutoExposureMax = mat_autoexposure_max.GetFloat();
	}

	switch( hdrType )
	{
		case HDR_TYPE_NONE:
		case HDR_TYPE_INTEGER:
		{
			if (mat_debug_bloom.GetInt()==1)
			{
				static int wx=0;
				wx=(wx+1) & 63;
				materials->SetRenderTarget(NULL);
				int dest_width,dest_height;
				materials->GetRenderTargetDimensions( dest_width, dest_height );
				materials->Viewport( 0, 0, dest_width, dest_height );
				materials->ClearColor3ub(0,0,0);
				materials->ClearBuffers(true,true);
				materials->Viewport( wx+100, 100, 20,20 );
				materials->ClearColor3ub(255,255,255);
				materials->ClearBuffers(true,true);
				materials->Viewport( 20,20,20,20 );
				materials->ClearBuffers(true,true);
				materials->Viewport( 950,20,20,20 );
				materials->ClearBuffers(true,true);
				materials->Viewport( 950,950,20,20 );
				materials->ClearBuffers(true,true);
				materials->Viewport( 20,950,20,20 );
				materials->ClearBuffers(true,true);
				materials->Viewport( 0, 0, dest_width, dest_height );
			}
			bool did_update=false;
			if ( ( hdrType != HDR_TYPE_NONE && mat_dynamic_tonemapping.GetInt() ) || mat_show_histogram.GetInt())
			{
				did_update=true;
// 				if (mat_debug_autoexposure.GetInt())
// 				{
// 					materials->ClearColor3ub(128,0,0);
// 					materials->ClearBuffers(true,true);
// 				}
				Rect_t actualRect;
				UpdateScreenEffectTexture( 0, x, y, w, h, false, &actualRect );
//				UpdateScreenEffectTexture( 0, x, y, w, h, true );
				int dest_width,dest_height;
				materials->GetRenderTargetDimensions( dest_width, dest_height );
				g_HDR_HistogramSystem.Update();
				float avg_lum=g_HDR_HistogramSystem.GetAverageLuminance();
				float last_scale=materials->GetToneMappingScaleLinear().x;
				float actual_unscaled_luminance=avg_lum*(1.0/last_scale);
				actual_unscaled_luminance=max(0.000000001,avg_lum);
				float target_scale=0.005/actual_unscaled_luminance;
				float scalevalue=max(flAutoExposureMin,
									 min(flAutoExposureMax,target_scale));
				if (mat_debug_autoexposure.GetInt())
					Warning("avg_lum=%f ra=%f target=%f adj target=%f\n",
							avg_lum,actual_unscaled_luminance,target_scale,scalevalue);
				if (mat_dynamic_tonemapping.GetInt())
					SetToneMapScale(scalevalue,flAutoExposureMin,flAutoExposureMax);
			}
			// bloom
			if ( bloom_enabled && ( bloom_coeff > 0.0 ) && ( engine->GetDXSupportLevel() >= 80 ) )
			{
				materials->SetRenderTarget(NULL);
				int dest_width,dest_height;
				materials->GetRenderTargetDimensions( dest_width, dest_height );
				materials->Viewport( 0, 0, dest_width, dest_height );
				if ( !did_update )
				{
					UpdateScreenEffectTexture( 0, x, y, w, h, true );
					did_update=true;
				}

				IMaterial *downsample_mat = materials->FindMaterial( "dev/downsample_non_hdr", TEXTURE_GROUP_OTHER, true);
				IMaterial *xblur_mat = materials->FindMaterial( "dev/blurfilterx_nohdr", TEXTURE_GROUP_OTHER, true );
				IMaterial *yblur_mat = materials->FindMaterial( "dev/blurfiltery_and_add_nohdr", TEXTURE_GROUP_OTHER, true );
				ITexture *dest_rt0 = materials->FindTexture( "_rt_SmallFB0", TEXTURE_GROUP_RENDER_TARGET );
				ITexture *dest_rt1 = materials->FindTexture( "_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET );

				// downsample fb to rt0
				SetRenderTargetAndViewPort( dest_rt0 );
				// note the -2's below. Thats because we are downsampling on each axis and the shader
				// accesses pixels on both sides of the source coord
				materials->DrawScreenSpaceRectangle( downsample_mat, 0, 0, dest_width/4, dest_height/4,
													0, 0, dest_width-2, dest_height-2,
													dest_width, dest_height );
				// guassian blur x rt0 to rt1
				SetRenderTargetAndViewPort( dest_rt1 );
				materials->DrawScreenSpaceRectangle( xblur_mat, 0, 0, dest_width/4, dest_height/4,
													0, 0, dest_width/4-1, dest_height/4-1,
													dest_width/4, dest_height/4 );
				// gaussian blur y and add rt1 to frame buffer
				materials->Viewport( 0, 0, dest_width, dest_height );
				materials->SetRenderTarget( NULL );
				IMaterialVar *pBloomAmountVar = yblur_mat->FindVar( "$bloomamount", NULL );
				pBloomAmountVar->SetFloatValue( bloom_coeff );
				int bloomadd_x=0;
				int bloomadd_y=0;
				int bloomadd_width=dest_width;
				int bloomadd_height=dest_height;
				float bloomsrc_x=0;
				float bloomsrc_y=-.5;						// compensate for phase shift due to mismatched input/output sizes
				float bloomsrc_maxx=dest_width/4-1;
				float bloomsrc_maxy=dest_height/4-1;
				if ( IsSplitScreenHDR() )
				{
					bloomadd_x+=dest_width/2;
					bloomadd_width-=dest_width/2;
					bloomsrc_x+=dest_width/8;
				}
				materials->DrawScreenSpaceRectangle(yblur_mat,bloomadd_x,bloomadd_y,
													bloomadd_width,bloomadd_height,
													bloomsrc_x,bloomsrc_y,
													bloomsrc_maxx,bloomsrc_maxy,
													dest_rt1->GetActualWidth(),
													dest_rt1->GetActualHeight());
			}

			if ( mat_show_histogram.GetInt() && ( engine->GetDXSupportLevel() >= 90 ) )
			{
				g_HDR_HistogramSystem.DisplayHistogram();
			}
		}
		break;

		case HDR_TYPE_FLOAT:
		{
			int dest_width,dest_height;
			materials->GetRenderTargetDimensions( dest_width, dest_height );
			if (mat_dynamic_tonemapping.GetInt() || mat_show_histogram.GetInt())
			{
				g_HDR_HistogramSystem.Update();
//				Warning("avg_lum=%f\n",g_HDR_HistogramSystem.GetAverageLuminance());
				if (mat_dynamic_tonemapping.GetInt())
				{
					float avg_lum=max(0.0001,g_HDR_HistogramSystem.GetAverageLuminance());
					float scalevalue=max(flAutoExposureMin,
										 min(flAutoExposureMax,0.18/avg_lum));
					mat_hdr_tonemapscale.SetValue(scalevalue);
				}
			}
			IMaterial *pBloomMaterial;
			pBloomMaterial = materials->FindMaterial( "dev/floattoscreen_combine", "" );
			IMaterialVar *pBloomAmountVar = pBloomMaterial->FindVar( "$bloomamount", NULL );
			pBloomAmountVar->SetFloatValue( bloom_coeff);
			if ( IsSplitScreenHDR() )
			{
				// do split screen
				ClipBox mycb;
				mycb.m_minx=mycb.m_miny=0;
				mycb.m_maxx=dest_width/2;
				mycb.m_maxy=dest_height-1;
				ApplyPostProcessingPasses(HDRSimulate_NonHDR,&mycb);
				mycb.m_minx=mycb.m_maxx;
				mycb.m_maxx=dest_width-1;
				ApplyPostProcessingPasses(HDRFinal_Float,&mycb);
			}
			else
			{
				if( bloom_enabled)
					ApplyPostProcessingPasses(HDRFinal_Float);
				else
					ApplyPostProcessingPasses(HDRFinal_Float_NoBloom);
			}
			materials->SetRenderTarget(NULL);
			if ( mat_show_histogram.GetInt() && (engine->GetDXSupportLevel()>=90))
				g_HDR_HistogramSystem.DisplayHistogram();
			if (mat_dynamic_tonemapping.GetInt())
			{
				float avg_lum=max(0.0001,g_HDR_HistogramSystem.GetAverageLuminance());
				float scalevalue=max(flAutoExposureMin,
					min(flAutoExposureMax,0.023/avg_lum));
				SetToneMapScale(scalevalue,flAutoExposureMin,flAutoExposureMax);
			}
			materials->SetRenderTarget( NULL );
			break;
		}
	}
}
#else
void ResetToneMapping(float value)
{
}
#endif // !_XBOX

#ifdef _XBOX

#define FULLSCREEN_WIDTH	640
#define FULLSCREEN_HEIGHT	480
#define DOWNSAMPLE_WIDTH	(FULLSCREEN_WIDTH>>1)
#define DOWNSAMPLE_HEIGHT	(FULLSCREEN_HEIGHT>>1)

void CViewRender::DoScreenSpaceBloom()
{
	int rt_width, rt_height;

	if ( !mat_bloomscale.GetFloat() || !m_BloomDownsample.IsValid() )
	{
		return;
	}

	// rt_waterReflection = DownSample( backBuffer )
	ITexture *dest_rt = GetWaterReflectionTexture();
	materials->PushRenderTargetAndViewport( dest_rt );
	materials->GetRenderTargetDimensions( rt_width, rt_height );
	Assert( rt_width >= DOWNSAMPLE_WIDTH && rt_height >= DOWNSAMPLE_HEIGHT );
	materials->DrawScreenSpaceRectangle(
		m_BloomDownsample, 0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT,
		0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, 1, 1 );

	// rt_waterRefraction = BlurX( rt_waterReflection )
	dest_rt = GetWaterRefractionTexture();
	materials->SetRenderTarget( dest_rt );
	materials->DrawScreenSpaceRectangle(
		m_BloomBlurX, 0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT,
		0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT, rt_width, rt_height );

	// rt_waterReflection = BlurY( rt_waterRefraction )
	dest_rt = GetWaterReflectionTexture();
	materials->SetRenderTarget( dest_rt );
	materials->DrawScreenSpaceRectangle(
		m_BloomBlurY, 0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT,
		0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT, rt_width, rt_height );

	// restore screen
	materials->PopRenderTargetAndViewport();

	// FrameBuffer += rt_waterReflection
	materials->DrawScreenSpaceRectangle(
		m_BloomAdd, 0, 0, FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT,
		0, 0, DOWNSAMPLE_WIDTH, DOWNSAMPLE_HEIGHT, rt_width, rt_height );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Renders voice feedback and other sprites attached to players
// Input  : none
//-----------------------------------------------------------------------------
void CViewRender::RenderPlayerSprites()
{
#ifndef _XBOX
	GetClientVoiceMgr()->DrawHeadLabels();
#endif
}

//-----------------------------------------------------------------------------
// Sets up, cleans up the main 3D view
//-----------------------------------------------------------------------------
void CViewRender::SetupMain3DView( const CViewSetup &view, int &nClearFlags )
{
	// FIXME: I really want these fields removed from CViewSetup 
	// and passed in as independent flags
	// Clear the color here if requested.
	nClearFlags &= ~VIEW_CLEAR_DEPTH;
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		nClearFlags |= VIEW_CLEAR_DEPTH;
	}

	// If we are using HDR, we render to the HDR full frame buffer texture
	// instead of whatever was previously the render target
	if( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_FLOAT )
	{
		render->Push3DView( view, nClearFlags, true, GetFullFrameFrameBufferTexture( 0 ), m_Frustum );
	}
	else
	{
		render->Push3DView( view, nClearFlags, false, NULL, m_Frustum );
	}

	// If we didn't clear the depth here, we'll need to clear it later
	nClearFlags ^= VIEW_CLEAR_DEPTH;
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		// If we cleared the color here, we don't need to clear it later
		nClearFlags &= ~( VIEW_CLEAR_COLOR | VIEW_CLEAR_FULL_TARGET );
	}
}

void CViewRender::CleanupMain3DView( const CViewSetup &view )
{
	render->PopView( m_Frustum );
}


//-----------------------------------------------------------------------------
// Queues up an overlay rendering
//-----------------------------------------------------------------------------
void CViewRender::QueueOverlayRenderView( const CViewSetup &view, int nClearFlags, int whatToDraw )
{
#ifndef _XBOX
	// Can't have 2 in a single scene
	Assert( !m_bDrawOverlay );
    m_bDrawOverlay = true;
	m_OverlayViewSetup = view;
	m_OverlayClearFlags = nClearFlags;
	m_OverlayDrawFlags = whatToDraw;
#endif
}

	
//-----------------------------------------------------------------------------
// Purpose: This renders the entire 3D view and the in-game hud/viewmodel
// Input  : &view - 
//			whatToDraw - 
//-----------------------------------------------------------------------------
// This renders the entire 3D view.
void CViewRender::RenderViewEx( const CViewSetup &view, int nClearFlags, int whatToDraw )
{
	m_CurrentView = view;

	VPROF( "CViewRender::RenderViewEx" );

	ITexture *saveRenderTarget = materials->GetRenderTarget();

	g_pClientShadowMgr->AdvanceFrame();

#ifdef USE_MONITORS
	if ( cl_drawmonitors.GetBool() && g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 70 )
	{
		DrawMonitors( view );	
	}
#endif

	g_bRenderingView = true;

	// Must be first 
	render->SceneBegin();
	materials->TurnOnToneMapping();

	// clear happens here probably
	SetupMain3DView( view, nClearFlags );
		 	  
	bool bDrew3dSkybox = false;
	bool bSkyboxVisible = false;

	// if the 3d skybox world is drawn, then don't draw the normal skybox
	Draw3dSkyboxworld( view, nClearFlags, bDrew3dSkybox, bSkyboxVisible );

	// Force it to clear the framebuffer if they're in solid space.
	if ( ( nClearFlags & VIEW_CLEAR_COLOR ) == 0 )
	{
		if ( enginetrace->GetPointContents( view.origin ) == CONTENTS_SOLID )
		{
			nClearFlags |= VIEW_CLEAR_COLOR;
		}
	}

	// Render world and all entities, particles, etc.
	if( !g_pIntroData )
	{
		ViewDrawScene( bDrew3dSkybox, bSkyboxVisible, view, nClearFlags, VIEW_MAIN, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );
	}
	else
	{
		ViewDrawScene_Intro( view, nClearFlags, *g_pIntroData );
	}

	// We can still use the 'current view' stuff set up in ViewDrawScene
	s_bCanAccessCurrentView = true;

	if ( !IsXbox() )
	{
		engine->DrawPortals();
	}

	DisableFog();

	// Finish scene
	render->SceneEnd();

	// Draw lightsources if enabled
	render->DrawLights();

	RenderPlayerSprites();
									  
	// Now actually draw the viewmodel
	DrawViewModels( view, whatToDraw & RENDERVIEW_DRAWVIEWMODEL );

	PixelVisibility_EndScene();

	// Draw fade over entire screen if needed
	byte color[4];
	bool blend;
	vieweffects->GetFadeParams( view.context, &color[0], &color[1], &color[2], &color[3], &blend );

	// Draw an overlay to make it even harder to see inside smoke particle systems.
	DrawSmokeFogOverlay();

	// Overlay screen fade on entire screen
	IMaterial* pMaterial = blend ? m_ModulateSingleColor : m_TranslucentSingleColor;
	render->ViewDrawFade( color, pMaterial );
	PerformScreenOverlay( view.x, view.y, view.width, view.height );

	// Prevent sound stutter if going slow
	engine->Sound_ExtraUpdate();	
	    
#ifndef _XBOX
	if ( !building_cubemaps.GetBool() && view.m_bDoBloomAndToneMapping )
	{
		DoPostProcessingEffects( view.x, view.y, view.width, view.height );
	}
#else
	DoScreenSpaceBloom();
#endif

	// And here are the screen-space effects

	// Grab the pre-color corrected frame for editing purposes
	engine->GrabPreColorCorrectedFrame( view.x, view.y, view.width, view.height );

	PerformScreenSpaceEffects( view.x, view.y, view.width, view.height );
	if ( !IsXbox() && g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_INTEGER )
	{
		materials->SetToneMappingScaleLinear(Vector(1,1,1));
	}

	CleanupMain3DView( view );

	materials->SetRenderTarget( saveRenderTarget );

#ifndef _XBOX
	// Draw the overlay
	if ( m_bDrawOverlay )
	{	   
		// This allows us to be ok if there are nested overlay views
		CViewSetup currentView = m_CurrentView;
		CViewSetup tempView = m_OverlayViewSetup;
		tempView.fov = ScaleFOVByWidthRatio( tempView.fov, tempView.m_flAspectRatio / ( 4.0f / 3.0f ) );
		tempView.m_bDoBloomAndToneMapping = false;	// FIXME: Hack to get Mark up and running
		m_bDrawOverlay = false;
		RenderViewEx( tempView, m_OverlayClearFlags, m_OverlayDrawFlags );
		m_CurrentView = currentView;
	}
#endif

	// Draw the 2D graphics
	render->Push2DView( view, 0, false, NULL, m_Frustum );

	Render2DEffectsPreHUD( view );

	if ( whatToDraw & RENDERVIEW_DRAWHUD )
	{
		VPROF_BUDGET( "VGui_DrawHud", VPROF_BUDGETGROUP_OTHER_VGUI );
		// paint the vgui screen
		VGui_PreRender();

		// Make sure the client .dll root panel is at the proper point before doing the "SolveTraverse" calls
		vgui::VPANEL root = enginevgui->GetPanel( PANEL_CLIENTDLL );
		if ( root != 0 )
		{
			vgui::ipanel()->SetPos( root, view.x, view.y );
			vgui::ipanel()->SetSize( root, view.width, view.height );
		}
		// Same for client .dll tools
		root = enginevgui->GetPanel( PANEL_CLIENTDLL_TOOLS );
		if ( root != 0 )
		{
			vgui::ipanel()->SetPos( root, view.x, view.y );
			vgui::ipanel()->SetSize( root, view.width, view.height );
		}

		// The crosshair, etc. needs to get at the current setup stuff
		AllowCurrentViewAccess( true );

		// Draw the in-game stuff based on the actual viewport being used
		render->VGui_Paint( PAINT_INGAMEPANELS );

		AllowCurrentViewAccess( false );

		VGui_PostRender();

		g_pClientMode->PostRenderVGui();
		materials->Flush();
	}

	Draw2DDebuggingInfo( view );

	m_AnglesHistory[m_AnglesHistoryCounter] = view.angles;
	m_AnglesHistoryCounter = (m_AnglesHistoryCounter+1) & ANGLESHISTORY_MASK;
	
	// If the angles are moving fast enough, allow LOD transitions.
	float angleMovementDelta = 0;
	for(int i=0; i < ANGLESHISTORY_SIZE; i++)
	{
		angleMovementDelta += (m_AnglesHistory[(i+1) & ANGLESHISTORY_MASK] - m_AnglesHistory[i]).Length();
	}

	angleMovementDelta /= ANGLESHISTORY_SIZE;
	if(angleMovementDelta > r_TransitionSensitivity.GetFloat())
	{
		r_DoCovertTransitions.SetValue(1);
	}
	else
	{
		r_DoCovertTransitions.SetValue(0);
	}

	Render2DEffectsPostHUD( view );

	g_bRenderingView = false;

	// We can no longer use the 'current view' stuff set up in ViewDrawScene
	s_bCanAccessCurrentView = false;

	// Next frame!
	++m_FrameNumber;

	if ( !IsXbox() )
	{
		GenerateOverdrawForTesting();
	}

	render->PopView( m_Frustum );
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPreHUD( const CViewSetup &view )
{
}

//-----------------------------------------------------------------------------
// Purpose: Renders extra 2D effects in derived classes while the 2D view is on the stack
//-----------------------------------------------------------------------------
void CViewRender::Render2DEffectsPostHUD( const CViewSetup &view )
{
}

//-----------------------------------------------------------------------------
// Purpose: Renders entire view
// Input  : &view - 
//			drawViewModel - 
//-----------------------------------------------------------------------------
void CViewRender::RenderView( const CViewSetup &view, int nClearFlags, bool bDrawViewModel )
{
	int flags = RENDERVIEW_DRAWHUD;
	if ( bDrawViewModel )
	{
		flags |= RENDERVIEW_DRAWVIEWMODEL;
	}
	RenderViewEx( view, nClearFlags, flags );
}


int CViewRender::GetDrawFlags()
{
	return m_DrawFlags;
}


void ViewTransform( const Vector &worldSpace, Vector &viewSpace )
{
	const VMatrix &viewMatrix = engine->WorldToViewMatrix();
	Vector3DMultiplyPosition( viewMatrix, worldSpace, viewSpace );
}


//-----------------------------------------------------------------------------
// Purpose: UNDONE: Clean this up some, handle off-screen vertices
// Input  : *point - 
//			*screen - 
// Output : int
//-----------------------------------------------------------------------------
int ScreenTransform( const Vector& point, Vector& screen )
{
// UNDONE: Clean this up some, handle off-screen vertices
	float w;
	const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

	screen.x = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen.y = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
//	z		 = worldToScreen[2][0] * point[0] + worldToScreen[2][1] * point[1] + worldToScreen[2][2] * point[2] + worldToScreen[2][3];
	w		 = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];

	// Just so we have something valid here
	screen.z = 0.0f;

	bool behind;
	if( w < 0.001f )
	{
		behind = true;
		screen.x *= 100000;
		screen.y *= 100000;
	}
	else
	{
		behind = false;
		float invw = 1.0f / w;
		screen.x *= invw;
		screen.y *= invw;
	}

	return behind;
}



//-----------------------------------------------------------------------------
//
// NOTE: Below here is all of the stuff that needs to be done for water rendering
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Determines what kind of water we're going to use
//-----------------------------------------------------------------------------
void CViewRender::DetermineWaterRenderInfo( const VisibleFogVolumeInfo_t &fogVolumeInfo, CViewRender::WaterRenderInfo_t &info )
{
	// By default, assume cheap water (even if there's no water in the scene!)
	info.m_bCheapWater = true;
	info.m_bRefract = false;
	info.m_bReflect = false;
	info.m_bReflectEntities = false;
	info.m_bDrawWaterSurface = false;
	info.m_bOpaqueWater = true;

	IMaterial *pWaterMaterial = fogVolumeInfo.m_pFogVolumeMaterial;
	if (( fogVolumeInfo.m_nVisibleFogVolume == -1 ) || !pWaterMaterial )
		return;

	// Use cheap water if mat_drawwater is set
	info.m_bDrawWaterSurface = mat_drawwater.GetBool();
	if ( !info.m_bDrawWaterSurface )
	{
		info.m_bOpaqueWater = false;
		return;
	}

	// Determine if the water surface is opaque or not
	info.m_bOpaqueWater = !pWaterMaterial->IsTranslucent();

	// DX level 70 can't handle anything but cheap water
	if (engine->GetDXSupportLevel() < 80)
		return;

	bool bForceCheap = false;
	bool bForceExpensive = r_waterforceexpensive.GetBool();

	// The material can override the default settings though
	IMaterialVar *pForceCheapVar = pWaterMaterial->FindVar( "$forcecheap", NULL, false );
	IMaterialVar *pForceExpensiveVar = pWaterMaterial->FindVar( "$forceexpensive", NULL, false );
	if ( pForceCheapVar && pForceCheapVar->IsDefined() )
	{
		bForceCheap = ( pForceCheapVar->GetIntValue() != 0 );
		if ( bForceCheap )
		{
			bForceExpensive = false;
		}
	}
	if ( !bForceCheap && pForceExpensiveVar && pForceExpensiveVar->IsDefined() )
	{
		 bForceExpensive = bForceExpensive || ( pForceExpensiveVar->GetIntValue() != 0 );
	}

	bool bDebugCheapWater = r_debugcheapwater.GetBool();
	if( bDebugCheapWater )
	{
		Msg( "Water material: %s dist to water: %f\nforcecheap: %s forceexpensive: %s\n", 
			pWaterMaterial->GetName(), fogVolumeInfo.m_flDistanceToWater, 
			bForceCheap ? "true" : "false", bForceExpensive ? "true" : "false" );
	}

	// Unless expensive water is active, reflections are off.
	bool bLocalReflection;
	if( !bForceExpensive || !r_WaterDrawReflection.GetBool() )
	{
		bLocalReflection = false;
	}
	else
	{
		IMaterialVar *pReflectTextureVar = pWaterMaterial->FindVar( "$reflecttexture", NULL, false );
		bLocalReflection = pReflectTextureVar && (pReflectTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);
	}

	// FIXME: I disabled cheap water LOD when local specular is specified.
	// There are very few places that appear to actually
	// take advantage of it (places where water is in the PVS, but outside of LOD range).
	// It was 2 hours before code lock, and I had the choice of either doubling fill-rate everywhere
	// by making cheap water lod actually work (the water LOD wasn't actually rendering!!!)
	// or to just always render the reflection + refraction if there's a local specular specified.
	// Note that water LOD *does* work with refract-only water

	// Check if the water is out of the cheap water LOD range; if so, use cheap water
	if ( ( (fogVolumeInfo.m_flDistanceToWater >= m_flCheapWaterEndDistance) && !bLocalReflection ) || bForceCheap )
 		return;

	// Get the material that is for the water surface that is visible and check to see
	// what render targets need to be rendered, if any.
	if ( !r_WaterDrawRefraction.GetBool() )
	{
		info.m_bRefract = false;
	}
	else
	{
		IMaterialVar *pRefractTextureVar = pWaterMaterial->FindVar( "$refracttexture", NULL, false );
		info.m_bRefract = pRefractTextureVar && (pRefractTextureVar->GetType() == MATERIAL_VAR_TYPE_TEXTURE);

		// Refractive water can be seen through
		if ( info.m_bRefract )
		{
			info.m_bOpaqueWater = false;
		}
	}

#ifndef _XBOX
	info.m_bReflect = bLocalReflection;
	if ( info.m_bReflect )
	{
		if( r_waterforcereflectentities.GetBool() )
		{
			info.m_bReflectEntities = true;
		}
		else
		{
			IMaterialVar *pReflectEntitiesVar = pWaterMaterial->FindVar( "$reflectentities", NULL, false );
			info.m_bReflectEntities = pReflectEntitiesVar && (pReflectEntitiesVar->GetIntValue() != 0);
		}
	}
#endif

	info.m_bCheapWater = !info.m_bReflect && !info.m_bRefract;

	if( bDebugCheapWater )
	{
		Warning( "refract: %s reflect: %s\n", info.m_bRefract ? "true" : "false", info.m_bReflect ? "true" : "false" );
	}
}

//-----------------------------------------------------------------------------
// Draws the world and all entities
//-----------------------------------------------------------------------------
void CViewRender::WaterDrawWorldAndEntities( bool bDrawSkybox, const CViewSetup &viewIn, int nClearFlags )
{
	MDLCACHE_CRITICAL_SECTION();

	VisibleFogVolumeInfo_t fogVolumeInfo;
	render->GetVisibleFogVolume( viewIn.origin, &fogVolumeInfo );

	WaterRenderInfo_t info;
	DetermineWaterRenderInfo( fogVolumeInfo, info );

	if ( info.m_bCheapWater )
	{
		ViewDrawScene_NoWater( bDrawSkybox, viewIn, nClearFlags, fogVolumeInfo, info );
		return;
	}

	// Blat out the visible fog leaf if we're not going to use it
	if ( !r_ForceWaterLeaf.GetBool() )
	{
		fogVolumeInfo.m_nVisibleFogVolumeLeaf = -1;
	}

	// We can see water of some sort
	if ( !fogVolumeInfo.m_bEyeInFogVolume )
	{
		ViewDrawScene_EyeAboveWater( bDrawSkybox, viewIn, nClearFlags, fogVolumeInfo, info );
	}
	else
	{
		ViewDrawScene_EyeUnderWater( bDrawSkybox, viewIn, nClearFlags, fogVolumeInfo, info );
	}
}


//-----------------------------------------------------------------------------
// Pushes a water render target
//-----------------------------------------------------------------------------
static Vector SavedLinearLightMapScale(-1,-1,-1);			// x<0 = no saved scale

static void SetLightmapScaleForWater(void)
{
	if (g_pMaterialSystemHardwareConfig->GetHDRType()==HDR_TYPE_INTEGER)
	{
		SavedLinearLightMapScale=materials->GetToneMappingScaleLinear();
		Vector t25=SavedLinearLightMapScale;
		t25*=0.25;
		materials->SetToneMappingScaleLinear(t25);
	}
}
void CViewRender::PushWaterRenderTarget( CViewSetup &view, int nClearFlags, float waterHeight, int flags )
{
	float spread = 2.0f;
	float origWaterHeight = waterHeight;
	if( flags & DF_FUDGE_UP )
	{
		waterHeight += spread;
	}
	else
	{
		waterHeight -= spread;
	}

	MaterialHeightClipMode_t clipMode = MATERIAL_HEIGHTCLIPMODE_DISABLE;
	if ( ( flags & DF_CLIP_Z ) && mat_clipz.GetBool() )
	{
		if( flags & DF_CLIP_BELOW )
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_ABOVE_HEIGHT;
		}
		else
		{
			clipMode = MATERIAL_HEIGHTCLIPMODE_RENDER_BELOW_HEIGHT;
		}
	}

	if( flags & DF_RENDER_REFRACTION )
	{
		ITexture *pTexture = GetWaterRefractionTexture();

		// Use the aspect ratio of the main view! So, don't recompute it here
		view.x = view.y = 0;
		view.width = pTexture->GetActualWidth();
		view.height = pTexture->GetActualHeight();

		materials->SetFogZ( waterHeight );
		materials->SetHeightClipZ( waterHeight );
		materials->SetHeightClipMode( clipMode );

		// Have to re-set up the view since we reset the size
 		render->Push3DView( view, nClearFlags, true, pTexture, m_Frustum );
		return;
	}
	
	if( flags & DF_RENDER_REFLECTION )
	{
		ITexture *pTexture = GetWaterReflectionTexture();

		materials->SetFogZ( waterHeight );

		// Use the aspect ratio of the main view! So, don't recompute it here
		view.x = view.y = 0;
		view.width = pTexture->GetActualWidth();
		view.height = pTexture->GetActualHeight();
		view.angles[0] = -view.angles[0];
		view.angles[2] = -view.angles[2];
		view.origin[2] -= 2.0f * ( view.origin[2] - (origWaterHeight));
		bool bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();
		if( bSoftwareUserClipPlane && ( view.origin[2] > waterHeight - r_eyewaterepsilon.GetFloat() ) )
		{
			waterHeight = view.origin[2] + r_eyewaterepsilon.GetFloat();
		}

 		materials->SetHeightClipZ( waterHeight );
		materials->SetHeightClipMode( clipMode );

		render->Push3DView( view, nClearFlags, true, pTexture, m_Frustum );
 		SetLightmapScaleForWater();
		return;
	}

	if ( nClearFlags & (VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR) )
	{
		materials->ClearBuffers( nClearFlags & VIEW_CLEAR_COLOR, nClearFlags & VIEW_CLEAR_DEPTH );
	}

	materials->SetHeightClipMode( clipMode );
	if ( clipMode != MATERIAL_HEIGHTCLIPMODE_DISABLE )
	{   
		materials->SetHeightClipZ( waterHeight );
	}
}


//-----------------------------------------------------------------------------
// Pops a water render target
//-----------------------------------------------------------------------------
void CViewRender::PopWaterRenderTarget( int nFlags )
{
	materials->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );
	if( nFlags & (DF_RENDER_REFRACTION | DF_RENDER_REFLECTION) )
	{
		render->PopView( m_Frustum );
		if (SavedLinearLightMapScale.x>=0)
		{
			materials->SetToneMappingScaleLinear(SavedLinearLightMapScale);
			SavedLinearLightMapScale.x=-1;
		}
	}
}


//-----------------------------------------------------------------------------
// Draws the world + entities
//-----------------------------------------------------------------------------
void CViewRender::WaterDrawHelper( const CViewSetup &view, ClientWorldListInfo_t &info, CRenderList &renderList, 
								   float waterHeight, int flags, view_id_t viewID, float waterZAdjust, int iForceViewLeaf )
{
	int savedViewID = g_CurrentViewID;
	g_CurrentViewID = viewID;

	// Need a copy of the view since pushing a water render target
	// make change the aspect ratio of the projection space transform
	CViewSetup actualView = view; 

	int nClearFlags = 0;
	if ( flags & DF_CLEARDEPTH )
	{
		nClearFlags |= VIEW_CLEAR_DEPTH;
	}
	if ( flags & DF_CLEARCOLOR )
 	{
		nClearFlags |= VIEW_CLEAR_COLOR;
	}
		 
	PushWaterRenderTarget( actualView, nClearFlags, waterHeight, flags );

	if( flags & DF_BUILDWORLDLISTS )
	{
		bool bDrawEntities = ( flags & DF_DRAW_ENTITITES ) != 0;
		bool bUpdateLightmaps = ( flags & DF_UPDATELIGHTMAPS ) != 0;
		BuildWorldRenderLists( &actualView, info, bUpdateLightmaps, bDrawEntities, iForceViewLeaf );
	}

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();
    
	// Update our render view flags.
	m_DrawFlags = flags | m_BaseDrawFlags;

	ITexture *pSaveFrameBufferCopyTexture = materials->GetFrameBufferCopyTexture( 0 );
	if ( engine->GetDXSupportLevel() >= 80 )
	{
		materials->SetFrameBufferCopyTexture( GetPowerOfTwoFrameBufferTexture() );
	}

	DrawWorld( info, renderList, m_DrawFlags, waterZAdjust );

	// FIXME: This may have to be static, not sure if we'll get a stack overflow
	ClientWorldListInfo_t tmpInfo;
	tmpInfo.m_pLeafList = (LeafIndex_t*)stackalloc( info.m_LeafCount * sizeof(LeafIndex_t) );
	tmpInfo.m_pLeafFogVolume = (LeafFogVolume_t*)stackalloc( info.m_LeafCount * sizeof(LeafFogVolume_t) );
	tmpInfo.m_pActualLeafIndex = (LeafIndex_t*)stackalloc( info.m_LeafCount * sizeof(LeafIndex_t) );
	ClientWorldListInfo_t *pInfo = ComputeActualWorldListInfo( info, m_DrawFlags, tmpInfo );


	if ( flags & DF_DRAW_ENTITITES )
	{
		BuildRenderableRenderLists( &actualView, *pInfo, renderList );
		DrawOpaqueRenderables( *pInfo, renderList );
		DrawTranslucentRenderables( *pInfo, renderList, m_DrawFlags, false );
	}
	else
	{
		// Draw translucent world brushes only, no entities
		DrawTranslucentWorldInLeaves( pInfo->m_LeafCount - 1, 0, *pInfo, m_DrawFlags );
	}

	// issue the pixel visibility tests
	if ( CurrentViewID() != VIEW_MAIN )
	{
		PixelVisibility_EndCurrentView();
	}

	materials->SetFrameBufferCopyTexture( pSaveFrameBufferCopyTexture );
	PopWaterRenderTarget( m_DrawFlags );

	m_DrawFlags = m_BaseDrawFlags;
	g_CurrentViewID = savedViewID;
}


//-----------------------------------------------------------------------------
// Returns true if the view plane intersects the water
//-----------------------------------------------------------------------------
bool DoesViewPlaneIntersectWater( float waterZ, int leafWaterDataID )
{
	if ( leafWaterDataID == -1 )
		return false;

	VMatrix viewMatrix, projectionMatrix, viewProjectionMatrix, inverseViewProjectionMatrix;
	materials->GetMatrix( MATERIAL_VIEW, &viewMatrix );
	materials->GetMatrix( MATERIAL_PROJECTION, &projectionMatrix );
	MatrixMultiply( projectionMatrix, viewMatrix, viewProjectionMatrix );
	MatrixInverseGeneral( viewProjectionMatrix, inverseViewProjectionMatrix );

	Vector mins, maxs;
	ClearBounds( mins, maxs );
	Vector testPoint[4];
	testPoint[0].Init( -1.0f, -1.0f, 0.0f );
	testPoint[1].Init( -1.0f, 1.0f, 0.0f );
	testPoint[2].Init( 1.0f, -1.0f, 0.0f );
	testPoint[3].Init( 1.0f, 1.0f, 0.0f );
	int i;
	bool bAbove = false;
	bool bBelow = false;
	float fudge = 7.0f;
	for( i = 0; i < 4; i++ )
	{
		Vector worldPos;
		Vector3DMultiplyPositionProjective( inverseViewProjectionMatrix, testPoint[i], worldPos );
		AddPointToBounds( worldPos, mins, maxs );
//		Warning( "viewplanez: %f waterZ: %f\n", worldPos.z, waterZ );
		if( worldPos.z + fudge > waterZ )
		{
			bAbove = true;
		}
		if( worldPos.z - fudge < waterZ )
		{
			bBelow = true;
		}
	}

	// early out if the near plane doesn't cross the z plane of the water.
	if( !( bAbove && bBelow ) )
		return false;

	Vector vecFudge( fudge, fudge, fudge );
	mins -= vecFudge;
	maxs += vecFudge;
	
	// the near plane does cross the z value for the visible water volume.  Call into
	// the engine to find out if the near plane intersects the water volume.
	return render->DoesBoxIntersectWaterVolume( mins, maxs, leafWaterDataID );
} 

static void CalcWaterEyeAdjustments( const CViewSetup &view, const VisibleFogVolumeInfo_t &fogInfo,
										  float &newWaterHeight, float &waterZAdjust, bool bSoftwareUserClipPlane )
{
	if( !bSoftwareUserClipPlane )
	{
		newWaterHeight = fogInfo.m_flWaterHeight;
		waterZAdjust = 0.0f;
		return;
	}
	
	newWaterHeight = fogInfo.m_flWaterHeight;
	float eyeToWaterZDelta = view.origin[2] - fogInfo.m_flWaterHeight;
	float epsilon = r_eyewaterepsilon.GetFloat();
	waterZAdjust = 0.0f;
	if( fabs( eyeToWaterZDelta ) < epsilon )
	{
		if( eyeToWaterZDelta > 0 )
		{
			newWaterHeight = view.origin[2] - epsilon;
		}
		else
		{
			newWaterHeight = view.origin[2] + epsilon;
		}
		waterZAdjust = newWaterHeight - fogInfo.m_flWaterHeight;
	}

//	Warning( "view.origin[2]: %f newWaterHeight: %f fogInfo.m_flWaterHeight: %f waterZAdjust: %f\n", 
//		( float )view.origin[2], newWaterHeight, fogInfo.m_flWaterHeight, waterZAdjust );
}


#ifdef _XBOX
//-----------------------------------------------------------------------------
// Draws a perspective-correct dudv map into the reflection texture
//-----------------------------------------------------------------------------
void CViewRender::DrawScreenSpaceWaterDuDv( const CViewSetup &view, float waterZAdjust )
{
	ITexture *pTexture = GetWaterReflectionTexture();

	// Use the aspect ratio of the main view! So, don't recompute it here
	CViewSetup actualView = view;
	actualView.width = pTexture->GetActualWidth();
	actualView.height = pTexture->GetActualHeight();

	// these DU/DV constants need to be 64, not 128 on xbox because the texture is signed and we only use the positive half of that space
	materials->ClearColor4ub( 255, 64, 64, 0 );
	render->Push3DView( actualView, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, true, pTexture, m_Frustum );
	materials->SetHeightClipMode( MATERIAL_HEIGHTCLIPMODE_DISABLE );

 	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	render->DrawWaterDuDv( waterZAdjust );

	render->PopView( m_Frustum );
}
#endif // _XBOX

//-----------------------------------------------------------------------------
// Draws the scene when the view point is above the level of the water
//-----------------------------------------------------------------------------
#include "tier0/memdbgoff.h"

void CViewRender::ViewDrawScene_EyeAboveWater( bool bDrawSkybox, const CViewSetup &view,
											   int nClearFlags, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	VPROF( "CViewRender::ViewDrawScene_EyeAboveWater" );

	bool bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();
	
	ClientWorldListInfo_t info;	
#ifndef _XBOX
	CRenderList renderList;
	CRenderList *pRenderList = &renderList;
#else
	void *ptr = MemAllocScratch(sizeof(CRenderList));
	CRenderList *pRenderList = new (ptr) CRenderList;
#endif

	float newWaterHeight, waterZAdjust;
	CalcWaterEyeAdjustments( view, fogInfo, newWaterHeight, waterZAdjust, bSoftwareUserClipPlane );

	// eye is outside of water
	
	// render the reflection
	if( waterInfo.m_bReflect )
	{
		// NOTE: Clearing the color is unnecessary since we're drawing the skybox
		// and dest-alpha is never used in the reflection
		int nFlags = DF_RENDER_REFLECTION | DF_CLIP_Z | DF_CLIP_BELOW | 
			DF_RENDER_ABOVEWATER | DF_CLEARDEPTH | DF_UPDATELIGHTMAPS | DF_BUILDWORLDLISTS;

		// NOTE: This will cause us to draw the 2d skybox in the reflection 
		// (which we want to do instead of drawing the 3d skybox)
		nFlags |= DF_DRAWSKYBOX;

		if( waterInfo.m_bReflectEntities )
		{
			nFlags |= DF_DRAW_ENTITITES;
		}

		// Disable occlusion visualization in reflection
		bool bVisOcclusion = r_visocclusion.GetInt();
		r_visocclusion.SetValue( 0 );

		EnableWorldFog();
		WaterDrawHelper( view, info, *pRenderList, fogInfo.m_flWaterHeight, nFlags, VIEW_REFLECTION, 
			0.0f, fogInfo.m_nVisibleFogVolumeLeaf );

		r_visocclusion.SetValue( bVisOcclusion );

		materials->Flush();
	}
	
	unsigned char ucFogColor[3];

	int nFlags;
	bool bViewIntersectsWater = false;
	// render refraction
	int nFirstPassFlags = DF_BUILDWORLDLISTS | DF_UPDATELIGHTMAPS | DF_CLEARCOLOR;
	if ( waterInfo.m_bRefract )
	{
		nFlags = nFirstPassFlags | DF_RENDER_REFRACTION | DF_CLIP_Z | 
			DF_RENDER_UNDERWATER | DF_FUDGE_UP | 
			DF_DRAW_ENTITITES | DF_MAINTAINWORLDLISTS | DF_CLEARDEPTH;
		nFirstPassFlags = 0;
		
		render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, true );
		
		SetClearColorToFogColor( ucFogColor );
		WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, VIEW_REFRACTION, 
			waterZAdjust, -1 );
		if( !bSoftwareUserClipPlane )
		{
			bViewIntersectsWater = DoesViewPlaneIntersectWater( fogInfo.m_flWaterHeight, fogInfo.m_nVisibleFogVolume );
		}

#ifdef _XBOX
		// This is a workaround for the lack of perspective correction on the XBox for dudv maps
		EnableWorldFog();
		DrawScreenSpaceWaterDuDv( view, waterZAdjust );
#endif

		materials->ClearColor4ub( 0, 0, 0, 255 );
	}

	materials->Flush();
	
	EnableWorldFog();
	
	// render the world
	nFlags = nFirstPassFlags | DF_RENDER_ABOVEWATER | DF_CLEARDEPTH | DF_DRAW_ENTITITES | DF_MAINTAINWORLDLISTS;
	if( bViewIntersectsWater && !bSoftwareUserClipPlane )
	{
		// This is necessary to keep the non-water fogged world from drawing underwater in 
		// the case where we want to partially see into the water.
		nFlags |= DF_CLIP_Z | DF_CLIP_BELOW;
	}

	if ( bDrawSkybox )
	{
		nFlags |= DF_DRAWSKYBOX;
		nFlags &= ~DF_CLEARCOLOR;
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		nFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		nFlags |= DF_RENDER_UNDERWATER;
	}
	
	WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, CurrentViewID(), waterZAdjust, -1 );

	if( waterZAdjust != 0.0f && bSoftwareUserClipPlane && waterInfo.m_bRefract )
	{
		nFlags = DF_RENDER_UNDERWATER;
		WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, CurrentViewID(), waterZAdjust,-1 );
	}
	else if( bViewIntersectsWater )
	{
		materials->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
		render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, true );
		nFlags = DF_RENDER_UNDERWATER | DF_CLIP_Z | DF_DRAW_ENTITITES;
		WaterDrawHelper( view, info, *pRenderList, fogInfo.m_flWaterHeight, nFlags, VIEW_NONE, 0.0f, -1 );
	}
	materials->ClearColor4ub( 0, 0, 0, 255 );

#ifdef _XBOX
	MemFreeScratch();
#endif
}

#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Draws the scene when the view point is under the level of the water
//-----------------------------------------------------------------------------
#include "tier0/memdbgoff.h"

void CViewRender::ViewDrawScene_EyeUnderWater( bool bDrawSkybox, const CViewSetup &view, 
											   int nClearFlags, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	// FIXME: The 3d skybox shouldn't be drawn when the eye is under water

	VPROF( "CViewRender::ViewDrawScene_EyeUnderWater" );

	bool bSoftwareUserClipPlane = g_pMaterialSystemHardwareConfig->UseFastClipping();

	float newWaterHeight, waterZAdjust;
	CalcWaterEyeAdjustments( view, fogInfo, newWaterHeight, waterZAdjust, bSoftwareUserClipPlane );

	ClientWorldListInfo_t info;	

#ifndef _XBOX
	CRenderList renderList;
	CRenderList *pRenderList = &renderList;
#else
	void *ptr = MemAllocScratch(sizeof(CRenderList));
	CRenderList *pRenderList = new (ptr) CRenderList;
#endif

	int nFirstPassFlags = DF_BUILDWORLDLISTS | DF_UPDATELIGHTMAPS;

	render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, true );
	unsigned char ucFogColor[3];
	materials->GetFogColor( ucFogColor );
	materials->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );

	// render refraction (out of water)
	if ( waterInfo.m_bRefract )
	{
		// NOTE: Refraction renders into the back buffer, over the top of the 3D skybox
		// It is then blitted out into the refraction target. This is so that
		// we only have to set up 3d sky vis once, and only render it once also!
		int nFlags = nFirstPassFlags | DF_CLIP_Z | 
			DF_CLIP_BELOW | DF_RENDER_ABOVEWATER | 
			DF_DRAW_ENTITITES | DF_MAINTAINWORLDLISTS | DF_CLEARDEPTH;
			   
		if ( bDrawSkybox )
		{
			nFlags |= DF_DRAWSKYBOX | DF_CLIP_SKYBOX | DF_CLEARCOLOR;
		}
		nFirstPassFlags = 0;

		EnableWorldFog();
		WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, VIEW_REFRACTION, waterZAdjust, -1 );

		Rect_t srcRect;
		srcRect.x = view.x;
		srcRect.y = view.y;
		srcRect.width = view.width;
		srcRect.height = view.height;

		ITexture *pTexture = GetWaterRefractionTexture();
		materials->CopyRenderTargetToTextureEx( pTexture, 0, &srcRect, NULL );

#ifdef _XBOX
		// This is a workaround for the lack of perspective correction on the XBox for dudv maps
		render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, false );
		DrawScreenSpaceWaterDuDv( view, waterZAdjust );
		materials->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
#endif
	}
	
	// NOTE: We're not drawing the 2d skybox under water since it's assumed to not be visible.

	// render the world underwater
	// Clear the color to get the appropriate underwater fog color
	int nFlags = nFirstPassFlags | DF_MAINTAINWORLDLISTS | DF_FUDGE_UP |
		DF_RENDER_UNDERWATER | DF_CLEARDEPTH | DF_DRAW_ENTITITES;
					  
	if( !bSoftwareUserClipPlane )
	{
		nFlags |= DF_CLIP_Z;
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		nFlags |= DF_RENDER_WATER;
	}
	if ( !waterInfo.m_bRefract && !waterInfo.m_bOpaqueWater )
	{
		nFlags |= DF_RENDER_ABOVEWATER;
	}

	render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, false );
	WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, CurrentViewID(), waterZAdjust, -1 );

	if( waterZAdjust != 0.0f && bSoftwareUserClipPlane && waterInfo.m_bRefract )
	{
		nFlags = DF_RENDER_ABOVEWATER;
		WaterDrawHelper( view, info, *pRenderList, newWaterHeight, nFlags, CurrentViewID(), waterZAdjust, -1 );
	}
	materials->ClearColor4ub( 0, 0, 0, 255 );

#ifdef _XBOX
	MemFreeScratch();
#endif
}

//-----------------------------------------------------------------------------
// Draws the scene when there's no water or cheap water
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_NoWater( bool bDrawSkybox, const CViewSetup &view, int nClearFlags,
	const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& waterInfo )
{
	VPROF( "CViewRender::ViewDrawScene_NoWater" );

	ClientWorldListInfo_t info;	
#ifndef _XBOX
	CRenderList renderList;
	CRenderList *pRenderList = &renderList;
#else
	void *ptr = MemAllocScratch(sizeof(CRenderList));
	CRenderList *pRenderList = new (ptr) CRenderList;
#endif

	int nFlags = DF_BUILDWORLDLISTS | DF_DRAW_ENTITITES | DF_UPDATELIGHTMAPS;
	if ( nClearFlags & VIEW_CLEAR_COLOR )
	{
		nFlags |= DF_CLEARCOLOR;
	}
	if ( nClearFlags & VIEW_CLEAR_DEPTH )
	{
		nFlags |= DF_CLEARDEPTH;
	}

	if ( !waterInfo.m_bOpaqueWater )
	{
		nFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
	}
	else
	{
		bool bViewIntersectsWater = DoesViewPlaneIntersectWater( fogInfo.m_flWaterHeight, fogInfo.m_nVisibleFogVolume );
		if( bViewIntersectsWater )
		{
			// have to draw both sides if we can see both.
			nFlags |= DF_RENDER_UNDERWATER | DF_RENDER_ABOVEWATER;
		}
		else if ( fogInfo.m_bEyeInFogVolume )
		{
			nFlags |= DF_RENDER_UNDERWATER;
		}
		else
		{
			nFlags |= DF_RENDER_ABOVEWATER;
		}
	}
	if ( waterInfo.m_bDrawWaterSurface )
	{
		nFlags |= DF_RENDER_WATER;
	}

	if ( !fogInfo.m_bEyeInFogVolume )
	{
		if ( bDrawSkybox )
		{
			nFlags |= DF_DRAWSKYBOX;
		}
		EnableWorldFog();
	}
	else
	{
		nFlags |= DF_CLEARCOLOR;

		render->SetFogVolumeState( fogInfo.m_nVisibleFogVolume, false );

		unsigned char ucFogColor[3];
		materials->GetFogColor( ucFogColor );
		materials->ClearColor4ub( ucFogColor[0], ucFogColor[1], ucFogColor[2], 255 );
	}

	WaterDrawHelper( view, info, *pRenderList, 0.0f, nFlags, CurrentViewID(), 0.0f, m_iForceViewLeaf );
	materials->ClearColor4ub( 0, 0, 0, 255 );

#ifdef _XBOX
	MemFreeScratch();
#endif
}

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Methods related to controlling the cheap water distance
//-----------------------------------------------------------------------------
void CViewRender::SetCheapWaterStartDistance( float flCheapWaterStartDistance )
{
	m_flCheapWaterStartDistance = flCheapWaterStartDistance;
}

void CViewRender::SetCheapWaterEndDistance( float flCheapWaterEndDistance )
{
	m_flCheapWaterEndDistance = flCheapWaterEndDistance;
}

void CViewRender::GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance )
{
	flCheapWaterStartDistance = m_flCheapWaterStartDistance;
	flCheapWaterEndDistance = m_flCheapWaterEndDistance;
}

static void CheapWaterStart_f( void )
{
	if( engine->Cmd_Argc() == 2 )
	{
		float dist = atof( engine->Cmd_Argv( 1 ) );
		view->SetCheapWaterStartDistance( dist );
	}
	else
	{
		float start, end;
		view->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterstart: %f\n", start );
	}
}

static void CheapWaterEnd_f( void )
{
	if( engine->Cmd_Argc() == 2 )
	{
		float dist = atof( engine->Cmd_Argv( 1 ) );
		view->SetCheapWaterEndDistance( dist );
	}
	else
	{
		float start, end;
		view->GetWaterLODParams( start, end );
		Warning( "r_cheapwaterend: %f\n", end );
	}
}


//-----------------------------------------------------------------------------
// A console command allowing you to draw a material as an overlay
//-----------------------------------------------------------------------------
static void ScreenOverlay_f( void )
{
	if( engine->Cmd_Argc() == 2 )
	{
		if ( !Q_stricmp( "off", engine->Cmd_Argv(1) ) )
		{
			view->SetScreenOverlayMaterial( NULL );
		}
		else
		{
			IMaterial *pMaterial = materials->FindMaterial( engine->Cmd_Argv(1), TEXTURE_GROUP_OTHER, false );
			if ( !IsErrorMaterial( pMaterial ) )
			{
				view->SetScreenOverlayMaterial( pMaterial );
			}
			else
			{
				view->SetScreenOverlayMaterial( NULL );
			}
		}
	}
	else
	{
		IMaterial *pMaterial = view->GetScreenOverlayMaterial();
		Warning( "r_screenoverlay: %s\n", pMaterial ? pMaterial->GetName() : "off" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &view - 
//			&introData - 
//-----------------------------------------------------------------------------
void CViewRender::ViewDrawScene_Intro( const CViewSetup &view, int nClearFlags, const IntroData_t &introData )
{
	VPROF( "CViewRender::ViewDrawScene" );

	// -----------------------------------------------------------------------
	// Set the clear color to black since we are going to be adding up things
	// in the frame buffer.
	// -----------------------------------------------------------------------
	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	materials->ClearColor4ub( 0, 0, 0, 255 );
	
	// -----------------------------------------------------------------------
	// Draw the primary scene and copy it to the first framebuffer texture
	// -----------------------------------------------------------------------	
	CViewSetup playerView = view;
	playerView.origin = introData.m_vecCameraView;
	playerView.m_vUnreflectedOrigin = introData.m_vecCameraView;
	playerView.angles = introData.m_vecCameraViewAngles;
	if ( introData.m_playerViewFOV )
	{
		playerView.fov = ScaleFOVByWidthRatio( introData.m_playerViewFOV, engine->GetScreenAspectRatio() / ( 4.0f / 3.0f ) );
	}
	SetupCurrentView( playerView.origin, playerView.angles, VIEW_INTRO_PLAYER );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view, clear frame/z buffer if necessary
	unsigned int visFlags;
	SetupVis( playerView, visFlags );

	if( introData.m_bDrawSecondary )
	{
		// NOTE: We only increment this once since time doesn't move forward.
		ParticleMgr()->IncrementFrameCode();
	}

	if( introData.m_bDrawPrimary )
	{
		DrawWorldAndEntities( true /* drawSkybox */, playerView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH );
	}
	else
	{
		materials->ClearBuffers( true, true );
	}

	Rect_t actualRect;
	UpdateScreenEffectTexture( 0, view.x, view.y, view.width, view.height, false, &actualRect );
	
	// -----------------------------------------------------------------------
	// Draw the secondary scene and copy it to the second framebuffer texture
	// -----------------------------------------------------------------------
	CViewSetup cameraView = view;
	SetupCurrentView( cameraView.origin, cameraView.angles, VIEW_INTRO_CAMERA );

	// Invoke pre-render methods
	IGameSystem::PreRenderAllSystems();

	// Start view, clear frame/z buffer if necessary
	SetupVis( cameraView, visFlags );

	// Clear alpha to 255 so that masking with the vortigaunts (0) works properly.
	materials->ClearColor4ub( 0, 0, 0, 255 );

	if( introData.m_bDrawSecondary )
	{
		DrawWorldAndEntities( true /* drawSkybox */, cameraView, VIEW_CLEAR_COLOR | VIEW_CLEAR_DEPTH );
	}
	else
	{
		materials->ClearBuffers( true, true );
	}
	UpdateScreenEffectTexture( 1, view.x, view.y, view.width, view.height );

	// -----------------------------------------------------------------------
	// Draw quads on the screen for each screenspace pass.
	// -----------------------------------------------------------------------
	// Find the material that we use to render the overlays
	IMaterial *pOverlayMaterial = materials->FindMaterial( "scripted/intro_screenspaceeffect", TEXTURE_GROUP_OTHER );
	IMaterialVar *pModeVar = pOverlayMaterial->FindVar( "$mode", NULL );
	IMaterialVar *pAlphaVar = pOverlayMaterial->FindVar( "$alpha", NULL );

	materials->ClearBuffers( true, true );
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();
	materials->LoadIdentity();

	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	materials->LoadIdentity();
	
	int passID;
	for( passID = 0; passID < introData.m_Passes.Count(); passID++ )
	{
		const IntroDataBlendPass_t& pass = introData.m_Passes[passID];
		if ( pass.m_Alpha == 0 )
			continue;

		// Pick one of the blend modes for the material.
		if( pass.m_BlendMode >= 0 && pass.m_BlendMode < 9  )
		{
			pModeVar->SetIntValue( pass.m_BlendMode );
		}
		else
		{
			Assert(0);
		}
		// Set the alpha value for the material.
		pAlphaVar->SetFloatValue( pass.m_Alpha );
		
		// Draw a quad for this pass.
		ITexture *pTexture = GetFullFrameFrameBufferTexture( 0 );
		materials->DrawScreenSpaceRectangle( pOverlayMaterial, view.x, view.y, view.width, view.height,
											actualRect.x, actualRect.y, actualRect.x+actualRect.width-1, actualRect.y+actualRect.height-1, 
											pTexture->GetActualWidth(), pTexture->GetActualHeight() );
	}
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();
	
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
	
	// Draw the starfield
	// FIXME
	// blur?
	
	// Disable fog for the rest of the stuff
	DisableFog();
	
	// Here are the overlays...
	CGlowOverlay::DrawOverlays( view.m_bCacheFullSceneState );

	// issue the pixel visibility tests
	PixelVisibility_EndCurrentView();

	// And here are the screen-space effects
	PerformScreenSpaceEffects( view.x, view.y, view.width, view.height );

	// Make sure sound doesn't stutter
	engine->Sound_ExtraUpdate();

	// Debugging info goes over the top
	Draw3DDebuggingInfo( view );

	// Let the particle manager simulate things that haven't been simulated.
	ParticleMgr()->PostRender();

	FinishCurrentView();
}

#ifdef _XBOX
void ViewTexture_f()
{
	int		argc;
	char*	enable;

	argc = engine->Cmd_Argc();
	if (argc == 1)
	{
		// toggle
		enable = mat_viewTextureEnable.GetInt() ? "0" : "1";
	}
	else
	{
		// force on
		enable = "1";
	}

	mat_viewTextureEnable.SetValue(enable);
	if (argc >= 2)
		mat_viewTextureName.SetValue(engine->Cmd_Argv(1));
	if (argc >= 3)
		mat_viewTextureScale.SetValue(engine->Cmd_Argv(2));
}
static ConCommand mat_viewTexture("mat_viewTexture", ViewTexture_f, "Show/Hide texture [name] [scale]");
#endif


static ConCommand r_cheapwaterstart( "r_cheapwaterstart", CheapWaterStart_f );
static ConCommand r_cheapwaterend( "r_cheapwaterend", CheapWaterEnd_f );
static ConCommand r_screenspacematerial( "r_screenoverlay", ScreenOverlay_f );


