//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npc_turret_floor.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "hl2_dll/hl2_player.h"
#include "soundenvelope.h"
#include "physics_saverestore.h"
#include "IEffects.h"
#include "basehlcombatweapon_shared.h"
#include "phys_controller.h"
#include "ai_interactions.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *GetMassEquivalent(float flMass);

#define	DISABLE_SHOT	0

//Debug visualization
ConVar	g_debug_turret( "g_debug_turret", "0" );

extern ConVar physcannon_tracelength;

// Interactions
int	g_interactionTurretStillStanding	= 0;

float CNPC_FloorTurret::fMaxTipControllerVelocity = 300.0f * 300.0f;
float CNPC_FloorTurret::fMaxTipControllerAngularVelocity = 90.0f * 90.0f;


#define	FLOOR_TURRET_MODEL			"models/combine_turrets/floor_turret.mdl"
#define FLOOR_TURRET_GLOW_SPRITE	"sprites/glow1.vmt"
#define FLOOR_TURRET_BC_YAW			"aim_yaw"
#define FLOOR_TURRET_BC_PITCH		"aim_pitch"
#define	FLOOR_TURRET_RANGE			1200
#define	FLOOR_TURRET_MAX_WAIT		5
#define FLOOR_TURRET_SHORT_WAIT		2.0		// Used for FAST_RETIRE spawnflag
#define	FLOOR_TURRET_PING_TIME		1.0f	//LPB!!

#define	FLOOR_TURRET_VOICE_PITCH_LOW	45
#define	FLOOR_TURRET_VOICE_PITCH_HIGH	100

//Aiming variables
#define	FLOOR_TURRET_MAX_NOHARM_PERIOD		0.0f
#define	FLOOR_TURRET_MAX_GRACE_PERIOD		3.0f

//Spawnflags
// BUG: These all stomp Base NPC spawnflags. Any Base NPC code called by this
//		this class may have undesired side effects due to these being set.
#define SF_FLOOR_TURRET_AUTOACTIVATE		0x00000020
#define SF_FLOOR_TURRET_STARTINACTIVE		0x00000040
#define SF_FLOOR_TURRET_FASTRETIRE			0x00000080
#define SF_FLOOR_TURRET_OUT_OF_AMMO			0x00000100

//Activities
int ACT_FLOOR_TURRET_OPEN;
int ACT_FLOOR_TURRET_CLOSE;
int ACT_FLOOR_TURRET_OPEN_IDLE;
int ACT_FLOOR_TURRET_CLOSED_IDLE;
int ACT_FLOOR_TURRET_FIRE;

