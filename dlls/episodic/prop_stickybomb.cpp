//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Temp entity for testing physcannon ammo
//
//=============================================================================

#include "cbase.h"
#include "props.h"
#include "vphysics/constraints.h"
#include "physics_saverestore.h"

class CPropStickyBomb : public CPhysicsProp
{
	DECLARE_CLASS( CPropStickyBomb, CPhysicsProp );
	DECLARE_DATADESC();

public:
					CPropStickyBomb( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	UpdateOnRemove( void );

private:
	bool	StickToWorld( int index, gamevcollisionevent_t *pEvent );
	bool	StickToEntity( int index, gamevcollisionevent_t *pEvent );
	bool	CreateConstraintToObject( CBaseEntity *pObject );
	bool	CreateConstraintToNPC( CAI_BaseNPC *pNPC );
	void	OnConstraintBroken( void );

	IPhysicsConstraint			*m_pConstraint;
	EHANDLE						m_hConstrainedEntity;
};

LINK_ENTITY_TO_CLASS( prop_stickybomb, CPropStickyBomb );

BEGIN_DATADESC( CPropStickyBomb )
	DEFINE_FIELD( m_hConstrainedEntity, FIELD_EHANDLE ),
	DEFINE_PHYSPTR( m_pConstraint ),
END_DATADESC()

CPropStickyBomb::CPropStickyBomb( void ) : m_pConstraint(NULL)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropStickyBomb::Precache( void )
{
	m_iszBreakableModel = AllocPooledString( "models/props_outland/pumpkin_explosive.mdl" );
	PrecacheModel( STRING( m_iszBreakableModel ) );
	PrecacheScriptSound( "NPC_AntlionGrub.Squash" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropStickyBomb::Spawn( void )
{	
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Stick to the world
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropStickyBomb::StickToWorld( int index, gamevcollisionevent_t *pEvent )
{
	Vector vecDir = pEvent->preVelocity[ index ];
	float speed = VectorNormalize( vecDir );

	// Make sure the object is travelling fast enough to stick.
	if( speed > 1000.0f )
	{
		Vector vecPos;
		QAngle angles;
		VPhysicsGetObject()->GetPosition( &vecPos, &angles );

		Vector vecVelocity = pEvent->preVelocity[0];
		VectorNormalize(vecVelocity);

		trace_t tr;
		UTIL_TraceLine( vecPos, vecPos + (vecVelocity * 64), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// Finally, inhibit sticking in metal, grates, sky, or anything else that doesn't make a sound.
		surfacedata_t *psurf= physprops->GetSurfaceData( pEvent->surfaceProps[!index] );

		if ( psurf->game.material != 'X' )
		{
			EmitSound( "NPC_AntlionGrub.Squash" );

			Vector savePosition = vecPos;

			Vector vecEmbed = pEvent->preVelocity[ index ];
			VectorNormalize( vecEmbed );
			vecEmbed *= 8;

			vecPos += vecEmbed;

			Teleport( &vecPos, NULL, NULL );
			SetEnableMotionPosition( savePosition, angles );  // this uses hierarchy, so it must be set after teleport

			VPhysicsGetObject()->EnableMotion( false );
			AddSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON );
			SetCollisionGroup( COLLISION_GROUP_DEBRIS );

			UTIL_DecalTrace( &tr, "PaintSplatGreen" );
			
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Create a constraint between this object and another
// Input  : *pObject - Object to constrain ourselves to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropStickyBomb::CreateConstraintToObject( CBaseEntity *pObject )
{
	if ( m_pConstraint != NULL )
	{
		// Should we destroy the constraint and make a new one at this point?
		Assert( 0 );
		return false;
	}

	if ( pObject == NULL )
		return false;

	IPhysicsObject *pPhysObject = pObject->VPhysicsGetObject();
	if ( pPhysObject == NULL )
		return false;

	IPhysicsObject *pMyPhysObject = VPhysicsGetObject();
	if ( pPhysObject == NULL )
		return false;

	// Create the fixed constraint
	constraint_fixedparams_t fixedConstraint;
	fixedConstraint.Defaults();
	fixedConstraint.InitWithCurrentObjectState( pPhysObject, pMyPhysObject );

	IPhysicsConstraint *pConstraint = physenv->CreateFixedConstraint( pPhysObject, pMyPhysObject, NULL, fixedConstraint );
	if ( pConstraint == NULL )
		return false;

	// Hold on to us
	m_pConstraint = pConstraint;
	pConstraint->SetGameData( (void *)this );
	m_hConstrainedEntity = pObject;

	// Disable collisions between the two ents
	PhysDisableObjectCollisions( pPhysObject, pMyPhysObject );

	EmitSound( "NPC_AntlionGrub.Squash" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropStickyBomb::UpdateOnRemove( void )
{
	OnConstraintBroken();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropStickyBomb::OnConstraintBroken( void )
{
	// Destroy the constraint
	if ( m_pConstraint != NULL )
	{ 
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}

	if ( m_hConstrainedEntity != NULL )
	{
		// Re-enable the collisions between the objects
		IPhysicsObject *pPhysObject = m_hConstrainedEntity->VPhysicsGetObject();
		IPhysicsObject *pMyPhysObject = VPhysicsGetObject();
		PhysEnableEntityCollisions( pPhysObject, pMyPhysObject );
		m_hConstrainedEntity = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNPC - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropStickyBomb::CreateConstraintToNPC( CAI_BaseNPC *pNPC )
{
	// Find the nearest bone to our position
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Stick to an entity (using hierarchy if we can)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropStickyBomb::StickToEntity( int index, gamevcollisionevent_t *pEvent )
{
	// Make sure the object is travelling fast enough to stick
	float flSpeedSqr = ( pEvent->preVelocity[ index ] ).LengthSqr();
	if ( flSpeedSqr < Square( 1000.0f ) )
		return false;

	CBaseEntity *pOther = pEvent->pEntities[!index];

	// Handle NPCs
	CAI_BaseNPC *pNPC = pOther->MyNPCPointer();
	if ( pNPC != NULL )
	{
		// Attach this object to the nearest bone
		// TODO: How does this affect the NPC's behavior?
		return CreateConstraintToNPC( pNPC );
	}

	// Handle physics props
	CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pOther);
	if ( pProp != NULL )
	{
		// Create a constraint to this object
		return CreateConstraintToObject( pProp );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Handle a collision using our special behavior
//-----------------------------------------------------------------------------
void CPropStickyBomb::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Find out what we hit
	CBaseEntity *pVictim = pEvent->pEntities[!index];
	if ( pVictim == NULL || m_pConstraint != NULL )
	{
		BaseClass::VPhysicsCollision( index, pEvent );
		return;
	}

	// Attempt to stick to the world
	if ( pVictim->IsWorld() )
	{
		// Done if we succeeded
		if ( StickToWorld( index, pEvent ) )
			return;

		// Just bounce
		BaseClass::VPhysicsCollision( index, pEvent );
		return;
	}

	// Attempt to stick to an entity
	if ( StickToEntity( index, pEvent ) )
		return;

	// Just bounce
	BaseClass::VPhysicsCollision( index, pEvent );
}
