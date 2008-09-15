//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:	Antlion Guard
//
//=============================================================================//

#include "cbase.h"
#include "ai_hint.h"
#include "ai_memory.h"
#include "ai_moveprobe.h"
#include "npcevent.h"
#include "IEffects.h"
#include "ndebugoverlay.h"
#include "soundent.h"
#include "soundenvelope.h"
#include "ai_squad.h"
#include "ai_network.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_senses.h"
#include "npc_rollermine.h"
#include "ai_blended_movement.h"
#include "physics_prop_ragdoll.h"
#include "iservervehicle.h"
#include "player_pickup.h"
#include "props.h"
#include "antlion_dust.h"
#include "npc_antlion.h"
#include "decals.h"
#include "prop_combine_ball.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

inline void TraceHull_SkipPhysics( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const CBaseEntity *ignore, 
					 int collisionGroup, trace_t *ptr, float minMass );

ConVar	g_debug_antlionguard( "g_debug_antlionguard", "0" );
ConVar	sk_antlionguard_dmg_charge( "sk_antlionguard_dmg_charge", "0" );
ConVar	sk_antlionguard_dmg_shove( "sk_antlionguard_dmg_shove", "0" );

// Spawnflags 
#define	SF_ANTLIONGUARD_SERVERSIDE_RAGDOLL	( 1 << 16 )
#define SF_ANTLIONGUARD_INSIDE_FOOTSTEPS	( 1 << 17 )

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())
#define	ANTLIONGUARD_MODEL		"models/antlion_guard.mdl"
#define	MIN_BLAST_DAMAGE		25.0f
#define MIN_CRUSH_DAMAGE		20.0f

//==================================================
//
// Antlion Guard
//
//==================================================

#define ANTLIONGUARD_MAX_OBJECTS				128
#define	ANTLIONGUARD_MIN_OBJECT_MASS			8
#define	ANTLIONGUARD_MAX_OBJECT_MASS			750
#define	ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT	350
#define ANTLIONGUARD_OBJECTFINDING_FOV			DOT_45DEGREE // 1/sqrt(2)

//Melee definitions
#define	ANTLIONGUARD_MELEE1_RANGE		156.0f
#define	ANTLIONGUARD_MELEE1_CONE		0.7f

// Antlion summoning
#define ANTLIONGUARD_SUMMON_COUNT		3

// Sight
#define	ANTLIONGUARD_FOV_NORMAL			-0.4f

#define	ANTLIONGUARD_CHARGE_MIN			256
#define	ANTLIONGUARD_CHARGE_MAX			2048

ConVar	sk_antlionguard_health( "sk_antlionguard_health", "0" );

//==================================================
// AntlionGuardSchedules
//==================================================

enum
{
	SCHED_ANTLIONGUARD_CHARGE = LAST_SHARED_SCHEDULE,
	SCHED_ANTLIONGUARD_CHARGE_CRASH,
	SCHED_ANTLIONGUARD_CHARGE_CANCEL,
	SCHED_ANTLIONGUARD_PHYSICS_ATTACK,
	SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,
	SCHED_ANTLIONGUARD_UNBURROW,
	SCHED_ANTLIONGUARD_CHARGE_TARGET,
	SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,
	SCHED_ANTLIONGUARD_MELEE_ATTACK1,
	SCHED_ANTLIONGUARD_SUMMON,
	SCHED_ANTLIONGUARD_PATROL_RUN,
	SCHED_ANTLIONGUARD_ROAR,
	SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE,
	SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK,
	SCHED_ANTLIONGUARD_CANT_ATTACK,
};


//==================================================
// AntlionGuardTasks
//==================================================

enum
{
	TASK_ANTLIONGUARD_CHARGE = LAST_SHARED_TASK,
	TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT,
	TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT,
	TASK_ANTLIONGUARD_SUMMON,
	TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY,
	TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION,
	TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE,
	TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE,
	TASK_ANTLIONGUARD_OPPORTUNITY_THROW,
	TASK_ANTLIONGUARD_FIND_PHYSOBJECT,
};

//==================================================
// AntlionGuardConditions
//==================================================

enum
{
	COND_ANTLIONGUARD_PHYSICS_TARGET = LAST_SHARED_CONDITION,
	COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID,
	COND_ANTLIONGUARD_HAS_CHARGE_TARGET,
	COND_ANTLIONGUARD_CAN_SUMMON,
};

//==================================================
// AntlionGuard Activities
//==================================================

Activity ACT_ANTLIONGUARD_SEARCH;
Activity ACT_ANTLIONGUARD_PEEK_FLINCH;
Activity ACT_ANTLIONGUARD_PEEK_ENTER;
Activity ACT_ANTLIONGUARD_PEEK_EXIT;
Activity ACT_ANTLIONGUARD_PEEK1;
Activity ACT_ANTLIONGUARD_BARK;
Activity ACT_ANTLIONGUARD_PEEK_SIGHTED;
Activity ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT;
Activity ACT_ANTLIONGUARD_FLINCH_LIGHT;
Activity ACT_ANTLIONGUARD_UNBURROW;
Activity ACT_ANTLIONGUARD_ROAR;
Activity ACT_ANTLIONGUARD_RUN_HURT;

// Flinches
Activity ACT_ANTLIONGUARD_PHYSHIT_FR;
Activity ACT_ANTLIONGUARD_PHYSHIT_FL;
Activity ACT_ANTLIONGUARD_PHYSHIT_RR;
Activity ACT_ANTLIONGUARD_PHYSHIT_RL;

// Charge
Activity ACT_ANTLIONGUARD_CHARGE_START;
Activity ACT_ANTLIONGUARD_CHARGE_CANCEL;
Activity ACT_ANTLIONGUARD_CHARGE_RUN;
Activity ACT_ANTLIONGUARD_CHARGE_CRASH;
Activity ACT_ANTLIONGUARD_CHARGE_STOP;
Activity ACT_ANTLIONGUARD_CHARGE_HIT;
Activity ACT_ANTLIONGUARD_CHARGE_ANTICIPATION;

// Anim events
int AE_ANTLIONGUARD_CHARGE_HIT;
int AE_ANTLIONGUARD_SHOVE_PHYSOBJECT;
int AE_ANTLIONGUARD_SHOVE;
int AE_ANTLIONGUARD_FOOTSTEP_LIGHT;
int AE_ANTLIONGUARD_FOOTSTEP_HEAVY;
int AE_ANTLIONGUARD_CHARGE_EARLYOUT;
int AE_ANTLIONGUARD_VOICE_GROWL;
int AE_ANTLIONGUARD_VOICE_BARK;
int AE_ANTLIONGUARD_VOICE_PAIN;
int AE_ANTLIONGUARD_VOICE_SQUEEZE;
int AE_ANTLIONGUARD_VOICE_SCRATCH;
int AE_ANTLIONGUARD_VOICE_GRUNT;
int AE_ANTLIONGUARD_VOICE_ROAR;
int AE_ANTLIONGUARD_BURROW_OUT;

struct PhysicsObjectCriteria_t
{
	CBaseEntity *pTarget;
	float	flRadius;
	float	flTargetCone;
	bool	bPreferObjectsAlongTargetVector;  // Prefer objects that we can strike easily as we move towards our target
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_AntlionGuard : public CAI_BlendedNPC
{
public:
	DECLARE_CLASS( CNPC_AntlionGuard, CAI_BlendedNPC );
	DECLARE_DATADESC();

			CNPC_AntlionGuard( void );

	Class_T	Classify( void ) { return CLASS_ANTLION; }
	virtual int		GetSoundInterests( void ) { return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER); }
	virtual bool	QueryHearSound( CSound *pSound );

	const impactdamagetable_t &GetPhysicsImpactDamageTable( void );

	int		MeleeAttack1Conditions( float flDot, float flDist );
	int		TranslateSchedule( int scheduleType );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	DeathSound( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );
	int		SelectSchedule( void );

	virtual float GetAutoAimRadius() { return 36.0f; }
	
	void	Precache( void );
	void	Spawn( void );
	void	Activate( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	void	PrescheduleThink( void );
	void	GatherConditions( void );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	void	StopLoopingSounds();
	
	// Input handlers.
	void	InputSetShoveTarget( inputdata_t &inputdata );
	void	InputSetChargeTarget( inputdata_t &inputdata );
	void	InputClearChargeTarget( inputdata_t &inputdata );
	void	InputUnburrow( inputdata_t &inputdata );
	void	InputRagdoll( inputdata_t &inputdata );
	void	InputEnableBark( inputdata_t &inputdata );
	void	InputDisableBark( inputdata_t &inputdata );
	void	InputSummonedAntlionDied( inputdata_t &inputdata );

	bool	IsLightDamage( const CTakeDamageInfo &info );
	bool	IsHeavyDamage( const CTakeDamageInfo &info );
	bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool	BecomeRagdollOnClient( const Vector &force );

	float	MaxYawSpeed( void );
	bool	OverrideMove( float flInterval );
	bool	CanBecomeRagdoll( void );

	bool	ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity );

	Activity	NPC_TranslateActivity( Activity baseAct );

	DEFINE_CUSTOM_AI;

private:

	void	Footstep( bool bHeavy );
	int		SelectCombatSchedule( void );
	int		SelectUnreachableSchedule( void );
	bool	CanSummon( bool bIgnoreTime );
	void	SummonAntlions( void );
					
	void	ChargeLookAhead( void );
	bool	EnemyIsRightInFrontOfMe( CBaseEntity **pEntity );
	bool	HandleChargeImpact( Vector vecImpact, CBaseEntity *pEntity );
	bool	ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime, bool bCheckForCancel );
	bool	ShouldWatchEnemy( void );
					
	void	ImpactShock( const Vector &origin, float radius, float magnitude, CBaseEntity *pIgnored = NULL );
	void	BuildScheduleTestBits( void );
	void	Shove( void );
	void	FoundEnemy( void );
	void	LostEnemy( void );
	void	UpdateHead( void );
	void	UpdatePhysicsTarget( bool bPreferObjectsAlongTargetVector, float flRadius = ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT );
	void	MaintainPhysicsTarget( void );
	void	ChargeDamage( CBaseEntity *pTarget );
	void	StartSounds( void );
	void	SetHeavyDamageAnim( const Vector &vecSource );
	float	ChargeSteer( void );

	CBaseEntity *FindPhysicsObjectTarget( const PhysicsObjectCriteria_t &criteria );
	Vector	GetPhysicsHitPosition( CBaseEntity *pObject, CBaseEntity *pTarget, Vector *vecTrajectory, float *flClearDistance );
	bool	CanStandAtShoveTarget( CBaseEntity *pShoveObject, CBaseEntity *pTarget, Vector *pOut );

	int				m_nFlinchActivity;

	bool			m_bStopped;
	bool			m_bIsBurrowed;
	bool			m_bBarkEnabled;
	float			m_flNextSummonTime;
	int				m_iNumLiveAntlions;
							
	float			m_flSearchNoiseTime;
	float			m_flAngerNoiseTime;
	float			m_flBreathTime;
	float			m_flChargeTime;
	float			m_flPhysicsCheckTime;
	float			m_flNextHeavyFlinchTime;
	float			m_flNextRoarTime;
	int				m_iChargeMisses;
	bool			m_bDecidedNotToStop;
	bool			m_bCavernBreed;			// If this guard is meant to be a cavern dweller (uses different assets)
	bool			m_bInCavern;			// Behavioral hint telling the guard to change his behavior
					
	Vector			m_vecPhysicsTargetStartPos;
	Vector			m_vecPhysicsHitPosition;
					
	EHANDLE			m_hShoveTarget;
	EHANDLE			m_hChargeTarget;
	EHANDLE			m_hChargeTargetPosition;
	EHANDLE			m_hOldTarget;
	EHANDLE			m_hLastFailedPhysicsTarget;
	EHANDLE			m_hPhysicsTarget;
					
	COutputEvent	m_OnSummon;

	CSoundPatch		*m_pGrowlHighSound;
	CSoundPatch		*m_pGrowlLowSound;
	CSoundPatch		*m_pGrowlIdleSound;
	CSoundPatch		*m_pBreathSound;
	CSoundPatch		*m_pConfusedSound;

	string_t		m_iszPhysicsPropClass;
};

//==================================================
// CNPC_AntlionGuard::m_DataDesc
//==================================================