//Datatable
BEGIN_DATADESC( CNPC_FloorTurret )

	DEFINE_FIELD( m_iAmmoType,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bAutoStart,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBlinkState,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEnabled,		FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flShotTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flLastSight,	FIELD_TIME ),
	DEFINE_FIELD( m_flThrashTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flPingTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flNextActivateSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_bCarriedByPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUseCarryAngles, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flPlayerDropTime, FIELD_TIME ),
	DEFINE_FIELD( m_hLastNPCToKickMe, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flKnockOverFailedTime, FIELD_TIME ),

	DEFINE_FIELD( m_vecGoalAngles,FIELD_VECTOR ),
	DEFINE_FIELD( m_iEyeAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iMuzzleAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iEyeState,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hEyeGlow,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_pMotionController,FIELD_EHANDLE),
	DEFINE_FIELD( m_vecEnemyLKP,	FIELD_VECTOR ),

	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_FIELD( m_bHackedByAlyx, FIELD_BOOLEAN ),
	
	DEFINE_THINKFUNC( Retire ),
	DEFINE_THINKFUNC( Deploy ),
	DEFINE_THINKFUNC( ActiveThink ),
	DEFINE_THINKFUNC( SearchThink ),
	DEFINE_THINKFUNC( AutoSearchThink ),
	DEFINE_THINKFUNC( TippedThink ),
	DEFINE_THINKFUNC( InactiveThink ),
	DEFINE_THINKFUNC( SuppressThink ),
	DEFINE_THINKFUNC( DisabledThink ),

	DEFINE_USEFUNC( ToggleUse ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_OnDeploy, "OnDeploy" ),
	DEFINE_OUTPUT( m_OnRetire, "OnRetire" ),
	DEFINE_OUTPUT( m_OnTipped, "OnTipped" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),

	DEFINE_BASENPCINTERACTABLE_DATADESC(),

	// DEFINE_FIELD( m_ShotSounds, FIELD_SHORT ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_turret_floor, CNPC_FloorTurret );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_FloorTurret::CNPC_FloorTurret( void )
{
	m_bActive = false;
	m_hEyeGlow = NULL;
	m_iAmmoType = -1;
	m_bAutoStart = false;
	m_flPingTime = 0;
	m_flNextActivateSoundTime = 0;
	m_bCarriedByPlayer = false;
	m_bUseCarryAngles = false;
	m_flPlayerDropTime = 0;
	m_flShotTime = 0;
	m_flLastSight = 0;
	m_bBlinkState = false;
	m_flThrashTime = 0;
	m_pMotionController = NULL;
	m_bEnabled = false;

	m_vecGoalAngles.Init();

	m_vecEnemyLKP = vec3_invalid;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_FloorTurret::~CNPC_FloorTurret( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::UpdateOnRemove( void )
{
	if ( m_pMotionController != NULL )
	{
		UTIL_Remove( m_pMotionController );
		m_pMotionController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Precache( void )
{
	PrecacheModel( FLOOR_TURRET_MODEL );	
	PrecacheModel( FLOOR_TURRET_GLOW_SPRITE );

	// Activities
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_CLOSED_IDLE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_OPEN_IDLE );
	ADD_CUSTOM_ACTIVITY( CNPC_FloorTurret, ACT_FLOOR_TURRET_FIRE );
	
	PrecacheScriptSound( "NPC_FloorTurret.Retire" );
	PrecacheScriptSound( "NPC_FloorTurret.Deploy" );
	PrecacheScriptSound( "NPC_FloorTurret.Move" );
	PrecacheScriptSound( "NPC_Combine.WeaponBash" );
	PrecacheScriptSound( "NPC_FloorTurret.Activate" );
	PrecacheScriptSound( "NPC_FloorTurret.Alert" );
	m_ShotSounds = PrecacheScriptSound( "NPC_FloorTurret.ShotSounds" );
	PrecacheScriptSound( "NPC_FloorTurret.Die" );
	PrecacheScriptSound( "NPC_FloorTurret.Retract");
	PrecacheScriptSound( "NPC_FloorTurret.Alarm");
	PrecacheScriptSound( "NPC_FloorTurret.Ping");
	PrecacheScriptSound( "NPC_FloorTurret.DryFire");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the entity
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Spawn( void )
{ 
	Precache();

	SetModel( FLOOR_TURRET_MODEL );
	
	BaseClass::Spawn();

	SetBlocksLOS( false );

	m_HackedGunPos	= Vector( 0, 0, 12.75 );
	SetViewOffset( EyeOffset( ACT_IDLE ) );
	m_flFieldOfView	= 0.4f; // 60 degrees
	m_takedamage	= DAMAGE_EVENTS_ONLY;
	m_iHealth		= 100;
	m_iMaxHealth	= 100;

	AddEFlags( EFL_NO_DISSOLVE );

	SetPoseParameter( FLOOR_TURRET_BC_YAW, 0 );
	SetPoseParameter( FLOOR_TURRET_BC_PITCH, 0 );

	m_iAmmoType = GetAmmoDef()->Index( "PISTOL" );

	m_iMuzzleAttachment = LookupAttachment( "eyes" );
	m_iEyeAttachment = 2;	// FIXME: what's the correct name?

	// FIXME: Do we ever need m_bAutoStart? (Sawyer)
	m_spawnflags |= SF_FLOOR_TURRET_AUTOACTIVATE;

	//Set our autostart state
	m_bAutoStart = !!( m_spawnflags & SF_FLOOR_TURRET_AUTOACTIVATE );
	m_bEnabled	 = ( ( m_spawnflags & SF_FLOOR_TURRET_STARTINACTIVE ) == false );

	//Do we start active?
	if ( m_bAutoStart && m_bEnabled )
	{
		SetThink( &CNPC_FloorTurret::AutoSearchThink );
		SetEyeState( TURRET_EYE_DORMANT );
	}
	else
	{
		SetThink( &CNPC_FloorTurret::DisabledThink );
		SetEyeState( TURRET_EYE_DISABLED );
	}

	// Start 
	if ( OnSide() )
	{
		SetThink( &CNPC_FloorTurret::DisabledThink );
		SetEyeState( TURRET_EYE_DISABLED );
	}

	//Stagger our starting times
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ) );

	SetUse( &CNPC_FloorTurret::ToggleUse );

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Activate( void )
{
	BaseClass::Activate();

	// Force the eye state to the current state so that our glows are recreated after transitions
	SetEyeState( m_iEyeState );

	if ( !m_pMotionController )
	{
		// Create the motion controller
		m_pMotionController = CTurretTipController::CreateTipController( this );

		// Enable the controller
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Enable();
		}
	}

	// See if we've started tipped (if so, skip thrashing)
	if ( OnSide() )
	{
		SetThink( &CNPC_FloorTurret::InactiveThink );
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

//-----------------------------------------------------------------------------

bool CNPC_FloorTurret::CreateVPhysics( void )
{
	//Spawn our physics hull
	if ( VPhysicsInitNormal( SOLID_VPHYSICS, 0, false ) == NULL )
	{
		DevMsg( "npc_turret_floor unable to spawn physics object!\n" );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Retract and stop attacking
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Retire( void )
{
	if ( PreThink( TURRET_RETIRING ) )
		return;

	//Level out the turret
	m_vecGoalAngles = GetAbsAngles();
	SetNextThink( gpGlobals->curtime + 0.05f );

	//Set ourselves to close
	if ( GetActivity() != ACT_FLOOR_TURRET_CLOSE )
	{
		//Set our visible state to dormant
		SetEyeState( TURRET_EYE_DORMANT );

		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
		
		//If we're done moving to our desired facing, close up
		if ( UpdateFacing() == false )
		{
			SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSE );
			EmitSound( "NPC_FloorTurret.Retire" );

			//Notify of the retraction
			m_OnRetire.FireOutput( NULL, this );
		}
	}
	else if ( IsActivityFinished() )
	{	
		m_bActive		= false;
		m_flLastSight	= 0;

		SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSED_IDLE );

		//Go back to auto searching
		if ( m_bAutoStart )
		{
			SetThink( &CNPC_FloorTurret::AutoSearchThink );
			SetNextThink( gpGlobals->curtime + 0.05f );
		}
		else
		{
			//Set our visible state to dormant
			SetEyeState( TURRET_EYE_DISABLED );
			SetThink( &CNPC_FloorTurret::DisabledThink );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deploy and start attacking
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Deploy( void )
{
	if ( PreThink( TURRET_DEPLOYING ) )
		return;

	m_vecGoalAngles = GetAbsAngles();

	SetNextThink( gpGlobals->curtime + 0.05f );

	//Show we've seen a target
	SetEyeState( TURRET_EYE_SEE_TARGET );

	//Open if we're not already
	if ( GetActivity() != ACT_FLOOR_TURRET_OPEN )
	{
		m_bActive = true;
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN );
		EmitSound( "NPC_FloorTurret.Deploy" );

		//Notify we're deploying
		m_OnDeploy.FireOutput( NULL, this );
	}

	//If we're done, then start searching
	if ( IsActivityFinished() )
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );

		m_flShotTime  = gpGlobals->curtime + 1.0f;

		m_flPlaybackRate = 0;
		SetThink( &CNPC_FloorTurret::SearchThink );

		EmitSound( "NPC_FloorTurret.Move" );
	}

	m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	// Drop our mass a lot so that we can be moved easily with +USE
	if ( reason != PUNTED_BY_CANNON )
	{
		Assert( VPhysicsGetObject() );

		m_bCarriedByPlayer = true;
		m_OnPhysGunPickup.FireOutput( this, this );

		// We want to use preferred carry angles if we're not nicely upright
		Vector	up;
		GetVectors( NULL, NULL, &up );
		m_bUseCarryAngles = DotProduct( up, Vector(0,0,1) ) < 0.9f;
	}

	// Clear out our last NPC to kick me, because it makes no sense now
	m_hLastNPCToKickMe = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;
	
	m_bCarriedByPlayer = false;
	m_bUseCarryAngles = false;
	m_OnPhysGunDrop.FireOutput( this, this );

	// If this is a friendly turret, remember that it was just dropped
	if ( IRelationType( pPhysGunUser ) != D_HT )
	{
		m_flPlayerDropTime = gpGlobals->curtime + 2.0;
	}

	// Restore our mass to the original value
	Assert( VPhysicsGetObject() );
}

//-----------------------------------------------------------------------------
// Purpose: Whether this should return carry angles
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer )
{
	// Don't use preferred angles on enemy turrets
	if ( IRelationType( pPlayer ) == D_HT )
		return false;

	return m_bUseCarryAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	// Prevent players pulling enemy turrets from afar if they're in front of the turret
	if ( reason == PICKED_UP_BY_CANNON && IRelationType( pPhysGunUser ) == D_HT )
	{
		Vector vecForward;
		GetVectors( &vecForward, NULL, NULL );
		Vector vecForce = (pPhysGunUser->GetAbsOrigin() - GetAbsOrigin());
		float flDistance = VectorNormalize( vecForce );

		// If it's over the physcannon tracelength, we're pulling it
		if ( flDistance > physcannon_tracelength.GetFloat() )
		{
			float flDot = DotProduct( vecForward, vecForce );
			if ( flDot > 0.5 )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt)
{
	if ( interactionType == g_interactionCombineBash )
	{
		// We've been bashed by a combine soldier. Remember who it was, if we haven't got an active kicker
		if ( !m_hLastNPCToKickMe )
		{
			m_hLastNPCToKickMe = sourceEnt;
			m_flKnockOverFailedTime = gpGlobals->curtime + 3.0;
		}

		// Get knocked away
		Vector forward, up;
		AngleVectors( sourceEnt->GetLocalAngles(), &forward, NULL, &up );
		ApplyAbsVelocityImpulse( forward * 100 + up * 50 );
		CTakeDamageInfo info( sourceEnt, sourceEnt, 30, DMG_CLUB );
		CalculateMeleeDamageForce( &info, forward, GetAbsOrigin() );
		TakeDamage( info );

		EmitSound( "NPC_Combine.WeaponBash" );
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed at which the turret can face a target
//-----------------------------------------------------------------------------
float CNPC_FloorTurret::MaxYawSpeed( void )
{
	//TODO: Scale by difficulty?
	return 360.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this turret was recently dropped by a player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::WasJustDroppedByPlayer( void )
{
	if ( m_flPlayerDropTime > gpGlobals->curtime )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the turret to face its desired angles
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::UpdateFacing( void )
{
	bool  bMoved = false;
	UpdateMuzzleMatrix();

	Vector vecGoalDir;
	AngleVectors( m_vecGoalAngles, &vecGoalDir );

	Vector vecGoalLocalDir;
	VectorIRotate( vecGoalDir, m_muzzleToWorld, vecGoalLocalDir );

	if ( g_debug_turret.GetBool() )
	{
		Vector	vecMuzzle, vecMuzzleDir;

		MatrixGetColumn( m_muzzleToWorld, 3, vecMuzzle );
		MatrixGetColumn( m_muzzleToWorld, 0, vecMuzzleDir );

		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecMuzzleDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecMuzzleDir*256), 255, 255, 0, false, 0.05 );
		
		NDebugOverlay::Cross3D( vecMuzzle, -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMuzzle+(vecGoalDir*256), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 0.05 );
		NDebugOverlay::Line( vecMuzzle, vecMuzzle+(vecGoalDir*256), 255, 0, 0, false, 0.05 );
	}

	QAngle vecGoalLocalAngles;
	VectorAngles( vecGoalLocalDir, vecGoalLocalAngles );

	// Update pitch
	float flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.x, 0.0, 0.05f * MaxYawSpeed() ) );
	
	int iPose = LookupPoseParameter( FLOOR_TURRET_BC_PITCH );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + ( flDiff / 1.5f ) );

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	// Update yaw
	flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.y, 0.0, 0.05f * MaxYawSpeed() ) );

	iPose = LookupPoseParameter( FLOOR_TURRET_BC_YAW );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + ( flDiff / 1.5f ) );

	if ( fabs( flDiff ) > 0.1f )
	{
		bMoved = true;
	}

	// You're going to make decisions based on this info.  So bump the bone cache after you calculate everything
	InvalidateBoneCache();

	return bMoved;
}

void CNPC_FloorTurret::DryFire( void )
{
	EmitSound( "NPC_FloorTurret.DryFire");
	EmitSound( "NPC_FloorTurret.Activate" );

 	if ( RandomFloat( 0, 1 ) > 0.5 )
	{
		m_flShotTime = gpGlobals->curtime + random->RandomFloat( 1, 2.5 );
	}
	else
	{
		m_flShotTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turret will continue to fire on a target's position when it loses sight of it
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SuppressThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_SUPPRESSING ) )
		return;

	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Look for a new enemy
	HackFindEnemy();

	//If we've acquired an enemy, start firing at it
	if ( !GetEnemy() )
	{
		SetThink( &CNPC_FloorTurret::ActiveThink );
		return;
	}

	//See if we're done suppressing
	if ( gpGlobals->curtime > m_flLastSight )
	{
		// Should we look for a new target?
		ClearEnemyMemory();
		SetEnemy( NULL );
		SetThink( &CNPC_FloorTurret::SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		
		SpinDown();

		if ( m_spawnflags & SF_FLOOR_TURRET_FASTRETIRE )
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		}

		return;
	}

	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_vecEnemyLKP;

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;	

	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = vecMidEnemy - vecMid;
	VectorNormalize( vecDirToEnemyEyes );

	QAngle vecAnglesToEnemy;
	VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

	//Draw debug info
	if ( g_debug_turret.GetBool() )
	{
		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMidEnemy, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.05f );
	}

	if ( m_flShotTime < gpGlobals->curtime && m_vecEnemyLKP != vec3_invalid )
	{
		Vector vecMuzzle, vecMuzzleDir;
		UpdateMuzzleMatrix();
		MatrixGetColumn( m_muzzleToWorld, 0, vecMuzzleDir );
		MatrixGetColumn( m_muzzleToWorld, 3, vecMuzzle );

		//Fire the gun
		if ( DotProduct( vecDirToEnemy, vecMuzzleDir ) >= 0.9848 ) // 10 degree slop
		{
			if( m_spawnflags & SF_FLOOR_TURRET_OUT_OF_AMMO )
			{
				DryFire();
			}
			else
			{
				SetActivity( ACT_RESET );
				SetActivity( (Activity) ACT_FLOOR_TURRET_FIRE );

				//Fire the weapon
#if !DISABLE_SHOT
				Shoot( vecMuzzle, vecMuzzleDir );
#endif
			}
		} 
	}
	else
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
	}

	//If we can see our enemy, face it
	m_vecGoalAngles.y = vecAnglesToEnemy.y;
	m_vecGoalAngles.x = vecAnglesToEnemy.x;

	//Turn to face
	UpdateFacing();
}

