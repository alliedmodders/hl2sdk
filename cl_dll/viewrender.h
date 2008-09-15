//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( VIEWRENDER_H )
#define VIEWRENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "tier1/utlstack.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CRenderList;
class IClientVehicle;
class C_PointCamera;
class IScreenSpaceEffect;
enum ScreenSpaceEffectType_t;

#ifdef HL2_EPISODIC
	class CStunEffect;
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Data specific to intro mode to control rendering.
//-----------------------------------------------------------------------------
struct IntroDataBlendPass_t
{
	int m_BlendMode;
	float m_Alpha; // in [0.0f,1.0f]  This needs to add up to 1.0 for all passes, unless you are fading out.
};

struct IntroData_t
{
	bool	m_bDrawPrimary;
	bool	m_bDrawSecondary;
	Vector	m_vecCameraView;
	QAngle	m_vecCameraViewAngles;
	float	m_playerViewFOV;
	CUtlVector<IntroDataBlendPass_t> m_Passes;

	// Fade overriding for the intro
	float	m_flCurrentFadeColor[4];
};

// Robin, make this point at something to get intro mode.
extern IntroData_t *g_pIntroData;

// This identifies the view for certain systems that are unique per view (e.g. pixel visibility)
// NOTE: This is identifying which logical part of the scene an entity is being redered in
// This is not identifying a particular render target necessarily.  This is mostly needed for entities that
// can be rendered more than once per frame (pixel vis queries need to be identified per-render call)
enum view_id_t
{
	VIEW_NONE = -1,
	VIEW_MAIN = 0,
	VIEW_3DSKY = 1,
	VIEW_MONITOR = 2,
	VIEW_REFLECTION = 3,
	VIEW_REFRACTION = 4,
	VIEW_INTRO_PLAYER = 5,
	VIEW_INTRO_CAMERA = 6
};


//-----------------------------------------------------------------------------
// Purpose: Stored pitch drifting variables
//-----------------------------------------------------------------------------
class CPitchDrift
{
public:
	float		pitchvel;
	bool		nodrift;
	float		driftmove;
	double		laststop;
};


//-----------------------------------------------------------------------------
// Purpose: Stored pitch drifting variables
//-----------------------------------------------------------------------------
struct ClientWorldListInfo_t : public WorldListInfo_t
{
	ClientWorldListInfo_t() : m_pActualLeafIndex(0) {}

	// Because we remap leaves to eliminate unused leaves, we need a remap
	// when drawing translucent surfaces, which requires the *original* leaf index
	// using m_pActualLeafMap[ remapped leaf index ] == actual leaf index
	LeafIndex_t *m_pActualLeafIndex;
};


