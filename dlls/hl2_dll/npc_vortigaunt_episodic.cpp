//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ====
//
// Purpose: Vortigaunt - now much friendlier!
//
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "globalstate.h"
#include "npcevent.h"
#include "Sprite.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"
#include "ai_squadslot.h"

#include "npc_vortigaunt_episodic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	VORTIGAUNT_LIMP_HEALTH				20
#define	VORTIGAUNT_SENTENCE_VOLUME			(float)0.35 // volume of vortigaunt sentences
#define	VORTIGAUNT_VOL						0.35		// volume of vortigaunt sounds
#define	VORTIGAUNT_ATTN						ATTN_NORM	// attenutation of vortigaunt sentences
#define	VORTIGAUNT_HEAL_RECHARGE			15.0		// How long to rest between heals
#define	VORTIGAUNT_ZAP_GLOWGROW_TIME		0.5			// How long does glow last
#define	VORTIGAUNT_HEAL_GLOWGROW_TIME		1.4			// How long does glow last
#define	VORTIGAUNT_GLOWFADE_TIME			0.5			// How long does it fade
#define	VORTIGAUNT_BEAM_HURT				0
#define	VORTIGAUNT_BEAM_HEAL				1

#define	PLAYER_CRITICAL_HEALTH_PERC	0.3f

static const char *VORTIGAUNT_LEFT_CLAW = "leftclaw";
static const char *VORTIGAUNT_RIGHT_CLAW = "rightclaw";

static const char *VORT_CURE = "VORT_CURE";
static const char *VORT_CURESTOP = "VORT_CURESTOP";
static const char *VORT_CURE_INTERRUPT = "VORT_CURE_INTERRUPT";
static const char *VORT_ATTACK = "VORT_ATTACK";
static const char *VORT_MAD = "VORT_MAD";
static const char *VORT_SHOT = "VORT_SHOT";
static const char *VORT_PAIN = "VORT_PAIN";
static const char *VORT_DIE = "VORT_DIE";
static const char *VORT_KILL = "VORT_KILL";
static const char *VORT_LINE_FIRE = "VORT_LINE_FIRE";
static const char *VORT_POK = "VORT_POK";
static const char *VORT_EXTRACT_START = "VORT_EXTRACT_START";
static const char *VORT_EXTRACT_FINISH = "VORT_EXTRACT_FINISH";

// Target must be within this range to heal
#define	HEAL_RANGE			(15*12) //ft
#define	HEAL_SEARCH_RANGE	(40*12) //ft

ConVar	sk_vortigaunt_health( "sk_vortigaunt_health","0");
ConVar	sk_vortigaunt_armor_charge( "sk_vortigaunt_armor_charge","30");
ConVar	sk_vortigaunt_dmg_claw( "sk_vortigaunt_dmg_claw","0");
ConVar	sk_vortigaunt_dmg_rake( "sk_vortigaunt_dmg_rake","0");
ConVar	sk_vortigaunt_dmg_zap( "sk_vortigaunt_dmg_zap","0");

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionVortigauntStomp		= 0;
int	g_interactionVortigauntStompFail	= 0;
int	g_interactionVortigauntStompHit		= 0;
int	g_interactionVortigauntKick			= 0;
int	g_interactionVortigauntClaw			= 0;

//=========================================================
// Vortigaunt activities
//=========================================================
int	ACT_VORTIGAUNT_AIM;
int ACT_VORTIGAUNT_START_HEAL;
int ACT_VORTIGAUNT_HEAL_LOOP;
int ACT_VORTIGAUNT_END_HEAL;
int ACT_VORTIGAUNT_TO_ACTION;
int ACT_VORTIGAUNT_TO_IDLE;
int ACT_VORTIGAUNT_HEAL; // Heal gesture

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
int AE_VORTIGAUNT_CLAW_LEFT;
int AE_VORTIGAUNT_CLAW_RIGHT;
int AE_VORTIGAUNT_ZAP_POWERUP;
int AE_VORTIGAUNT_ZAP_SHOOT;
int AE_VORTIGAUNT_ZAP_DONE;
int AE_VORTIGAUNT_HEAL_STARTGLOW;
int AE_VORTIGAUNT_HEAL_STARTBEAMS;
int AE_VORTIGAUNT_HEAL_STARTSOUND;
int AE_VORTIGAUNT_SWING_SOUND;
int AE_VORTIGAUNT_SHOOT_SOUNDSTART;
int AE_VORTIGAUNT_HEAL_PAUSE;

//-----------------------------------------------------------------------------
// Squad slots
//-----------------------------------------------------------------------------
enum SquadSlot_T
{	
	SQUAD_SLOT_HEAL_PLAYER = LAST_SHARED_SQUADSLOT,
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Vortigaunt )

	DEFINE_FIELD( m_flHealHinderedTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_bHealBeamActive,		FIELD_BOOLEAN ),
	DEFINE_ARRAY( m_pBeam,					FIELD_EHANDLE, VORTIGAUNT_MAX_BEAMS ),
	DEFINE_FIELD( m_iBeams,					FIELD_INTEGER),
	DEFINE_FIELD( m_nLightningSprite,		FIELD_INTEGER),
	DEFINE_FIELD( m_fGlowAge,				FIELD_FLOAT),
	DEFINE_FIELD( m_fGlowScale,				FIELD_FLOAT),
	DEFINE_FIELD( m_fGlowChangeTime,		FIELD_FLOAT),
	DEFINE_FIELD( m_bGlowTurningOn,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_nCurGlowIndex,			FIELD_INTEGER),
	DEFINE_FIELD( m_pLeftHandGlow,			FIELD_EHANDLE),
	DEFINE_FIELD( m_pRightHandGlow,			FIELD_EHANDLE),
	DEFINE_FIELD( m_flNextHealTime,			FIELD_TIME),
	DEFINE_FIELD( m_nCurrentHealAmt,		FIELD_INTEGER),
	DEFINE_FIELD( m_nLastArmorAmt,			FIELD_INTEGER),
	DEFINE_FIELD( m_iSuitSound,				FIELD_INTEGER),
	DEFINE_FIELD( m_flSuitSoundTime,		FIELD_TIME),
	DEFINE_FIELD( m_flPainTime,				FIELD_TIME),
	DEFINE_FIELD( m_nextLineFireTime,		FIELD_TIME),
	DEFINE_KEYFIELD( m_bArmorRechargeEnabled,FIELD_BOOLEAN, "ArmorRechargeEnabled" ),
	DEFINE_FIELD( m_bForceArmorRecharge,	FIELD_BOOLEAN),
	DEFINE_FIELD( m_iCurrentRechargeGoal,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bExtractingBugbait,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_iLeftHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iRightHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hHealTarget,			FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_bRegenerateHealth,	FIELD_BOOLEAN, "HealthRegenerateEnabled" ),
	// m_AssaultBehavior	(auto saved by AI)
	// m_LeadBehavior
	// DEFINE_FIELD( m_bStopLoopingSounds, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_USEFUNC( Use ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableArmorRecharge",	InputEnableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableArmorRecharge", InputDisableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"ChargeTarget",			InputChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ExtractBugbait",		InputExtractBugbait ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableHealthRegeneration",	InputEnableHealthRegeneration ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableHealthRegeneration",InputDisableHealthRegeneration ),

	// Outputs
	DEFINE_OUTPUT(m_OnFinishedExtractingBugbait,	"OnFinishedExtractingBugbait"),
	DEFINE_OUTPUT(m_OnFinishedChargingTarget,		"OnFinishedChargingTarget"),
	DEFINE_OUTPUT(m_OnPlayerUse,					"OnPlayerUse" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_vortigaunt, CNPC_Vortigaunt );

