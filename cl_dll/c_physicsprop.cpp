//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "model_types.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "solidsetdefaults.h"
#include "bone_setup.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "view.h"
#include "clienteffectprecachesystem.h"
#include "c_physicsprop.h"
#include "tier0/vprof.h"
#include "ivrenderview.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_PhysicsProp, DT_PhysicsProp, CPhysicsProp)
	RecvPropBool( RECVINFO( m_bAwake ) ),
END_RECV_TABLE()

ConVar r_PhysPropStaticLighting( "r_PhysPropStaticLighting", "1" );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysicsProp::C_PhysicsProp( void )
{
	m_pPhysicsObject = NULL;
	m_takedamage = DAMAGE_YES;

	// default true so static lighting will get recomputed when we go to sleep
	m_bAwakeLastTime = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PhysicsProp::~C_PhysicsProp( void )
{
}


ConVar r_visualizeproplightcaching( "r_visualizeproplightcaching", "0" );

//-----------------------------------------------------------------------------
// Purpose: Draws the object
// Input  : flags - 
//-----------------------------------------------------------------------------
// FIXME!!!!  I hate the fact that InternalDrawModel is always such a huge cut-and-paste job.
int C_PhysicsProp::InternalDrawModel( int flags )
{
	VPROF( "C_PhysicsProp::InternalDrawModel" );

	//-----------------------------------------------------------------------------
	// Overriding C_BaseAnimating::InternalDrawModel so that we can detect when the
	// prop is asleep.  This allows us to bake the lighting for the model.
	//-----------------------------------------------------------------------------
	if ( !GetModel() )
	{
		return 0;
	}

	if ( IsEffectActive( EF_ITEM_BLINK ) )
	{
		flags |= STUDIO_ITEM_BLINK;
	}

	// This should never happen, but if the server class hierarchy has bmodel entities derived from CBaseAnimating or does a
	// SetModel with the wrong type of model, this could occur.
	if ( modelinfo->GetModelType( GetModel() ) != mod_studio )
	{
		return BaseClass::DrawModel( flags );
	}

	// Make sure hdr is valid for drawing
	if ( !GetModelPtr() )
	{
		// inhibit drawing and state setting until all data available
		return 0;
	}

	CreateModelInstance();

	if ( r_PhysPropStaticLighting.GetBool() && m_bAwakeLastTime != m_bAwake )
	{
		if ( m_bAwakeLastTime && !m_bAwake )
		{
			// transition to sleep, bake lighting now, once
			if ( !modelrender->RecomputeStaticLighting( GetModelInstance() ) )
			{
				// not valid for drawing
				return 0;
			}

			if ( r_visualizeproplightcaching.GetBool() )
			{
				float color[] = { 0.0f, 1.0f, 0.0f, 1.0f };
				render->SetColorModulation( color );
			}
		}
		else if ( r_visualizeproplightcaching.GetBool() )
		{
			float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
			render->SetColorModulation( color );
		}
	}

	if ( !m_bAwake && r_PhysPropStaticLighting.GetBool() )
	{
		// going to sleep, have static lighting
		flags |= STUDIO_STATIC_LIGHTING;
	}
	
	Vector tmpOrigin = GetRenderOrigin();
	
	int drawn = modelrender->DrawModel( 
		flags, 
		this,
		GetModelInstance(),
		index, 
		GetModel(),
		GetRenderOrigin(),
		GetRenderAngles(),
		m_nSkin,
		m_nBody,
		m_nHitboxSet );

	if ( vcollide_wireframe.GetBool() )
	{
		if ( IsRagdoll() )
		{
			m_pRagdoll->DrawWireframe();
		}
		else
		{
			vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
			if ( pCollide && pCollide->solidCount == 1 )
			{
				static color32 debugColor = {0,255,255,0};
				matrix3x4_t matrix;
				AngleMatrix( GetAbsAngles(), GetAbsOrigin(), matrix );
				engine->DebugDrawPhysCollide( pCollide->solids[0], NULL, matrix, debugColor );
			}
		}
	}

	// track state
	m_bAwakeLastTime = m_bAwake;
	
	return drawn;
}
