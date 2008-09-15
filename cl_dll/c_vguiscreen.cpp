//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "networkstringtable_clientdll.h"
#include <KeyValues.h>
#include "PanelMetaClassMgr.h"
#include <vgui_controls/Controls.h>
#include "VMatrix.h"
#include "VGUIMatSurface/IMatSystemSurface.h"
#include "view.h"
#include "CollisionUtils.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IVGui.h>
#include "ienginevgui.h"
#include "in_buttons.h"
#include <vgui/Mousecode.h>
#include "materialsystem/IMesh.h"
#include "ClientEffectPrecacheSystem.h"
#include "C_VGuiScreen.h"
#include "IClientMode.h"
#include "vgui_bitmapbutton.h"
#include "vgui_bitmappanel.h"
#include "filesystem.h"

#include <vgui/IInputInternal.h>
extern vgui::IInputInternal *g_InputInternal;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define VGUI_SCREEN_MODE_RADIUS	80

//Precache the materials
CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectVGuiScreen )
CLIENTEFFECT_MATERIAL( "engine/writez" )
CLIENTEFFECT_REGISTER_END()



// ----------------------------------------------------------------------------- //
// This is a cache of preloaded keyvalues.
// ----------------------------------------------------------------------------- // 

CUtlDict<KeyValues*, int> g_KeyValuesCache;

KeyValues* CacheKeyValuesForFile( const char *pFilename )
{
	MEM_ALLOC_CREDIT();
	int i = g_KeyValuesCache.Find( pFilename );
	if ( i == g_KeyValuesCache.InvalidIndex() )
	{
		KeyValues *rDat = new KeyValues( pFilename );
		rDat->LoadFromFile( filesystem, pFilename, NULL );
		g_KeyValuesCache.Insert( pFilename, rDat );
		return rDat;		
	}
	else
	{
		return g_KeyValuesCache[i];
	}
}

void ClearKeyValuesCache()
{
	MEM_ALLOC_CREDIT();
	for ( int i=g_KeyValuesCache.First(); i != g_KeyValuesCache.InvalidIndex(); i=g_KeyValuesCache.Next( i ) )
	{
		g_KeyValuesCache[i]->deleteThis();
	}
	g_KeyValuesCache.Purge();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_VGuiScreen : public C_BaseEntity
{
	DECLARE_CLASS( C_VGuiScreen, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_VGuiScreen();

	virtual void PreDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual int DrawModel( int flags );
	virtual bool ShouldDraw() { return !IsEffectActive(EF_NODRAW); }
	virtual void ClientThink( );
	virtual void GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles );

	const char *PanelName() const;

	// The view screen has the cursor pointing at it
	void GainFocus( );
	void LoseFocus();

	// Button state...
	void SetButtonState( int nButtonState );

	// Is the screen backfaced given a view position?
	bool IsBackfacing( const Vector &viewOrigin );

	// Return intersection point of ray with screen in barycentric coords
	bool IntersectWithRay( const Ray_t &ray, float *u, float *v, float *t );

	// Is the screen turned on?
	bool IsActive() const;

	// Are we only visible to teammates?
	bool IsVisibleOnlyToTeammates() const;

	// Are we visible to someone on this team?
	bool IsVisibleToTeam( int nTeam );

	bool IsAttachedToViewModel() const;

	virtual RenderGroup_t GetRenderGroup();

	bool AcceptsInput() const;
	void SetAcceptsInput( bool acceptsinput );

private:
	// Vgui screen management
	void CreateVguiScreen( const char *pTypeName );
	void DestroyVguiScreen( );

	//  Computes the panel to world transform
	void ComputePanelToWorld();

	// Computes control points of the quad describing the screen
	void ComputeEdges( Vector *pUpperLeft, Vector *pUpperRight, Vector *pLowerLeft );

	// Writes the z buffer
	void DrawScreenOverlay();

private:
	int m_nPixelWidth; 
	int m_nPixelHeight;
	float m_flWidth; 
	float m_flHeight;
	int m_nPanelName;	// The name of the panel 
	int	m_nButtonState;
	int m_nButtonPressed;
	int m_nButtonReleased;
	int m_nOldPx;
	int m_nOldPy;
	int m_nOldButtonState;
	int m_nAttachmentIndex;
	int m_nOverlayMaterial;
	int m_fScreenFlags;

	int	m_nOldPanelName;
	int m_nOldOverlayMaterial;

	bool m_bLooseThinkNextFrame;

	bool	m_bAcceptsInput;

	CMaterialReference	m_WriteZMaterial;
	CMaterialReference	m_OverlayMaterial;

	VMatrix	m_PanelToWorld;

	CPanelWrapper m_PanelWrapper;
};

IMPLEMENT_CLIENTCLASS_DT(C_VGuiScreen, DT_VGuiScreen, CVGuiScreen)
	RecvPropFloat( RECVINFO(m_flWidth) ),
	RecvPropFloat( RECVINFO(m_flHeight) ),
	RecvPropInt( RECVINFO(m_fScreenFlags) ),
	RecvPropInt( RECVINFO(m_nPanelName) ),
	RecvPropInt( RECVINFO(m_nAttachmentIndex) ),
	RecvPropInt( RECVINFO(m_nOverlayMaterial) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
C_VGuiScreen::C_VGuiScreen()
{
	m_nOldPanelName = m_nPanelName = -1;
	m_nOldOverlayMaterial = m_nOverlayMaterial = -1;
	m_nOldPx = m_nOldPy = -1;
	m_nButtonState = 0;
	m_bLooseThinkNextFrame = false;
	m_bAcceptsInput = true;

	m_WriteZMaterial.Init( "engine/writez", TEXTURE_GROUP_VGUI );
	m_OverlayMaterial.Init( m_WriteZMaterial );
}

//-----------------------------------------------------------------------------
// Network updates
//-----------------------------------------------------------------------------
void C_VGuiScreen::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );
	m_nOldPanelName = m_nPanelName;
	m_nOldOverlayMaterial = m_nOverlayMaterial;
}

