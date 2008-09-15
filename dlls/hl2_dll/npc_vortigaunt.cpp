//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "game.h"				// For skill levels
#include "globalstate.h"
#include "npc_talker.h"
#include "ai_motor.h"
#include "ai_schedule.h"
#include "scripted.h"
#include "basecombatweapon.h"
#include "soundent.h"
#include "npcevent.h"
#include "ai_hull.h"
#include "animation.h"
#include "ammodef.h"				// For DMG_CLUB
#include "Sprite.h"
#include "npc_vortigaunt.h"
#include "activitylist.h"
#include "player.h"
#include "items.h"
#include "basegrenade_shared.h"
#include "ai_interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"

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
#define	VORTIGAUNT_STOMP_DIST				40
#define	VORTIGAUNT_BEAM_HURT				0
#define	VORTIGAUNT_BEAM_HEAL				1
#define	VORTIGAUNT_STOMP_TURN_OFFSET		20

#define	VORTIGAUNT_LEFT_CLAW				"leftclaw"
#define	VORTIGAUNT_RIGHT_CLAW				"rightclaw"

#define VORT_CURE "VORT_CURE"
#define VORT_CURESTOP "VORT_CURESTOP"
#define VORT_CURE_INTERRUPT "VORT_CURE_INTERRUPT"
#define VORT_ATTACK "VORT_ATTACK"
#define VORT_MAD "VORT_MAD"
#define VORT_SHOT "VORT_SHOT"
#define VORT_PAIN "VORT_PAIN"
#define VORT_DIE "VORT_DIE"
#define VORT_KILL "VORT_KILL"
#define VORT_LINE_FIRE "VORT_LINE_FIRE"
#define VORT_POK "VORT_POK"
#define VORT_EXTRACT_START "VORT_EXTRACT_START"
#define VORT_EXTRACT_FINISH "VORT_EXTRACT_FINISH"

// Target must be within this range to heal
#define	HEAL_RANGE			256

ConVar	sk_vortigaunt_health( "sk_vortigaunt_health","0");
ConVar	sk_vortigaunt_armor_charge( "sk_vortigaunt_armor_charge","30");
ConVar	sk_vortigaunt_dmg_claw( "sk_vortigaunt_dmg_claw","0");
ConVar	sk_vortigaunt_dmg_rake( "sk_vortigaunt_dmg_rake","0");
ConVar	sk_vortigaunt_dmg_zap( "sk_vortigaunt_dmg_zap","0");

//=========================================================
// Vortigaunt activities
//=========================================================
int	ACT_VORTIGAUNT_AIM;
int ACT_VORTIGAUNT_START_HEAL;
int ACT_VORTIGAUNT_HEAL_LOOP;
int ACT_VORTIGAUNT_END_HEAL;
int ACT_VORTIGAUNT_TO_ACTION;
int ACT_VORTIGAUNT_TO_IDLE;
int ACT_VORTIGAUNT_STOMP;
int ACT_VORTIGAUNT_DEFEND;
int ACT_VORTIGAUNT_TO_DEFEND;
int ACT_VORTIGAUNT_FROM_DEFEND;

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
int AE_VORTIGAUNT_KICK;
int AE_VORTIGAUNT_STOMP;
int AE_VORTIGAUNT_HEAL_STARTSOUND;
int AE_VORTIGAUNT_SWING_SOUND;
int AE_VORTIGAUNT_SHOOT_SOUNDSTART;
int AE_VORTIGAUNT_DEFEND_BEAMS;


