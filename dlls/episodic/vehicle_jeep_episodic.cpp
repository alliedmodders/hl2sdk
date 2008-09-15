//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "vehicle_jeep_episodic.h"
#include "collisionutils.h"
#include "npc_alyx_episodic.h"

LINK_ENTITY_TO_CLASS( prop_vehicle_jeep, CPropJeepEpisodic );

BEGIN_DATADESC( CPropJeepEpisodic )

	DEFINE_FIELD( m_bEntranceLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExitLocked, FIELD_BOOLEAN ),

	DEFINE_OUTPUT( m_OnCompanionEnteredVehicle, "OnCompanionEnteredVehicle" ),
	DEFINE_OUTPUT( m_OnCompanionExitedVehicle, "OnCompanionExitedVehicle" ),
	DEFINE_OUTPUT( m_OnHostileEnteredVehicle, "OnHostileEnteredVehicle" ),
	DEFINE_OUTPUT( m_OnHostileExitedVehicle, "OnHostileExitedVehicle" ),
	
	DEFINE_INPUTFUNC( FIELD_VOID, "LockEntrance",	InputLockEntrance ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnlockEntrance",	InputUnlockEntrance ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LockExit",		InputLockExit ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnlockExit",		InputUnlockExit ),

END_DATADESC();

//=============================================================================
// Episodic jeep

CPropJeepEpisodic::CPropJeepEpisodic( void ) : 
m_bEntranceLocked( false ),
m_bExitLocked( false )
{
	m_bHasGun = false;
	m_bUnableToFire = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Spawn( void )
{
	BaseClass::Spawn();

	SetBlocksLOS( false );

	CBasePlayer	*pPlayer = UTIL_GetLocalPlayer();
	if ( pPlayer != NULL )
	{
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::NPC_FinishedEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// FIXME: This will be moved to the NPCs entering and exiting
	// Fire our outputs
	if ( bCompanion	)
	{
		m_OnCompanionEnteredVehicle.FireOutput( this, pPassenger );
	}
	else
	{
		m_OnHostileEnteredVehicle.FireOutput( this, pPassenger );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::NPC_FinishedExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// FIXME: This will be moved to the NPCs entering and exiting
	// Fire our outputs
	if ( bCompanion	)
	{
		m_OnCompanionExitedVehicle.FireOutput( this, pPassenger );
	}
	else
	{
		m_OnHostileExitedVehicle.FireOutput( this, pPassenger );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::NPC_CanEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Must be unlocked
	if ( bCompanion && m_bEntranceLocked )
		return false;

	return BaseClass::NPC_CanEnterVehicle( pPassenger, bCompanion );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::NPC_CanExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Must be unlocked
	if ( bCompanion && m_bExitLocked )
		return false;

	return BaseClass::NPC_CanExitVehicle( pPassenger, bCompanion );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputLockEntrance( inputdata_t &data )
{
	m_bEntranceLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputUnlockEntrance( inputdata_t &data )
{
	m_bEntranceLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputLockExit( inputdata_t &data )
{
	m_bExitLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputUnlockExit( inputdata_t &data )
{
	m_bExitLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::PassengerInTransition( void )
{
	// FIXME: Big hack - we need a way to bridge this data better
	// TODO: Get a list of passengers we can traverse instead
	CBaseEntity *pPassenger = gEntList.FindEntityByClassname( NULL, "npc_alyx" );
	if ( pPassenger == NULL )
		return false;

	CNPC_Alyx *pAlyx = static_cast<CNPC_Alyx *>(pPassenger);
	if ( pAlyx->GetPassengerState() == PASSENGER_STATE_ENTERING ||
		 pAlyx->GetPassengerState() == PASSENGER_STATE_EXITING )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether to override our normal punting behavior
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::ShouldPuntUseLaunchForces( void )
{
	if ( IsOverturned() || PassengerInTransition() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Override velocity if our passenger is transitioning or we're upside-down
//-----------------------------------------------------------------------------
Vector CPropJeepEpisodic::PhysGunLaunchVelocity( const Vector &forward, float flMass )
{
	// Disallow
	if ( PassengerInTransition() )
		return vec3_origin;

	return Vector( 0, 0, 500 ); 
}

//-----------------------------------------------------------------------------
// Purpose: Rolls the vehicle when its trying to upright itself from a punt
//-----------------------------------------------------------------------------
AngularImpulse CPropJeepEpisodic::PhysGunLaunchAngularImpulse( void ) 
{ 
	// Disallow
	if ( PassengerInTransition() )
		return Vector( 0, 0, 0 );

	// Roll!
	return Vector( 0, 300, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Get the upright strength based on what state we're in
//-----------------------------------------------------------------------------
float CPropJeepEpisodic::GetUprightStrength( void ) 
{ 
	// Lesser if overturned
	if ( IsOverturned() )
		return 16.0f;
	
	// Strong if upright already (prevents tipping)
	return 64.0f; 
}
