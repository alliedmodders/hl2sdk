//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "FX_Fleck.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// CFleckParticles
//


CSmartPtr<CFleckParticles> CFleckParticles::Create( const char *pDebugName, const Vector &vCenter )
{
	CFleckParticles *pRet = new CFleckParticles( pDebugName );
	if ( pRet )
	{
		// Set its bbox once so it doesn't update 500 times as the flecks fly outwards.
		Vector vDims( 5, 5, 5 );
		pRet->GetBinding().SetBBox( vCenter - vDims, vCenter + vDims );
	}
	return pRet;
}


//-----------------------------------------------------------------------------
// Purpose: Test for surrounding collision surfaces for quick collision testing for the particle system
// Input  : &origin - starting position
//			*dir - direction of movement (if NULL, will do a point emission test in four directions)
//			angularSpread - looseness of the spread
//			minSpeed - minimum speed
//			maxSpeed - maximum speed
//			gravity - particle gravity for the sytem
//			dampen - dampening amount on collisions
//			flags - extra information
//-----------------------------------------------------------------------------
void CFleckParticles::Setup( const Vector &origin, const Vector *direction, float angularSpread, float minSpeed, float maxSpeed, float gravity, float dampen, int flags )
{
	//See if we've specified a direction
	m_ParticleCollision.Setup( origin, direction, angularSpread, minSpeed, maxSpeed, gravity, dampen );
}


void CFleckParticles::RenderParticles( CParticleRenderIterator *pIterator )
{
	const FleckParticle *pParticle = (const FleckParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		Vector	tPos;
		TransformParticle( ParticleMgr()->GetModelView(), pParticle->m_Pos, tPos );
		float sortKey = (int) tPos.z;
		
		Vector	color;
		color[0] = pParticle->m_uchColor[0] / 255.0f;
		color[1] = pParticle->m_uchColor[1] / 255.0f;
		color[2] = pParticle->m_uchColor[2] / 255.0f;
		//Render it
		RenderParticle_ColorSizeAngle(
			pIterator->GetParticleDraw(),
			tPos,
			color,
			1.0f - (pParticle->m_flLifetime / pParticle->m_flDieTime),
			pParticle->m_uchSize,
			pParticle->m_flRoll );

		pParticle = (const FleckParticle*)pIterator->GetNext( sortKey );
	}
}


void CFleckParticles::SimulateParticles( CParticleSimulateIterator *pIterator )
{
	FleckParticle *pParticle = (FleckParticle*)pIterator->GetFirst();
	while ( pParticle )
	{
		const float	timeDelta = pIterator->GetTimeDelta();

		//Should this particle die?
		pParticle->m_flLifetime += timeDelta;

		if ( pParticle->m_flLifetime >= pParticle->m_flDieTime )
		{
			pIterator->RemoveParticle( pParticle );
		}
		else
		{
			pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;

			//Simulate the movement with collision
			trace_t trace;
			m_ParticleCollision.MoveParticle( pParticle->m_Pos, pParticle->m_vecVelocity, &pParticle->m_flRollDelta, timeDelta, &trace );

			// If we're in solid, then stop moving
			if ( trace.allsolid )
			{
				pParticle->m_vecVelocity = vec3_origin;
				pParticle->m_flRollDelta = 0.0f;
			}
		}

		pParticle = (FleckParticle*)pIterator->GetNext();
	}
}