//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionVortigauntStomp		= 0;
int	g_interactionVortigauntStompFail	= 0;
int	g_interactionVortigauntStompHit		= 0;
int	g_interactionVortigauntKick			= 0;
int	g_interactionVortigauntClaw			= 0;

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Vortigaunt )

	DEFINE_FIELD( m_flNextNPCThink,		FIELD_TIME),
	DEFINE_ARRAY( m_pBeam,				FIELD_EHANDLE, VORTIGAUNT_MAX_BEAMS ),
	DEFINE_FIELD( m_iBeams,				FIELD_INTEGER),
	DEFINE_FIELD( m_nLightningSprite,		FIELD_INTEGER),
	DEFINE_FIELD( m_fGlowAge,				FIELD_FLOAT),
	DEFINE_FIELD( m_fGlowScale,			FIELD_FLOAT),
	DEFINE_FIELD( m_fGlowChangeTime,		FIELD_FLOAT),
	DEFINE_FIELD( m_bGlowTurningOn,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_nCurGlowIndex,		FIELD_INTEGER),
	DEFINE_FIELD( m_pLeftHandGlow,		FIELD_EHANDLE),
	DEFINE_FIELD( m_pRightHandGlow,		FIELD_EHANDLE),
	DEFINE_FIELD( m_flNextHealTime,		FIELD_TIME),
	DEFINE_FIELD( m_nCurrentHealAmt,	FIELD_INTEGER),
	DEFINE_FIELD( m_nLastArmorAmt,		FIELD_INTEGER),
	DEFINE_FIELD( m_iSuitSound,			FIELD_INTEGER),
	DEFINE_FIELD( m_flSuitSoundTime,		FIELD_TIME),
	DEFINE_FIELD( m_painTime,				FIELD_TIME),
	DEFINE_FIELD( m_nextLineFireTime,		FIELD_TIME),
	DEFINE_FIELD( m_bInBarnacleMouth,		FIELD_BOOLEAN),
	DEFINE_KEYFIELD( m_bArmorRechargeEnabled, FIELD_BOOLEAN, "ArmorRechargeEnabled" ),
	DEFINE_FIELD( m_bForceArmorRecharge,	FIELD_BOOLEAN),
	DEFINE_FIELD( m_iCurrentRechargeGoal,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hVictim,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_bExtractingBugbait,		FIELD_BOOLEAN),
	DEFINE_FIELD( m_iLeftHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iRightHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hHealTarget,			FIELD_EHANDLE ),

	//								m_AssaultBehavior	(auto saved by AI)
	//								m_LeadBehavior		(auto saved by AI)
	// DEFINE_FIELD( m_bStopLoopingSounds, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_USEFUNC( Use ),

	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableArmorRecharge",	InputEnableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableArmorRecharge", InputDisableArmorRecharge ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"ChargeTarget",			InputChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ExtractBugbait",		InputExtractBugbait ),

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
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsHealPositionValid(void)
{
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( !pPlayer )
		return false;

	Vector vecToPlayer = ( pPlayer->EyePosition() - EyePosition() );
	// Make sure he's still within heal range
	if ( vecToPlayer.LengthSqr() > (HEAL_RANGE*HEAL_RANGE) )
		return false;

	// Make sure the player stays in front of me.
	// Don't do a straight viewcone check, because my head turns to follow the player, and
	// that will allow the player to move too far around me. Instead, do a viewcone check
	// from my base facing.
	vecToPlayer.z = 0;
	VectorNormalize( vecToPlayer );
	Vector facingDir = BodyDirection2D();
	float flDot = DotProduct( vecToPlayer, facingDir );
	if ( flDot < VIEW_FIELD_NARROW )
		return false;

	// Now ensure he's still visible
	return ( FVisible( pPlayer ) );
}

