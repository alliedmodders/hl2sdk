//
// Episodic screen-space effects
//

#include "cbase.h"
#include "screenspaceeffects.h"
#include "rendertexture.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "cdll_client_int.h"
#include "materialsystem/itexture.h"
#include "keyvalues.h"
#include "ClientEffectPrecacheSystem.h"

#include "episodic_screenspaceeffects.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectEpScreenspace )
CLIENTEFFECT_MATERIAL( "effects/stun" )
CLIENTEFFECT_MATERIAL( "effects/introblur" )
CLIENTEFFECT_REGISTER_END()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CStunEffect::Init( void ) 
{
	m_pStunTexture = NULL;
	m_flDuration = 0.0f;
	m_flFinishTime = 0.0f;
	m_bUpdated = false;
}

//------------------------------------------------------------------------------
// Purpose: Pick up changes in our parameters
//------------------------------------------------------------------------------
void CStunEffect::SetParameters( KeyValues *params )
{
	if( params->FindKey( "duration" ) )
	{
		m_flDuration = params->GetFloat( "duration" );
		m_flFinishTime = gpGlobals->curtime + m_flDuration;
		m_bUpdated = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CStunEffect::Render( int x, int y, int w, int h )
{
	// Make sure we're ready to play this effect
	if ( m_flFinishTime < gpGlobals->curtime )
		return;

	IMaterial *pMaterial = materials->FindMaterial( "effects/stun", TEXTURE_GROUP_CLIENT_EFFECTS, true );
	if ( pMaterial == NULL )
		return;

	bool bResetBaseFrame = m_bUpdated;

	// Set ourselves to the proper rendermode
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();
	materials->LoadIdentity();
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	materials->LoadIdentity();

	// Get our current view
	if ( m_pStunTexture == NULL )
	{
		m_pStunTexture = GetPowerOfTwoFrameBufferTexture();
	}

	// Draw the texture if we're using it
	if ( m_pStunTexture != NULL )
	{
		bool foundVar;
		IMaterialVar* pBaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );

		if ( bResetBaseFrame )
		{
			// Save off this pass
			Rect_t srcRect;
			srcRect.x = x;
			srcRect.y = y;
			srcRect.width = w;
			srcRect.height = h;
			pBaseTextureVar->SetTextureValue( m_pStunTexture );

			materials->CopyRenderTargetToTextureEx( m_pStunTexture, 0, &srcRect, NULL );
			materials->SetFrameBufferCopyTexture( m_pStunTexture );
			m_bUpdated = false;
		}

		byte overlaycolor[4] = { 255, 255, 255, 0 };

		float flEffectPerc = ( m_flFinishTime - gpGlobals->curtime ) / m_flDuration;
		overlaycolor[3] = (byte) (150.0f * flEffectPerc);

		render->ViewDrawFade( overlaycolor, pMaterial );

		float viewOffs = ( flEffectPerc * 32.0f ) * cos( gpGlobals->curtime * 10.0f * cos( gpGlobals->curtime * 2.0f ) );
		float vX = x + viewOffs;
		float vY = y;

		// just do one pass for dxlevel < 80.
		if (g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 80)
		{
			materials->DrawScreenSpaceRectangle( pMaterial, vX, vY, w, h,
				0, 0, m_pStunTexture->GetActualWidth()-1, m_pStunTexture->GetActualHeight()-1, 
				m_pStunTexture->GetActualWidth(), m_pStunTexture->GetActualHeight() );

			render->ViewDrawFade( overlaycolor, pMaterial );

			materials->DrawScreenSpaceRectangle( pMaterial, x, y, w, h,
				0, 0, m_pStunTexture->GetActualWidth()-1, m_pStunTexture->GetActualHeight()-1, 
				m_pStunTexture->GetActualWidth(), m_pStunTexture->GetActualHeight() );
		}

		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pBaseTextureVar->SetTextureValue( m_pStunTexture );

		materials->CopyRenderTargetToTextureEx( m_pStunTexture, 0, &srcRect, NULL );
	}

	// Restore our state
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
}

// ================================================================================================================
//
//  Ep 1. Intro blur
//
// ================================================================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEP1IntroEffect::Init( void ) 
{
	m_pStunTexture = NULL;
	m_flDuration = 0.0f;
	m_flFinishTime = 0.0f;
	m_bUpdateView = true;
	m_bFadeOut = false;
}

//------------------------------------------------------------------------------
// Purpose: Pick up changes in our parameters
//------------------------------------------------------------------------------
void CEP1IntroEffect::SetParameters( KeyValues *params )
{
	if( params->FindKey( "duration" ) )
	{
		m_flDuration = params->GetFloat( "duration" );
		m_flFinishTime = gpGlobals->curtime + m_flDuration;
	}

	if( params->FindKey( "fadeout" ) )
	{
		m_bFadeOut = ( params->GetInt( "fadeout" ) == 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the alpha value depending on various factors and time
//-----------------------------------------------------------------------------
inline unsigned char CEP1IntroEffect::GetFadeAlpha( void )
{
	// Find our percentage between fully "on" and "off" in the pulse range
	float flEffectPerc = ( m_flDuration == 0.0f ) ? 0.0f : ( m_flFinishTime - gpGlobals->curtime ) / m_flDuration;
	flEffectPerc = clamp( flEffectPerc, 0.0f, 1.0f );

	if  ( m_bFadeOut )
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 50.0f * flEffectPerc, 0.0f, 50.0f );

		// Non-HDR
		return (unsigned char) clamp( 64.0f * flEffectPerc, 0.0f, 64.0f );
	}
	else
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 64.0f * flEffectPerc, 50.0f, 64.0f );

		// Non-HDR
		return (unsigned char) clamp( 128.0f * flEffectPerc, 64.0f, 128.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CEP1IntroEffect::Render( int x, int y, int w, int h )
{
	if ( ( m_flFinishTime == 0 ) || ( IsEnabled() == false ) )
		return;

	IMaterial *pMaterial = materials->FindMaterial( "effects/introblur", TEXTURE_GROUP_CLIENT_EFFECTS, true );
	if ( pMaterial == NULL )
		return;

	// Set ourselves to the proper rendermode
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PushMatrix();
	materials->LoadIdentity();
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PushMatrix();
	materials->LoadIdentity();

	// Get our current view
	if ( m_pStunTexture == NULL )
	{
		m_pStunTexture = GetWaterRefractionTexture();
	}

	// Draw the texture if we're using it
	if ( m_pStunTexture != NULL )
	{
		bool foundVar;
		IMaterialVar* pBaseTextureVar = pMaterial->FindVar( "$basetexture", &foundVar, false );

		if ( m_bUpdateView )
		{
			// Save off this pass
			Rect_t srcRect;
			srcRect.x = x;
			srcRect.y = y;
			srcRect.width = w;
			srcRect.height = h;
			pBaseTextureVar->SetTextureValue( m_pStunTexture );

			materials->CopyRenderTargetToTextureEx( m_pStunTexture, 0, &srcRect, NULL );
			materials->SetFrameBufferCopyTexture( m_pStunTexture );
			m_bUpdateView = false;
		}

		byte overlaycolor[4] = { 255, 255, 255, 0 };
		
		// Get our fade value depending on our fade duration
		overlaycolor[3] = GetFadeAlpha();

		// Disable overself if we're done fading out
		if ( m_bFadeOut && overlaycolor[3] == 0 )
		{
			// Takes effect next frame (we don't want to hose our matrix stacks here)
			g_pScreenSpaceEffects->DisableScreenSpaceEffect( "episodic_intro" );
			m_bUpdateView = true;
		}

		// Calculate some wavey noise to jitter the view by
		float vX = 2.0f * -fabs( cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 6.0 ) );
		float vY = 2.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 5.0 );

		// Scale percentage
		float flScalePerc = 0.02f + ( 0.01f * cosf( gpGlobals->curtime * 2.0f ) * cosf( gpGlobals->curtime * 0.5f ) );
		
		// Scaled offsets for the UVs (as texels)
		float flUOffset = ( m_pStunTexture->GetActualWidth() - 1 ) * flScalePerc * 0.5f;
		float flVOffset = ( m_pStunTexture->GetActualHeight() - 1 ) * flScalePerc * 0.5f;
		
		// New UVs with scaling offsets
		float flU1 = flUOffset;
		float flU2 = ( m_pStunTexture->GetActualWidth() - 1 ) - flUOffset;
		float flV1 = flVOffset;
		float flV2 = ( m_pStunTexture->GetActualHeight() - 1 ) - flVOffset;

		// Draw the "zoomed" overlay
		materials->DrawScreenSpaceRectangle( pMaterial, vX, vY, w, h,
			flU1, flV1, 
			flU2, flV2, 
			m_pStunTexture->GetActualWidth(), m_pStunTexture->GetActualHeight() );

		render->ViewDrawFade( overlaycolor, pMaterial );

		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pBaseTextureVar->SetTextureValue( m_pStunTexture );

		materials->CopyRenderTargetToTextureEx( m_pStunTexture, 0, &srcRect, NULL );
	}

	// Restore our state
	materials->MatrixMode( MATERIAL_VIEW );
	materials->PopMatrix();
	materials->MatrixMode( MATERIAL_PROJECTION );
	materials->PopMatrix();
}
