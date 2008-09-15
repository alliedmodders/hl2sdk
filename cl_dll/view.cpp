//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//


#include "cbase.h"
#include "view.h"
#include "iviewrender.h"
#include "iviewrender_beams.h"
#include "view_shared.h"
#include "ivieweffects.h"
#include "iinput.h"
#include "iclientmode.h"
#include "prediction.h"
#include "viewrender.h"
#include "c_te_legacytempents.h"
#include "cl_mat_stub.h"
#include "tier0/vprof.h"
#include "IClientVehicle.h"
#include "engine/IEngineTrace.h"
#include "vmatrix.h"
#include "rendertexture.h"
#include "c_world.h"
#include <KeyValues.h>
#include "igameevents.h"
#include "smoke_fog_overlay.h"
#include "bitmap/tgawriter.h"
#include "hltvcamera.h"
#include "input.h"
#include "filesystem.h"
#include "materialsystem/itexture.h"
#include "vstdlib/icommandline.h"
#include "IEngineVGui.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "ScreenSpaceEffects.h"

#if defined( HL2_CLIENT_DLL ) || defined( CSTRIKE_DLL )
#define USE_MONITORS
#endif

#ifdef USE_MONITORS
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMaterialSystemHardwareConfig.h"
#include "c_point_camera.h"

#ifdef _XBOX
#include "xbox/xbox_platform.h"
#include "xbox/xbox_win32stubs.h"
#include "xbox/xbox_core.h"
#endif

#endif // USE_MONITORS
	
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
		  
void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height );
bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );
bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles );


extern ConVar default_fov;
extern bool g_bRenderingScreenshot;

#if !defined( _XBOX )
#define SAVEGAME_SCREENSHOT_WIDTH	180
#define SAVEGAME_SCREENSHOT_HEIGHT	100
#else
#define SAVEGAME_SCREENSHOT_WIDTH	128
#define SAVEGAME_SCREENSHOT_HEIGHT	128
#endif

CViewRender g_DefaultViewRender;
IViewRender *view = NULL;	// set in cldll_client_init.cpp if no mod creates their own

#if _DEBUG
bool g_bRenderingCameraView = false;
#endif

static Vector g_vecRenderOrigin(0,0,0);
static QAngle g_vecRenderAngles(0,0,0);
static Vector g_vecPrevRenderOrigin(0,0,0);	// Last frame's render origin
static QAngle g_vecPrevRenderAngles(0,0,0); // Last frame's render angles
static Vector g_vecVForward(0,0,0), g_vecVRight(0,0,0), g_vecVUp(0,0,0);
static VMatrix g_matCamInverse;

extern ConVar cl_forwardspeed;

static ConVar v_centermove( "v_centermove", "0.15");
static ConVar v_centerspeed( "v_centerspeed","500" );

// 54 degrees approximates a 35mm camera - we determined that this makes the viewmodels
// and motions look the most natural.
ConVar v_viewmodel_fov( "viewmodel_fov", "54", FCVAR_CHEAT );
ConVar mat_viewportscale( "mat_viewportscale", "1.0", FCVAR_CHEAT );
ConVar cl_leveloverview( "cl_leveloverview", "0", FCVAR_CHEAT );

static ConVar r_mapextents( "r_mapextents", "16384", FCVAR_CHEAT, 
						   "Set the max dimension for the map.  This determines the far clipping plane" );

// UNDONE: Delete this or move to the material system?
ConVar	gl_clear( "gl_clear","0");

static ConVar r_farz( "r_farz", "-1", FCVAR_CHEAT, "Override the far clipping plane. -1 means to use the value in env_fog_controller." );
static ConVar cl_demoviewoverride( "cl_demoviewoverride", "0", 0, "Override view during demo playback" );

static Vector s_DemoView;
static QAngle s_DemoAngle;

static void CalcDemoViewOverride( Vector &origin, QAngle &angles )
{
	engine->SetViewAngles( s_DemoAngle );

	input->ExtraMouseSample( gpGlobals->absoluteframetime, true );

	engine->GetViewAngles( s_DemoAngle );

	Vector forward, right, up;

	AngleVectors( s_DemoAngle, &forward, &right, &up );

	float speed = gpGlobals->absoluteframetime * cl_demoviewoverride.GetFloat() * 320;
	
	s_DemoView += speed * input->KeyState (&in_forward) * forward  ;
	s_DemoView -= speed * input->KeyState (&in_back) * forward ;

	s_DemoView += speed * input->KeyState (&in_moveright) * right ;
	s_DemoView -= speed * input->KeyState (&in_moveleft) * right ;

	origin = s_DemoView;
	angles = s_DemoAngle;
}



