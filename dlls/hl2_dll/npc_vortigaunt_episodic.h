//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the base version of the vortigaunt
//
//=============================================================================//

#ifndef NPC_VORTIGAUNT_H
#define NPC_VORTIGAUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_behavior.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"

#define		VORTIGAUNT_MAX_BEAMS				8

class CBeam;
class CSprite;

enum VortigauntHealState_t
{
	HEAL_STATE_NONE,
	HEAL_STATE_WARMUP,
	HEAL_STATE_HEALING,
	HEAL_STATE_COOLDOWN,
};

//=========================================================
//	>> CNPC_Vortigaunt
//=========================================================
class CNPC_Vortigaunt : public CNPC_PlayerCompanion
{
	DECLARE_CLASS( CNPC_Vortigaunt, CNPC_PlayerCompanion );

public:
	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual float	MaxYawSpeed( void );

	virtual void	PrescheduleThink( void );
	virtual void	BuildScheduleTestBits( void );

	virtual int		RangeAttack1Conditions( float flDot, float flDist );

	virtual void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void		AlertSound( void );
	virtual Class_T		Classify ( void ) { return CLASS_VORTIGAUNT; }
	virtual void		HandleAnimEvent( animevent_t *pEvent );
	virtual Activity	NPC_TranslateActivity( Activity eNewActivity );

	virtual void	UpdateOnRemove( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual	void	GatherConditions( void );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	StartTask( const Task_t *pTask );

	// Navigation/Locomotion
	virtual bool	OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	
	virtual void	DeclineFollowing( void );
	virtual bool	CanBeUsedAsAFriend( void );
	virtual bool	IsPlayerAlly( void ) { return true; }

	// Override these to set behavior
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );
	virtual bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool			IsLeading( void ) { return ( GetRunningBehavior() == &m_LeadBehavior && m_LeadBehavior.HasGoal() ); }

	void			DeathSound( const CTakeDamageInfo &info );
	void			PainSound( const CTakeDamageInfo &info );
	
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual void	SpeakSentence( int sentType );

	virtual int		IRelationPriority( CBaseEntity *pTarget );
	virtual bool	IsReadinessCapable( void ) { return true; }
	virtual float	GetReadinessDecay() { return 30.0f; }
	virtual bool	ShouldRegenerateHealth( void ) { return m_bRegenerateHealth; }

	void	InputEnableArmorRecharge( inputdata_t &data );
	void	InputDisableArmorRecharge( inputdata_t &data );
	void	InputExtractBugbait( inputdata_t &data );
	void	InputChargeTarget( inputdata_t &data );

	// Health regeneration
	void	InputEnableHealthRegeneration( inputdata_t &data );
	void	InputDisableHealthRegeneration( inputdata_t &data );

private:

	bool	HealGestureHasLOS( void );
	bool	PlayerBelowHealthPercentage( float flPerc );
	void	StartHealing( void );
	void	StopHealing( bool bInterrupt = false );
	void	MaintainHealSchedule( void );
	bool	ShouldHealTarget( CBaseEntity *pTarget, bool bResetTimer );
	int		SelectHealSchedule( void );

	Vector	GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy = true );
	void	CreateBeamBlast( const Vector &vecOrigin );

private:
	//=========================================================
	// Vortigaunt schedules
	//=========================================================
	enum
	{
		SCHED_VORTIGAUNT_STAND = BaseClass::NEXT_SCHEDULE,
		SCHED_VORTIGAUNT_RANGE_ATTACK,
		SCHED_VORTIGAUNT_HEAL,
		SCHED_VORTIGAUNT_EXTRACT_BUGBAIT,
		SCHED_VORTIGAUNT_FACE_PLAYER,
		SCHED_VORTIGAUNT_RUN_TO_PLAYER,
	};

	//=========================================================
	// Vortigaunt Tasks 
	//=========================================================
	enum 
	{
		TASK_VORTIGAUNT_HEAL_WARMUP = BaseClass::NEXT_TASK,
		TASK_VORTIGAUNT_HEAL,
		TASK_VORTIGAUNT_EXTRACT_WARMUP,
		TASK_VORTIGAUNT_EXTRACT,
		TASK_VORTIGAUNT_EXTRACT_COOLDOWN,
		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT,
		TASK_VORTIGAUNT_WAIT_FOR_PLAYER,
		TASK_VORTIGAUNT_GET_HEAL_TARGET,
	};

	//=========================================================
	// Vortigaunt Conditions
	//=========================================================
	enum
	{
		COND_VORTIGAUNT_CAN_HEAL = BaseClass::NEXT_CONDITION,
		COND_VORTIGAUNT_HEAL_TARGET_TOO_FAR,	// Outside or heal range
		COND_VORTIGAUNT_HEAL_TARGET_BLOCKED,	// Blocked by an obstruction
		COND_VORTIGAUNT_HEAL_TARGET_BEHIND_US,	// Not within our "forward" range
		COND_VORTIGAUNT_HEAL_VALID,				// All conditions satisfied	
	};

	// ------------
	// Beams
	// ------------
	void			ClearBeams( void );
	void			ArmBeam( int side , int beamType);
	void			ZapBeam( int side );
	void			BeamGlow( void );
	CHandle<CBeam>	m_pBeam[VORTIGAUNT_MAX_BEAMS];
	int				m_iBeams;
	int				m_nLightningSprite;
	bool			m_bHealBeamActive;

	// ---------------
	//  Glow
	// ----------------
	void			ClearHandGlow( void );
	float			m_fGlowAge;
	float			m_fGlowScale;
	float			m_fGlowChangeTime;
	bool			m_bGlowTurningOn;
	int				m_nCurGlowIndex;
	
	CHandle<CSprite>	m_pLeftHandGlow;
	CHandle<CSprite>	m_pRightHandGlow;
	
	void			StartHandGlow( int beamType );
	void			EndHandGlow( void );

	// ----------------
	//  Healing
	// ----------------
	bool				m_bRegenerateHealth;
	float				m_flNextHealTime;		// Next time allowed to heal player
	int					m_nCurrentHealAmt;		// How much healed this session
	int					m_nLastArmorAmt;		// Player armor at end of last heal
	int					m_iSuitSound;			// 0 = off, 1 = startup, 2 = going
	float				m_flSuitSoundTime;
	EHANDLE				m_hHealTarget;			// The person that I'm going to heal.
	
	VortigauntHealState_t	m_eHealState;
	
	void			GatherHealConditions( void );
	void			HealBeam( int side );

	float			m_flHealHinderedTime;
	float			m_flPainTime;
	float			m_nextLineFireTime;
	bool			m_bArmorRechargeEnabled;
	bool			m_bForceArmorRecharge;
	int				m_iCurrentRechargeGoal;

	bool			m_bExtractingBugbait;

	COutputEvent	m_OnFinishedExtractingBugbait;
	COutputEvent	m_OnFinishedChargingTarget;
	COutputEvent	m_OnPlayerUse;

	CAI_AssaultBehavior			m_AssaultBehavior;
	CAI_LeadBehavior			m_LeadBehavior;
	
	//Adrian: Let's do it the right way!
	int m_iLeftHandAttachment;
	int m_iRightHandAttachment;
	bool				m_bStopLoopingSounds;

public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

#endif // NPC_VORTIGAUNT_H
