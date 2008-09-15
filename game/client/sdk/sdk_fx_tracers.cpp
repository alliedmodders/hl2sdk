//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "tier0/vprof.h"
#include "fx_line.h"
#include "fx_quad.h"
#include "view.h"
#include "particles_localspace.h"
#include "dlight.h"
#include "iefx.h"
#include "ClientEffectPrecacheSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern Vector GetTracerOrigin( const CEffectData &data );
extern void FX_TracerSound( const Vector &start, const Vector &end, int iTracerType );

extern ConVar muzzleflash_light;

//-----------------------------------------------------------------------------
// Purpose: Gauss Gun's Tracer
//-----------------------------------------------------------------------------
void GaussTracerCallback( const CEffectData &data )
{
	float flVelocity = data.m_flScale;
	bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	FX_GaussTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin, flVelocity, bWhiz );
}

DECLARE_CLIENT_EFFECT( "GaussTracer", GaussTracerCallback );