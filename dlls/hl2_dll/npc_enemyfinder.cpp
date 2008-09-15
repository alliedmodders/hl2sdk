//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Bullseyes act as targets for other NPC's to attack and to trigger
//			events 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "decals.h"
#include "IEffects.h"
#include "ai_squad.h"
#include "ai_utils.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_ENEMY_FINDER_CHECK_VIS (1 << 16)
#define SF_ENEMY_FINDER_APC_VIS (1 << 17)
#define SF_ENEMY_FINDER_SHORT_MEMORY (1 << 18)
#define SF_ENEMY_FINDER_ENEMY_ALLOWED (1 << 19)

ConVar  ai_debug_enemyfinders( "ai_debug_enemyfinders", "0" );


class CNPC_EnemyFinder : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CNPC_EnemyFinder, CAI_BaseNPC );

	CNPC_EnemyFinder()
	{
		m_PlayerFreePass.SetOuter( this );
	}


	void	Precache( void );
	void	Spawn( void );
	void	StartNPC ( void );
	void	PrescheduleThink();
	bool 	ShouldAlwaysThink();
	void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	void	GatherConditions( void );
	bool	ShouldChooseNewEnemy();
	bool	IsValidEnemy( CBaseEntity *pTarget );
	bool	CanBeAnEnemyOf( CBaseEntity *pEnemy ) { return HasSpawnFlags( SF_ENEMY_FINDER_ENEMY_ALLOWED ); }
	bool	FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker );
	Class_T Classify( void );
	bool CanBeSeenBy( CAI_BaseNPC *pNPC ) { return CanBeAnEnemyOf( pNPC ); } // allows entities to be 'invisible' to NPC senses.

	virtual int	SelectSchedule( void );
	virtual void DrawDebugGeometryOverlays( void );

	// Input handlers.
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );

	virtual	void Wake( bool bFireOutput = true );

private:
	int		m_nStartOn;
	float	m_flMinSearchDist;
	float	m_flMaxSearchDist;
	CAI_FreePass m_PlayerFreePass;
	CSimpleSimTimer m_ChooseEnemyTimer;

	bool	m_bEnemyStatus;

	COutputEvent m_OnLostEnemies;
	COutputEvent m_OnAcquireEnemies;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_enemyfinder, CNPC_EnemyFinder );


//-----------------------------------------------------------------------------
// Custom schedules.
//-----------------------------------------------------------------------------
enum
{
	SCHED_EFINDER_SEARCH = LAST_SHARED_SCHEDULE,
};

IMPLEMENT_CUSTOM_AI( npc_enemyfinder, CNPC_EnemyFinder );