void C_VGuiScreen::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ((type == DATA_UPDATE_CREATED) || (m_nPanelName != m_nOldPanelName))
	{
		CreateVguiScreen( PanelName() );
		m_nButtonState = 0;
	}

	// Set up the overlay material
	if (m_nOldOverlayMaterial != m_nOverlayMaterial)
	{
		m_OverlayMaterial.Shutdown();

		const char *pMaterialName = GetMaterialNameFromIndex(m_nOverlayMaterial);
		if (pMaterialName)
		{
			m_OverlayMaterial.Init( pMaterialName, TEXTURE_GROUP_VGUI );
		}
		else
		{
			m_OverlayMaterial.Init( m_WriteZMaterial );
		}
	}
}

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );

//-----------------------------------------------------------------------------
// Returns the attachment render origin + origin
//-----------------------------------------------------------------------------
void C_VGuiScreen::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pOrigin, QAngle *pAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();
	if (pEnt && (m_nAttachmentIndex > 0))
	{
		C_BaseAnimating::PushAllowBoneAccess( true, true );
		pEnt->GetAttachment( m_nAttachmentIndex, *pOrigin, *pAngles );
		C_BaseAnimating::PopBoneAccess();
		
		if ( IsAttachedToViewModel() )
		{
			FormatViewModelAttachment( *pOrigin, true );
		}
	}
	else
	{
		BaseClass::GetAimEntOrigin( pAttachedTo, pOrigin, pAngles );
	}
}


//-----------------------------------------------------------------------------
// Create, destroy vgui panels...
//-----------------------------------------------------------------------------
void C_VGuiScreen::CreateVguiScreen( const char *pTypeName )
{
	// Clear out any old screens.
	DestroyVguiScreen();

	// Create the new screen...
	VGuiScreenInitData_t initData( this );
	m_PanelWrapper.Activate( pTypeName, NULL, 0, &initData );

	// Retrieve the panel dimensions
	vgui::Panel *pPanel = m_PanelWrapper.GetPanel();
	if (pPanel)
	{
		int x, y;
		pPanel->GetBounds( x, y, m_nPixelWidth, m_nPixelHeight );
	}
	else
	{
		m_nPixelWidth = m_nPixelHeight = 0;
	}
}

void C_VGuiScreen::DestroyVguiScreen( )
{
	m_PanelWrapper.Deactivate();
}


