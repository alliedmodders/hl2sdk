//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Companion NPCs riding in cars
//
//=============================================================================

#include "cbase.h"
#include "ai_speech.h"
#include "ai_pathfinder.h"
#include "ai_waypoint.h"
#include "ai_navigator.h"
#include "ai_navgoaltype.h"
#include "ai_memory.h"
#include "ai_behavior_passenger_companion.h"
#include "ai_squadslot.h"
#include "npc_playercompanion.h"
#include "ai_route.h"
#include "saverestore_utlvector.h"

#define	TLK_PASSENGER_WARN_COLLISION	"TLK_PASSENGER_WARN_COLLISION"
#define	TLK_PASSENGER_IMPACT			"TLK_PASSENGER_IMPACT"
#define	TLK_PASSENGER_OVERTURNED		"TLK_PASSENGER_OVERTURNED"
#define	TLK_PASSENGER_REQUEST_UPRIGHT	"TLK_PASSENGER_REQUEST_UPRIGHT"

#define	PASSENGER_NEAR_VEHICLE_THRESHOLD	64.0f

#define MIN_OVERTURNED_DURATION			1.0f // seconds
#define MIN_FAILED_EXIT_ATTEMPTS		2
#define MIN_OVERTURNED_WARN_DURATION	4.0f // seconds

ConVar passenger_impact_response_threshold( "passenger_impact_response_threshold", "-500.0" );
ConVar passenger_collision_response_threshold( "passenger_collision_response_threshold", "500.0" );
extern ConVar passenger_debug_transition;

// Custom activities
Activity ACT_PASSENGER_IDLE_AIM;
Activity ACT_PASSENGER_RELOAD;
Activity ACT_PASSENGER_OVERTURNED;
Activity ACT_PASSENGER_IMPACT;
Activity ACT_PASSENGER_IMPACT_WEAPON;
Activity ACT_PASSENGER_POINT;
Activity ACT_PASSENGER_POINT_BEHIND;
Activity ACT_PASSENGER_IDLE_READY;

BEGIN_DATADESC( CAI_PassengerBehaviorCompanion )

	DEFINE_EMBEDDED( m_vehicleState ),
	DEFINE_EMBEDDED( m_VehicleMonitor ),

	DEFINE_UTLVECTOR( m_FailedEntryPositions, FIELD_EMBEDDED ),
	
	DEFINE_FIELD( m_flOverturnedDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_flUnseenDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_nExitAttempts, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextOverturnWarning, FIELD_TIME ),

END_DATADESC();

BEGIN_SIMPLE_DATADESC( FailPosition_t )
	
	DEFINE_FIELD( vecPosition, FIELD_VECTOR ),
	DEFINE_FIELD( flTime, FIELD_TIME ),

END_DATADESC();