//-----------------------------------------------------------------------------
// Purpose: Allows the turret to fire on targets if they're visible
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::ActiveThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_ACTIVE ) )
		return;

	HackFindEnemy();
	
	//Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	//If we've become inactive, go back to searching
	if ( ( m_bActive == false ) || ( GetEnemy() == NULL ) )
	{
		SetEnemy( NULL );
		m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		SetThink( &CNPC_FloorTurret::SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		return;
	}
	
	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = GetEnemy()->BodyTarget( vecMid );

	// Store off our last seen location so we can suppress it later
	m_vecEnemyLKP = vecMidEnemy;

	//Look for our current enemy
	bool bEnemyInFOV = FInViewCone( GetEnemy() );
	bool bEnemyVisible = FVisible( GetEnemy() ) && GetEnemy()->IsAlive();	

	// Robin: This is a hack to get around the fact that the muzzle for the turret
	// is outside it's vcollide. This means that if it leans against a thin wall, 
	// the muzzle can be on the other side of the wall, where it's then able to see
	// and shoot at targets. This check ensures that nothing has come between the
	// center of the turret and the muzzle.
	if ( bEnemyVisible )
	{
		trace_t tr;
		Vector vecCenter;
		CollisionProp()->CollisionToWorldSpace( Vector(0,0,52), &vecCenter );
		UTIL_TraceLine( vecCenter, vecMid, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			bEnemyVisible = false;
		}
	}

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;	
	float	flDistToEnemy = VectorNormalize( vecDirToEnemy );

	//Draw debug info
	if ( g_debug_turret.GetBool() )
	{
		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( GetEnemy()->WorldSpaceCenter(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, GetEnemy()->WorldSpaceCenter(), 0, 255, 0, false, 0.05 );

		NDebugOverlay::Cross3D( vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Cross3D( vecMidEnemy, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05 );
		NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.05f );
	}

	//See if they're past our FOV of attack
	if ( bEnemyInFOV == false )
	{
		// Should we look for a new target?
		ClearEnemyMemory();
		SetEnemy( NULL );
		
		if ( m_spawnflags & SF_FLOOR_TURRET_FASTRETIRE )
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;
		}

		SetThink( &CNPC_FloorTurret::SearchThink );
		m_vecGoalAngles = GetAbsAngles();
		
		SpinDown();

		return;
	}

	//Current enemy is not visible
	if ( ( bEnemyVisible == false ) || ( flDistToEnemy > FLOOR_TURRET_RANGE ))
	{
		m_flLastSight = gpGlobals->curtime + 2.0f;

		ClearEnemyMemory();
		SetEnemy( NULL );
		SetThink( &CNPC_FloorTurret::SuppressThink );

		return;
	}

	if ( g_debug_turret.GetBool() )
	{
		Vector vecMuzzle, vecMuzzleDir;

		UpdateMuzzleMatrix();
		MatrixGetColumn( m_muzzleToWorld, 0, vecMuzzleDir );
		MatrixGetColumn( m_muzzleToWorld, 3, vecMuzzle );

		// Visualize vertical firing ranges
		for ( int i = 0; i < 4; i++ )
		{
			QAngle angMaxDownPitch = GetAbsAngles();

			switch( i )
			{
			case 0:	angMaxDownPitch.x -= 15; break;
			case 1:	angMaxDownPitch.x += 15; break;
			case 2:	angMaxDownPitch.x -= 25; break;
			case 3:	angMaxDownPitch.x += 25; break;
			default:
				break;
			}

			Vector vecMaxDownPitch;
			AngleVectors( angMaxDownPitch, &vecMaxDownPitch );
			NDebugOverlay::Line( vecMuzzle, vecMuzzle + (vecMaxDownPitch*256), 255, 255, 255, false, 0.1 );
		}
	}

	if ( m_flShotTime < gpGlobals->curtime )
	{
		Vector vecMuzzle, vecMuzzleDir;

		UpdateMuzzleMatrix();
		MatrixGetColumn( m_muzzleToWorld, 0, vecMuzzleDir );
		MatrixGetColumn( m_muzzleToWorld, 3, vecMuzzle );

		Vector2D vecDirToEnemy2D = vecDirToEnemy.AsVector2D();
		Vector2D vecMuzzleDir2D = vecMuzzleDir.AsVector2D();

		bool bCanShoot = true;
		float minCos3d = DOT_10DEGREE; // 10 degrees slop

		if ( flDistToEnemy < 60.0 )
		{
			vecDirToEnemy2D.NormalizeInPlace();
			vecMuzzleDir2D.NormalizeInPlace();

			bCanShoot = ( vecDirToEnemy2D.Dot(vecMuzzleDir2D) >= DOT_10DEGREE );
			minCos3d = 0.7071; // 45 degrees
		}

		//Fire the gun
		if ( bCanShoot ) // 10 degree slop XY
		{
			float dot3d = DotProduct( vecDirToEnemy, vecMuzzleDir );

			if( m_spawnflags & SF_FLOOR_TURRET_OUT_OF_AMMO )
			{
				DryFire();
			}
			else
			{
				if ( dot3d >= minCos3d ) 
				{
					SetActivity( ACT_RESET );
					SetActivity( (Activity) ACT_FLOOR_TURRET_FIRE );

					//Fire the weapon
#if !DISABLE_SHOT
					Shoot( vecMuzzle, vecMuzzleDir, (dot3d < DOT_10DEGREE) );
				}
#endif
			}
		} 
	}
	else
	{
		SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
	}

	//If we can see our enemy, face it
	if ( bEnemyVisible )
	{
		//We want to look at the enemy's eyes so we don't jitter
		Vector	vecDirToEnemyEyes = GetEnemy()->WorldSpaceCenter() - vecMid;
		VectorNormalize( vecDirToEnemyEyes );

		QAngle vecAnglesToEnemy;
		VectorAngles( vecDirToEnemyEyes, vecAnglesToEnemy );

		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;
	}

	//Turn to face
	UpdateFacing();
}