//-----------------------------------------------------------------------------
// Purpose: Implements the interface to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CViewRender : public IViewRender
{
public:
					CViewRender();
	virtual			~CViewRender( void ) {}

// Implementation of IViewRender interface
public:

	virtual void	Init( void );
	virtual void	Shutdown( void );

	// Render functions
	virtual void	OnRenderStart();
	virtual	void	Render( vrect_t *rect );
	virtual void	RenderView( const CViewSetup &view, int nClearFlags, bool drawViewmodel );
	virtual void	RenderPlayerSprites();
	virtual void	Render2DEffectsPreHUD( const CViewSetup &view );
	virtual void	Render2DEffectsPostHUD( const CViewSetup &view );

	// What are we currently rendering? Returns a combination of DF_ flags.
	virtual int		GetDrawFlags();

	virtual void	StartPitchDrift( void );
	virtual void	StopPitchDrift( void );

	// Called once per level change
	void			LevelInit( void );
	void			LevelShutdown( void );

	// Add entity to transparent entity queue

	virtual VPlane*	GetFrustum();

	bool			ShouldDrawBrushModels( void );
	bool			ShouldDrawEntities( void );

	const CViewSetup *GetViewSetup( ) const;
	const CViewSetup *GetPlayerViewSetup( ) const;
	
	void			AddVisOrigin( const Vector& origin );
	void			ClearAllCustomVisOrigins ( void );		  // Remove all current vis origins in the list, return to using the main view
	void			DisableVis( void );

	void			ForceVisOverride ( VisOverrideData_t& visData );
	void			ForceViewLeaf ( int iViewLeaf );

	int				FrameNumber() const;
	int				BuildWorldListsNumber() const;

	// Sets up the view model position relative to the local player
	void			MoveViewModels( );

	// Gets the abs origin + angles of the view models
	void			GetViewModelPosition( int nIndex, Vector *pPos, QAngle *pAngle );

	void			SetCheapWaterStartDistance( float flCheapWaterStartDistance );
	void			SetCheapWaterEndDistance( float flCheapWaterEndDistance );

	void			GetWaterLODParams( float &flCheapWaterStartDistance, float &flCheapWaterEndDistance );

	void			DriftPitch (void);

	virtual void	RenderViewEx( const CViewSetup &view, int nClearFlags, int whatToDraw );

	virtual void	QueueOverlayRenderView( const CViewSetup &view, int nClearFlags, int whatToDraw );

	virtual float	GetZNear();
	virtual float	GetZFar();
	virtual void	GetScreenFadeDistances( float *min, float *max );

private:
	struct WaterRenderInfo_t
	{
		bool m_bCheapWater;
		bool m_bReflect;
		bool m_bRefract;
		bool m_bReflectEntities;
		bool m_bDrawWaterSurface;
		bool m_bOpaqueWater;
	};

	// Draw setup
	void			BoundOffsets( void );

	float			CalcRoll (const QAngle& angles, const Vector& velocity, float rollangle, float rollspeed);

	void			SetupRenderList( const CViewSetup *pView, ClientWorldListInfo_t& info, CRenderList &renderList );

	// General draw methods
	// baseDrawFlags is a combination of DF_ defines. DF_MONITOR is passed into here while drawing a monitor.
	void			ViewDrawScene( bool bDrew3dSkybox, bool bSkyboxVisible, const CViewSetup &view, int nClearFlags, view_id_t viewID, bool bDrawViewModel = false, int baseDrawFlags = 0 );

	void			Draw3dSkyboxworld( const CViewSetup &view, int &nClearFlags, bool &bDrew3dSkybox, bool &bSkyboxVisible );
	
	// If iForceViewLeaf is not -1, then it uses the specified leaf as your starting area for setting up area portal culling.
	// This is used by water since your reflected view origin is often in solid space, but we still want to treat it as though
	// the first portal we're looking out of is a water portal, so our view effectively originates under the water.
	void			BuildWorldRenderLists( const CViewSetup *pView, ClientWorldListInfo_t& info, bool bUpdateLightmaps, bool bDrawEntities, int iForceViewLeaf );

	// Purpose: Builds render lists for renderables. Called once for refraction, once for over water
	void			BuildRenderableRenderLists( const CViewSetup *pView, ClientWorldListInfo_t& info, CRenderList &renderList );

	void			DrawWorld( ClientWorldListInfo_t& info, CRenderList &renderList, int flags, float waterZAdjust );

	void			DrawMonitors( const CViewSetup &cameraView );

	bool			DrawOneMonitor( ITexture *pRenderTarget, int cameraNum, C_PointCamera *pCameraEnt, const CViewSetup &cameraView, C_BasePlayer *localPlayer, 
						int x, int y, int width, int height );

	void			SetupVis( const CViewSetup& view, unsigned int &visFlags );

	// Drawing primitives
	bool			ShouldDrawViewModel( bool drawViewmodel );
	void			DrawViewModels( const CViewSetup &view, bool drawViewmodel );

	// Fog setup
	void			EnableWorldFog( void );
	void			Enable3dSkyboxFog( void );
	void			DisableFog( void );
	
	// Draws all opaque/translucent renderables in leaves that were rendered
	void			DrawOpaqueRenderables( ClientWorldListInfo_t& info, CRenderList &renderList );
	void			DrawTranslucentRenderables( ClientWorldListInfo_t& info,
						CRenderList &renderList, int nDrawFlags, bool bInSkybox );

	// Renders all translucent entities in the render list
	void			DrawTranslucentRenderablesNoWorld( CRenderList &renderList, bool bInSkybox );

	// Sets up the view parameters
	void			SetUpView();
	// Sets up the view parameters of map overview mode (cl_leveloverview)
	void			SetUpOverView();

	// Purpose: Renders world and all entities, etc.
	void			DrawWorldAndEntities( bool drawSkybox, const CViewSetup &view, int nClearFlags );

	// Draws all the debugging info
	void			Draw3DDebuggingInfo( const CViewSetup &view );
	void			Draw2DDebuggingInfo( const CViewSetup &view );

	void			PerformScreenSpaceEffects( int x, int y, int w, int h );

	// Overlays
	void			SetScreenOverlayMaterial( IMaterial *pMaterial );
	IMaterial		*GetScreenOverlayMaterial( );
	void			PerformScreenOverlay( int x, int y, int w, int h );

	// Water-related methods
	void			WaterDrawWorldAndEntities( bool drawSkybox, const CViewSetup &view, int nClearFlags );
	
	void			WaterDrawHelper( const CViewSetup &view, ClientWorldListInfo_t &info, CRenderList &renderList, 
						float waterHeight, int flags, view_id_t viewID, float waterZAdjust, int iForceViewLeaf );
	
	void			PushWaterRenderTarget( CViewSetup &view, int nClearFlags, float waterHeight, int flags );	// see DrawFlags_t
	void			PopWaterRenderTarget( int nFlags );

	void			ViewDrawScene_EyeAboveWater( bool bDrawSkybox, const CViewSetup &view, int nClearFlags, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info );
	void			ViewDrawScene_EyeUnderWater( bool bDrawSkybox, const CViewSetup &view, int nClearFlags, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info );
	void			ViewDrawScene_NoWater( bool bDrawSkybox, const CViewSetup &view, int nClearFlags, const VisibleFogVolumeInfo_t &fogInfo, const WaterRenderInfo_t& info );
	void			ViewDrawScene_Intro( const CViewSetup &view, int nClearFlags, const IntroData_t &introData );

#ifdef _XBOX
	// Draws a perspective-correct dudv map into the reflection texture
	void			DrawScreenSpaceWaterDuDv( const CViewSetup &view, float waterZAdjust );
#endif

	// Renders all translucent world surfaces in a particular set of leaves
	void			DrawTranslucentWorldInLeaves( int iCurLeaf, int iFinalLeaf, ClientWorldListInfo_t &info, int nDrawFlags );

	// Renders all translucent world + detail objects in a particular set of leaves
	void			DrawTranslucentWorldAndDetailPropsInLeaves( int iCurLeaf, int iFinalLeaf, ClientWorldListInfo_t &info, int nDrawFlags, int &nDetailLeafCount, LeafIndex_t* pDetailLeafList );

	// Computes us some geometry to render the frustum planes
	void			ComputeFrustumRenderGeometry( Vector pRenderPoint[8] );

	// generates a low-res screenshot for save games
	void			WriteSaveGameScreenshot( const char *filename );

	// renders the frustum
	void			RenderFrustum( );

	// Purpose: Computes the actual world list info based on the render flags
	ClientWorldListInfo_t *ComputeActualWorldListInfo( const ClientWorldListInfo_t& info, int nDrawFlags, ClientWorldListInfo_t& tmpInfo );

	// Determines what kind of water we're going to use
	void			DetermineWaterRenderInfo( const VisibleFogVolumeInfo_t &fogVolumeInfo, CViewRender::WaterRenderInfo_t &info );
	
	void			DrawRenderablesInList( CUtlVector< IClientRenderable * > &list );

	virtual void	WriteSaveGameScreenshotOfSize( const char *pFilename, int width, int height );

	void			DoScreenSpaceBloom();

	// Sets up, cleans up the main 3D view
	void			SetupMain3DView( const CViewSetup &view, int &nClearFlags );
	void			CleanupMain3DView( const CViewSetup &view );

private:
	enum
	{
		ANGLESHISTORY_SIZE	= 8,
		ANGLESHISTORY_MASK	= 7,
	};

	// Combination of DF_ flags.
	int m_DrawFlags;
	int m_BaseDrawFlags;	// Set in ViewDrawScene and OR'd into m_DrawFlags as it goes.

	// This stores all of the view setup parameters that the engine needs to know about
	CViewSetup		m_View;
	
	// This stores the current view
 	CViewSetup		m_CurrentView;

	// VIS Overrides
	// Set to true to turn off client side vis ( !!!! rendering will be slow since everything will draw )
	bool			m_bForceNoVis;		

	// Set to true if you want to use multiple origins for doing client side map vis culling
	// NOTE:  In generaly, you won't want to do this, and by default the 3d origin of the camera, as above,
	//  will be used as the origin for vis, too.
	bool			m_bOverrideVisOrigin;
	// Number of origins to use from m_rgVisOrigins
	int				m_nNumVisOrigins;
	// Array of origins
	Vector			m_rgVisOrigins[ MAX_VIS_LEAVES ];

	// The view data overrides for visibility calculations with area portals
	VisOverrideData_t m_VisData;
	bool			m_bOverrideVisData;

	// The starting leaf to determing which area to start in when performing area portal culling on the engine
	// Default behavior is to use the leaf the camera position is in.
	int				m_iForceViewLeaf;

	Frustum			m_Frustum;

	// Pitch drifting data
	CPitchDrift		m_PitchDrift;

	// For tracking angles history.
	QAngle			m_AnglesHistory[ANGLESHISTORY_SIZE];
	int				m_AnglesHistoryCounter;

	// The frame number
	int				m_FrameNumber;
	int				m_BuildWorldListsNumber;
	int				m_BuildRenderableListsNumber;

	// Some cvars needed by this system
	const ConVar	*m_pDrawEntities;
	const ConVar	*m_pDrawBrushModels;

	// Some materials used...
	CMaterialReference	m_TranslucentSingleColor;
	CMaterialReference	m_ModulateSingleColor;
	CMaterialReference	m_ScreenOverlayMaterial;

	Vector			m_vecLastFacing;
	float			m_flCheapWaterStartDistance;
	float			m_flCheapWaterEndDistance;

#ifndef _XBOX
	CViewSetup			m_OverlayViewSetup;
	int					m_OverlayClearFlags;
	int					m_OverlayDrawFlags;
	bool				m_bDrawOverlay;
#endif

#ifdef _XBOX
	CMaterialReference	m_BloomDownsample;
	CMaterialReference	m_BloomBlurX;
	CMaterialReference	m_BloomBlurY;
	CMaterialReference	m_BloomAdd;
#endif

};

#endif // VIEWRENDER_H