BEGIN_DATADESC( CNPC_EnemyFinder )

	DEFINE_EMBEDDED( m_PlayerFreePass ),
	DEFINE_EMBEDDED( m_ChooseEnemyTimer ),

	// Inputs
	DEFINE_INPUT( m_nStartOn,			FIELD_INTEGER,	"StartOn" ),
	DEFINE_INPUT( m_flFieldOfView,	FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( m_flMinSearchDist,	FIELD_FLOAT,	"MinSearchDist" ),
	DEFINE_INPUT( m_flMaxSearchDist,	FIELD_FLOAT,	"MaxSearchDist" ),

	DEFINE_FIELD( m_bEnemyStatus, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

	DEFINE_OUTPUT( m_OnLostEnemies, "OnLostEnemies"),
	DEFINE_OUTPUT( m_OnAcquireEnemies, "OnAcquireEnemies"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InitCustomSchedules( void )
{
	INIT_CUSTOM_AI( CNPC_EnemyFinder );

	ADD_CUSTOM_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
	AI_LOAD_SCHEDULE( CNPC_EnemyFinder, SCHED_EFINDER_SEARCH );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder on.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOn( inputdata_t &inputdata )
{
	SetThink( &CNPC_EnemyFinder::CallNPCThink );
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning the enemy finder off.
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::InputTurnOff( inputdata_t &inputdata )
{
	SetThink(NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Precache( void )
{
	PrecacheModel( "models/roller.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::Spawn( void )
{
	Precache();

	SetModel( "models/roller.mdl" );
	// This is a dummy model that is never used!
	UTIL_SetSize(this, vec3_origin, vec3_origin);

	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( DONT_BLEED );
	SetGravity( 0.0 );
	m_iHealth			= 1;
	
	AddFlag( FL_NPC );

	SetSolid( SOLID_NONE );

	m_bEnemyStatus = false;

	if (m_flFieldOfView < -1.0)
	{
		DevMsg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 0.5;
	}
	else if (m_flFieldOfView > 1.0)
	{
		DevMsg("ERROR: EnemyFinder field of view must be between -1.0 and 1.0\n");
		m_flFieldOfView		= 1.0;
	}
	CapabilitiesAdd	( bits_CAP_SQUAD );

	NPCInit();

	// Set this after NPCInit()
	m_takedamage	= DAMAGE_NO;
	AddEffects( EF_NODRAW );
	m_NPCState		= NPC_STATE_ALERT;	// always alert

	SetViewOffset( vec3_origin );
	if ( m_flMaxSearchDist )
	{
		SetDistLook( m_flMaxSearchDist );
	}

	if ( HasSpawnFlags( SF_ENEMY_FINDER_SHORT_MEMORY ) )
	{
		GetEnemies()->SetEnemyDiscardTime( 0.2 );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
int CNPC_EnemyFinder::SelectSchedule( void )
{
	return SCHED_EFINDER_SEARCH;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::Wake( bool bFireOutput )
{
	BaseClass::Wake( bFireOutput );

	//Enemy finder is not allowed to become visible.
	AddEffects( EF_NODRAW );
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::FVisible( CBaseEntity *pTarget, int traceMask, CBaseEntity **ppBlocker )
{
	float flTargetDist = GetAbsOrigin().DistTo( pTarget->GetAbsOrigin() );
	if ( flTargetDist < m_flMinSearchDist)
		return false;

	if ( m_flMaxSearchDist && flTargetDist > m_flMaxSearchDist)
		return false;

	if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
		return true;

	if ( !HasSpawnFlags(SF_ENEMY_FINDER_APC_VIS) )
	{
		bool bIsVisible = BaseClass::FVisible( pTarget, traceMask, ppBlocker );
		
		if ( bIsVisible && pTarget == m_PlayerFreePass.GetPassTarget() )
			bIsVisible = m_PlayerFreePass.ShouldAllowFVisible( bIsVisible );

		return bIsVisible;
	}

	// Make sure I can see the target from my position
	trace_t tr;

	// Trace from launch position to target position.  
	// Use position above actual barral based on vertical launch speed
	Vector vStartPos = GetAbsOrigin();
	Vector vEndPos	 = pTarget->EyePosition();

	CBaseEntity *pVehicle = NULL;
	if ( pTarget->IsPlayer() )
	{
		CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pTarget);
		pVehicle = pPlayer->GetVehicleEntity();
	}

	CTraceFilterSkipTwoEntities traceFilter( pTarget, pVehicle, COLLISION_GROUP_NONE );
	AI_TraceLine( vStartPos, vEndPos, MASK_SHOT, &traceFilter, &tr );
	if ( ppBlocker )
	{
		*ppBlocker = tr.m_pEnt;
	}
	return (tr.fraction == 1.0);
}


//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::ShouldChooseNewEnemy()
{
	if ( m_ChooseEnemyTimer.Expired() )
	{
		m_ChooseEnemyTimer.Set( 0.3 );
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Override base class to check range and visibility
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::IsValidEnemy( CBaseEntity *pTarget )
{
	float flTargetDist = GetAbsOrigin().DistTo( pTarget->GetAbsOrigin() );
	if (flTargetDist < m_flMinSearchDist)
		return false;

	if ( m_flMaxSearchDist && flTargetDist > m_flMaxSearchDist)
		return false;

	if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
		return true;

	if ( GetSenses()->DidSeeEntity( pTarget ) )
		return true;

	// Trace from launch position to target position.  
	// Use position above actual barral based on vertical launch speed
	Vector vStartPos = GetAbsOrigin();
	Vector vEndPos	 = pTarget->EyePosition();

	// Test our line of sight to the target
	trace_t tr;
	AI_TraceLOS( vStartPos, vEndPos, this, &tr );

	// If the player is in a vehicle, see if we can see that instead
	if ( pTarget->IsPlayer() )
	{
		CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pTarget);
		if ( tr.m_pEnt == pPlayer->GetVehicleEntity() )
			return true;
	}

	// Line must be clear
	if ( tr.fraction == 1.0f || tr.m_pEnt == pTarget )
		return true;

	// Otherwise we can't see anything
	return false;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::StartNPC ( void )
{
	AddSpawnFlags(SF_NPC_FALL_TO_GROUND);	// this prevents CAI_BaseNPC from slamming the finder to 
											// the ground just because it's not MOVETYPE_FLY
	BaseClass::StartNPC();

	if ( AI_IsSinglePlayer() && m_PlayerFreePass.GetParams().duration > 0.1 )
	{
		m_PlayerFreePass.SetPassTarget( UTIL_PlayerByIndex(1) );

		AI_FreePassParams_t freePassParams = m_PlayerFreePass.GetParams();

		freePassParams.coverDist = 120;
		freePassParams.peekEyeDist = 1.75;
		freePassParams.peekEyeDistZ = 4;

		m_PlayerFreePass.SetParams( freePassParams );
	}

	if (!m_nStartOn)
	{
		SetThink(NULL);
	}
}

//------------------------------------------------------------------------------
void CNPC_EnemyFinder::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	bool bHasEnemies = GetEnemies()->NumEnemies() > 0;
	
	if ( GetEnemies()->NumEnemies() > 0 )
	{
		//If I haven't seen my enemy in half a second then we'll assume he's gone.
		if ( gpGlobals->curtime - GetEnemyLastTimeSeen() >= 0.5f )
		{
			bHasEnemies = false;
		}
	}

	if ( m_bEnemyStatus != bHasEnemies )
	{
		if ( bHasEnemies )
		{
			m_OnAcquireEnemies.FireOutput( this, this );
		}
		else
		{
			m_OnLostEnemies.FireOutput( this, this );
		}
		
		m_bEnemyStatus = bHasEnemies;
	}

	if( ai_debug_enemyfinders.GetBool() )
	{
		m_debugOverlays |= OVERLAY_BBOX_BIT;

		if( IsInSquad() && GetSquad()->NumMembers() > 1 )
		{
			AISquadIter_t iter;
			CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember( &iter ) : NULL;
			while ( pSquadmate )
			{
				NDebugOverlay::Line( WorldSpaceCenter(), pSquadmate->EyePosition(), 255, 255, 0, false, 0.1f );
				pSquadmate = m_pSquad->GetNextMember( &iter );
			}
		}
	}
}

//------------------------------------------------------------------------------
bool CNPC_EnemyFinder::ShouldAlwaysThink()
{
	if ( BaseClass::ShouldAlwaysThink() )
		return true;
		
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer && IRelationType( pPlayer ) == D_HT )
	{
		float playerDistSqr = GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );

		if ( !m_flMaxSearchDist || playerDistSqr <= Square(m_flMaxSearchDist) )
		{
			if ( !FBitSet( m_spawnflags, SF_ENEMY_FINDER_CHECK_VIS) )
				return true;
				
			if ( playerDistSqr <= Square( 50 * 12 ) )
				return true;
		}
	}
	
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_EnemyFinder::GatherConditions()
{
	// This works with old data because need to do before base class so as to not choose as enemy
	m_PlayerFreePass.Update();
	BaseClass::GatherConditions();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_EnemyFinder::Classify( void )
{
	if ( GetSquad() )
	{
		AISquadIter_t iter;
		CAI_BaseNPC *pSquadmate = GetSquad()->GetFirstMember( &iter );
		while ( pSquadmate )
		{
			if ( pSquadmate != this && !pSquadmate->ClassMatches( GetClassname() ) )
			{
				return pSquadmate->Classify();
			}
			pSquadmate = GetSquad()->GetNextMember( &iter );
		}
	}

	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Add a visualizer to the text, if turned on
//-----------------------------------------------------------------------------
void CNPC_EnemyFinder::DrawDebugGeometryOverlays( void )
{
	// Turn on npc_relationships if we're displaying text
	int oldDebugOverlays = m_debugOverlays;
	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		m_debugOverlays |= OVERLAY_NPC_RELATION_BIT;
	}

	// Draw our base overlays
	BaseClass::DrawDebugGeometryOverlays();

	// Restore the old values
	m_debugOverlays = oldDebugOverlays;
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_EFINDER_SEARCH
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_EFINDER_SEARCH,

	"	Tasks"
	"		TASK_WAIT_RANDOM			0.5		"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
);
