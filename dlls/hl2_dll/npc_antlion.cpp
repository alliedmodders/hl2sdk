//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Antlion - nasty bug
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_moveprobe.h"
#include "ai_route.h"
#include "npcevent.h"
#include "gib.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "antlion_dust.h"
#include "engine/IEngineSound.h"
#include "globalstate.h"
#include "movevars_shared.h"
#include "te_effect_dispatch.h"
#include "vehicle_base.h"
#include "mapentities.h"
#include "antlion_maker.h"
#include "npc_antlion.h"
#include "decals.h"
#include "hl2_shareddefs.h"
#include "weapon_physcannon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Debug visualization
ConVar	g_debug_antlion( "g_debug_antlion", "0" );

ConVar	sk_antlion_health( "sk_antlion_health", "0" );
ConVar	sk_antlion_swipe_damage( "sk_antlion_swipe_damage", "0" );
ConVar	sk_antlion_jump_damage( "sk_antlion_jump_damage", "0" );
ConVar  sk_antlion_air_attack_dmg( "sk_antlion_air_attack_dmg", "0" );

ConVar  g_test_new_antlion_jump( "g_test_new_antlion_jump", "1", FCVAR_ARCHIVE );
ConVar	antlion_easycrush( "antlion_easycrush", "1" );

 
extern ConVar bugbait_radius;

int AE_ANTLION_WALK_FOOTSTEP;
int AE_ANTLION_MELEE_HIT1;
int AE_ANTLION_MELEE_HIT2;
int AE_ANTLION_MELEE_POUNCE;
int AE_ANTLION_FOOTSTEP_SOFT;
int AE_ANTLION_FOOTSTEP_HEAVY;
int AE_ANTLION_START_JUMP;
int AE_ANTLION_BURROW_IN;
int AE_ANTLION_BURROW_OUT;
int AE_ANTLION_VANISH;
int AE_ANTLION_OPEN_WINGS;
int AE_ANTLION_CLOSE_WINGS;
int AE_ANTLION_MELEE1_SOUND;
int AE_ANTLION_MELEE2_SOUND;

//Attack range definitions
#define	ANTLION_MELEE1_RANGE		100.0f
#define	ANTLION_MELEE2_RANGE		64.0f
#define	ANTLION_MELEE2_RANGE_MAX	175.0f
#define	ANTLION_MELEE2_RANGE_MIN	64.0f
#define	ANTLION_JUMP_MIN			128.0f

#define	ANTLION_JUMP_MAX_RISE		512.0f
#define	ANTLION_JUMP_MAX			1024.0f

#define	ANTLION_MIN_BUGBAIT_GOAL_TARGET_RADIUS	512

//Interaction IDs
int g_interactionAntlionFoundTarget = 0;

#define	ANTLION_MODEL	"models/antlion.mdl"

#define	ANTLION_BURROW_IN	0
#define	ANTLION_BURROW_OUT	1

#define	ANTLION_BUGBAIT_NAV_TOLERANCE	200

#define	ANTLION_OBEY_FOLLOW_TIME	5.0f


//==================================================
// AntlionSquadSlots
//==================================================

enum
{	
	SQUAD_SLOT_ANTLION_JUMP = LAST_SHARED_SQUADSLOT,
};

//==================================================
// Antlion Activities
//==================================================

int ACT_ANTLION_JUMP_START;
int	ACT_ANTLION_DISTRACT;
int ACT_ANTLION_DISTRACT_ARRIVED;
int ACT_ANTLION_BURROW_IN;
int ACT_ANTLION_BURROW_OUT;
int ACT_ANTLION_BURROW_IDLE;
int	ACT_ANTLION_RUN_AGITATED;
int ACT_ANTLION_FLIP;
int ACT_ANTLION_POUNCE;
int ACT_ANTLION_POUNCE_MOVING;
int ACT_ANTLION_DROWN;
int ACT_ANTLION_LAND;

//==================================================
// CNPC_Antlion
//==================================================

CNPC_Antlion::CNPC_Antlion( void )
{
	m_flIdleDelay	= 0.0f;
	m_flBurrowTime	= 0.0f;
	m_flJumpTime	= 0.0f;
	m_flPounceTime	= 0.0f;
	m_flObeyFollowTime = 0.0f;
	m_iUnBurrowAttempts = 0;

	m_flAlertRadius	= 256.0f;
	m_flFieldOfView	= -0.5f;

	m_bStartBurrowed	= false;
	m_bAgitatedSound	= false;
	m_bWingsOpen		= false;
	
	//FIXME: This isn't a sure bet
	m_flIgnoreSoundTime	= 0.0f;
	m_bHasHeardSound	= false;

	m_flNextAcknowledgeTime = 0.0f;
	m_flNextJumpPushTime = 0.0f;

	m_vecLastJumpAttempt.Init();
	m_vecSavedJump.Init();

	m_hFightGoalTarget = NULL;
	m_hFollowTarget = NULL;
	m_bLoopingStarted = false;

	m_bForcedStuckJump = false;
	m_nBodyBone = -1;
}

LINK_ENTITY_TO_CLASS( npc_antlion, CNPC_Antlion );

//==================================================
// CNPC_Antlion::m_DataDesc
//==================================================