BEGIN_DATADESC( CNPC_AntlionGuard )

	DEFINE_FIELD( m_nFlinchActivity,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bStopped,			FIELD_BOOLEAN ), 	
	DEFINE_KEYFIELD( m_bIsBurrowed,		FIELD_BOOLEAN, "startburrowed" ),
	DEFINE_KEYFIELD( m_bBarkEnabled,	FIELD_BOOLEAN, "allowbark" ),
	DEFINE_FIELD( m_flNextSummonTime,	FIELD_TIME ),
	DEFINE_FIELD( m_iNumLiveAntlions,	FIELD_INTEGER ),

	DEFINE_FIELD( m_flSearchNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( m_flAngerNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( m_flBreathTime,		FIELD_TIME ), 
	DEFINE_FIELD( m_flChargeTime,		FIELD_TIME ),

	DEFINE_FIELD( m_hShoveTarget,				FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hChargeTarget,				FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hChargeTargetPosition,		FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hOldTarget,					FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLastFailedPhysicsTarget,	FIELD_EHANDLE ),

	DEFINE_FIELD( m_hPhysicsTarget,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecPhysicsTargetStartPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPhysicsHitPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flPhysicsCheckTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flNextHeavyFlinchTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flNextRoarTime,				FIELD_TIME ),
	DEFINE_FIELD( m_iChargeMisses,				FIELD_INTEGER ),
	DEFINE_FIELD( m_bDecidedNotToStop,			FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_bCavernBreed,FIELD_BOOLEAN, "cavernbreed" ),
	DEFINE_KEYFIELD( m_bInCavern,	FIELD_BOOLEAN, "incavern" ),

	DEFINE_OUTPUT( m_OnSummon,			"OnSummon" ),

	DEFINE_SOUNDPATCH( m_pGrowlHighSound ),
	DEFINE_SOUNDPATCH( m_pGrowlLowSound ),
	DEFINE_SOUNDPATCH( m_pGrowlIdleSound ),
	DEFINE_SOUNDPATCH( m_pBreathSound ),
	DEFINE_SOUNDPATCH( m_pConfusedSound ),

	DEFINE_INPUTFUNC( FIELD_STRING,	"SetShoveTarget", InputSetShoveTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetChargeTarget", InputSetChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearChargeTarget", InputClearChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Unburrow", InputUnburrow ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Ragdoll", InputRagdoll ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableBark", InputEnableBark ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableBark", InputDisableBark ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SummonedAntlionDied", InputSummonedAntlionDied ),

END_DATADESC()


//Fast Growl (Growl High)
envelopePoint_t envAntlionGuardFastGrowl[] =
{
	{	1.0f, 1.0f,
		0.2f, 0.4f,
	},
	{	0.1f, 0.1f,
		0.8f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.8f,
	},
};


//Bark 1 (Growl High)
envelopePoint_t envAntlionGuardBark1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Bark 2 (Confused)
envelopePoint_t envAntlionGuardBark2[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.2f, 0.3f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Pain
envelopePoint_t envAntlionGuardPain1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	-1.0f, -1.0f,
		0.5f, 0.8f,
	},
	{	0.1f, 0.2f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.5f, 0.75f,
	},
};

//Squeeze (High Growl)
envelopePoint_t envAntlionGuardSqueeze[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Scratch (Low Growl)
envelopePoint_t envAntlionGuardScratch[] =
{
	{	1.0f, 1.0f,
		0.4f, 0.6f,
	},
	{	0.5f, 0.5f,
		0.4f, 0.6f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Grunt
envelopePoint_t envAntlionGuardGrunt[] =
{
	{	0.6f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.1f, 0.2f,
	},
};

envelopePoint_t envAntlionGuardGrunt2[] =
{
	{	0.2f, 0.4f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//==============================================================================================
// ANTLION GUARD PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t antlionGuardLinearTable[] =
{
	{ 100*100,	10 },
	{ 250*250,	25 },
	{ 350*350,	50 },
	{ 500*500,	75 },
	{ 1000*1000,100 },
};

static impactentry_t antlionGuardAngularTable[] =
{
	{  50* 50, 10 },
	{ 100*100, 25 },
	{ 150*150, 50 },
	{ 200*200, 75 },
};

impactdamagetable_t gAntlionGuardImpactDamageTable =
{
	antlionGuardLinearTable,
	antlionGuardAngularTable,
	
	ARRAYSIZE(antlionGuardLinearTable),
	ARRAYSIZE(antlionGuardAngularTable),

	200*200,// minimum linear speed squared
	180*180,// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	15,		// can't take damage from anything under 15kg

	10,		// anything less than 10kg is "small"
	5,		// never take more than 1 pt of damage from anything under 15kg
	128*128,// <15kg objects must go faster than 36 in/s to do damage

	45,		// large mass in kg 
	2,		// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
	1,		// large mass falling scale
	0,		// my min velocity
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const impactdamagetable_t
//-----------------------------------------------------------------------------
const impactdamagetable_t &CNPC_AntlionGuard::GetPhysicsImpactDamageTable( void )
{
	return gAntlionGuardImpactDamageTable;
}

//==================================================
// CNPC_AntlionGuard
//==================================================

CNPC_AntlionGuard::CNPC_AntlionGuard( void ) : m_bCavernBreed( false ), m_bInCavern( false )
{
	m_iszPhysicsPropClass = AllocPooledString( "prop_physics" );
}

LINK_ENTITY_TO_CLASS( npc_antlionguard, CNPC_AntlionGuard );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Precache( void )
{
	PrecacheModel( ANTLIONGUARD_MODEL );

	PrecacheScriptSound( "NPC_AntlionGuard.Shove" );
	PrecacheScriptSound( "NPC_AntlionGuard.HitHard" );

	if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
	{
		PrecacheScriptSound( "NPC_AntlionGuard.Inside.StepLight" );
		PrecacheScriptSound( "NPC_AntlionGuard.Inside.StepHeavy" );
	}
	else
	{
		PrecacheScriptSound( "NPC_AntlionGuard.StepLight" );
		PrecacheScriptSound( "NPC_AntlionGuard.StepHeavy" );
	}

#if HL2_EPISODIC
	PrecacheScriptSound( "NPC_AntlionGuard.NearStepLight" );
	PrecacheScriptSound( "NPC_AntlionGuard.NearStepHeavy" );
	PrecacheScriptSound( "NPC_AntlionGuard.FarStepLight" );
	PrecacheScriptSound( "NPC_AntlionGuard.FarStepHeavy" );
	PrecacheScriptSound( "NPC_AntlionGuard.BreatheLoop" );
#endif // HL2_EPISODIC

	PrecacheScriptSound( "NPC_AntlionGuard.Anger" );
	PrecacheScriptSound( "NPC_AntlionGuard.Roar" );
	PrecacheScriptSound( "NPC_AntlionGuard.Die" );

	PrecacheScriptSound( "NPC_AntlionGuard.GrowlHigh" );
	PrecacheScriptSound( "NPC_AntlionGuard.GrowlIdle" );
	PrecacheScriptSound( "NPC_AntlionGuard.BreathSound" );
	PrecacheScriptSound( "NPC_AntlionGuard.Confused" );
	PrecacheScriptSound( "NPC_AntlionGuard.Fallover" );

	PrecacheScriptSound( "NPC_AntlionGuard.FrustratedRoar" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Spawn( void )
{
	Precache();

	// Switch our skin (for now), if we're the cavern guard
	if ( m_bCavernBreed )
	{
		m_nSkin = 1;
	}

	SetModel( ANTLIONGUARD_MODEL );

	SetHullType( HULL_LARGE );
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	SetNavType( NAV_GROUND );
	SetBloodColor( BLOOD_COLOR_YELLOW );

	m_iHealth = sk_antlionguard_health.GetFloat();
	m_iMaxHealth = m_iHealth;
	m_flFieldOfView	= ANTLIONGUARD_FOV_NORMAL;
	
	m_flPhysicsCheckTime = 0;
	m_flChargeTime = 0;
	m_flNextRoarTime = 0;
	m_flNextSummonTime = 0;
	m_iNumLiveAntlions = 0;
	m_iChargeMisses = 0;
	m_flNextHeavyFlinchTime = 0;
	m_bDecidedNotToStop = false;

	ClearHintGroup();
	
	m_bStopped = false;

	m_hShoveTarget = NULL;
	m_hChargeTarget = NULL;
	m_hChargeTargetPosition = NULL;
	m_hPhysicsTarget = NULL;
	
	m_hLastFailedPhysicsTarget = NULL;

	m_HackedGunPos.x		= 10;
	m_HackedGunPos.y		= 0;
	m_HackedGunPos.z		= 30;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_SKIP_NAV_GROUND_CHECK );

	NPCInit();

	BaseClass::Spawn();

	//See if we're supposed to start burrowed
	if ( m_bIsBurrowed )
	{
		AddEffects( EF_NODRAW );
		AddFlag( FL_NOTARGET );

		m_spawnflags |= SF_NPC_GAG;
		
		AddSolidFlags( FSOLID_NOT_SOLID );
		
		m_takedamage = DAMAGE_NO;
	}

	// Do not dissolve
	AddEFlags( EFL_NO_DISSOLVE );

	// We get a minute of free knowledge about the target
	GetEnemies()->SetEnemyDiscardTime( 120.0f );
	GetEnemies()->SetFreeKnowledgeDuration( 60.0f );

	// We need to bloat the absbox to encompass all the hitboxes
	Vector absMin = -Vector(100,100,0);
	Vector absMax = Vector(100,100,128);

	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &absMin, &absMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Activate( void )
{
	BaseClass::Activate();

	// Find all nearby physics objects and add them to the list of objects we will sense
	CBaseEntity *pObject = NULL;
	while ( ( pObject = gEntList.FindEntityInSphere( pObject, WorldSpaceCenter(), 2500 ) ) != NULL )
	{
		// Can't throw around debris
		if ( pObject->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
			continue;

		// We can only throw a few types of things
		if ( !FClassnameIs( pObject, "prop_physics" ) && !FClassnameIs( pObject, "func_physbox" ) )
			continue;

		// Ensure it's mass is within range
		IPhysicsObject *pPhysObj = pObject->VPhysicsGetObject();
		if( ( pPhysObj == NULL ) || ( pPhysObj->GetMass() > ANTLIONGUARD_MAX_OBJECT_MASS ) || ( pPhysObj->GetMass() < ANTLIONGUARD_MIN_OBJECT_MASS ) )
			continue;

		// Tell the AI sensing list that we want to consider this
		g_AI_SensedObjectsManager.AddEntity( pObject );

		if ( g_debug_antlionguard.GetInt() == 5 )
		{
			Msg("Antlion Guard: Added prop with model '%s' to sense list.\n", STRING(pObject->GetModelName()) );
			pObject->m_debugOverlays |= OVERLAY_BBOX_BIT;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the guard's allowed to summon antlions
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::CanSummon( bool bIgnoreTime )
{
	if ( !m_bBarkEnabled )
		return false;

	if ( !bIgnoreTime && m_flNextSummonTime > gpGlobals->curtime )
		return false;

	// Hit the max number of them allowed? Only summon when we're 2 down.
	if ( m_iNumLiveAntlions >= max(1, ANTLIONGUARD_SUMMON_COUNT-1) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Our enemy is unreachable. Select a schedule.
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::SelectUnreachableSchedule( void )
{
	// Summon antlions if we can
	if ( HasCondition(COND_ANTLIONGUARD_CAN_SUMMON) )
		return SCHED_ANTLIONGUARD_SUMMON;

	// Otherwise, look for a physics objects nearby
	m_hLastFailedPhysicsTarget = NULL;
	UpdatePhysicsTarget( false );
	if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) )
		return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;

	// Otherwise, roar at the player
	if ( HasCondition(COND_SEE_ENEMY) && m_flNextRoarTime < gpGlobals->curtime )
	{
		if ( hl2_episodic.GetBool() == true && m_bInCavern == false )
		{
			if ( GetEnemy() )
			{
				Vector dir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize(dir);

				GetMotor()->SetIdealYaw( -dir );
			}

			return SCHED_MOVE_AWAY;
		}

		// Don't roar in a cavern setting
		if ( hl2_episodic.GetBool() == false || m_bInCavern == false )
		{
			m_flNextRoarTime = gpGlobals->curtime + RandomFloat( 20,40 );
			return SCHED_ANTLIONGUARD_ROAR;
		}
	}

	return SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::SelectCombatSchedule( void )
{
	ClearHintGroup();

	bool bCanCharge = false;
	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		bCanCharge = ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), true, false );
	}

	// Attack if we can
	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	// Otherwise, summon antlions
	if ( HasCondition(COND_ANTLIONGUARD_CAN_SUMMON) )
	{
		// If I can charge, and have antlions, charge instead
		if ( bCanCharge && m_iNumLiveAntlions )
			return SCHED_ANTLIONGUARD_CHARGE;

		return SCHED_ANTLIONGUARD_SUMMON;
	}

	// See if we can bark
	if ( HasCondition( COND_ENEMY_UNREACHABLE ) )
		return SelectUnreachableSchedule();

	//Physics target
	if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) )
		return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;

	// If we've missed a couple of times, and we can summon, make it harder
	if ( m_iChargeMisses >= 2 && CanSummon(true) )
	{
		m_iChargeMisses--;
		return SCHED_ANTLIONGUARD_SUMMON;
	}

	// Charging
	if ( bCanCharge )
		return SCHED_ANTLIONGUARD_CHARGE;

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime, bool bCheckForCancel )
{
	// Don't charge in tight spaces
	if ( hl2_episodic.GetBool() && m_bInCavern )
		return false;

	// Must have a target
	if ( !GetEnemy() )
		return false;

	// Don't check the distance once we start charging
	if ( !bCheckForCancel )
	{
		// Don't allow use to charge again if it's been too soon
		if ( useTime && ( m_flChargeTime > gpGlobals->curtime ) )
			return false;

		float distance = UTIL_DistApprox2D( startPos, endPos );

		// Must be within our tolerance range
		if ( ( distance < ANTLIONGUARD_CHARGE_MIN ) || ( distance > ANTLIONGUARD_CHARGE_MAX ) )
			return false;
	}

	//FIXME: We'd like to exclude small physics objects from this check!

	// We only need to hit the endpos with the edge of our bounding box
	Vector vecDir = endPos - startPos;
	VectorNormalize( vecDir );
	float flWidth = WorldAlignSize().x * 0.5;
	Vector vecTargetPos = endPos - (vecDir * flWidth);

	// See if we can directly move there
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_GROUND, startPos, vecTargetPos, MASK_NPCSOLID_BRUSHONLY, GetEnemy(), &moveTrace );
	
	// Draw the probe
	if ( g_debug_antlionguard.GetInt() == 1 )
	{
		Vector	enemyDir	= (vecTargetPos - startPos);
		float	enemyDist	= VectorNormalize( enemyDir );

		NDebugOverlay::BoxDirection( startPos, GetHullMins(), GetHullMaxs() + Vector(enemyDist,0,0), enemyDir, 0, 255, 0, 8, 1.0f );
	}

	// If we're not blocked, charge
	if ( IsMoveBlocked( moveTrace ) )
	{
		// Don't allow it if it's too close to us
		if ( UTIL_DistApprox( WorldSpaceCenter(), moveTrace.vEndPosition ) < ANTLIONGUARD_CHARGE_MIN )
			return false;

		// Allow some special cases to not block us
		if ( moveTrace.pObstruction != NULL )
		{
			// If we've hit the world, see if it's a cliff
			if ( moveTrace.pObstruction == GetContainingEntity( INDEXENT(0) ) )
			{	
				// Can't be too far above/below the target
				if ( fabs( moveTrace.vEndPosition.z - vecTargetPos.z ) > StepHeight() )
					return false;

				// Allow it if we got pretty close
				if ( UTIL_DistApprox( moveTrace.vEndPosition, vecTargetPos ) < 64 )
					return true;
			}

			// Hit things that will take damage
			if ( moveTrace.pObstruction->m_takedamage != DAMAGE_NO )
				return true;

			// Hit things that will move
			if ( moveTrace.pObstruction->GetMoveType() == MOVETYPE_VPHYSICS )
				return true;
		}

		return false;
	}

	// Only update this if we've requested it
	if ( useTime )
	{
		m_flChargeTime = gpGlobals->curtime + 4.0f;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::SelectSchedule( void )
{
	// Don't do anything if we're burrowed
	if ( m_bIsBurrowed )
		return SCHED_IDLE_STAND;

	// Debug physics object finding
	if ( 0 ) //g_debug_antlionguard.GetInt() == 3 )
	{
		m_flPhysicsCheckTime = 0;
		UpdatePhysicsTarget( false );
		return SCHED_IDLE_STAND;
	}

	// Flinch on heavy damage, but not if we've flinched too recently.
	// This is done to prevent stun-locks from grenades.
	if ( HasCondition( COND_HEAVY_DAMAGE ) && m_flNextHeavyFlinchTime < gpGlobals->curtime )
	{
		m_flNextHeavyFlinchTime = gpGlobals->curtime + 8.0;
		return SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY;
	}

	// Charge after a target if it's set
	if ( m_hChargeTarget && m_hChargeTargetPosition )
	{
		ClearCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
		ClearHintGroup();
		
		if ( m_hChargeTarget->IsAlive() == false )
		{
			m_hChargeTarget	= NULL;
			m_hChargeTargetPosition = NULL;
			SetEnemy( m_hOldTarget );
			
			if ( m_hOldTarget == NULL )
			{
				m_NPCState = NPC_STATE_ALERT;
			}
		}
		else
		{
			m_hOldTarget = GetEnemy();
			SetEnemy( m_hChargeTarget );

			//If we can't see the target, run to somewhere we can
			if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false, false ) == false )
				return SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION;

			return SCHED_ANTLIONGUARD_CHARGE_TARGET;
		}
	}

	// See if we need to clear a path to our enemy
	if ( HasCondition( COND_ENEMY_OCCLUDED ) || HasCondition( COND_ENEMY_UNREACHABLE ) )
	{
		CBaseEntity *pBlocker = GetEnemyOccluder();

		if ( ( pBlocker != NULL ) && FClassnameIs( pBlocker, "prop_physics" ) )
		{
			m_hPhysicsTarget = pBlocker;
			return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;
		}
	}

	//Only do these in combat states
	if ( m_NPCState == NPC_STATE_COMBAT && GetEnemy() )
		return SelectCombatSchedule();

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::MeleeAttack1Conditions( float flDot, float flDist )
{
	// Don't attack again too soon
	if ( GetNextAttack() > gpGlobals->curtime )
		return 0;

	// While charging, we can't melee attack
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return 0;

	if ( hl2_episodic.GetBool() && m_bInCavern )
	{
		// Predict where they'll be and see if THAT is within range
		Vector vecPredPos;
		UTIL_PredictedPosition( GetEnemy(), 0.25f, &vecPredPos );
		if ( ( GetAbsOrigin() - vecPredPos ).Length() > ANTLIONGUARD_MELEE1_RANGE )
			return COND_TOO_FAR_TO_ATTACK;
	}
	else
	{
		// Must be close enough
		if ( flDist > ANTLIONGUARD_MELEE1_RANGE )
			return COND_TOO_FAR_TO_ATTACK;
	}

	// Must be within a viable cone
	if ( flDot < ANTLIONGUARD_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	// If the enemy is on top of me, I'm allowed to hit the sucker
	if ( GetEnemy()->GetGroundEntity() == this )
		return COND_CAN_MELEE_ATTACK1;

	trace_t	tr;
	TraceHull_SkipPhysics( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5 );

	// If we hit anything, go for it
	if ( tr.fraction < 1.0f )
		return COND_CAN_MELEE_ATTACK1;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_AntlionGuard::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	// Stay still
	if (( eActivity == ACT_ANTLIONGUARD_SEARCH ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_ENTER ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_EXIT ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK1 ) || 
		( eActivity == ACT_ANTLIONGUARD_BARK ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_SIGHTED ) || 
		( eActivity == ACT_MELEE_ATTACK1 ) )
		return 0.0f;

	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy != NULL && pEnemy->IsPlayer() == false )
		return 16.0f;

	// Turn slowly when you're charging
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
		return 4.0f;

	if ( hl2_episodic.GetBool() && m_bInCavern )
	{
		// Allow a better turning rate when moving quickly but not charging the player
		if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN ) && IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) == false )
			return 16.0f;
	}

	// Turn more slowly as we close in on our target
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN )
	{
		if ( pEnemy == NULL )
			return 2.0f;

		float dist = UTIL_DistApprox2D( GetEnemy()->WorldSpaceCenter(), WorldSpaceCenter() );

		if ( dist > 512 )
			return 16.0f;

		//FIXME: Alter by skill level
		float yawSpeed = RemapVal( dist, 0, 512, 1.0f, 2.0f );
		yawSpeed = clamp( yawSpeed, 1.0f, 2.0f );

		return yawSpeed;
	}

	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP )
		return 8.0f;

	switch( eActivity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 40.0f;
		break;
	
	case ACT_RUN:
	default:
		return 20.0f;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::OverrideMove( float flInterval )
{
	// If the guard's charging, we're handling the movement
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return true;

	return BaseClass::OverrideMove( flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Shove( void )
{
	if ( GetNextAttack() > gpGlobals->curtime )
		return;

	CBaseEntity *pHurt = NULL;
	CBaseEntity *pTarget;

	pTarget = ( m_hShoveTarget == NULL ) ? GetEnemy() : m_hShoveTarget.Get();

	if ( pTarget == NULL )
	{
		m_hShoveTarget = NULL;
		return;
	}

	//Always damage bullseyes if we're scripted to hit them
	if ( pTarget->Classify() == CLASS_BULLSEYE )
	{
		pTarget->TakeDamage( CTakeDamageInfo( this, this, 1.0f, DMG_CRUSH ) );
		m_hShoveTarget = NULL;
		return;
	}

	float damage = ( pTarget->IsPlayer() ) ? sk_antlionguard_dmg_shove.GetFloat() : 250;

	// If the target's still inside the shove cone, ensure we hit him	
	Vector vecForward, vecEnd;
	AngleVectors( GetAbsAngles(), &vecForward );
  	float flDistSqr = ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr();
  	Vector2D v2LOS = ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).AsVector2D();
  	Vector2DNormalize(v2LOS);
  	float flDot	= DotProduct2D (v2LOS, vecForward.AsVector2D() );
	if ( flDistSqr < (ANTLIONGUARD_MELEE1_RANGE*ANTLIONGUARD_MELEE1_RANGE) && flDot >= ANTLIONGUARD_MELEE1_CONE )
	{
		vecEnd = pTarget->WorldSpaceCenter();
	}
	else
	{
		vecEnd = WorldSpaceCenter() + ( BodyDirection3D() * ANTLIONGUARD_MELEE1_RANGE );
	}

	// Use the melee trace to ensure we hit everything there
	trace_t tr;
	CTakeDamageInfo	dmgInfo( this, this, damage, DMG_SLASH );
	CTraceFilterMelee traceFilter( this, COLLISION_GROUP_NONE, &dmgInfo, 1.0, true );
	Ray_t ray;
	ray.Init( WorldSpaceCenter(), vecEnd, Vector(-16,-16,-16), Vector(16,16,16) );
	enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );
	pHurt = tr.m_pEnt;

	// Knock things around
	ImpactShock( tr.endpos, 100.0f, 250.0f );

	if ( pHurt )
	{
		Vector	traceDir = ( tr.endpos - tr.startpos );
		VectorNormalize( traceDir );

		// Generate enough force to make a 75kg guy move away at 600 in/sec
		Vector vecForce = traceDir * ImpulseScale( 75, 600 );
		CTakeDamageInfo info( this, this, vecForce, tr.endpos, damage, DMG_CLUB );
		pHurt->TakeDamage( info );

		m_hShoveTarget = NULL;

		EmitSound( "NPC_AntlionGuard.Shove" );

		// If the player, throw him around
		if ( pHurt->IsPlayer() )
		{
			//Punch the view
			pHurt->ViewPunch( QAngle(20,0,-20) );
			
			//Shake the screen
			UTIL_ScreenShake( pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );

			//Red damage indicator
			color32 red = {128,0,0,128};
			UTIL_ScreenFade( pHurt, red, 1.0f, 0.1f, FFADE_IN );

			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			pHurt->ApplyAbsVelocityImpulse( forward * 400 + up * 150 );
		}	
		else
		{
			CBaseCombatCharacter *pVictim = ToBaseCombatCharacter( pHurt );
			
			if ( pVictim )
			{
				if ( NPC_Rollermine_IsRollermine( pVictim ) )
				{
					Pickup_OnPhysGunDrop( pVictim, NULL, LAUNCHED_BY_CANNON );
				}

				// FIXME: This causes NPCs that are not physically motivated to hop into the air strangely -- jdw
				// pVictim->ApplyAbsVelocityImpulse( BodyDirection2D() * 400 + Vector( 0, 0, 250 ) );
			}

			m_hShoveTarget = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTraceFilterCharge : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCharge );
	
	CTraceFilterCharge( const IHandleEntity *passentity, int collisionGroup, CNPC_AntlionGuard *pAttacker )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_pAttacker(pAttacker)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			if ( pEntity->m_takedamage == DAMAGE_NO )
				return false;

			// Translate the vehicle into its driver for damage
			if ( pEntity->GetServerVehicle() != NULL )
			{
				CBaseEntity *pDriver = pEntity->GetServerVehicle()->GetPassenger();

				if ( pDriver != NULL )
				{
					pEntity = pDriver;
				}
			}
	
			Vector	attackDir = pEntity->WorldSpaceCenter() - m_pAttacker->WorldSpaceCenter();
			VectorNormalize( attackDir );

			float	flDamage = ( pEntity->IsPlayer() ) ? sk_antlionguard_dmg_shove.GetFloat() : 250;;

			CTakeDamageInfo info( m_pAttacker, m_pAttacker, flDamage, DMG_CRUSH );
			CalculateMeleeDamageForce( &info, attackDir, info.GetAttacker()->WorldSpaceCenter(), 4.0f );

			CBaseCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

			// Only do these comparisons between NPCs
			if ( pVictimBCC )
			{
				// Can only damage other NPCs that we hate
				if ( m_pAttacker->IRelationType( pEntity ) == D_HT )
				{
					pEntity->TakeDamage( info );
					return true;
				}
			}
			else
			{
				// Otherwise just damage passive objects in our way
				pEntity->TakeDamage( info );
				Pickup_ForcePlayerToDropThisObject( pEntity );
			}
		}

		return false;
	}

public:
	const IHandleEntity *m_pPassEnt;
	int					m_collisionGroup;
	CNPC_AntlionGuard	*m_pAttacker;
};

#define	MIN_FOOTSTEP_NEAR_DIST	80*12.0f // ft

//-----------------------------------------------------------------------------
// Purpose: Plays a footstep sound with temporary distance fades
// Input  : bHeavy - Larger back hoof is considered a "heavy" step
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Footstep( bool bHeavy )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	Assert( pPlayer != NULL );
	if ( pPlayer == NULL )
		return;

	float flDistanceToPlayer = ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length();
	float flNearVolume = RemapValClamped( flDistanceToPlayer, (10*12.0f), MIN_FOOTSTEP_NEAR_DIST, VOL_NORM, 0.0f );

	EmitSound_t soundParams;
	CPASAttenuationFilter filter( this );

	if ( bHeavy )
	{
		if ( flNearVolume > 0.0f )
		{
			soundParams.m_pSoundName = "NPC_AntlionGuard.NearStepHeavy";
			soundParams.m_flVolume = flNearVolume;
			soundParams.m_nFlags = SND_CHANGE_VOL;
			EmitSound( filter, entindex(), soundParams );
		}

		EmitSound( "NPC_AntlionGuard.FarStepHeavy" );
	}
	else
	{
		if ( flNearVolume > 0.0f )
		{
			soundParams.m_pSoundName = "NPC_AntlionGuard.NearStepLight";
			soundParams.m_flVolume = flNearVolume;
			soundParams.m_nFlags = SND_CHANGE_VOL;
			EmitSound( filter, entindex(), soundParams );
		}

		EmitSound( "NPC_AntlionGuard.FarStepLight" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_ANTLIONGUARD_CHARGE_EARLYOUT )
	{
		// Robin: Removed this because it usually made him look less intelligent, not more.
		//		  This code left here so we don't get warnings in the console.

		/*
		// Cancel the charge if it's no longer valid
		if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false, true ) == false )
		{
			SetSchedule( SCHED_ANTLIONGUARD_CHARGE_CANCEL );
		}
		*/

		return;
	}

	// Don't handle anim events after death
	if ( m_NPCState == NPC_STATE_DEAD )
	{
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	{
		if ( m_hPhysicsTarget == NULL )
		{
			// Disrupt other objects near us anyway
			ImpactShock( WorldSpaceCenter(), 150, 250.0f );
			return;
		}

		//Setup the throw velocity
		IPhysicsObject *physObj = m_hPhysicsTarget->VPhysicsGetObject();

		Vector	targetDir = ( GetEnemy()->GetAbsOrigin() - m_hPhysicsTarget->WorldSpaceCenter() );
		float	targetDist = VectorNormalize( targetDir );

		Vector shoveDir = m_hPhysicsTarget->WorldSpaceCenter() - WorldSpaceCenter();
		float shoveDist = VectorNormalize( shoveDir );

		// Must still be close enough to our target
		if ( shoveDist > 300.0f )
		{
			// Pick a new target next time (this one foiled us!)
			m_hLastFailedPhysicsTarget = m_hPhysicsTarget;
			m_hPhysicsTarget = NULL;
			return;
		}

		if ( targetDist < 512 )
			targetDist = 512;

		if ( targetDist > 1024 )
			targetDist = 1024;

		if ( NPC_Rollermine_IsRollermine( m_hPhysicsTarget ) )
		{
			Pickup_OnPhysGunDrop( m_hPhysicsTarget, NULL, LAUNCHED_BY_CANNON );
		}

		targetDir *= targetDist * 3 * physObj->GetMass();	//FIXME: Scale by skill
		targetDir[2] += physObj->GetMass() * 350.0f;
		
		//Display dust
		Vector vecRandom = RandomVector( -1, 1);
		VectorNormalize( vecRandom );
		g_pEffects->Dust( m_hPhysicsTarget->WorldSpaceCenter(), vecRandom, 64.0f, 32 );

		// If it's being held by the player, break that bond
		Pickup_ForcePlayerToDropThisObject( m_hPhysicsTarget );

		EmitSound( "NPC_AntlionGuard.HitHard" );

		//Send it flying
		AngularImpulse angVel( random->RandomFloat(-180, 180), 100, random->RandomFloat(-360, 360) );
		physObj->ApplyForceCenter( targetDir );
		physObj->AddVelocity( NULL, &angVel );
		
		//Clear the state information, we're done
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );

		// Disrupt other objects near it
		ImpactShock( m_hPhysicsTarget->WorldSpaceCenter(), 150, 250.0f, m_hPhysicsTarget );

		m_hPhysicsTarget = NULL;
		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_CHARGE_HIT )
	{
		UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 4.0f, 1.0f, 512, SHAKE_START );
		EmitSound( "NPC_AntlionGuard.HitHard" );

		Vector	startPos = GetAbsOrigin();
		float	checkSize = ( CollisionProp()->BoundingRadius() + 8.0f );
		Vector	endPos = startPos + ( BodyDirection3D() * checkSize );

		CTraceFilterCharge traceFilter( this, COLLISION_GROUP_NONE, this );

		Ray_t ray;
		ray.Init( startPos, endPos, GetHullMins(), GetHullMaxs() );

		trace_t tr;
		enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );

		if ( g_debug_antlionguard.GetInt() == 1 )
		{
			Vector hullMaxs = GetHullMaxs();
			hullMaxs.x += checkSize;

			NDebugOverlay::BoxDirection( startPos, GetHullMins(), hullMaxs, BodyDirection2D(), 100, 255, 255, 20, 1.0f );
		}

		//NDebugOverlay::Box3D( startPos, endPos, BodyDirection2D(), 
		if ( m_hChargeTarget && m_hChargeTarget->IsAlive() == false )
		{
			m_hChargeTarget = NULL;
			m_hChargeTargetPosition = NULL;
		}

		// Cause a shock wave from this point which will distrupt nearby physics objects
		ImpactShock( tr.endpos, 200, 500 );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_SHOVE )
	{
		EmitSound("NPC_AntlionGuard.StepLight", pEvent->eventtime );
		Shove();
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_FOOTSTEP_LIGHT )
	{
		if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
		{
#if HL2_EPISODIC
			Footstep( false );
#else 
			EmitSound("NPC_AntlionGuard.Inside.StepLight", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		else
		{
#if HL2_EPISODIC
			Footstep( false );
#else 
			EmitSound("NPC_AntlionGuard.StepLight", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_FOOTSTEP_HEAVY )
	{
		if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
		{
#if HL2_EPISODIC
			Footstep( true );
#else 
			EmitSound( "NPC_AntlionGuard.Inside.StepHeavy", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		else
		{
#if HL2_EPISODIC
			Footstep( true );
#else 
			EmitSound( "NPC_AntlionGuard.StepHeavy", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_GROWL )
	{
		StartSounds();

		float duration = 0.0f;

		if ( random->RandomInt( 0, 10 ) < 6 )
		{
			duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardFastGrowl, ARRAYSIZE(envAntlionGuardFastGrowl) );
		}
		else
		{
			duration = 1.0f;
			EmitSound( "NPC_AntlionGuard.FrustratedRoar" );
			ENVELOPE_CONTROLLER.SoundFadeOut( m_pGrowlHighSound, 0.5f, false );
		}
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + random->RandomFloat( 2.0f, 4.0f );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}
		

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_BARK )
	{
		StartSounds();

		float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark1, ARRAYSIZE(envAntlionGuardBark1) );
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + random->RandomFloat( 2.0f, 4.0f );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
		
		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_ROAR )
	{
		StartSounds();

		float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardFastGrowl, ARRAYSIZE(envAntlionGuardFastGrowl) );
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + random->RandomFloat( 2.0f, 4.0f );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;

		EmitSound( "NPC_AntlionGuard.Roar" );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_PAIN )
	{
		StartSounds();

		float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardPain1, ARRAYSIZE(envAntlionGuardPain1) );
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
		
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
		
		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_SQUEEZE )
	{	
		StartSounds();

		float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardSqueeze, ARRAYSIZE(envAntlionGuardSqueeze) );
		
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, random->RandomFloat( 2.0f, 4.0f ) );
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 60, 80 ), random->RandomFloat( 2.0f, 4.0f ) );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + ( duration * 0.5f );

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_SCRATCH )
	{	
		StartSounds();

		float duration = random->RandomFloat( 2.0f, 4.0f );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, duration );
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 60, 80 ), duration );

		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration;

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}
		
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_GRUNT )
	{	
		StartSounds();

		float duration = ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt, ARRAYSIZE(envAntlionGuardGrunt) );
		ENVELOPE_CONTROLLER.SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt2, ARRAYSIZE(envAntlionGuardGrunt2) );
		
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration;
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_BURROW_OUT )
	{
		EmitSound( "NPC_Antlion.BurrowOut" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		UTIL_CreateAntlionDust( GetAbsOrigin() + Vector(0,0,24), GetLocalAngles() );

		RemoveEffects( EF_NODRAW );
		RemoveFlag( FL_NOTARGET );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::SetHeavyDamageAnim( const Vector &vecSource )
{
	Vector vFacing = BodyDirection2D();
	Vector vDamageDir = ( vecSource - WorldSpaceCenter() );

	vDamageDir.z = 0.0f;

	VectorNormalize( vDamageDir );

	Vector vDamageRight, vDamageUp;
	VectorVectors( vDamageDir, vDamageRight, vDamageUp );

	float damageDot = DotProduct( vFacing, vDamageDir );
	float damageRightDot = DotProduct( vFacing, vDamageRight );

	// See if it's in front
	if ( damageDot > 0.0f )
	{
		// See if it's right
		if ( damageRightDot > 0.0f )
		{
			m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_FR;
		}
		else
		{
			m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_FL;
		}
	}
	else
	{
		// Otherwise it's from behind
		if ( damageRightDot < 0.0f )
		{
			m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_RR;
		}
		else
		{
			m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_RL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo dInfo = info;

	// Don't take damage from another antlion guard!
	if ( dInfo.GetAttacker() && FClassnameIs( dInfo.GetAttacker(), "npc_antlionguard" ) )
		return 0;

	// Hack to make antlion guard harder in HARD
	if ( g_pGameRules->IsSkillLevel(SKILL_HARD) && !(info.GetDamageType() & DMG_CRUSH) )
	{
		dInfo.SetDamage( dInfo.GetDamage() * 0.75 );
	}

	// Only take damage from what we classify as heavy damages (explosions, refrigerators, etc)
	if ( IsHeavyDamage( dInfo ) )
	{
		// Cap damage taken
		if ( dInfo.GetDamage() > 100 )
		{
			dInfo.SetDamage( 100 );
		}

		// Always take a set amount of damage from a combine ball
		if ( info.GetInflictor() && UTIL_IsCombineBall( info.GetInflictor() ) )
		{
			dInfo.SetDamage( 50 );
		}

		UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 8.0f, 0.5f, 512, SHAKE_START );

		// Set our response animation
		SetHeavyDamageAnim( dInfo.GetDamagePosition() );

		// Explosive barrels don't carry through their attacker, so this 
		// condition isn't set, and we still want to flinch. So we set it.
		SetCondition( COND_HEAVY_DAMAGE );

		return BaseClass::OnTakeDamage_Alive( dInfo );
	}
	
	return BaseClass::OnTakeDamage_Alive( dInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAttacker - 
//			flDamage - 
//			&vecDir - 
//			*ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	// Bullets are weak against us, buckshot less so
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		info.ScaleDamage( 0.5f );
	}
	else if ( info.GetDamageType() & DMG_BULLET )
	{
		info.ScaleDamage( 0.25f );
	}

	// Make sure we haven't rounded down to a minimal amount
	if ( info.GetDamage() < 1.0f )
	{
		info.SetDamage( 1.0f );
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY:
		SetIdealActivity( (Activity) m_nFlinchActivity );
		break;

	case TASK_ANTLIONGUARD_SUMMON:
		SummonAntlions();
		m_OnSummon.FireOutput( this, this, 0 );
		TaskComplete();
		break;

	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to shove a NULL physics target!\n" );
				break;
			}
			
			//Get the direction and distance to our thrown object
			Vector	dirToTarget = ( m_hPhysicsTarget->WorldSpaceCenter() - WorldSpaceCenter() );
			float	distToTarget = VectorNormalize( dirToTarget );
			dirToTarget.z = 0;

			//Validate it's still close enough to shove
			//FIXME: Real numbers
			if ( distToTarget > 256.0f )
			{
				m_hLastFailedPhysicsTarget = NULL;
				m_hPhysicsTarget = NULL;

				TaskFail( "Shove target moved\n" );
				break;
			}

			//Validate its offset from our facing
			float targetYaw = UTIL_VecToYaw( dirToTarget );
			float offset = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( GetLocalAngles().y ) );

			if ( fabs( offset ) > 55 )
			{
				m_hLastFailedPhysicsTarget = NULL;
				m_hPhysicsTarget = NULL;

				TaskFail( "Shove target off-center\n" );
				break;
			}

			//Blend properly
			SetPoseParameter( "throw", offset );

			//Start playing the animation
			SetActivity( ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT );
		}
		break;

	case TASK_ANTLIONGUARD_FIND_PHYSOBJECT:
		{
			// Force the antlion guard to find a physobject
			m_flPhysicsCheckTime = 0;
			UpdatePhysicsTarget( false, 1024 );
			if ( m_hPhysicsTarget )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( "Failed to find a physobject.\n" );
			}
		}
		break;

	case TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to find a path to NULL physics target!\n" );
				break;
			}
			
			Vector vecGoalPos = m_vecPhysicsHitPosition; 
			AI_NavGoal_t goal( GOALTYPE_LOCATION, vecGoalPos, ACT_RUN );

			if ( GetNavigator()->SetGoal( goal ) )
			{
				if ( g_debug_antlionguard.GetInt() == 1 )
				{
					NDebugOverlay::Cross3D( vecGoalPos, Vector(8,8,8), -Vector(8,8,8), 0, 255, 0, true, 2.0f );
					NDebugOverlay::Line( vecGoalPos, m_hPhysicsTarget->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
					NDebugOverlay::Line( m_hPhysicsTarget->WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
				}

				// Face the enemy
				GetNavigator()->SetArrivalDirection( GetEnemy() );
				TaskComplete();
			}
			else
			{
				if ( g_debug_antlionguard.GetInt() == 1 )
				{
					NDebugOverlay::Cross3D( vecGoalPos, Vector(8,8,8), -Vector(8,8,8), 255, 0, 0, true, 2.0f );
					NDebugOverlay::Line( vecGoalPos, m_hPhysicsTarget->WorldSpaceCenter(), 255, 0, 0, true, 2.0f );
					NDebugOverlay::Line( m_hPhysicsTarget->WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), 255, 0, 0, true, 2.0f );
				}

				m_hLastFailedPhysicsTarget = m_hPhysicsTarget;
				m_hPhysicsTarget = NULL;
				TaskFail( "Unable to navigate to physics attack target!\n" );
				break;
			}
		}
		break;

	case TASK_ANTLIONGUARD_CHARGE:
		{
			SetActivity( ACT_ANTLIONGUARD_CHARGE_START );
			m_bDecidedNotToStop = false;
		}
		break;


	case TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION:
		{
			if ( !m_hChargeTargetPosition )
			{
				TaskFail( "Tried to find a charge position without one specified.\n" );
				break;
			}

			// Move to the charge position
			AI_NavGoal_t goal( GOALTYPE_LOCATION, m_hChargeTargetPosition->GetAbsOrigin(), ACT_RUN );
			if ( GetNavigator()->SetGoal( goal ) )
			{
				// We want to face towards the charge target
				Vector vecDir = m_hChargeTarget->GetAbsOrigin() - m_hChargeTargetPosition->GetAbsOrigin();
				VectorNormalize( vecDir );
				vecDir.z = 0;
				GetNavigator()->SetArrivalDirection( vecDir );
				TaskComplete();
			}
			else
			{
				m_hChargeTarget = NULL;
				m_hChargeTargetPosition = NULL;
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE:
		{
			if ( !GetEnemy() )
			{
				TaskFail( FAIL_NO_ENEMY );
				break;
			}

			// Find the nearest node to the enemy
			int iNearestNode = GetPathfinder()->NearestNodeToPoint( GetEnemy()->GetAbsOrigin() );
			if ( iNearestNode != NO_NODE )
			{
				// Get the node and move to it
				CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( iNearestNode );
				if ( pNode )
				{
					Vector vecNodePos = pNode->GetPosition( GetHullType() );
					AI_NavGoal_t goal( GOALTYPE_LOCATION, vecNodePos, ACT_RUN );
					if ( GetNavigator()->SetGoal( goal ) )
					{
						GetNavigator()->SetArrivalDirection( GetEnemy() );
						TaskComplete();
						break;
					}
				}
			}

			TaskFail( FAIL_NO_ROUTE );
			break;
		}
		break;

	case TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE:
		{
			// Chase the enemy, but allow local navigation to succeed if it gets within the goal tolerance
			GetNavigator()->SetLocalSucceedOnWithinTolerance( true );

			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}

			GetNavigator()->SetLocalSucceedOnWithinTolerance( false );
		}
		break;

	case TASK_ANTLIONGUARD_OPPORTUNITY_THROW:
		{
			// If we've got some live antlions, look for a physics object to throw
			if ( m_iNumLiveAntlions >= 2 && RandomFloat(0,1) > 0.5 )
			{
				m_hLastFailedPhysicsTarget = NULL;
				UpdatePhysicsTarget( false );
				if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) )
				{
					SetSchedule( SCHED_ANTLIONGUARD_PHYSICS_ATTACK );
				}
			}

			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate & apply damage & force for a charge to a target.