//-----------------------------------------------------------------------------
// Is the screen active?
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IsActive() const
{
	return (m_fScreenFlags & VGUI_SCREEN_ACTIVE) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IsAttachedToViewModel() const
{
	return (m_fScreenFlags & VGUI_SCREEN_ATTACHED_TO_VIEWMODEL) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_VGuiScreen::AcceptsInput() const
{
	return m_bAcceptsInput;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : acceptsinput - 
//-----------------------------------------------------------------------------
void C_VGuiScreen::SetAcceptsInput( bool acceptsinput )
{
	m_bAcceptsInput = acceptsinput;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_VGuiScreen::GetRenderGroup()
{
	if ( IsAttachedToViewModel() )
		return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT;

	return BaseClass::GetRenderGroup();
}

//-----------------------------------------------------------------------------
// Are we only visible to teammates?
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IsVisibleOnlyToTeammates() const
{
	return (m_fScreenFlags & VGUI_SCREEN_VISIBLE_TO_TEAMMATES) != 0;
}

//-----------------------------------------------------------------------------
// Are we visible to someone on this team?
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IsVisibleToTeam( int nTeam )
{
	// FIXME: Should this maybe go into a derived class of some sort?
	// Don't bother with screens on the wrong team
	if (IsVisibleOnlyToTeammates() && (nTeam > 0))
	{
		// Hmmm... sort of a hack...
		C_BaseEntity *pOwner = GetOwnerEntity();
		if ( pOwner && (nTeam != pOwner->GetTeamNumber()) )
			return false;
	}
	
	return true;
}


//-----------------------------------------------------------------------------
// Activate, deactivate the view screen
//-----------------------------------------------------------------------------
void C_VGuiScreen::GainFocus( )
{
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_VGuiScreen::LoseFocus()
{
	m_bLooseThinkNextFrame = true;
	m_nOldButtonState = 0;
}

void C_VGuiScreen::SetButtonState( int nButtonState )
{
	m_nOldButtonState = m_nButtonState;
	m_nButtonState = nButtonState;

 	int nButtonsChanged = m_nOldButtonState ^ m_nButtonState;
	
	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_nButtonPressed =  nButtonsChanged & m_nButtonState;		// The changed ones still down are "pressed"
	m_nButtonReleased = nButtonsChanged & (~m_nButtonState);	// The ones not down are "released"
}



//-----------------------------------------------------------------------------
// Returns the panel name 
//-----------------------------------------------------------------------------
const char *C_VGuiScreen::PanelName() const
{
	return g_StringTableVguiScreen->GetString( m_nPanelName );
}


//-----------------------------------------------------------------------------
// Purpose: Deal with input
//-----------------------------------------------------------------------------
void C_VGuiScreen::ClientThink( void )
{
	BaseClass::ClientThink();

	// FIXME: We should really be taking bob, shake, and roll into account
	// but if we did, then all the inputs would be generated multiple times
	// if the world was rendered multiple times (for things like water, etc.)

	vgui::Panel *pPanel = m_PanelWrapper.GetPanel();
	if (!pPanel)
		return;
	
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer)
		return;

	// Generate a ray along the view direction
	Vector vecEyePosition = pLocalPlayer->EyePosition();
	
	QAngle viewAngles = pLocalPlayer->EyeAngles( );

	Vector viewDir, endPos;
	AngleVectors( viewAngles, &viewDir );
	VectorMA( vecEyePosition, 1000.0f, viewDir, endPos );

	// Compute cursor position...
	Ray_t lookDir;
	lookDir.Init( vecEyePosition, endPos );
	
	float u, v;

	if (!IntersectWithRay( lookDir, &u, &v, NULL ))
		return;

	if ( ((u < 0) || (v < 0) || (u > 1) || (v > 1)) && !m_bLooseThinkNextFrame)
		return;

	// This will cause our panel to grab all input!
	g_pClientMode->ActivateInGameVGuiContext( pPanel );

	// Convert (u,v) into (px,py)
	int px = (int)(u * m_nPixelWidth + 0.5f);
	int py = (int)(v * m_nPixelHeight + 0.5f);

	// Generate mouse input commands
	if ((px != m_nOldPx) || (py != m_nOldPy))
	{
		g_InputInternal->InternalCursorMoved( px, py );
		m_nOldPx = px;
		m_nOldPy = py;
	}

	if (m_nButtonPressed & IN_ATTACK)
	{
		g_InputInternal->InternalMousePressed(vgui::MOUSE_LEFT);
	}
	if (m_nButtonPressed & IN_ATTACK2)
	{
		g_InputInternal->InternalMousePressed(vgui::MOUSE_RIGHT);
	}
	if ( (m_nButtonReleased & IN_ATTACK) || m_bLooseThinkNextFrame) // for a button release on loosing focus
	{
		g_InputInternal->InternalMouseReleased(vgui::MOUSE_LEFT);
	}
	if (m_nButtonReleased & IN_ATTACK2)
	{
		g_InputInternal->InternalMouseReleased(vgui::MOUSE_RIGHT);
	}

	if ( m_bLooseThinkNextFrame == true )
	{
		m_bLooseThinkNextFrame = false;
		SetNextClientThink( CLIENT_THINK_NEVER );
	}


	g_pClientMode->DeactivateInGameVGuiContext( );
}


//-----------------------------------------------------------------------------
// Computes control points of the quad describing the screen
//-----------------------------------------------------------------------------
void C_VGuiScreen::ComputeEdges( Vector *pUpperLeft, Vector *pUpperRight, Vector *pLowerLeft )
{
	Vector vecOrigin = GetAbsOrigin();
	Vector xaxis, yaxis;
	AngleVectors( GetAbsAngles(), &xaxis, &yaxis, NULL );

	// NOTE: Have to multiply by -1 here because yaxis goes out the -y axis in AngleVectors actually...
	yaxis *= -1.0f;

	VectorCopy( vecOrigin, *pLowerLeft );
	VectorMA( vecOrigin, m_flHeight, yaxis, *pUpperLeft );
	VectorMA( *pUpperLeft, m_flWidth, xaxis, *pUpperRight );
}


//-----------------------------------------------------------------------------
// Return intersection point of ray with screen in barycentric coords
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IntersectWithRay( const Ray_t &ray, float *u, float *v, float *t )
{
	// Perform a raycast to see where in barycentric coordinates the ray hits
	// the viewscreen; if it doesn't hit it, you're not in the mode
	Vector origin, upt, vpt;
	ComputeEdges( &origin, &upt, &vpt );
	return ComputeIntersectionBarycentricCoordinates( ray, origin, upt, vpt, *u, *v, t );
}


//-----------------------------------------------------------------------------
// Is the vgui screen backfacing?
//-----------------------------------------------------------------------------
bool C_VGuiScreen::IsBackfacing( const Vector &viewOrigin )
{
	// Compute a ray from camera to center of the screen..
	Vector cameraToScreen;
	VectorSubtract( GetAbsOrigin(), viewOrigin, cameraToScreen );

	// Figure out the face normal
	Vector zaxis;
	GetVectors( NULL, NULL, &zaxis );

	// The actual backface cull
	return (DotProduct( zaxis, cameraToScreen ) > 0.0f);
}


//-----------------------------------------------------------------------------
//  Computes the panel center to world transform
//-----------------------------------------------------------------------------
void C_VGuiScreen::ComputePanelToWorld()
{
	// The origin is at the upper-left corner of the screen
	Vector vecOrigin, vecUR, vecLL;
	ComputeEdges( &vecOrigin, &vecUR, &vecLL );
	m_PanelToWorld.SetupMatrixOrgAngles( vecOrigin, GetAbsAngles() );
}


//-----------------------------------------------------------------------------
// a pass to set the z buffer...
//-----------------------------------------------------------------------------
void C_VGuiScreen::DrawScreenOverlay()
{
	materials->MatrixMode( MATERIAL_MODEL );
	materials->PushMatrix();
	materials->LoadMatrix( m_PanelToWorld );

	unsigned char pColor[4] = {255, 255, 255, 255};

	CMeshBuilder meshBuilder;
	IMesh *pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_OverlayMaterial );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( 0.0f, 0.0f, 0 );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_flWidth, 0.0f, 0 );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_flWidth, -m_flHeight, 0 );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 0.0f, -m_flHeight, 0 );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	materials->PopMatrix();
}