//-----------------------------------------------------------------------------
// Purpose: Target doesn't exist or has eluded us, so search for one
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_SEARCHING ) )
		return;

	SetNextThink( gpGlobals->curtime + 0.05f );

	SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );

	//If our enemy has died, pick a new enemy
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire the target
	if ( GetEnemy() == NULL )
	{
		HackFindEnemy();
	}

	//If we've found a target, spin up the barrel and start to attack
	if ( GetEnemy() != NULL )
	{
		//Give players a grace period
		if ( GetEnemy()->IsPlayer() )
		{
			m_flShotTime  = gpGlobals->curtime + 0.5f;
		}
		else
		{
			m_flShotTime  = gpGlobals->curtime + 0.1f;
		}

		m_flLastSight = 0;
		SetThink( &CNPC_FloorTurret::ActiveThink );
		SetEyeState( TURRET_EYE_SEE_TARGET );

		SpinUp();
 
		if ( gpGlobals->curtime > m_flNextActivateSoundTime )
		{
			EmitSound( "NPC_FloorTurret.Activate" );
			m_flNextActivateSoundTime = gpGlobals->curtime + 3.0;
		}
		return;
	}

	//Are we out of time and need to retract?
 	if ( gpGlobals->curtime > m_flLastSight )
	{
		//Before we retrace, make sure that we are spun down.
		m_flLastSight = 0;
		SetThink( &CNPC_FloorTurret::Retire );
		return;
	}
	
	//Display that we're scanning
	m_vecGoalAngles.x = GetAbsAngles().x + ( sin( gpGlobals->curtime * 1.0f ) * 15.0f );
	m_vecGoalAngles.y = GetAbsAngles().y + ( sin( gpGlobals->curtime * 2.0f ) * 60.0f );

	//Turn and ping
	UpdateFacing();
	Ping();
}