//-----------------------------------------------------------------------------
// Purpose: Check the heal position. If it's bad, break out current schedule
//			and setup for healing at a later time.
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::CheckHealPosition( void )
{
	if ( IsHealPositionValid() )
		return true;

	// Heal position isn't valid. Abort heal.
	Speak( VORT_CURE_INTERRUPT );
	ClearBeams();
	EndHandGlow();
	m_flNextHealTime = gpGlobals->curtime + 5.0;
	TaskFail(FAIL_BAD_POSITION);
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
#define VORT_HEAL_SENTENCE				0
#define VORT_DONE_HEAL_SENTENCE			1
#define	VORT_START_EXTRACT_SENTENCE		2
#define VORT_FINISH_EXTRACT_SENTENCE	3

void CNPC_Vortigaunt::SpeakSentence( int sentenceType )
{
	if (sentenceType == VORT_HEAL_SENTENCE)
	{
		if ( IsHealPositionValid() )
		{
			Speak( VORT_CURE );
		}
	}
	else if (sentenceType == VORT_DONE_HEAL_SENTENCE)
	{
		Speak( VORT_CURESTOP );
	}
	else if (sentenceType == VORT_START_EXTRACT_SENTENCE)
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
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{

	case TASK_VORTIGAUNT_GET_HEAL_TARGET:
	{
		// Sets our target to the entity that we cached earlier.
		if ( !m_hHealTarget )
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

	case TASK_VORTIGAUNT_HEAL_WARMUP:
		{
			ResetIdealActivity( (Activity)ACT_VORTIGAUNT_START_HEAL );
			break;
		}
	case TASK_VORTIGAUNT_HEAL:
		{
			ResetIdealActivity( (Activity)ACT_VORTIGAUNT_HEAL_LOOP );
			break;
		}
	case TASK_VORTIGAUNT_FACE_STOMP:
		{
			if (GetEnemy()!=NULL)
			{
				GetMotor()->SetIdealYaw( CalcIdealYaw ( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET );
				SetTurnActivity(); 
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
			}
			break;
		}
	case TASK_VORTIGAUNT_STOMP_ATTACK:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( (Activity)ACT_VORTIGAUNT_STOMP );
			break;
		}
	case TASK_VORTIGAUNT_GRENADE_KILL:
		{
			if (GetTarget() == NULL)
			{
				TaskComplete();
			}
			// Used to set delay between beam shot and damage
			ClearWait();

			break;
		}
	case TASK_VORTIGAUNT_ZAP_GRENADE_OWNER:
		{
			if (GetEnemy() == NULL)
			{
				TaskComplete();
			}
			// Used to set delay between beam shot and damage
			ClearWait();

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
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			if (GetEnemy() != NULL)
			{
				if (GetEnemy()->IsPlayer())
				{
					m_flPlaybackRate = 1.5;
				}
				if (!GetEnemy()->IsAlive())
				{
					if( IsActivityFinished() )
					{
						TaskComplete();
						break;
					}
				}
				// This is along attack sequence so if the enemy
				// becomes occluded bail
				if (HasCondition( COND_ENEMY_OCCLUDED ))
				{
					TaskComplete();
					break;
				}
			}
			BaseClass::RunTask( pTask );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			if (GetEnemy() == NULL)
			{
				if ( IsActivityFinished() )
				{
					TaskComplete();
				}
			}
			else
			{
				BaseClass::RunTask( pTask );
			}
			break;	
		}
	case TASK_MELEE_ATTACK2:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_STOMP_ATTACK:
		{
			// Face enemy slightly off center 
			if (GetEnemy() != NULL)
			{
				GetMotor()->SetIdealYawAndUpdate( CalcIdealYaw ( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET, AI_KEEP_YAW_SPEED );
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_FACE_STOMP:
		{
			if (GetEnemy()!=NULL)
			{
				GetMotor()->SetIdealYawAndUpdate( CalcIdealYaw( GetEnemy()->GetLocalOrigin() ) + VORTIGAUNT_STOMP_TURN_OFFSET );

				if ( FacingIdeal() )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
			}
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

	case TASK_VORTIGAUNT_HEAL_WARMUP:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			if ( !CheckHealPosition() )
				return;
			break;
		}
	case TASK_VORTIGAUNT_GRENADE_KILL:
		{
			if (GetTarget() == NULL)
			{
				TaskComplete();
				break;
			}

			// Face the grenade
			GetMotor()->SetIdealYawToTargetAndUpdate( GetTarget()->GetLocalOrigin() );

			// Shoot beam when grenade is close
			float flDist = (GetTarget()->GetLocalOrigin() - GetLocalOrigin()).Length();

			if (flDist < 225)
			{
				ClawBeam(GetTarget(),16, m_iLeftHandAttachment );
				ClawBeam(GetTarget(),16, m_iRightHandAttachment );
			}
			if (flDist < 150)
			{
				CBaseGrenade* pGrenade = dynamic_cast<CBaseGrenade*>((CBaseEntity*)GetTarget());
				if (!pGrenade)
				{
					TaskComplete();
				}
				pGrenade->Detonate();
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_ZAP_GRENADE_OWNER:
		{
			if (GetEnemy() == NULL)
			{
				TaskComplete();
				break;
			}

			// Face the grenade owner
			GetMotor()->SetIdealYawToTargetAndUpdate( GetEnemy()->GetLocalOrigin() );

			// Shoot beam only if enemy is close enough
			float flDist = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length();
			if (flDist < 350)
			{
				if (!IsWaitSet()					||
					IsWaitFinished() )
				{
					ClawBeam(GetEnemy(),5, m_iLeftHandAttachment );
					ClawBeam(GetEnemy(),5, m_iRightHandAttachment );

					// Set a delay and then we do damage.  Store in
					// m_hVictim in case we are interrupted
					if (!IsWaitSet())
					{
						SetWait( 0.2 );
						m_hVictim		 = GetEnemy();
					}
				}
				else
				{
					if (m_hVictim != NULL)
					{
						CTakeDamageInfo info( this, this, 15, DMG_SHOCK );
						CalculateMeleeDamageForce( &info, (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()), GetEnemy()->GetAbsOrigin() );
						m_hVictim->MyNPCPointer()->TakeDamage( info );
					}
					TaskComplete();
				}
			}
			else
			{
				// To far away to shoot
				TaskComplete();
			}
			break;
		}
	case TASK_VORTIGAUNT_HEAL:
		{
			CBasePlayer *pPlayer = AI_GetSinglePlayer();
			if (pPlayer)
			{
				if (!CheckHealPosition())
					return;

				// Charge the suit's armor
				pPlayer->IncrementArmorValue( 1, m_iCurrentRechargeGoal );

				// Stop healing once we've hit the maximum level we'll ever charge the player to.
				if ( pPlayer->ArmorValue() >= m_iCurrentRechargeGoal )
				{
					m_bForceArmorRecharge = false;
					m_flNextHealTime = gpGlobals->curtime + VORTIGAUNT_HEAL_RECHARGE;
					ClearBeams();
					EndHandGlow();
					TaskComplete();
					m_OnFinishedChargingTarget.FireOutput( this, this );
					return;
				}

				// ------------------------------------------
				//  Suit sound
				// ------------------------------------------
				// Play the on sound or the looping charging sound
				if (!m_iSuitSound)
				{
					m_iSuitSound++;
					EmitSound( "NPC_Vortigaunt.SuitOn" );
					m_flSuitSoundTime = 0.56 + gpGlobals->curtime;
				}
				if ((m_iSuitSound == 1) && (m_flSuitSoundTime <= gpGlobals->curtime))
				{
					m_iSuitSound++;

					CPASAttenuationFilter filter( this, "NPC_Vortigaunt.SuitCharge" );
					filter.MakeReliable();

					EmitSound( filter, entindex(), "NPC_Vortigaunt.SuitCharge" );

					m_bStopLoopingSounds = true;
				}
			}

			break;
		}

	case TASK_VORTIGAUNT_WAIT_FOR_PLAYER:
		{
			// Wait for the player to get near (before starting the bugbait sequence)
			// Get edict for one player
			CBasePlayer *pPlayer = AI_GetSinglePlayer();
			if ( pPlayer )
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
				TaskFail(FAIL_NO_PLAYER);
			}
		}
		break;


	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}




//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_Vortigaunt::GetSoundInterests ( void) 
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_Vortigaunt::Classify ( void )
{
	return	CLASS_VORTIGAUNT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_Vortigaunt::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( VORT_ATTACK );
		}
	}

}
//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
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
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}

	if ( gpGlobals->curtime < m_flNextAttack )
	{
		return( COND_NONE );
	}

	// Range attack is ineffective on manhack so never use it
	// Melee attack looks a lot better anyway
	if (GetEnemy()->Classify() == CLASS_MANHACK)
	{
		return( COND_NONE );
	}

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
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 70)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		// If I'm not facing attack clear TOOFAR which may have been set by range attack
		// Otherwise we may try to back away even when melee attack is valid
		ClearCondition(COND_TOO_FAR_TO_ATTACK);
		return COND_NOT_FACING_ATTACK;
	}

	// no melee attacks against rollermins
	if ( IsRoller(GetEnemy()) )
		return COND_NONE;

	return COND_CAN_MELEE_ATTACK1;
}

//------------------------------------------------------------------------------
// Purpose : Returns who I hit on a kick (or NULL)
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity*	CNPC_Vortigaunt::Kick( void )
{
	trace_t tr;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	Vector vecStart = WorldSpaceCenter();
	Vector vecEnd = vecStart + (forward * 70);

	AI_TraceHull( vecStart, vecEnd, Vector(-16,-16,-18), Vector(16,16,18), 
		MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	return tr.m_pEnt;
}

//------------------------------------------------------------------------------
// Purpose : Vortigaunt claws at his enemy
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Claw( int iAttachment)
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 40, Vector(-10,-10,-10), Vector(10,10,10),sk_vortigaunt_dmg_claw.GetFloat(), DMG_SLASH );
	if ( pHurt )
	{
		pHurt->ViewPunch( QAngle(5,0,-18) );
		// If hit manhack make temp glow
		if (pHurt->Classify() == CLASS_MANHACK)
		{
			ClawBeam( pHurt, 16, iAttachment );
		}
		// Play a random attack hit sound
		EmitSound( "NPC_Vortigaunt.Claw" );
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Vortigaunt::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_VORTIGAUNT_CLAW_LEFT )
	{
		Claw( m_iLeftHandAttachment );
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_CLAW_RIGHT )
	{
		Claw( m_iRightHandAttachment );
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
		//m_nSkin = m_iBeams / 2;
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

		m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 4.0 );
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_ZAP_DONE )
	{
		ClearBeams( );
		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTGLOW )
	{
		// Make hands glow
		StartHandGlow(VORTIGAUNT_BEAM_HEAL);
		return;
	}
	
	if ( pEvent->event == AE_VORTIGAUNT_HEAL_STARTBEAMS )
	{
		if ( !CheckHealPosition())
			return;

		HealBeam(-1);
		return;

		//m_nSkin = m_iBeams / 2;
	}

	if( pEvent->event == AE_VORTIGAUNT_KICK )
	{
		CBaseEntity *pHurt = Kick();
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );

		if (pBCC)
		{
			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );

			if ( pBCC->DispatchInteraction( g_interactionVortigauntKick, NULL, this ) )
			{
				pBCC->ApplyAbsVelocityImpulse( forward * 300 + up * 250 );

				CTakeDamageInfo info( this, this, pBCC->m_iHealth+1, DMG_CLUB );
				CalculateMeleeDamageForce( &info, forward, pHurt->GetAbsOrigin() );
				pBCC->TakeDamage( info );
			}
			else
			{
				pBCC->ViewPunch( QAngle(15,0,0) );

				CTakeDamageInfo info( this, this, sk_vortigaunt_dmg_claw.GetFloat(), DMG_CLUB );
				CalculateMeleeDamageForce( &info, forward, pHurt->GetAbsOrigin() );
				pBCC->TakeDamage( info );
			}
			EmitSound( "NPC_Vortigaunt.Kick" );
		}

		return;
	}

	if ( pEvent->event == AE_VORTIGAUNT_STOMP )
	{
		CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();

		if ( pBCC )
		{
			float fDist = (pBCC->GetLocalOrigin() - GetLocalOrigin()).Length();
			if (fDist > VORTIGAUNT_STOMP_DIST)
			{
				pBCC->DispatchInteraction( g_interactionVortigauntStompFail, NULL, this );
				return;
			}
			else
			{
				pBCC->DispatchInteraction( g_interactionVortigauntStompHit, NULL, this );
			}
			EmitSound( "NPC_Vortigaunt.Kick" );
		}

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

	if ( pEvent->event == AE_VORTIGAUNT_DEFEND_BEAMS )
	{
		DefendBeams();
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
// Purpose : Returnst true if entity is stompable
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Vortigaunt::IsStompable(CBaseEntity *pEntity)
{
	if (!pEntity)
		return false;

	float fDist = (pEntity->GetLocalOrigin() - GetLocalOrigin()).Length();
	if (fDist > VORTIGAUNT_STOMP_DIST)
		return false;

	CBaseCombatCharacter* pBCC = (CBaseCombatCharacter *)pEntity;
	
	if ( pBCC && pBCC->DispatchInteraction( g_interactionVortigauntStomp, NULL, this ) )
		return true;

	return false;
}

//------------------------------------------------------------------------------
// Purpose : Translate some activites for the Vortigaunt
// Input   :
// Output  :
//------------------------------------------------------------------------------
Activity CNPC_Vortigaunt::NPC_TranslateActivity( Activity eNewActivity )
{
	if ((eNewActivity == ACT_SIGNAL3)									&& 
		(SelectWeightedSequence ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE)	)
	{
		eNewActivity = ACT_IDLE;
	}

	if (eNewActivity == ACT_IDLE)
	{
		if ( m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT)
		{
			return ACT_IDLE_ANGRY;
		}
	}
	else if (eNewActivity == ACT_MELEE_ATTACK1)
	{
		// If enemy is low pick ATTACK2 (kick)
		if (GetEnemy() != NULL && (GetEnemy()->EyePosition().z - GetLocalOrigin().z) < 20)
		{
			return ACT_MELEE_ATTACK2;
		}
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//------------------------------------------------------------------------------
// Purpose : If I've been in alert for a while and nothing's happened, 
//			 go back to idle
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_Vortigaunt::ShouldGoToIdleState( void )
{
	if (m_flLastStateChangeTime + 10 < gpGlobals->curtime)
	{
		return true;
	}
	return false;
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
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::Event_Killed( const CTakeDamageInfo &info )
{
	ClearBeams();
	ClearHandGlow();

	BaseClass::Event_Killed( info );
}

bool CNPC_Vortigaunt::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_LeadBehavior );
	//AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}
//=========================================================
// Spawn
//=========================================================
void CNPC_Vortigaunt::Spawn()
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

	SetHullType(HULL_WIDE_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_vortigaunt_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 64 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	GetExpresser()->SetVoicePitch( random->RandomInt( 85, 110 ) );

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_NO_HIT_PLAYER );
	CapabilitiesAdd	( bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesAdd	( bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd	( bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	
	m_flNextHealTime		= 0;		// Next time allowed to heal player
	m_flEyeIntegRate		= 0.6;		// Got a big eyeball so turn it slower
	m_hVictim				= NULL;
	m_bForceArmorRecharge	= false;
	
	m_nCurGlowIndex			= 0;
	m_pLeftHandGlow			= NULL;
	m_pRightHandGlow		= NULL;

	m_bStopLoopingSounds	= false;

	m_iLeftHandAttachment = LookupAttachment( VORTIGAUNT_LEFT_CLAW );
	m_iRightHandAttachment = LookupAttachment( VORTIGAUNT_RIGHT_CLAW );

	NPCInit();

	SetUse( &CNPC_Vortigaunt::Use );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Vortigaunt::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

	m_nLightningSprite = PrecacheModel("sprites/lgtning.vmt");

	PrecacheModel("sprites/blueglow1.vmt");
	PrecacheModel("sprites/greenglow1.vmt");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	PrecacheScriptSound( "NPC_Vortigaunt.SuitOn" );
	PrecacheScriptSound( "NPC_Vortigaunt.SuitCharge" );
	PrecacheScriptSound( "NPC_Vortigaunt.Claw" );
	PrecacheScriptSound( "NPC_Vortigaunt.ZapPowerup" );
	PrecacheScriptSound( "NPC_Vortigaunt.Shoot" );
	PrecacheScriptSound( "NPC_Vortigaunt.Kick" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartHealLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.Swing" );
	PrecacheScriptSound( "NPC_Vortigaunt.StartShootLoop" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepLeft" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepRight" );
	PrecacheScriptSound( "NPC_Vortigaunt.ClawBeam" );

	BaseClass::Precache();
}	

// Init talk data
void CNPC_Vortigaunt::TalkInit()
{
	BaseClass::TalkInit();

	// vortigaunt will try to talk to friends in this order:
	m_szFriends[0] = "npc_vortigaunt";
	m_szFriends[1] = "npc_citizen";

	// get voice for head - just one barney voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	if ( !IsInAScript() && m_NPCState != NPC_STATE_SCRIPT )
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

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Vortigaunt::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::OnTakeDamage_Alive( info );
	if (!IsAlive())
	{
		return ret;
	}

	if ( m_NPCState != NPC_STATE_PRONE && (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( GetEnemy() == NULL )
		{
			// If I'm already suspicious, get mad
			if (m_afMemory & bits_MEMORY_SUSPICIOUS)
			{
				// Alright, now I'm pissed!
				Speak( VORT_MAD );

				Remember( bits_MEMORY_PROVOKED );

				// Allowed to hit the player now!
				CapabilitiesRemove(bits_CAP_NO_HIT_PLAYER);

				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( VORT_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && (m_lifeState != LIFE_DEAD ))
		{
			Speak( VORT_SHOT );
		}
	}
	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CNPC_Vortigaunt::PainSound( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_painTime)
		return;
	
	m_painTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

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
// Input  :
// Output :
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
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Vortigaunt::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		return SCHED_VORTIGAUNT_RANGE_ATTACK;
		break;

	case SCHED_MELEE_ATTACK1:
		if (IsStompable(GetEnemy()))
		{
			return SCHED_VORTIGAUNT_STOMP_ATTACK;
		}
		else
		{
			// Tell my enemy I'm about to punch them so they can do something
			// special if they want to
			if ((GetEnemy() != NULL) && (GetEnemy()->MyNPCPointer() != NULL))
			{
				Vector vFacingDir = BodyDirection2D( );
				Vector vClawPos = EyePosition() + vFacingDir*30;
				GetEnemy()->MyNPCPointer()->DispatchInteraction( g_interactionVortigauntClaw, &vClawPos, this );
			}
			return SCHED_VORTIGAUNT_MELEE_ATTACK;
		}
		break;

	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
			{
				// just look straight ahead
				return SCHED_VORTIGAUNT_STAND;
			}
			else
				return baseType;	
			break;

		}
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
bool CNPC_Vortigaunt::ShouldHealTarget( void )
{
	if ( IsInAScript() )
		return false;

	// Must be allowed to do this
	if ( m_bArmorRechargeEnabled == false )
		return false;

	// Can't be too soon
	if ( m_flNextHealTime > gpGlobals->curtime ) 
		return false;

	// Don't heal when fighting
	if ( m_NPCState == NPC_STATE_COMBAT )
		return false;

	if ( GetEnemy() )
		return false;

	if ( IsCurSchedule( SCHED_VORTIGAUNT_EXTRACT_BUGBAIT ) )
		return false;

	// Can't heal if we're leading the player
	if ( IsLeading() )
		return false;

	// Need to be looking at the player
	if ( !HasCondition( COND_SEE_PLAYER ) )
		return false;

	// Can't be saying something
//	if ( GetExpresser()->IsSpeaking() )
//		return false;

	// Find a likely target in range
	CBaseEntity *pEntity = PlayerInRange( GetLocalOrigin(), HEAL_RANGE );

	if ( pEntity != NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pEntity );
		
		// Make sure the player's got a suit
		if ( !pPlayer->IsSuitEquipped() )
			return false;

		if ( pPlayer->GetFlags() & FL_NOTARGET )
			return false;

		// See if the player needs armor, or tau-cannon ammo
		if ( pPlayer->ArmorValue() < sk_vortigaunt_armor_charge.GetInt() )
		{
			m_iCurrentRechargeGoal = pPlayer->ArmorValue() + sk_vortigaunt_armor_charge.GetInt();
			if ( m_iCurrentRechargeGoal > 100 )
				m_iCurrentRechargeGoal = 100;
				
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

	// See if we should heal the player
	if ( ShouldHealTarget() )
		return SCHED_VORTIGAUNT_HEAL;

	return SCHED_NONE;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_Vortigaunt::SelectSchedule( void )
{
	ClearBeams();
	EndHandGlow();

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
	{
		return SCHED_MOVE_AWAY;
	}

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_VORTIGAUNT_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_VORTIGAUNT_BARNACLE_HIT;
			}
		}
	case NPC_STATE_COMBAT:
		{
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
			{
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}

			if ( HasCondition( COND_HEAR_DANGER ) )
			{
				return SCHED_TAKE_COVER_FROM_BEST_SOUND;
			}

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
				if (random->RandomInt(0,2) == 0)
				{
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_COMBAT_FACE;
				}
			}
		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:

		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}

		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( VORT_KILL );
		}

		break;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Vortigaunt::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Force choosing of a new schedule
		ClearSchedule();
		m_bInBarnacleMouth	= true;
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		SetIdealState( NPC_STATE_IDLE );

		m_bInBarnacleMouth	= false;
		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			SetGroundEntity( NULL );
		}
		SetIdealState( NPC_STATE_PRONE );
		
		CTakeDamageInfo info;
		PainSound( info );
		return true;
	}
	return false;
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
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::PrescheduleThink( void )
{
	if ( m_bGlowTurningOn )
	{
		m_bGlowTurningOn = false;

		if ( m_pLeftHandGlow != NULL )
		{
			m_pLeftHandGlow->SetScale( 0.5f, 2.0f );
			m_pLeftHandGlow->SetBrightness( 200, 2.0f );
		}

		if ( m_pRightHandGlow != NULL )
		{
			m_pRightHandGlow->SetScale( 0.5f, 2.0f );
			m_pRightHandGlow->SetBrightness( 200, 2.0f );
		}
	}

	// See if we're able to heal now
	if ( ShouldHealTarget() )
	{
		SetCondition( COND_VORTIGAUNT_CAN_HEAL );
	}
	else
	{
		ClearCondition( COND_VORTIGAUNT_CAN_HEAL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_IDLE_STAND ) || IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		SetCustomInterruptCondition( COND_VORTIGAUNT_CAN_HEAL );
	}

	if ( ( ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
		   IsCurSchedule(SCHED_HIDE_AND_RELOAD ) || 
		   IsCurSchedule(SCHED_RELOAD ) || 
		   IsCurSchedule(SCHED_STANDOFF ) || 
		   IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY ) || 
		   IsCurSchedule(SCHED_COMBAT_FACE ) || 
		   IsCurSchedule(SCHED_ALERT_FACE )  ||
		   IsCurSchedule(SCHED_COMBAT_STAND ) || 
		   IsCurSchedule(SCHED_ALERT_FACE_BESTSOUND) ||
		   IsCurSchedule(SCHED_ALERT_STAND) ) )
	{
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
	}
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================
void CNPC_Vortigaunt::ArmBeam( int side, int beamType )
{
	trace_t tr;
	float flDist = 1.0;
	
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );
	Vector vecSrc = GetLocalOrigin() + up * 36 + right * side * 16 + forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = forward * random->RandomFloat( -1, 1 ) + right * side * random->RandomFloat( 0, 1 ) + up * random->RandomFloat( -1, 1 );
		trace_t tr1;
		AI_TraceLine ( vecSrc, vecSrc + vecAim * 512, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr1);
		if (flDist > tr1.fraction)
		{
			tr = tr1;
			flDist = tr.fraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	UTIL_ImpactTrace( &tr, DMG_CLUB );

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
	m_iBeams++;
}

//------------------------------------------------------------------------------
// Purpose : Short beam on had in grenade defend mode
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::DefendBeams( void )
{
	Vector vBeamStart;

	// -----------
	// Left hand
	// -----------
	int i;
	GetAttachment( 1, vBeamStart );
	for (i=0;i<4;i++)
	{
		Vector vBeamPos = vBeamStart;
		vBeamPos.x += random->RandomFloat(-8,8);
		vBeamPos.y += random->RandomFloat(-8,8);
		vBeamPos.z += random->RandomFloat(-8,8);

		CBeam  *pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->PointEntInit( vBeamPos, this );
		pBeam->SetEndAttachment( 1 );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( 2 );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.1 );
	}

	// -----------
	// Right hand
	// -----------
	GetAttachment( 2, vBeamStart );
	for (i=4;i<VORTIGAUNT_MAX_BEAMS;i++)
	{
		Vector vBeamPos = vBeamStart;
		vBeamPos.x += random->RandomFloat(-8,8);
		vBeamPos.y += random->RandomFloat(-8,8);
		vBeamPos.z += random->RandomFloat(-8,8);

		CBeam  *pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->PointEntInit( vBeamPos, this );
		pBeam->SetEndAttachment( 2 );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( 2 );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.1 );
	}
}

//------------------------------------------------------------------------------
// Purpose : Glow for when vortigaunt punches manhack
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClawBeam( CBaseEntity *pHurt, int nNoise, int iAttachment )
{
	for (int i=0;i<2;i++)
	{
		CBeam* pBeam = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
		pBeam->EntsInit( this, pHurt );
		pBeam->SetStartAttachment( iAttachment);
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( nNoise );
		pBeam->SetColor( 96, 128, 16 );
		pBeam->LiveForTime( 0.2 );
	}
	EmitSound( "NPC_Vortigaunt.ClawBeam" );
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
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/blueglow1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 3 );
		}
		else
		{
			m_pLeftHandGlow = CSprite::SpriteCreate( "sprites/greenglow1.vmt", GetLocalOrigin(), FALSE );
			m_pLeftHandGlow->SetAttachment( this, 3 );
		}
		
		m_pLeftHandGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pLeftHandGlow->SetBrightness( 0 );
		m_pLeftHandGlow->SetScale( 0 );
	}
	
	if ( beamType != VORTIGAUNT_BEAM_HEAL ) 
	{
		if ( m_pRightHandGlow == NULL )
		{	
			m_pRightHandGlow = CSprite::SpriteCreate( "sprites/greenglow1.vmt", GetLocalOrigin(), FALSE );
			m_pRightHandGlow->SetAttachment( this, 4 );

			m_pRightHandGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
			m_pRightHandGlow->SetBrightness( 0 );
			m_pRightHandGlow->SetScale( 0 );
		}
	}
}


//------------------------------------------------------------------------------
// Purpose : Fade glow from hands
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::EndHandGlow( void )
{
	m_bGlowTurningOn	= false;
	m_fGlowChangeTime = VORTIGAUNT_GLOWFADE_TIME;

	if ( m_pLeftHandGlow )
	{
		m_pLeftHandGlow->SetScale( 0, 0.5f );
		m_pLeftHandGlow->FadeAndDie( 0.5f );
	}

	if ( m_pRightHandGlow )
	{
		m_pRightHandGlow->SetScale( 0, 0.5f );
		m_pRightHandGlow->FadeAndDie( 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: vortigaunt shoots at noisy body target (fixes problems in cover, etc)
//			NOTE: His weapon doesn't have any error
// Input  :
// Output :
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

//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CNPC_Vortigaunt::BeamGlow( )
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


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CNPC_Vortigaunt::WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 3.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->WorldSpaceCenter(), this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 12 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CNPC_Vortigaunt::ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	trace_t tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	vecSrc = GetLocalOrigin() + up * 36;
	vecAim = GetShootEnemyDir( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * right * random->RandomFloat( 0, deflection ) + up * random->RandomFloat( -deflection, deflection );

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

				AI_TraceLine ( vecSrc, vOrigin, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
			}

			CRagdollBoogie::Create( pTest, 200, gpGlobals->curtime, 1.0f );
		}
	}
	else
	{
		AI_TraceLine ( vecSrc, vecSrc + vecAim * 1024, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
	}

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.endpos, this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 3 );
	m_iBeams++;

	pEntity = tr.m_pEnt;
	if (pEntity != NULL && m_takedamage)
	{
		CTakeDamageInfo dmgInfo( this, this, sk_vortigaunt_dmg_zap.GetFloat(), DMG_SHOCK );
		dmgInfo.SetDamagePosition( tr.endpos );
		VectorNormalize( vecAim );// not a unit vec yet
		// hit like a 5kg object flying 400 ft/s
		dmgInfo.SetDamageForce( 5 * 400 * 12 * vecAim );
		pEntity->DispatchTraceAttack( dmgInfo, vecAim, &tr );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::HealBeam( int side )
{
	if (m_iBeams >= VORTIGAUNT_MAX_BEAMS)
		return;

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	trace_t tr;
	Vector vecSrc		= GetLocalOrigin() + up * 36;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.vmt", 5.0 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->EntsInit( GetTarget(), this );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? m_iLeftHandAttachment : m_iRightHandAttachment );
	m_pBeam[m_iBeams]->SetColor( 0, 0, 255 );

	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 6.5 );
	m_iBeams++;
}

//------------------------------------------------------------------------------
// Purpose : Clear Glow
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_Vortigaunt::ClearHandGlow( )
{
	if (m_pLeftHandGlow)
	{
		UTIL_Remove( m_pLeftHandGlow );
		m_pLeftHandGlow = NULL;
	}
	if (m_pRightHandGlow)
	{
		UTIL_Remove( m_pRightHandGlow );
		m_pRightHandGlow = NULL;
	}
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CNPC_Vortigaunt::ClearBeams( )
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
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputEnableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CNPC_Vortigaunt::InputDisableArmorRecharge( inputdata_t &data )
{
	m_bArmorRechargeEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
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
// Input  : &data - 
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


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_vortigaunt, CNPC_Vortigaunt )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_TASK(TASK_VORTIGAUNT_HEAL_WARMUP)
	DECLARE_TASK(TASK_VORTIGAUNT_HEAL)
	DECLARE_TASK(TASK_VORTIGAUNT_FACE_STOMP)
	DECLARE_TASK(TASK_VORTIGAUNT_STOMP_ATTACK)
	DECLARE_TASK(TASK_VORTIGAUNT_GRENADE_KILL)
	DECLARE_TASK(TASK_VORTIGAUNT_ZAP_GRENADE_OWNER)
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
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_STOMP)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_DEFEND)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_TO_DEFEND)
	DECLARE_ACTIVITY(ACT_VORTIGAUNT_FROM_DEFEND)

	DECLARE_CONDITION(COND_VORTIGAUNT_CAN_HEAL)

	DECLARE_INTERACTION( g_interactionVortigauntStomp )
	DECLARE_INTERACTION( g_interactionVortigauntStompFail )
	DECLARE_INTERACTION( g_interactionVortigauntStompHit )
	DECLARE_INTERACTION( g_interactionVortigauntKick )
	DECLARE_INTERACTION( g_interactionVortigauntClaw )

	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_LEFT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_CLAW_RIGHT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_POWERUP )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_SHOOT )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_ZAP_DONE )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTGLOW )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTBEAMS )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_KICK )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_STOMP )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_HEAL_STARTSOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SWING_SOUND )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_SHOOT_SOUNDSTART )
	DECLARE_ANIMEVENT( AE_VORTIGAUNT_DEFEND_BEAMS )

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
	// > SCHED_VORTIGAUNT_MELEE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_MELEE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY						0"
		"		TASK_MELEE_ATTACK1					0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_STOMP_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_STOMP_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_VORTIGAUNT_FACE_STOMP			0"
		"		TASK_VORTIGAUNT_STOMP_ATTACK		0"
		""
		"	Interrupts"
				// New_Enemy	Don't interrupt, finish current attack first
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_GRENADE_KILL
	//
	// Zap an incoming grenade (GetTarget())
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_GRENADE_KILL,

		"	Tasks"
		"		TASK_VORTIGAUNT_GRENADE_KILL		0"
		"		TASK_WAIT							0.3"
		"		TASK_VORTIGAUNT_ZAP_GRENADE_OWNER	0"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
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
		"		TASK_SPEAK_SENTENCE				0"					// VORT_HEAL_SENTENCE
		"		TASK_VORTIGAUNT_HEAL_WARMUP		0"
		"		TASK_VORTIGAUNT_HEAL			0"
		"		TASK_SPEAK_SENTENCE				1"					// VORT_DONE_HEAL_SENTENCE
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
		"		TASK_TALKER_HEADRESET				0"					// reset head position"
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
	// > SCHED_VORTIGAUNT_BARNACLE_HIT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_HIT,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_BARNACLE_PULL"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_PULL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_PULL,

		"	Tasks"
		"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_CHOMP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_CHOMP,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_VORTIGAUNT_BARNACLE_CHEW"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_VORTIGAUNT_BARNACLE_CHEW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_VORTIGAUNT_BARNACLE_CHEW,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
	)

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
		"		TASK_SPEAK_SENTENCE					2"					// Start extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_FACE_TARGET					0"
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_VORTIGAUNT_EXTRACT_WARMUP		0"
		"		TASK_VORTIGAUNT_EXTRACT				0"
		"		TASK_VORTIGAUNT_EXTRACT_COOLDOWN	0"
		"		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT	0"
		"		TASK_SPEAK_SENTENCE					3"					// Finish extracting sentence
		"		TASK_WAIT_FOR_SPEAK_FINISH			1"
		"		TASK_WAIT							2"
		""
		"	Interrupts"
	)
	
AI_END_CUSTOM_NPC()