//-----------------------------------------------------------------------------
// Draws the panel using a 3D transform...
//-----------------------------------------------------------------------------
int	C_VGuiScreen::DrawModel( int flags )
{
	vgui::Panel *pPanel = m_PanelWrapper.GetPanel();
	if (!pPanel || !IsActive())
		return 0;
	
	// Don't bother drawing stuff not visible to me...
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pLocalPlayer || !IsVisibleToTeam(pLocalPlayer->GetTeamNumber()) )
		return 0;
	
	// Backface cull the entire panel here...
	if (IsBackfacing(CurrentViewOrigin()))
		return 0;

	// Recompute the panel-to-world center
	// FIXME: Can this be cached off?
	ComputePanelToWorld();

	g_pMatSystemSurface->DrawPanelIn3DSpace( pPanel->GetVPanel(), m_PanelToWorld, 
		m_nPixelWidth, m_nPixelHeight, m_flWidth, m_flHeight );

	// Finally, a pass to set the z buffer...
	DrawScreenOverlay();

	return 1;
}



//-----------------------------------------------------------------------------
//
// Enumator class for finding vgui screens close to the local player
//
//-----------------------------------------------------------------------------
class CVGuiScreenEnumerator : public IPartitionEnumerator
{
	DECLARE_CLASS_GAMEROOT( CVGuiScreenEnumerator, IPartitionEnumerator );
public:
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	int	GetScreenCount();
	C_VGuiScreen *GetVGuiScreen( int index );

private:
	CUtlVector< CHandle< C_VGuiScreen > > m_VguiScreens;
};