// for special behavior with rollermines
static bool IsRoller( CBaseEntity *pRoller )
{
	return FClassnameIs( pRoller, "npc_rollermine" );
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the player is below a certain percentage
//		    of their maximum health
// Input  : flPerc - Percentage to test against
// Output : Returns true if less than supplied parameter
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::PlayerBelowHealthPercentage( float flPerc )
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
		return false;

	if ( pPlayer->GetMaxHealth() == 0.0f )
		return false;

	float flHealthPerc = (float) pPlayer->GetHealth() / (float) pPlayer->GetMaxHealth();
	return ( flHealthPerc <= flPerc );
}

#define	VORT_START_EXTRACT_SENTENCE		500
#define VORT_FINISH_EXTRACT_SENTENCE	501

//------------------------------------------------------------------------------
// Purpose: Make the vort speak a line
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::SpeakSentence( int sentenceType )
{
	if (sentenceType == VORT_START_EXTRACT_SENTENCE)
	{
		Speak( VORT_EXTRACT_START );
	}
	else if (sentenceType == VORT_FINISH_EXTRACT_SENTENCE)
	{
		Speak( VORT_EXTRACT_FINISH );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{

	// Sets our target to the entity that we cached earlier.
	case TASK_VORTIGAUNT_GET_HEAL_TARGET:
	{
		if ( m_hHealTarget == NULL )
		{
			TaskFail( FAIL_NO_TARGET );
		}
		else
		{
			SetTarget( m_hHealTarget );
			TaskComplete();
		}
		
		break;
	}
	
	case TASK_VORTIGAUNT_EXTRACT_WARMUP:
	{
		ResetIdealActivity( (Activity) ACT_VORTIGAUNT_TO_ACTION );
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT:
	{
		SetActivity( (Activity) ACT_RANGE_ATTACK1 );
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT_COOLDOWN:
	{
		ResetIdealActivity( (Activity)ACT_VORTIGAUNT_TO_IDLE );
		break;
	}

	case TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT:
		{
			// Cheat, and fire both outputs
			m_OnFinishedExtractingBugbait.FireOutput( this, this );
			TaskComplete();
			break;
		}

	case TASK_VORTIGAUNT_HEAL:
	{
		AddGesture( (Activity) ACT_VORTIGAUNT_HEAL );
		m_eHealState = HEAL_STATE_WARMUP;
		TaskComplete();
		break;
	}

	case TASK_VORTIGAUNT_WAIT_FOR_PLAYER:
	{
		// Wait for the player to get near (before starting the bugbait sequence)
		break;
	}

	default:
		{
			BaseClass::StartTask( pTask );	
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			// If our enemy is gone, dead or out of sight, pick a new one
			if ( HasCondition( COND_ENEMY_OCCLUDED ) ||
				 GetEnemy() == NULL ||
				 GetEnemy()->IsAlive() == false )
			{
				CBaseEntity *pNewEnemy = BestEnemy();
				if ( pNewEnemy != NULL )
				{
					SetEnemy( pNewEnemy );
					SetState( NPC_STATE_COMBAT );
				}
			}

			BaseClass::RunTask( pTask );
			break;
		}
	
	case TASK_VORTIGAUNT_EXTRACT_WARMUP:
	{
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}
	
	case TASK_VORTIGAUNT_EXTRACT:
	{
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}

	case TASK_VORTIGAUNT_EXTRACT_COOLDOWN:
	{
		if ( IsActivityFinished() )
		{
			m_bExtractingBugbait = false;
			TaskComplete();
		}
		break;
	}

	case TASK_VORTIGAUNT_WAIT_FOR_PLAYER:
	{
		// Wait for the player to get near (before starting the bugbait sequence)
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer != NULL )
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
			SetTurnActivity();
			if ( GetMotor()->DeltaIdealYaw() < 10 )
			{
				// Wait for the player to get close enough
				if ( UTIL_DistApprox( GetAbsOrigin(), pPlayer->GetAbsOrigin() ) < 384 )
				{
					TaskComplete();
				}
			}
		}
		else
		{
			TaskFail( FAIL_NO_PLAYER );
		}
		break;
	}

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::AlertSound( void )
{
	if ( GetEnemy() != NULL && IsOkToCombatSpeak() )
	{
		Speak( VORT_ATTACK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows each sequence to have a different turn rate associated with it.
// Output : float CNPC_Vortigaunt::MaxYawSpeed
//-----------------------------------------------------------------------------
float CNPC_Vortigaunt::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 35;
		break;
	case ACT_WALK:
		return 35;
		break;
	case ACT_RUN:
		return 45;
		break;
	default:
		return 35;
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose : For innate range attack
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( GetEnemy() == NULL )
		return( COND_NONE );

	if ( gpGlobals->curtime < m_flNextAttack )
		return( COND_NONE );

	// dvs: Allow up-close range attacks for episodic as the vort's melee
	// attack is rather ineffective.
#ifndef HL2_EPISODIC
	if ( flDist <= 70 )
	{
		return( COND_TOO_CLOSE_TO_ATTACK );
	}
	else
#endif // HL2_EPISODIC
	if ( flDist > 1500 * 12 )	// 1500ft max
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_RANGE_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::HandleAnimEvent( animevent_t *pEvent )
{
	// Start of our heal loop
	if ( pEvent->event == AE_VORTIGAUNT_HEAL_PAUSE )
	{
		StartHealing();
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_ZAP_POWERUP )
	{
		// speed up attack when on hard
		if ( g_iSkillLevel == SKILL_HARD )
			 m_flPlaybackRate = 1.5;

		ArmBeam( -1 ,VORTIGAUNT_BEAM_HURT );
		ArmBeam( 1	,VORTIGAUNT_BEAM_HURT );
		BeamGlow();

		// Make hands glow if not already glowing
		if ( m_fGlowAge == 0 )
		{
			StartHandGlow( VORTIGAUNT_BEAM_HURT );
		}

		CPASAttenuationFilter filter( this );
		
		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.ZapPowerup", params, NULL ) )
		{
			EmitSound_t ep( params );
			ep.m_nPitch = 100 + m_iBeams * 10;
	
			EmitSound( filter, entindex(), ep );

			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_ZAP_SHOOT )
	{
		ClearBeams();

		ClearMultiDamage();

		ZapBeam( -1 );
		ZapBeam( 1 );
		EndHandGlow();

		EmitSound( "NPC_Vortigaunt.Shoot" );
		m_bStopLoopingSounds = true;
		ApplyMultiDamage();

		if ( m_bExtractingBugbait == true )
		{
			// Spawn bugbait!
			CBaseCombatWeapon *pWeapon = Weapon_Create( "weapon_bugbait" );
			if ( pWeapon )
			{
				// Starting above the body, spawn closer and closer to the vort until it's clear
				Vector vecSpawnOrigin = GetTarget()->WorldSpaceCenter() + Vector(0, 0, 32);
				int iNumAttempts = 4;
				Vector vecToVort = (WorldSpaceCenter() - vecSpawnOrigin);
				float flDistance = VectorNormalize( vecToVort ) / (iNumAttempts-1);
				int i = 0;
				for (; i < iNumAttempts; i++ )
				{
					trace_t tr;
					CTraceFilterSkipTwoEntities traceFilter( GetTarget(), this, COLLISION_GROUP_NONE );
					AI_TraceLine( vecSpawnOrigin, vecSpawnOrigin, MASK_SHOT, &traceFilter, &tr );

					if ( tr.fraction == 1.0 && !tr.m_pEnt )
					{
						// Make sure it can fit there
						AI_TraceHull( vecSpawnOrigin, vecSpawnOrigin, -Vector(16,16,16), Vector(16,16,48), MASK_SHOT, &traceFilter, &tr );
						if ( tr.fraction == 1.0 && !tr.m_pEnt )
							break;
					}

					//NDebugOverlay::Box( vecSpawnOrigin, pWeapon->WorldAlignMins(), pWeapon->WorldAlignMins(), 255,0,0, 64, 100 );

					// Move towards the vort
					vecSpawnOrigin = vecSpawnOrigin + (vecToVort * flDistance);
				}

				// HACK: If we've still failed, just spawn it on the player 
				if ( i == iNumAttempts )
				{
					CBasePlayer	*pPlayer = AI_GetSinglePlayer();
					if ( pPlayer )
					{
						vecSpawnOrigin = pPlayer->WorldSpaceCenter();
					}
				}

				//NDebugOverlay::Box( vecSpawnOrigin, -Vector(20,20,20), Vector(20,20,20), 0,255,0, 64, 100 );

				pWeapon->SetAbsOrigin( vecSpawnOrigin );
				pWeapon->Drop( Vector(0,0,1) );
			}

			CEffectData	data;
			
			data.m_vOrigin = GetTarget()->WorldSpaceCenter();
			data.m_vNormal = WorldSpaceCenter() - GetTarget()->WorldSpaceCenter();
			VectorNormalize( data.m_vNormal );
			
			data.m_flScale = 4;

			DispatchEffect( "AntlionGib", data );
		}

		m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 1.0, 2.0 );
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_ZAP_DONE )
	{
		ClearBeams();
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTGLOW )
	{
		// Make hands glow
		StartHandGlow( VORTIGAUNT_BEAM_HEAL );
		m_eHealState = HEAL_STATE_WARMUP;
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTBEAMS )
	{
		HealBeam(1);
		m_eHealState = HEAL_STATE_HEALING;
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTSOUND )
	{
		CPASAttenuationFilter filter( this );

		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.StartHealLoop", params, NULL ) )
		{
			EmitSound_t ep( params );
			ep.m_nPitch = 100 + m_iBeams * 10;

			EmitSound( filter, entindex(), ep );
			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_SWING_SOUND )
	{
		EmitSound( "NPC_Vortigaunt.Swing" );	
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_SHOOT_SOUNDSTART )
	{
		CPASAttenuationFilter filter( this );
		CSoundParameters params;
		if ( GetParametersForSound( "NPC_Vortigaunt.StartShootLoop", params, NULL ) )
		{
			EmitSound_t ep( params );
			ep.m_nPitch = 100 + m_iBeams * 10;

			EmitSound( filter, entindex(), ep );
			m_bStopLoopingSounds = true;
		}
		return;
	}

	if ( pEvent->event == AE_NPC_LEFTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepLeft", pEvent->eventtime );
		return;
	}

	if ( pEvent->event == AE_NPC_RIGHTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepRight", pEvent->eventtime );
		return;
	}
	
	BaseClass::HandleAnimEvent( pEvent );
}

//------------------------------------------------------------------------------
// Purpose : Translate some activites for the Vortigaunt
//------------------------------------------------------------------------------
Activity CNPC_Vortigaunt::NPC_TranslateActivity( Activity eNewActivity )
{
	// NOTE: This is a stand-in until the readiness system can handle non-weapon holding NPC's
	if ( eNewActivity == ACT_IDLE )
	{
		// More than relaxed means we're stimulated
		if ( GetReadinessLevel() >= AIRL_STIMULATED )
			return ACT_IDLE_STIMULATED;
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::UpdateOnRemove( void)
{
	ClearBeams();
	ClearHandGlow();

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Event_Killed( const CTakeDamageInfo &info )
{
	ClearBeams();
	ClearHandGlow();

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Spawn( void )
{
	// Allow multiple models (for slaves), but default to vortigaunt.mdl
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/vortigaunt.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	SetHullType( HULL_WIDE_HUMAN );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_vortigaunt_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 64 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_NO_HIT_PLAYER | bits_CAP_NO_HIT_SQUADMATES | bits_CAP_FRIENDLY_DMG_IMMUNE );
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_SHOOT );

	m_flNextHealTime		= 0;		// Next time allowed to heal player
	m_flEyeIntegRate		= 0.6;		// Got a big eyeball so turn it slower
	m_bForceArmorRecharge	= false;
	m_flHealHinderedTime	= 0.0f;
	
	m_nCurGlowIndex			= 0;
	m_pLeftHandGlow			= NULL;
	m_pRightHandGlow		= NULL;

	m_bStopLoopingSounds	= false;

	m_iLeftHandAttachment = LookupAttachment( VORTIGAUNT_LEFT_CLAW );
	m_iRightHandAttachment = LookupAttachment( VORTIGAUNT_RIGHT_CLAW );

	NPCInit();

	SetUse( &CNPC_Vortigaunt::Use );

	// Hold-over until spawn collisions are worked out
	m_bReadinessCapable = IsReadinessCapable();
	SetReadinessValue( 0.0f );
	SetReadinessSensitivity( random->RandomFloat( 0.7, 1.3 ) );

	// Default to off
	m_bHealBeamActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

	m_nLightningSprite = PrecacheModel("sprites/lgtning.vmt");

	PrecacheModel("sprites/physbeam.vmt");
	PrecacheModel("sprites/physring1.vmt");
	PrecacheModel("sprites/vortring1.vmt");

	PrecacheScriptSound( "NPC_Vortigaunt.SuitOn" );
	PrecacheScriptSound( "NPC_Vortigaunt.SuitCharge" );
	PrecacheScriptSound( "NPC_Vortigaunt.ZapPowerup" );
	PrecacheScriptSound( "NPC_Vortigaunt.Shoot" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartHealLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.Swing" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartShootLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepLeft" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepRight" );

	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	if ( !IsInAScript() && m_NPCState != NPC_STATE_SCRIPT )
	{
		if ( ShouldHealTarget( pActivator, true ) )
		{
			// We should heal the player. Don't say idle stuff.
		}
		else
		{
			// First, try to speak the +USE concept
			if ( IsOkToSpeakInResponseToPlayer() )
			{
				if ( !Speak( TLK_USE ) )
				{
					// If we haven't said hi, say that first
					if ( !SpokeConcept( TLK_HELLO ) )
					{
						Speak( TLK_HELLO );
					}
					else
					{
						Speak( TLK_IDLE );
					}
				}
				else
				{
					// Don't say hi after you've said your +USE speech
					SetSpokeConcept( TLK_HELLO, NULL );	
				}
			}
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Vortigaunt::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime < m_flPainTime )
		return;
	
	m_flPainTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

	Speak( VORT_PAIN );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Vortigaunt::DeathSound( const CTakeDamageInfo &info )
{
	Speak( VORT_DIE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	if ( (info.GetDamageType() & DMG_SHOCK) && FClassnameIs( info.GetAttacker(), GetClassname() ) )
	{
		// mask off damage from other vorts for now
		info.SetDamage( 0.01 );
	}

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f );
		}
		break;
	case 10:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			info.SetDamage( info.GetDamage() - 20 );
			if (info.GetDamage() <= 0)
			{
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
				info.SetDamage( 0.01 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_VORTIGAUNT_RANGE_ATTACK;
		break;

	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			ClearBeams();
			EndHandGlow();

			return SCHED_COMBAT_FACE;
			break;
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			baseType = BaseClass::TranslateSchedule(scheduleType);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			if (HasMemory(bits_MEMORY_INCOVER))
			{
				// Occasionally run somewhere so I don't just
				// stand around forever if my enemy is stationary
				if (random->RandomInt(0,5) == 5)
				{
					return SCHED_PATROL_RUN;
				}
				else
				{
					return SCHED_VORTIGAUNT_STAND;
				}
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			ClearBeams();
			EndHandGlow();

			return SCHED_RUN_RANDOM;
			break;
		}
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::ShouldHealTarget( CBaseEntity *pTarget, bool bResetTimer )
{
	// Don't bother if we're already healing
	if ( m_eHealState != HEAL_STATE_NONE )
		return false;

	// Must be allowed to do this
	if ( m_bArmorRechargeEnabled == false )
		return false;

	// Don't interrupt a script
	if ( IsInAScript() )
		return false;

	if ( bResetTimer )
	{
		m_flNextHealTime = gpGlobals->curtime;
	}
	else if ( m_flNextHealTime > gpGlobals->curtime ) 
	{
		return false;
	}
	
	if ( IsCurSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT ) )
		return false;

	// Can't heal if we're leading the player
	if ( IsLeading() )
		return false;

	// Must be a valid squad activity to do
	if ( IsStrategySlotRangeOccupied( SQUAD_SLOT_HEAL_PLAYER, SQUAD_SLOT_HEAL_PLAYER ) )
		return false;

	// Only consider things in here if the player is NOT at critical health
	if ( PlayerBelowHealthPercentage( PLAYER_CRITICAL_HEALTH_PERC ) == false )
	{
		// Don't heal when fighting
		if ( m_NPCState == NPC_STATE_COMBAT )
			return false;

		// No enemies
		if ( GetEnemy() )
			return false;

		// No recent damage
		if ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
			return false;
	}

	CBaseEntity *pEntity;
	if ( pTarget != NULL )
	{
		pEntity = pTarget;
	}
	else
	{
		// Need to be looking at the player to decide to heal them.
		if ( HasCondition( COND_SEE_PLAYER ) == false )
			return false;
	
		// Find a likely target in range
		pEntity = PlayerInRange( GetLocalOrigin(), HEAL_SEARCH_RANGE );
	}
	
	if ( pEntity != NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pEntity );
		
		// Make sure the player's got a suit
		if ( pPlayer->IsSuitEquipped() == false )
			return false;

		// FIXME: Huh?
		if ( pPlayer->GetFlags() & FL_NOTARGET )
			return false;

		// See if the player needs armor, or tau-cannon ammo
		if ( pPlayer->ArmorValue() < sk_vortigaunt_armor_charge.GetInt() )
		{
			OccupyStrategySlot( SQUAD_SLOT_HEAL_PLAYER );
			m_hHealTarget = pEntity;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectHealSchedule( void )
{
	// If our lead behavior has a goal, don't wait around to heal anyone
	if ( m_LeadBehavior.HasGoal() )
		return SCHED_NONE;

	// Cannot already be healing the player
	if ( m_eHealState != HEAL_STATE_NONE )
	{
		// Check for an interruption occuring
		if ( PlayerBelowHealthPercentage( PLAYER_CRITICAL_HEALTH_PERC ) == false && 
		   ( HasCondition( COND_HEAVY_DAMAGE ) ||
			 HasCondition( COND_LIGHT_DAMAGE ) ||
			 HasCondition( COND_NEW_ENEMY ) ) )
		{
			StopHealing( true );
			return SCHED_NONE;
		}

		// Decide if the player is at a valid range.  If so, stand and face
		if ( HasCondition( COND_VORTIGAUNT_HEAL_VALID ) )
			return SCHED_IDLE_STAND;

		// If the player is too far away or blocked, give chase
		if ( HasCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR ) ||
			 HasCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED ) )
		{
			return SCHED_VORTIGAUNT_RUN_TO_PLAYER;
		}

		// If the player is behind us, turn to face him
		if ( HasCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US ) )
			return SCHED_VORTIGAUNT_FACE_PLAYER;

		return SCHED_NONE;
	}

	// See if we should heal the player
	if ( ShouldHealTarget( NULL, false ) )
		return SCHED_VORTIGAUNT_HEAL;

	return SCHED_NONE;
}

//------------------------------------------------------------------------------
// Purpose: Select a schedule
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectSchedule( void )
{
	// If we're currently supposed to be doing something scripted, do it immediately.
	if ( m_bExtractingBugbait )
		return SCHED_VORTIGAUNT_EXTRACT_BUGBAIT;

	if ( m_bForceArmorRecharge )
	{
		m_flNextHealTime = 0;
		return SCHED_VORTIGAUNT_HEAL;
	}

	int schedule = SelectHealSchedule();
	if ( schedule != SCHED_NONE )
		return schedule;

	if ( HasCondition( COND_PLAYER_PUSHING ) && GlobalEntity_GetState("gordon_precriminal") != GLOBAL_ON )
		return SCHED_MOVE_AWAY;

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				return SCHED_TAKE_COVER_FROM_ENEMY;

			if ( HasCondition( COND_HEAR_DANGER ) )
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;

			if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
			{
				Speak( VORT_KILL );
			}

			// If I might hit the player shooting...
			else if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				if ( IsOkToCombatSpeak() && m_nextLineFireTime	< gpGlobals->curtime)
				{
					Speak( VORT_LINE_FIRE );
					m_nextLineFireTime = gpGlobals->curtime + 3.0f;
				}

				// Run to a new location or stand and aim
				if ( random->RandomInt( 0, 2 ) == 0 )
					return SCHED_ESTABLISH_LINE_OF_FIRE;

				return SCHED_COMBAT_FACE;
			}
		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:

		// Run from danger
		if ( HasCondition( COND_HEAR_DANGER ) )
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;

		// Heal a player if they can be
		if ( HasCondition( COND_VORTIGAUNT_CAN_HEAL ) )
			return SCHED_VORTIGAUNT_HEAL;

		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( VORT_KILL );
		}

		break;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::DeclineFollowing( void )
{
	Speak( VORT_POK );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if you're willing to be idly talked to by other friends.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::CanBeUsedAsAFriend( void )
{
	// We don't want to be used if we're busy
	if ( IsCurSchedule( SCHED_VORTIGAUNT_HEAL ) )
		return false;
	
	if ( IsCurSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT ) )
		return false;

	return BaseClass::CanBeUsedAsAFriend();
}

//-----------------------------------------------------------------------------
// Purpose: Start our heal loop
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartHealing( void )
{
	// Find the layer and stop it from moving forward in the cycle
	int nLayer = FindGestureLayer( (Activity) ACT_VORTIGAUNT_HEAL );
	SetLayerPlaybackRate( nLayer, 0.0f );

	// We're now in the healing loop
	m_eHealState = HEAL_STATE_HEALING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StopHealing( bool bInterrupt )
{
	m_eHealState = HEAL_STATE_NONE;
	m_bForceArmorRecharge = false;

	ClearBeams();
	EndHandGlow();
	VacateStrategySlot();
	
	// See if we're completely interrupting the heal or just ending normally
	if ( bInterrupt )
	{
		RemoveGesture( (Activity) ACT_VORTIGAUNT_HEAL );
		m_flNextHealTime = gpGlobals->curtime + 2.0f;
	}
	else
	{
		// Start our animation back up again
		int nLayer = FindGestureLayer( (Activity) ACT_VORTIGAUNT_HEAL );
		SetLayerPlaybackRate( nLayer, 1.0f );

		m_flNextHealTime = gpGlobals->curtime + VORTIGAUNT_HEAL_RECHARGE;
		m_OnFinishedChargingTarget.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update our heal schedule and gestures if we're currently healing
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::MaintainHealSchedule( void )
{
	// Need to be healing
	if ( m_eHealState == HEAL_STATE_NONE )
		return;

	// For now, we only heal the player
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer == NULL )
		return;

	// FIXME: Implement this in the animation
	// SetAimTarget( pPlayer );

	// If we're in the healing phase, heal our target (if able)
	if ( m_eHealState == HEAL_STATE_HEALING )
	{
		// FIXME: We need to have better logic controlling this
		if ( HasCondition( COND_VORTIGAUNT_HEAL_VALID ) )
		{
			// Clear our timer
			m_flHealHinderedTime = 0.0f;

			// Turn the heal beam back on if it's not already
			HealBeam( 1 );

			// Charge the suit's armor
			pPlayer->IncrementArmorValue( 1, sk_vortigaunt_armor_charge.GetInt() );

			// Stop healing once we've hit the maximum level we'll ever charge the player to.
			if ( pPlayer->ArmorValue() >= sk_vortigaunt_armor_charge.GetInt() )
			{
				SpeakIfAllowed( VORT_CURESTOP );
				StopHealing();
				return;
			}

			// Play the on sound or the looping charging sound
			if ( m_iSuitSound == 0 )
			{
				m_iSuitSound++;
				EmitSound( "NPC_Vortigaunt.SuitOn" );
				m_flSuitSoundTime = 0.56 + gpGlobals->curtime;
			}
			else if ( m_iSuitSound == 1 && m_flSuitSoundTime <= gpGlobals->curtime )
			{
				m_iSuitSound++;

				CPASAttenuationFilter filter( this, "NPC_Vortigaunt.SuitCharge" );
				filter.MakeReliable();
				EmitSound( filter, entindex(), "NPC_Vortigaunt.SuitCharge" );

				m_bStopLoopingSounds = true;
			}
		}
		else
		{
			// Turn off the main beam but leave the hand glow active
			ClearBeams();
			
			// Increment a counter to let us know how long we've failed
			m_flHealHinderedTime += gpGlobals->curtime - GetLastThink();
			
			if ( m_flHealHinderedTime > 2.0f )
			{
				// If too long, stop trying
				StopHealing();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do various non-schedule specific maintainence
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::PrescheduleThink( void )
{
	// Update glows
	if ( m_bGlowTurningOn )
	{
		m_bGlowTurningOn = false;

		if ( m_pLeftHandGlow != NULL )
		{
			m_pLeftHandGlow->SetScale( 0.5f, 1.0f );
			m_pLeftHandGlow->SetBrightness( 200, 1.0f );
			m_pLeftHandGlow->AnimateForTime( random->RandomFloat( 50.0f, 60.0f ), 10.0f );
		}

		if ( m_pRightHandGlow != NULL )
		{
			m_pRightHandGlow->SetScale( 0.75f, 1.0f );
			m_pRightHandGlow->SetBrightness( 200, 1.0f );
			m_pRightHandGlow->AnimateForTime( random->RandomFloat( 50.0f, 60.0f ), 10.0f );
		}
	}

	// Update our healing (if active)
	MaintainHealSchedule();

	// Let the base class have a go
	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::BuildScheduleTestBits( void )
{
	// Call to base
	BaseClass::BuildScheduleTestBits();

	// Allow healing to interrupt us if we're standing around
	if ( IsCurSchedule( SCHED_IDLE_STAND ) || 
		 IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		SetCustomInterruptCondition( COND_VORTIGAUNT_CAN_HEAL );
	}

	// Always interrupt when healing
	if ( m_eHealState != HEAL_STATE_NONE )
	{
		// Interrupt if we're not already adjusting
		if ( IsCurSchedule( SCHED_VORTIGAUNT_RUN_TO_PLAYER ) == false )
		{
			SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
			SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
		}

		// Interrupt if we're not already turning
		if ( IsCurSchedule( SCHED_VORTIGAUNT_FACE_PLAYER ) == false )
		{
			SetCustomInterruptCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Small beam from arm to nearby geometry
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::ArmBeam( int side, int beamType )
{
	trace_t tr;
	float flDist = 1.0;
	
	if ( m_iBeams >= VORTIGAUNT_MAX_BEAMS )
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
	Vector vecSrc = GetLocalOrigin() + up * 36 + right * side * 16 + forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = forward * random->RandomFloat( -1, 1 ) + right * side * random->RandomFloat( 0, 1 ) + up * random->RandomFloat( -1, 1 );
		trace_t tr1;
		AI_TraceLine ( vecSrc, vecSrc + vecAim * 512, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1);
		
		// Don't hit the sky
		if ( tr1.surface.flags & SURF_SKY )
			continue;

		// Choose a farther distance if we have one
		if ( flDist > tr1.fraction )
		{
			tr = tr1;
			flDist = tr.fraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	// NOTE: We don't want to make the sounds on the walls!
	// UTIL_ImpactTrace( &tr, DMG_CLUB );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );

	if (beamType == VORTIGAUNT_BEAM_HEAL)
	{
		m_pBeam[m_iBeams]->SetColor( 0, 0, 255 );
	}
	else
	{
		m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	}
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 12.5 );
	m_pBeam[m_iBeams]->SetEndWidth( 6.0f );
	m_pBeam[m_iBeams]->SetWidth( 1.0f );
	m_pBeam[m_iBeams]->LiveForTime( 3.0f );	// Fail-safe
	m_iBeams++;
}

//------------------------------------------------------------------------------
// Purpose : Put glowing sprites on hands
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::StartHandGlow( int beamType )
{
	m_bGlowTurningOn	= true;
	m_fGlowAge			= 0;

	if (beamType == VORTIGAUNT_BEAM_HEAL)
	{
		m_fGlowChangeTime = VORTIGAUNT_HEAL_GLOWGROW_TIME;
		m_fGlowScale = 0.6f;
	}
	else 
	{
		m_fGlowChangeTime = VORTIGAUNT_ZAP_GLOWGROW_TIME;
		m_fGlowScale = 0.8f;
	}

	if ( m_pLeftHandGlow == NULL )
	{
		if ( beamType == VORTIGAUNT_BEAM_HEAL )
		{
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/physring1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 4 );
		}
		else
		{
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/vortring1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 3 );
		}
		
		m_pLeftHandGlow->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 0, kRenderFxNone );
		m_pLeftHandGlow->SetBrightness( 0 );
		m_pLeftHandGlow->SetScale( 0 );
	}
	
	if ( beamType != VORTIGAUNT_BEAM_HEAL ) 
	{
		if ( m_pRightHandGlow == NULL )
		{	
			m_pRightHandGlow = CSprite::SpriteCreate( "sprites/vortring1.vmt", GetLocalOrigin(), FALSE );
			m_pRightHandGlow->SetAttachment( this, 4 );

			m_pRightHandGlow->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 0, kRenderFxNone );
			m_pRightHandGlow->SetBrightness( 0 );
			m_pRightHandGlow->SetScale( 0 );
		}
	}
}


//------------------------------------------------------------------------------
// Purpose: Fade glow from hands.
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::EndHandGlow()
{
	m_bGlowTurningOn	= false;
	m_fGlowChangeTime = VORTIGAUNT_GLOWFADE_TIME;

	if ( m_pLeftHandGlow )
	{
		m_pLeftHandGlow->SetScale( 0, 0.4f );
		m_pLeftHandGlow->FadeAndDie( 0.4f );
	}

	if ( m_pRightHandGlow )
	{
		m_pRightHandGlow->SetScale( 0, 0.4f );
		m_pRightHandGlow->FadeAndDie( 0.4f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: vortigaunt shoots at noisy body target (fixes problems in cover, etc)
//			NOTE: His weapon doesn't have any error
//-----------------------------------------------------------------------------
Vector CNPC_Vortigaunt::GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy )
{
	CBaseEntity *pEnemy = GetEnemy();

	if ( pEnemy )
	{
		return (pEnemy->BodyTarget(shootOrigin, bNoisy)-shootOrigin);
	}
	else
	{
		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		return forward;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( IsRoller( pEnemy ) )
	{
		CAI_BaseNPC *pNPC = pEnemy->MyNPCPointer();
		if ( pNPC && pNPC->GetEnemy() != NULL )
			return true;
		return false;
	}

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: Brighten all beams
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::BeamGlow( void )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates a blast where the beam has struck a target
// Input  : &vecOrigin - position to eminate from
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::CreateBeamBlast( const Vector &vecOrigin )
{
	CSprite *pBlastSprite = CSprite::SpriteCreate( "sprites/vortring1.vmt", vecOrigin, true );
	if ( pBlastSprite != NULL )
	{
		pBlastSprite->SetTransparency( kRenderTransAddFrameBlend, 255, 255, 255, 255, kRenderFxNone );
		pBlastSprite->SetBrightness( 255 );
		pBlastSprite->SetScale( random->RandomFloat( 1.0f, 1.5f ) );
		pBlastSprite->AnimateAndDie( 45.0f );
	}

	CPVSFilter filter( vecOrigin );
	te->GaussExplosion( filter, 0.0f, vecOrigin, Vector( 0, 0, 1 ), 0 );
}

#define COS_60	0.5f	// sqrt(1)/2

//-----------------------------------------------------------------------------
// Purpose: Heavy damage directly forward
// Input  : side - Handedness of the beam
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::ZapBeam( int side )
{
	if ( m_iBeams >= VORTIGAUNT_MAX_BEAMS )
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	Vector vecSrc = GetLocalOrigin() + up * 36;
	Vector vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.01f;
	
	// If we're too far off our center, the shot must miss!
	if ( DotProduct( vecAim, forward ) < COS_60 )
	{
		// Missed, so just shoot forward
		vecAim = forward + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	}
	else
	{
		// Within the tolerance, so shoot directly
		vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );
	}

	trace_t tr;

	if ( m_bExtractingBugbait == true )
	{
		CRagdollProp *pTest = dynamic_cast< CRagdollProp *>( GetTarget() );

		if ( pTest )
		{
			ragdoll_t *m_ragdoll = pTest->GetRagdoll();

			if ( m_ragdoll )
			{
				Vector vOrigin;
				m_ragdoll->list[0].pObject->GetPosition( &vOrigin, 0 );

				AI_TraceLine ( vecSrc, vOrigin, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			}

			CRagdollBoogie::Create( pTest, 200, gpGlobals->curtime, 1.0f );
		}
	}
	else
	{
		AI_TraceLine ( vecSrc, vecSrc + vecAim * 1024, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	}

	// Create a dynamic beam that will destroy itself when finished
	CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0 );
	if ( pBeam == NULL )
		return;

	pBeam->PointEntInit( tr.endpos, this );
	pBeam->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );
	pBeam->SetColor( 180, 255, 96 );
	pBeam->SetBrightness( 255 );
	pBeam->SetNoise( 3 );
	pBeam->LiveForTime( 0.2f );

	CBaseEntity *pEntity = tr.m_pEnt;
	if ( pEntity != NULL && m_takedamage )
	{
		CTakeDamageInfo dmgInfo( this, this, sk_vortigaunt_dmg_zap.GetFloat(), DMG_SHOCK );
		dmgInfo.SetDamagePosition( tr.endpos );
		VectorNormalize( vecAim );// not a unit vec yet
		// hit like a 5kg object flying 400 ft/s
		dmgInfo.SetDamageForce( 5 * 400 * 12 * vecAim );
		pEntity->DispatchTraceAttack( dmgInfo, vecAim, &tr );
	}

	// Create a cover for the end of the beam
	CreateBeamBlast( tr.endpos );
}

//------------------------------------------------------------------------------
// Purpose : Start the healing beam
// Input   : side - Handedness of the beam
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::HealBeam( int side )
{
	if ( m_iBeams >= VORTIGAUNT_MAX_BEAMS )
		return;

	if ( m_bHealBeamActive )
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	trace_t tr;
	Vector vecSrc = GetLocalOrigin() + up * 36;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/physbeam.vmt", 6.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->EntsInit( GetTarget(), this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );
	m_pBeam[m_iBeams]->SetColor( 128, 128, 255 );

	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 2 );
	m_pBeam[m_iBeams]->SetScrollRate( 15 );
	m_iBeams++;

	m_bHealBeamActive = true;
}

//------------------------------------------------------------------------------
// Purpose: Clear glow from hands immediately
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearHandGlow( void )
{
	if ( m_pLeftHandGlow != NULL )
	{
		UTIL_Remove( m_pLeftHandGlow );
		m_pLeftHandGlow = NULL;
	}

	if ( m_pRightHandGlow != NULL )
	{
		UTIL_Remove( m_pRightHandGlow );
		m_pRightHandGlow = NULL;
	}
}

//------------------------------------------------------------------------------
// Purpose: remove all beams
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearBeams( void )
{
	for (int i = 0; i < VORTIGAUNT_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}

	m_iBeams = 0;
	m_nSkin = 0;

	// Stop looping suit charge sound.
	if (m_iSuitSound > 1) 
	{
		StopSound( "NPC_Vortigaunt.SuitCharge" );
		m_iSuitSound = 0;
	}

	if ( m_bStopLoopingSounds )
	{
		StopSound( "NPC_Vortigaunt.StartHealLoop" );
		StopSound( "NPC_Vortigaunt.StartShootLoop" );
		StopSound( "NPC_Vortigaunt.SuitCharge" );
		StopSound( "NPC_Vortigaunt.Shoot" );
		StopSound( "NPC_Vortigaunt.ZapPowerup" );
		m_bStopLoopingSounds = false;
	}

	m_bHealBeamActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEnableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDisableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputChargeTarget( inputdata_t &data )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, data.value.String(), NULL, data.pActivator, data.pCaller );

	// Must be valid
	if ( pTarget == NULL )
	{
		DevMsg( 1, "Unable to charge from unknown entity: %s!\n", data.value.String() );
		return;
	}

	int playerArmor = (pTarget->IsPlayer()) ? ((CBasePlayer *)pTarget)->ArmorValue() : 0;

	if ( playerArmor >= 100 || ( pTarget->GetFlags() & FL_NOTARGET ) )
	{
		m_OnFinishedChargingTarget.FireOutput( this, this );
		return;
	}

	m_hHealTarget = pTarget;
	
	m_iCurrentRechargeGoal = playerArmor + sk_vortigaunt_armor_charge.GetInt();
	if ( m_iCurrentRechargeGoal > 100 )
		m_iCurrentRechargeGoal = 100;
	m_bForceArmorRecharge = true;

	SetCondition( COND_PROVOKED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputExtractBugbait( inputdata_t &data )
{
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, data.value.String(), NULL, data.pActivator, data.pCaller );

	// Must be valid
	if ( pTarget == NULL )
	{
		DevMsg( 1, "Unable to extract bugbait from unknown entity %s!\n", data.value.String() );
		return;
	}

	// Keep this as our target
	SetTarget( pTarget );

	// Start to extract
	m_bExtractingBugbait = true;
	SetSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT );
}


//-----------------------------------------------------------------------------
// The vort overloads the CNPC_PlayerCompanion version because he uses different
// rules. The player companion rules looked too sensitive to muck with.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.flTotalDist - pMoveGoal->directTrace.flDistObstructed < GetHullWidth() * 1.5 )
	{
		CAI_BaseNPC *pBlocker = pMoveGoal->directTrace.pObstruction->MyNPCPointer();
		if ( pBlocker && pBlocker->IsPlayerAlly() && !pBlocker->IsMoving() && !pBlocker->IsInAScript() )
		{
			if ( pBlocker->ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
				 pBlocker->ConditionInterruptsCurSchedule( COND_PLAYER_PUSHING ) )
			{
				// HACKHACK
				pBlocker->GetMotor()->SetIdealYawToTarget( WorldSpaceCenter() );
				pBlocker->SetSchedule( SCHED_MOVE_AWAY );
			}

		}
	}

	return BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

//-----------------------------------------------------------------------------
// Purpose: Allows the vortigaunt to use health regeneration
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEnableHealthRegeneration( inputdata_t &data )
{
	m_bRegenerateHealth = true;
}

//-----------------------------------------------------------------------------
// Purpose: Stops the vortigaunt from using health regeneration (default)
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDisableHealthRegeneration( inputdata_t &data )
{
	m_bRegenerateHealth = false;
}

extern int ACT_ANTLION_FLIP;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::IRelationPriority( CBaseEntity *pTarget )
{
	int priority = BaseClass::IRelationPriority( pTarget );

	if ( pTarget->Classify() == CLASS_ANTLION )
	{
		// Make vort prefer Antlions that are flipped onto their backs.
		// UNLESS it has a different enemy that could melee attack it while its back is turned.
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer();
		if ( pNPC && pNPC->GetActivity() == ACT_ANTLION_FLIP )
		{
			if ( GetEnemy() && GetEnemy() != pTarget )
			{
				// I have an enemy that is not this thing. If that enemy is near, I shouldn't become distracted.
				if ( GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin()) < Square(180) )
					return priority;
			}

			priority++;
		}
	}

	return priority;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the heal gesture can successfully reach the player
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::HealGestureHasLOS( void )
{
	//For now the player is always our target
	CBaseEntity *pTargetEnt = AI_GetSinglePlayer();
	if ( pTargetEnt == NULL )
		return false;

	// Find our left hand as the starting point
	Vector vecHandPos;
	QAngle vecHandAngle;
	GetAttachment( "rhand", vecHandPos, vecHandAngle );

	// Trace to our target, skipping ourselves and the target
	trace_t tr;
	CTraceFilterSkipTwoEntities filter( this, pTargetEnt, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecHandPos, pTargetEnt->WorldSpaceCenter(), MASK_SHOT, &filter, &tr );

	// Must be clear
	if ( tr.fraction < 1.0f || tr.startsolid || tr.allsolid )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions for our healing behavior
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::GatherHealConditions( void )
{
	// Start assuming that we'll succeed
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
	ClearCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );
	SetCondition( COND_VORTIGAUNT_HEAL_VALID );

	// For now we only act on the player
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer != NULL )
	{
		Vector vecToPlayer = ( pPlayer->WorldSpaceCenter() - WorldSpaceCenter() );

		// Make sure he's still within heal range
		if ( vecToPlayer.LengthSqr() > (HEAL_RANGE*HEAL_RANGE) )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR );
			ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}

		vecToPlayer.z = 0.0f;
		VectorNormalize( vecToPlayer );
		Vector facingDir = BodyDirection2D();

		// Check our direction towards the player
		if ( DotProduct( vecToPlayer, facingDir ) < VIEW_FIELD_NARROW )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US );
			ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}

		// Now ensure he's not blocked
		if ( HealGestureHasLOS() == false )
		{
			SetCondition( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED );
			ClearCondition( COND_VORTIGAUNT_HEAL_VALID );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gather conditions specific to this NPC
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::GatherConditions( void )
{
	// Call our base
	BaseClass::GatherConditions();

	ClearCondition( COND_VORTIGAUNT_CAN_HEAL );

	// See if we're able to heal now
	if ( m_eHealState == HEAL_STATE_NONE )
	{
		if ( ShouldHealTarget( NULL, false ) )
		{
			SetCondition( COND_VORTIGAUNT_CAN_HEAL );
		}
	}
	else
	{
		// Get our state for healing
		GatherHealConditions();
	}
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_vortigaunt, CNPC_Vortigaunt )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_TASK(TASK_VORTIGAUNT_HEAL)
	DECLARE_TASK(TASK_VORTIGAUNT_EXTRACT)
	DECLARE_TASK(TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT)
	DECLARE_TASK(TASK_VORTIGAUNT_WAIT_FOR_PLAYER)

	DECLARE_TASK( TASK_VORTIGAUNT_EXTRACT_WARMUP )
	DECLARE_TASK( TASK_VORTIGAUNT_EXTRACT_COOLDOWN )
	DECLARE_TASK( TASK_VORTIGAUNT_GET_HEAL_TARGET )

	DECLARE_ACTIVITY(ACT_VORTIGAUNT_AIM)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_START_HEAL)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_HEAL_LOOP)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_END_HEAL)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_ACTION)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_IDLE)
	DECLARE_ACTIVITY( ACT_VORTIGAUNT_HEAL )

	DECLARE_CONDITION( COND_VORTIGAUNT_CAN_HEAL )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_BLOCKED )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US )
	DECLARE_CONDITION( COND_VORTIGAUNT_HEAL_VALID )

	DECLARE_SQUADSLOT( SQUAD_SLOT_HEAL_PLAYER )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_LEFT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_RIGHT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_POWERUP )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_SHOOT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_DONE )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTGLOW )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTBEAMS )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTSOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SWING_SOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SHOOT_SOUNDSTART )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_PAUSE )

	//=========================================================
	// > SCHED_VORTIGAUNT_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_RANGE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_WAIT						0.2" // Wait a sec before killing beams
		""
		"	Interrupts"
	);


	//=========================================================
	// > SCHED_VORTIGAUNT_HEAL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_HEAL,

		"	Tasks"
		"		TASK_VORTIGAUNT_GET_HEAL_TARGET	0"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_STAND"
		"		TASK_GET_PATH_TO_TARGET			0"
		"		TASK_MOVE_TO_TARGET_RANGE		128"				// Move within 128 of target ent (client)
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_PLAYER				0"
		"		TASK_VORTIGAUNT_HEAL			0"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							2"					// repick IDLESTAND every two seconds."
		//"		TASK_TALKER_HEADRESET				0"					// reset head position"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_EXTRACT_BUGBAIT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_EXTRACT_BUGBAIT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_VORTIGAUNT_STAND"
		"		TASK_STOP_MOVING					0"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			128"				// Move within 128 of target ent (client)
		"		TASK_STOP_MOVING					0"
		"		TASK_VORTIGAUNT_WAIT_FOR_PLAYER		0"
		"		TASK_SPEAK_SENTENCE					500"				// Start extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_FACE_TARGET					0"
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_VORTIGAUNT_EXTRACT_WARMUP		0"
		"		TASK_VORTIGAUNT_EXTRACT				0"
		"		TASK_VORTIGAUNT_EXTRACT_COOLDOWN	0"
		"		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT	0"
		"		TASK_SPEAK_SENTENCE					501"				// Finish extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_WAIT							2"
		""
		"	Interrupts"
	)
	
	//=========================================================
	// > SCHED_VORTIGAUNT_FACE_PLAYER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_FACE_PLAYER,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_TARGET_PLAYER		0"
		"		TASK_FACE_PLAYER		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_RUN_TO_PLAYER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_RUN_TO_PLAYER,

		"	Tasks"
		"		TASK_TARGET_PLAYER					0"
		"		TASK_GET_PATH_TO_TARGET				0"
		"		TASK_MOVE_TO_TARGET_RANGE			96"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	);	

AI_END_CUSTOM_NPC()