//-----------------------------------------------------------------------------
// Purpose: Watch for a target to wander into our view
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::AutoSearchThink( void )
{
	//Allow descended classes a chance to do something before the think function
	if ( PreThink( TURRET_AUTO_SEARCHING ) )
		return; 

	//Spread out our thinking
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.2f, 0.4f ) );

	//If the enemy is dead, find a new one
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		SetEnemy( NULL );
	}

	//Acquire Target
	if ( GetEnemy() == NULL )
	{
		HackFindEnemy();
	}

	//Deploy if we've got an active target
	if ( GetEnemy() != NULL )
	{
		SetThink( &CNPC_FloorTurret::Deploy );
		EmitSound( "NPC_FloorTurret.Alert" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire!
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy, bool bStrict )
{
	FireBulletsInfo_t info;

	if ( !bStrict && GetEnemy() != NULL )
	{
		Vector vecDir = GetActualShootTrajectory( vecSrc );

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
	}
	else
	{
		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecDirToEnemy;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = GetAttackSpread( NULL, GetEnemy() );
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
	}

	FireBullets( info );
	EmitSound( "NPC_FloorTurret.ShotSounds", m_ShotSounds );
	DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( m_NPCState == NPC_STATE_DEAD )
		return false;

	// Don't shoot at other turrets.
	if ( pEnemy->m_iClassname == m_iClassname )
		return false;

	// If our eye is stuck in something, don't shoot
	if ( UTIL_PointContents(EyePosition()) & MASK_SHOT )
		return false;

	// Turrets have limited vertical aim capability
	//	- Can only aim +-15 degrees, + the 10 degree slop they're allowed.
	Vector los = ( pEnemy->EyePosition() - EyePosition() );
	QAngle angleToTarget;
	VectorAngles( los, angleToTarget );
	float flZDiff = fabs( AngleNormalize( angleToTarget.x - GetAbsAngles().x) );
	if ( flZDiff > 28.0 )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnemy - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::CanBeAnEnemyOf( CBaseEntity *pEnemy )
{
	// If we're out of ammo, make friendly companions ignore us
	if ( m_spawnflags & SF_FLOOR_TURRET_OUT_OF_AMMO )
	{
		if ( pEnemy->Classify() == CLASS_PLAYER_ALLY_VITAL )
			return false;
	} 

	// If we're on the side, we're never anyone's enemy
	if ( OnSide() )
		return false;

	return BaseClass::CanBeAnEnemyOf( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: The turret has been tipped over and will thrash for awhile
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::TippedThink( void )
{
	// Update our PVS state
	CheckPVSCondition();

	//Animate
	StudioFrameAdvance();

	SetNextThink( gpGlobals->curtime + 0.05f );
	SetEnemy( NULL );

	// If we're not on side anymore, stop thrashing
	if ( !OnSide() )
	{
		ReturnToLife();
		return;
	}

	//See if we should continue to thrash
	if ( gpGlobals->curtime < m_flThrashTime )
	{
		if ( m_flShotTime < gpGlobals->curtime )
		{
			if( m_spawnflags & SF_FLOOR_TURRET_OUT_OF_AMMO )
			{
				DryFire();
			}
			else
			{
				Vector vecMuzzle, vecMuzzleDir;
				UpdateMuzzleMatrix();
				MatrixGetColumn( m_muzzleToWorld, 0, vecMuzzleDir );
				MatrixGetColumn( m_muzzleToWorld, 3, vecMuzzle );

				SetActivity( ACT_RESET );
				SetActivity( (Activity) ACT_FLOOR_TURRET_FIRE );

#if !DISABLE_SHOT
				Shoot( vecMuzzle, vecMuzzleDir );
#endif
			}

			m_flShotTime = gpGlobals->curtime + 0.05f;
		}

		m_vecGoalAngles.x = GetAbsAngles().x + random->RandomFloat( -60, 60 );
		m_vecGoalAngles.y = GetAbsAngles().y + random->RandomFloat( -60, 60 );

		UpdateFacing();
	}
	else
	{
		//Face forward
		m_vecGoalAngles = GetAbsAngles();

		//Set ourselves to close
		if ( GetActivity() != ACT_FLOOR_TURRET_CLOSE )
		{
			SetActivity( (Activity) ACT_FLOOR_TURRET_OPEN_IDLE );
			
			//If we're done moving to our desired facing, close up
			if ( UpdateFacing() == false )
			{
				//Make any last death noises and anims
				EmitSound( "NPC_FloorTurret.Die" );
				SpinDown();

				SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSE );
				EmitSound( "NPC_FloorTurret.Retract" );

				CTakeDamageInfo	info;
				info.SetDamage( 1 );
				info.SetDamageType( DMG_CRUSH );
				Event_Killed( info );
			}
		}
		else if ( IsActivityFinished() )
		{	
			m_bActive		= false;
			m_flLastSight	= 0;

			SetActivity( (Activity) ACT_FLOOR_TURRET_CLOSED_IDLE );

			// Don't need to store last NPC anymore, because I've been knocked over
			if ( m_hLastNPCToKickMe )
			{
				m_hLastNPCToKickMe = NULL;
				m_flKnockOverFailedTime = 0;
			}

			//Try to look straight
			if ( UpdateFacing() == false )
			{
				m_OnTipped.FireOutput( this, this );
				SetEyeState( TURRET_EYE_DEAD );
				SetCollisionGroup( COLLISION_GROUP_DEBRIS );

				// Start thinking slowly to see if we're ever set upright somehow
				SetThink( &CNPC_FloorTurret::InactiveThink );
				SetNextThink( gpGlobals->curtime + 1.0f );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This turret is dead. See if it ever becomes upright again, and if 
//			so, become active again.
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InactiveThink( void )
{
	// Update our PVS state
	CheckPVSCondition();

	SetNextThink( gpGlobals->curtime + 1.0f );

	// Wake up if we're not on our side
	if ( !OnSide() && m_bEnabled )
	{
		ReturnToLife();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::ReturnToLife( void )
{
	m_flThrashTime = 0;

	// Enable the tip controller
	m_pMotionController->Enable( true );

	// Return to life
	SetState( NPC_STATE_IDLE );
	m_lifeState = LIFE_ALIVE;
	SetCollisionGroup( COLLISION_GROUP_NONE );

	// Become active again
	Enable();
}	

//-----------------------------------------------------------------------------
// Purpose: The turret is not doing anything at all
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::DisabledThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.5 );
	if ( OnSide() )
	{
		m_OnTipped.FireOutput( this, this );
		SetEyeState( TURRET_EYE_DEAD );
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		SetThink( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The turret doesn't run base AI properly, which is a bad decision.
//			As a result, it has to manually find enemies.
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::HackFindEnemy( void )
{
	// We have to refresh our memories before finding enemies, so
	// dead enemies are cleared out before new ones are added.
	GetEnemies()->RefreshMemories();

	GetSenses()->Look( FLOOR_TURRET_RANGE );
	SetEnemy( BestEnemy() );
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the turret is upright enough to function
// Output : Returns true if the turret is tipped over
//-----------------------------------------------------------------------------
inline bool CNPC_FloorTurret::OnSide( void )
{
	Vector	up;
	GetVectors( NULL, NULL, &up );

	return ( DotProduct( up, Vector(0,0,1) ) < 0.5f );
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CNPC_FloorTurret::PreThink( turretState_e state )
{
	// Hack to disable turrets when ai is disabled
	if ( CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI )
	{
		// Push our think out into the future
		SetNextThink( gpGlobals->curtime + 0.1f );
		return true;
	}
 
	CheckPVSCondition();

	//Animate
	StudioFrameAdvance();
 
	//See if we've tipped, but only do this if we're not being carried
	if ( !IsBeingCarriedByPlayer() )
	{
		if ( OnSide() == false )
		{
			// If I still haven't fallen over after an NPC has tried to knock me down, let them know
			if ( m_hLastNPCToKickMe && m_flKnockOverFailedTime < gpGlobals->curtime )
			{
				m_hLastNPCToKickMe->DispatchInteraction( g_interactionTurretStillStanding, NULL, this );
				m_hLastNPCToKickMe = NULL;
			}

			//Debug visualization
			if ( g_debug_turret.GetBool() )
			{
				Vector	up;
				GetVectors( NULL, NULL, &up );

				NDebugOverlay::Line( GetAbsOrigin()+(up*32), GetAbsOrigin()+(up*128), 0, 255, 0, false, 2.0f );
				NDebugOverlay::Cross3D( GetAbsOrigin()+(up*32), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 2.0f );
				NDebugOverlay::Cross3D( GetAbsOrigin()+(up*128), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 2.0f );
			}
		}
		else
		{
			//Thrash around for a bit
			m_flThrashTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 2.5f );
			SetNextThink( gpGlobals->curtime + 0.05f );

			SetThink( &CNPC_FloorTurret::TippedThink );

			//Stop being targetted
			SetState( NPC_STATE_DEAD );
			m_lifeState = LIFE_DEAD;

			//Disable the tip controller
			m_pMotionController->Enable( false );

			SetEyeState( TURRET_EYE_SEE_TARGET );

			SpinUp();
			EmitSound( "NPC_FloorTurret.Alarm" );

			//Debug visualization
			if ( g_debug_turret.GetBool() )
			{
				Vector	up;
				GetVectors( NULL, NULL, &up );

				NDebugOverlay::Line( GetAbsOrigin()+(up*32), GetAbsOrigin()+(up*128), 255, 0, 0, false, 2.0f );
				NDebugOverlay::Cross3D( GetAbsOrigin()+(up*32), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 2.0f );
				NDebugOverlay::Cross3D( GetAbsOrigin()+(up*128), -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, false, 2.0f );
			}

			//Interrupt current think function
			return true;
		}
	}

	//Do not interrupt current think function
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the glowing eye attached to the turret
// Input  : state - state the eye should be in
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SetEyeState( eyeState_t state )
{
	// Must have a valid eye to affect
	if ( !m_hEyeGlow )
	{
		// Create our eye sprite
		m_hEyeGlow = CSprite::SpriteCreate( FLOOR_TURRET_GLOW_SPRITE, GetLocalOrigin(), false );
		if ( !m_hEyeGlow )
			return;

		m_hEyeGlow->SetTransparency( kRenderWorldGlow, 255, 0, 0, 128, kRenderFxNoDissipation );
		m_hEyeGlow->SetAttachment( this, m_iEyeAttachment );
	}

	m_iEyeState = state;

	//Set the state
	switch( state )
	{
	default:
	case TURRET_EYE_SEE_TARGET: //Fade in and scale up
		m_hEyeGlow->SetColor( 255, 0, 0 );
		m_hEyeGlow->SetBrightness( 164, 0.1f );
		m_hEyeGlow->SetScale( 0.4f, 0.1f );
		break;

	case TURRET_EYE_SEEKING_TARGET: //Ping-pongs
		
		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_hEyeGlow->SetColor( 255, 128, 0 );

		if ( m_bBlinkState )
		{
			//Fade up and scale up
			m_hEyeGlow->SetScale( 0.25f, 0.1f );
			m_hEyeGlow->SetBrightness( 164, 0.1f );
		}
		else
		{
			//Fade down and scale down
			m_hEyeGlow->SetScale( 0.2f, 0.1f );
			m_hEyeGlow->SetBrightness( 64, 0.1f );
		}

		break;

	case TURRET_EYE_DORMANT: //Fade out and scale down
		m_hEyeGlow->SetColor( 0, 255, 0 );
		m_hEyeGlow->SetScale( 0.1f, 0.5f );
		m_hEyeGlow->SetBrightness( 64, 0.5f );
		break;

	case TURRET_EYE_DEAD: //Fade out slowly
		m_hEyeGlow->SetColor( 255, 0, 0 );
		m_hEyeGlow->SetScale( 0.1f, 3.0f );
		m_hEyeGlow->SetBrightness( 0, 3.0f );
		break;

	case TURRET_EYE_DISABLED:
		m_hEyeGlow->SetColor( 0, 255, 0 );
		m_hEyeGlow->SetScale( 0.1f, 1.0f );
		m_hEyeGlow->SetBrightness( 0, 1.0f );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a pinging noise so the player knows where we are
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Ping( void )
{
	//See if it's time to ping again
	if ( m_flPingTime > gpGlobals->curtime )
		return;

	//Ping!
	EmitSound( "NPC_FloorTurret.Ping" );

	SetEyeState( TURRET_EYE_SEEKING_TARGET );

	m_flPingTime = gpGlobals->curtime + FLOOR_TURRET_PING_TIME;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Toggle( void )
{
	//This turret is on its side, it can't function
	if ( OnSide() || ( IsAlive() == false ) )
		return;

	//Toggle the state
	if ( m_bEnabled )
	{
		Disable();
	}
	else 
	{
		Enable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable the turret and deploy
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Enable( void )
{
	// Always allow us to come back to life, even if not right now
	m_bEnabled = true;

	//This turret is on its side, it can't function
	if ( OnSide() || ( IsAlive() == false ) )
		return;

	// if the turret is flagged as an autoactivate turret, re-enable its ability open self.
	if ( m_spawnflags & SF_FLOOR_TURRET_AUTOACTIVATE )
	{
		m_bAutoStart = true;
	}

	SetThink( &CNPC_FloorTurret::Deploy );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Retire the turret until enabled again
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::Disable( void )
{
	//This turret is on its side, it can't function
	if ( OnSide() || ( IsAlive() == false ) )
		return;

	if ( m_bEnabled )
	{
		m_bEnabled = false;
		m_bAutoStart = false;

		SetEnemy( NULL );
		SetThink( &CNPC_FloorTurret::Retire );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
		SetThink( &CNPC_FloorTurret::DisabledThink );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the turret's state via input function
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputEnable( inputdata_t &inputdata )
{
	Enable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::InputDisable( inputdata_t &inputdata )
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: Allow players and npc's to turn the turret on and off
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	switch( useType )
	{
	case USE_OFF:
		Disable();
		break;
	case USE_ON:
		Enable();
		break;
	case USE_SET:
		break;
	case USE_TOGGLE:
		Toggle( );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reduce physics forces from the front
//-----------------------------------------------------------------------------
int CNPC_FloorTurret::VPhysicsTakeDamage( const CTakeDamageInfo &info )
{
	bool bShouldIgnoreFromFront = false;

	// Ignore crossbow bolts hitting us from the front
	bShouldIgnoreFromFront = ( info.GetDamageType() & DMG_BULLET ) != 0;

	// Ignore bullets from the front
	if ( !bShouldIgnoreFromFront )
	{
		bShouldIgnoreFromFront = FClassnameIs( info.GetInflictor(), "crossbow_bolt" );
	}

	// Did it hit us on the front?
	if ( bShouldIgnoreFromFront )
	{
		Vector vecForward;
		GetVectors( &vecForward, NULL, NULL );
		Vector vecForce = info.GetDamageForce();
		VectorNormalize( vecForce );
		float flDot = DotProduct( vecForward, vecForce );
		if ( flDot < -0.85 )
			return 0;
	}

	return BaseClass::VPhysicsTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CNPC_FloorTurret::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo	newInfo = info;

	if ( info.GetDamageType() & (DMG_SLASH|DMG_CLUB) )
	{
		// Take extra force from melee hits
		newInfo.ScaleDamageForce( 2.0f );
		
		// Disable our upright controller for some time
		if ( m_pMotionController != NULL )
		{
			m_pMotionController->Suspend( 2.0f );
		}
	}
	else if ( info.GetDamageType() & DMG_BLAST )
	{
		newInfo.ScaleDamageForce( 2.0f );
	}
	else if ( (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & DMG_BUCKSHOT) )
	{
		// Bullets, but not buckshot, do extra push
		newInfo.ScaleDamageForce( 2.5f );
	}

	// Manually apply vphysics because AI_BaseNPC takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( newInfo );

	// Bump up our search time
	m_flLastSight = gpGlobals->curtime + FLOOR_TURRET_MAX_WAIT;

	// Start looking around in anger if we were idle
	if ( IsAlive() && m_bEnabled && m_bAutoStart && GetActivity() == ACT_FLOOR_TURRET_CLOSED_IDLE )
	{
		SetThink( &CNPC_FloorTurret::Deploy );
	}

	return BaseClass::OnTakeDamage( newInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SpinUp( void )
{
}

#define	FLOOR_TURRET_MIN_SPIN_DOWN	1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FloorTurret::SpinDown( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_FloorTurret::GetAttackDamageScale( CBaseEntity *pVictim )
{
	CBaseCombatCharacter *pBCC = pVictim->MyCombatCharacterPointer();

	// Do extra damage to antlions & combine
	if ( pBCC )
	{
		if ( pBCC->Classify() == CLASS_ANTLION )
			return 2.0;
			
		if ( pBCC->Classify() == CLASS_COMBINE )
			return 2.0;
	}

	return BaseClass::GetAttackDamageScale( pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_FloorTurret::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget ) 
{
	WeaponProficiency_t weaponProficiency = WEAPON_PROFICIENCY_AVERAGE;

	// Switch our weapon proficiency based upon our target
	if ( pTarget )
	{
		if ( pTarget->Classify() == CLASS_PLAYER || pTarget->Classify() == CLASS_ANTLION || pTarget->Classify() == CLASS_ZOMBIE )
		{
			// Make me much more accurate
			weaponProficiency = WEAPON_PROFICIENCY_PERFECT;
		}
		else if ( pTarget->Classify() == CLASS_COMBINE )
		{
			// Make me more accurate
			weaponProficiency = WEAPON_PROFICIENCY_VERY_GOOD;
		}
	}

	return VECTOR_CONE_10DEGREES * ((CBaseHLCombatWeapon::GetDefaultProficiencyValues())[ weaponProficiency ].spreadscale);
}

//------------------------------------------------------------------------------
// Do we have a physics attacker?
//------------------------------------------------------------------------------
CBasePlayer *CNPC_FloorTurret::HasPhysicsAttacker( float dt )
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if ( IsHeldByPhyscannon( ) || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime) )
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_FloorTurret::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			EntityText( text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}

void CNPC_FloorTurret::UpdateMuzzleMatrix()
{
	if ( gpGlobals->tickcount != m_muzzleToWorldTick )
	{
		m_muzzleToWorldTick = gpGlobals->tickcount;
		GetAttachment( m_iMuzzleAttachment, m_muzzleToWorld );
	}
}

// 
// Tip controller
//

LINK_ENTITY_TO_CLASS( floorturret_tipcontroller, CTurretTipController );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CTurretTipController )

	DEFINE_FIELD( m_bEnabled,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flSuspendTime,	FIELD_TIME ),
	DEFINE_FIELD( m_worldGoalAxis,	FIELD_VECTOR ),
	DEFINE_FIELD( m_localTestAxis,	FIELD_VECTOR ),
	DEFINE_PHYSPTR( m_pController ),
	DEFINE_FIELD( m_angularLimit,		FIELD_FLOAT ),
	DEFINE_FIELD( m_pParentTurret,	FIELD_CLASSPTR ),

END_DATADESC()



CTurretTipController::~CTurretTipController()
{
	if ( m_pController )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Spawn( void )
{
	m_bEnabled = true;

	// align the object's local Z axis
	m_localTestAxis.Init( 0, 0, 1 );
	// with the world's Z axis
	m_worldGoalAxis.Init( 0, 0, 1 );

	// recover from up to 25 degrees / sec angular velocity
	m_angularLimit	= 25;
	m_flSuspendTime	= 0;

	SetMoveType( MOVETYPE_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Activate( void )
{
	BaseClass::Activate();

	if ( m_pParentTurret == NULL )
	{
		UTIL_Remove(this);
		return;
	}

	IPhysicsObject *pPhys = m_pParentTurret->VPhysicsGetObject();

	if ( pPhys == NULL )
	{
		UTIL_Remove(this);
		return;
	}

	//Setup the motion controller
	if ( !m_pController )
	{
		m_pController = physenv->CreateMotionController( (IMotionEvent *)this );
		m_pController->AttachObject( pPhys, true );
	}
	else
	{
		m_pController->SetEventHandler( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Actual simulation for tip controller
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CTurretTipController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( Enabled() == false )
		return SIM_NOTHING;

	// Don't simulate if we're being carried by the player
	if ( m_pParentTurret->IsBeingCarriedByPlayer() )
		return SIM_NOTHING;

	float flAngularLimit = m_angularLimit;

	// If we were just dropped by a friendly player, stabilise better
	if ( m_pParentTurret->WasJustDroppedByPlayer() )
	{
		// Increase the controller strength a little
		flAngularLimit += 20;
	}
	else
	{
		// If the turret has some vertical velocity, don't simulate
		Vector vecVelocity;
		AngularImpulse angImpulse;
		pObject->GetVelocity( &vecVelocity, &angImpulse );
		if ( (vecVelocity.LengthSqr() > CNPC_FloorTurret::fMaxTipControllerVelocity) || (angImpulse.LengthSqr() > CNPC_FloorTurret::fMaxTipControllerAngularVelocity) )
			return SIM_NOTHING;
	}

	linear.Init();

	AngularImpulse angVel;
	pObject->GetVelocity( NULL, &angVel );

	matrix3x4_t matrix;
	// get the object's local to world transform
	pObject->GetPositionMatrix( &matrix );

	// Get the alignment axis in object space
	Vector currentLocalTargetAxis;
	VectorIRotate( m_worldGoalAxis, matrix, currentLocalTargetAxis );

	float invDeltaTime = (1/deltaTime);
	angular = ComputeRotSpeedToAlignAxes( m_localTestAxis, currentLocalTargetAxis, angVel, 1.0, invDeltaTime * invDeltaTime, flAngularLimit * invDeltaTime );

	return SIM_LOCAL_ACCELERATION;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTurretTipController::Enable( bool state )
{
	m_bEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CTurretTipController::Suspend( float time )
{
	m_flSuspendTime = gpGlobals->curtime + time;
}


float CTurretTipController::SuspendedTill( void )
{
	return m_flSuspendTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTurretTipController::Enabled( void )
{
	if ( m_flSuspendTime > gpGlobals->curtime )
		return false;

	return m_bEnabled;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_turret_floor, CNPC_FloorTurret )

	DECLARE_INTERACTION( g_interactionTurretStillStanding );	

AI_END_CUSTOM_NPC()