BEGIN_DATADESC( CNPC_Antlion )

	DEFINE_KEYFIELD( m_bStartBurrowed,		FIELD_BOOLEAN,	"startburrowed" ),
	DEFINE_KEYFIELD( m_bIgnoreBugbait,		FIELD_BOOLEAN,	"ignorebugbait" ),
	DEFINE_KEYFIELD( m_flAlertRadius,		FIELD_FLOAT,	"radius" ),
	DEFINE_KEYFIELD( m_flEludeDistance,		FIELD_FLOAT,	"eludedist" ),

	DEFINE_FIELD( m_flIdleDelay,			FIELD_TIME ),
	DEFINE_FIELD( m_flBurrowTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flJumpTime,				FIELD_TIME ),
	DEFINE_FIELD( m_flPounceTime,			FIELD_TIME ),
	DEFINE_FIELD( m_iUnBurrowAttempts,		FIELD_INTEGER ),
	DEFINE_FIELD( m_iContext,				FIELD_INTEGER ),
	DEFINE_FIELD( m_vecSavedJump,			FIELD_VECTOR ),
	DEFINE_FIELD( m_vecLastJumpAttempt,		FIELD_VECTOR ),
	DEFINE_FIELD( m_flIgnoreSoundTime,		FIELD_TIME ),
	DEFINE_FIELD( m_vecHeardSound,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_bHasHeardSound,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAgitatedSound,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWingsOpen,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextAcknowledgeTime,	FIELD_TIME ),
	DEFINE_FIELD( m_hFollowTarget,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hFightGoalTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_strParentSpawner,		FIELD_STRING ),
	DEFINE_FIELD( m_flSuppressFollowTime,	FIELD_FLOAT ),
	DEFINE_FIELD( m_MoveState,				FIELD_INTEGER ),
	DEFINE_FIELD( m_flObeyFollowTime,		FIELD_TIME ),
	DEFINE_FIELD( m_bLeapAttack,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDisableJump,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeDrown,			FIELD_TIME ),
	DEFINE_FIELD( m_flNextJumpPushTime,		FIELD_TIME ),
	DEFINE_FIELD( m_bForcedStuckJump,		FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_bLoopingStarted, FIELD_BOOLEAN ),
	//			  m_FollowBehavior
	//			  m_AssaultBehavior
	
	DEFINE_INPUTFUNC( FIELD_VOID,	"Unburrow", InputUnburrow ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Burrow",	InputBurrow ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"BurrowAway",	InputBurrowAway ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"FightToPosition", InputFightToPosition ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"StopFightToPosition", InputStopFightToPosition ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableJump", InputEnableJump ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableJump", InputDisableJump ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"IgnoreBugbait", InputIgnoreBugbait ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"HearBugbait", InputHearBugbait ),

	DEFINE_OUTPUT( m_OnReachFightGoal, "OnReachedFightGoal" ),

	// Function Pointers
	DEFINE_ENTITYFUNC( Touch ),
	DEFINE_USEFUNC( BurrowUse ),

	// DEFINE_FIELD( FIELD_SHORT, m_hFootstep ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Spawn( void )
{
	Precache();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	SetModel( ANTLION_MODEL );

	SetHullType(HULL_MEDIUM);
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetNavType( NAV_GROUND );

	SetBloodColor( BLOOD_COLOR_YELLOW );

	m_NPCState	= NPC_STATE_NONE;
	m_iHealth	= sk_antlion_health.GetFloat();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_STEP );

	//Only do this if a squadname appears in the entity
	if ( m_SquadName != NULL_STRING )
	{
		CapabilitiesAdd( bits_CAP_SQUAD );
	}

	SetCollisionGroup( HL2COLLISION_GROUP_ANTLION );

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK2 );
	
	// JAY: Optimize these out for now
	if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == false )
		 CapabilitiesAdd( bits_CAP_SKIP_NAV_GROUND_CHECK );

	NPCInit();

	// Antlions will always pursue
	m_flDistTooFar = 9999999.0f;

	m_bDisableJump = false;

	//See if we're supposed to start burrowed
	if ( m_bStartBurrowed )
	{
		AddEffects( EF_NODRAW );
		AddFlag( FL_NOTARGET );
		m_spawnflags |= SF_NPC_GAG;
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage	= DAMAGE_NO;

		SetState( NPC_STATE_IDLE );
		SetActivity( (Activity) ACT_ANTLION_BURROW_IDLE );
		SetSchedule( SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER );

		SetUse( &CNPC_Antlion::BurrowUse );
	}

	BaseClass::Spawn();

	m_nSkin = random->RandomInt( 0, ANTLION_SKIN_COUNT-1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Activate( void )
{
	// If we're friendly to the player, setup a relationship to reflect it
	if ( IsAllied() )
	{
		// Handle all clients
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer != NULL )
			{
				AddEntityRelationship( pPlayer, D_LI, 99 );
			}
		}
	}

	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose: override this to simplify the physics shadow of the antlions
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CreateVPhysics()
{
	bool bRet = BaseClass::CreateVPhysics();
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Precache( void )
{
	PrecacheModel( ANTLION_MODEL );

// XBox doesn't use the smaller gibs, so don't cache them
#ifdef _XBOX
	PrecacheModel( "models/gibs/antlion_gib_large_2.mdl" );		// Head
	PrecacheModel( "models/gibs/antlion_gib_medium_1.mdl" );	// Pinchers
	PrecacheModel( "models/gibs/antlion_gib_medium_2.mdl" );	// Leg
#else
	PrecacheModel( "models/gibs/antlion_gib_large_1.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_large_2.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_large_3.mdl" );

	PrecacheModel( "models/gibs/antlion_gib_medium_1.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_medium_2.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_medium_3.mdl" );

	PrecacheModel( "models/gibs/antlion_gib_small_1.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_small_2.mdl" );
	PrecacheModel( "models/gibs/antlion_gib_small_3.mdl" );
#endif

	PrecacheScriptSound( "NPC_Antlion.RunOverByVehicle" );
	PrecacheScriptSound( "NPC_Antlion.MeleeAttack" );
	m_hFootstep = PrecacheScriptSound( "NPC_Antlion.Footstep" );
	PrecacheScriptSound( "NPC_Antlion.BurrowIn" );
	PrecacheScriptSound( "NPC_Antlion.BurrowOut" );
	PrecacheScriptSound( "NPC_Antlion.FootstepSoft" );
	PrecacheScriptSound( "NPC_Antlion.FootstepHeavy" );
	PrecacheScriptSound( "NPC_Antlion.MeleeAttackSingle" );
	PrecacheScriptSound( "NPC_Antlion.MeleeAttackDouble" );
	PrecacheScriptSound( "NPC_Antlion.Distracted" );
	PrecacheScriptSound( "NPC_Antlion.Idle" );
	PrecacheScriptSound( "NPC_Antlion.Pain" );
	PrecacheScriptSound( "NPC_Antlion.Land" );
	PrecacheScriptSound( "NPC_Antlion.WingsOpen" );
	PrecacheScriptSound( "NPC_Antlion.LoopingAgitated" );
	PrecacheScriptSound( "NPC_Antlion.Distracted" );

#ifdef HL2_EPISODIC
	PrecacheScriptSound( "NPC_Antlion.MeleeAttack_Muffled" );
	PrecacheScriptSound( "NPC_Antlion.TrappedMetal" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline CBaseEntity *CNPC_Antlion::EntityToWatch( void )
{
	return ( m_hFollowTarget != NULL ) ? m_hFollowTarget.Get() : GetEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::UpdateHead( void )
{
	float yaw = GetPoseParameter( "head_yaw" );
	float pitch = GetPoseParameter( "head_pitch" );

	CBaseEntity *pTarget = EntityToWatch();

	if ( pTarget != NULL )
	{
		Vector	enemyDir = pTarget->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( enemyDir );
		
		if ( DotProduct( enemyDir, BodyDirection3D() ) < 0.0f )
		{
			SetPoseParameter( "head_yaw",	UTIL_Approach( 0, yaw, 10 ) );
			SetPoseParameter( "head_pitch", UTIL_Approach( 0, pitch, 10 ) );
			
			return;
		}

		float facingYaw = VecToYaw( BodyDirection3D() );
		float yawDiff = VecToYaw( enemyDir );
		yawDiff = UTIL_AngleDiff( yawDiff, facingYaw + yaw );

		float facingPitch = UTIL_VecToPitch( BodyDirection3D() );
		float pitchDiff = UTIL_VecToPitch( enemyDir );
		pitchDiff = UTIL_AngleDiff( pitchDiff, facingPitch + pitch );

		SetPoseParameter( "head_yaw", UTIL_Approach( yaw + yawDiff, yaw, 50 ) );
		SetPoseParameter( "head_pitch", UTIL_Approach( pitch + pitchDiff, pitch, 50 ) );
	}
	else
	{
		SetPoseParameter( "head_yaw",	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( "head_pitch", UTIL_Approach( 0, pitch, 10 ) );
	}
}

#define	ANTLION_VIEW_FIELD_NARROW	0.85f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FInViewCone( CBaseEntity *pEntity )
{
	m_flFieldOfView = ( GetEnemy() != NULL ) ? ANTLION_VIEW_FIELD_NARROW : VIEW_FIELD_WIDE;

	return BaseClass::FInViewCone( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecSpot - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FInViewCone( const Vector &vecSpot )
{
	m_flFieldOfView = ( GetEnemy() != NULL ) ? ANTLION_VIEW_FIELD_NARROW : VIEW_FIELD_WIDE;

	return BaseClass::FInViewCone( vecSpot );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Event_Killed( const CTakeDamageInfo &info )
{
	//Turn off wings
	SetWings( false );
	VacateStrategySlot();

	if ( IsCurSchedule(SCHED_ANTLION_BURROW_IN) || IsCurSchedule(SCHED_ANTLION_BURROW_OUT) )
	{
		AddEFlags( EF_NOSHADOW );
	}

	if ( info.GetDamageType() & DMG_CRUSH )
	{
		CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );
	}

	BaseClass::Event_Killed( info );

	CBaseEntity *pAttacker = info.GetInflictor();

	if ( pAttacker && pAttacker->GetServerVehicle() && ShouldGib( info ) == true )
	{
		trace_t tr;
		UTIL_TraceLine( GetAbsOrigin() + Vector( 0, 0, 64 ), pAttacker->GetAbsOrigin(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		UTIL_DecalTrace( &tr, "Antlion.Splat" );

		SpawnBlood( GetAbsOrigin(), g_vecAttackDir, BloodColor(), info.GetDamage() );

		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), "NPC_Antlion.RunOverByVehicle" );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input: 
//==================================================

void CNPC_Antlion::MeleeAttack( float distance, float damage, QAngle &viewPunch, Vector &shove )
{
	Vector vecForceDir;

	// Always hurt bullseyes for now
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->Classify() == CLASS_BULLSEYE ) )
	{
		vecForceDir = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
		CTakeDamageInfo info( this, this, damage, DMG_SLASH );
		CalculateMeleeDamageForce( &info, vecForceDir, GetEnemy()->GetAbsOrigin() );
		GetEnemy()->TakeDamage( info );
		return;
	}

	CBaseEntity *pHurt = CheckTraceHullAttack( distance, -Vector(16,16,32), Vector(16,16,32), damage, DMG_SLASH, 5.0f );

	if ( pHurt )
	{
		vecForceDir = ( pHurt->WorldSpaceCenter() - WorldSpaceCenter() );

		//FIXME: Until the interaction is setup, kill combine soldiers in one hit -- jdw
		if ( FClassnameIs( pHurt, "npc_combine_s" ) )
		{
			CTakeDamageInfo	dmgInfo( this, this, pHurt->m_iHealth+25, DMG_SLASH );
			CalculateMeleeDamageForce( &dmgInfo, vecForceDir, pHurt->GetAbsOrigin() );
			pHurt->TakeDamage( dmgInfo );
			return;
		}

		CBasePlayer *pPlayer = ToBasePlayer( pHurt );

		if ( pPlayer != NULL )
		{
			//Kick the player angles
			if ( !(pPlayer->GetFlags() & FL_GODMODE ) && pPlayer->GetMoveType() != MOVETYPE_NOCLIP )
			{
				pPlayer->ViewPunch( viewPunch );

				Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize(dir);

				QAngle angles;
				VectorAngles( dir, angles );
				Vector forward, right;
				AngleVectors( angles, &forward, &right, NULL );

				//Push the target back
				pHurt->ApplyAbsVelocityImpulse( - right * shove[1] - forward * shove[0] );
			}
		}

		// Play a random attack hit sound
		EmitSound( "NPC_Antlion.MeleeAttack" );
	}
}

// Number of times the antlions will attempt to generate a random chase position
#define NUM_CHASE_POSITION_ATTEMPTS		3

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &targetPos - 
//			&result - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FindChasePosition( const Vector &targetPos, Vector &result )
{
	if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == true )
	{
		 result = targetPos;
		 return true;
	}

	Vector runDir = ( targetPos - GetAbsOrigin() );
	VectorNormalize( runDir );
	
	Vector	vRight, vUp;
	VectorVectors( runDir, vRight, vUp );

	for ( int i = 0; i < NUM_CHASE_POSITION_ATTEMPTS; i++ )
	{
		result	= targetPos;
		result += -runDir * random->RandomInt( 64, 128 );
		result += vRight * random->RandomInt( -128, 128 );
		
		//FIXME: We need to do a more robust search here
		// Find a ground position and try to get there
		if ( GetGroundPosition( result, result ) )
			return true;
	}
	
	//TODO: If we're making multiple inquiries to this, make sure it's evenly spread

	if ( g_debug_antlion.GetInt() == 1 )
	{
		NDebugOverlay::Cross3D( result, -Vector(32,32,32), Vector(32,32,32), 255, 255, 0, true, 2.0f );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &testPos - 
//-----------------------------------------------------------------------------
bool CNPC_Antlion::GetGroundPosition( const Vector &testPos, Vector &result )
{
	// Trace up to clear the ground
	trace_t	tr;
	AI_TraceHull( testPos, testPos + Vector( 0, 0, 64 ), NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// If we're stuck in solid, this can't be valid
	if ( tr.allsolid )
	{
		if ( g_debug_antlion.GetInt() == 3 )
		{
			NDebugOverlay::BoxDirection( testPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ) + Vector( 0, 0, 128 ), Vector( 0, 0, 1 ), 255, 0, 0, true, 2.0f );
		}

		return false;
	}

	if ( g_debug_antlion.GetInt() == 3 )
	{
		NDebugOverlay::BoxDirection( testPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ) + Vector( 0, 0, 128 ), Vector( 0, 0, 1 ), 0, 255, 0, true, 2.0f );
	}

	// Trace down to find the ground
	AI_TraceHull( tr.endpos, tr.endpos - Vector( 0, 0, 128 ), NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	if ( g_debug_antlion.GetInt() == 3 )
	{
		NDebugOverlay::BoxDirection( tr.endpos, NAI_Hull::Mins( GetHullType() ) - Vector( 0, 0, 256 ), NAI_Hull::Maxs( GetHullType() ), Vector( 0, 0, 1 ), 255, 255, 0, true, 2.0f );
	}

	// We must end up on the floor with this trace
	if ( tr.fraction < 1.0f )
	{
		if ( g_debug_antlion.GetInt() == 3 )
		{
			NDebugOverlay::Cross3D( tr.endpos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), 255, 0, 0, true, 2.0f );
		}

		result = tr.endpos;
		return true;
	}

	// Ended up in open space
	return false;
}
void CNPC_Antlion::ManageFleeCapabilities( bool bEnable )
{
	if ( bEnable == false )
	{
		//Remove the jump capabilty when we build our route.
		//We'll enable it back again after the route has been built.
		CapabilitiesRemove( bits_CAP_MOVE_JUMP );

		if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == false  )
			 CapabilitiesRemove( bits_CAP_SKIP_NAV_GROUND_CHECK );
	}
	else
	{
		if ( m_bDisableJump == false )
			 CapabilitiesAdd( bits_CAP_MOVE_JUMP );

		if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == false  )
			 CapabilitiesAdd( bits_CAP_SKIP_NAV_GROUND_CHECK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : soundType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::GetPathToSoundFleePoint( int soundType )
{
	CSound *pSound = GetLoudestSoundOfType( soundType );

	if ( pSound == NULL  )
	{
		//NOTENOTE: If you're here, there's a disparity between Listen() and GetLoudestSoundOfType() - jdw
		TaskFail( "Unable to find thumper sound!" );
		return false;
	}

	ManageFleeCapabilities( false );

	//Try and find a hint-node first
	CHintCriteria	hintCriteria;

	hintCriteria.SetHintType( HINT_ANTLION_THUMPER_FLEE_POINT );
	hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
	hintCriteria.AddIncludePosition( WorldSpaceCenter(), 2500 );

	CAI_Hint *pHint = CAI_HintManager::FindHint( WorldSpaceCenter(), hintCriteria );

	Vector vecFleeGoal;
	Vector vecSoundPos = pSound->GetSoundOrigin();

	// Put the sound location on the same plane as the antlion.
	vecSoundPos.z = GetAbsOrigin().z;

	Vector vecFleeDir = GetAbsOrigin() - vecSoundPos;
	VectorNormalize( vecFleeDir );

	if ( pHint != NULL )
	{
		// Get our goal position
		pHint->GetPosition( this, &vecFleeGoal );

		// Find a route to that position
		AI_NavGoal_t goal( vecFleeGoal, (Activity) ACT_ANTLION_RUN_AGITATED, 128, AIN_DEF_FLAGS );

		if ( GetNavigator()->SetGoal( goal ) )
		{
			pHint->Lock( this );
			pHint->Unlock( 2.0f );

			GetNavigator()->SetArrivalActivity( (Activity) ACT_ANTLION_DISTRACT_ARRIVED );
			GetNavigator()->SetArrivalDirection( -vecFleeDir );

			ManageFleeCapabilities( true );
			return true;
		}
	}

	//Make us offset this a little at least
	float flFleeYaw = VecToYaw( vecFleeDir ) + random->RandomInt( -20, 20 );

	vecFleeDir = UTIL_YawToVector( flFleeYaw );

	// Move us to the outer radius of the noise (with some randomness)
	vecFleeGoal = vecSoundPos + vecFleeDir * ( pSound->Volume() + random->RandomInt( 32, 64 ) );

	// Find a route to that position
	AI_NavGoal_t goal( vecFleeGoal + Vector( 0, 0, 8 ), (Activity) ACT_ANTLION_RUN_AGITATED, 512, AIN_DEF_FLAGS );

	if ( GetNavigator()->SetGoal( goal ) )
	{
		GetNavigator()->SetArrivalActivity( (Activity) ACT_ANTLION_DISTRACT_ARRIVED );
		GetNavigator()->SetArrivalDirection( -vecFleeDir );

		ManageFleeCapabilities( true );
		return true;
	}

	ManageFleeCapabilities( true );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_ANTLION_WALK_FOOTSTEP )
	{
		MakeAIFootstepSound( 240.0f );
		EmitSound( "NPC_Antlion.Footstep", m_hFootstep, pEvent->eventtime );
		return;
	}

	if ( pEvent->event == AE_ANTLION_MELEE_HIT1 )
	{
		MeleeAttack( ANTLION_MELEE1_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 20.0f, 0.0f, -12.0f ), Vector( -250.0f, 1.0f, 1.0f ) );
		return;
	}

	if ( pEvent->event == AE_ANTLION_MELEE_HIT2 )
	{
		MeleeAttack( ANTLION_MELEE1_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 20.0f, 0.0f, 0.0f ), Vector( -350.0f, 1.0f, 1.0f ) );
		return;
	}

	if ( pEvent->event == AE_ANTLION_MELEE_POUNCE )
	{
		MeleeAttack( ANTLION_MELEE2_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 4.0f, 0.0f, 0.0f ), Vector( -250.0f, 1.0f, 1.0f ) );
		return;
	}
		
	if ( pEvent->event == AE_ANTLION_OPEN_WINGS )
	{
		SetWings( true );
		return;
	}

	if ( pEvent->event == AE_ANTLION_CLOSE_WINGS )
	{
		SetWings( false );
		return;
	}

	if ( pEvent->event == AE_ANTLION_VANISH )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage	= DAMAGE_NO;
		AddEffects( EF_NODRAW );
		SetWings( false );

		return;
	}

	if ( pEvent->event == AE_ANTLION_BURROW_IN )
	{
		//Burrowing sound
		EmitSound( "NPC_Antlion.BurrowIn" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		CreateDust();

		if ( GetHintNode() )
		{
			GetHintNode()->Unlock( 2.0f );
		}

		return;
	}

	if ( pEvent->event == AE_ANTLION_BURROW_OUT )
	{
		EmitSound( "NPC_Antlion.BurrowOut" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		CreateDust();

		RemoveEffects( EF_NODRAW );
		RemoveFlag( FL_NOTARGET );

		return;
	}

	if ( pEvent->event == AE_ANTLION_FOOTSTEP_SOFT )
	{
		EmitSound( "NPC_Antlion.FootstepSoft", pEvent->eventtime );
		return;
	}

	if ( pEvent->event == AE_ANTLION_FOOTSTEP_HEAVY )
	{
		EmitSound( "NPC_Antlion.FootstepHeavy", pEvent->eventtime );
		return;
	}
	
	
	if ( pEvent->event == AE_ANTLION_MELEE1_SOUND )
	{
		EmitSound( "NPC_Antlion.MeleeAttackSingle" );
		return;
	}
	
	if ( pEvent->event == AE_ANTLION_MELEE2_SOUND )
	{
		EmitSound( "NPC_Antlion.MeleeAttackDouble" );
		return;
	}

	if ( pEvent->event == AE_ANTLION_START_JUMP )
	{
		StartJump();
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

bool CNPC_Antlion::IsUnusableNode(int iNodeID, CAI_Hint *pHint)
{
	bool iBaseReturn = BaseClass::IsUnusableNode( iNodeID, pHint );

	if ( g_test_new_antlion_jump.GetBool() == 0 )
		 return iBaseReturn;

	CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( iNodeID );

	if ( pNode )
	{
		if ( pNode->IsLocked() )
			 return true;
	}

	return iBaseReturn;
}

void CNPC_Antlion::LockJumpNode( void )
{
	if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == false )
		 return;
	
	if ( GetNavigator()->GetPath() == NULL )
		 return;

	if ( g_test_new_antlion_jump.GetBool() == false )
		 return;

	AI_Waypoint_t *pWaypoint = GetNavigator()->GetPath()->GetCurWaypoint();

	while ( pWaypoint )
	{
		AI_Waypoint_t *pNextWaypoint = pWaypoint->GetNext();
		if ( pNextWaypoint && pNextWaypoint->NavType() == NAV_JUMP && pWaypoint->iNodeID != NO_NODE )
		{
			CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( pWaypoint->iNodeID );

			if ( pNode )
			{
				//NDebugOverlay::Box( pNode->GetOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 255, 0, 0, 0, 2 );
				pNode->Lock( 0.5f );
				break;
			}
		}
		else
		{
			pWaypoint = pWaypoint->GetNext();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Antlion::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	bool iBaseReturn = BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );

	if ( g_test_new_antlion_jump.GetBool() == false )
		 return iBaseReturn;

	if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) == false )
		 return iBaseReturn;

	CAI_BaseNPC *pBlocker = pMoveGoal->directTrace.pObstruction->MyNPCPointer();

	if ( pBlocker && pBlocker->Classify() == CLASS_ANTLION )
	{
		// HACKHACK
		CNPC_Antlion *pAntlion = dynamic_cast< CNPC_Antlion * > ( pBlocker );

		if ( pAntlion )
		{
			if ( pAntlion->AllowedToBePushed() == true && GetEnemy() == NULL )
			{
				//NDebugOverlay::Box( pAntlion->GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 2 );
				pAntlion->GetMotor()->SetIdealYawToTarget( WorldSpaceCenter() );
				pAntlion->SetSchedule( SCHED_MOVE_AWAY );
				pAntlion->m_flNextJumpPushTime = gpGlobals->curtime + 2.0f;
			}
		}
	}

	return iBaseReturn;
}

bool NPC_Antlion_IsAntlion( CBaseEntity *pEntity )
{
	CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion *>(pEntity);

	return pAntlion ? true : false;
}

class CTraceFilterAntlion : public CTraceFilterEntitiesOnly
{
public:
	CTraceFilterAntlion( const CBaseEntity *pEntity ) { m_pIgnore = pEntity; }

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( m_pIgnore == pEntity )
			 return false;
		
		if ( pEntity->IsNPC() == false )
			 return false;
		
		if ( NPC_Antlion_IsAntlion( pEntity ) )
			 return true;
		
		return false;
	}
private:
	
	const CBaseEntity		*m_pIgnore;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
void CNPC_Antlion::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask ) 
	{
	case TASK_ANNOUNCE_ATTACK:
		{
			EmitSound( "NPC_Antlion.MeleeAttackSingle" );
			TaskComplete();
			break;
		}

	case TASK_ANTLION_FACE_JUMP:
		break;

	case TASK_ANTLION_DROWN:
	{
		// Set the gravity really low here! Sink slowly
		SetGravity( 0 );
		SetAbsVelocity( vec3_origin );
		m_flTimeDrown = gpGlobals->curtime + 4;
		break;
	}

	case TASK_ANTLION_REACH_FIGHT_GOAL:

		m_OnReachFightGoal.FireOutput( this, this );
		TaskComplete();
		break;

	case TASK_ANTLION_DISMOUNT_NPC:
		{
			CBaseEntity *pGroundEnt = GetGroundEntity();
			
			if( pGroundEnt != NULL )
			{
				trace_t trace;
				CTraceFilterAntlion traceFilter( this );
				AI_TraceHull( GetAbsOrigin(), GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, &traceFilter, &trace );

				if ( trace.m_pEnt )
				{
					OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth+1, DMG_GENERIC ) );
					return;
				}

				// Jump behind the other NPC so I don't block their path.
				Vector vecJumpDir; 

				pGroundEnt->GetVectors( &vecJumpDir, NULL, NULL );

				SetGroundEntity( NULL );
				
				// Bump up
				UTIL_SetOrigin( this, GetAbsOrigin() + Vector( 0, 0 , 1 ) );
				
				SetAbsVelocity( vecJumpDir * -200 + Vector( 0, 0, 100 ) );

				SetActivity( (Activity) ACT_ANTLION_FLIP );
			}
			else
			{
				// Dead or gone now
				TaskComplete();
			}
		}

		break;

	case TASK_ANTLION_FACE_BUGBAIT:
			
		//Must have a saved sound
		//FIXME: This isn't assured to be still pointing to the right place, need to protect this
		if ( !m_bHasHeardSound )
		{
			TaskFail( "No remembered bug bait sound to run to!" );
			return;
		}

		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecHeardSound );
		SetTurnActivity();

		break;
	
	case TASK_ANTLION_GET_PATH_TO_BUGBAIT:
		{
			//Must have a saved sound
			//FIXME: This isn't assured to be still pointing to the right place, need to protect this
			if ( !m_bHasHeardSound )
			{
				TaskFail( "No remembered bug bait sound to run to!" );
				return;
			}
			
			Vector	goalPos;

			// Find the position to chase to
			if ( FindChasePosition( m_vecHeardSound, goalPos ) )
			{
				AI_NavGoal_t goal( goalPos, (Activity) ACT_ANTLION_RUN_AGITATED, ANTLION_BUGBAIT_NAV_TOLERANCE );
				
				//Try to run directly there
				if ( GetNavigator()->SetGoal( goal, AIN_DISCARD_IF_FAIL ) == false )
				{
					//Try and get as close as possible otherwise
					AI_NavGoal_t nearGoal( GOALTYPE_LOCATION_NEAREST_NODE, goalPos, (Activity) ACT_ANTLION_RUN_AGITATED, ANTLION_BUGBAIT_NAV_TOLERANCE );

					if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
					{
						//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
						ClearCondition( COND_TASK_FAILED );

						LockJumpNode();
						TaskComplete();
						return;
					}
					else
					{
						TaskFail( "Antlion failed to find path to bugbait position\n" );
						return;
					}
				}
				else
				{
					LockJumpNode();
					TaskComplete();
					return;
				}
			}

			TaskFail( "Antlion failed to find path to bugbait position\n" );
			break;
		}

	case TASK_ANTLION_WAIT_FOR_TRIGGER:
		m_flIdleDelay = gpGlobals->curtime + 1.0f;

		break;

	case TASK_ANTLION_JUMP:
		
		if ( CheckLanding() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_CHECK_FOR_UNBORROW:
		
		m_iUnBurrowAttempts = 0;

		if ( ValidBurrowPoint( GetAbsOrigin() ) )
		{
			m_spawnflags &= ~SF_NPC_GAG;
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			TaskComplete();
		}

		break;

	case TASK_ANTLION_BURROW_WAIT:
		
		if ( pTask->flTaskData == 1.0f )
		{
			//Set our next burrow time
			m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 1, 6 );
		}

		break;

	case TASK_ANTLION_FIND_BURROW_IN_POINT:
		
		if ( FindBurrow( GetAbsOrigin(), pTask->flTaskData, ANTLION_BURROW_IN ) == false )
		{
			TaskFail( "TASK_ANTLION_FIND_BURROW_IN_POINT: Unable to find burrow in position\n" );
		}
		else
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_FIND_BURROW_OUT_POINT:
		
		if ( FindBurrow( GetAbsOrigin(), pTask->flTaskData, ANTLION_BURROW_OUT ) == false )
		{
			TaskFail( "TASK_ANTLION_FIND_BURROW_OUT_POINT: Unable to find burrow out position\n" );
		}
		else
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_BURROW:
		Burrow();
		TaskComplete();

		break;

	case TASK_ANTLION_UNBURROW:
		Unburrow();
		TaskComplete();

		break;

	case TASK_ANTLION_VANISH:
		AddEffects( EF_NODRAW );
		AddFlag( FL_NOTARGET );
		m_spawnflags |= SF_NPC_GAG;
		
		// If the task parameter is non-zero, remove us when we vanish
		if ( pTask->flTaskData )
		{
			CBaseEntity *pOwner = GetOwnerEntity();
			
			if( pOwner != NULL )
			{
				pOwner->DeathNotice( this );
				SetOwnerEntity( NULL );
			}

			// NOTE: We can't UTIL_Remove here, because we're in the middle of running our AI, and
			//		 we'll crash later in the bowels of the AI. Remove ourselves next frame.
			SetThink( &CNPC_Antlion::SUB_Remove );
			SetNextThink( gpGlobals->curtime + 0.1 );
		}

		TaskComplete();

		break;

	case TASK_ANTLION_GET_THUMPER_ESCAPE_PATH:
		{
			if ( GetPathToSoundFleePoint( SOUND_THUMPER ) )			
			{
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_REACHABLE_NODE );
			}
		}
		
		break;

	case TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH:
		{
			if ( GetPathToSoundFleePoint( SOUND_PHYSICS_DANGER ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_REACHABLE_NODE );
			}
		}
		
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLION_FACE_JUMP:
		{
			Vector	jumpDir = m_vecSavedJump;
			VectorNormalize( jumpDir );
			
			QAngle	jumpAngles;
			VectorAngles( jumpDir, jumpAngles );

			GetMotor()->SetIdealYawAndUpdate( jumpAngles[YAW], AI_KEEP_YAW_SPEED );
			SetTurnActivity();
			
			if ( GetMotor()->DeltaIdealYaw() < 2 )
			{
				TaskComplete();
			}
		}

		break;

	case TASK_ANTLION_DROWN:
		if( gpGlobals->curtime > m_flTimeDrown )
		{
			OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth+1, DMG_DROWN ) );
			TaskComplete();
		}
		break;

	case TASK_ANTLION_REACH_FIGHT_GOAL:
		break;

	case TASK_ANTLION_DISMOUNT_NPC:
		
		if ( GetFlags() & FL_ONGROUND )
		{
			CBaseEntity *pGroundEnt = GetGroundEntity();

			if ( ( pGroundEnt != NULL ) && ( ( pGroundEnt->MyNPCPointer() != NULL ) || pGroundEnt->GetSolidFlags() & FSOLID_NOT_STANDABLE ) )
			{
				// Jump behind the other NPC so I don't block their path.
				Vector vecJumpDir; 

				pGroundEnt->GetVectors( &vecJumpDir, NULL, NULL );

				SetGroundEntity( NULL );	
				
				// Bump up
				UTIL_SetOrigin( this, GetAbsOrigin() + Vector( 0, 0 , 1 ) );
				
				SetAbsVelocity( vecJumpDir * -200 + Vector( 0, 0, 100 ) );

				SetActivity( (Activity) ACT_ANTLION_FLIP );
			}
			else if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}
		
		break;

	case TASK_ANTLION_FACE_BUGBAIT:
			
		//Must have a saved sound
		//FIXME: This isn't assured to be still pointing to the right place, need to protect this
		if ( !m_bHasHeardSound )
		{
			TaskFail( "No remembered bug bait sound to run to!" );
			return;
		}

		GetMotor()->SetIdealYawToTargetAndUpdate( m_vecHeardSound );

		if ( FacingIdeal() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_WAIT_FOR_TRIGGER:
		
		if ( ( m_flIdleDelay > gpGlobals->curtime ) || GetEntityName() != NULL_STRING )
			return;

		TaskComplete();

		break;

	case TASK_ANTLION_JUMP:

		if ( CheckLanding() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLION_CHECK_FOR_UNBORROW:
		
		//Must wait for our next check time
		if ( m_flBurrowTime > gpGlobals->curtime )
			return;

		//See if we can pop up
		if ( ValidBurrowPoint( GetAbsOrigin() ) )
		{
			m_spawnflags &= ~SF_NPC_GAG;
			RemoveSolidFlags( FSOLID_NOT_SOLID );

			TaskComplete();
			return;
		}

		//Try again in a couple of seconds
		m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
		m_iUnBurrowAttempts++;

		// Robin: If we fail 10 times, kill ourself.
		// This deals with issues where the game relies out antlion spawners
		// firing their OnBlocked output, but the spawner isn't attempting to 
		// spawn because it has multiple live children lying around stuck under
		// physics props unable to unburrow.
		if ( m_iUnBurrowAttempts >= 10 )
		{
			m_takedamage = DAMAGE_YES;
			OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth+1, DMG_GENERIC ) );
		}

		break;

	case TASK_ANTLION_BURROW_WAIT:
		
		//See if enough time has passed
		if ( m_flBurrowTime < gpGlobals->curtime )
		{
			TaskComplete();
		}
		
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