IterationRetval_t CVGuiScreenEnumerator::EnumElement( IHandleEntity *pHandleEntity )
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
	if ( pEnt == NULL )
		return ITERATION_CONTINUE;

	// FIXME.. pretty expensive...
	C_VGuiScreen *pScreen = dynamic_cast<C_VGuiScreen*>(pEnt); 
	if ( pScreen )
	{
		int i = m_VguiScreens.AddToTail( );
		m_VguiScreens[i].Set( pScreen );
	}

	return ITERATION_CONTINUE;
}

int	CVGuiScreenEnumerator::GetScreenCount()
{
	return m_VguiScreens.Count();
}

C_VGuiScreen *CVGuiScreenEnumerator::GetVGuiScreen( int index )
{
	return m_VguiScreens[index].Get();
}							


//-----------------------------------------------------------------------------
//
// Look for vgui screens, returns true if it found one ...
//
//-----------------------------------------------------------------------------
C_BaseEntity *FindNearbyVguiScreen( const Vector &viewPosition, const QAngle &viewAngle, int nTeam )
{
	// Get the view direction...
	Vector lookDir;
	AngleVectors( viewAngle, &lookDir );

	// Create a ray used for raytracing 
	Vector lookEnd;
	VectorMA( viewPosition, 2.0f * VGUI_SCREEN_MODE_RADIUS, lookDir, lookEnd );

	Ray_t lookRay;
	lookRay.Init( viewPosition, lookEnd );

	// Look for vgui screens that are close to the player
	CVGuiScreenEnumerator localScreens;
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_NON_STATIC_EDICTS, viewPosition, VGUI_SCREEN_MODE_RADIUS, false, &localScreens );

	Vector vecOut, vecViewDelta;

	float flBestDist = 2.0f;
	C_VGuiScreen *pBestScreen = NULL;
	for (int i = localScreens.GetScreenCount(); --i >= 0; )
	{
		C_VGuiScreen *pScreen = localScreens.GetVGuiScreen(i);

		// Don't bother with screens I'm behind...
		if (pScreen->IsBackfacing(viewPosition))
			continue;

		// Don't bother with screens that are turned off
		if (!pScreen->IsActive())
			continue;

		// FIXME: Should this maybe go into a derived class of some sort?
		// Don't bother with screens on the wrong team
		if (!pScreen->IsVisibleToTeam(nTeam))
			continue;

		if ( !pScreen->AcceptsInput() )
			continue;

		// Test perpendicular distance from the screen...
		pScreen->GetVectors( NULL, NULL, &vecOut );
		VectorSubtract( viewPosition, pScreen->GetAbsOrigin(), vecViewDelta );
		float flPerpDist = DotProduct(vecViewDelta, vecOut);
		if ( (flPerpDist < 0) || (flPerpDist > VGUI_SCREEN_MODE_RADIUS) )
			continue;

		// Perform a raycast to see where in barycentric coordinates the ray hits
		// the viewscreen; if it doesn't hit it, you're not in the mode
		float u, v, t;
		if (!pScreen->IntersectWithRay( lookRay, &u, &v, &t ))
			continue;

		// Barycentric test
		if ((u < 0) || (v < 0) || (u > 1) || (v > 1))
			continue;

		if ( t < flBestDist )
		{
			flBestDist = t;
			pBestScreen = pScreen;
		}
	}

	return pBestScreen;
}