//-----------------------------------------------------------------------------
// Accessors to return the main view (where the player's looking)
//-----------------------------------------------------------------------------
const Vector &MainViewOrigin()
{
	return g_vecRenderOrigin;
}

const QAngle &MainViewAngles()
{
	return g_vecRenderAngles;
}

const Vector &MainViewForward()
{
	return g_vecVForward;
}

const Vector &MainViewRight()
{
	return g_vecVRight;
}

const Vector &MainViewUp()
{
	return g_vecVUp;
}

const VMatrix &MainWorldToViewMatrix()
{
	return g_matCamInverse;
}

const Vector &PrevMainViewOrigin()
{
	return g_vecPrevRenderOrigin;
}

const QAngle &PrevMainViewAngles()
{
	return g_vecPrevRenderAngles;
}

//-----------------------------------------------------------------------------
// Compute the world->camera transform
//-----------------------------------------------------------------------------
void ComputeCameraVariables( const Vector &vecOrigin, const QAngle &vecAngles, 
	Vector *pVecForward, Vector *pVecRight, Vector *pVecUp, VMatrix *pMatCamInverse )
{
	// Compute view bases
	AngleVectors( vecAngles, pVecForward, pVecRight, pVecUp );

	for (int i = 0; i < 3; ++i)
	{
		(*pMatCamInverse)[0][i] = (*pVecRight)[i];	
		(*pMatCamInverse)[1][i] = (*pVecUp)[i];	
		(*pMatCamInverse)[2][i] = -(*pVecForward)[i];	
		(*pMatCamInverse)[3][i] = 0.0F;
	}
	(*pMatCamInverse)[0][3] = -DotProduct( *pVecRight, vecOrigin );
	(*pMatCamInverse)[1][3] = -DotProduct( *pVecUp, vecOrigin );
	(*pMatCamInverse)[2][3] =  DotProduct( *pVecForward, vecOrigin );
	(*pMatCamInverse)[3][3] = 1.0F;
}


