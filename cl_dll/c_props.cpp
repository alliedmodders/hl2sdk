//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "c_breakableprop.h"
#include "c_physicsprop.h"
#include "c_physbox.h"
#include "props_shared.h"

#define CPhysBox C_PhysBox
#define CPhysicsProp C_PhysicsProp


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_DynamicProp : public C_BreakableProp
{
	DECLARE_CLASS( C_DynamicProp, C_BreakableProp );
public:
	DECLARE_CLIENTCLASS();

	// constructor, destructor
	C_DynamicProp( void );
	~C_DynamicProp( void );

	void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	unsigned int ComputeClientSideAnimationFlags();

private:
	C_DynamicProp( const C_DynamicProp & );

	bool	m_bUseHitboxesForRenderBox;
	int		m_iCachedFrameCount;
	Vector	m_vecCachedRenderMins;
	Vector	m_vecCachedRenderMaxs;
};

IMPLEMENT_CLIENTCLASS_DT(C_DynamicProp, DT_DynamicProp, CDynamicProp)
	RecvPropBool(RECVINFO(m_bUseHitboxesForRenderBox)),
END_RECV_TABLE()

C_DynamicProp::C_DynamicProp( void )
{
	m_iCachedFrameCount = -1;
}

C_DynamicProp::~C_DynamicProp( void )
{
}

//-----------------------------------------------------------------------------
// implements these so ragdolls can handle frustum culling & leaf visibility
//-----------------------------------------------------------------------------
void C_DynamicProp::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	if ( m_bUseHitboxesForRenderBox )
	{
		if ( GetModel() )
		{
			studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel( GetModel() );
			if ( !pStudioHdr || GetSequence() == -1 )
			{
				theMins = vec3_origin;
				theMaxs = vec3_origin;
				return;
			}

			// Only recompute if it's a new frame
			if ( gpGlobals->framecount != m_iCachedFrameCount )
			{
				ComputeEntitySpaceHitboxSurroundingBox( &m_vecCachedRenderMins, &m_vecCachedRenderMaxs );
				m_iCachedFrameCount = gpGlobals->framecount;
			}

			theMins = m_vecCachedRenderMins;
			theMaxs = m_vecCachedRenderMaxs;
			return;
		}
	}

	BaseClass::GetRenderBounds( theMins, theMaxs );
}

unsigned int C_DynamicProp::ComputeClientSideAnimationFlags()
{
	if ( GetSequence() != -1 )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		if ( GetSequenceCycleRate(pStudioHdr, GetSequence()) != 0.0f )
		{
			return BaseClass::ComputeClientSideAnimationFlags();
		}
	}

	// no sequence or no cycle rate, don't do any per-frame calcs
	return 0;
}

// ------------------------------------------------------------------------------------------ //
// ------------------------------------------------------------------------------------------ //
class C_BasePropDoor : public C_DynamicProp
{
	DECLARE_CLASS( C_BasePropDoor, C_DynamicProp );
public:
	DECLARE_CLIENTCLASS();

	// constructor, destructor
	C_BasePropDoor( void );
	virtual ~C_BasePropDoor( void );

	virtual void OnDataChanged( DataUpdateType_t type );

	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

private:
	C_BasePropDoor( const C_BasePropDoor & );
};

IMPLEMENT_CLIENTCLASS_DT(C_BasePropDoor, DT_BasePropDoor, CBasePropDoor)
END_RECV_TABLE()

C_BasePropDoor::C_BasePropDoor( void )
{
}

C_BasePropDoor::~C_BasePropDoor( void )
{
}

void C_BasePropDoor::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetSolid(SOLID_VPHYSICS);
		VPhysicsInitShadow( false, false );
	}
	else if ( VPhysicsGetObject() )
	{
		VPhysicsGetObject()->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, TICK_INTERVAL );
	}
}

bool C_BasePropDoor::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !VPhysicsGetObject() )
		return false;

	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return false;

	physcollision->TraceBox( ray, VPhysicsGetObject()->GetCollide(), GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.DidHit() )
	{
		trace.surface.surfaceProps = VPhysicsGetObject()->GetMaterialIndex();
		return true;
	}

	return false;
}

// ------------------------------------------------------------------------------------------ //
// Special version of func_physbox.
// ------------------------------------------------------------------------------------------ //
#ifndef _XBOX
class CPhysBoxMultiplayer : public CPhysBox, public IMultiplayerPhysics
{
public:
	DECLARE_CLASS( CPhysBoxMultiplayer, CPhysBox );

	virtual int GetMultiplayerPhysicsMode()
	{
		return m_iPhysicsMode;
	}

	virtual float GetMass()
	{
		return m_fMass;
	}

	virtual bool IsAsleep()
	{
		Assert ( 0 );
		return true;
	}

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float, m_fMass );

	DECLARE_CLIENTCLASS();
};

IMPLEMENT_CLIENTCLASS_DT( CPhysBoxMultiplayer, DT_PhysBoxMultiplayer, CPhysBoxMultiplayer )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
END_RECV_TABLE()


class CPhysicsPropMultiplayer : public CPhysicsProp, public IMultiplayerPhysics
{
	DECLARE_CLASS( CPhysicsPropMultiplayer, CPhysicsProp );

	virtual int GetMultiplayerPhysicsMode()
	{
		Assert( m_iPhysicsMode != PHYSICS_MULTIPLAYER_CLIENTSIDE );
		Assert( m_iPhysicsMode != PHYSICS_MULTIPLAYER_AUTODETECT );
		return m_iPhysicsMode;
	}

	virtual float GetMass()
	{
		return m_fMass;
	}

	virtual bool IsAsleep()
	{
		return !m_bAwake;
	}

	virtual void ComputeWorldSpaceSurroundingBox( Vector *mins, Vector *maxs )
	{
		Assert( mins != NULL && maxs != NULL );
		if ( !mins || !maxs )
			return;

		// Take our saved collision bounds, and transform into world space
		TransformAABB( EntityToWorldTransform(), m_collisionMins, m_collisionMaxs, *mins, *maxs );
	}

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float, m_fMass );
	CNetworkVector( m_collisionMins );
	CNetworkVector( m_collisionMaxs );

	DECLARE_CLIENTCLASS();
};

IMPLEMENT_CLIENTCLASS_DT( CPhysicsPropMultiplayer, DT_PhysicsPropMultiplayer, CPhysicsPropMultiplayer )
	RecvPropInt( RECVINFO( m_iPhysicsMode ) ),
	RecvPropFloat( RECVINFO( m_fMass ) ),
	RecvPropVector( RECVINFO( m_collisionMins ) ),
	RecvPropVector( RECVINFO( m_collisionMaxs ) ),
END_RECV_TABLE()
#endif