bool CNPC_Antlion::AllowedToBePushed( void )
{
	if ( IsCurSchedule( SCHED_ANTLION_BURROW_WAIT ) || 
		IsCurSchedule(SCHED_ANTLION_BURROW_IN) || 
		IsCurSchedule(SCHED_ANTLION_BURROW_OUT) ||
		IsCurSchedule(SCHED_ANTLION_BURROW_AWAY ) ||
		IsCurSchedule( SCHED_ANTLION_RUN_TO_FIGHT_GOAL ) )
		return false;

	if ( IsRunningDynamicInteraction() )
		return false;

	if ( IsMoving() == false && IsCurSchedule( SCHED_ANTLION_FLIP ) == false
		 && GetNavType() != NAV_JUMP && m_flNextJumpPushTime <= gpGlobals->curtime )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	const float MAX_JUMP_RISE		= 512;
	const float MAX_JUMP_DROP		= 512;
	const float MAX_JUMP_DISTANCE	= 1024;
	const float MIN_JUMP_DISTANCE   = 128;

	if ( CAntlionRepellant::IsPositionRepellantFree( endPos ) == false )
		 return false;
	
	//Adrian: Don't try to jump if my destination is right next to me.
	if ( ( endPos - GetAbsOrigin()).Length() < MIN_JUMP_DISTANCE ) 
		 return false;

	if ( HasSpawnFlags( SF_ANTLION_USE_GROUNDCHECKS ) && g_test_new_antlion_jump.GetBool() == true )
	{
		trace_t	tr;
		AI_TraceHull( endPos, endPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
		
		if ( tr.m_pEnt )
		{
			CAI_BaseNPC *pBlocker = tr.m_pEnt->MyNPCPointer();

			if ( pBlocker && pBlocker->Classify() == CLASS_ANTLION )
			{
				// HACKHACK
				CNPC_Antlion *pAntlion = dynamic_cast< CNPC_Antlion * > ( pBlocker );

				if ( pAntlion )
				{
					if ( pAntlion->AllowedToBePushed() == true )
					{
					//	NDebugOverlay::Line( GetAbsOrigin(), endPos, 255, 0, 0, 0, 2 );
					//	NDebugOverlay::Box( pAntlion->GetAbsOrigin(), GetHullMins(), GetHullMaxs(), 0, 0, 255, 0, 2 );
						pAntlion->GetMotor()->SetIdealYawToTarget( endPos );
						pAntlion->SetSchedule( SCHED_MOVE_AWAY );
						pAntlion->m_flNextJumpPushTime = gpGlobals->curtime + 2.0f;
					}
				}
			}
		}
	}

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE );
}