bool R_CullSphere(
	VPlane const *pPlanes,
	int nPlanes,
	Vector const *pCenter,
	float radius)
{
	for(int i=0; i < nPlanes; i++)
		if(pPlanes[i].DistTo(*pCenter) < -radius)
			return true;
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void StartPitchDrift( void )
{
	view->StartPitchDrift();
}

static ConCommand centerview( "centerview", StartPitchDrift );

//-----------------------------------------------------------------------------
// Purpose: Initializes all view systems
//-----------------------------------------------------------------------------
void CViewRender::Init( void )
{
#ifndef _XBOX
	m_bDrawOverlay = false;
#endif

	m_pDrawEntities		= cvar->FindVar( "r_drawentities" );
	m_pDrawBrushModels	= cvar->FindVar( "r_drawbrushmodels" );

	memset( &m_PitchDrift, 0, sizeof( m_PitchDrift ) );

	beams->InitBeams();
	tempents->Init();

	m_TranslucentSingleColor.Init( "debug/debugtranslucentsinglecolor", TEXTURE_GROUP_OTHER );
	m_ModulateSingleColor.Init( "engine/modulatesinglecolor", TEXTURE_GROUP_OTHER );

	// FIXME:  
	QAngle angles;
	engine->GetViewAngles( angles );
	AngleVectors( angles, &m_vecLastFacing );

}


//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelInit( void )
{
	beams->ClearBeams();
	tempents->Clear();

	m_FrameNumber = 0;
	m_BuildWorldListsNumber = 0;
	m_BuildRenderableListsNumber = 0;

	// Clear our overlay materials
	m_ScreenOverlayMaterial.Init( NULL );

	// Init all IScreenSpaceEffects
	g_pScreenSpaceEffects->InitScreenSpaceEffects( );

#ifdef _XBOX
	// set per map
	extern ConVar mat_bloomscale;
	if ( mat_bloomscale.GetFloat() )
	{
		// setup bloom materials
		m_BloomDownsample.Init( "dev/downsample_non_hdr", TEXTURE_GROUP_OTHER );
		m_BloomBlurX.Init( "dev/blurfilterx_nohdr", TEXTURE_GROUP_OTHER );
		m_BloomBlurY.Init( "dev/blurfiltery_nohdr", TEXTURE_GROUP_OTHER );
		m_BloomAdd.Init( "dev/bloomadd", TEXTURE_GROUP_OTHER );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called once per level change
//-----------------------------------------------------------------------------
void CViewRender::LevelShutdown( void )
{
#ifdef _XBOX
	// clear bloom materials
	m_BloomDownsample.Init( NULL );
	m_BloomBlurX.Init( NULL );
	m_BloomBlurY.Init( NULL );
	m_BloomAdd.Init( NULL );
#endif
	g_pScreenSpaceEffects->ShutdownScreenSpaceEffects( );
}

//-----------------------------------------------------------------------------
// Purpose: Called at shutdown
//-----------------------------------------------------------------------------
void CViewRender::Shutdown( void )
{
	m_TranslucentSingleColor.Shutdown( );
	m_ModulateSingleColor.Shutdown( );
	beams->ShutdownBeams();
	tempents->Shutdown();
}


//-----------------------------------------------------------------------------
// Returns the frame number
//-----------------------------------------------------------------------------

int CViewRender::FrameNumber( void ) const
{
	return m_FrameNumber;
}

//-----------------------------------------------------------------------------
// Returns the worldlists build number
//-----------------------------------------------------------------------------

int CViewRender::BuildWorldListsNumber( void ) const
{
	return m_BuildWorldListsNumber;
}

//-----------------------------------------------------------------------------
// Purpose: Make sure view origin is pretty close so we don't look from inside a wall
//-----------------------------------------------------------------------------
void CViewRender::BoundOffsets( void )
{
	int	horiz_gap		= 14;
	int	vert_gap_down	= 22;
	int vert_gap_up		= 30;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	Vector playerOrigin = player->GetAbsOrigin();

	// Absolutely bound refresh reletive to entity clipping hull
	//  so the view can never be inside a solid wall
	if ( m_View.origin[0] < ( playerOrigin[0] - horiz_gap ) )
	{
		m_View.origin[0] = playerOrigin[0] - horiz_gap;
	}
	else if ( m_View.origin[0] > ( playerOrigin[0] + horiz_gap ))
	{
		m_View.origin[0] = playerOrigin[0] + horiz_gap;
	}

	if ( m_View.origin[1] < ( playerOrigin[1] - horiz_gap ) )
	{
		m_View.origin[1] = playerOrigin[1] - horiz_gap;
	}
	else if ( m_View.origin[1] > ( playerOrigin[1] + horiz_gap ) )
	{
		m_View.origin[1] = playerOrigin[1] + horiz_gap;
	}

	if ( m_View.origin[2] < ( playerOrigin[2] - vert_gap_down ) )
	{
		m_View.origin[2] = playerOrigin[2] - vert_gap_down;
	}
	else if ( m_View.origin[2] > ( playerOrigin[2] + vert_gap_up ) )
	{
		m_View.origin[2] = playerOrigin[2] + vert_gap_up;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Start moving pitch toward ideal
//-----------------------------------------------------------------------------
void CViewRender::StartPitchDrift (void)
{
	if ( m_PitchDrift.laststop == gpGlobals->curtime )
	{
		// Something else is blocking the drift.
		return;		
	}

	if ( m_PitchDrift.nodrift || !m_PitchDrift.pitchvel )
	{
		m_PitchDrift.pitchvel	= v_centerspeed.GetFloat();
		m_PitchDrift.nodrift	= false;
		m_PitchDrift.driftmove	= 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::StopPitchDrift (void)
{
	m_PitchDrift.laststop	= gpGlobals->curtime;
	m_PitchDrift.nodrift	= true;
	m_PitchDrift.pitchvel	= 0;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the client pitch angle towards cl.idealpitch sent by the server.
// If the user is adjusting pitch manually, either with lookup/lookdown,
//   mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
//-----------------------------------------------------------------------------
void CViewRender::DriftPitch (void)
{
	float		delta, move;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	if ( engine->IsHLTV() || ( player->GetGroundEntity() == NULL ) || engine->IsPlayingDemo() )
	{
		m_PitchDrift.driftmove = 0;
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Don't count small mouse motion
	if ( m_PitchDrift.nodrift )
	{
		if ( fabs( input->GetLastForwardMove() ) < cl_forwardspeed.GetFloat() )
		{
			m_PitchDrift.driftmove = 0;
		}
		else
		{
			m_PitchDrift.driftmove += gpGlobals->frametime;
		}
	
		if ( m_PitchDrift.driftmove > v_centermove.GetFloat() )
		{
			StartPitchDrift ();
		}
		return;
	}
	
	// How far off are we
	delta = prediction->GetIdealPitch() - player->GetAbsAngles()[ PITCH ];
	if ( !delta )
	{
		m_PitchDrift.pitchvel = 0;
		return;
	}

	// Determine movement amount
	move = gpGlobals->frametime * m_PitchDrift.pitchvel;
	// Accelerate
	m_PitchDrift.pitchvel += gpGlobals->frametime * v_centerspeed.GetFloat();
	
	// Move predicted pitch appropriately
	if (delta > 0)
	{
		if ( move > delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() + QAngle( move, 0, 0 ) );
	}
	else if ( delta < 0 )
	{
		if ( move > -delta )
		{
			m_PitchDrift.pitchvel = 0;
			move = -delta;
		}
		player->SetLocalAngles( player->GetLocalAngles() - QAngle( move, 0, 0 ) );
	}
}

// This is called by cdll_client_int to setup view model origins. This has to be done before
// simulation so entities can access attachment points on view models during simulation.
void CViewRender::OnRenderStart()
{
	SetUpView();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetViewSetup( void ) const
{
	return &m_CurrentView;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : const CViewSetup
//-----------------------------------------------------------------------------
const CViewSetup *CViewRender::GetPlayerViewSetup( void ) const
{   
	return &m_View;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//-----------------------------------------------------------------------------
void CViewRender::AddVisOrigin( const Vector& origin )
{
	m_bOverrideVisOrigin = true;

	// Don't allow them to write past array length
	AssertMsg( m_nNumVisOrigins < MAX_VIS_LEAVES, "Added more origins than will fit in the array!" );

	// If the vis origin count is greater than the size of our array, just fail to add this origin
	if ( m_nNumVisOrigins >= MAX_VIS_LEAVES )
		return;

	m_rgVisOrigins[ m_nNumVisOrigins++ ] = origin;
}

//-----------------------------------------------------------------------------
// Purpose: Return to main view with one vis origin, and stop using custom added vis origins
//			(added via AddVisOrigin).
// Input  : - 
//-----------------------------------------------------------------------------
void CViewRender::ClearAllCustomVisOrigins( void )
{
	m_bOverrideVisOrigin = false;
	m_nNumVisOrigins = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CViewRender::DisableVis( void )
{
	m_bForceNoVis = true;
}

static Vector s_TestOrigin;
static QAngle s_TestAngles;

//-----------------------------------------------------------------------------
// Purpose: Overrides the 'view origin' for the purposes of visibility calculation for area portals.
//			The default behavior is to use the camera's position. This call overrides that position with the parameter
//			vector for the next BuildWorldLists call (and is then reset to the default behavior)
// Input: VisOverrideData_t - visData struct which contains an override origin for area portal backface cull tests
//							  and a float tolerance which determines how close the specified origin must be to
//							  an area portal boundary before the area portal is considered to take up the whole screen.
// NOTE: This effects the next drawn scene, then the settings return to default. Call this before each ViewDrawScene
//-----------------------------------------------------------------------------
void CViewRender::ForceVisOverride( VisOverrideData_t& visData )
{
	m_VisData = visData;
	m_bOverrideVisData = true;
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the 'view leaf' for the purposes of determining the starting area for flowing through area portals
//			The default behavior is to use the camera's position. This call overrides that position with the parameter
//			vector for the next BuildWorldLists call (and is then reset to the default behavior)
// Input: pVisOrigin- The origin to use for visibility tests
// NOTE: This effects the next drawn scene, then the settings return to default. Call this before each ViewDrawScene
//-----------------------------------------------------------------------------
void CViewRender::ForceViewLeaf( int iViewLeaf )
{
	m_iForceViewLeaf = iViewLeaf;
}

//-----------------------------------------------------------------------------
// Gets znear + zfar
//-----------------------------------------------------------------------------
float CViewRender::GetZNear()
{
	return VIEW_NEARZ;
}

float CViewRender::GetZFar()
{
	// Initialize view structure with default values
	float farZ;
	if ( r_farz.GetFloat() < 1 )
	{
		// Use the far Z from the map's parameters.
		farZ = r_mapextents.GetFloat() * 1.73205080757f;
		
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if( pPlayer )
		{
			CPlayerLocalData *local = &pPlayer->m_Local;

			if ( local->m_fog.farz > 0 )
			{
				farZ = local->m_fog.farz;
			}
		}
	}
	else
	{
		farZ = r_farz.GetFloat();
	}

	return farZ;
}

	
//-----------------------------------------------------------------------------
// Sets up the view parameters
//-----------------------------------------------------------------------------
void CViewRender::SetUpView()
{
	// Initialize view structure with default values
	float farZ = GetZFar();

	m_View.zFar				= farZ;
	m_View.zFarViewmodel	= farZ;
	// UNDONE: Make this farther out? 
	//  closest point of approach seems to be view center to top of crouched box
	m_View.zNear			= GetZNear();
	m_View.zNearViewmodel	= 1;
	m_View.fov				= default_fov.GetFloat();

	m_View.context			= 0;
	m_View.m_bOrtho			= false;

	// Enable spatial partition access to edicts
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

#ifndef _XBOX
	if ( engine->IsHLTV() )
	{
		HLTVCamera()->CalcView( m_View.origin, m_View.angles, m_View.fov );
	}
	else
#endif
	{
		// FIXME: Are there multiple views? If so, then what?
		// FIXME: What happens when there's no player?
		if (pPlayer)
		{
			pPlayer->CalcView( m_View.origin, m_View.angles, m_View.zNear, m_View.zFar, m_View.fov );

			// If we are looking through another entities eyes, then override the angles/origin for m_View
			int viewentity = render->GetViewEntity();

			if ( !g_nKillCamMode && (pPlayer->entindex() != viewentity) )
			{
				C_BaseEntity *ve = cl_entitylist->GetEnt( viewentity );
				if ( ve )
				{
					VectorCopy( ve->GetAbsOrigin(), m_View.origin );
					VectorCopy( ve->GetAbsAngles(), m_View.angles );
				}
			}

			pPlayer->CalcViewModelView( m_View.origin, m_View.angles );
		}

		// FIXME:  Should this if go away and we move this code to CalcView?
		if ( !engine->IsPaused() )
		{
			g_pClientMode->OverrideView( &m_View );
		}
	}

#ifndef _XBOX
	// give the toolsystem a chance to override the view
	ToolFramework_SetupEngineView( m_View.origin, m_View.angles, m_View.fov );
#endif

	if ( engine->IsPlayingDemo() )
	{
		if ( cl_demoviewoverride.GetFloat() > 0.0f )
		{
			// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
			CalcDemoViewOverride( m_View.origin, m_View.angles );
		}
		else
		{
			s_DemoView = m_View.origin;
			s_DemoAngle = m_View.angles;
		}
	}

	//Find the offset our current FOV is from the default value
	float flFOVOffset = default_fov.GetFloat() - m_View.fov;

	//Adjust the viewmodel's FOV to move with any FOV offsets on the viewer's end
	m_View.fovViewmodel		= g_pClientMode->GetViewModelFOV() - flFOVOffset;

	// Disable spatical partition access
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	// Enable access to all model bones
	C_BaseAnimating::AllowBoneAccess( true, true );

	// Remember the origin, not reflected on a water plane.
	m_View.m_vUnreflectedOrigin = m_View.origin;
	
	// Compute the world->main camera transform
	ComputeCameraVariables( m_View.origin, m_View.angles, 
		&g_vecVForward, &g_vecVRight, &g_vecVUp, &g_matCamInverse );

	// set up the hearing origin...
#ifndef _XBOX
	ToolFramework_SetupEngineMicrophone( m_View.origin, m_View.angles );
#endif
	engine->SetHearingOrigin( m_View.origin, m_View.angles );

	g_vecPrevRenderOrigin = g_vecRenderOrigin;
	g_vecPrevRenderAngles = g_vecRenderAngles;
	g_vecRenderOrigin = m_View.origin;
	g_vecRenderAngles = m_View.angles;

	s_TestOrigin = m_View.origin;
	s_TestAngles = m_View.angles;
}

void CViewRender::WriteSaveGameScreenshotOfSize( const char *pFilename, int width, int height )
{
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();

	g_bRenderingScreenshot = true;

	// Push back buffer on the stack with small viewport
	materials->PushRenderTargetAndViewport( NULL, 0, 0, width, height );
	
	// render out to the backbuffer
	CViewSetup viewSetup = m_View;
	viewSetup.x = 0;
	viewSetup.y = 0;
	viewSetup.width = width;
	viewSetup.height = height;
	viewSetup.fov = ScaleFOVByWidthRatio( m_View.fov, ( (float)width / (float)height ) / ( 4.0f / 3.0f ) );
	viewSetup.m_bRenderToSubrectOfLargerScreen = true;
//	viewSetup.m_bDoBloomAndToneMapping	= true;

	// draw out the scene
	// Don't draw the HUD or the viewmodel
	RenderViewEx( viewSetup, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, 0 );

#ifndef _XBOX
	{
		// get the data from the backbuffer and save to disk
		// bitmap bits
		unsigned char *pImage = ( unsigned char * )malloc( width * 3 * height );

		// Get Bits from the material system
		materials->ReadPixels( 0, 0, width, height, pImage, IMAGE_FORMAT_RGB888 );

		// allocate a buffer to write the tga into
		int iMaxTGASize = 1024 + (width * height * 4);
		void *pTGA = malloc( iMaxTGASize );
		CUtlBuffer buffer( pTGA, iMaxTGASize );

		if( !TGAWriter::WriteToBuffer( pImage, buffer, width, height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 ) )
		{
			Error( "Couldn't write bitmap data snapshot.\n" );
		}
		
		free( pImage );

		// async write to disk (this will take ownership of the memory)
		char szPathedFileName[_MAX_PATH];
		Q_snprintf( szPathedFileName, sizeof(szPathedFileName), "//MOD/%s", pFilename );

		filesystem->AsyncWrite( szPathedFileName, buffer.Base(), buffer.TellPut(), true );
	}
#else
	// get bits from the material system
	unsigned char *pRawImage = new unsigned char[width*height*4];
	materials->ReadPixels( 0, 0, width, height, pRawImage, IMAGE_FORMAT_LINEAR_RGBA8888 );	

	int size = 4 + ImageLoader::GetMemRequired( width, height, 1, IMAGE_FORMAT_DXT1, false );
	int signatureSize = XBX_GetSigSize( XBX_CALCSIG_TYPE );
	unsigned char *pDxtImage = new unsigned char[size + signatureSize];

	((short*)pDxtImage)[0] = width;
	((short*)pDxtImage)[1] = height;
	XGCompressRect( 
		pDxtImage + 4, 
		D3DFMT_DXT1, 
		0, 
		width, 
		height, 
		pRawImage, 
		D3DFMT_LIN_A8B8G8R8,
		width*4, 
		0, 
		0 );

	delete [] pRawImage;

	XBX_CalculateSignature( pDxtImage, pDxtImage+size, size, XBX_CALCSIG_TYPE );

	// async write raw bits and free memory
	filesystem->AsyncWrite( pFilename, pDxtImage, size + signatureSize, true, false );
#endif

	// restore our previous state
	materials->PopRenderTargetAndViewport();
	
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
	
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();

	g_bRenderingScreenshot = false;
}

//-----------------------------------------------------------------------------
// Purpose: takes a screenshot of the save game
//-----------------------------------------------------------------------------
void CViewRender::WriteSaveGameScreenshot( const char *pFilename )
{
	WriteSaveGameScreenshotOfSize( pFilename, SAVEGAME_SCREENSHOT_WIDTH, SAVEGAME_SCREENSHOT_HEIGHT );
}

//-----------------------------------------------------------------------------
// Purpose: Sets up scene and renders camera view
// Input  : cameraNum - 
//			&cameraView
//			*localPlayer - 
//			x - 
//			y - 
//			width - 
//			height - 
//			highend - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CViewRender::DrawOneMonitor( ITexture *pRenderTarget, int cameraNum, C_PointCamera *pCameraEnt, 
	const CViewSetup &cameraView, C_BasePlayer *localPlayer, int x, int y, int width, int height )
{
#ifdef USE_MONITORS
	VPROF_INCREMENT_COUNTER( "cameras rendered", 1 );
	// Setup fog state for the camera.
	fogparams_t oldFogParams;
	float flOldZFar = 0.0f;

	bool fogEnabled = pCameraEnt->IsFogEnabled();

	CViewSetup monitorView = cameraView;

	if ( fogEnabled )
	{	
		if ( !localPlayer )
			return false;

		// Save old fog data.
		oldFogParams = localPlayer->m_Local.m_fog;
		flOldZFar = cameraView.zFar;

		localPlayer->m_Local.m_fog.enable = true;
		localPlayer->m_Local.m_fog.start = pCameraEnt->GetFogStart();
		localPlayer->m_Local.m_fog.end = pCameraEnt->GetFogEnd();
		localPlayer->m_Local.m_fog.farz = pCameraEnt->GetFogEnd();

		unsigned char r, g, b;
		pCameraEnt->GetFogColor( r, g, b );
		localPlayer->m_Local.m_fog.colorPrimary.SetR( r );
		localPlayer->m_Local.m_fog.colorPrimary.SetG( g );
		localPlayer->m_Local.m_fog.colorPrimary.SetB( b );

		monitorView.zFar = pCameraEnt->GetFogEnd();
	}

	monitorView.width = width;
	monitorView.height = height;
	monitorView.x = x;
	monitorView.y = y;
	monitorView.origin = pCameraEnt->GetAbsOrigin();
	monitorView.angles = pCameraEnt->GetAbsAngles();
	monitorView.fov = pCameraEnt->GetFOV();
	monitorView.m_bOrtho = false;
	monitorView.m_flAspectRatio = pCameraEnt->UseScreenAspectRatio() ? 0.0f : 1.0f;

 	render->Push3DView( monitorView, VIEW_CLEAR_DEPTH | VIEW_CLEAR_COLOR, true, pRenderTarget, m_Frustum );
	ViewDrawScene( false, true, monitorView, 0, VIEW_MONITOR );
 	render->PopView( m_Frustum );

	// Reset the world fog parameters.
	if ( fogEnabled )
	{
		localPlayer->m_Local.m_fog = oldFogParams;
		monitorView.zFar = flOldZFar;
	}
#endif // USE_MONITORS
	return true;
}

void CViewRender::DrawMonitors( const CViewSetup &cameraView )
{
#ifdef USE_MONITORS

	// Early out if no cameras
	C_PointCamera *pCameraEnt = GetPointCameraList();
	if ( !pCameraEnt )
	{
		return;
	}

#ifdef _DEBUG
	g_bRenderingCameraView = true;
#endif

	// FIXME: this should check for the ability to do a render target maybe instead.
	// FIXME: shouldn't have to truck through all of the visible entities for this!!!!
	ITexture *pCameraTarget = GetCameraTexture();
	int width = pCameraTarget->GetActualWidth();
	int height = pCameraTarget->GetActualHeight();

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	
	for ( int cameraNum = 0; pCameraEnt != NULL; pCameraEnt = pCameraEnt->m_pNext )
	{
		if ( !pCameraEnt->IsActive() || pCameraEnt->IsDormant() )
			continue;

		if ( !DrawOneMonitor( pCameraTarget, cameraNum, pCameraEnt, cameraView, player, 0, 0, width, height ) )
			continue;

		++cameraNum;
	}

#ifdef _DEBUG
	g_bRenderingCameraView = false;
#endif

#endif // USE_MONITORS
}


float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets view parameters for level overview mode
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::SetUpOverView()
{
	static int oldCRC = 0;

	m_View.m_bOrtho = true;

	float aspect = (float)m_View.width/(float)m_View.height;

	int size_y = 1024.0f * cl_leveloverview.GetFloat(); // scale factor, 1024 = OVERVIEW_MAP_SIZE
	int	size_x = size_y * aspect;	// standard screen aspect 

	m_View.origin.x -= size_x / 2;
	m_View.origin.y += size_y / 2;

	m_View.m_OrthoLeft   = 0;
	m_View.m_OrthoTop    = -size_y;
	m_View.m_OrthoRight  = size_x;
	m_View.m_OrthoBottom = 0;

	m_View.angles = QAngle( 90, 90, 0 );

	// simple movement detector, show position if moved
	int newCRC = m_View.origin.x + m_View.origin.y + m_View.origin.z;
	if ( newCRC != oldCRC )
	{
		Msg( "Overview: scale %.2f, pos_x %.0f, pos_y %.0f\n", cl_leveloverview.GetFloat(),
			m_View.origin.x, m_View.origin.y );
		oldCRC = newCRC;
	}

	materials->ClearColor4ub( 0, 255, 0, 255 );

	// render->DrawTopView( true );
}

//-----------------------------------------------------------------------------
// Purpose: Render current view into specified rectangle
// Input  : *rect - 
//-----------------------------------------------------------------------------
void CViewRender::Render( vrect_t *rect )
{
	Assert(s_TestOrigin == m_View.origin);
	Assert(s_TestAngles == m_View.angles);

	VPROF_BUDGET( "CViewRender::Render", "CViewRender::Render" );

	vrect_t vr = *rect;

	// Stub out the material system if necessary.
#ifndef _XBOX
	CMatStubHandler matStub;
#endif
	bool drawViewModel;

	engine->EngineStats_BeginFrame();
	
	// Assume normal vis
	m_bForceNoVis			= false;
	m_bOverrideVisOrigin	= false;
	m_nNumVisOrigins		= 0;
	m_bOverrideVisData		= false;
	m_iForceViewLeaf		= -1;

	float aspectRatio = engine->GetScreenAspectRatio() * 0.75f;	 // / (4/3)
	m_View.fov = ScaleFOVByWidthRatio( m_View.fov,  aspectRatio );
	m_View.fovViewmodel = ScaleFOVByWidthRatio( m_View.fovViewmodel, aspectRatio );
	
	// Let the client mode hook stuff.
	g_pClientMode->PreRender(&m_View);

	g_pClientMode->AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );
#if !defined( _XBOX )
	ToolFramework_AdjustEngineViewport( vr.x, vr.y, vr.width, vr.height );
#endif

	float flViewportScale = mat_viewportscale.GetFloat();
	if ( flViewportScale == 0.0f )
	{
		flViewportScale = 1.0f;
	}

	m_View.x				= vr.x;
	m_View.y				= vr.y;
	m_View.width			= vr.width * flViewportScale;
	m_View.height			= vr.height * flViewportScale;
	m_View.m_flAspectRatio	= (float)m_View.width / (float)m_View.height;
#ifdef _XBOX
	m_View.m_flAspectRatio *= aspectRatio;	// xbox is anamorphic, scale to match pixel shape
#endif

	int nClearFlags = VIEW_CLEAR_DEPTH;
	if ( gl_clear.GetInt() != 0 )
	{
		nClearFlags |= VIEW_CLEAR_COLOR;
	}

	// Determine if we should draw view model ( client mode override )
	drawViewModel = g_pClientMode->ShouldDrawViewModel();

	if ( cl_leveloverview.GetFloat() > 0 )
	{
		SetUpOverView();		
		nClearFlags |= VIEW_CLEAR_COLOR;
		drawViewModel = false;
	}

	// Apply any player specific overrides
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		// Override view model if necessary
		if ( !pPlayer->m_Local.m_bDrawViewmodel )
		{
			drawViewModel = false;
		}
	}

	render->SetMainView( m_View.origin, m_View.angles );

	RenderView( m_View, nClearFlags, drawViewModel );

	g_pClientMode->PostRender();

	engine->EngineStats_EndFrame();

#ifndef _XBOX
	// Stop stubbing the material system so we can see the budget panel
	matStub.End();
#endif

	CViewSetup view2d;

	// Draw all of the UI stuff "fullscreen"
	view2d.x				= rect->x;
	view2d.y				= rect->y;
	view2d.width			= rect->width;
	view2d.height			= rect->height;
	render->Push2DView( view2d, 0, false, NULL, m_Frustum );
	render->VGui_Paint( PAINT_UIPANELS );
	render->PopView( m_Frustum );
}

CON_COMMAND( spec_pos, "dump position and angles to the console" )
{
	Warning( "spec_goto %.1f %.1f %.1f %.1f %.1f\n", MainViewOrigin().x, MainViewOrigin().y, 
		MainViewOrigin().z, MainViewAngles().x, MainViewAngles().y );
}

CON_COMMAND( getpos, "dump position and angles to the console" )
{
	Warning( "setpos %f %f %f;", MainViewOrigin().x, MainViewOrigin().y, MainViewOrigin().z );
	Warning( "setang %f %f %f\n", MainViewAngles().x, MainViewAngles().y, MainViewAngles().z );
}