void ActivateVguiScreen( C_BaseEntity *pVguiScreenEnt )
{
	if (pVguiScreenEnt)
	{
		Assert( dynamic_cast<C_VGuiScreen*>(pVguiScreenEnt) );
		C_VGuiScreen *pVguiScreen = static_cast<C_VGuiScreen*>(pVguiScreenEnt);
		pVguiScreen->GainFocus( );
	}
}

void SetVGuiScreenButtonState( C_BaseEntity *pVguiScreenEnt, int nButtonState )
{
	if (pVguiScreenEnt)
	{
		Assert( dynamic_cast<C_VGuiScreen*>(pVguiScreenEnt) );
		C_VGuiScreen *pVguiScreen = static_cast<C_VGuiScreen*>(pVguiScreenEnt);
		pVguiScreen->SetButtonState( nButtonState );
	}
}

void DeactivateVguiScreen( C_BaseEntity *pVguiScreenEnt )
{
	if (pVguiScreenEnt)
	{
		Assert( dynamic_cast<C_VGuiScreen*>(pVguiScreenEnt) );
		C_VGuiScreen *pVguiScreen = static_cast<C_VGuiScreen*>(pVguiScreenEnt);
		pVguiScreen->LoseFocus( );
	}
}

CVGuiScreenPanel::CVGuiScreenPanel( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
{
	m_hEntity = NULL;
}

CVGuiScreenPanel::CVGuiScreenPanel( vgui::Panel *parent, const char *panelName, vgui::HScheme hScheme )
	: BaseClass( parent, panelName, hScheme )
{
	m_hEntity = NULL;
}


bool CVGuiScreenPanel::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	const char *pResFile = pKeyValues->GetString( "resfile" );
	if (pResFile[0] != 0)
	{
		KeyValues *pCachedKeyValues = CacheKeyValuesForFile( pResFile );
		LoadControlSettings( pResFile, NULL, pCachedKeyValues );
	}

	// Dimensions in pixels
	int nWidth, nHeight;
	nWidth = pKeyValues->GetInt( "pixelswide", 240 );
	nHeight = pKeyValues->GetInt( "pixelshigh", 160 );
	if ((nWidth <= 0) || (nHeight <= 0))
		return false;

	// If init data isn't specified, then we're just precaching.
	if ( pInitData )
	{
		m_hEntity.Set( pInitData->m_pEntity );

		C_VGuiScreen *screen = dynamic_cast< C_VGuiScreen * >( pInitData->m_pEntity );
		if ( screen )
		{
			bool acceptsInput = pKeyValues->GetInt( "acceptsinput", 1 ) ? true : false;
			screen->SetAcceptsInput( acceptsInput );
		}
	}

	SetBounds( 0, 0, nWidth, nHeight );

	return true;
}

vgui::Panel *CVGuiScreenPanel::CreateControlByName(const char *controlName)
{
	// Check the panel metaclass manager to make these controls...
	if (!Q_strncmp(controlName, "MaterialImage", 20))
	{
		return new CBitmapPanel(NULL, "BitmapPanel");
	}

	if (!Q_strncmp(controlName, "MaterialButton", 20))
	{
		return new CBitmapButton(NULL, "BitmapButton", "");
	}

	// Didn't find it? Just use the default stuff
	return BaseClass::CreateControlByName( controlName );
}

DECLARE_VGUI_SCREEN_FACTORY( CVGuiScreenPanel, "vgui_screen_panel" );