bool CNPC_Antlion::IsFirmlyOnGround( void )
{
	if( !( GetFlags()&FL_ONGROUND ) )
		return false;

	trace_t tr;

	float flHeight =  fabs( GetHullMaxs().z - GetHullMins().z );
	
	Vector vOrigin = GetAbsOrigin() + Vector( GetHullMins().x, GetHullMins().y, 0 );
//	NDebugOverlay::Line( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), 255, 0, 0, true, 5 );
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), MASK_NPCSOLID, this, GetCollisionGroup(), &tr );

	if ( tr.fraction != 1.0f )
		 return true;
	
	vOrigin = GetAbsOrigin() - Vector( GetHullMins().x, GetHullMins().y, 0 );
//	NDebugOverlay::Line( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), 255, 0, 0, true, 5 );
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), MASK_NPCSOLID, this, GetCollisionGroup(), &tr );

	if ( tr.fraction != 1.0f )
		 return true;

	vOrigin = GetAbsOrigin() + Vector( GetHullMins().x, -GetHullMins().y, 0 );
//	NDebugOverlay::Line( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), 255, 0, 0, true, 5 );
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), MASK_NPCSOLID, this, GetCollisionGroup(), &tr );

	if ( tr.fraction != 1.0f )
		 return true;

	vOrigin = GetAbsOrigin() + Vector( -GetHullMins().x, GetHullMins().y, 0 );
//	NDebugOverlay::Line( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), 255, 0, 0, true, 5 );
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, flHeight * 0.5  ), MASK_NPCSOLID, this, GetCollisionGroup(), &tr );

	if ( tr.fraction != 1.0f )
		 return true;
	
	return false;
}