//			Done outside of the guard because we need to do this inside a trace filter.
//-----------------------------------------------------------------------------
void ApplyChargeDamage( CBaseEntity *pAntlionGuard, CBaseEntity *pTarget, float flDamage )
{
	Vector attackDir = ( pTarget->WorldSpaceCenter() - pAntlionGuard->WorldSpaceCenter() );
	VectorNormalize( attackDir );
	Vector offset = RandomVector( -32, 32 ) + pTarget->WorldSpaceCenter();

	// Generate enough force to make a 75kg guy move away at 700 in/sec
	Vector vecForce = attackDir * ImpulseScale( 75, 700 );

	// Deal the damage
	CTakeDamageInfo	info( pAntlionGuard, pAntlionGuard, vecForce, offset, flDamage, DMG_CLUB );
	pTarget->TakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: A simple trace filter class to skip small moveable physics objects
//-----------------------------------------------------------------------------
class CTraceFilterSkipPhysics : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterSkipPhysics );
	
	CTraceFilterSkipPhysics( const IHandleEntity *passentity, int collisionGroup, float minMass )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_minMass(minMass)
	{
	}
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			// don't test small moveable physics objects (unless it's an NPC)
			if ( !pEntity->IsNPC() && pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
				Assert(pPhysics);
				if ( pPhysics->IsMoveable() && pPhysics->GetMass() < m_minMass )
					return false;
			}

			// If we hit an antlion, don't stop, but kill it
			if ( pEntity->Classify() == CLASS_ANTLION )
			{
				CBaseEntity *pGuard = (CBaseEntity*)EntityFromEntityHandle( m_pPassEnt );
				ApplyChargeDamage( pGuard, pEntity, pEntity->GetHealth() );
				return false;
			}
		}

		return true;
	}



