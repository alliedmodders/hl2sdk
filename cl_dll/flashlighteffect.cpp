//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "flashlighteffect.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"

#ifdef HL2_EPISODIC
	#include "c_basehlplayer.h"
#endif


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void r_newflashlightCallback_f( ConVar *var, char const *pOldString );

static ConVar r_newflashlight( "r_newflashlight", "1", FCVAR_CHEAT, "", r_newflashlightCallback_f );
static ConVar r_flashlightlockposition( "r_flashlightlockposition", "0", FCVAR_CHEAT );
static ConVar r_flashlightfov( "r_flashlightfov", "45.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetx( "r_flashlightoffsetx", "10.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsety( "r_flashlightoffsety", "-20.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetz( "r_flashlightoffsetz", "24.0", FCVAR_CHEAT );
static ConVar r_flashlightnear( "r_flashlightnear", "1.0", FCVAR_CHEAT );
static ConVar r_flashlightfar( "r_flashlightfar", "750.0", FCVAR_CHEAT );
static ConVar r_flashlightconstant( "r_flashlightconstant", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightlinear( "r_flashlightlinear", "100.0", FCVAR_CHEAT );
static ConVar r_flashlightquadratic( "r_flashlightquadratic", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightvisualizetrace( "r_flashlightvisualizetrace", "0", FCVAR_CHEAT );

void r_newflashlightCallback_f( ConVar *var, char const *pOldString )
{
	if( engine->GetDXSupportLevel() < 70 )
	{
		r_newflashlight.SetValue( 0 );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nEntIndex - The m_nEntIndex of the client entity that is creating us.
//			vecPos - The position of the light emitter.
//			vecDir - The direction of the light emission.
//-----------------------------------------------------------------------------
CFlashlightEffect::CFlashlightEffect(int nEntIndex)
{
	m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_nEntIndex = nEntIndex;

	m_bIsOn = false;
	m_pPointLight = NULL;
	if( engine->GetDXSupportLevel() < 70 )
	{
		r_newflashlight.SetValue( 0 );
	}	
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFlashlightEffect::~CFlashlightEffect()
{
	LightOff();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffect::TurnOn()
{
	m_bIsOn = true;
	m_flDistMod = 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffect::TurnOff()
{
	if (m_bIsOn)
	{
		m_bIsOn = false;
		LightOff();
	}
}

// Custom trace filter that skips the player and the view model.
// If we don't do this, we'll end up having the light right in front of us all
// the time.
class CTraceFilterSkipPlayerAndViewModel : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		// Test against the vehicle too?
		// FLASHLIGHTFIXME: how do you know that you are actually inside of the vehicle?
		C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if( pEntity &&
			( dynamic_cast<C_BaseViewModel *>( pEntity ) != NULL ) ||
			( dynamic_cast<C_BasePlayer *>( pEntity ) != NULL ) ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		{
			return false;
		}
		else
		{
			return true;
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CFlashlightEffect::UpdateLightNew(const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp)
{
	FlashlightState_t state;

	Vector end = vecPos + r_flashlightoffsety.GetFloat() * vecUp;

	trace_t pmEye, pmEyeBack;
	CTraceFilterSkipPlayerAndViewModel traceFilter;

	UTIL_TraceHull( vecPos, end, Vector( -4, -4, -4 ), Vector ( 4, 4, 4 ), MASK_SOLID & ~(CONTENTS_HITBOX), &traceFilter, &pmEye );

	if ( pmEye.fraction != 1.0f )
	{
		end = vecPos;
	}

	int iMask = MASK_OPAQUE_AND_NPCS;
	iMask &= ~CONTENTS_HITBOX;
	iMask |= CONTENTS_WINDOW;

	// Trace a line outward, skipping the player model and the view model.
	//Eye -> EyeForward
	UTIL_TraceHull( end, vecPos + vecForward * r_flashlightfar.GetFloat(), Vector( -4, -4, -4 ), Vector ( 4, 4, 4 ), iMask, &traceFilter, &pmEye );
	UTIL_TraceHull( end, vecPos - vecForward * 128, Vector( -4, -4, -4 ), Vector ( 4, 4, 4 ), iMask, &traceFilter, &pmEyeBack );

	float flDist;
	float flLength = (pmEye.endpos - end).Length();

	if ( flLength <= 128 )
	{
		flDist = ( ( 1.0f - ( flLength / 128 ) ) * 128.0f );
	}
	else
	{
		flDist = 0.0f;
	}


	m_flDistMod = Lerp( 0.3f, m_flDistMod, flDist );

	float flMaxDist = (pmEyeBack.endpos - end).Length();
	if( m_flDistMod > flMaxDist )
		m_flDistMod = flMaxDist;

	Vector vStartPos = end;
	Vector vEndPos = pmEye.endpos;
	Vector vDir = vEndPos - vStartPos;
	
	VectorNormalize( vDir );

	if ( vDir == vec3_origin )
	{
		vDir = vecForward;
	}

	vStartPos = vStartPos - vDir * m_flDistMod;

	if ( r_flashlightvisualizetrace.GetBool() == true )
	{
		debugoverlay->AddBoxOverlay( vEndPos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), QAngle( 0, 0, 0 ), 0, 0, 255, 16, 0 );
		debugoverlay->AddLineOverlay( vStartPos, vEndPos, 255, 0, 0, false, 0 );
	}

	state.m_vecLightOrigin = vStartPos;
	state.m_vecLightDirection = vDir;

	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();

	bool bFlicker = false;

#ifdef HL2_EPISODIC
	
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->m_HL2Local.m_flSuitPower <= 10.0f )
	{
		float flScale = SimpleSplineRemapVal( pPlayer->m_HL2Local.m_flSuitPower, 10.0f, 4.8f, 1.0f, 0.0f );
		flScale = clamp( flScale, 0.0f, 1.0f );

		if ( flScale < 0.35f )
		{
			float flFlicker = cosf( gpGlobals->curtime * 6.0f ) * sinf( gpGlobals->curtime * 15.0f );
			
			if ( flFlicker > 0.25f && flFlicker < 0.75f )
			{
				// On
				state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale;
			}
			else
			{
				// Off
				state.m_fLinearAtten = 0.0f;
			}
		}
		else
		{
			float flNoise = cosf( gpGlobals->curtime * 7.0f ) * sinf( gpGlobals->curtime * 25.0f );
			state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale + 1.5f * flNoise;
		}

		state.m_fHorizontalFOVDegrees = r_flashlightfov.GetFloat() - ( 16.0f * (1.0f-flScale) );
		state.m_fVerticalFOVDegrees = r_flashlightfov.GetFloat() - ( 16.0f * (1.0f-flScale) );
		
		bFlicker = true;
	}

#endif
	
	if ( bFlicker == false )
	{
		state.m_fLinearAtten = r_flashlightlinear.GetFloat();
		state.m_fHorizontalFOVDegrees = r_flashlightfov.GetFloat();
		state.m_fVerticalFOVDegrees = r_flashlightfov.GetFloat();
	}

	state.m_fConstantAtten = r_flashlightconstant.GetFloat();
	state.m_Color.Init( 1.0f, 1.0f, 1.0f );
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat();
	
	if( m_FlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_FlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
	}
	else
	{
		if( !r_flashlightlockposition.GetBool() )
		{
			g_pClientShadowMgr->UpdateFlashlightState( m_FlashlightHandle, state );
		}
	}
	
	g_pClientShadowMgr->UpdateProjectedTexture( m_FlashlightHandle, true );
	
	// Kill the old flashlight method if we have one.
	LightOffOld();
}

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CFlashlightEffect::UpdateLightOld(const Vector &vecPos, const Vector &vecDir, int nDistance)
{
	if ( !m_pPointLight || ( m_pPointLight->key != m_nEntIndex ))
	{
		// Set up the environment light
		m_pPointLight = effects->CL_AllocDlight(m_nEntIndex);
		m_pPointLight->flags = 0.0f;
		m_pPointLight->radius = 80;
	}
	
	// For bumped lighting
	VectorCopy(vecDir, m_pPointLight->m_Direction);
	
	Vector end;
	end = vecPos + nDistance * vecDir;
	
	// Trace a line outward, skipping the player model and the view model.
	trace_t pm;
	CTraceFilterSkipPlayerAndViewModel traceFilter;
	UTIL_TraceLine( vecPos, end, MASK_ALL, &traceFilter, &pm );
	VectorCopy( pm.endpos, m_pPointLight->origin );
	
	float falloff = pm.fraction * nDistance;
	
	if ( falloff < 500 )
		falloff = 1.0;
	else
		falloff = 500.0 / falloff;
	
	falloff *= falloff;
	
	m_pPointLight->radius = 80;
	m_pPointLight->color.r = m_pPointLight->color.g = m_pPointLight->color.b = 255 * falloff;
	m_pPointLight->color.exponent = 0;
	
	// Make it live for a bit
	m_pPointLight->die = gpGlobals->curtime + 0.2f;
	
	// Update list of surfaces we influence
	render->TouchLight( m_pPointLight );
	
	// kill the new flashlight if we have one
	LightOffNew();
}

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CFlashlightEffect::UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance)
{
	if ( !m_bIsOn )
	{
		return;
	}
	if( r_newflashlight.GetBool() )
	{
		UpdateLightNew( vecPos, vecDir, vecRight, vecUp );
	}
	else
	{
		UpdateLightOld( vecPos, vecDir, nDistance );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffect::LightOffNew()
{
	// Clear out the light
	if( m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_FlashlightHandle );
		m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffect::LightOffOld()
{	
	if ( m_pPointLight && ( m_pPointLight->key == m_nEntIndex ) )
	{
		m_pPointLight->die = gpGlobals->curtime;
		m_pPointLight = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffect::LightOff()
{	
	LightOffOld();
	LightOffNew();
}

CHeadlightEffect::CHeadlightEffect() 
{

}

CHeadlightEffect::~CHeadlightEffect()
{
	
}

void CHeadlightEffect::UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance )
{
	if ( IsOn() == false )
		 return;

	FlashlightState_t state;
	state.m_vecLightDirection = vecDir;
		
	Vector end = vecPos + vecRight + vecUp + vecDir;
	
	VectorNormalize( state.m_vecLightDirection );
	
	// Trace a line outward, skipping the player model and the view model.
	trace_t pm;
	CTraceFilterSkipPlayerAndViewModel traceFilter;
	UTIL_TraceLine( vecPos, end, MASK_ALL, &traceFilter, &pm );
	VectorCopy( pm.endpos, state.m_vecLightOrigin );
	pm.endpos -= vecDir * 4.0f;
	
	state.m_fHorizontalFOVDegrees = 90.0f;
	state.m_fVerticalFOVDegrees = 75.0f;
	state.m_fQuadraticAtten = 0.0f;
	state.m_fLinearAtten = 0.0f;
	state.m_fConstantAtten = 1.0f;
	state.m_Color.Init( 1.0f, 1.0f, 1.0f );
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat();
	
	if( GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE )
	{
		SetFlashlightHandle( g_pClientShadowMgr->CreateFlashlight( state ) );
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState( GetFlashlightHandle(), state );
	}
	
	g_pClientShadowMgr->UpdateProjectedTexture( GetFlashlightHandle(), true );
}