int CNPC_Antlion::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( m_FollowBehavior.GetNumFailedFollowAttempts() >= 2 )
	{
		if( IsFirmlyOnGround() == false )
		{
			Vector vecJumpDir; 
				
			vecJumpDir.z = 0;
			vecJumpDir.x = 0;
			vecJumpDir.y = 0;
			
			while( vecJumpDir.x == 0 && vecJumpDir.y == 0 )
			{
				vecJumpDir.x = random->RandomInt( -1, 1 ); 
				vecJumpDir.y = random->RandomInt( -1, 1 );
			}

			vecJumpDir.NormalizeInPlace();

			SetGroundEntity( NULL );
	
			m_vecSavedJump = vecJumpDir * 512 + Vector( 0, 0, 256 );
			m_bForcedStuckJump = true;
	
			return SCHED_ANTLION_JUMP;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldJump( void )
{
	if ( GetEnemy() == NULL )
		return false;

	//Too soon to try to jump
	if ( m_flJumpTime > gpGlobals->curtime )
		return false;

	// only jump if you're on the ground
  	if (!(GetFlags() & FL_ONGROUND) || GetNavType() == NAV_JUMP )
		return false;

	// Don't jump if I'm not allowed
	if ( ( CapabilitiesGet() & bits_CAP_MOVE_JUMP ) == false )
		return false;

	Vector vEnemyForward, vForward;

	GetEnemy()->GetVectors( &vEnemyForward, NULL, NULL );
	GetVectors( &vForward, NULL, NULL );

	float flDot = DotProduct( vForward, vEnemyForward );

	if ( flDot < 0.5f )
		 flDot = 0.5f;

	Vector vecPredictedPos;

	//Get our likely position in two seconds
	UTIL_PredictedPosition( GetEnemy(), flDot * 2.5f, &vecPredictedPos );

	// Don't jump if we're already near the target
	if ( ( GetAbsOrigin() - vecPredictedPos ).LengthSqr() < (512*512) )
		return false;

	//Don't retest if the target hasn't moved enough
	//FIXME: Check your own distance from last attempt as well
	if ( ( ( m_vecLastJumpAttempt - vecPredictedPos ).LengthSqr() ) < (128*128) )
	{
		m_flJumpTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );		
		return false;
	}

	Vector	targetDir = ( vecPredictedPos - GetAbsOrigin() );

	float flDist = VectorNormalize( targetDir );

	// don't jump at target it it's very close
	if (flDist < ANTLION_JUMP_MIN)
		return false;

	Vector	targetPos = vecPredictedPos + ( targetDir * (GetHullWidth()*4.0f) );

	if ( CAntlionRepellant::IsPositionRepellantFree( targetPos ) == false )
		 return false;

	// Try the jump
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_JUMP, GetAbsOrigin(), targetPos, MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );

	//See if it succeeded
	if ( IsMoveBlocked( moveTrace.fStatus ) )
	{
		if ( g_debug_antlion.GetInt() == 2 )
		{
			NDebugOverlay::Box( targetPos, GetHullMins(), GetHullMaxs(), 255, 0, 0, 0, 5 );
			NDebugOverlay::Line( GetAbsOrigin(), targetPos, 255, 0, 0, 0, 5 );
		}

		m_flJumpTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 2.0f );
		return false;
	}

	if ( g_debug_antlion.GetInt() == 2 )
	{
		NDebugOverlay::Box( targetPos, GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 5 );
		NDebugOverlay::Line( GetAbsOrigin(), targetPos, 0, 255, 0, 0, 5 );
	}

	//Save this jump in case the next time fails
	m_vecSavedJump = moveTrace.vJumpVelocity;
	m_vecLastJumpAttempt = targetPos;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::TranslateSchedule( int scheduleType )
{
	if ( ( m_hFollowTarget != NULL ) || IsAllied() )
	{
		if ( ( scheduleType == SCHED_IDLE_STAND ) || ( scheduleType == SCHED_ALERT_STAND ) )
			return SCHED_ANTLION_BUGBAIT_IDLE_STAND;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

Activity CNPC_Antlion::NPC_TranslateActivity( Activity baseAct )
{
	return BaseClass::NPC_TranslateActivity( baseAct );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::ChooseMoveSchedule( void )
{
	// See if we need to invalidate our fight goal
	if ( ShouldResumeFollow() )
	{
		// Set us back to following
		SetMoveState( ANTLION_MOVE_FOLLOW );

		// Tell our parent that we've swapped modes
		CAntlionTemplateMaker *pMaker = dynamic_cast<CAntlionTemplateMaker *>(GetOwnerEntity());

		if ( pMaker != NULL )
		{
			pMaker->SetChildMoveState( ANTLION_MOVE_FOLLOW );
		}
	}

	// Figure out our move state
	switch( m_MoveState )
	{
	case ANTLION_MOVE_FREE:
		return SCHED_NONE;	// Let the base class handle us
		break;

	// Fighting to a position
	case ANTLION_MOVE_FIGHT_TO_GOAL:
		{
			if ( m_hFightGoalTarget )
			{
				float targetDist = UTIL_DistApprox( WorldSpaceCenter(), m_hFightGoalTarget->GetAbsOrigin() );

				if ( targetDist > 256 )
				{
					Vector testPos;
					Vector targetPos = ( m_hFightGoalTarget ) ? m_hFightGoalTarget->GetAbsOrigin() : m_vSavePosition;

					// Find a suitable chase position
					if ( FindChasePosition( targetPos, testPos ) )
					{
						m_vSavePosition = testPos;
						return SCHED_ANTLION_RUN_TO_FIGHT_GOAL;
					}
				}
			}
		}
		break;

	// Following a goal
	case ANTLION_MOVE_FOLLOW:
		{
			if ( m_FollowBehavior.CanSelectSchedule() )
			{
				// See if we should burrow away if our target it too far off
				if ( ShouldAbandonFollow() )
					return SCHED_ANTLION_BURROW_AWAY;

				DeferSchedulingToBehavior( &m_FollowBehavior );
				return BaseClass::SelectSchedule();
			}
		}
		break;
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::SelectSchedule( void )
{
	// Clear out this condition
	ClearCondition( COND_ANTLION_RECEIVED_ORDERS );

	// If we're supposed to be burrowed, stay there
	if ( m_bStartBurrowed )
		return SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER;

	// See if a friendly player is pushing us away
	if ( HasCondition( COND_PLAYER_PUSHING ) )
		return SCHED_MOVE_AWAY;

	//Flipped?
	if ( HasCondition( COND_ANTLION_FLIPPED ) )
	{
		ClearCondition( COND_ANTLION_FLIPPED );
		return SCHED_ANTLION_FLIP;
	}

	if( HasCondition( COND_ANTLION_IN_WATER ) )
	{
		// No matter what, drown in water
		return SCHED_ANTLION_DROWN;
	}

	// If we're flagged to burrow away when eluded, do so
	if ( ( m_spawnflags & SF_ANTLION_BURROW_ON_ELUDED ) && ( HasCondition( COND_ENEMY_UNREACHABLE ) || HasCondition( COND_ENEMY_TOO_FAR ) ) )
		return SCHED_ANTLION_BURROW_AWAY;

	//Hear a thumper?
	if ( HasCondition( COND_HEAR_THUMPER ) )
	{
		// Ignore thumpers that aren't visible
		CSound *pSound = GetLoudestSoundOfType( SOUND_THUMPER );
		
		if ( pSound )
		{
			CTakeDamageInfo info;
			PainSound( info );
			ClearCondition( COND_HEAR_THUMPER );

			return SCHED_ANTLION_FLEE_THUMPER;
		}
	}

	//Hear a physics danger sound?
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		CTakeDamageInfo info;
		PainSound( info );
		return SCHED_ANTLION_FLEE_PHYSICS_DANGER;
	}

	//On another NPC's head?
	if( HasCondition( COND_ANTLION_ON_NPC ) )
	{
		// You're on an NPC's head. Get off.
		return SCHED_ANTLION_DISMOUNT_NPC;
	}

	//Hear bug bait splattered?
	if ( HasCondition( COND_HEAR_BUGBAIT ) && ( m_bIgnoreBugbait == false ) )
	{
		//Play a special sound
		if ( m_flNextAcknowledgeTime < gpGlobals->curtime )
		{
			EmitSound( "NPC_Antlion.Distracted" );
			m_flNextAcknowledgeTime = gpGlobals->curtime + 1.0f;
		}
		
		m_flIdleDelay = gpGlobals->curtime + 4.0f;

		//If the sound is valid, act upon it
		if ( m_bHasHeardSound )
		{		
			//Mark anything in the area as more interesting
			CBaseEntity *pTarget = NULL;
			CBaseEntity *pNewEnemy = NULL;
			Vector		soundOrg = m_vecHeardSound;

			//Find all entities within that sphere
			while ( ( pTarget = gEntList.FindEntityInSphere( pTarget, soundOrg, bugbait_radius.GetInt() ) ) != NULL )
			{
				CAI_BaseNPC *pNPC = pTarget->MyNPCPointer();

				if ( pNPC == NULL )
					continue;

				if ( pNPC->CanBeAnEnemyOf( this ) == false )
					continue;

				//Check to see if the default relationship is hatred, and if so intensify that
				if ( ( IRelationType( pNPC ) == D_HT ) && ( pNPC->IsPlayer() == false ) )
				{
					AddEntityRelationship( pNPC, D_HT, 99 );
					
					//Try to spread out the enemy distribution
					if ( ( pNewEnemy == NULL ) || ( random->RandomInt( 0, 1 ) ) )
					{
						pNewEnemy = pNPC;
						continue;
					}
				}
			}
			
			// If we have a new enemy, take it
			if ( pNewEnemy != NULL )
			{
				//Setup our ignore info
				SetEnemy( pNewEnemy );
			}
			
			ClearCondition( COND_HEAR_BUGBAIT );

			return SCHED_ANTLION_CHASE_BUGBAIT;
		}
	}

	if( m_AssaultBehavior.CanSelectSchedule() )
	{
		DeferSchedulingToBehavior( &m_AssaultBehavior );
		return BaseClass::SelectSchedule();
	}

	//Otherwise do basic state schedule selection
	switch ( m_NPCState )
	{	
	case NPC_STATE_COMBAT:
		{
			// Lunge at the enemy
			if ( HasCondition( COND_CAN_MELEE_ATTACK2 ) )
			{
				m_flPounceTime = gpGlobals->curtime + 1.5f;

				if ( m_bLeapAttack == true )
					 return SCHED_ANTLION_POUNCE_MOVING;
				else
					 return SCHED_ANTLION_POUNCE;
			}

			// Try to jump
			if ( HasCondition( COND_ANTLION_CAN_JUMP ) )
				return SCHED_ANTLION_JUMP;
		}
		break;

	default:
		{
			int	moveSched = ChooseMoveSchedule();

			if ( moveSched != SCHED_NONE )
				return moveSched;

		}
		break;
	}

	return BaseClass::SelectSchedule();
}

void CNPC_Antlion::Ignite ( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
#ifdef HL2_EPISODIC
	float flDamage = m_iHealth + 1;

	CTakeDamageInfo	dmgInfo( this, this, flDamage, DMG_GENERIC );
	GuessDamageForce( &dmgInfo, Vector( 0, 0, 8 ), GetAbsOrigin() );
	TakeDamage( dmgInfo );
#else
	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );
#endif

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Antlion::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;

	if( hl2_episodic.GetBool() && antlion_easycrush.GetBool() )
	{
		if( newInfo.GetDamageType() & DMG_CRUSH )
		{
			if( newInfo.GetInflictor() && newInfo.GetInflictor()->VPhysicsGetObject() )
			{
				float flMass = newInfo.GetInflictor()->VPhysicsGetObject()->GetMass();

				if( flMass > 250.0f && newInfo.GetDamage() < GetHealth() )
				{
					newInfo.SetDamage( GetHealth() );
				}
			}
		}
	}

	return BaseClass::OnTakeDamage_Alive( newInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo newInfo = info;

	Vector	vecShoveDir = vecDir;
	vecShoveDir.z = 0.0f;

	//Are we already flipped?
	if ( GetActivity() == ACT_ANTLION_FLIP )
	{
		//If we were hit by physics damage, move with it
		if ( newInfo.GetDamageType() & (DMG_CRUSH|DMG_PHYSGUN) )
		{
			PainSound( newInfo );
			ApplyAbsVelocityImpulse( ( vecShoveDir * random->RandomInt( 500.0f, 1000.0f ) ) + Vector(0,0,64.0f) );
			SetGroundEntity( NULL );
		}

		//More vulnerable when flipped
		newInfo.ScaleDamage( 4.0f );
	}
	else if ( newInfo.GetDamageType() & (DMG_PHYSGUN) || 
			( newInfo.GetDamageType() & (DMG_BLAST|DMG_CRUSH) && newInfo.GetDamage() >= 25.0f ) )
	{
		// Don't do this if we're in an interaction
		if ( !IsRunningDynamicInteraction() )
 		{
			//Grenades, physcannons, and physics impacts make us fuh-lip!
			
			if( hl2_episodic.GetBool() )
			{
				PainSound( newInfo );

				if( GetFlags() & FL_ONGROUND )
				{
					// Only flip if on the ground.
					SetCondition( COND_ANTLION_FLIPPED );
				}

				ApplyAbsVelocityImpulse( ( vecShoveDir * random->RandomInt( 500.0f, 1000.0f ) ) + Vector(0,0,64.0f) );
				SetGroundEntity( NULL );
			}
			else
			{
				//Don't flip off the deck
				if ( GetFlags() & FL_ONGROUND )
				{
					PainSound( newInfo );

					SetCondition( COND_ANTLION_FLIPPED );

					//Get tossed!
					ApplyAbsVelocityImpulse( ( vecShoveDir * random->RandomInt( 500.0f, 1000.0f ) ) + Vector(0,0,64.0f) );
					SetGroundEntity( NULL );
				}
			}
		}
	}

	BaseClass::TraceAttack( newInfo, vecDir, ptr );
}

void CNPC_Antlion::StopLoopingSounds( void )
{
	if ( m_bLoopingStarted )
	{
		StopSound( "NPC_Antlion.WingsOpen" );
		m_bLoopingStarted = false;
	}
	if ( m_bAgitatedSound )
	{
		StopSound( "NPC_Antlion.LoopingAgitated" );
		m_bAgitatedSound = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::IdleSound( void )
{
	EmitSound( "NPC_Antlion.Idle" );
	m_flIdleDelay = gpGlobals->curtime + 4.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Antlion.Pain" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
float CNPC_Antlion::GetIdealAccel( void ) const
{
	return GetIdealSpeed() * 2.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_Antlion::MaxYawSpeed( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 32.0f;
		break;
	
	case ACT_WALK:
		return 16.0f;
		break;
	
	default:
	case ACT_RUN:
		return 32.0f;
		break;
	}

	return 32.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldPlayIdleSound( void )
{
	//Only do idles in the right states
	if ( ( m_NPCState != NPC_STATE_IDLE && m_NPCState != NPC_STATE_ALERT ) )
		return false;

	//Gagged monsters don't talk
	if ( m_spawnflags & SF_NPC_GAG )
		return false;

	//Don't cut off another sound or play again too soon
	if ( m_flIdleDelay > gpGlobals->curtime )
		return false;

	//Randomize it a bit
	if ( random->RandomInt( 0, 20 ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::MeleeAttack1Conditions( float flDot, float flDist )
{
#if 1 //NOTENOTE: Use predicted position melee attacks

	float		flPrDist, flPrDot;
	Vector		vecPrPos;
	Vector2D	vec2DPrDir;

	//Get our likely position in one half second
	UTIL_PredictedPosition( GetEnemy(), 0.5f, &vecPrPos );

	//Get the predicted distance and direction
	flPrDist	= ( vecPrPos - GetAbsOrigin() ).Length();
	vec2DPrDir	= ( vecPrPos - GetAbsOrigin() ).AsVector2D();

	Vector vecBodyDir = BodyDirection2D();

	Vector2D vec2DBodyDir = vecBodyDir.AsVector2D();
	
	flPrDot	= DotProduct2D ( vec2DPrDir, vec2DBodyDir );

	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.m_pEnt != GetEnemy() && tr.fraction < 1.0f )
		return 0;

	if ( flPrDist > ANTLION_MELEE1_RANGE )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flPrDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

#else

	if ( flDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

	float flAdjustedDist = ANTLION_MELEE1_RANGE;

	if ( GetEnemy() )
	{
		// Give us extra space if our enemy is in a vehicle
		CBaseCombatCharacter *pCCEnemy = GetEnemy()->MyCombatCharacterPointer();
		if ( pCCEnemy != NULL && pCCEnemy->IsInAVehicle() )
		{
			flAdjustedDist *= 2.0f;
		}
	}

	if ( flDist > flAdjustedDist )
		return COND_TOO_FAR_TO_ATTACK;

	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
		return 0;

#endif

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Antlion::MeleeAttack2Conditions( float flDot, float flDist )
{
	// See if it's too soon to pounce again
	if ( m_flPounceTime > gpGlobals->curtime )
		return 0;

	float		flPrDist, flPrDot;
	Vector		vecPrPos;
	Vector2D	vec2DPrDir;

	//Get our likely position in one half second
	UTIL_PredictedPosition( GetEnemy(), 0.25f, &vecPrPos );

	//Get the predicted distance and direction
	flPrDist	= ( vecPrPos - GetAbsOrigin() ).Length();
	vec2DPrDir	= ( vecPrPos - GetAbsOrigin() ).AsVector2D();

	Vector vecBodyDir = BodyDirection2D();

	Vector2D vec2DBodyDir = vecBodyDir.AsVector2D();
	
	flPrDot	= DotProduct2D ( vec2DPrDir, vec2DBodyDir );

	if ( ( flPrDist > ANTLION_MELEE2_RANGE_MAX ) || ( flPrDist < ANTLION_MELEE2_RANGE_MIN ) )
	{
		m_flPounceTime = gpGlobals->curtime + 0.2f;
		return COND_TOO_FAR_TO_ATTACK;
	}

	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), -Vector(8,8,8), Vector(8,8,8), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
		return 0;

	if ( IsMoving() )
		 m_bLeapAttack = true;
	else
		 m_bLeapAttack = false;

	return COND_CAN_MELEE_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : interactionType - 
//			*data - 
//			*sender - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sender )
{
	//Check for a target found while burrowed
	if ( interactionType == g_interactionAntlionFoundTarget )
	{
		CBaseEntity	*pOther = (CBaseEntity *) data;
		
		//Randomly delay
		m_flBurrowTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 1.0f );
		BurrowUse( pOther, pOther, USE_ON, 0.0f );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::Alone( void )
{
	if ( m_pSquad == NULL )
		return true;

	if ( m_pSquad->NumMembers() <= 1 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::StartJump( void )
{
	if ( m_bForcedStuckJump == false )
	{
		//Must be jumping at an enemy
		if ( GetEnemy() == NULL )
			return;

		//Don't jump if we're not on the ground
		if ( ( GetFlags() & FL_ONGROUND ) == false )
			return;
	}

	//Take us off the ground
	SetGroundEntity( NULL );
	SetAbsVelocity( m_vecSavedJump );

	m_bForcedStuckJump = false;

	//Setup our jump time so that we don't try it again too soon
	m_flJumpTime = gpGlobals->curtime + random->RandomInt( 2, 6 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : sHint - 
//			nNodeNum - 
// Output : bool CAI_BaseNPC::FValidateHintType
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FValidateHintType( CAI_Hint *pHint )
{
	switch ( m_iContext )
	{
	case ANTLION_BURROW_OUT:
		{			
			//See if this is a valid point
			Vector vHintPos;
			pHint->GetPosition(this,&vHintPos);

			if ( ValidBurrowPoint( vHintPos ) == false )
				return false;
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::ClearBurrowPoint( const Vector &origin )
{
	CBaseEntity *pEntity = NULL;
	float		flDist;
	Vector		vecSpot, vecCenter, vecForce;

	bool bPlayerInSphere = false;

	//Iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( origin, 128 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
	{
		if ( pEntity->Classify() == CLASS_PLAYER )
		{
			bPlayerInSphere = true;
			continue;
		}

		if ( pEntity->m_takedamage != DAMAGE_NO && pEntity->Classify() != CLASS_PLAYER && pEntity->VPhysicsGetObject() )
		{
			vecSpot	 = pEntity->BodyTarget( origin );
			vecForce = ( vecSpot - origin ) + Vector( 0, 0, 16 );

			// decrease damage for an ent that's farther from the bomb.
			flDist = VectorNormalize( vecForce );

			//float mass = pEntity->VPhysicsGetObject()->GetMass();
			CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1.0f, 1.0f, 1.0f ), &vecCenter );

			if ( flDist <= 128.0f )
			{
				pEntity->VPhysicsGetObject()->Wake();
				pEntity->VPhysicsGetObject()->ApplyForceOffset( vecForce * 250.0f, vecCenter );
			}
		}
	}
	
	if ( bPlayerInSphere == false )
	{
		//Cause a ruckus
		UTIL_ScreenShake( origin, 1.0f, 80.0f, 1.0f, 256.0f, SHAKE_START );
	}
}

bool NPC_CheckBrushExclude( CBaseEntity *pEntity, CBaseEntity *pBrush );
//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimpleNPCExclude : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterSimpleNPCExclude, CTraceFilterSimple );

	CTraceFilterSimpleNPCExclude( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		Assert( dynamic_cast<CBaseEntity*>(pHandleEntity) );
		CBaseEntity *pTestEntity = static_cast<CBaseEntity*>(pHandleEntity);

		if ( GetPassEntity() )
		{
			CBaseEntity *pEnt = gEntList.GetBaseEntity( GetPassEntity()->GetRefEHandle() );

			if ( pEnt->IsNPC() )
			{
				if ( NPC_CheckBrushExclude( pEnt, pTestEntity ) == true )
					return false;
			}
		}
		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Determine whether a point is valid or not for burrowing up into
// Input  : &point - point to test for validity
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ValidBurrowPoint( const Vector &point )
{
	trace_t	tr;

	CTraceFilterSimpleNPCExclude filter( this, COLLISION_GROUP_NONE );
	AI_TraceHull( point, point+Vector(0,0,1), GetHullMins(), GetHullMaxs(), 
		MASK_NPCSOLID, &filter, &tr );

	//See if we were able to get there
	if ( ( tr.startsolid ) || ( tr.allsolid ) || ( tr.fraction < 1.0f ) )
	{
		CBaseEntity *pEntity = tr.m_pEnt;

		//If it's a physics object, attempt to knock is away, unless it's a car
		if ( ( pEntity ) && ( pEntity->VPhysicsGetObject() ) && ( pEntity->GetServerVehicle() == NULL ) )
		{
			ClearBurrowPoint( point );
		}

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a burrow point for the antlion
// Input  : distance - radius to search for burrow spot in
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::FindBurrow( const Vector &origin, float distance, int type, bool excludeNear )
{
	//Burrowing in?
	if ( type == ANTLION_BURROW_IN )
	{
		//Attempt to find a burrowing point
		CHintCriteria	hintCriteria;

		hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
		hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );

		hintCriteria.AddIncludePosition( origin, distance );
		
		if ( excludeNear )
		{
			hintCriteria.AddExcludePosition( origin, 128 );
		}

		CAI_Hint *pHint = CAI_HintManager::FindHint( this, hintCriteria );

		if ( pHint == NULL )
			return false;

		//Free up the node for use
		if ( GetHintNode() )
		{
			GetHintNode()->Unlock(0);
		}

		SetHintNode( pHint );

		//Lock the node
		pHint->Lock(this);

		//Setup our path and attempt to run there
		Vector vHintPos;
		GetHintNode()->GetPosition( this, &vHintPos );

		AI_NavGoal_t goal( vHintPos, ACT_RUN );

		return GetNavigator()->SetGoal( goal );
	}

	//Burrow out
	m_iContext = ANTLION_BURROW_OUT;

	CHintCriteria	hintCriteria;

	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
	hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );

	if ( GetEnemy() != NULL )
	{
		hintCriteria.AddIncludePosition( GetEnemy()->GetAbsOrigin(), distance );
	}

	//Attempt to find an open burrow point
	CAI_Hint *pHint = CAI_HintManager::FindHint( this, hintCriteria );

	m_iContext = -1;

	if ( pHint == NULL )
		return false;

	//Free up the node for use
	if (GetHintNode())
	{
		GetHintNode()->Unlock(0);
	}

	SetHintNode( pHint );
	pHint->Lock(this);

	Vector burrowPoint;
	pHint->GetPosition(this,&burrowPoint);

	UTIL_SetOrigin( this, burrowPoint );

	//Burrowing out
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:	Cause the antlion to unborrow
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------

void CNPC_Antlion::BurrowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//Don't allow us to do this again
	SetUse( NULL );
	
	//Allow idle sounds again
	m_spawnflags &= ~SF_NPC_GAG;

	//If the player activated this, then take them as an enemy
	if ( ( pCaller != NULL ) && ( pCaller->IsPlayer() ) )
	{
		SetEnemy( pActivator );
	}

	//Start trying to surface
	SetSchedule( SCHED_ANTLION_WAIT_UNBORROW );
}

//-----------------------------------------------------------------------------
// Purpose: Monitor the antlion's jump to play the proper landing sequence
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CheckLanding( void )
{
	trace_t	tr;
	Vector	testPos;

	//Amount of time to predict forward
	const float	timeStep = 0.1f;

	//Roughly looks one second into the future
	testPos = GetAbsOrigin() + ( GetAbsVelocity() * timeStep );
	testPos[2] -= ( 0.5 * sv_gravity.GetFloat() * GetGravity() * timeStep * timeStep);

	if ( g_debug_antlion.GetInt() == 2 )
	{
		NDebugOverlay::Line( GetAbsOrigin(), testPos, 255, 0, 0, 0, 0.5f );
		NDebugOverlay::Cross3D( m_vecSavedJump, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, true, 0.5f );
	} 
	
	// Look below
	AI_TraceHull( GetAbsOrigin(), testPos, NAI_Hull::Mins( GetHullType() ), NAI_Hull::Maxs( GetHullType() ), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	//See if we're about to contact, or have already contacted the ground
	if ( ( tr.fraction != 1.0f ) || ( GetFlags() & FL_ONGROUND ) )
	{
		int	sequence = SelectWeightedSequence( (Activity)ACT_ANTLION_LAND );

		if ( GetSequence() != sequence )
		{
			SetWings( false );
			VacateStrategySlot();
			SetIdealActivity( (Activity) ACT_ANTLION_LAND );

			CreateDust( false );
			EmitSound( "NPC_Antlion.Land" );

			if ( GetEnemy() && GetEnemy()->IsPlayer()  )
			{
				CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

				if ( pPlayer && pPlayer->IsInAVehicle() == false )
					 MeleeAttack( ANTLION_MELEE1_RANGE, sk_antlion_swipe_damage.GetFloat(), QAngle( 4.0f, 0.0f, 0.0f ), Vector( -250.0f, 1.0f, 1.0f ) );
			}

			SetAbsVelocity( GetAbsVelocity() * 0.33f );
			return false;
		}

		return IsActivityFinished();
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC )
{
	//If we're under the ground, don't look at enemies
	if ( IsEffectActive( EF_NODRAW ) )
		return false;

	return BaseClass::QuerySeeEntity(pEntity, bOnlyHateOrFearIfNPC);
}

//-----------------------------------------------------------------------------
// Purpose: Turns the antlion's wings on or off
// Input  : state - on or off
//-----------------------------------------------------------------------------
void CNPC_Antlion::SetWings( bool state )
{
	if ( m_bWingsOpen == state )
		return;

	m_bWingsOpen = state;

	if ( m_bWingsOpen )
	{
		CPASAttenuationFilter filter( this, "NPC_Antlion.WingsOpen" );
		filter.MakeReliable();

		EmitSound( filter, entindex(), "NPC_Antlion.WingsOpen" );
		SetBodygroup( 1, 1 );
		m_bLoopingStarted = true;
	}
	else
	{
		StopSound( "NPC_Antlion.WingsOpen" );
		SetBodygroup( 1, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Burrow( void )
{
	SetWings( false );

	//Stop us from taking damage and being solid
	m_spawnflags |= SF_NPC_GAG;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Unburrow( void )
{
	m_bStartBurrowed = false;
	SetWings( false );

	//Become solid again and visible
	m_spawnflags &= ~SF_NPC_GAG;
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage	= DAMAGE_YES;

	SetGroundEntity( NULL );

	//If we have an enemy, come out facing them
	if ( GetEnemy() )
	{
		Vector	dir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize(dir);

		QAngle angles = GetAbsAngles();
		angles[ YAW ] = UTIL_VecToYaw( dir );
		SetLocalAngles( angles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputUnburrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	SetSchedule( SCHED_ANTLION_WAIT_UNBORROW );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputBurrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	SetSchedule( SCHED_ANTLION_BURROW_IN );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputBurrowAway( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	SetSchedule( SCHED_ANTLION_BURROW_AWAY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::CreateDust( bool placeDecal )
{
	trace_t	tr;
	AI_TraceLine( GetAbsOrigin()+Vector(0,0,1), GetAbsOrigin()-Vector(0,0,64), MASK_SOLID_BRUSHONLY | CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );

		if ( hl2_episodic.GetBool() == true || ( pdata->game.material == CHAR_TEX_CONCRETE ) || 
			 ( pdata->game.material == CHAR_TEX_DIRT ) ||
			 ( pdata->game.material == CHAR_TEX_SAND ) ) 
		{

			UTIL_CreateAntlionDust( tr.endpos + Vector(0,0,24), GetAbsAngles() );
			
			if ( placeDecal )
			{
				UTIL_DecalTrace( &tr, "Antlion.Unburrow" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSound - 
//-----------------------------------------------------------------------------
bool CNPC_Antlion::QueryHearSound( CSound *pSound )
{
	if ( !BaseClass::QueryHearSound( pSound ) )
		return false;
		
	if ( pSound->m_iType == SOUND_BUGBAIT )
	{
		//Must be more recent than the current
		if ( pSound->SoundExpirationTime() <= m_flIgnoreSoundTime )
			return false;

		//If we can hear it, store it
		m_bHasHeardSound = (pSound != NULL);
		if ( m_bHasHeardSound )
		{
			m_vecHeardSound = pSound->GetSoundOrigin();
			m_flIgnoreSoundTime	= pSound->SoundExpirationTime();
		}
	}

	//Do the normal behavior at this point
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Antlion::BuildScheduleTestBits( void )
{
	//Don't allow any modifications when scripted
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return;

	// If we're allied with the player, don't be startled by him
	if ( IsAllied() )
	{
		ClearCustomInterruptCondition( COND_HEAR_PLAYER );
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
	}

	//Make sure we interrupt a run schedule if we can jump
	if ( IsCurSchedule(SCHED_CHASE_ENEMY) )
	{
		SetCustomInterruptCondition( COND_ANTLION_CAN_JUMP );
		SetCustomInterruptCondition( COND_ENEMY_UNREACHABLE );
	}

	if ( !IsCurSchedule( SCHED_ANTLION_DROWN ) )
	{
		// Interrupt any schedule unless already drowning.
		SetCustomInterruptCondition( COND_ANTLION_IN_WATER );
	}
	else
	{
		// Don't stop drowning just because you're in water!
		ClearCustomInterruptCondition( COND_ANTLION_IN_WATER );
	}

	// Make sure we don't stop in midair
	/*
	if ( GetActivity() == ACT_JUMP || GetActivity() == ACT_GLIDE || GetActivity() == ACT_LAND )
	{
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
	}
	*/
	
	//Interrupt any schedule unless already fleeing, burrowing, burrowed, or unburrowing.
	if( !IsCurSchedule(SCHED_ANTLION_FLEE_THUMPER)			&& 		
		!IsCurSchedule(SCHED_ANTLION_FLEE_PHYSICS_DANGER)	&& 		
		!IsCurSchedule(SCHED_ANTLION_BURROW_IN)				&& 		
		!IsCurSchedule(SCHED_ANTLION_WAIT_UNBORROW)			&& 		
		!IsCurSchedule(SCHED_ANTLION_BURROW_OUT)			&&
		!IsCurSchedule(SCHED_ANTLION_BURROW_WAIT)			&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER)&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW)&&
		!IsCurSchedule(SCHED_ANTLION_WAIT_UNBORROW)			&&
		!IsCurSchedule(SCHED_ANTLION_JUMP)					&&
		!IsCurSchedule(SCHED_ANTLION_FLIP)					&&
		!IsCurSchedule(SCHED_ANTLION_DISMOUNT_NPC)			&& 
		( GetFlags() & FL_ONGROUND ) )
	{
		// Only do these if not jumping as well
		if (!IsCurSchedule(SCHED_ANTLION_JUMP))
		{
			if ( GetEnemy() == NULL )
			{
				SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
			}
			
			SetCustomInterruptCondition( COND_HEAR_THUMPER );
			SetCustomInterruptCondition( COND_HEAR_BUGBAIT );
			SetCustomInterruptCondition( COND_ANTLION_FLIPPED );

			if ( GetNavType() != NAV_JUMP )
				 SetCustomInterruptCondition( COND_ANTLION_RECEIVED_ORDERS );
		}

		SetCustomInterruptCondition( COND_ANTLION_ON_NPC );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsValidEnemy( CBaseEntity *pEnemy )
{
	//See if antlions are friendly to the player in this map
	if ( IsAllied() && pEnemy->IsPlayer() )
		return false;

	if ( pEnemy->IsWorld() )
		return false;

	//If we're chasing bugbait, close to within a certain radius before picking up enemies
	if ( IsCurSchedule( GetGlobalScheduleId( SCHED_ANTLION_CHASE_BUGBAIT ) ) && ( GetNavigator() != NULL ) )
	{
		//If the enemy is without the target radius, then don't allow them
		if ( ( GetNavigator()->IsGoalActive() ) && ( GetNavigator()->GetGoalPos() - pEnemy->GetAbsOrigin() ).Length() > bugbait_radius.GetFloat() )
			return false;
	}

	// If we're following an entity we limit our attack distances
	if ( m_FollowBehavior.GetFollowTarget() != NULL )
	{
		float enemyDist = ( pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();

		if ( m_flObeyFollowTime > gpGlobals->curtime )
		{
			// Unless we're right next to the enemy, follow our target
			if ( enemyDist > (128*128) )
				return false;
		}
		else
		{
			// Otherwise don't follow if the target is far 
			if ( enemyDist > (2000*2000) )
				return false;
		}
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// See if I've landed on an NPC!
	CBaseEntity *pGroundEnt = GetGroundEntity();
	
	if ( ( ( pGroundEnt != NULL ) && ( pGroundEnt->GetSolidFlags() & FSOLID_NOT_STANDABLE ) ) && ( GetFlags() & FL_ONGROUND ) && ( !IsEffectActive( EF_NODRAW ) && !pGroundEnt->IsEffectActive( EF_NODRAW ) ) )
	{
		SetCondition( COND_ANTLION_ON_NPC );
	}
	else
	{
		ClearCondition( COND_ANTLION_ON_NPC );
	}

	// See if our follow target is too far off
/*	if ( m_hFollowTarget != NULL )
	{
		float targetDist = UTIL_DistApprox( WorldSpaceCenter(), m_hFollowTarget->GetAbsOrigin() );

		if ( targetDist > 400 )
		{
			SetCondition( COND_ANTLION_FOLLOW_TARGET_TOO_FAR );
		}
		else
		{
			ClearCondition( COND_ANTLION_FOLLOW_TARGET_TOO_FAR );
		}
	}*/

	if ( IsCurSchedule( SCHED_ANTLION_BURROW_WAIT ) == false && 
		 IsCurSchedule(SCHED_ANTLION_BURROW_IN) == false && 
		 IsCurSchedule(SCHED_ANTLION_BURROW_OUT) == false && 
		 IsCurSchedule(SCHED_FALL_TO_GROUND ) == false &&
		 IsEffectActive( EF_NODRAW ) == false )
	{
		if( m_lifeState == LIFE_ALIVE && GetWaterLevel() > 1 )
		{
			// Start Drowning!
			SetCondition( COND_ANTLION_IN_WATER );
		}
	}

	//Ignore the player pushing me if I'm flipped over!
	if ( IsCurSchedule( SCHED_ANTLION_FLIP ) )
		 ClearCondition( COND_PLAYER_PUSHING );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::PrescheduleThink( void )
{
	UpdateHead();

	Activity eActivity = GetActivity();

	//See if we need to play their agitated sound
	if ( ( eActivity == ACT_ANTLION_RUN_AGITATED ) && ( m_bAgitatedSound == false ) )
	{
		//Start sound
		CPASAttenuationFilter filter( this, "NPC_Antlion.LoopingAgitated" );
		filter.MakeReliable();

		EmitSound( filter, entindex(), "NPC_Antlion.LoopingAgitated" );
		m_bAgitatedSound = true;
	}
	else if ( ( eActivity != ACT_ANTLION_RUN_AGITATED ) && ( m_bAgitatedSound == true ) )
	{
		//Stop sound
		StopSound( "NPC_Antlion.LoopingAgitated" );
		m_bAgitatedSound = false;
	}

	//See if our wings got interrupted from being turned off
	if (    ( m_bWingsOpen ) &&
			( eActivity != ACT_ANTLION_JUMP_START ) && 
			( eActivity != ACT_JUMP ) && 
			( eActivity != ACT_GLIDE ) && 
			( eActivity != ACT_ANTLION_LAND ) && 
			( eActivity != ACT_ANTLION_DISTRACT ))
	{
		SetWings( false );
	}

	// Make sure we've turned off our burrow state if we're not in it
	if ( IsEffectActive( EF_NODRAW ) &&
		 ( eActivity != ACT_ANTLION_BURROW_IDLE ) &&
		 ( eActivity != ACT_ANTLION_BURROW_OUT ) &&
		 ( eActivity != ACT_ANTLION_BURROW_IN) )
	{
		DevMsg( "Antlion failed to unburrow properly!\n" );
		Assert( 0 );
		RemoveEffects( EF_NODRAW );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage	= DAMAGE_YES;
		RemoveFlag( FL_NOTARGET );
		m_spawnflags &= ~SF_NPC_GAG;
	}

	//New Enemy? Try to jump at him.
	if ( HasCondition( COND_NEW_ENEMY ) )
	{
		m_flJumpTime = 0.0f;
	}

	if ( ShouldJump() )
	{
		SetCondition( COND_ANTLION_CAN_JUMP );
	}
	else
	{
		ClearCondition( COND_ANTLION_CAN_JUMP );
	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDamage - 
//			bitsDamageType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsLightDamage( const CTakeDamageInfo &info )
{
	if ( ( random->RandomInt( 0, 1 ) ) && ( info.GetDamage() > 3 ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::IsAllied( void )
{
	return ( GlobalEntity_GetState( "antlion_allied" ) == GLOBAL_ON );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldResumeFollow( void )
{
	if ( IsAllied() == false )
		return false;

	if ( m_MoveState == ANTLION_MOVE_FOLLOW || m_hFollowTarget == NULL )
		return false;

	if ( m_flSuppressFollowTime > gpGlobals->curtime )
		return false;

	if ( GetEnemy() != NULL )
	{
		m_flSuppressFollowTime = gpGlobals->curtime + random->RandomInt( 5, 10 );
		return false;
	}

	//TODO: See if the follow target has wandered off too far from where we last followed them to
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldAbandonFollow( void )
{
	// Never give up if we can see the goal
	if ( m_FollowBehavior.FollowTargetVisible() )
		return false;

	// Never give up if we're too close
	float flDistance = UTIL_DistApprox2D( m_FollowBehavior.GetFollowTarget()->WorldSpaceCenter(), WorldSpaceCenter() );

	if ( flDistance < 1500 )
		return false;

	if ( flDistance > 1500 * 2.0f )
		return true;

	// If we've failed too many times, give up
	if ( m_FollowBehavior.GetNumFailedFollowAttempts() )
		return true;

	// If the target simply isn't reachable to us, give up
	if ( m_FollowBehavior.TargetIsUnreachable() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::SetFightTarget( CBaseEntity *pTarget )
{
	m_hFightGoalTarget = pTarget;

	SetCondition( COND_ANTLION_RECEIVED_ORDERS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputFightToPosition( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );

	if ( pEntity != NULL )
	{
		SetFightTarget( pEntity );
		SetFollowTarget( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputStopFightToPosition( inputdata_t &inputdata )
{
	SetFightTarget( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// Do the base class
	BaseClass::GatherEnemyConditions( pEnemy );

	// Only continue if we burrow when eluded
	if ( ( m_spawnflags & SF_ANTLION_BURROW_ON_ELUDED ) == false )
		return;

	// If we're not already too far away, check again
	//TODO: Check to make sure we don't already have a condition set that removes the need for this
	if ( HasCondition( COND_ENEMY_UNREACHABLE ) == false )
	{
		Vector	predPosition;
		UTIL_PredictedPosition( GetEnemy(), 1.0f, &predPosition );

		Vector	predDir = ( predPosition - GetAbsOrigin() );
		float	predLength = VectorNormalize( predDir );

		// See if we'll be outside our effective target range
		if ( predLength > m_flEludeDistance )
		{
			Vector	predVelDir = ( predPosition - GetEnemy()->GetAbsOrigin() );
			float	predSpeed  = VectorNormalize( predVelDir );

			// See if the enemy is moving mostly away from us
			if ( ( predSpeed > 512.0f ) && ( DotProduct( predVelDir, predDir ) > 0.0f ) )
			{
				// Mark the enemy as eluded and burrow away
				ClearEnemyMemory();
				SetEnemy( NULL );
				SetIdealState( NPC_STATE_ALERT );
				SetCondition( COND_ENEMY_UNREACHABLE );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::ShouldGib( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & (DMG_NEVERGIB|DMG_DISSOLVE) )
		return false;

	if ( info.GetDamageType() & (DMG_ALWAYSGIB|DMG_BLAST) )
		return true;

	if ( m_iHealth < -20 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CorpseGib( const CTakeDamageInfo &info )
{
	CEffectData	data;
	
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
	VectorNormalize( data.m_vNormal );
	
	data.m_flScale = RemapVal( m_iHealth, 0, -500, 1, 3 );
	data.m_flScale = clamp( data.m_flScale, 1, 3 );

	DispatchEffect( "AntlionGib", data );

	//CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::Touch( CBaseEntity *pOther )
{
	//See if the touching entity is a vehicle
	CBasePlayer *pPlayer = ToBasePlayer( AI_GetSinglePlayer() );
	
	// FIXME: Technically we'll want to check to see if a vehicle has touched us with the player OR NPC driver

	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		IServerVehicle	*pVehicle = pPlayer->GetVehicle();
		CBaseEntity *pVehicleEnt = pVehicle->GetVehicleEnt();

		if ( pVehicleEnt == pOther )
		{
			CPropVehicleDriveable	*pDrivableVehicle = dynamic_cast<CPropVehicleDriveable *>( pVehicleEnt );

			if ( pDrivableVehicle != NULL )
			{
				//Get tossed!
				Vector	vecShoveDir = pOther->GetAbsVelocity();
				Vector	vecTargetDir = GetAbsOrigin() - pOther->GetAbsOrigin();
				
				VectorNormalize( vecShoveDir );
				VectorNormalize( vecTargetDir );

				bool bBurrowingOut = IsCurSchedule( SCHED_ANTLION_BURROW_OUT );

				if ( ( ( pDrivableVehicle->m_nRPM > 75 ) && DotProduct( vecShoveDir, vecTargetDir ) <= 0 ) || bBurrowingOut == true )
				{
					if ( GetActivity() == ACT_ANTLION_FLIP || bBurrowingOut == true )
					{
						float flDamage = m_iHealth;

						if ( random->RandomInt( 0, 10 ) > 4 )
							 flDamage += 25;
									
						CTakeDamageInfo	dmgInfo( pVehicleEnt, pPlayer, flDamage, DMG_VEHICLE );
					
						CalculateMeleeDamageForce( &dmgInfo, vecShoveDir, pOther->GetAbsOrigin() );
						TakeDamage( dmgInfo );
					}
					else
					{
						// We're being shoved
						CTakeDamageInfo	dmgInfo( pVehicleEnt, pPlayer, 0, DMG_VEHICLE );
						PainSound( dmgInfo );

						SetCondition( COND_ANTLION_FLIPPED );

						vecTargetDir[2] = 0.0f;

						ApplyAbsVelocityImpulse( ( vecTargetDir * 250.0f ) + Vector(0,0,64.0f) );
						SetGroundEntity( NULL );

						CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, GetAbsOrigin(), 256, 0.5f, this );
					}
				}
			}
		}
	}

	BaseClass::Touch( pOther );

#ifdef HL2_EPISODIC 
	if ( GetActivity() == ACT_GLIDE && IsValidEnemy( pOther ) )
	{
		CTakeDamageInfo	dmgInfo( this, this, sk_antlion_air_attack_dmg.GetInt(), DMG_SLASH );

		CalculateMeleeDamageForce( &dmgInfo, Vector( 0, 0, 1 ), GetAbsOrigin() );
		pOther->TakeDamage( dmgInfo );

		//Kick the player angles
		if ( pOther->IsPlayer() && !(pOther->GetFlags() & FL_GODMODE ) && pOther->GetMoveType() != MOVETYPE_NOCLIP )
		{
			pOther->ViewPunch( QAngle( 4.0f, 0.0f, 0.0f ) );
		}
	}
#endif

	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
		// Don't test for this if the pusher isn't friendly
		if ( IsValidEnemy( pOther ) )
			return;

		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;
	
		if ( !IsCurSchedule( SCHED_MOVE_AWAY ) && !IsCurSchedule( SCHED_ANTLION_BURROW_OUT ) )
			 TestPlayerPushing( pOther );
	}

	//Adrian: Explode if hit by gunship!
	//Maybe only do this if hit by the propellers?
	if ( pOther->IsNPC() )
	{
		if ( pOther->Classify() == CLASS_COMBINE_GUNSHIP )
		{
			float flDamage = m_iHealth + 25;
						
			CTakeDamageInfo	dmgInfo( pOther, pOther, flDamage, DMG_GENERIC );
			GuessDamageForce( &dmgInfo, (pOther->GetAbsOrigin() - GetAbsOrigin()), pOther->GetAbsOrigin() );
			TakeDamage( dmgInfo );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Antlion::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
  	// FIXME: this will break scripted sequences that walk when they have an enemy
  	if ( GetEnemy() && GetNavigator()->GetMovementActivity() == ACT_RUN )
  	{
		Vector vecEnemyLKP = GetEnemyLKP();
		
		// Only start facing when we're close enough
		if ( UTIL_DistApprox( vecEnemyLKP, GetAbsOrigin() ) < 512 )
		{
			AddFacingTarget( GetEnemy(), vecEnemyLKP, 1.0, 0.2 );
		}
	}

	//Adrian: Make antlions face the thumper while they flee away.
	if ( IsCurSchedule( SCHED_ANTLION_FLEE_THUMPER ) )
	{
		CSound *pSound = GetLoudestSoundOfType( SOUND_THUMPER );

		if ( pSound )
		{
			AddFacingTarget( pSound->GetSoundOrigin(), 1.0, 0.5f );
		}
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputDisableJump( inputdata_t &inputdata )
{
	m_bDisableJump = true;
	CapabilitiesRemove( bits_CAP_MOVE_JUMP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputEnableJump( inputdata_t &inputdata )
{
	m_bDisableJump = false;
	CapabilitiesAdd( bits_CAP_MOVE_JUMP );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::SetFollowTarget( CBaseEntity *pTarget )
{
	m_FollowBehavior.SetFollowTarget( pTarget );
	m_hFollowTarget = pTarget;
	m_flObeyFollowTime = gpGlobals->curtime + ANTLION_OBEY_FOLLOW_TIME;

	SetCondition( COND_ANTLION_RECEIVED_ORDERS );

	// Play an acknowledgement noise
	if ( m_flNextAcknowledgeTime < gpGlobals->curtime )
	{
		EmitSound( "NPC_Antlion.Distracted" );
		m_flNextAcknowledgeTime = gpGlobals->curtime + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Antlion::CreateBehaviors( void )
{
	AddBehavior( &m_FollowBehavior );
	AddBehavior( &m_AssaultBehavior );

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputIgnoreBugbait( inputdata_t &inputdata )
{
	m_bIgnoreBugbait = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::InputHearBugbait( inputdata_t &inputdata )
{
	m_bIgnoreBugbait = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CNPC_Antlion::SetMoveState( AntlionMoveState_e state )
{
	m_MoveState = state;

	switch( m_MoveState )
	{
	case ANTLION_MOVE_FOLLOW:

		m_FollowBehavior.SetFollowTarget( m_hFollowTarget );
		
		// Clear any previous state
		m_flSuppressFollowTime = 0;
		
		break;
	
	case ANTLION_MOVE_FIGHT_TO_GOAL:
		
		m_FollowBehavior.SetFollowTarget( NULL );

		// Keep the time we started this
		m_flSuppressFollowTime = gpGlobals->curtime + random->RandomInt( 10, 15 );
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Special version helps other NPCs hit overturned antlion
//-----------------------------------------------------------------------------
Vector CNPC_Antlion::BodyTarget( const Vector &posSrc, bool bNoisy /*= true*/ )
{ 
	// Cache the bone away to avoid future lookups
	if ( m_nBodyBone == -1 )
	{
		CBaseAnimating *pAnimating = GetBaseAnimating();
		m_nBodyBone = pAnimating->LookupBone( "Antlion.Body_Bone" );
	}

	// Get the exact position in our center of mass (thorax)
	Vector vecResult;
	QAngle vecAngle;
	GetBonePosition( m_nBodyBone, vecResult, vecAngle );
	
	if ( bNoisy )
		return vecResult + RandomVector( -8, 8 );

	return vecResult;
}

// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CAntlionRepellant )
	DEFINE_KEYFIELD( m_flRepelRadius,	FIELD_FLOAT,	"repelradius" ),
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Disable", InputDisable ),
END_DATADESC()

static CUtlVector< CHandle< CAntlionRepellant > >m_hRepellantList;


CAntlionRepellant::~CAntlionRepellant()
{
	m_hRepellantList.FindAndRemove( this );
}

void CAntlionRepellant::Spawn( void )
{
	BaseClass::Spawn();
	m_bEnabled = true;

	m_hRepellantList.AddToTail( this );
}

void CAntlionRepellant::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	if ( m_hRepellantList.HasElement( this ) == false )
		 m_hRepellantList.AddToTail( this );
}

void CAntlionRepellant::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	m_hRepellantList.FindAndRemove( this );
}

float CAntlionRepellant::GetRadius( void )
{
	if ( m_bEnabled == false )
		 return 0.0f;

	return m_flRepelRadius;
}

void CAntlionRepellant::OnRestore( void )
{
	BaseClass::OnRestore();

	if ( m_bEnabled == true )
	{
		if ( m_hRepellantList.HasElement( this ) == false )
			 m_hRepellantList.AddToTail( this );
	}
}

bool CAntlionRepellant::IsPositionRepellantFree( Vector vDesiredPos )
{
	for ( int i = 0; i < m_hRepellantList.Count(); i++ )
	{
		if ( m_hRepellantList[i] )
		{
			CAntlionRepellant *pRep = m_hRepellantList[i].Get();

			if ( pRep )
			{
				float flDist = (vDesiredPos - pRep->GetAbsOrigin()).Length();

				if ( flDist <= pRep->GetRadius() )
					 return false;
			}
		}
	}

	return true;
}

LINK_ENTITY_TO_CLASS( point_antlion_repellant, CAntlionRepellant);

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_antlion, CNPC_Antlion )

	//Register our interactions
	DECLARE_INTERACTION( g_interactionAntlionFoundTarget )

	//Conditions
	DECLARE_CONDITION( COND_ANTLION_FLIPPED )
	DECLARE_CONDITION( COND_ANTLION_ON_NPC )
	DECLARE_CONDITION( COND_ANTLION_CAN_JUMP )
	DECLARE_CONDITION( COND_ANTLION_FOLLOW_TARGET_TOO_FAR )
	DECLARE_CONDITION( COND_ANTLION_RECEIVED_ORDERS )
	DECLARE_CONDITION( COND_ANTLION_IN_WATER )
		
	//Squad slots
	DECLARE_SQUADSLOT( SQUAD_SLOT_ANTLION_JUMP )

	//Tasks
	DECLARE_TASK( TASK_ANTLION_SET_CHARGE_GOAL )
	DECLARE_TASK( TASK_ANTLION_BURROW )
	DECLARE_TASK( TASK_ANTLION_UNBURROW )
	DECLARE_TASK( TASK_ANTLION_VANISH )
	DECLARE_TASK( TASK_ANTLION_FIND_BURROW_IN_POINT )
	DECLARE_TASK( TASK_ANTLION_FIND_BURROW_OUT_POINT )
	DECLARE_TASK( TASK_ANTLION_BURROW_WAIT )
	DECLARE_TASK( TASK_ANTLION_CHECK_FOR_UNBORROW )
	DECLARE_TASK( TASK_ANTLION_JUMP )
	DECLARE_TASK( TASK_ANTLION_WAIT_FOR_TRIGGER )
	DECLARE_TASK( TASK_ANTLION_GET_THUMPER_ESCAPE_PATH )
	DECLARE_TASK( TASK_ANTLION_GET_PATH_TO_BUGBAIT )
	DECLARE_TASK( TASK_ANTLION_FACE_BUGBAIT )
	DECLARE_TASK( TASK_ANTLION_DISMOUNT_NPC )
	DECLARE_TASK( TASK_ANTLION_REACH_FIGHT_GOAL )
	DECLARE_TASK( TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH )
	DECLARE_TASK( TASK_ANTLION_FACE_JUMP )
	DECLARE_TASK( TASK_ANTLION_DROWN )
	

	//Activities
	DECLARE_ACTIVITY( ACT_ANTLION_DISTRACT )
	DECLARE_ACTIVITY( ACT_ANTLION_DISTRACT_ARRIVED )
	DECLARE_ACTIVITY( ACT_ANTLION_JUMP_START )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_IN )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_OUT )
	DECLARE_ACTIVITY( ACT_ANTLION_BURROW_IDLE )
	DECLARE_ACTIVITY( ACT_ANTLION_RUN_AGITATED )
	DECLARE_ACTIVITY( ACT_ANTLION_FLIP )
	DECLARE_ACTIVITY( ACT_ANTLION_POUNCE )
	DECLARE_ACTIVITY( ACT_ANTLION_POUNCE_MOVING )
	DECLARE_ACTIVITY( ACT_ANTLION_DROWN )
	DECLARE_ACTIVITY( ACT_ANTLION_LAND )

	//Events
	DECLARE_ANIMEVENT( AE_ANTLION_WALK_FOOTSTEP )
	DECLARE_ANIMEVENT( AE_ANTLION_MELEE_HIT1 )
	DECLARE_ANIMEVENT( AE_ANTLION_MELEE_HIT2 )
	DECLARE_ANIMEVENT( AE_ANTLION_MELEE_POUNCE )
	DECLARE_ANIMEVENT( AE_ANTLION_FOOTSTEP_SOFT )
	DECLARE_ANIMEVENT( AE_ANTLION_FOOTSTEP_HEAVY )
	DECLARE_ANIMEVENT( AE_ANTLION_START_JUMP )
	DECLARE_ANIMEVENT( AE_ANTLION_BURROW_IN )
	DECLARE_ANIMEVENT( AE_ANTLION_BURROW_OUT )
	DECLARE_ANIMEVENT( AE_ANTLION_VANISH )
	DECLARE_ANIMEVENT( AE_ANTLION_OPEN_WINGS )
	DECLARE_ANIMEVENT( AE_ANTLION_CLOSE_WINGS )
	DECLARE_ANIMEVENT( AE_ANTLION_MELEE1_SOUND )
	DECLARE_ANIMEVENT( AE_ANTLION_MELEE2_SOUND )

	//Schedules

	//==================================================
	// Jump
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_JUMP,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_ANTLION_FACE_JUMP			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ANTLION_JUMP_START"
		"		TASK_ANTLION_JUMP				0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for unborrow (once burrow has been triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_UNBORROW,

		"	Tasks"
		"		TASK_ANTLION_BURROW_WAIT		0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Burrow Wait
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_WAIT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_BURROW_WAIT			1"
		"		TASK_ANTLION_FIND_BURROW_OUT_POINT	1024"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Burrow In
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_IN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_ANTLION_BURROW					0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_ANTLION_BURROW_IN"
		"		TASK_ANTLION_VANISH					0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Run to burrow in
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_RUN_TO_BURROW_IN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_SET_TOLERANCE_DISTANCE			8"
		"		TASK_ANTLION_FIND_BURROW_IN_POINT	512"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_IN"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_GIVE_WAY"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
	)

	//==================================================
	// Burrow Out
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_OUT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_UNBURROW			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ANTLION_BURROW_OUT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for unborrow (triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_FOR_UNBORROW_TRIGGER,

		"	Tasks"
		"		TASK_ANTLION_WAIT_FOR_TRIGGER	0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Wait for clear burrow spot (triggered)
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_WAIT_FOR_CLEAR_UNBORROW,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLION_BURROW_WAIT"
		"		TASK_ANTLION_CHECK_FOR_UNBORROW		1"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_ANTLION_BURROW_OUT"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// Run from the sound of a thumper!
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLEE_THUMPER,
		
		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_IDLE_STAND"
		"		TASK_ANTLION_GET_THUMPER_ESCAPE_PATH	0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_STOP_MOVING						0"
		"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ANTLION_DISTRACT_ARRIVED"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ANTLION_FLIPPED"
	)

	//==================================================
	// SCHED_ANTLION_CHASE_BUGBAIT
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_CHASE_BUGBAIT,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLION_GET_PATH_TO_BUGBAIT	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_STOP_MOVING					0"
		"		TASK_ANTLION_FACE_BUGBAIT			0"
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLION_FLIP
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLIP,

		"	Tasks"
		"		TASK_STOP_MOVING	0"
		"		TASK_PLAY_SEQUENCE	ACTIVITY:ACT_ANTLION_FLIP"

		"	Interrupts"
		"		COND_TASK_FAILED"
	)
	
	//=========================================================
	// Headcrab has landed atop another NPC. Get down!
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_DISMOUNT_NPC,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_ANTLION_DISMOUNT_NPC	0"

		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_RUN_TO_FIGHT_GOAL,

		"	Tasks"
		"		TASK_SET_TOLERANCE_DISTANCE		128"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_ANTLION_REACH_FIGHT_GOAL	0"

		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANTLION_CAN_JUMP"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_RUN_TO_FOLLOW_GOAL,

		"	Tasks"
		"		TASK_SET_TOLERANCE_DISTANCE		128"
		"		TASK_GET_PATH_TO_SAVEPOSITION	0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"

		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANTLION_CAN_JUMP"
		"		COND_ANTLION_FOLLOW_TARGET_TOO_FAR"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BUGBAIT_IDLE_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_PLAYER		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT				2"

		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_ANTLION_CAN_JUMP"
		"		COND_ANTLION_FOLLOW_TARGET_TOO_FAR"
		"		COND_GIVE_WAY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_BURROW_AWAY,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_ANTLION_BURROW		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANTLION_BURROW_IN"
		"		TASK_ANTLION_VANISH		1"

		"	Interrupts"
	)

	//==================================================
	// Run from the sound of a physics crash
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_FLEE_PHYSICS_DANGER,
		
		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ANTLION_GET_PHYSICS_DANGER_ESCAPE_PATH	1024"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_STOP_MOVING							0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	// Pounce forward at our enemy
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_POUNCE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_RESET_ACTIVITY		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANTLION_POUNCE"

		"	Interrupts"
		"		COND_TASK_FAILED"
	)
	// Pounce forward at our enemy
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_POUNCE_MOVING,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"
		"		TASK_RESET_ACTIVITY		0"
		"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_ANTLION_POUNCE_MOVING"

		"	Interrupts"
		"		COND_TASK_FAILED"
	)

		//=========================================================
	// The irreversible process of drowning
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLION_DROWN,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_ANTLION_DROWN"
		"		TASK_ANTLION_DROWN			0"
		""
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()