private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
	float m_minMass;
};

inline void TraceHull_SkipPhysics( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const CBaseEntity *ignore, 
					 int collisionGroup, trace_t *ptr, float minMass )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterSkipPhysics traceFilter( ignore, collisionGroup, minMass );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if our charge target is right in front of the guard
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::EnemyIsRightInFrontOfMe( CBaseEntity **pEntity )
{
	if ( !GetEnemy() )
		return false;

	if ( (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).LengthSqr() < (156*156) )
	{
		Vector vecLOS = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
		vecLOS.z = 0;
		VectorNormalize( vecLOS );
		Vector vBodyDir = BodyDirection2D();
		if ( DotProduct( vecLOS, vBodyDir ) > 0.8 )
		{
			// He's in front of me, and close. Make sure he's not behind a wall.
			trace_t tr;
			UTIL_TraceLine( WorldSpaceCenter(), GetEnemy()->EyePosition(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.m_pEnt == GetEnemy() )
			{
				*pEntity = tr.m_pEnt;
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: While charging, look ahead and see if we're going to run into anything.
//			If we are, start the gesture so it looks like we're anticipating the hit.
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::ChargeLookAhead( void )
{
	trace_t	tr;
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );
	Vector vecTestPos = GetAbsOrigin() + ( vecForward * m_flGroundSpeed * 0.75 );
	Vector testHullMins = GetHullMins();
	testHullMins.z += (StepHeight() * 2);
	TraceHull_SkipPhysics( GetAbsOrigin(), vecTestPos, testHullMins, GetHullMaxs(), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5 );

	//NDebugOverlay::Box( tr.startpos, testHullMins, GetHullMaxs(), 0, 255, 0, true, 0.1f );
	//NDebugOverlay::Box( vecTestPos, testHullMins, GetHullMaxs(), 255, 0, 0, true, 0.1f );

	if ( tr.fraction != 1.0 )
	{
		// Start playing the hit animation
		AddGesture( ACT_ANTLIONGUARD_CHARGE_ANTICIPATION );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles the guard charging into something. Returns true if it hit the world.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::HandleChargeImpact( Vector vecImpact, CBaseEntity *pEntity )
{
	// Cause a shock wave from this point which will disrupt nearby physics objects
	ImpactShock( vecImpact, 128, 350 );

	// Did we hit anything interesting?
	if ( !pEntity || pEntity->IsWorld() )
	{
		// Robin: Due to some of the finicky details in the motor, the guard will hit
		//		  the world when it is blocked by our enemy when trying to step up 
		//		  during a moveprobe. To get around this, we see if the enemy's within
		//		  a volume in front of the guard when we hit the world, and if he is,
		//		  we hit him anyway.
		EnemyIsRightInFrontOfMe( &pEntity );

		// Did we manage to find him? If not, increment our charge miss count and abort.
		if ( pEntity->IsWorld() )
		{
			m_iChargeMisses++;
			return true;
		}
	}

	// Hit anything we don't like
	if ( IRelationType( pEntity ) == D_HT && ( GetNextAttack() < gpGlobals->curtime ) )
	{
		EmitSound( "NPC_AntlionGuard.Shove" );

		if ( !IsPlayingGesture( ACT_ANTLIONGUARD_CHARGE_HIT ) )
		{
			RestartGesture( ACT_ANTLIONGUARD_CHARGE_HIT );
		}
		
		ChargeDamage( pEntity );
		
		pEntity->ApplyAbsVelocityImpulse( ( BodyDirection2D() * 400 ) + Vector( 0, 0, 200 ) );

		if ( !pEntity->IsAlive() && GetEnemy() == pEntity )
		{
			SetEnemy( NULL );
		}

		SetNextAttack( gpGlobals->curtime + 2.0f );
		SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );

		// We've hit something, so clear our miss count
		m_iChargeMisses = 0;
		return false;
	}

	// Hit something we don't hate. If it's not moveable, crash into it.
	if ( pEntity->GetMoveType() == MOVETYPE_NONE || pEntity->GetMoveType() == MOVETYPE_PUSH )
		return true;

	// If it's a vphysics object that's too heavy, crash into it too.
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		if ( pPhysics )
		{
			// If the object is being held by the player, knock it out of his hands
			if ( pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				Pickup_ForcePlayerToDropThisObject( pEntity );
				return false;
			}

			if ( (!pPhysics->IsMoveable() || pPhysics->GetMass() > VPhysicsGetObject()->GetMass() * 0.5f ) )
				return true;
		}
	}

	return false;

	/*
	
	ROBIN: Wrote & then removed this. If we want to have large rocks that the guard
		   should smack around, then we should enable it.

	else
	{
		// If we hit a physics prop, smack the crap out of it. (large rocks)
		// Factor the object mass into it, because we want to move it no matter how heavy it is.
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			CTakeDamageInfo info( this, this, 250, DMG_BLAST );
			info.SetDamagePosition( vecImpact );
			float flForce = ImpulseScale( pEntity->VPhysicsGetObject()->GetMass(), 250 );
			flForce *= random->RandomFloat( 0.85, 1.15 );

			// Calculate the vector and stuff it into the takedamageinfo
			Vector vecForce = BodyDirection3D();
			VectorNormalize( vecForce );
			vecForce *= flForce;
			vecForce *= phys_pushscale.GetFloat();
			info.SetDamageForce( vecForce );

			pEntity->VPhysicsTakeDamage( info );
		}
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_AntlionGuard::ChargeSteer( void )
{
	trace_t	tr;
	Vector	testPos, steer, forward, right;
	QAngle	angles;
	const float	testLength = m_flGroundSpeed * 0.15f;

	//Get our facing
	GetVectors( &forward, &right, NULL );

	steer = forward;

	const float faceYaw	= UTIL_VecToYaw( forward );

	//Offset right
	VectorAngles( forward, angles );
	angles[YAW] += 45.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = GetAbsOrigin() + ( forward * testLength );

	//Offset by step height
	Vector testHullMins = GetHullMins();
	testHullMins.z += (StepHeight() * 2);

	//Probe
	TraceHull_SkipPhysics( GetAbsOrigin(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5f );

	//Debug info
	if ( g_debug_antlionguard.GetInt() == 1 )
	{
		if ( tr.fraction == 1.0f )
		{
  			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
   		}
   		else
   		{
  			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}

	//Add in this component
	steer += ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Offset left
	angles[YAW] -= 90.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = GetAbsOrigin() + ( forward * testLength );

	// Probe
	TraceHull_SkipPhysics( GetAbsOrigin(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5f );

	//Debug
	if ( g_debug_antlionguard.GetInt() == 1 )
	{
		if ( tr.fraction == 1.0f )
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
		}
		else
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}

	//Add in this component
	steer -= ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Debug
	if ( g_debug_antlionguard.GetInt() == 1 )
	{
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( steer * 512.0f ), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( GetAbsOrigin() + ( steer * 512.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 255, 0, true, 0.1f );

		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( BodyDirection3D() * 256.0f ), 255, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( GetAbsOrigin() + ( BodyDirection3D() * 256.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 0, 255, true, 0.1f );
	}

	return UTIL_AngleDiff( UTIL_VecToYaw( steer ), faceYaw );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::RunTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY:
		
		AutoMovement( );
		
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
	
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}

		break;

	case TASK_ANTLIONGUARD_CHARGE:
		{
			Activity eActivity = GetActivity();

			// See if we're trying to stop after hitting/missing our target
			if ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP || eActivity == ACT_ANTLIONGUARD_CHARGE_CRASH ) 
			{
				if ( IsActivityFinished() )
				{
					TaskComplete();
					return;
				}

				// Still in the process of slowing down. Run movement until it's done.
				AutoMovement();
				return;
			}

			// Check for manual transition
			if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_START ) && ( IsActivityFinished() ) )
			{
				SetActivity( ACT_ANTLIONGUARD_CHARGE_RUN );
			}

			// See if we're still running
			if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN || eActivity == ACT_ANTLIONGUARD_CHARGE_START ) 
			{
				if ( HasCondition( COND_NEW_ENEMY ) || HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) )
				{
					SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
					return;
				}
				else 
				{
					if ( GetEnemy() != NULL )
					{
						Vector	goalDir = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
						VectorNormalize( goalDir );

						if ( DotProduct( BodyDirection2D(), goalDir ) < 0.25f )
						{
							if ( !m_bDecidedNotToStop )
							{
								// We've missed the target. Randomly decide not to stop, which will cause
								// the guard to just try and swing around for another pass.
								m_bDecidedNotToStop = true;
								if ( RandomFloat(0,1) > 0.3 )
								{
									m_iChargeMisses++;
									SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
								}
							}
						}
						else
						{
							m_bDecidedNotToStop = false;
						}
					}
				}
			}

			// Steer towards our target
			float idealYaw;
			if ( GetEnemy() == NULL )
			{
				idealYaw = GetMotor()->GetIdealYaw();
			}
			else
			{
				idealYaw = CalcIdealYaw( GetEnemy()->GetAbsOrigin() );
			}

			// Add in our steering offset
			idealYaw += ChargeSteer();
			
			// Turn to face
			GetMotor()->SetIdealYawAndUpdate( idealYaw );

			// See if we're going to run into anything soon
			ChargeLookAhead();

			// Let our animations simply move us forward. Keep the result
			// of the movement so we know whether we've hit our target.
			AIMoveTrace_t moveTrace;
			if ( AutoMovement( GetEnemy(), &moveTrace ) == false )
			{
				// Only stop if we hit the world
				if ( HandleChargeImpact( moveTrace.vEndPosition, moveTrace.pObstruction ) )
				{
					// If we're starting up, this is an error
					if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
					{
						TaskFail( "Unable to make initial movement of charge\n" );
						return;
					}

					// Crash unless we're trying to stop already
					if ( eActivity != ACT_ANTLIONGUARD_CHARGE_STOP )
					{
						if ( moveTrace.fStatus == AIMR_BLOCKED_WORLD && moveTrace.vHitNormal == vec3_origin )
						{
							SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
						}
						else
						{
							SetActivity( ACT_ANTLIONGUARD_CHARGE_CRASH );
						}
					}
				}
				else if ( moveTrace.pObstruction )
				{
					// If we hit an antlion, don't stop, but kill it
					if ( moveTrace.pObstruction->Classify() == CLASS_ANTLION )
					{
						if ( FClassnameIs( moveTrace.pObstruction, "npc_antlionguard" ) )
						{
							// Crash unless we're trying to stop already
							if ( eActivity != ACT_ANTLIONGUARD_CHARGE_STOP )
							{
								SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
							}
						}
						else
						{
							ApplyChargeDamage( this, moveTrace.pObstruction, moveTrace.pObstruction->GetHealth() );
						}
					}
				}
			}
		}
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Summon antlions around the guard
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::SummonAntlions( void )
{
	// We want to spawn them around the guard
	Vector vecForward, vecRight;
	AngleVectors( QAngle(0,GetAbsAngles().y,0), &vecForward, &vecRight, NULL );

	// Spawn positions
	struct spawnpos_t
	{
		float flForward;
		float flRight;
	};

	spawnpos_t sAntlionSpawnPositions[] =
	{
		{ 0, 200 },
		{ 0, -200 },
		{ 128, 128 },
		{ 128, -128 },
		{ -128, 128 },
		{ -128, -128 },
		{ 200, 0 },
		{ -200, 0 },
	};

	// Only spawn up to our max count
	int iSpawnPoint = 0;
	for ( int i = 0; (m_iNumLiveAntlions < ANTLIONGUARD_SUMMON_COUNT) && (iSpawnPoint < ARRAYSIZE(sAntlionSpawnPositions)); i++ )
	{
		// Determine spawn position for the antlion
		Vector vecSpawn = GetAbsOrigin() + ( sAntlionSpawnPositions[iSpawnPoint].flForward * vecForward ) + ( sAntlionSpawnPositions[iSpawnPoint].flRight * vecRight );
		iSpawnPoint++;
		// Randomise it a little
		vecSpawn.x += RandomFloat( -64, 64 );
		vecSpawn.y += RandomFloat( -64, 64 );
		vecSpawn.z += 64;

		// Make sure it's clear, and make sure we hit something
		trace_t	tr;
		UTIL_TraceHull( vecSpawn, vecSpawn - Vector(0,0,128), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), MASK_NPCSOLID, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.startsolid || tr.allsolid || tr.fraction == 1.0 )
		{
			if ( g_debug_antlionguard.GetInt() == 2 )
			{
				NDebugOverlay::Box( tr.endpos, NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 255, 0, 0, true, 5.0f );
			}
			continue;
		}

		// Ensure it's dirt or sand
		surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( ( pdata->game.material != CHAR_TEX_DIRT ) && ( pdata->game.material != CHAR_TEX_SAND ) )
		{
			if ( g_debug_antlionguard.GetInt() == 2 )
			{
				NDebugOverlay::Box( tr.endpos, NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 255, 128, 128, true, 5.0f );
			}
			continue;
		}

		// Make sure the guard can see it
		trace_t	tr_vis;
		UTIL_TraceLine( WorldSpaceCenter(), tr.endpos, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr_vis );
		if ( tr_vis.fraction != 1.0 )
		{
			if ( g_debug_antlionguard.GetInt() == 2 )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), tr.endpos, 255, 0, 0, true, 5.0f );
			}
			continue;
		}

		CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName( "npc_antlion" );
		if ( !pent )
			break;

		CNPC_Antlion *pAntlion = assert_cast<CNPC_Antlion*>(pent);

		if ( g_debug_antlionguard.GetInt() == 2 )
		{
			NDebugOverlay::Box( tr.endpos, NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 0, 255, 0, true, 5.0f );
			NDebugOverlay::Line( WorldSpaceCenter(), tr.endpos, 0, 255, 0, true, 5.0f );
		}

		vecSpawn = tr.endpos;
		pAntlion->SetAbsOrigin( vecSpawn );

		// Start facing our enemy if we have one, otherwise just match the guard.
		Vector vecFacing = vecForward;
		if ( GetEnemy() )
		{
			vecFacing = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize( vecFacing );
		}
		QAngle vecAngles;
		VectorAngles( vecFacing, vecAngles );
		pAntlion->SetAbsAngles( vecAngles );

		pAntlion->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
		pAntlion->AddSpawnFlags( SF_NPC_FADE_CORPSE );

		// Make the antlion fire my input when he dies
		pAntlion->KeyValue( "OnDeath", UTIL_VarArgs("%s,SummonedAntlionDied,,0,-1", STRING(GetEntityName())) );

		// Start the antlion burrowed, and tell him to come up
		pAntlion->m_bStartBurrowed = true;
		DispatchSpawn( pAntlion );
		pAntlion->Activate();
		g_EventQueue.AddEvent( pAntlion, "Unburrow", RandomFloat(0.1, 1.0), this, this );

		// Add it to our squad
		if ( m_pSquad )
		{
			m_pSquad->AddToSquad( pAntlion );
		}

		// Set the antlion's enemy to our enemy
		if ( GetEnemy() )
		{
			pAntlion->SetEnemy( GetEnemy() );
			pAntlion->SetState( NPC_STATE_COMBAT );
			pAntlion->UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin() );
		}

		m_iNumLiveAntlions++;
	}

	if ( g_debug_antlionguard.GetInt() == 2 )
	{
		Msg("Guard summoned antlion count: %d\n", m_iNumLiveAntlions );
	}

	if ( m_iNumLiveAntlions > 2 )
	{
		m_flNextSummonTime = gpGlobals->curtime + RandomFloat( 15,20 );
	}
	else
	{
		m_flNextSummonTime = gpGlobals->curtime + RandomFloat( 10,15 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::FoundEnemy( void )
{
	m_flAngerNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_COMBAT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::LostEnemy( void )
{
	m_flSearchNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_ALERT );

	m_OnLostPlayer.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSetShoveTarget( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );

	if ( pTarget == NULL )
	{
		Warning( "**Guard %s cannot find shove target %s\n", GetClassname(), inputdata.value.String() );
		m_hShoveTarget = NULL;
		return;
	}

	m_hShoveTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSetChargeTarget( inputdata_t &inputdata )
{
	if ( !IsAlive() )
		return;

	// Pull the target & position out of the string
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get charge target name
	char *pszParam = strtok(parseString," ");
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, pszParam, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pTarget )
	{
		Warning( "ERROR: Guard %s cannot find charge target '%s'\n", STRING(GetEntityName()), pszParam );
		return;
	}

	// Get the charge position name
	pszParam = strtok(NULL," ");
	CBaseEntity *pPosition = gEntList.FindEntityByName( NULL, pszParam, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pPosition )
	{
		Warning( "ERROR: Guard %s cannot find charge position '%s'\nMake sure you've specifed the parameters as [target start]!\n", STRING(GetEntityName()), pszParam );
		return;
	}

	// Make sure we don't stack charge targets
	if ( m_hChargeTarget )
	{
		if ( GetEnemy() == m_hChargeTarget )
		{
			SetEnemy( NULL );
		}
	}

	SetCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	m_hChargeTarget = pTarget;
	m_hChargeTargetPosition = pPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputClearChargeTarget( inputdata_t &inputdata )
{
	m_hChargeTarget = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseAct - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CNPC_AntlionGuard::NPC_TranslateActivity( Activity baseAct )
{
	//See which run to use
	if ( ( baseAct == ACT_RUN ) && IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return (Activity) ACT_ANTLIONGUARD_CHARGE_RUN;

	// Do extra code if we're trying to close on an enemy in a confined space
	if ( hl2_episodic.GetBool() && m_bInCavern )
	{
		if ( GetEnemy() != NULL )
		{
			// FIXME: This might want to be the route distance to our enemy instead
			float flDistToEnemy = ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).Length();
			if ( flDistToEnemy > 256.0f )
				return (Activity) ACT_ANTLIONGUARD_CHARGE_RUN;
		}
	}

	if ( ( baseAct == ACT_RUN ) && ( m_iHealth <= (m_iMaxHealth/4) ) )
		return (Activity) ACT_ANTLIONGUARD_RUN_HURT;

	return baseAct;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::ShouldWatchEnemy( void )
{
	Activity nActivity = GetActivity();

	if ( ( nActivity == ACT_ANTLIONGUARD_SEARCH ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_ENTER ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_EXIT ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK1 ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_SIGHTED ) || 
		 ( nActivity == ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_FR ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_FL ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_RR ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_RL ) || 
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_CRASH ) || 
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_HIT ) ||
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_ANTICIPATION ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::UpdateHead( void )
{
	float yaw = GetPoseParameter( "head_yaw" );
	float pitch = GetPoseParameter( "head_pitch" );

	// If we should be watching our enemy, turn our head
	if ( ShouldWatchEnemy() && ( GetEnemy() != NULL ) )
	{
		Vector	enemyDir = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( enemyDir );
		
		float angle = VecToYaw( BodyDirection3D() );
		float angleDiff = VecToYaw( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + yaw );

		SetPoseParameter( "head_yaw", UTIL_Approach( yaw + angleDiff, yaw, 50 ) );

		angle = UTIL_VecToPitch( BodyDirection3D() );
		angleDiff = UTIL_VecToPitch( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + pitch );

		SetPoseParameter( "head_pitch", UTIL_Approach( pitch + angleDiff, pitch, 50 ) );
	}
	else
	{
		// Otherwise turn the head back to its normal position
		SetPoseParameter( "head_yaw",	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( "head_pitch", UTIL_Approach( 0, pitch, 10 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::MaintainPhysicsTarget( void )
{
	if ( m_hPhysicsTarget == NULL )
		return;

	// Update our current target to make sure it's still valid
	float flTargetDistSqr = ( m_hPhysicsTarget->WorldSpaceCenter() - m_vecPhysicsTargetStartPos ).LengthSqr();
	bool bTargetMoved = ( flTargetDistSqr > (64.0f*64.0f) );
	bool bEnemyCloser = ( ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() <= flTargetDistSqr );

	// Make sure this hasn't moved too far or that the player is now closer
	if ( bTargetMoved || bEnemyCloser )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );
		m_hPhysicsTarget = NULL;
		return;
	}
	else
	{
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );
	}

	if ( g_debug_antlionguard.GetInt() == 3 )
	{
		NDebugOverlay::Cross3D( m_hPhysicsTarget->WorldSpaceCenter(), -Vector(32,32,32), Vector(32,32,32), 255, 255, 255, true, 1.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::UpdatePhysicsTarget( bool bPreferObjectsAlongTargetVector, float flRadius )
{
	if ( GetEnemy() == NULL )
		return;

	// Already have a target, don't bother looking
	if ( m_hPhysicsTarget != NULL )
		return;

	// Too soon to check again
	if ( m_flPhysicsCheckTime > gpGlobals->curtime )
		return;

	// Attempt to find a valid shove target
	PhysicsObjectCriteria_t criteria;
	criteria.pTarget = GetEnemy();
	criteria.flRadius = flRadius;
	criteria.flTargetCone = ANTLIONGUARD_OBJECTFINDING_FOV;
	criteria.bPreferObjectsAlongTargetVector = bPreferObjectsAlongTargetVector;

	m_hPhysicsTarget = FindPhysicsObjectTarget( criteria );
		
	// Found one, so interrupt us if we care
	if ( m_hPhysicsTarget != NULL )
	{
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_vecPhysicsTargetStartPos = m_hPhysicsTarget->WorldSpaceCenter();
		m_hLastFailedPhysicsTarget = NULL;

		// We must steer around this obstacle until we've thrown it, this stops us from
		// shoving it out of the way while we travel there
		//m_hPhysicsTarget->SetNavIgnore();
	}

	// Don't search again for another second
	m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Let the probe know I can run through small debris
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity )
{
	if ( m_iszPhysicsPropClass != pEntity->m_iClassname )
		return BaseClass::ShouldProbeCollideAgainstEntity( pEntity );

	if ( m_hPhysicsTarget == pEntity )
		return false;

	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysObj = pEntity->VPhysicsGetObject();

		if( pPhysObj && pPhysObj->GetMass() <= ANTLIONGUARD_MAX_OBJECT_MASS )
		{
			return false;
		}
	}

	return BaseClass::ShouldProbeCollideAgainstEntity( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Don't do anything after death
	if ( m_NPCState == NPC_STATE_DEAD )
		return;

	// If we're burrowed, then don't do any of this
	if ( m_bIsBurrowed )
		return;

	// Automatically update our physics target when chasing enemies
	if ( IsCurSchedule( SCHED_CHASE_ENEMY ) || IsCurSchedule( SCHED_ANTLIONGUARD_PATROL_RUN ))
	{
		UpdatePhysicsTarget( true );
	}
	else if ( !IsCurSchedule( SCHED_ANTLIONGUARD_PHYSICS_ATTACK ) )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_hPhysicsTarget = NULL;
	}

	UpdateHead();

	if ( ( m_flGroundSpeed <= 0.0f ) )
	{
		if ( m_bStopped == false )
		{
			StartSounds();

			float duration = random->RandomFloat( 2.0f, 8.0f );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.0f, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 40, 60 ), duration );
			
			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, duration );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 120, 140 ), duration );

			m_flBreathTime = gpGlobals->curtime + duration - (duration*0.75f);
		}
		
		m_bStopped = true;

		if ( m_flBreathTime < gpGlobals->curtime )
		{
			StartSounds();

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, random->RandomFloat( 0.2f, 0.3f ), random->RandomFloat( 0.5f, 1.0f ) );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 80, 120 ), random->RandomFloat( 0.5f, 1.0f ) );

			m_flBreathTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 8.0f );
		}
	}
	else
	{
		if ( m_bStopped ) 
		{
			StartSounds();

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pBreathSound, 0.6f, random->RandomFloat( 2.0f, 4.0f ) );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pBreathSound, random->RandomInt( 140, 160 ), random->RandomFloat( 2.0f, 4.0f ) );

			ENVELOPE_CONTROLLER.SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 1.0f );
			ENVELOPE_CONTROLLER.SoundChangePitch( m_pGrowlIdleSound, random->RandomInt( 90, 110 ), 0.2f );
		}


		m_bStopped = false;
	}

	// Put danger sounds out in front of us
	for ( int i = 0; i < 3; i++ )
	{
		CSoundEnt::InsertSound( SOUND_DANGER, WorldSpaceCenter() + ( BodyDirection3D() * 128 * (i+1) ), 128, 0.1f, this );		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( CanSummon(false) )
	{
		SetCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}
	else
	{
		ClearCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}

	// Make sure our physics target is still valid
	MaintainPhysicsTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::StopLoopingSounds()
{
	//Stop all sounds
	ENVELOPE_CONTROLLER.SoundDestroy( m_pGrowlHighSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pGrowlIdleSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pBreathSound );
	ENVELOPE_CONTROLLER.SoundDestroy( m_pConfusedSound );
	
	
	m_pGrowlHighSound	= NULL;
	m_pGrowlIdleSound	= NULL;
	m_pBreathSound		= NULL;
	m_pConfusedSound	= NULL;

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputUnburrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	if ( m_bIsBurrowed == false )
		return;

	m_spawnflags &= ~SF_NPC_GAG;
	
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	
	m_takedamage = DAMAGE_YES;

	SetSchedule( SCHED_ANTLIONGUARD_UNBURROW );

	m_bIsBurrowed = false;
}

//-----------------------------------------------------------------------------
// Purpose: Return the point at which the guard wants to stand on to knock the physics object at the target entity
// Input  : *pObject - Object to be shoved.
//			*pTarget - Target to be shoved at.
//			*vecTrajectory - Trajectory to our target
//			*flClearDistance - Distance behind the entity we're clear to use
// Output : Position at which to attempt to strike the object
//-----------------------------------------------------------------------------
Vector CNPC_AntlionGuard::GetPhysicsHitPosition( CBaseEntity *pObject, CBaseEntity *pTarget, Vector *vecTrajectory, float *flClearDistance )
{
	// Get the trajectory we want to knock the object along
	Vector vecToTarget = pTarget->WorldSpaceCenter() - pObject->WorldSpaceCenter();
	VectorNormalize( vecToTarget );
	vecToTarget.z = 0;

	// Get the distance we want to be from the object when we hit it
	IPhysicsObject *pPhys = pObject->VPhysicsGetObject();
	Vector extent = physcollision->CollideGetExtent( pPhys->GetCollide(), pObject->GetAbsOrigin(), pObject->GetAbsAngles(), -vecToTarget );
	float flDist = ( extent - pObject->WorldSpaceCenter() ).Length() + CollisionProp()->BoundingRadius() + 32.0f;
	
	if ( vecTrajectory != NULL )
	{
		*vecTrajectory = vecToTarget;
	}

	if ( flClearDistance != NULL )
	{
		*flClearDistance = flDist;
	}

	// Position at which we'd like to be
	return (pObject->WorldSpaceCenter() + ( vecToTarget * -flDist ));
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the guard can stand in a position to strike a specified object
// Input  : *pShoveObject - Object being shoved
//			*pTarget - Target we're shoving the object at
//			*pOut - The position we decide to stand at
// Output : Returns true if the guard can stand and deliver.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::CanStandAtShoveTarget( CBaseEntity *pShoveObject, CBaseEntity *pTarget, Vector *pOut )
{
	// Get the position we want to be at to swing at the object
	float flClearDistance;
	Vector vecTrajectory;
	Vector vecHitPosition = GetPhysicsHitPosition( pShoveObject, pTarget, &vecTrajectory, &flClearDistance );

	// See if we can stand at this position (FIXME: Better function for this?)
	trace_t tr;
	UTIL_TraceHull( vecHitPosition, vecHitPosition, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.startsolid || tr.allsolid || (tr.m_pEnt && tr.m_pEnt != pShoveObject) )
	{
		// Can we get at it at from an angle on the side of it we're on?
		Vector vecUp(0,0,1);
		Vector vecRight;
		CrossProduct( vecUp, vecTrajectory, vecRight );

		// Is the guard to the right? or the left?
		Vector vecToGuard = ( WorldSpaceCenter() - pShoveObject->WorldSpaceCenter() );
		VectorNormalize( vecToGuard );
		if ( DotProduct( vecRight, vecToGuard ) > 0 )
		{
			vecHitPosition += (vecRight * 64) + (vecTrajectory * 32);
		}
		else
		{
			vecHitPosition -= (vecRight * 64) + (vecTrajectory * 32);
		}

		// Now try and stand at this side position
		UTIL_TraceHull( vecHitPosition, vecHitPosition, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.startsolid || tr.allsolid || (tr.m_pEnt && tr.m_pEnt != pShoveObject) )
			return false;
	}

	if ( pOut != NULL )
	{
		*pOut = vecHitPosition;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Search for a physics item to swat at the player
// Output : Returns the object we're going to swat.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_AntlionGuard::FindPhysicsObjectTarget( const PhysicsObjectCriteria_t &criteria )
{
	// Must have a valid target entity
 	if ( criteria.pTarget == NULL )
		return NULL;

	// Get the vector to our target, from ourself
	Vector vecDirToTarget = criteria.pTarget->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecDirToTarget );
	vecDirToTarget.z = 0;

	// Cost is determined by distance to the object, modified by how "in line" it is with our target direction of travel
	// Use the distance to the player as the base cost for throwing an object (avoids pushing things too close to the player)
	float flLeastCost = ( criteria.pTarget->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
	float flCost;

	AISightIter_t iter;
	CBaseEntity *pObject;
	CBaseEntity	*pNearest = NULL;
	Vector vecBestHitPosition = vec3_origin;

	// Look through the list of sensed objects for possible targets
	for ( pObject = GetSenses()->GetFirstSeenEntity( &iter, SEEN_MISC ); pObject; pObject = GetSenses()->GetNextSeenEntity( &iter ) )
	{
		// If we couldn't shove this object last time, don't try again
		if ( pObject == m_hLastFailedPhysicsTarget )
			continue;

		// Ignore things less than half a foot in diameter
		if ( pObject->CollisionProp()->BoundingRadius() < 6.0f )
			continue;

		IPhysicsObject *pPhysObj = pObject->VPhysicsGetObject();
		if ( pPhysObj == NULL )
			continue;

		// Ignore motion disabled props
		if ( pPhysObj->IsMoveable() == false )
			continue;

		// Ignore things lighter than 5kg
		if ( pPhysObj->GetMass() < 5.0f )
			continue;

		// Ignore objects moving too quickly (they'll be too hard to catch otherwise)
		Vector	velocity;
		pPhysObj->GetVelocity( &velocity, NULL );
		if ( velocity.LengthSqr() > (16*16) )
			continue;

		// Get the direction from us to the physics object
		Vector vecDirToObject = pObject->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDirToObject );
		vecDirToObject.z = 0;
		
		Vector vecObjCenter = pObject->WorldSpaceCenter();
		float flDistSqr = 0.0f;
		float flDot = 0.0f;

		// If we want to find things along the vector to the target, do so
		if ( criteria.bPreferObjectsAlongTargetVector )
		{
			// Object must be closer than our target
			if ( ( GetAbsOrigin() - vecObjCenter ).LengthSqr() > ( GetAbsOrigin() - criteria.pTarget->GetAbsOrigin() ).LengthSqr() )
				continue;

			// Calculate a "cost" to this object
			flDistSqr = ( GetAbsOrigin() - vecObjCenter ).LengthSqr();
			flDot = DotProduct( vecDirToTarget, vecDirToObject );
			
			// Ignore things outside our allowed cone
			if ( flDot < criteria.flTargetCone )
				continue;

			// The more perpendicular we are, the higher the cost
			float flCostScale = RemapValClamped( flDot, 1.0f, criteria.flTargetCone, 1.0f, 4.0f );
			flCost = flDistSqr * flCostScale;
		}
		else
		{
			// Straight distance cost
			flCost = ( GetAbsOrigin() - vecObjCenter ).LengthSqr();
		}

		// This must be a less costly object to use
		if ( flCost >= flLeastCost )
		{
			if ( g_debug_antlionguard.GetInt() == 3 )
			{
				NDebugOverlay::Box( vecObjCenter, -Vector(16,16,16), Vector(16,16,16), 255, 0, 0, 0, 2.0f );
			}

			continue;
		}

		// Check for a (roughly) clear trajectory path from the object to target
		trace_t	tr;
		UTIL_TraceLine( vecObjCenter, criteria.pTarget->BodyTarget( vecObjCenter ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
		
		// See how close to our target we got (we still look good hurling things that won't necessarily hit)
		if ( ( tr.endpos - criteria.pTarget->WorldSpaceCenter() ).LengthSqr() > (256*256) )
			continue;

		// Must be able to stand at a position to hit the object
		Vector vecHitPosition;
		if ( CanStandAtShoveTarget( pObject, criteria.pTarget, &vecHitPosition ) == false )
		{
			if ( g_debug_antlionguard.GetInt() == 3 )
			{
				NDebugOverlay::HorzArrow( GetAbsOrigin(), pObject->WorldSpaceCenter(), 32.0f, 255, 0, 0, 64, true, 2.0f );
			}
			continue;
		}

		// Take this as the best object so far
		pNearest = pObject;
		flLeastCost = flCost;
		vecBestHitPosition = vecHitPosition;
		
		if ( g_debug_antlionguard.GetInt() == 3 )
		{
			NDebugOverlay::HorzArrow( GetAbsOrigin(), pObject->WorldSpaceCenter(), 16.0f, 255, 255, 0, 0, true, 2.0f );
		}
	}

	// Set extra info if we've succeeded
	if ( pNearest != NULL )
	{
		m_vecPhysicsHitPosition = vecBestHitPosition;
	
		if ( g_debug_antlionguard.GetInt() == 3 )
		{
			NDebugOverlay::HorzArrow( GetAbsOrigin(), pNearest->WorldSpaceCenter(), 32.0f, 0, 255, 0, 128, true, 2.0f );
		}
	}

	return pNearest;
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	// Interrupt if we can shove something
	if ( IsCurSchedule( SCHED_CHASE_ENEMY ) )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		SetCustomInterruptCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}

	// Interrupt if we've been given a charge target
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) == false )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	}

	// Once we commit to doing this, just do it!
	if ( IsCurSchedule( SCHED_MELEE_ATTACK1 ) )
	{
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
	}

	// Always take heavy damage
	SetCustomInterruptCondition( COND_HEAVY_DAMAGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			radius - 
//			magnitude - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::ImpactShock( const Vector &origin, float radius, float magnitude, CBaseEntity *pIgnored )
{
	// Also do a local physics explosion to push objects away
	float	adjustedDamage, flDist;
	Vector	vecSpot;
	float	falloff = 1.0f / 2.5f;

	CBaseEntity *pEntity = NULL;

	// Find anything within our radius
	while ( ( pEntity = gEntList.FindEntityInSphere( pEntity, origin, radius ) ) != NULL )
	{
		// Don't affect the ignored target
		if ( pEntity == pIgnored )
			continue;
		if ( pEntity == this )
			continue;

		// UNDONE: Ask the object if it should get force if it's not MOVETYPE_VPHYSICS?
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS || ( pEntity->VPhysicsGetObject() && pEntity->IsPlayer() == false ) )
		{
			vecSpot = pEntity->BodyTarget( GetAbsOrigin() );
			
			// decrease damage for an ent that's farther from the bomb.
			flDist = ( GetAbsOrigin() - vecSpot ).Length();

			if ( radius == 0 || flDist <= radius )
			{
				adjustedDamage = flDist * falloff;
				adjustedDamage = magnitude - adjustedDamage;
		
				if ( adjustedDamage < 1 )
				{
					adjustedDamage = 1;
				}

				CTakeDamageInfo info( this, this, adjustedDamage, DMG_BLAST );
				CalculateExplosiveDamageForce( &info, (vecSpot - GetAbsOrigin()), GetAbsOrigin() );

				pEntity->VPhysicsTakeDamage( info );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::ChargeDamage( CBaseEntity *pTarget )
{
	if ( pTarget == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pTarget );

	if ( pPlayer != NULL )
	{
		//Kick the player angles
		pPlayer->ViewPunch( QAngle( 20, 20, -30 ) );	

		Vector	dir = pPlayer->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( dir );
		dir.z = 0.0f;
		
		Vector vecNewVelocity = dir * 250.0f;
		vecNewVelocity[2] += 128.0f;
		pPlayer->SetAbsVelocity( vecNewVelocity );

		color32 red = {128,0,0,128};
		UTIL_ScreenFade( pPlayer, red, 1.0f, 0.1f, FFADE_IN );
	}
	
	// Player takes less damage
	float flDamage = ( pPlayer == NULL ) ? 250 : sk_antlionguard_dmg_charge.GetFloat();
	
	// If it's being held by the player, break that bond
	Pickup_ForcePlayerToDropThisObject( pTarget );

	// Calculate the physics force
	ApplyChargeDamage( this, pTarget, flDamage );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputRagdoll( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	//Set us to nearly dead so the velocity from death is minimal
	SetHealth( 1 );

	CTakeDamageInfo info( this, this, GetHealth(), DMG_CRUSH );
	BaseClass::TakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_AntlionGuard::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY_FAILED:
		{
			int baseType = BaseClass::TranslateSchedule(scheduleType);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			return SelectUnreachableSchedule();
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::StartSounds( void )
{
	//Initialize the additive sound channels
	CPASAttenuationFilter filter( this );

	if ( m_pGrowlHighSound == NULL )
	{
		m_pGrowlHighSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_VOICE, "NPC_AntlionGuard.GrowlHigh",	ATTN_NORM );
		
		if ( m_pGrowlHighSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pGrowlHighSound,0.0f, 100 );
		}
	}

	if ( m_pGrowlIdleSound == NULL )
	{
		m_pGrowlIdleSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_STATIC, "NPC_AntlionGuard.GrowlIdle",	ATTN_NORM );

		if ( m_pGrowlIdleSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pGrowlIdleSound,0.0f, 100 );
		}
	}

	if ( m_pBreathSound == NULL )
	{
		m_pBreathSound	= ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_ITEM, "NPC_AntlionGuard.BreathSound",		ATTN_NORM );
		
		if ( m_pBreathSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pBreathSound,	0.0f, 100 );
		}
	}

	if ( m_pConfusedSound == NULL )
	{
		m_pConfusedSound = ENVELOPE_CONTROLLER.SoundCreate( filter, entindex(), CHAN_WEAPON,"NPC_AntlionGuard.Confused",	ATTN_NORM );

		if ( m_pConfusedSound )
		{
			ENVELOPE_CONTROLLER.Play( m_pConfusedSound,	0.0f, 100 );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputEnableBark( inputdata_t &inputdata )
{
	m_bBarkEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputDisableBark( inputdata_t &inputdata )
{
	m_bBarkEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_AntlionGuard.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	// Tell all of my antlions to burrow away, 'cos they fear the Freeman
	if ( m_iNumLiveAntlions )
	{
		CBaseEntity	*pSearch = NULL;

		// Iterate through all antlions and see if there are any orphans
		while ( ( pSearch = gEntList.FindEntityByClassname( pSearch, "npc_antlion" ) ) != NULL )
		{
			CNPC_Antlion *pAntlion = assert_cast<CNPC_Antlion *>(pSearch);

			// See if it's a live orphan
			if ( pAntlion && pAntlion->GetOwnerEntity() == NULL && pAntlion->IsAlive() )
			{
				g_EventQueue.AddEvent( pAntlion, "BurrowAway", RandomFloat(0.1, 2.0), this, this );
			}
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't become a ragdoll until we've finished our death anim
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::CanBecomeRagdoll( void )
{
	if ( IsCurSchedule( SCHED_DIE ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &force - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::BecomeRagdollOnClient( const Vector &force )
{
	if ( !CanBecomeRagdoll() ) 
		return false;

	EmitSound( "NPC_AntlionGuard.Fallover" );

	// Become server-side ragdoll if we're flagged to do it
	if ( m_spawnflags & SF_ANTLIONGUARD_SERVERSIDE_RAGDOLL )
	{
		CTakeDamageInfo	info;

		// Fake the info
		info.SetDamageType( DMG_GENERIC );
		info.SetDamageForce( force );
		info.SetDamagePosition( WorldSpaceCenter() );

		CBaseEntity *pRagdoll = CreateServerRagdoll( this, 0, info, COLLISION_GROUP_NONE );

		// Transfer our name to the new ragdoll
		pRagdoll->SetName( GetEntityName() );
		pRagdoll->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		
		// Get rid of our old body
		UTIL_Remove(this);

		return true;
	}

	return BaseClass::BecomeRagdollOnClient( force );
}

//-----------------------------------------------------------------------------
// Purpose: Override how we face our target as we move
// Output :
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
  	Vector		vecFacePosition = vec3_origin;
	CBaseEntity	*pFaceTarget = NULL;
	bool		bFaceTarget = false;

	// FIXME: this will break scripted sequences that walk when they have an enemy
	if ( m_hChargeTarget )
	{
		vecFacePosition = m_hChargeTarget->GetAbsOrigin();
		pFaceTarget = m_hChargeTarget;
		bFaceTarget = true;
	}
#ifdef HL2_EPISODIC
	else if ( GetEnemy() && IsCurSchedule( SCHED_ANTLIONGUARD_CANT_ATTACK ) )
	{
		// Always face our enemy when randomly patrolling around
		vecFacePosition = GetEnemy()->EyePosition();
		pFaceTarget = GetEnemy();
		bFaceTarget = true;
	}
#endif	// HL2_EPISODIC
	else if ( GetEnemy() && GetNavigator()->GetMovementActivity() == ACT_RUN )
  	{
		Vector vecEnemyLKP = GetEnemyLKP();
		
		// Only start facing when we're close enough
		if ( ( UTIL_DistApprox( vecEnemyLKP, GetAbsOrigin() ) < 512 ) || IsCurSchedule( SCHED_ANTLIONGUARD_PATROL_RUN ) )
		{
			vecFacePosition = vecEnemyLKP;
			pFaceTarget = GetEnemy();
			bFaceTarget = true;
		}
	}

	// Face
	if ( bFaceTarget )
	{
		AddFacingTarget( pFaceTarget, vecFacePosition, 1.0, 0.2 );
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Struck by blast
	if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( info.GetDamage() > MIN_BLAST_DAMAGE )
			return true;
	}

	// Struck by large object
	if ( info.GetDamageType() & DMG_CRUSH )
	{
		IPhysicsObject *pPhysObject = info.GetInflictor()->VPhysicsGetObject();

		if ( ( pPhysObject != NULL ) && ( pPhysObject->GetGameFlags() & FVPHYSICS_WAS_THROWN ) )
		{
			// Always take hits from a combine ball
			if ( UTIL_IsAR2CombineBall( info.GetInflictor() ) )
				return true;

			// If we're under half health, stop being interrupted by heavy damage
			if ( GetHealth() < (GetMaxHealth() * 0.25) )
				return false;

			// Ignore physics damages that don't do much damage
			if ( info.GetDamage() < MIN_CRUSH_DAMAGE )
				return false;

			return true;
		}

		return false;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::IsLightDamage( const CTakeDamageInfo &info )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pChild - 
//-----------------------------------------------------------------------------
void CNPC_AntlionGuard::InputSummonedAntlionDied( inputdata_t &inputdata )
{
	m_iNumLiveAntlions--;
	Assert( m_iNumLiveAntlions >= 0 );

	if ( g_debug_antlionguard.GetInt() == 2 )
	{
		Msg("Guard summoned antlion count: %d\n", m_iNumLiveAntlions );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Filter out sounds we don't care about
// Input  : *pSound - sound to test against
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AntlionGuard::QueryHearSound( CSound *pSound )
{
	// Don't bother with danger sounds from antlions or other guards
	if ( pSound->SoundType() == SOUND_DANGER && ( pSound->m_hOwner != NULL && pSound->m_hOwner->Classify() == CLASS_ANTLION ) )
		return false;

	return BaseClass::QueryHearSound( pSound );
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_antlionguard, CNPC_AntlionGuard )

	//Tasks
	DECLARE_TASK( TASK_ANTLIONGUARD_CHARGE )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT )
	DECLARE_TASK( TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_TASK( TASK_ANTLIONGUARD_SUMMON )
	DECLARE_TASK( TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE )
	DECLARE_TASK( TASK_ANTLIONGUARD_OPPORTUNITY_THROW )
	DECLARE_TASK( TASK_ANTLIONGUARD_FIND_PHYSOBJECT )

	//Activities
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SEARCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_FLINCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_ENTER )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_EXIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK1 )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_BARK )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_SIGHTED )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_START )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CANCEL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_RUN )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CRASH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_STOP )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_HIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_ANTICIPATION )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_FLINCH_LIGHT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_UNBURROW )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_ROAR )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_RUN_HURT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_FR )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_FL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_RR )	
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_RL )		
	
	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_CHARGE_HIT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_SHOVE )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_FOOTSTEP_LIGHT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_FOOTSTEP_HEAVY )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_CHARGE_EARLYOUT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_GROWL )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_BARK )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_PAIN )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_SQUEEZE )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_SCRATCH )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_GRUNT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_BURROW_OUT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_ROAR )

	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_TARGET )
	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID )
	DECLARE_CONDITION( COND_ANTLIONGUARD_HAS_CHARGE_TARGET )
	DECLARE_CONDITION( COND_ANTLIONGUARD_CAN_SUMMON )

	//==================================================
	// SCHED_ANTLIONGUARD_SUMMON
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_SUMMON,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_FACE_ENEMY							0"
		"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ANTLIONGUARD_BARK"
		"		TASK_ANTLIONGUARD_SUMMON				0"
		"		TASK_ANTLIONGUARD_OPPORTUNITY_THROW		0"
		"	"
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"

		// These are deliberately left out so they can be detected during the 
		// charge Task and correctly play the charge stop animation.
		//"		COND_NEW_ENEMY"
		//"		COND_ENEMY_DEAD"
		//"		COND_LOST_ENEMY"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_TARGET
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_SMASH
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CRASH,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CRASH"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_ATTACK
	//==================================================

	DEFINE_SCHEDULE
	( 
		SCHED_ANTLIONGUARD_PHYSICS_ATTACK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT	0"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_FACE_ENEMY								0"
		"		TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID"
	)

	//==================================================
	// SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK
	//==================================================

	DEFINE_SCHEDULE
	( 
		SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_ANTLIONGUARD_CANT_ATTACK"
		"		TASK_ANTLIONGUARD_FIND_PHYSOBJECT			0"
		"		TASK_SET_SCHEDULE							SCHEDULE:SCHED_ANTLIONGUARD_PHYSICS_ATTACK"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CANT_ATTACK
	//		If we're here, the guard can't chase enemy, can't find a physobject to attack with, and can't summon
	//==================================================

#ifdef HL2_EPISODIC

	DEFINE_SCHEDULE
	( 
		SCHED_ANTLIONGUARD_CANT_ATTACK,

		"	Tasks"
		"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_GET_PATH_TO_RANDOM_NODE	200"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_WAIT_PVS					0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
	)

#else

	DEFINE_SCHEDULE
	( 
	SCHED_ANTLIONGUARD_CANT_ATTACK,

	"	Tasks"
	"		TASK_WAIT								5"
	""
	"	Interrupts"
	)

#endif

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_RESET_ACTIVITY						0"
		"		TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY	0"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_UNBURROW
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_UNBURROW,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_UNBURROW"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_CANCEL
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CANCEL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CANCEL"
		""
		"	Interrupts"
	)
	
	//==================================================
	// SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,

		"	Tasks"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION	0"
		"		TASK_RUN_PATH									0"
		"		TASK_WAIT_FOR_MOVEMENT							0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_GIVE_WAY"
		"		COND_TASK_FAILED"
	)

	//=========================================================
	// > SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE,

		"	Tasks"
		"		TASK_STOP_MOVING									0"
		"		TASK_SET_FAIL_SCHEDULE								SCHEDULE:SCHED_ANTLIONGUARD_PATROL_RUN"
		"		TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE	150"
		"		TASK_RUN_PATH										0"
		"		TASK_WAIT_FOR_MOVEMENT								0"
		"		TASK_FACE_ENEMY										0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_CAN_SUMMON"
	);

	//=========================================================
	// > PATROL_RUN
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PATROL_RUN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK"
		"		TASK_SET_ROUTE_SEARCH_TIME					5"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE	200"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_ANTLIONGUARD_CAN_SUMMON"
	);

	//==================================================
	// SCHED_ANTLIONGUARD_ROAR
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_ROAR,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_ROAR"
		"	"
		"	Interrupts"
	)


AI_END_CUSTOM_NPC()