CAI_PassengerBehaviorCompanion::CAI_PassengerBehaviorCompanion( void ) : 
m_flUnseenDuration( 0.0f ),
m_flNextOverturnWarning( 0.0f ),
m_flOverturnedDuration( 0.0f ), 
m_nExitAttempts( 0 )
{
	memset( &m_vehicleState, 0, sizeof( m_vehicleState ) );
	m_VehicleMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : int
//-----------------------------------------------------------------------------
Activity CAI_PassengerBehaviorCompanion::NPC_TranslateActivity( Activity activity )
{
	Activity newActivity = BaseClass::NPC_TranslateActivity( activity );

	// Handle animations from inside the vehicle
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Make sure idles are always vehicle idles
		if ( newActivity == ACT_IDLE )
		{
			newActivity = (Activity) ACT_PASSENGER_IDLE;
		}

		// Alter idle depending on the vehicle's state
		if ( newActivity == ACT_PASSENGER_IDLE )
		{
			// Always play the overturned animation
			if ( m_vehicleState.m_bWasOverturned )
				return ACT_PASSENGER_OVERTURNED;

			// If we have an enemy and a gun, aim
			if ( GetEnemy() != NULL && HasCondition( COND_SEE_ENEMY ) && GetOuter()->GetActiveWeapon() )
				return ACT_PASSENGER_IDLE_AIM;

			CNPC_PlayerCompanion *pCompanion = dynamic_cast<CNPC_PlayerCompanion *>(GetOuter());
			if ( pCompanion != NULL && pCompanion->GetReadinessLevel() >= AIRL_STIMULATED )
				return ACT_PASSENGER_IDLE_READY;

		}

		// Override reloads
		if ( newActivity == ACT_RELOAD )
			return ACT_PASSENGER_RELOAD;

		// Override range attacks
		if ( newActivity == ACT_RANGE_ATTACK1 )
			return (Activity) ACT_PASSENGER_RANGE_ATTACK1;

		// FIXME: Translation is never called for scripted events
		// Do special logic in points
		/*
		if ( newActivity == ACT_PASSENGER_POINT )
		{
			// See if this is behind us
			float curYaw = GetOuter()->GetPoseParameter( "aim_yaw" );
			if ( fabs( curYaw ) > 180.0f )
				return ACT_PASSENGER_POINT_BEHIND;
		}
		*/
	}

	return newActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed the vehicle is moving at
// Output : units per second
//-----------------------------------------------------------------------------
float CAI_PassengerBehaviorCompanion::GetVehicleSpeed( void )
{
	if ( m_hVehicle == NULL )
	{
		Assert(0);
		return -1.0f;
	}

	Vector	vecVelocity;
	m_hVehicle->GetVelocity( &vecVelocity, NULL );

	// Get our speed
	return vecVelocity.Length();
}

//-----------------------------------------------------------------------------
// Purpose: Detect oncoming collisions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherVehicleCollisionConditions( const Vector &localVelocity )
{
	// Look for walls in front of us
	if ( localVelocity.y > passenger_collision_response_threshold.GetFloat() )
	{
		// Detect an upcoming collision
		Vector vForward;
		m_hVehicle->GetVectors( &vForward, NULL, NULL );

		// Use a smaller bounding box to make it detect mostly head-on impacts
		Vector	mins, maxs;
		mins.Init( -16, -16, 32 );
		maxs.Init(  16,  16, 64 );

		// Look 3/4 a second into the future
		float dt = 0.75f;
		float distance = localVelocity.y * dt;

		// Trace ahead of us to see what's there
		trace_t	tr;
		UTIL_TraceHull( m_hVehicle->GetAbsOrigin(), m_hVehicle->GetAbsOrigin() + ( vForward * distance ), mins, maxs, MASK_SOLID_BRUSHONLY, m_hVehicle, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() )
		{
			// We need to see how "head-on" to the surface we are
			float impactDot = DotProduct( tr.plane.normal, vForward );

			// Don't warn over grazing blows or slopes
			if ( impactDot < -0.9f && tr.plane.normal.z < 0.75f )
			{
				// Only warn if it's not too soon to do it again
				if ( m_vehicleState.m_flNextWarningTime < gpGlobals->curtime )
				{
					// TODO: Turn this into a condition so that we can interrupt other schedules
					GetOuter()->GetExpresser()->Speak( TLK_PASSENGER_WARN_COLLISION );
					m_vehicleState.m_flNextWarningTime = gpGlobals->curtime + 5.0f;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions we can comment on or react to while riding in the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherVehicleStateConditions( void )
{
	if ( m_hVehicle == NULL )
		return;

	// Get the vehicle's boost state
	if ( m_hVehicle->m_nBoostTimeLeft < 100.0f )
	{
		if ( m_vehicleState.m_bWasBoosting == false )
		{
			m_vehicleState.m_bWasBoosting = true;
		}
	}
	else
	{
		m_vehicleState.m_bWasBoosting = false;
	}

	Vector	localVelocity;
	GetLocalVehicleVelocity( &localVelocity );

	// Get our speed
	float flSpeedSqr = localVelocity.LengthSqr();

	// See if we've crossed over the threshold between movement and... stillness
	if ( m_vehicleState.m_flLastSpeed > STOPPED_VELOCITY_THRESHOLD_SQR && flSpeedSqr < STOPPED_VELOCITY_THRESHOLD_SQR )
	{
		SetCondition( COND_VEHICLE_STOPPED );
	}
	else
	{
		ClearCondition( COND_VEHICLE_STOPPED );
	}

	// Store off the speed
	m_vehicleState.m_flLastSpeed = flSpeedSqr;

	// Find our delta velocity from the last frame
	Vector	deltaVelocity = ( localVelocity - m_vehicleState.m_vecLastLocalVelocity );
	m_vehicleState.m_vecLastLocalVelocity = localVelocity;

	// Detect a sudden stop
	if ( deltaVelocity.y < passenger_impact_response_threshold.GetFloat() )
	{
		SetCondition( COND_VEHICLE_HARD_IMPACT );
		GetOuter()->GetExpresser()->Speak( TLK_PASSENGER_IMPACT );
	}

	// Detect being overturned
	if ( m_hVehicle->IsOverturned() )
	{	
		if ( m_vehicleState.m_bWasOverturned == false )
		{
			SetCondition( COND_VEHICLE_OVERTURNED );
			m_vehicleState.m_bWasOverturned = true;

			if ( m_vehicleState.m_flNextWarningTime < gpGlobals->curtime )
			{
				// FIXME: Delay for a bit
				GetOuter()->GetExpresser()->Speak( TLK_PASSENGER_OVERTURNED );
				m_vehicleState.m_flNextWarningTime = gpGlobals->curtime + 5.0f;
			}
		}
	}
	else
	{
		ClearCondition( COND_VEHICLE_OVERTURNED );
		m_vehicleState.m_bWasOverturned = false;
	}

	// See if we're going to collide with anything soon
	GatherVehicleCollisionConditions( localVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: Handles exit failure notifications
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::OnExitVehicleFailed( void )
{
	m_nExitAttempts++;
}

//-----------------------------------------------------------------------------
// Purpose: Track how long we've been overturned
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::UpdateStuckStatus( void )
{
	if ( m_hVehicle == NULL )
		return;

	// Always clear this to start out with
	ClearCondition( COND_CAN_LEAVE_STUCK_VEHICLE );

	// If we can't exit the vehicle, then don't bother with these checks
	if ( m_hVehicle->NPC_CanExitVehicle( GetOuter(), true ) == false )
		return;

	bool bVisibleToPlayer = false;
	bool bPlayerInVehicle = false;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	if ( pPlayer )
	{
		bVisibleToPlayer = pPlayer->FInViewCone( GetOuter()->GetAbsOrigin() );
		bPlayerInVehicle = pPlayer->IsInAVehicle();
	}

	// If we're not overturned, just reset our counter
	if ( m_vehicleState.m_bWasOverturned == false )
	{
		m_flOverturnedDuration = 0.0f;
		m_flUnseenDuration = 0.0f;
	}
	else
	{
		// Add up the time since we last checked
		m_flOverturnedDuration += ( gpGlobals->curtime - GetLastThink() );
	}

	// Warn about being stuck upside-down if it's been long enough
	if ( m_flOverturnedDuration > MIN_OVERTURNED_WARN_DURATION && m_flNextOverturnWarning < gpGlobals->curtime )
	{
		SetCondition( COND_WARN_OVERTURNED );
	}

	// If the player can see us or is still in the vehicle, we never exit
	if ( bVisibleToPlayer || bPlayerInVehicle )
	{
		// Reset our timer
		m_flUnseenDuration = 0.0f;
		return;
	}

	// Add up the time since we last checked
	m_flUnseenDuration += ( gpGlobals->curtime - GetLastThink() );

	// If we've been overturned for long enough or tried to exit one too many times
	if ( m_vehicleState.m_bWasOverturned )
	{
		if ( m_flUnseenDuration > MIN_OVERTURNED_DURATION )
		{
			SetCondition( COND_CAN_LEAVE_STUCK_VEHICLE );
		}
	}
	else if ( m_nExitAttempts >= MIN_FAILED_EXIT_ATTEMPTS )
	{
		// The player can't be looking at us
		SetCondition( COND_CAN_LEAVE_STUCK_VEHICLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions for our use in making decisions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::GatherConditions( void )
{
	// Code below relies on these conditions being set first!
	BaseClass::GatherConditions();

	// In-car conditions
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		// Get info on how we're driving
		GatherVehicleStateConditions();
		
		// See if we're upside-down
		UpdateStuckStatus();
	}

	// Make sure a vehicle doesn't stray from its mark
	if ( IsCurSchedule( SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE ) )
	{
		if ( m_VehicleMonitor.TargetMoved( m_hVehicle ) )
		{
			SetCondition( COND_VEHICLE_MOVED_FROM_MARK );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::AimGun( void )
{
	// Aim at enemies we may have
	if ( GetEnemy() && HasCondition( COND_SEE_ENEMY ) )
	{
		Vector vecShootOrigin = GetOuter()->Weapon_ShootPosition();
		Vector vecShootDir = GetOuter()->GetShootEnemyDir( vecShootOrigin, false );

		//NDebugOverlay::Cross3D( vecShootOrigin, 32, 255, 0, 0, true, 0.5f );
		//NDebugOverlay::Line( vecShootOrigin, vecShootOrigin + ( vecShootDir * 128 ), 255, 0, 0, true, 0.5f );

		GetOuter()->SetAim( vecShootDir );
	}
	else
	{
		// Stop aiming
		GetOuter()->RelaxAim();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Select schedules when we're riding in the car
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectScheduleInsideVehicle( void )
{
	// Overturned
	if ( HasCondition( COND_VEHICLE_OVERTURNED ) )
		return SCHED_PASSENGER_OVERTURNED;

	if ( HasCondition( COND_VEHICLE_HARD_IMPACT	) )
		return SCHED_PASSENGER_IMPACT;

	// Look for exiting the vehicle
	if ( HasCondition( COND_CAN_LEAVE_STUCK_VEHICLE ) )
		return SCHED_PASSENGER_EXIT_STUCK_VEHICLE;

	// Fire on targets 
	if ( GetEnemy() )
	{
		// Always face
		GetOuter()->AddLookTarget( GetEnemy(), 1.0f, 2.0f );
		
		if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) && ( GetOuter()->GetShotRegulator()->IsInRestInterval() == false ) )
			return SCHED_PASSENGER_RANGE_ATTACK1;
	}

	// Say an overturned line
	if ( HasCondition( COND_WARN_OVERTURNED ) )
	{
		GetOuter()->GetExpresser()->Speak( TLK_PASSENGER_REQUEST_UPRIGHT );
		m_flNextOverturnWarning = gpGlobals->curtime + random->RandomFloat( 5.0f, 10.0f );
		ClearCondition( COND_WARN_OVERTURNED );
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Select schedules while we're outisde the car
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectScheduleOutsideVehicle( void )
{
	// Reset our mark
	m_VehicleMonitor.SetMark( m_hVehicle, 8.0f );

	// Wait if the vehicle is moving
	if ( ( GetVehicleSpeed() > STOPPED_VELOCITY_THRESHOLD ) || m_hVehicle->IsOverturned() )
	{
		GetOuter()->SetTarget( m_hVehicle );
		return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED;
	}

	// If we intend to enter, run to the vehicle
	if ( m_PassengerIntent == PASSENGER_INTENT_ENTER )
		return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE;

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Overrides the schedule selection
// Output : int - Schedule to play
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectSchedule( void )
{
	// Entering schedule
	if ( HasCondition( COND_ENTERING_VEHICLE ) )
	{
		ClearCondition( COND_ENTERING_VEHICLE );
		return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE;
	}

	// Handle schedules based on our passenger state
	if ( GetPassengerState() == PASSENGER_STATE_OUTSIDE )
	{
		int nSched = SelectScheduleOutsideVehicle();
		if ( nSched != SCHED_NONE )
			return nSched;
	}
	else if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		int nSched = SelectScheduleInsideVehicle();
		if ( nSched != SCHED_NONE )
			return nSched;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	switch( failedTask )
	{
	case TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT:
		{
			// If we're not close enough, then get nearer the target
			if ( UTIL_DistApprox( m_hVehicle->GetAbsOrigin(), GetOuter()->GetAbsOrigin() ) > PASSENGER_NEAR_VEHICLE_THRESHOLD )
				return SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED;

			// Stand around and wait for something to open up
			GetOuter()->SetTarget( m_hVehicle );	
				
			return SCHED_PASSENGER_ENTER_VEHICLE_PAUSE;
		}
		break;

	case TASK_GET_PATH_TO_NEAR_VEHICLE:
		return SCHED_PASSENGER_ENTER_VEHICLE_PAUSE;
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: Start to enter the vehicle
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::EnterVehicle( void )
{
	BaseClass::EnterVehicle();

	m_nExitAttempts = 0;
	m_VehicleMonitor.SetMark( m_hVehicle, 8.0f );
	
	// Remove this flag because we're sitting so close we always think we're going to hit the player
	// FIXME: We need to store this state so we don't incorrectly restore it later
	GetOuter()->CapabilitiesRemove( bits_CAP_NO_HIT_PLAYER );

	// Discard enemies quickly
	GetOuter()->GetEnemies()->SetEnemyDiscardTime( 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::FinishEnterVehicle( void )
{
	BaseClass::FinishEnterVehicle();

	// We succeeded
	ResetVehicleEntryFailedState();
}

//-----------------------------------------------------------------------------
// Purpose: Vehicle has been completely exited
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::FinishExitVehicle( void )
{
	BaseClass::FinishExitVehicle();

	m_nExitAttempts = 0;
	m_VehicleMonitor.ClearMark();

	// FIXME: We need to store this state so we don't incorrectly restore it later
	GetOuter()->CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER );

	// FIXME: Restore this properly
	GetOuter()->GetEnemies()->SetEnemyDiscardTime( AI_DEF_ENEMY_DISCARD_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Tries to build a route to the entry point of the target vehicle.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::FindPathToVehicleEntryPoint( void )
{
	// Set our custom move name
	bool bFindNearest = ( GetOuter()->m_NPCState == NPC_STATE_COMBAT || GetOuter()->m_NPCState == NPC_STATE_ALERT );
	int nSequence = FindEntrySequence( bFindNearest );
	if ( nSequence  == -1 )
		return false;

	// We have to do this specially because the activities are not named
	SetTransitionSequence( nSequence );

	// Get the entry position
	Vector vecEntryPoint;
	QAngle vecEntryAngles;
	GetEntryPoint( m_nTransitionSequence, &vecEntryPoint, &vecEntryAngles );

	// Find the actual point on the ground to base our pathfinding on
	Vector vecActualPoint;
	if ( FindGroundAtPosition( vecEntryPoint, 16, 64, &vecActualPoint ) == false )
	{
		MarkVehicleEntryFailed( vecEntryPoint );
		return false;
	}

	// If we're already close enough, just succeed
	float flDistToGoal = ( GetOuter()->GetAbsOrigin() - vecActualPoint ).Length();
	if ( flDistToGoal < 8 )
		return true;

	// Setup our goal
	AI_NavGoal_t goal( GOALTYPE_LOCATION );
	goal.arrivalActivity = ACT_SCRIPT_CUSTOM_MOVE;
	goal.dest = vecActualPoint;

	// Try and set a direct route
	if ( GetOuter()->GetNavigator()->SetGoal( goal ) )
	{
		GetOuter()->GetNavigator()->SetArrivalDirection( vecEntryAngles );
		//GetOuter()->GetNavigator()->SetArrivalActivity( ACT_SCRIPT_CUSTOM_MOVE );
		//GetOuter()->GetNavigator()->SetArrivalSpeed( 64 );

		return true;
	}

	// We failed, so remember it
	MarkVehicleEntryFailed( vecEntryPoint );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Tests the route and position to see if it's valid
// Input  : &vecTestPos - position to test
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::CanExitAtPosition( const Vector &vecTestPos )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	if ( pPlayer == NULL )
		return false;

	// Can't be in our potential view
	if ( pPlayer->FInViewCone( vecTestPos ) )
		return false;

	// Check to see if that path is clear and valid
	return IsValidTransitionPoint( GetOuter()->WorldSpaceCenter(), vecTestPos );
}

#define	NUM_EXIT_ITERATIONS	8

//-----------------------------------------------------------------------------
// Purpose: Find a position we can use to exit the vehicle via teleportation
// Input  : *vecResult - safe place to exit to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::GetStuckExitPos( Vector *vecResult )
{
	// Get our right direction
	Vector vecVehicleRight;
	m_hVehicle->GetVectors( NULL, &vecVehicleRight, NULL );
	
	// Get the vehicle's rough horizontal bounds
	float	flVehicleRadius = m_hVehicle->CollisionProp()->BoundingRadius2D();

	// Use the vehicle's center as our hub
	Vector	vecCenter = m_hVehicle->WorldSpaceCenter();

	// Angle whose tan is: y/x
	float	flCurAngle = atan2f( vecVehicleRight.y, vecVehicleRight.x );
	float	flAngleIncr = (M_PI*2.0f)/(float)NUM_EXIT_ITERATIONS;
	Vector	vecTestPos;

	// Test a number of discrete exit routes
	for ( int i = 0; i <= NUM_EXIT_ITERATIONS-1; i++ )
	{
		// Get our position
		SinCos( flCurAngle, &vecTestPos.y, &vecTestPos.x );
		vecTestPos.z = 0.0f;
		vecTestPos *= flVehicleRadius;
		vecTestPos += vecCenter;

		// Test the position
		if ( CanExitAtPosition( vecTestPos ) )
		{
			// Take the result
			*vecResult = vecTestPos;
			return true;
		}

		// Move to the next iteration
		flCurAngle += flAngleIncr;
	}

	// None found
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to get out of an overturned vehicle when the player isn't looking
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::ExitStuckVehicle( void )
{
	// Try and find an exit position
	Vector vecExitPos;
	if ( GetStuckExitPos( &vecExitPos ) == false )
		return false;

	// Detach from the parent
	GetOuter()->SetParent( NULL );

	// Do all necessary clean-up
	FinishExitVehicle();

	// Teleport to the destination 
	// TODO: Make sure that the player can't see this!
	GetOuter()->Teleport( &vecExitPos, &vec3_angle, &vec3_origin );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::StartTask( const Task_t *pTask )
{
	// We need to override these so we never face
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		if ( pTask->iTask == TASK_FACE_TARGET || 
			 pTask->iTask == TASK_FACE_ENEMY ||
			 pTask->iTask == TASK_FACE_IDEAL ||
			 pTask->iTask == TASK_FACE_HINTNODE ||
			 pTask->iTask == TASK_FACE_LASTPOSITION ||
			 pTask->iTask == TASK_FACE_PATH ||
			 pTask->iTask == TASK_FACE_PLAYER ||
			 pTask->iTask == TASK_FACE_REASONABLE ||
			 pTask->iTask == TASK_FACE_SAVEPOSITION ||
			 pTask->iTask == TASK_FACE_SCRIPT )
		{
			return TaskComplete();
		}
	}

	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT:
		{
			// Reserve an entry point
			if ( ReserveEntryPoint( VEHICLE_SEAT_ANY ) == false )
			{
				TaskFail( "No valid entry point!\n" );
				return;
			}

			// Find where we're going
			if ( FindPathToVehicleEntryPoint() )
			{
				TaskComplete();
				return;
			}

			// We didn't find a path
			TaskFail( "TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT: Unable to run to entry point" );
		}
		break;

	case TASK_GET_PATH_TO_TARGET:
		{
			GetOuter()->SetTarget( m_hVehicle );
			BaseClass::StartTask( pTask );
		}
		break;

	case TASK_GET_PATH_TO_NEAR_VEHICLE:
		{
			// Find the passenger offset we're going for
			Vector vecRight;
			m_hVehicle->GetVectors( NULL, &vecRight, NULL );
			Vector vecTargetOffset = vecRight * 64.0f;

			// Try and find a path near there
			AI_NavGoal_t goal( GOALTYPE_TARGETENT, vecTargetOffset, AIN_DEF_ACTIVITY, 64.0f, AIN_UPDATE_TARGET_POS, m_hVehicle );
			GetOuter()->SetTarget( m_hVehicle );
			if ( GetOuter()->GetNavigator()->SetGoal( goal ) )
			{
				TaskComplete();
				return;
			}

			TaskFail( "Unable to find path to get closer to vehicle!\n" );
			return;
		}

		break;

	case TASK_PASSENGER_RELOAD:
		{
			GetOuter()->SetIdealActivity( ACT_PASSENGER_RELOAD );
			return;
		}
		break;
	
	case TASK_PASSENGER_EXIT_STUCK_VEHICLE:
		{
			if ( ExitStuckVehicle() )
			{
				TaskComplete();
				return;
			}

			TaskFail("Unable to exit overturned vehicle!\n");
		}
		break;

	case TASK_PASSENGER_OVERTURNED:
		{
			// Go into our overturned animation
			if ( GetOuter()->GetIdealActivity() != ACT_PASSENGER_OVERTURNED )
			{
				GetOuter()->SetIdealActivity( ACT_PASSENGER_OVERTURNED );
			}

			TaskComplete();
		}
		break;

	case TASK_PASSENGER_IMPACT:
		{
			// Go into our impact animation
			GetOuter()->ResetIdealActivity( ACT_PASSENGER_IMPACT );
			
			// Delay for twice the duration of our impact animation
			int nSequence = GetOuter()->SelectWeightedSequence( ACT_PASSENGER_IMPACT ); 
			GetOuter()->SetNextAttack( gpGlobals->curtime + (GetOuter()->SequenceDuration( nSequence ) * 2.0f ) );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_PASSENGER_RELOAD:
		{
			if ( GetOuter()->IsSequenceFinished() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_PASSENGER_IMPACT:
		{
			if ( GetOuter()->IsSequenceFinished() )
			{
				TaskComplete();
				return;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add custom interrupt conditions
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::BuildScheduleTestBits( void )
{
	// Always break on being able to exit
	if ( GetPassengerState() == PASSENGER_STATE_INSIDE )
	{
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_CAN_LEAVE_STUCK_VEHICLE ) );
		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_VEHICLE_HARD_IMPACT) );
		
		if ( GetOuter()->IsCurSchedule( SCHED_PASSENGER_OVERTURNED ) == false )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_VEHICLE_OVERTURNED ) );
		}
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to build a local route to a goal and append it to the chain (if it exists)
// Input  : &vecStart - Path's starting position
//			&vecEnd - Path's ending position
//			**pWaypoints - pointer to a waypoint list to append the data to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::AppendLocalPath( const Vector &vecStart, const Vector &vecEnd, AI_Waypoint_t **pWaypoints )
{
	// Find the next waypoint to use
	AI_Waypoint_t *pNextRoute = GetOuter()->GetPathfinder()->BuildLocalRoute( vecStart, vecEnd, NULL, 0, NO_NODE, bits_BUILD_GROUND, 8.0f );
	if ( pNextRoute == NULL )
		return false;

	// We cannot simply our route
	pNextRoute->ModifyFlags( bits_WP_DONT_SIMPLIFY, true );

	// Start the list
	if ( (*pWaypoints) == NULL )
	{
		*pWaypoints = pNextRoute;
	}
	else
	{
		// Concat the list
		AddWaypointLists( (*pWaypoints), pNextRoute );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the "quadrant" in which we're aligned around an entity
// Input  : &vecPos - position to test
//			*pCenter - Center entity (facing and origin used)
// Output : top-left = 0, bottom-left = 1, bottom-right = 2, top-right = 3 (winding counter-clockwise)
//-----------------------------------------------------------------------------
inline int FindQuadrantForPosition( const Vector &vecPos, CBaseEntity *pCenterEnt )
{
	// Find the direction to our goal entity
	Vector vecPosToGoalDir = ( pCenterEnt->GetAbsOrigin() - vecPos );
	vecPosToGoalDir.z = 0.0f;
	VectorNormalize( vecPosToGoalDir );

	// Get our goal entity's facing
	Vector vecForward, vecRight;
	pCenterEnt->GetVectors( &vecForward, &vecRight, NULL );

	// Find in what "quadrant" the target is in
	float flForwardDot = DotProduct( vecForward, vecPosToGoalDir );
	float flRightDot = DotProduct( vecRight, vecPosToGoalDir );

	int nQuadrant;
	if ( flForwardDot < 0.0f )
	{
		// Top right / left
		nQuadrant = ( flRightDot < 0.0f ) ? 3 : 0;
	}
	else
	{
		// Bottom right / left
		nQuadrant = ( flRightDot < 0.0f ) ? 2 : 1;
	}

	return nQuadrant;
}

//-----------------------------------------------------------------------------
// Purpose: Get the next node (with wrapping) around a circularly wound path
// Input  : nLastNode - The starting node
//			nDirection - Direction we're moving
//			nNumNodes - Total nodes in the chain
//-----------------------------------------------------------------------------
inline int GetNextNode( int nLastNode, int nDirection, int nNumNodes )
{
	// FIXME: Account for non-singular steps

	int nNextNode = nLastNode + nDirection;
	if ( nNextNode > (nNumNodes-1) )
		nNextNode = 0;
	else if ( nNextNode < 0 )
		nNextNode = (nNumNodes-1);

	return nNextNode;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to construct a local path given a set of nodes and criteria
// Input  : *vecPositions - positions of the nodes to use
//			nNumNodes - number of nodes supplied
//			nDirection - direction in which to traverse the node list
// Output : Returns the path constructed, NULL if none could be made
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_PassengerBehaviorCompanion::BuildNodeRouteAroundVehicle( const VehicleAvoidParams_t &avoidParams )
{
	// DEBUG
	/*
	for ( int i = 0; i < avoidParams.nNumNodes; i++ )
	{
		if ( i == avoidParams.nStartNode )
		{
			NDebugOverlay::Box( avoidParams.pNodePositions[i], -Vector(14,14,14), Vector(14,14,14), 0, 255, 0, true, 1.0f );
		}
		else if ( i == avoidParams.nEndNode	)
		{
			NDebugOverlay::Box( avoidParams.pNodePositions[i], -Vector(14,14,14), Vector(14,14,14), 0, 0, 255, true, 1.0f );
		}
		else
		{
			NDebugOverlay::Box( avoidParams.pNodePositions[i], -Vector(14,14,14), Vector(14,14,14), 255, 0, 0, true, 1.0f );
		}
	}
	*/
	// END DEBUG

	// Get the next node along the path
	int nNextNode = GetNextNode( avoidParams.nStartNode, avoidParams.nDirection, avoidParams.nNumNodes );

	// NDebugOverlay::Cross3D( avoidParams.pNodePositions[nNextNode], 32.0f, 255, 0, 0, true, 2.0f );

	// If we're not moving directly to the goal, see if we're already along the path
	if ( nNextNode != avoidParams.nEndNode )
	{
		// Find the direction between us and our first node
		Vector vecStartDir = ( GetOuter()->GetAbsOrigin() - avoidParams.pNodePositions[avoidParams.nStartNode] );
		VectorNormalize( vecStartDir );

		// Find the direction of the next leg of the path
		Vector vecPathDir = ( avoidParams.pNodePositions[avoidParams.nStartNode] - avoidParams.pNodePositions[nNextNode] );
		VectorNormalize( vecPathDir );

		// We're closer to the next node on our path, just go there!
		if ( DotProduct( vecStartDir, vecPathDir ) < 0.0f )
		{
			// Validate that we have a clear shot to it
			trace_t tr;
			UTIL_TraceHull( GetOuter()->GetAbsOrigin(), avoidParams.pNodePositions[nNextNode], GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_VEHICLE, &tr );
			if ( tr.fraction < 1.0f )
			{
				// Nope, run to our start
				nNextNode = avoidParams.nStartNode;

				//NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), avoidParams.pNodePositions[nNextNode], 4, 255, 0, 0, 128, true, 2.0f );
			}
			/*else
			{
				NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), avoidParams.pNodePositions[nNextNode], 4, 0, 255, 0, 128, true, 2.0f );
			}*/
		}
	}

	AI_Waypoint_t *pHeadRoute = NULL;

	// Attempt to path to our next node (skipping the first if possible)
	if ( AppendLocalPath( avoidParams.vecStartPos, avoidParams.pNodePositions[nNextNode], &pHeadRoute ) == false )
	{
		//NDebugOverlay::HorzArrow( avoidParams.vecStartPos, avoidParams.pNodePositions[nNextNode], 32, 255, 0, 0, 128, true, 2.0f );

		// Failing that, just run to our start position
		nNextNode = avoidParams.nStartNode;
		if ( AppendLocalPath( avoidParams.vecStartPos, avoidParams.pNodePositions[nNextNode], &pHeadRoute ) == false )
		{
			//NDebugOverlay::HorzArrow( avoidParams.vecStartPos, avoidParams.pNodePositions[nNextNode], 32, 255, 0, 0, 128, true, 2.0f );
			return false;
		}
	}

	// Now walk the path and keep adding point on until we're finished
	int nLastNode = 0;
	int nSteps = 0;
	while ( nSteps < avoidParams.nNumNodes )
	{
		// Move the node ahead
		nLastNode = nNextNode;
		nNextNode = GetNextNode( nNextNode, avoidParams.nDirection, avoidParams.nNumNodes );

		// Try the next leg of the path
		if ( AppendLocalPath( avoidParams.pNodePositions[nLastNode], avoidParams.pNodePositions[nNextNode], &pHeadRoute ) == false )
		{
			//NDebugOverlay::HorzArrow( avoidParams.pNodePositions[nLastNode], avoidParams.pNodePositions[nNextNode], 16, 255, 0, 0, 128, true, 2.0f );
			return NULL;
		}

		// See if we're at the final node (finished if we are)
		if ( nNextNode == avoidParams.nEndNode )
			return pHeadRoute;

		//NDebugOverlay::HorzArrow( avoidParams.pNodePositions[nLastNode], avoidParams.pNodePositions[nNextNode], 16, 0, 255, 0, 128, true, 2.0f );

		// Increment the counter
		nSteps++;
	}

	// No path found!
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Quickly walk a circular path to find the winding that's shortest
// Input  : *pParams - avoid params
//-----------------------------------------------------------------------------
inline void FindShortestDirectionToNode( VehicleAvoidParams_t *pParams )
{
	int nNextNode = GetNextNode( pParams->nStartNode, 1, pParams->nNumNodes );
	int nDistance = 1;

	// Try going counter-clockwise
	for ( int i = 0; i < pParams->nNumNodes; i++ )
	{
		if ( nNextNode == pParams->nEndNode )
			break;

		nNextNode = GetNextNode( nNextNode, 1, pParams->nNumNodes );
		nDistance++;
	}

	nNextNode = GetNextNode( pParams->nStartNode, -1, pParams->nNumNodes );

	// Now go the other way and see if it's shorter to do so
	for ( int i = 0; i < nDistance; i++ )
	{
		if ( nNextNode == pParams->nEndNode )
		{
			pParams->nDirection = -1;
			return;
		}

		nNextNode = GetNextNode( nNextNode, -1, pParams->nNumNodes );
	}

	pParams->nDirection = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Try to build a route around a blocking vehicle (lest we bump into it dumbly)
// Output : Returns true if a path was built successfully
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::BuildVehicleAvoidancePath( CBaseEntity *pVehicle, const Vector &vecMoveDir )
{
	// Get our local OBB and inflate it by our hull size
	Vector vecLocalMins = pVehicle->CollisionProp()->OBBMins();
	Vector vecLocalMaxs = pVehicle->CollisionProp()->OBBMaxs();

	// FIXME: For some reason the OBB is crazy-big!
	// FIXME: Shrink the OBB for giggles
	vecLocalMins[0] += GetOuter()->GetHullWidth()*0.5f;
	vecLocalMins[1] += GetOuter()->GetHullWidth()*0.5f;

	vecLocalMaxs[0] -= GetOuter()->GetHullWidth()*0.5f;
	vecLocalMaxs[1] -= GetOuter()->GetHullWidth()*0.5f;

	vecLocalMins[2] = 0.0f;
	vecLocalMaxs[2] = 0.0f;

	// Get the four corners in worldspace as our points of circumnavigation
	Vector vecNodes[4];
	Vector vecScratch;
	matrix3x4_t matColToWorld = pVehicle->CollisionProp()->CollisionToWorldTransform();

	vecScratch.z = 0.0f;

	// Top left
	vecScratch.x = vecLocalMins.x;
	vecScratch.y = vecLocalMaxs.y;
	VectorTransform( vecScratch, matColToWorld, vecNodes[0] );

	// Bottom left
	vecScratch.x = vecLocalMins.x;
	vecScratch.y = vecLocalMins.y;
	VectorTransform( vecScratch, matColToWorld, vecNodes[1] );

	// Bottom right
	vecScratch.x = vecLocalMaxs.x;
	vecScratch.y = vecLocalMins.y;
	VectorTransform( vecScratch, matColToWorld, vecNodes[2] );

	// Top right
	vecScratch.x = vecLocalMaxs.x;
	vecScratch.y = vecLocalMaxs.y;
	VectorTransform( vecScratch, matColToWorld, vecNodes[3] );

	VehicleAvoidParams_t avoidParams;

	// Get the quadrants we're moving between
	avoidParams.nStartNode = FindQuadrantForPosition( GetOuter()->GetAbsOrigin(), pVehicle );
	avoidParams.nEndNode = FindQuadrantForPosition( GetOuter()->GetNavigator()->GetPath()->ActualGoalPosition(), pVehicle );

	// If we're moving within the same quadrant, we've got no path
	if ( avoidParams.nStartNode == avoidParams.nEndNode )
		return false;

	// Get the direction of traversal for the array ( 1 = counter-clockwise )
	avoidParams.pNodePositions = vecNodes;
	avoidParams.nNumNodes = ARRAYSIZE( vecNodes );

	// Specify the points in space we're coming from and going to
	avoidParams.vecStartPos = GetOuter()->GetAbsOrigin();
	avoidParams.vecGoalPos = GetOuter()->GetNavigator()->GetPath()->ActualGoalPosition();

	// Find our shortest path to the node in question
	FindShortestDirectionToNode( &avoidParams );

	// Try and build a route around the vehicle
	AI_Waypoint_t *pAvoidRoute = BuildNodeRouteAroundVehicle( avoidParams );
	if ( pAvoidRoute == NULL )
	{
		// Try the opposite direction
		avoidParams.nDirection = -avoidParams.nDirection;
		pAvoidRoute = BuildNodeRouteAroundVehicle( avoidParams );
		if ( pAvoidRoute == NULL )
			return false;
	}

	// Splice this into our current node path
	GetOuter()->GetNavigator()->GetPath()->PrependWaypoints( pAvoidRoute );
	GetOuter()->GetNavigator()->SimplifyPath();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Removes all failed points
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::ResetVehicleEntryFailedState( void )
{
	m_FailedEntryPositions.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Adds a failed position to the list and marks when it occured
// Input  : &vecPosition - Position that failed
//-----------------------------------------------------------------------------
void CAI_PassengerBehaviorCompanion::MarkVehicleEntryFailed( const Vector &vecPosition )
{
	FailPosition_t failPos;
	failPos.flTime = gpGlobals->curtime;
	failPos.vecPosition = vecPosition;
	m_FailedEntryPositions.AddToTail( failPos );
}

//-----------------------------------------------------------------------------
// Purpose: See if a vector is near enough to a previously failed position
// Input  : &vecPosition - position to test
// Output : Returns true if the point is near enough another to be considered equivalent
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::PointIsWithinEntryFailureRadius( const Vector &vecPosition )
{
	// Test this point against our known failed points and reject it if it's too near
	for ( int i = 0; i < m_FailedEntryPositions.Count(); i++ )
	{
		if ( ( vecPosition - m_FailedEntryPositions[i].vecPosition ).LengthSqr() < (32.0f*32.0f) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PassengerBehaviorCompanion::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// If we've hit something we need to see what it might be
	if ( pMoveGoal->bHasTraced && pMoveGoal->directTrace.pObstruction != NULL )
	{
		// See if we're a buggy
		CPropJeepEpisodic *pBuggy = dynamic_cast<CPropJeepEpisodic *>(pMoveGoal->directTrace.pObstruction);
		if ( pBuggy != NULL )
		{
			IServerVehicle *pServerVehicle = pBuggy->GetServerVehicle();
			if ( pServerVehicle != NULL )
			{
				Vector vecGoalPos = GetNavigator()->GetPath()->ActualGoalPosition();
				float flGoalDist = ( GetAbsOrigin() - vecGoalPos ).Length();
				if ( flGoalDist > distClear )
				{
					bool bSucceeded = BuildVehicleAvoidancePath( pServerVehicle->GetVehicleEnt(), pMoveGoal->target );
					if ( bSucceeded )
					{
						ResetVehicleEntryFailedState();
						return true;
					}

					// Mark this point as having failed, so try a new entrance when we re-tried
					MarkVehicleEntryFailed( GetNavigator()->GetPath()->ActualGoalPosition() );
					return false;
				}
			}
		}
	}

	return BaseClass::OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------
// Purpose: Find the proper sequence to use (weighted by priority or distance from current position)
//			to enter the vehicle.
// Input  : bNearest - Use distance as the criteria for a "best" sequence.  Otherwise the order of the
//					   seats is their priority.
// Output : int - sequence index
//-----------------------------------------------------------------------------
int CAI_PassengerBehaviorCompanion::FindEntrySequence( bool bNearest /*= false*/ )
{
	// Get a list of all our animations
	const PassengerSeatAnims_t *pEntryAnims = m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatAnims( GetOuter(), PASSENGER_SEAT_ENTRY );
	if ( pEntryAnims == NULL )
		return -1;

	// Get the ultimate position we'll end up at
	Vector	vecStartPos, vecEndPos;
	if ( m_hVehicle->GetServerVehicle()->NPC_GetPassengerSeatPosition( GetOuter(), &vecEndPos, NULL ) == false )
		return -1;

	const CPassengerSeatTransition *pTransition;
	Vector	vecSeatDir;
	float	flNearestDist = 99999999999.9f;
	float	flSeatDist;
	int		nNearestSequence = -1;
	int		nSequence;

	// Test each animation (sorted by priority) for the best match
	for ( int i = 0; i < pEntryAnims->Count(); i++ )
	{
		// Find the activity for this animation name
		pTransition = &pEntryAnims->Element(i);
		nSequence = GetOuter()->LookupSequence( STRING( pTransition->GetAnimationName() ) );
		if ( nSequence == -1 )
			continue;

		// Test this entry for validity
		GetEntryPoint( nSequence, &vecStartPos );

		// See if this entry position is in our list of known unreachable places
		if ( PointIsWithinEntryFailureRadius( vecStartPos ) )
			continue;

		// Check to see if we can use this
		if ( IsValidTransitionPoint( vecStartPos, vecEndPos ) )
		{
			// If we're just looking for the first, we're done
			if ( bNearest == false )
				return nSequence;

			// Otherwise distance is the deciding factor
			vecSeatDir = ( vecStartPos - GetOuter()->GetAbsOrigin() );
			flSeatDist = VectorNormalize( vecSeatDir );

			// Closer, take it
			if ( flSeatDist < flNearestDist )
			{
				flNearestDist = flSeatDist;
				nNearestSequence = nSequence;
			}
		}

	}

	return nNearestSequence;
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_PassengerBehaviorCompanion )
{
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE_AIM )
	DECLARE_ACTIVITY( ACT_PASSENGER_RELOAD )
	DECLARE_ACTIVITY( ACT_PASSENGER_OVERTURNED )
	DECLARE_ACTIVITY( ACT_PASSENGER_IMPACT )
	DECLARE_ACTIVITY( ACT_PASSENGER_IMPACT_WEAPON )
	DECLARE_ACTIVITY( ACT_PASSENGER_POINT )
	DECLARE_ACTIVITY( ACT_PASSENGER_POINT_BEHIND )
	DECLARE_ACTIVITY( ACT_PASSENGER_IDLE_READY )

	DECLARE_TASK( TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT )
	DECLARE_TASK( TASK_GET_PATH_TO_NEAR_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_RELOAD )
	DECLARE_TASK( TASK_PASSENGER_EXIT_STUCK_VEHICLE )
	DECLARE_TASK( TASK_PASSENGER_OVERTURNED )
	DECLARE_TASK( TASK_PASSENGER_IMPACT )

	DECLARE_CONDITION( COND_HARD_IMPACT )
	DECLARE_CONDITION( COND_VEHICLE_MOVED_FROM_MARK )
	DECLARE_CONDITION( COND_VEHICLE_STOPPED )
	DECLARE_CONDITION( COND_CAN_LEAVE_STUCK_VEHICLE )
	DECLARE_CONDITION( COND_WARN_OVERTURNED )

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_TOLERANCE_DISTANCE		4"
		"		TASK_SET_ROUTE_SEARCH_TIME		5"
		"		TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_PASSENGER_ENTER_VEHICLE"
		""
		"	Interrupts"
		"		COND_VEHICLE_MOVED_FROM_MARK"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_TOLERANCE_DISTANCE		32"
		"		TASK_SET_ROUTE_SEARCH_TIME		3"
		"		TASK_GET_PATH_TO_NEAR_VEHICLE	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
		"		COND_VEHICLE_MOVED_FROM_MARK"
		"		COND_VEHICLE_STOPPED"
	)

	DEFINE_SCHEDULE
	( 
		SCHED_PASSENGER_ENTER_VEHICLE_PAUSE,

		"	Tasks"
		"		TASK_STOP_MOVING			1"
		"		TASK_FACE_TARGET			0"
		"		TASK_WAIT					2"
		""
		"	Interrupts"
		"		COND_VEHICLE_STOPPED"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_HEAR_DANGER"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
	)
	
	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_EXIT_STUCK_VEHICLE,

		"	Tasks"
		"		TASK_PASSENGER_EXIT_STUCK_VEHICLE		0"
		""
		"	Interrupts"
	)
	
	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_RELOAD,

		"	Tasks"
		"		TASK_PASSENGER_RELOAD		0"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_PASSENGER_OVERTURNED,

		"	Tasks"
		"		TASK_PASSENGER_OVERTURNED	0"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
	SCHED_PASSENGER_IMPACT,

	"	Tasks"
	"		TASK_PASSENGER_IMPACT	0"
	""
	"	Interrupts"
	)

	AI_END_CUSTOM_SCHEDULE_PROVIDER()
}
