//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc_physicsflyer.h"
#include "soundenvelope.h"
#include "ai_hint.h"
#include "ai_moveprobe.h"
#include "ai_squad.h"
#include "beam_shared.h"
#include "explode.h"
#include "globalstate.h"
#include "soundent.h"
#include "npc_citizen17.h"
#include "gib.h"
#include "smoke_trail.h"
#include "spotlightend.h"
#include "IEffects.h"
#include "items.h"
#include "ai_route.h"
#include "player_pickup.h"
#include "weapon_physcannon.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Parameters for how the scanner relates to citizens.
//-----------------------------------------------------------------------------
#define SCANNER_CIT_INSPECT_DELAY		10		// Check for citizens this often
#define	SCANNER_CIT_INSPECT_GROUND_DIST	500		// How far to look for citizens to inspect
#define	SCANNER_CIT_INSPECT_FLY_DIST	1500	// How far to look for citizens to inspect

#define SCANNER_CIT_INSPECT_LENGTH		5		// How long does the inspection last
#define SCANNER_HINT_INSPECT_LENGTH		5		// How long does the inspection last
#define SCANNER_SOUND_INSPECT_LENGTH	5		// How long does the inspection last

#define SCANNER_HINT_INSPECT_DELAY		15		// Check for hint nodes this often
	
#define	SCANNER_BANK_RATE				30
#define	SCANNER_MAX_SPEED				250
#define	SCANNER_MAX_DIVE_BOMB_SPEED		2500
#define SCANNER_SQUAD_FLY_DIST			500		// How far to scanners stay apart
#define SCANNER_SQUAD_HELP_DIST			4000	// How far will I fly to help

#define	SPOTLIGHT_WIDTH					32

#define SCANNER_SPOTLIGHT_NEAR_DIST		64
#define SCANNER_SPOTLIGHT_FAR_DIST		256
#define SCANNER_SPOTLIGHT_FLY_HEIGHT	72
#define SCANNER_NOSPOTLIGHT_FLY_HEIGHT	72

#define SCANNER_FLASH_MIN_DIST			900		// How far does flash effect enemy
#define SCANNER_FLASH_MAX_DIST			1200	// How far does flash effect enemy

#ifdef _XBOX
#define	SCANNER_FLASH_MAX_VALUE			164		// XBox brightness
#else
#define	SCANNER_FLASH_MAX_VALUE			240		// How bright is maximum flash
#endif

#define SCANNER_PHOTO_NEAR_DIST			64
#define SCANNER_PHOTO_FAR_DIST			128

#define	SCANNER_FOLLOW_DIST				128

#define SCANNER_ATTACK_NEAR_DIST		150		// Fly attack min distance
#define SCANNER_ATTACK_FAR_DIST			300		// Fly attack max distance
#define SCANNER_ATTACK_RANGE			350		// Attack max distance
#define	SCANNER_ATTACK_MIN_DELAY		8		// Min time between attacks
#define	SCANNER_ATTACK_MAX_DELAY		12		// Max time between attacks
#define	SCANNER_EVADE_TIME				1		// How long to evade after take damage

#define	SCANNER_NUM_GIBS				6		// Number of gibs in gib file

// Sentences
#define SCANNER_SENTENCE_ATTENTION	0
#define SCANNER_SENTENCE_HANDSUP	1
#define SCANNER_SENTENCE_PROCEED	2
#define SCANNER_SENTENCE_CURIOUS	3

// Strider Scout Scanners
#define SCANNER_SCOUT_MAX_SPEED			150

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_CSCANNER_NO_DYNAMIC_LIGHT	(1 << 16)
#define SF_CSCANNER_STRIDER_SCOUT		(1 << 17)

ConVar	sk_scanner_health( "sk_scanner_health","0");
ConVar	sk_scanner_dmg_dive( "sk_scanner_dmg_dive","0");
ConVar	g_debug_cscanner( "g_debug_cscanner", "0" );

//-----------------------------------------------------------------------------
// Think contexts
//-----------------------------------------------------------------------------
static const char *s_pDiveBombSoundThinkContext = "DiveBombSoundThinkContext";


//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
static int ACT_SCANNER_SMALL_FLINCH_ALERT = 0;
static int ACT_SCANNER_SMALL_FLINCH_COMBAT = 0;
static int ACT_SCANNER_INSPECT = 0;
static int ACT_SCANNER_WALK_ALERT = 0;
static int ACT_SCANNER_WALK_COMBAT = 0;
static int ACT_SCANNER_FLARE = 0;
static int ACT_SCANNER_RETRACT = 0;
static int ACT_SCANNER_FLARE_PRONGS = 0;
static int ACT_SCANNER_RETRACT_PRONGS = 0;
static int ACT_SCANNER_FLARE_START = 0;

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionScannerInspect				= 0;
int	g_interactionScannerInspectBegin		= 0;
int g_interactionScannerInspectHandsUp		= 0;
int g_interactionScannerInspectShowArmband	= 0;//<<TEMP>>still to be completed
int	g_interactionScannerInspectDone			= 0;
int g_interactionScannerSupportEntity		= 0;
int g_interactionScannerSupportPosition		= 0;

//-----------------------------------------------------------------------------
// Animation events
//------------------------------------------------------------------------
int AE_SCANNER_CLOSED;

//-----------------------------------------------------------------------------
// Attachment points
//-----------------------------------------------------------------------------
#define SCANNER_ATTACHMENT_LIGHT	"light"
#define SCANNER_ATTACHMENT_FLASH	1
#define SCANNER_ATTACHMENT_LPRONG	2
#define SCANNER_ATTACHMENT_RPRONG	3

//-----------------------------------------------------------------------------
// Other defines.
//-----------------------------------------------------------------------------
#define SCANNER_MAX_BEAMS		4

//-----------------------------------------------------------------------------
// States for the scanner's sound.
//-----------------------------------------------------------------------------
enum ScannerFlyMode_t
{
	SCANNER_FLY_PHOTO = 0,		// Fly close to photograph entity
	SCANNER_FLY_PATROL,			// Fly slowly around the enviroment
	SCANNER_FLY_FAST,			// Fly quickly around the enviroment
	SCANNER_FLY_CHASE,			// Fly quickly around the enviroment
	SCANNER_FLY_SPOT,			// Fly above enity in spotlight position
	SCANNER_FLY_ATTACK,			// Get in my enemies face for spray or flash
	SCANNER_FLY_DIVE,			// Divebomb - only done when dead
	SCANNER_FLY_FOLLOW,			// Following a target
};

enum ScannerInspectAct_t
{
	SCANNER_INSACT_HANDS_UP,
	SCANNER_INSACT_SHOWARMBAND,
};

class CBeam;
class CSprite;
class SmokeTrail;
class CSpotlightEnd;


class CNPC_CScanner : public CAI_BasePhysicsFlyingBot, public CDefaultPlayerPickupVPhysics
{
DECLARE_CLASS( CNPC_CScanner, CAI_BasePhysicsFlyingBot );

public:
	CNPC_CScanner();

	virtual void	UpdateEfficiency( bool bInPVS );

	Class_T			Classify( void ) { return(CLASS_SCANNER); }
	int				GetSoundInterests( void ) { return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER); }
	virtual float	GetAutoAimRadius() { return 12.0f; }

	void			GatherConditions( void );
	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int				OnTakeDamage_Dying( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void			Gib(void);

	void			OnStateChange( NPC_STATE eOldState, NPC_STATE eNewState );
	void			ClampMotorForces( Vector &linear, AngularImpulse &angular );

	int				DrawDebugTextOverlays(void);

	bool			FValidateHintType(CAI_Hint *pHint);
	float			GetHeadTurnRate( void );

	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	
	// 	CDefaultPlayerPickupVPhysics
	void			OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void			OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );

	virtual int		TranslateSchedule( int scheduleType );
	Disposition_t	IRelationType(CBaseEntity *pTarget);

	void			IdleSound( void );
	void			DeathSound( const CTakeDamageInfo &info );
	void			AlertSound( void );
	void			PainSound( const CTakeDamageInfo &info );

	void			NPCThink( void );
	
	void			PrescheduleThink( void );
	int				MeleeAttack1Conditions ( float flDot, float flDist );
	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	int				SelectSchedule(void);
	bool			ShouldPlayIdleSound( void );
	void			ScannerEmitSound( const char *pszSoundName );
	void			Spawn(void);
	void			Activate();
	void			StartTask( const Task_t *pTask );
	void			OnScheduleChange( void );
	void			UpdateOnRemove( void );
	void			DeployMine();
	float			GetMaxSpeed();

	void			PostRunStopMoving()	{} // scanner can use "movement" activities but not be moving

	Activity		NPC_TranslateActivity( Activity eNewActivity );

	void			HandleAnimEvent( animevent_t *pEvent );

	void			InputDisableSpotlight( inputdata_t &inputdata );
	void			InputInspectTargetPhoto( inputdata_t &inputdata );
	void			InputInspectTargetSpotlight( inputdata_t &inputdata );
	void			InputDeployMine( inputdata_t &inputdata );
	void			InputEquipMine( inputdata_t &inputdata );
	void			InputShouldInspect( inputdata_t &inputdata );
	void			InputSetFlightSpeed( inputdata_t &inputdata );
	void			InputSetFollowTarget( inputdata_t &inputdata );
	void			InputClearFollowTarget( inputdata_t &inputdata );
	void			InputSetDistanceOverride( inputdata_t &inputdata );

	void			InspectTarget( inputdata_t &inputdata, ScannerFlyMode_t eFlyMode );

	virtual bool	CanBecomeServerRagdoll( void ) { return false;	}

public:
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	void			SpeakSentence( int sentenceType );

	// ------------------------------
	//	Inspecting
	// ------------------------------
	Vector			m_vInspectPos;
	float			m_fInspectEndTime;
	float			m_fCheckCitizenTime;	// Time to look for citizens to harass
	float			m_fCheckHintTime;		// Time to look for hints to inspect
	bool			m_bShouldInspect;
	bool			m_bOnlyInspectPlayers;
	bool			m_bNeverInspectPlayers;

	void			SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration);
	void			SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration);
	void			SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration);
	void			ClearInspectTarget(void);
	bool			HaveInspectTarget(void);
	Vector			InspectTargetPosition(void);
	bool			IsValidInspectTarget(CBaseEntity *pEntity);
	CBaseEntity*	BestInspectTarget(void);
	void			RequestInspectSupport(void);

	bool			IsStriderScout() { return HasSpawnFlags( SF_CSCANNER_STRIDER_SCOUT ); }

	// ------------------------
	//  Death Cleanup
	// ------------------------
	CTakeDamageInfo m_KilledInfo;

	// ------------------------
	//  Photographing
	// ------------------------
	float			m_fNextPhotographTime;
	CSprite*		m_pEyeFlash;

	void			TakePhoto( void );
	void			BlindFlashTarget( CBaseEntity *pTarget );

	// ------------------------------
	//	Spotlight
	// ------------------------------
	Vector			m_vSpotlightTargetPos;
	Vector			m_vSpotlightCurrentPos;
	CHandle<CBeam>	m_hSpotlight;
	CHandle<CSpotlightEnd> m_hSpotlightTarget;
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightAngVelocity;
	float			m_flSpotlightCurLength;
	float			m_flSpotlightMaxLength;
	float			m_flSpotlightGoalWidth;
	float			m_fNextSpotlightTime;
	int				m_nHaloSprite;
	
	void			SpotlightUpdate(void);
	Vector			SpotlightTargetPos(void);
	Vector			SpotlightCurrentPos(void);
	void			SpotlightCreate(void);
	void			SpotlightDestroy(void);

protected:
	void			BecomeClawScanner( void ) { m_bIsClawScanner = true; }

private:
	bool	MovingToInspectTarget( void );
	bool	GetGoalDirection( Vector *vOut );
	bool	IsEnemyPlayerInSuit();
	float	GetGoalDistance( void );

	bool	IsHeldByPhyscannon( );
	void	StartSmokeTrail( void );
	Vector	IdealGoalForMovement( const Vector &goalPos, const Vector &startPos, float idealRange, float idealHeight );

	// Take damage from being thrown by a physcannon 
	void TakeDamageFromPhyscannon( CBasePlayer *pPlayer );

	// Take damage from physics impacts
	void TakeDamageFromPhysicsImpact( int index, gamevcollisionevent_t *pEvent );

	// Do we have a physics attacker?
	CBasePlayer *HasPhysicsAttacker( float dt );

	// Purpose:
	void BlendPhyscannonLaunchSpeed();

	virtual void		StopLoopingSounds(void);

	CSoundPatch			*m_pEngineSound;

	// Movement

	float				m_flFlyNoiseBase;
	float				m_flEngineStallTime;
	float				m_fNextFlySoundTime;
	ScannerFlyMode_t	m_nFlyMode;

	Vector m_vecDiveBombDirection;		// The direction we dive bomb. Calculated at the moment of death.
	float m_flDiveBombRollForce;		// Used for roll while dive bombing.

	// physics influence
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;

	float				m_flGoalOverrideDistance;

	// Pose parameters

	int m_nPoseTail;
	int m_nPoseDynamo;
	int m_nPoseFlare;
	int m_nPoseFaceVert;
	int m_nPoseFaceHoriz;

	bool m_bIsClawScanner;	// Formerly the shield scanner.
	bool m_bIsOpen;			// Only for claw scanner

	bool m_bHasSpoken;

	COutputEvent		m_OnPhotographPlayer;
	COutputEvent		m_OnPhotographNPC;

	bool				OverridePathMove( CBaseEntity *pMoveTarget, float flInterval );
	bool				OverrideMove(float flInterval);
	void				MoveToTarget(float flInterval, const Vector &MoveTarget);
	void				MoveToSpotlight(float flInterval);
	void				MoveToPhotograph(float flInterval);
	void				MoveToAttack(float flInterval);
	void				MoveToDivebomb(float flInterval);
	void				MoveExecute_Alive(float flInterval);
	float				MinGroundDist(void);
	Vector				VelocityToEvade(CBaseCombatCharacter *pEnemy);
	void				PlayFlySound(void);

	void				SetBanking( float flInterval );
	void				UpdateHead( float flInterval );
	inline CBaseEntity *EntityToWatch( void );

	void DiveBombSoundThink();

	// Attacks

	SmokeTrail			*m_pSmokeTrail;

	bool				m_bNoLight;
	bool				m_bPhotoTaken;

	void				AttackPreFlash(void);
	void				AttackFlash(void);
	void				AttackFlashBlind(void);
	void				AttackDivebomb(void);
	void				AttackDivebombCollide(float flInterval);

	DEFINE_CUSTOM_AI;

	// Custom interrupt conditions
	enum
	{
		COND_SCANNER_FLY_CLEAR = BaseClass::NEXT_CONDITION,
		COND_SCANNER_FLY_BLOCKED,							
		COND_SCANNER_HAVE_INSPECT_TARGET,							
		COND_SCANNER_INSPECT_DONE,							
		COND_SCANNER_CAN_PHOTOGRAPH,
		COND_SCANNER_SPOT_ON_TARGET,
		COND_SCANNER_GRABBED_BY_PHYSCANNON,
		COND_SCANNER_RELEASED_FROM_PHYSCANNON,
	};

	// Custom schedules
	enum
	{
		SCHED_SCANNER_PATROL = BaseClass::NEXT_SCHEDULE,
		SCHED_SCANNER_SPOTLIGHT_HOVER,
		SCHED_SCANNER_SPOTLIGHT_INSPECT_POS,
		SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT,
		SCHED_SCANNER_PHOTOGRAPH_HOVER,
		SCHED_SCANNER_PHOTOGRAPH,
		SCHED_SCANNER_ATTACK_HOVER,
		SCHED_SCANNER_ATTACK_FLASH,
		SCHED_SCANNER_ATTACK_DIVEBOMB,
		SCHED_SCANNER_CHASE_ENEMY,
		SCHED_SCANNER_CHASE_TARGET,
		SCHED_SCANNER_MOVE_TO_INSPECT,
		SCHED_SCANNER_FOLLOW_HOVER,
		SCHED_SCANNER_HELD_BY_PHYSCANNON,
	};

	// Custom tasks
	enum
	{
		TASK_SCANNER_SET_FLY_PHOTO = BaseClass::NEXT_TASK,
		TASK_SCANNER_SET_FLY_PATROL,
		TASK_SCANNER_SET_FLY_CHASE,
		TASK_SCANNER_SET_FLY_SPOT,
		TASK_SCANNER_SET_FLY_ATTACK,
		TASK_SCANNER_SET_FLY_DIVE,
		TASK_SCANNER_PHOTOGRAPH,
		TASK_SCANNER_ATTACK_PRE_FLASH,
		TASK_SCANNER_ATTACK_FLASH,
		TASK_SCANNER_SPOT_INSPECT_ON,
		TASK_SCANNER_SPOT_INSPECT_WAIT,
		TASK_SCANNER_SPOT_INSPECT_OFF,
		TASK_SCANNER_CLEAR_INSPECT_TARGET,
		TASK_SCANNER_GET_PATH_TO_INSPECT_TARGET,
	};

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CNPC_CScanner )

	DEFINE_SOUNDPATCH( m_pEngineSound ),

	DEFINE_EMBEDDED( m_KilledInfo ),
	DEFINE_FIELD( m_flGoalOverrideDistance,	FIELD_FLOAT ),
	DEFINE_FIELD( m_bPhotoTaken,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vInspectPos,			FIELD_VECTOR ),
	DEFINE_FIELD( m_fInspectEndTime,		FIELD_TIME ),
	DEFINE_FIELD( m_fCheckCitizenTime,		FIELD_TIME ),
	DEFINE_FIELD( m_fCheckHintTime,			FIELD_TIME ),
	DEFINE_KEYFIELD( m_bShouldInspect,		FIELD_BOOLEAN,	"ShouldInspect" ),
	DEFINE_KEYFIELD( m_bOnlyInspectPlayers, FIELD_BOOLEAN,  "OnlyInspectPlayers" ),
	DEFINE_KEYFIELD( m_bNeverInspectPlayers,FIELD_BOOLEAN,  "NeverInspectPlayers" ),
	DEFINE_FIELD( m_fNextPhotographTime,	FIELD_TIME ),
//	DEFINE_FIELD( m_pEyeFlash,				FIELD_CLASSPTR ),
	DEFINE_FIELD( m_vSpotlightTargetPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpotlightCurrentPos,	FIELD_POSITION_VECTOR ),
// don't save (recreated after restore/transition)
//	DEFINE_FIELD( m_hSpotlight,				FIELD_EHANDLE ),
//	DEFINE_FIELD( m_hSpotlightTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_vSpotlightDir,			FIELD_VECTOR ),
	DEFINE_FIELD( m_vSpotlightAngVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpotlightCurLength,	FIELD_FLOAT ),
	DEFINE_FIELD( m_fNextSpotlightTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nHaloSprite,			FIELD_INTEGER ),
	DEFINE_FIELD( m_fNextFlySoundTime,		FIELD_TIME ),
	DEFINE_FIELD( m_nFlyMode,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseTail,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseDynamo,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFlare,				FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFaceVert,			FIELD_INTEGER ),
	DEFINE_FIELD( m_nPoseFaceHoriz,			FIELD_INTEGER ),

	DEFINE_FIELD( m_bIsClawScanner,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsOpen,				FIELD_BOOLEAN ),

	// DEFINE_FIELD( m_bHasSpoken,			FIELD_BOOLEAN ),

	DEFINE_FIELD( m_pSmokeTrail,			FIELD_CLASSPTR ),
	DEFINE_FIELD( m_flFlyNoiseBase,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flEngineStallTime,		FIELD_TIME ),

	DEFINE_FIELD( m_vecDiveBombDirection,	FIELD_VECTOR ),
	DEFINE_FIELD( m_flDiveBombRollForce,	FIELD_FLOAT ),

	DEFINE_KEYFIELD( m_flSpotlightMaxLength,	FIELD_FLOAT,	"SpotlightLength"),
	DEFINE_KEYFIELD( m_flSpotlightGoalWidth,	FIELD_FLOAT,	"SpotlightWidth"),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_KEYFIELD( m_bNoLight, FIELD_BOOLEAN, "SpotlightDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisableSpotlight", InputDisableSpotlight ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InspectTargetPhoto", InputInspectTargetPhoto ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InspectTargetSpotlight", InputInspectTargetSpotlight ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "InputShouldInspect", InputShouldInspect ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFollowTarget", InputSetFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearFollowTarget", InputClearFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDistanceOverride", InputSetDistanceOverride ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFlightSpeed", InputSetFlightSpeed ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DeployMine", InputDeployMine ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EquipMine", InputEquipMine ),

	DEFINE_OUTPUT( m_OnPhotographPlayer, "OnPhotographPlayer" ),
	DEFINE_OUTPUT( m_OnPhotographNPC, "OnPhotographNPC" ),

	DEFINE_THINKFUNC( DiveBombSoundThink ),

END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_cscanner, CNPC_CScanner);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_CScanner::CNPC_CScanner()
{
#ifdef _DEBUG
	m_vInspectPos.Init();
	m_vSpotlightTargetPos.Init();
	m_vSpotlightCurrentPos.Init();
	m_vSpotlightDir.Init();
	m_vSpotlightAngVelocity.Init();
	m_vCurrentBanking.Init();
#endif
	m_bShouldInspect = true;
	m_bOnlyInspectPlayers = false;
	m_bNeverInspectPlayers = false;
	m_pEngineSound = NULL;
	m_bHasSpoken = false;

	char szMapName[256];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName) );
	Q_strlower(szMapName);

	if( !Q_strnicmp( szMapName, "d3_c17", 6 ) )
	{
		// Streetwar scanners are claw scanners
		m_bIsClawScanner = true;
	}
	else
	{
		m_bIsClawScanner = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Spawn(void)
{
#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
	AddEffects( EF_NOSHADOW );
#endif // _XBOX

	// Check for user error
	if (m_flSpotlightMaxLength <= 0)
	{
		DevMsg("CNPC_CScanner::Spawn: Invalid spotlight length <= 0, setting to 500\n");
		m_flSpotlightMaxLength = 500;
	}
	
	if (m_flSpotlightGoalWidth <= 0)
	{
		DevMsg("CNPC_CScanner::Spawn: Invalid spotlight width <= 0, setting to 100\n");
		m_flSpotlightGoalWidth = 100;
	}

	if (m_flSpotlightGoalWidth > MAX_BEAM_WIDTH )
	{
		DevMsg("CNPC_CScanner::Spawn: Invalid spotlight width %.1f (max %.1f).\n", m_flSpotlightGoalWidth, MAX_BEAM_WIDTH );
		m_flSpotlightGoalWidth = MAX_BEAM_WIDTH; 
	}

	Precache();

	if( m_bIsClawScanner )
	{
		SetModel( "models/shield_scanner.mdl");
	}
	else
	{
		SetModel( "models/combine_scanner.mdl");
	}

	SetHullType( HULL_TINY_CENTERED );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_VPHYSICS );

	m_bloodColor		= DONT_BLEED;
	m_iHealth			= sk_scanner_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;
	
	SetNavType( NAV_FLY );
	
	AddFlag( FL_FLY );

	// This entity cannot be dissolved by the combine balls,
	// nor does it get killed by the mega physcannon.
	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL );

	m_flGoalOverrideDistance = 0.0f;

	// ------------------------------------
	//	Init all class vars 
	// ------------------------------------
	m_vInspectPos			= vec3_origin;
	m_fInspectEndTime		= 0;
	m_fCheckCitizenTime		= gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
	m_fCheckHintTime		= gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;
	m_fNextPhotographTime	= 0;

	m_vSpotlightTargetPos	= vec3_origin;
	m_vSpotlightCurrentPos	= vec3_origin;

	m_hSpotlight			= NULL;
	m_hSpotlightTarget		= NULL;

	AngleVectors( GetLocalAngles(), &m_vSpotlightDir );
	m_vSpotlightAngVelocity = vec3_origin;

	m_pEyeFlash				= 0;
	m_fNextSpotlightTime	= 0;
	m_fNextFlySoundTime		= 0;
	m_nFlyMode				= SCANNER_FLY_PATROL;
	m_vCurrentBanking		= m_vSpotlightDir;
	m_fHeadYaw				= 0;
	m_pSmokeTrail			= NULL;
	m_flSpotlightCurLength	= m_flSpotlightMaxLength;

	SetCurrentVelocity( vec3_origin );

	Vector	bobAmount;
	
	bobAmount.x = random->RandomFloat( -2.0f, 2.0f );
	bobAmount.y = random->RandomFloat( -2.0f, 2.0f );
	
	bobAmount.z = random->RandomFloat( 2.0f, 4.0f );

	if ( random->RandomInt( 0, 1 ) )
	{
		bobAmount.z *= -1.0f;
	}
	
	SetNoiseMod( bobAmount );

	// set flight speed
	m_flSpeed = GetMaxSpeed();

	m_nPoseTail = LookupPoseParameter( "tail_control" );
	m_nPoseDynamo = LookupPoseParameter( "dynamo_wheel" );
	m_nPoseFlare = LookupPoseParameter( "alert_control" );
	m_nPoseFaceVert = LookupPoseParameter( "flex_vert" );
	m_nPoseFaceHoriz = LookupPoseParameter( "flex_horz" );

	// --------------------------------------------

	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_SKIP_NAV_GROUND_CHECK );
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );

	m_bPhotoTaken = false;

	NPCInit();

	// Watch for this error state
	if ( m_bOnlyInspectPlayers && m_bNeverInspectPlayers )
	{
		Assert( 0 );
		Warning( "ERROR: Scanner set to never and always inspect players!\n" );
	}

	m_flFlyNoiseBase = random->RandomFloat( 0, M_PI );

	m_flNextAttack = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNPC_CScanner::Activate()
{
	BaseClass::Activate();

	// Have to do this here because sprites do not go across level transitions
	m_pEyeFlash = CSprite::SpriteCreate( "sprites/blueflare1.vmt", GetLocalOrigin(), FALSE );
	m_pEyeFlash->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEyeFlash->SetAttachment( this, LookupAttachment( SCANNER_ATTACHMENT_LIGHT ) );
	m_pEyeFlash->SetBrightness( 0 );
	m_pEyeFlash->SetScale( 1.4 );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNPC_CScanner::UpdateEfficiency( bool bInPVS )	
{
	SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); 
	SetMoveEfficiency( AIME_NORMAL ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : eOldState - 
//			eNewState - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::OnStateChange( NPC_STATE eOldState, NPC_STATE eNewState )
{
	if (( eNewState == NPC_STATE_ALERT ) || ( eNewState == NPC_STATE_COMBAT ))
	{
		SetPoseParameter(m_nPoseFlare, 1.0f);
	}
	else
	{
		SetPoseParameter(m_nPoseFlare, 0);
	}
}


//------------------------------------------------------------------------------
// Purpose: Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_CScanner::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// Turn off my spotlight when shot
	SpotlightDestroy();
	m_fNextSpotlightTime = gpGlobals->curtime + 2.0f;

	// Start smoking when we're nearly dead
	if ( m_iHealth < ( m_iMaxHealth - ( m_iMaxHealth / 4 ) ) )
	{
		StartSmokeTrail();
	}

	return (BaseClass::OnTakeDamage_Alive( info ));
}


//------------------------------------------------------------------------------
// Purpose: Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_CScanner::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	// do the damage
	m_iHealth -= info.GetDamage();
	
	if ( m_iHealth < -40 )
	{
		Gib();
		return 1;
	}

	return VPhysicsTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}
	
	BaseClass::TraceAttack( info, vecDir, ptr );
}


//-----------------------------------------------------------------------------
// Take damage from being thrown by a physcannon 
//-----------------------------------------------------------------------------
#define SCANNER_SMASH_SPEED 250.0	// How fast a scanner must slam into something to take full damage
void CNPC_CScanner::TakeDamageFromPhyscannon( CBasePlayer *pPlayer )
{
	CTakeDamageInfo info;
	info.SetDamageType( DMG_GENERIC );
	info.SetInflictor( this );
	info.SetAttacker( pPlayer );
	info.SetDamagePosition( GetAbsOrigin() );
	info.SetDamageForce( Vector( 1.0, 1.0, 1.0 ) );

	// Convert velocity into damage.
	Vector vel;
	VPhysicsGetObject()->GetVelocity( &vel, NULL );
	float flSpeed = vel.Length();

	float flFactor = flSpeed / SCANNER_SMASH_SPEED;

	// Clamp. Don't inflict negative damage or massive damage!
	flFactor = clamp( flFactor, 0.0f, 2.0f );
	float flDamage = m_iMaxHealth * flFactor;

#if 0
	Msg("Doing %f damage for %f speed!\n", flDamage, flSpeed );
#endif

	info.SetDamage( flDamage );
	TakeDamage( info );
}


//-----------------------------------------------------------------------------
// Take damage from physics impacts
//-----------------------------------------------------------------------------
void CNPC_CScanner::TakeDamageFromPhysicsImpact( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pHitEntity = pEvent->pEntities[!index];

	// NOTE: Augment the normal impact energy scale here.
	float flDamageScale = PlayerHasMegaPhysCannon() ? 10.0f : 5.0f;

	// Scale by the mapmaker's energyscale
	flDamageScale *= m_impactEnergyScale;

	int damageType = 0;
	float damage = CalculateDefaultPhysicsDamage( index, pEvent, flDamageScale, true, damageType );
	if ( damage == 0 )
		return;

	Vector damagePos;
	pEvent->pInternalData->GetContactPoint( damagePos );
	Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
	if ( damageForce == vec3_origin )
	{
		// This can happen if this entity is motion disabled, and can't move.
		// Use the velocity of the entity that hit us instead.
		damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
	}

	// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
	PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
}


//-----------------------------------------------------------------------------
// Is the scanner being held?
//-----------------------------------------------------------------------------
bool CNPC_CScanner::IsHeldByPhyscannon( )
{
	return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD);
}

	
//------------------------------------------------------------------------------
// Physics impact
//------------------------------------------------------------------------------
#define SCANNER_SMASH_TIME	0.75		// How long after being thrown from a physcannon that a manhack is eligible to die from impact
void CNPC_CScanner::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	// Take no impact damage while being carried.
	if ( IsHeldByPhyscannon( ) )
		return;

	CBasePlayer *pPlayer = HasPhysicsAttacker( SCANNER_SMASH_TIME );
	if( pPlayer )
	{
		TakeDamageFromPhyscannon( pPlayer );
		return;
	}

	// It also can take physics damage from things thrown by the player.
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( pHitEntity )
	{
		if ( pHitEntity->HasPhysicsAttacker( 0.5f ) )
		{
			// It can take physics damage from things thrown by the player.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
		else if ( FClassnameIs( pHitEntity, "prop_combine_ball" ) )
		{
			// It also can take physics damage from a combine ball.
			TakeDamageFromPhysicsImpact( index, pEvent );
		}
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::Gib( void )
{
	if ( IsMarkedForDeletion() )
		return;

	// Sparks
	for ( int i = 0; i < 4; i++ )
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}
	
	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0, &WorldSpaceCenter(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Spawn all gibs
	if( m_bIsClawScanner )
	{
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib1.mdl");
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib2.mdl");
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib3.mdl");
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib4.mdl");
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib5.mdl");
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/Shield_Scanner_Gib6.mdl");
	}
	else
	{
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/scanner_gib01.mdl" );
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/scanner_gib02.mdl" );
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/scanner_gib04.mdl" );
		CGib::SpawnSpecificGibs( this, 1, 500, 250, "models/gibs/scanner_gib05.mdl" );
	}

	// Cover the gib spawn
	ExplosionCreate( WorldSpaceCenter(), GetAbsAngles(), this, 64, 64, false );

	// Turn off any smoke trail
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_ParticleLifetime = 0;
		UTIL_Remove(m_pSmokeTrail);
		m_pSmokeTrail = NULL;
	}

	// FIXME: This is because we couldn't save/load the CTakeDamageInfo.
	// because it's midnight before the teamwide playtest. Real solution
	// is to add a datadesc to CTakeDamageInfo
	if ( m_KilledInfo.GetInflictor() )
	{
		BaseClass::Event_Killed( m_KilledInfo );
	}

	// Add a random chance of spawning a battery...
	if ( !HasSpawnFlags(SF_NPC_NO_WEAPON_DROP) && random->RandomFloat( 0.0f, 1.0f) < 0.3f )
	{
		CItem *pBattery = (CItem*)CreateEntityByName("item_battery");
		if ( pBattery )
		{
			pBattery->SetAbsOrigin( GetAbsOrigin() );
			pBattery->SetAbsVelocity( GetAbsVelocity() );
			pBattery->SetLocalAngularVelocity( GetLocalAngularVelocity() );
			pBattery->ActivateWhenAtRest();
			pBattery->Spawn();
		}
	}

	DeployMine();

	UTIL_Remove(this);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//			bPunting - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	if ( reason == PUNTED_BY_CANNON )
	{
		// There's about to be a massive change in velocity. 
		// Think immediately to handle changes in m_vCurrentVelocity;
		SetNextThink( gpGlobals->curtime + 0.01f );
		
		m_flEngineStallTime = gpGlobals->curtime + 2.0f;
		ScannerEmitSound( "DiveBomb" );
	}
	else
	{
		SetCondition( COND_SCANNER_GRABBED_BY_PHYSCANNON );
		ClearCondition( COND_SCANNER_RELEASED_FROM_PHYSCANNON );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysGunUser - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;
	
	ClearCondition( COND_SCANNER_GRABBED_BY_PHYSCANNON );
	SetCondition( COND_SCANNER_RELEASED_FROM_PHYSCANNON );
	
	if ( Reason == LAUNCHED_BY_CANNON )
	{
		m_flEngineStallTime = gpGlobals->curtime + 2.0f;

		// There's about to be a massive change in velocity. 
		// Think immediately to handle changes in m_vCurrentVelocity;
		SetNextThink( gpGlobals->curtime + 0.01f );
		ScannerEmitSound( "DiveBomb" );
	}
}


//------------------------------------------------------------------------------
// Do we have a physics attacker?
//------------------------------------------------------------------------------
CBasePlayer *CNPC_CScanner::HasPhysicsAttacker( float dt )
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if ( IsHeldByPhyscannon( ) || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime) )
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::StopLoopingSounds(void)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pEngineSound );
	m_pEngineSound = NULL;

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Event_Killed( const CTakeDamageInfo &info )
{
	// Copy off the takedamage info that killed me, since we're not going to call
	// up into the base class's Event_Killed() until we gib. (gibbing is ultimate death)
	m_KilledInfo = info;	

	DeployMine();

	ClearInspectTarget();

	// Interrupt whatever schedule I'm on
	SetCondition(COND_SCHEDULE_DONE);

	// Remove spotlight
	SpotlightDestroy();

	// Remove sprite
	UTIL_Remove(m_pEyeFlash);
	m_pEyeFlash = NULL;

	// If I have an enemy and I'm up high, do a dive bomb (unless dissolved)
	if ( !m_bIsClawScanner && GetEnemy() != NULL && (info.GetDamageType() & DMG_DISSOLVE) == false )
	{
		Vector vecDelta = GetLocalOrigin() - GetEnemy()->GetLocalOrigin();
		if ( ( vecDelta.z > 120 ) && ( vecDelta.Length() > 360 ) )
		{	
			// If I'm divebombing, don't take any more damage. It will make Event_Killed() be called again.
			// This is especially bad if someone machineguns the divebombing scanner. 
			AttackDivebomb();
			return;
		}
	}

	Gib();
}


//------------------------------------------------------------------------------
// Purpose: Checks to see if we hit anything while dive bombing.
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackDivebombCollide(float flInterval)
{
	//
	// Trace forward to see if I hit anything
	//
	Vector			checkPos = GetAbsOrigin() + (GetCurrentVelocity() * flInterval);
	trace_t			tr;
	CBaseEntity*	pHitEntity = NULL;
	AI_TraceHull( GetAbsOrigin(), checkPos, GetHullMins(), GetHullMaxs(), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if (tr.m_pEnt)
	{
		pHitEntity = tr.m_pEnt;

		// Did I hit an entity that isn't another scanner?
		if (pHitEntity && pHitEntity->Classify()!=CLASS_SCANNER)
		{
			if ( !pHitEntity->ClassMatches("item_battery") )
			{
				if ( !pHitEntity->IsWorld() )
				{
					CTakeDamageInfo info( this, this, sk_scanner_dmg_dive.GetFloat(), DMG_CLUB );
					CalculateMeleeDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
					pHitEntity->TakeDamage( info );
				}
				Gib();
			}
		}
	}

	if (tr.fraction != 1.0)
	{
		// We've hit something so deflect our velocity based on the surface
		// norm of what we've hit
		if (flInterval > 0)
		{
			float moveLen	= (1.0 - tr.fraction)*(GetAbsOrigin() - checkPos).Length();
			Vector vBounceVel	= moveLen*tr.plane.normal/flInterval;

			// If I'm right over the ground don't push down
			if (vBounceVel.z < 0)
			{
				float floorZ = GetFloorZ(GetAbsOrigin());
				if (abs(GetAbsOrigin().z - floorZ) < 36)
				{
					vBounceVel.z = 0;
				}
			}
			SetCurrentVelocity( GetCurrentVelocity() + vBounceVel );
		}
		CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHitEntity );

		if (pBCC)
		{
			// Spawn some extra blood where we hit
			SpawnBlood(tr.endpos, g_vecAttackDir, pBCC->BloodColor(), sk_scanner_dmg_dive.GetFloat());
		}
		else
		{
			if (!(m_spawnflags	& SF_NPC_GAG))
			{
				// <<TEMP>> need better sound here...
				ScannerEmitSound( "Shoot" );
			}
			// For sparks we must trace a line in the direction of the surface norm
			// that we hit.
			checkPos = GetAbsOrigin() - (tr.plane.normal * 24);

			AI_TraceLine( GetAbsOrigin(), checkPos,MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			if (tr.fraction != 1.0)
			{
				g_pEffects->Sparks( tr.endpos );

				CBroadcastRecipientFilter filter;
				te->DynamicLight( filter, 0.0,
					&GetAbsOrigin(), 255, 180, 100, 0, 50, 0.1, 0 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::FValidateHintType(CAI_Hint *pHint)
{
	return( pHint->HintType() == HINT_WORLD_WINDOW );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_CScanner::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
		case SCHED_IDLE_STAND:
		{
			return SCHED_SCANNER_PATROL;
		}
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : idealActivity - 
//			*pIdealWeaponActivity - 
// Output : int
//-----------------------------------------------------------------------------
Activity CNPC_CScanner::NPC_TranslateActivity( Activity eNewActivity )
{
	if( !m_bIsClawScanner )
	{
		return BaseClass::NPC_TranslateActivity( eNewActivity );
	}

	// The claw scanner came along a little late and doesn't have the activities
	// of the city scanner. So Just pick between these three
	if( eNewActivity == ACT_DISARM )
	{
		// Closing up.
		return eNewActivity;
	}

	if( m_bIsOpen )
	{
		return ACT_IDLE_ANGRY;
	}
	else
	{
		return ACT_IDLE;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CScanner::HandleAnimEvent( animevent_t *pEvent )
{
	if( pEvent->event == AE_SCANNER_CLOSED )
	{
		m_bIsOpen = false;
		SetActivity( ACT_IDLE );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Emit sounds specific to the NPC's state.
//-----------------------------------------------------------------------------
void CNPC_CScanner::AlertSound(void)
{
	ScannerEmitSound( "Alert" );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::DeathSound( const CTakeDamageInfo &info )
{
	ScannerEmitSound( "Die" );
}


//-----------------------------------------------------------------------------
// Purpose: Plays sounds while idle or in combat.
//-----------------------------------------------------------------------------
void CNPC_CScanner::IdleSound(void)
{
	if ( m_NPCState == NPC_STATE_COMBAT )
	{
		// dvs: the combat sounds should be related to what is happening, rather than random
		ScannerEmitSound( "Combat" );
	}
	else
	{
		ScannerEmitSound( "Idle" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Plays a sound when hurt.
//-----------------------------------------------------------------------------
void CNPC_CScanner::PainSound( const CTakeDamageInfo &info )
{
	ScannerEmitSound( "Pain" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::PlayFlySound(void)
{
	if ( IsMarkedForDeletion() )
	{
		return;
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	//Setup the sound if we're not already
	if ( m_pEngineSound == NULL )
	{
		// Create the sound
		CPASAttenuationFilter filter( this );

		if( m_bIsClawScanner )
		{
			m_pEngineSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "NPC_SScanner.FlyLoop", ATTN_NORM );
		}
		else
		{
			m_pEngineSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "NPC_CScanner.FlyLoop", ATTN_NORM );
		}

		Assert(m_pEngineSound);

		// Start the engine sound
		controller.Play( m_pEngineSound, 0.0f, 100.0f );
		controller.SoundChangeVolume( m_pEngineSound, 1.0f, 2.0f );
	}

	float	speed	 = GetCurrentVelocity().Length();
	float	flVolume = 0.25f + (0.75f*(speed/GetMaxSpeed()));
	int		iPitch	 = min( 255, 80 + (20*(speed/GetMaxSpeed())) );

	//Update our pitch and volume based on our speed
	controller.SoundChangePitch( m_pEngineSound, iPitch, 0.1f );
	controller.SoundChangeVolume( m_pEngineSound, flVolume, 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Plays the engine sound.
//-----------------------------------------------------------------------------
void CNPC_CScanner::NPCThink(void)
{
	if (!IsAlive())
	{
		SetActivity((Activity)ACT_SCANNER_RETRACT_PRONGS);
		StudioFrameAdvance( );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		BaseClass::NPCThink();
		SpotlightUpdate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::Precache(void)
{
	// Model
	if( m_bIsClawScanner )
	{
		PrecacheModel("models/shield_scanner.mdl");

		PrecacheModel("models/gibs/Shield_Scanner_Gib1.mdl");
		PrecacheModel("models/gibs/Shield_Scanner_Gib2.mdl");
		PrecacheModel("models/gibs/Shield_Scanner_Gib3.mdl");
		PrecacheModel("models/gibs/Shield_Scanner_Gib4.mdl");
		PrecacheModel("models/gibs/Shield_Scanner_Gib5.mdl");
		PrecacheModel("models/gibs/Shield_Scanner_Gib6.mdl");

		PrecacheScriptSound( "NPC_SScanner.Shoot");
		PrecacheScriptSound( "NPC_SScanner.Alert" );
		PrecacheScriptSound( "NPC_SScanner.Die" );
		PrecacheScriptSound( "NPC_SScanner.Combat" );
		PrecacheScriptSound( "NPC_SScanner.Idle" );
		PrecacheScriptSound( "NPC_SScanner.Pain" );
		PrecacheScriptSound( "NPC_SScanner.TakePhoto" );
		PrecacheScriptSound( "NPC_SScanner.AttackFlash" );
		PrecacheScriptSound( "NPC_SScanner.DiveBombFlyby" );
		PrecacheScriptSound( "NPC_SScanner.DiveBomb" );
		PrecacheScriptSound( "NPC_SScanner.DeployMine" );

		PrecacheScriptSound( "NPC_SScanner.FlyLoop" );
		UTIL_PrecacheOther( "combine_mine" );
	}
	else
	{
		PrecacheModel("models/combine_scanner.mdl");

		PrecacheModel("models/gibs/scanner_gib01.mdl" );
		PrecacheModel("models/gibs/scanner_gib02.mdl" );	
		PrecacheModel("models/gibs/scanner_gib02.mdl" );
		PrecacheModel("models/gibs/scanner_gib04.mdl" );
		PrecacheModel("models/gibs/scanner_gib05.mdl" );

		PrecacheScriptSound( "NPC_CScanner.Shoot");
		PrecacheScriptSound( "NPC_CScanner.Alert" );
		PrecacheScriptSound( "NPC_CScanner.Die" );
		PrecacheScriptSound( "NPC_CScanner.Combat" );
		PrecacheScriptSound( "NPC_CScanner.Idle" );
		PrecacheScriptSound( "NPC_CScanner.Pain" );
		PrecacheScriptSound( "NPC_CScanner.TakePhoto" );
		PrecacheScriptSound( "NPC_CScanner.AttackFlash" );
		PrecacheScriptSound( "NPC_CScanner.DiveBombFlyby" );
		PrecacheScriptSound( "NPC_CScanner.DiveBomb" );
		PrecacheScriptSound( "NPC_CScanner.DeployMine" );

		PrecacheScriptSound( "NPC_CScanner.FlyLoop" );
	}

	// Sprites
	m_nHaloSprite = PrecacheModel("sprites/light_glow03.vmt");
	PrecacheModel( "sprites/glow_test02.vmt" );

	BaseClass::Precache();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::SpeakSentence( int sentenceType )
{
	if (sentenceType == SCANNER_SENTENCE_ATTENTION)
	{
		ScannerEmitSound( "Attention" );
	}
	else if (sentenceType == SCANNER_SENTENCE_HANDSUP)
	{
		ScannerEmitSound( "Scan" );
	}
	else if (sentenceType == SCANNER_SENTENCE_PROCEED)
	{
		ScannerEmitSound( "Proceed" );
	}
	else if (sentenceType == SCANNER_SENTENCE_CURIOUS)
	{
		ScannerEmitSound( "Curious" );
	}
}


//------------------------------------------------------------------------------
// Purpose: Request help inspecting from other squad members
//------------------------------------------------------------------------------
void CNPC_CScanner::RequestInspectSupport(void)
{
	if (m_pSquad)
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			if (pSquadMember != this)
			{
				if (GetTarget())
				{
					pSquadMember->DispatchInteraction(g_interactionScannerSupportEntity,((void *)((CBaseEntity*)GetTarget())),this);
				}
				else
				{
					pSquadMember->DispatchInteraction(g_interactionScannerSupportPosition,((void *)m_vInspectPos.Base()),this);
				}
			}
		}
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_CScanner::IsValidInspectTarget(CBaseEntity *pEntity)
{
	// If a citizen, make sure he can be inspected again
	if (pEntity->Classify() == CLASS_CITIZEN_PASSIVE)
	{
		if (((CNPC_Citizen*)pEntity)->GetNextScannerInspectTime() > gpGlobals->curtime)
		{
			return false;
		}
	}

	// Make sure no other squad member has already chosen to 
	// inspect this entity
	if (m_pSquad)
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			if (pSquadMember->GetTarget() == pEntity)
			{
				return false;
			}
		}
	}

	// Do not inspect friendly targets
	if ( IRelationType( pEntity ) == D_LI )
		return false;

	return true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CBaseEntity* CNPC_CScanner::BestInspectTarget(void)
{
	if ( !m_bShouldInspect )
		return NULL;

	CBaseEntity*	pBestEntity = NULL;
	float			fBestDist	= MAX_COORD_RANGE;
	float			fTestDist;

	CBaseEntity *pEntity = NULL;

	// If I have a spotlight, search from the spotlight position
	// otherwise search from my position
	Vector	vSearchOrigin;
	float	fSearchDist;
	if (m_hSpotlightTarget != NULL)
	{
		vSearchOrigin	= m_hSpotlightTarget->GetAbsOrigin();
		fSearchDist		= SCANNER_CIT_INSPECT_GROUND_DIST;
	}
	else
	{
		vSearchOrigin	= WorldSpaceCenter();
		fSearchDist		= SCANNER_CIT_INSPECT_FLY_DIST;
	}

	if ( m_bOnlyInspectPlayers )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( !pPlayer )
			return NULL;

		if ( !pPlayer->IsAlive() || (pPlayer->GetFlags() & FL_NOTARGET) )
			return NULL;

		return WorldSpaceCenter().DistToSqr( pPlayer->EyePosition() ) <= (fSearchDist * fSearchDist) ? pPlayer : NULL;
	}

	CUtlVector<CBaseEntity *> candidates;
	float fSearchDistSq = fSearchDist * fSearchDist;
	int i;

	// Inspect players unless told otherwise
	if ( m_bNeverInspectPlayers == false )
	{
		// Players
		for ( i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( vSearchOrigin.DistToSqr(pPlayer->GetAbsOrigin()) < fSearchDistSq )
				{
					candidates.AddToTail( pPlayer );
				}
			}
		}
	}
	
	// NPCs
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	
	for ( i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		if ( ppAIs[i] != this && vSearchOrigin.DistToSqr(ppAIs[i]->GetAbsOrigin()) < fSearchDistSq )
		{
			candidates.AddToTail( ppAIs[i] );
		}
	}

	for ( i = 0; i < candidates.Count(); i++ )
	{
		pEntity = candidates[i];
		Assert( pEntity != this && (pEntity->MyNPCPointer() || pEntity->IsPlayer() ) );

		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if ( ( pNPC && pNPC->Classify() == CLASS_CITIZEN_PASSIVE ) || pEntity->IsPlayer() )
		{
			if ( pEntity->GetFlags() & FL_NOTARGET )
				continue;

			if ( pEntity->IsAlive() == false )
				continue;

			// Ensure it's within line of sight
			if ( !FVisible( pEntity ) )
				continue;

			fTestDist = ( GetAbsOrigin() - pEntity->EyePosition() ).Length();
			if ( fTestDist < fBestDist )
			{
				if ( IsValidInspectTarget( pEntity ) )
				{
					fBestDist	= fTestDist;
					pBestEntity	= pEntity; 
				}
			}
		}
	}
	return pBestEntity;
}


//------------------------------------------------------------------------------
// Purpose: Clears any previous inspect target and set inspect target to
//			 the given entity and set the durection of the inspection
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToEnt(CBaseEntity *pEntity, float fInspectDuration)
{
	ClearInspectTarget();
	SetTarget(pEntity);
	
	m_fInspectEndTime = gpGlobals->curtime + fInspectDuration;
}


//------------------------------------------------------------------------------
// Purpose: Clears any previous inspect target and set inspect target to
//			 the given hint node and set the durection of the inspection
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToHint(CAI_Hint *pHint, float fInspectDuration)
{
	ClearInspectTarget();

	float yaw = pHint->Yaw();
	// --------------------------------------------
	// Figure out the location that the hint hits
	// --------------------------------------------
	Vector vHintDir	= UTIL_YawToVector( yaw );

	Vector vHintOrigin;
	pHint->GetPosition( this, &vHintOrigin );

	Vector vHintEnd	= vHintOrigin + (vHintDir * 512);
	
	trace_t tr;
	AI_TraceLine ( vHintOrigin, vHintEnd, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
	
	if ( g_debug_cscanner.GetBool() )
	{
		NDebugOverlay::Line( vHintOrigin, tr.endpos, 255, 0, 0, true, 4.0f );
		NDebugOverlay::Cross3D( tr.endpos, -Vector(8,8,8), Vector(8,8,8), 255, 0, 0, true, 4.0f );
	}

	if (tr.fraction == 1.0f )
	{
		DevMsg("ERROR: Scanner hint node not facing a surface!\n");
	}
	else
	{
		SetHintNode( pHint );
		m_vInspectPos = tr.endpos;
		pHint->Lock( this );

		m_fInspectEndTime = gpGlobals->curtime + fInspectDuration;
	}
}


//------------------------------------------------------------------------------
// Purpose: Clears any previous inspect target and set inspect target to
//			 the given position and set the durection of the inspection
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_CScanner::SetInspectTargetToPos(const Vector &vInspectPos, float fInspectDuration)
{
	ClearInspectTarget();
	m_vInspectPos		= vInspectPos;

	m_fInspectEndTime	= gpGlobals->curtime + fInspectDuration;
}


//------------------------------------------------------------------------------
// Purpose: Clears out any previous inspection targets
//------------------------------------------------------------------------------
void CNPC_CScanner::ClearInspectTarget(void)
{
	if ( GetIdealState() != NPC_STATE_SCRIPT )
	{
		SetTarget( NULL );
	}

	ClearHintNode( SCANNER_HINT_INSPECT_LENGTH );
	m_vInspectPos	= vec3_origin;
}


//------------------------------------------------------------------------------
// Purpose: Returns true if there is a position to be inspected.
//------------------------------------------------------------------------------
bool CNPC_CScanner::HaveInspectTarget( void )
{
	if ( GetTarget() != NULL )
		return true;

	if ( m_vInspectPos != vec3_origin )
		return true;

	return false;
}


//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
Vector CNPC_CScanner::InspectTargetPosition(void)
{
	// If we have a target, return an adjust position
	if ( GetTarget() != NULL )
	{
		Vector	vEyePos = GetTarget()->EyePosition();

		// If in spotlight mode, aim for ground below target unless is client
		if ( m_nFlyMode == SCANNER_FLY_SPOT && !(GetTarget()->GetFlags() & FL_CLIENT) )
		{
			Vector vInspectPos;
			vInspectPos.x	= vEyePos.x;
			vInspectPos.y	= vEyePos.y;
			vInspectPos.z	= GetFloorZ( vEyePos );

			// Let's take three-quarters between eyes and ground
			vInspectPos.z	+= ( vEyePos.z - vInspectPos.z ) * 0.75f;

			return vInspectPos;
		}
		else
		{
			// Otherwise aim for eyes
			return vEyePos;
		}
	}
	else if ( m_vInspectPos != vec3_origin )
	{
		return m_vInspectPos;
	}
	else
	{
		DevMsg("InspectTargetPosition called with no target!\n");
		
		return m_vInspectPos;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputShouldInspect( inputdata_t &inputdata )
{
	m_bShouldInspect = ( inputdata.value.Int() != 0 );
	
	if ( !m_bShouldInspect )
	{
		if ( GetEnemy() == GetTarget() )
			SetEnemy(NULL);
		ClearInspectTarget();
		SetTarget(NULL);
		SpotlightDestroy();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputSetFlightSpeed(inputdata_t &inputdata)
{
	//FIXME: Currently unsupported

	/*
	m_flFlightSpeed = inputdata.value.Int();
	m_bFlightSpeedOverridden = (m_flFlightSpeed > 0);
	*/
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CScanner::DeployMine()
{
	CBaseEntity *child;
	// iterate through all children
	for ( child = FirstMoveChild(); child != NULL; child = child->NextMovePeer() )
	{
		if( FClassnameIs( child, "combine_mine" ) )
		{
			child->SetParent( NULL );
			child->SetAbsVelocity( GetAbsVelocity() );
			child->SetOwnerEntity( this );

			ScannerEmitSound( "DeployMine" );

			IPhysicsObject *pPhysObj = child->VPhysicsGetObject();
			if( pPhysObj )
			{
				// Make sure the mine's awake
				pPhysObj->Wake();
			}

			if( m_bIsClawScanner )
			{
				// Fold up.
				SetActivity( ACT_DISARM );
			}

			return;
		}
	}
}

float CNPC_CScanner::GetMaxSpeed()
{
	if( IsStriderScout() )
	{
		return SCANNER_SCOUT_MAX_SPEED;
	}

	return SCANNER_MAX_SPEED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputDeployMine(inputdata_t &inputdata)
{
	DeployMine();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputEquipMine(inputdata_t &inputdata)
{
	CBaseEntity *child;
	// iterate through all children
	for ( child = FirstMoveChild(); child != NULL; child = child->NextMovePeer() )
	{
		if( FClassnameIs( child, "combine_mine" ) )
		{
			// Already have a mine!
			return;
		}
	}

	CBaseEntity *pEnt;

	pEnt = CreateEntityByName( "combine_mine" );
	bool bPlacedMine = false;

	if( m_bIsClawScanner )
	{
		Vector	vecOrigin;
		QAngle	angles;
		int		attachment;

		attachment = LookupAttachment( "claw" );

		if( attachment > -1 )
		{
			GetAttachment( attachment, vecOrigin, angles );
			
			pEnt->SetAbsOrigin( vecOrigin );
			pEnt->SetAbsAngles( angles );
			pEnt->SetOwnerEntity( this );
			pEnt->SetParent( this, attachment );

			m_bIsOpen = true;
			SetActivity( ACT_IDLE_ANGRY );
			bPlacedMine = true;
		}
	}


	if( !bPlacedMine )
	{
		Vector vecMineLocation = GetAbsOrigin();
		vecMineLocation.z -= 32.0;

		pEnt->SetAbsOrigin( vecMineLocation );
		pEnt->SetAbsAngles( GetAbsAngles() );
		pEnt->SetOwnerEntity( this );
		pEnt->SetParent( this );
	}

	pEnt->Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go photograph an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputInspectTargetPhoto(inputdata_t &inputdata)
{
	m_vLastPatrolDir = vec3_origin;
	m_bPhotoTaken = false;
	InspectTarget( inputdata, SCANNER_FLY_PHOTO );
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go spotlight an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputInspectTargetSpotlight(inputdata_t &inputdata)
{
	InspectTarget( inputdata, SCANNER_FLY_SPOT );
}


//-----------------------------------------------------------------------------
// Purpose: Tells the scanner to go photo or spotlight an entity.
// Input  : String name or classname of the entity to inspect.
//-----------------------------------------------------------------------------
void CNPC_CScanner::InspectTarget( inputdata_t &inputdata, ScannerFlyMode_t eFlyMode )
{
	CBaseEntity *pEnt = gEntList.FindEntityGeneric( NULL, inputdata.value.String(), this, inputdata.pActivator );
	
	if ( pEnt != NULL )
	{
		// Set and begin to inspect our target
		SetInspectTargetToEnt( pEnt, SCANNER_CIT_INSPECT_LENGTH );
		
		m_nFlyMode = eFlyMode;
		SetCondition( COND_SCANNER_HAVE_INSPECT_TARGET );
		
		// Stop us from any other navigation we were doing
		GetNavigator()->ClearGoal();
	}
	else
	{
		DevMsg( "InspectTarget: target %s not found!\n", inputdata.value.String() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::MovingToInspectTarget( void )
{
	// If we're flying to a photograph target and the photo isn't yet taken, we're still moving to it
	if ( m_nFlyMode == SCANNER_FLY_PHOTO && m_bPhotoTaken == false )
		return true;

	// If we're still on a path, then we're still moving
	if ( HaveInspectTarget() && GetNavigator()->IsGoalActive() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Clear out our old conditions
	ClearCondition( COND_SCANNER_INSPECT_DONE );
	ClearCondition( COND_SCANNER_HAVE_INSPECT_TARGET );
	ClearCondition( COND_SCANNER_SPOT_ON_TARGET );
	ClearCondition( COND_SCANNER_CAN_PHOTOGRAPH );

	// We don't do any of these checks if we have an enemy
	if ( GetEnemy() )
		return;

	// --------------------------------------
	//  COND_SCANNER_INSPECT_DONE
	//
	// If my inspection over 
	// ---------------------------------------------------------

	// Refresh our timing if we're still moving to our inspection target
	if ( MovingToInspectTarget() )
	{
		m_fInspectEndTime = gpGlobals->curtime + SCANNER_CIT_INSPECT_LENGTH;
	}

	// Update our follow times
	if ( HaveInspectTarget() && gpGlobals->curtime > m_fInspectEndTime && m_nFlyMode != SCANNER_FLY_FOLLOW )
	{
		SetCondition ( COND_SCANNER_INSPECT_DONE );

		m_fCheckCitizenTime	= gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
		m_fCheckHintTime	= gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;
		ClearInspectTarget();
	}

	// ----------------------------------------------------------
	//  If I heard a sound and I don't have an enemy, inspect it
	// ----------------------------------------------------------
	if ( ( HasCondition( COND_HEAR_COMBAT ) || HasCondition( COND_HEAR_DANGER ) ) && m_nFlyMode != SCANNER_FLY_FOLLOW )
	{
		CSound *pSound = GetBestSound();
		
		if ( pSound )
		{
			// Chase an owner if we can
			if ( pSound->m_hOwner != NULL )
			{
				// Don't inspect sounds of things we like
				if ( IRelationType( pSound->m_hOwner ) != D_LI )
				{
					// Only bother if we can see it
					if ( FVisible( pSound->m_hOwner ) )
					{
						SetInspectTargetToEnt( pSound->m_hOwner, SCANNER_SOUND_INSPECT_LENGTH );
					}
				}
			}
			else
			{
				// Otherwise chase the specific sound
				Vector vSoundPos = pSound->GetSoundOrigin();
				SetInspectTargetToPos( vSoundPos, SCANNER_SOUND_INSPECT_LENGTH );
			}

			m_nFlyMode = (random->RandomInt(0,2)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
		}
	}

	// --------------------------------------
	//  COND_SCANNER_HAVE_INSPECT_TARGET
	//
	// Look for a nearby citizen or player to hassle. 
	// ---------------------------------------------------------

	// Check for citizens to inspect
	if ( gpGlobals->curtime	> m_fCheckCitizenTime && HaveInspectTarget() == false )
	{
		CBaseEntity *pBestEntity = BestInspectTarget();
		
		if ( pBestEntity != NULL )
		{
			SetInspectTargetToEnt( pBestEntity, SCANNER_CIT_INSPECT_LENGTH );
			m_nFlyMode = (random->RandomInt(0,3)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
			SetCondition ( COND_SCANNER_HAVE_INSPECT_TARGET );
		}
	}

	// Check for hints to inspect
	if ( gpGlobals->curtime > m_fCheckHintTime && HaveInspectTarget() == false )
	{
		SetHintNode( CAI_HintManager::FindHint( this, HINT_WORLD_WINDOW, 0, SCANNER_CIT_INSPECT_FLY_DIST ) );

		if ( GetHintNode() )
		{
			m_fCheckHintTime = gpGlobals->curtime + SCANNER_HINT_INSPECT_DELAY;

			m_nFlyMode = (random->RandomInt(0,2)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;

			SetInspectTargetToHint( GetHintNode(), SCANNER_HINT_INSPECT_LENGTH );

			SetCondition ( COND_SCANNER_HAVE_INSPECT_TARGET );
		}
	}

	// --------------------------------------
	//  COND_SCANNER_SPOT_ON_TARGET
	//
	//  True when spotlight is on target ent
	// --------------------------------------

	if ( m_hSpotlightTarget != NULL	&& HaveInspectTarget() && m_hSpotlightTarget->GetSmoothedVelocity().Length() < 25 )
	{
		// If I have a target entity, check my spotlight against the
		// actual position of the entity
		if (GetTarget())
		{
			float fInspectDist = (m_vSpotlightTargetPos - m_vSpotlightCurrentPos).Length();
			if ( fInspectDist < 100 )
			{
				SetCondition( COND_SCANNER_SPOT_ON_TARGET );
			}
		}
		// Otherwise just check by beam direction
		else
		{
			Vector vTargetDir = SpotlightTargetPos() - GetLocalOrigin();
			VectorNormalize(vTargetDir);
			float dotpr = DotProduct(vTargetDir, m_vSpotlightDir);
			if (dotpr > 0.95)
			{
				SetCondition( COND_SCANNER_SPOT_ON_TARGET );
			}
		}
	}

	// --------------------------------------------
	//  COND_SCANNER_CAN_PHOTOGRAPH
	//
	//  True when can photograph target ent
	// --------------------------------------------

	ClearCondition( COND_SCANNER_CAN_PHOTOGRAPH );

	if ( m_nFlyMode == SCANNER_FLY_PHOTO )
	{
		// Make sure I have something to photograph and I'm ready to photograph and I'm not moving to fast
		if ( gpGlobals->curtime > m_fNextPhotographTime && HaveInspectTarget() && GetCurrentVelocity().LengthSqr() < (64*64) )
		{
			// Check that I'm in the right distance range
			float  fInspectDist = (InspectTargetPosition() - GetAbsOrigin()).Length2D();
			
			// See if we're within range
			if ( fInspectDist > SCANNER_PHOTO_NEAR_DIST && fInspectDist < SCANNER_PHOTO_FAR_DIST )
			{
				// Make sure we're looking at the target
				if ( UTIL_AngleDiff( GetAbsAngles().y, VecToYaw( InspectTargetPosition() - GetAbsOrigin() ) ) < 4.0f )
				{
					trace_t tr;
					AI_TraceLine ( GetAbsOrigin(), InspectTargetPosition(), MASK_OPAQUE, GetTarget(), COLLISION_GROUP_NONE, &tr);
					
					if ( tr.fraction == 1.0f )
					{
						SetCondition( COND_SCANNER_CAN_PHOTOGRAPH );
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	// Go back to idling if we're done
	if ( GetIdealActivity() == ACT_SCANNER_FLARE_START )
	{
		if ( IsSequenceFinished() )
		{
			SetIdealActivity( (Activity) ACT_IDLE );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
//-----------------------------------------------------------------------------
int CNPC_CScanner::MeleeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return COND_NONE;
	}

	// Check too far to attack with 2D distance
	float vEnemyDist2D = (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()).Length2D();

	if (m_flNextAttack > gpGlobals->curtime)
	{
		return COND_NONE;
	}
	else if (vEnemyDist2D > SCANNER_ATTACK_RANGE)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: Overridden because if the player is a criminal, we hate them.
// Input  : pTarget - Entity with which to determine relationship.
// Output : Returns relationship value.
//-----------------------------------------------------------------------------
Disposition_t CNPC_CScanner::IRelationType(CBaseEntity *pTarget)
{
	// If it's the player and they are a criminal, we hates them
	if ( pTarget->Classify() == CLASS_PLAYER )
	{
		if ( GlobalEntity_GetState("gordon_precriminal") == GLOBAL_ON )
			return D_NU;
	}

	return BaseClass::IRelationType( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_SCANNER_PHOTOGRAPH:
		{
			if ( IsWaitFinished() )
			{	
				// If light was on turn it off
				if ( m_pEyeFlash->GetBrightness() > 0 )
				{
					m_pEyeFlash->SetBrightness( 0 );

					// I'm done with this target
					if ( gpGlobals->curtime > m_fInspectEndTime )
					{
						ClearInspectTarget();
						TaskComplete();
					}
					// Otherwise take another picture
					else
					{
						SetWait( 5.0f, 10.0f );
					}
				}
				// If light was off, take another picture
				else
				{
					TakePhoto();
					SetWait( 0.1f );
				}
			}
			break;
		}
		case TASK_SCANNER_ATTACK_PRE_FLASH:
		{
			AttackPreFlash();
			
			if ( IsWaitFinished() )
			{
				TaskComplete();
			}
			break;
		}
		case TASK_SCANNER_ATTACK_FLASH:
		{
			if (IsWaitFinished())
			{
				AttackFlashBlind();
				TaskComplete();
			}
			break;
		}
		default:
		{
			BaseClass::RunTask(pTask);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_CScanner::SelectSchedule(void)
{
	// Turn our flash off in case we were interrupted while it was on.
	if ( m_pEyeFlash )
	{
		m_pEyeFlash->SetBrightness( 0 );
	}

	// ----------------------------------------------------
	//  If I'm dead, go into a dive bomb
	// ----------------------------------------------------
	if ( m_iHealth <= 0 )
	{
		m_flSpeed = SCANNER_MAX_DIVE_BOMB_SPEED;
		return SCHED_SCANNER_ATTACK_DIVEBOMB;
	}

	// -------------------------------
	// If I'm in a script sequence
	// -------------------------------
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return(BaseClass::SelectSchedule());

	// -------------------------------
	// Flinch
	// -------------------------------
	if ( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) )
	{
		if ( IsHeldByPhyscannon( ) ) 
 			return SCHED_SMALL_FLINCH;

		if ( m_NPCState == NPC_STATE_IDLE )
			return SCHED_SMALL_FLINCH;

		if ( m_NPCState == NPC_STATE_ALERT )
		{
			if ( m_iHealth < ( 3 * sk_scanner_health.GetFloat() / 4 ))
				return SCHED_TAKE_COVER_FROM_ORIGIN;

			if ( SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
				return SCHED_SMALL_FLINCH;
		}
		else
		{
			if ( random->RandomInt( 0, 10 ) < 4 )
				return SCHED_SMALL_FLINCH;
		}
	}

	// I'm being held by the physcannon... struggle!
	if ( IsHeldByPhyscannon( ) ) 
		return SCHED_SCANNER_HELD_BY_PHYSCANNON;

	// ----------------------------------------------------------
	//  If I have an enemy
	// ----------------------------------------------------------
	if ( GetEnemy() != NULL && GetEnemy()->IsAlive() && m_bShouldInspect )
	{
		// Always chase the enemy
		SetInspectTargetToEnt( GetEnemy(), 9999 );

		// Patrol if the enemy has vanished
		if ( HasCondition( COND_LOST_ENEMY ) )
			return SCHED_SCANNER_PATROL;
		
		// Chase via route if we're directly blocked
		if ( HasCondition( COND_SCANNER_FLY_BLOCKED ) )
			return SCHED_SCANNER_CHASE_ENEMY;
		
		// Attack if it's time
		if ( gpGlobals->curtime < m_flNextAttack )
			return SCHED_SCANNER_SPOTLIGHT_HOVER;

		// Melee attack if possible
		if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
		{ 
			if ( random->RandomInt(0,1) )
				return SCHED_SCANNER_ATTACK_FLASH;

			// TODO: a schedule where he makes an alarm sound?
			return SCHED_SCANNER_CHASE_ENEMY;
		}

		// If I'm far from the enemy, stay up high and approach in spotlight mode
		float fAttack2DDist = ( GetEnemyLKP() - GetAbsOrigin() ).Length2D();

		if ( fAttack2DDist > SCANNER_ATTACK_FAR_DIST )
			return SCHED_SCANNER_SPOTLIGHT_HOVER;

		// Otherwise fly in low for attack
		return SCHED_SCANNER_ATTACK_HOVER;
	}

	// ----------------------------------------------------------
	//  If I have something to inspect
	// ----------------------------------------------------------
	if ( HaveInspectTarget() )
	{
		// Pathfind to our goal
		if ( HasCondition( COND_SCANNER_FLY_BLOCKED ) )
			return SCHED_SCANNER_MOVE_TO_INSPECT;

		// If I was chasing, pick with photographing or spotlighting 
		if ( m_nFlyMode == SCANNER_FLY_CHASE )
		{
			m_nFlyMode = (random->RandomInt(0,1)==0) ? SCANNER_FLY_SPOT : SCANNER_FLY_PHOTO;
		}

		// Handle spotlight
		if ( m_nFlyMode == SCANNER_FLY_SPOT )
		{
			if (HasCondition( COND_SCANNER_SPOT_ON_TARGET ))
			{
				if (GetTarget())
				{
					RequestInspectSupport();

					CAI_BaseNPC *pNPC = GetTarget()->MyNPCPointer();
					// If I'm leading the inspection, so verbal inspection
					if (pNPC && pNPC->GetTarget() == this)
					{
						return SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT;
					}

					return SCHED_SCANNER_SPOTLIGHT_HOVER;
				}

				return SCHED_SCANNER_SPOTLIGHT_INSPECT_POS;
			}

			return SCHED_SCANNER_SPOTLIGHT_HOVER;
		}
		
		// Handle photographing
		if ( m_nFlyMode == SCANNER_FLY_PHOTO )
		{
			if ( HasCondition( COND_SCANNER_CAN_PHOTOGRAPH ))
				return SCHED_SCANNER_PHOTOGRAPH;

			return SCHED_SCANNER_PHOTOGRAPH_HOVER;
		}
		
		// Handle following after a target
		if ( m_nFlyMode == SCANNER_FLY_FOLLOW )
		{
			//TODO: Randomly make noise, photograph, etc
			return SCHED_SCANNER_FOLLOW_HOVER;
		}

		// Handle patrolling
		if ( ( m_nFlyMode == SCANNER_FLY_PATROL ) || ( m_nFlyMode == SCANNER_FLY_FAST ) )
			return SCHED_SCANNER_PATROL;
	}

	// Default to patrolling around
	return SCHED_SCANNER_PATROL;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightDestroy(void)
{
	if ( m_hSpotlight )
	{
		UTIL_Remove(m_hSpotlight);
		m_hSpotlight = NULL;
		
		UTIL_Remove(m_hSpotlightTarget);
		m_hSpotlightTarget = NULL;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightCreate(void)
{
	// Make sure we don't already have one
	if ( m_hSpotlight != NULL )
		return;

	// Can we create a spotlight yet?
	if ( gpGlobals->curtime < m_fNextSpotlightTime )
		return;

	// If I have an enemy, start spotlight on my enemy
	if (GetEnemy() != NULL)
	{
		Vector vEnemyPos	= GetEnemyLKP();
		Vector vTargetPos	= vEnemyPos;
		vTargetPos.z		= GetFloorZ(vEnemyPos);
		m_vSpotlightDir = vTargetPos - GetLocalOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	// If I have an target, start spotlight on my target
	else if (GetTarget() != NULL)
	{
		Vector vTargetPos	= GetTarget()->GetLocalOrigin();
		vTargetPos.z		= GetFloorZ(GetTarget()->GetLocalOrigin());
		m_vSpotlightDir = vTargetPos - GetLocalOrigin();
		VectorNormalize(m_vSpotlightDir);
	}
	// Other wise just start looking down
	else
	{
		m_vSpotlightDir	= Vector(0,0,-1); 
	}

	trace_t tr;
	AI_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + m_vSpotlightDir * 2024, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

	m_hSpotlightTarget = (CSpotlightEnd*)CreateEntityByName( "spotlight_end" );
	m_hSpotlightTarget->Spawn();
	m_hSpotlightTarget->SetLocalOrigin( tr.endpos );
	m_hSpotlightTarget->SetOwnerEntity( this );
	// YWB:  Because the scanner only moves the target during think, make sure we interpolate over 0.1 sec instead of every tick!!!
	m_hSpotlightTarget->SetSimulatedEveryTick( false );

	// Using the same color as the beam...
	m_hSpotlightTarget->SetRenderColor( 255, 255, 255 );
	m_hSpotlightTarget->m_Radius = m_flSpotlightMaxLength;

	m_hSpotlight = CBeam::BeamCreate( "sprites/glow_test02.vmt", SPOTLIGHT_WIDTH );
	// Set the temporary spawnflag on the beam so it doesn't save (we'll recreate it on restore)
	m_hSpotlight->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_hSpotlight->SetColor( 255, 255, 255 ); 
	m_hSpotlight->SetHaloTexture( m_nHaloSprite );
	m_hSpotlight->SetHaloScale( 32 );
	m_hSpotlight->SetEndWidth( m_hSpotlight->GetWidth() );
	m_hSpotlight->SetBeamFlags( (FBEAM_SHADEOUT|FBEAM_NOTILE) );
	m_hSpotlight->SetBrightness( 32 );
	m_hSpotlight->SetNoise( 0 );
	m_hSpotlight->EntsInit( this, m_hSpotlightTarget );

	// attach to light
	m_hSpotlight->SetStartAttachment( LookupAttachment( SCANNER_ATTACHMENT_LIGHT ) );

	m_vSpotlightAngVelocity = vec3_origin;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
Vector CNPC_CScanner::SpotlightTargetPos(void)
{
	// ----------------------------------------------
	//  If I have an enemy 
	// ----------------------------------------------
	if (GetEnemy() != NULL)
	{
		// If I can see my enemy aim for him
		if (HasCondition(COND_SEE_ENEMY))
		{
			// If its client aim for his eyes
			if (GetEnemy()->GetFlags() & FL_CLIENT)
			{
				m_vSpotlightTargetPos = GetEnemy()->EyePosition();
			}
			// Otherwise same for his feet
			else
			{
				m_vSpotlightTargetPos	= GetEnemy()->GetLocalOrigin();
				m_vSpotlightTargetPos.z	= GetFloorZ(GetEnemy()->GetLocalOrigin());
			}
		}
		// Otherwise aim for last known position if I can see LKP
		else
		{
			Vector vLKP				= GetEnemyLKP();
			m_vSpotlightTargetPos.x	= vLKP.x;
			m_vSpotlightTargetPos.y	= vLKP.y;
			m_vSpotlightTargetPos.z	= GetFloorZ(vLKP);
		}
	}
	// ----------------------------------------------
	//  If I have an inspect target
	// ----------------------------------------------
	else if (HaveInspectTarget())
	{
		m_vSpotlightTargetPos = InspectTargetPosition();
	}
	else
	{
		// This creates a nice patrol spotlight sweep
		// in the direction that I'm travelling
		m_vSpotlightTargetPos	= GetCurrentVelocity();
		m_vSpotlightTargetPos.z = 0;
		VectorNormalize( m_vSpotlightTargetPos );
		m_vSpotlightTargetPos   *= 5;

		float noiseScale = 2.5;
		const Vector &noiseMod = GetNoiseMod();
		m_vSpotlightTargetPos.x += noiseScale*sin(noiseMod.x * gpGlobals->curtime + noiseMod.x);
		m_vSpotlightTargetPos.y += noiseScale*cos(noiseMod.y* gpGlobals->curtime + noiseMod.y);
		m_vSpotlightTargetPos.z -= fabs(noiseScale*cos(noiseMod.z* gpGlobals->curtime + noiseMod.z) );
		m_vSpotlightTargetPos   = GetLocalOrigin()+m_vSpotlightTargetPos * 2024;
	}

	return m_vSpotlightTargetPos;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
Vector CNPC_CScanner::SpotlightCurrentPos(void)
{
	Vector vTargetDir		= SpotlightTargetPos() - GetLocalOrigin();
	VectorNormalize(vTargetDir);

	if (!m_hSpotlight)
	{
		DevMsg("Spotlight pos. called w/o spotlight!\n");
		return vec3_origin;
	}
	// -------------------------------------------------
	//  Beam has momentum relative to it's ground speed
	//  so sclae the turn rate based on its distance
	//  from the beam source
	// -------------------------------------------------
	float	fBeamDist		= (m_hSpotlightTarget->GetLocalOrigin() - GetLocalOrigin()).Length();

	float	fBeamTurnRate	= atan(50/fBeamDist);
	Vector  vNewAngVelocity = fBeamTurnRate * (vTargetDir - m_vSpotlightDir);

	float	myDecay	 = 0.4;
	m_vSpotlightAngVelocity = (myDecay * m_vSpotlightAngVelocity + (1-myDecay) * vNewAngVelocity);

	// ------------------------------
	//  Limit overall angular speed
	// -----------------------------
	if (m_vSpotlightAngVelocity.Length() > 1)
	{

		Vector velDir = m_vSpotlightAngVelocity;
		VectorNormalize(velDir);
		m_vSpotlightAngVelocity = velDir * 1;
	}

	// ------------------------------
	//  Calculate new beam direction
	// ------------------------------
	m_vSpotlightDir = m_vSpotlightDir + m_vSpotlightAngVelocity;
	m_vSpotlightDir = m_vSpotlightDir;
	VectorNormalize(m_vSpotlightDir);


	// ---------------------------------------------
	//	Get beam end point.  Only collide with
	//  solid objects, not npcs
	// ---------------------------------------------
	trace_t tr;
	Vector vTraceEnd = GetAbsOrigin() + (m_vSpotlightDir * 2 * m_flSpotlightMaxLength);
	AI_TraceLine ( GetAbsOrigin(), vTraceEnd, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);

	return (tr.endpos);
}


//------------------------------------------------------------------------------
// Purpose: Update the direction and position of my spotlight
//------------------------------------------------------------------------------
void CNPC_CScanner::SpotlightUpdate(void)
{
	//FIXME: JDW - E3 Hack
	if ( m_bNoLight )
	{
		if ( m_hSpotlight )
		{
			SpotlightDestroy();
		}

		return;
	}

	if ((m_nFlyMode != SCANNER_FLY_SPOT) &&
		(m_nFlyMode != SCANNER_FLY_PATROL) && 
		(m_nFlyMode != SCANNER_FLY_FAST))
	{
		if ( m_hSpotlight )
		{	
			SpotlightDestroy();
		}
		return;
	}
	
	// If I don't have a spotlight attempt to create one

	if ( m_hSpotlight == NULL )
	{
		SpotlightCreate();
		
		if ( m_hSpotlight== NULL )
			return;
	}

	// Calculate the new homing target position
	m_vSpotlightCurrentPos = SpotlightCurrentPos();

	// ------------------------------------------------------------------
	//  If I'm not facing the spotlight turn it off 
	// ------------------------------------------------------------------
	Vector vSpotDir = m_vSpotlightCurrentPos - GetAbsOrigin();
	VectorNormalize(vSpotDir);
	
	Vector	vForward;
	AngleVectors( GetAbsAngles(), &vForward );

	float dotpr = DotProduct( vForward, vSpotDir );
	
	if ( dotpr < 0.0 )
	{
		// Leave spotlight off for a while
		m_fNextSpotlightTime = gpGlobals->curtime + 3.0f;

		SpotlightDestroy();
		return;
	}

	// --------------------------------------------------------------
	//  Update spotlight target velocity
	// --------------------------------------------------------------
	Vector vTargetDir  = (m_vSpotlightCurrentPos - m_hSpotlightTarget->GetLocalOrigin());
	float  vTargetDist = vTargetDir.Length();

	Vector vecNewVelocity = vTargetDir;
	VectorNormalize(vecNewVelocity);
	vecNewVelocity *= (10 * vTargetDist);

	// If a large move is requested, just jump to final spot as we
	// probably hit a discontinuity
	if (vecNewVelocity.Length() > 200)
	{
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= 200;
		m_hSpotlightTarget->SetLocalOrigin( m_vSpotlightCurrentPos );
	}
	m_hSpotlightTarget->SetAbsVelocity( vecNewVelocity );

	m_hSpotlightTarget->m_vSpotlightOrg = GetAbsOrigin();

	// Avoid sudden change in where beam fades out when cross disconinuities
	m_hSpotlightTarget->m_vSpotlightDir = m_hSpotlightTarget->GetLocalOrigin() - m_hSpotlightTarget->m_vSpotlightOrg;
	float flBeamLength	= VectorNormalize( m_hSpotlightTarget->m_vSpotlightDir );
	m_flSpotlightCurLength = (0.80*m_flSpotlightCurLength) + (0.2*flBeamLength);

	// Fade out spotlight end if past max length.  
	if (m_flSpotlightCurLength > 2*m_flSpotlightMaxLength)
	{
		m_hSpotlightTarget->SetRenderColorA( 0 );
		m_hSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else if (m_flSpotlightCurLength > m_flSpotlightMaxLength)		
	{
		m_hSpotlightTarget->SetRenderColorA( (1-((m_flSpotlightCurLength-m_flSpotlightMaxLength)/m_flSpotlightMaxLength)) );
		m_hSpotlight->SetFadeLength(m_flSpotlightMaxLength);
	}
	else
	{
		m_hSpotlightTarget->SetRenderColorA( 1.0 );
		m_hSpotlight->SetFadeLength(m_flSpotlightCurLength);
	}

	// Adjust end width to keep beam width constant
	float flNewWidth = SPOTLIGHT_WIDTH * ( flBeamLength/m_flSpotlightMaxLength);
	
	m_hSpotlight->SetWidth(flNewWidth);
	m_hSpotlight->SetEndWidth(flNewWidth);

	m_hSpotlightTarget->m_flLightScale = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: Called just before we are deleted.
//-----------------------------------------------------------------------------
void CNPC_CScanner::UpdateOnRemove( void )
{
	// Stop combat loops if I'm alive. If I'm dead, the die sound will already have stopped it.
	if ( IsAlive() && m_bHasSpoken )
	{
		SentenceStop();
	}
	SpotlightDestroy();
	BaseClass::UpdateOnRemove();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::TakePhoto(void)
{
	ScannerEmitSound( "TakePhoto" );
	
	m_pEyeFlash->SetScale( 1.4 );
	m_pEyeFlash->SetBrightness( 255 );
	m_pEyeFlash->SetColor(255,255,255);

	Vector vRawPos		= InspectTargetPosition();
	Vector vLightPos	= vRawPos;

	// If taking picture of entity, aim at feet
	if ( GetTarget() )
	{
		if ( GetTarget()->IsPlayer() )
		{
			m_OnPhotographPlayer.FireOutput( GetTarget(), this );
			BlindFlashTarget( GetTarget() );
		}
		
		if ( GetTarget()->MyNPCPointer() != NULL )
		{
			m_OnPhotographNPC.FireOutput( GetTarget(), this );
			GetTarget()->MyNPCPointer()->DispatchInteraction( g_interactionScannerInspectBegin, NULL, this );
		}
	}

	SetIdealActivity( (Activity) ACT_SCANNER_FLARE_START );

	m_bPhotoTaken = true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackPreFlash(void)
{
	ScannerEmitSound( "TakePhoto" );

	// If off turn on, if on turn off
	if (m_pEyeFlash->GetBrightness() == 0)
	{
		m_pEyeFlash->SetScale( 0.5 );
		m_pEyeFlash->SetBrightness( 255 );
		m_pEyeFlash->SetColor(255,0,0);
	}
	else
	{
		m_pEyeFlash->SetBrightness( 0 );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackFlash(void)
{
	ScannerEmitSound( "AttackFlash" );
	m_pEyeFlash->SetScale( 1.8 );
	m_pEyeFlash->SetBrightness( 255 );
	m_pEyeFlash->SetColor(255,255,255);

	if (GetEnemy() != NULL)
	{
		Vector pos = GetEnemyLKP();
		CBroadcastRecipientFilter filter;
		te->DynamicLight( filter, 0.0, &pos, 200, 200, 255, 0, 300, 0.2, 50 );

		if (GetEnemy()->IsPlayer())
		{
			m_OnPhotographPlayer.FireOutput(GetTarget(), this);
		}
		else if( GetEnemy()->MyNPCPointer() )
		{
			m_OnPhotographNPC.FireOutput(GetTarget(), this);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::BlindFlashTarget( CBaseEntity *pTarget )
{
	// Tell all the striders this person is here!
	CAI_BaseNPC **	ppAIs 	= g_AI_Manager.AccessAIs();
	int 			nAIs 	= g_AI_Manager.NumAIs();
	
	if( IsStriderScout() )
	{
		for ( int i = 0; i < nAIs; i++ )
		{
			if( FClassnameIs( ppAIs[ i ], "npc_strider" ) )
			{
				ppAIs[ i ]->UpdateEnemyMemory( pTarget, pTarget->GetAbsOrigin(), this );
			}
		}
	}

	// Only bother with player
	if ( pTarget->IsPlayer() == false )
		return;

	// Scale the flash value by how closely the player is looking at me
	Vector vFlashDir = GetAbsOrigin() - pTarget->EyePosition();
	VectorNormalize(vFlashDir);
	
	Vector vFacing;
	AngleVectors( pTarget->EyeAngles(), &vFacing );

	float dotPr	= DotProduct( vFlashDir, vFacing );

	// Not if behind us
	if ( dotPr > 0.5f )
	{
		// Make sure nothing in the way
		trace_t tr;
		AI_TraceLine ( GetAbsOrigin(), pTarget->EyePosition(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.startsolid == false && tr.fraction == 1.0)
		{
			color32 white = { 255, 255, 255, SCANNER_FLASH_MAX_VALUE * dotPr };
			UTIL_ScreenFade( pTarget, white, 3.0, 0.5, FFADE_IN );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackFlashBlind(void)
{
	if( GetEnemy() )
	{
		BlindFlashTarget( GetEnemy() );
	}

	m_pEyeFlash->SetBrightness( 0 );

	float fAttackDelay = random->RandomFloat(SCANNER_ATTACK_MIN_DELAY,SCANNER_ATTACK_MAX_DELAY);

	if( IsStriderScout() )
	{
		// Make strider scouts more snappy.
		fAttackDelay *= 0.5;
	}

	m_flNextAttack	= gpGlobals->curtime + fAttackDelay;
	m_fNextSpotlightTime = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::StartSmokeTrail( void )
{
	if ( m_pSmokeTrail != NULL )
		return;

	m_pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	
	if ( m_pSmokeTrail )
	{
		m_pSmokeTrail->m_SpawnRate = 10;
		m_pSmokeTrail->m_ParticleLifetime = 1;
		m_pSmokeTrail->m_StartSize		= 8;
		m_pSmokeTrail->m_EndSize		= 50;
		m_pSmokeTrail->m_SpawnRadius	= 10;
		m_pSmokeTrail->m_MinSpeed		= 15;
		m_pSmokeTrail->m_MaxSpeed		= 25;
		
		m_pSmokeTrail->m_StartColor.Init( 0.5f, 0.5f, 0.5f );
		m_pSmokeTrail->m_EndColor.Init( 0, 0, 0 );
		m_pSmokeTrail->SetLifetime( 500.0f );
		m_pSmokeTrail->FollowEntity( this );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_CScanner::AttackDivebomb( void )
{
	ScannerEmitSound( "DiveBomb" );

	if (m_hSpotlight)
	{
		SpotlightDestroy();
	}

	m_takedamage = DAMAGE_NO;

	StartSmokeTrail();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_SCANNER_GET_PATH_TO_INSPECT_TARGET:
		{
			// Must have somewhere to fly to
			if ( HaveInspectTarget() == false )
			{
				TaskFail( "No inspection target to fly to!\n" );
				return;
			}

			if ( GetTarget() )
			{	
				//FIXME: Tweak
				//Vector idealPos = IdealGoalForMovement( InspectTargetPosition(), GetAbsOrigin(), 128.0f, 128.0f );
				
				AI_NavGoal_t goal( GOALTYPE_TARGETENT, vec3_origin );
			
				if ( GetNavigator()->SetGoal( goal ) )
				{
					TaskComplete();
					return;
				}
			}
			else
			{
				AI_NavGoal_t goal( GOALTYPE_LOCATION, InspectTargetPosition() );
			
				if ( GetNavigator()->SetGoal( goal ) )
				{
					TaskComplete();
					return;
				}
			}

			// Don't try and inspect this target again for a few seconds
			CNPC_Citizen *pCitizen = dynamic_cast<CNPC_Citizen *>( GetTarget() );
			if ( pCitizen )
			{
				pCitizen->SetNextScannerInspectTime( gpGlobals->curtime + 5.0 );
			}

			TaskFail("No route to inspection target!\n");
		}
		break;

	case TASK_SCANNER_SPOT_INSPECT_ON:
	{
		if (GetTarget() == NULL)
		{
			TaskFail(FAIL_NO_TARGET);
		}
		else
		{
			CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
			if (!pNPC)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				pNPC->DispatchInteraction(g_interactionScannerInspectBegin,NULL,this);
				
				// Now we need some time to inspect
				m_fInspectEndTime = gpGlobals->curtime + SCANNER_CIT_INSPECT_LENGTH;
				TaskComplete();
			}
		}
		break;
	}
	case TASK_SCANNER_SPOT_INSPECT_WAIT:
	{
		if (GetTarget() == NULL)
		{
			TaskFail(FAIL_NO_TARGET);
		}
		else
		{
			CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
			if (!pNPC)
			{
				SetTarget( NULL );
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				//<<TEMP>>//<<TEMP>> armband too!
				pNPC->DispatchInteraction(g_interactionScannerInspectHandsUp,NULL,this);
			}
			TaskComplete();
		}
		break;
	}
	case TASK_SCANNER_SPOT_INSPECT_OFF:
	{
		if (GetTarget() == NULL)
		{
			TaskFail(FAIL_NO_TARGET);
		}
		else
		{
			CAI_BaseNPC* pNPC = GetTarget()->MyNPCPointer();
			if (!pNPC)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				pNPC->DispatchInteraction(g_interactionScannerInspectDone,NULL,this);

				// Clear target entity and don't inspect again for a while
				SetTarget( NULL );
				m_fCheckCitizenTime = gpGlobals->curtime + SCANNER_CIT_INSPECT_DELAY;
				TaskComplete();
			}
		}
		break;
	}
	case TASK_SCANNER_CLEAR_INSPECT_TARGET:
	{
		ClearInspectTarget();

		TaskComplete();
		break;
	}
	case TASK_SCANNER_SET_FLY_PHOTO:
	{
		m_nFlyMode = SCANNER_FLY_PHOTO;
		m_bPhotoTaken = false;

		// Leave spotlight off for a while
		m_fNextSpotlightTime = gpGlobals->curtime + 2.0;

		TaskComplete();
		break;
	}
	case TASK_SCANNER_SET_FLY_PATROL:
	{
		// Fly in partol mode and clear any
		// remaining target entity
		m_nFlyMode = SCANNER_FLY_PATROL;
		TaskComplete();
		break;
	}
	case TASK_SCANNER_SET_FLY_CHASE:
	{
		m_nFlyMode = SCANNER_FLY_CHASE;
		TaskComplete();
		break;
	}
	case TASK_SCANNER_SET_FLY_SPOT:
	{
		m_nFlyMode = SCANNER_FLY_SPOT;
		TaskComplete();
		break;
	}
	case TASK_SCANNER_SET_FLY_ATTACK:
	{
		m_nFlyMode = SCANNER_FLY_ATTACK;
		TaskComplete();
		break;
	}

	case TASK_SCANNER_SET_FLY_DIVE:
	{
		// Pick a direction to divebomb.
		if ( GetEnemy() != NULL )
		{
			// Fly towards my enemy
			Vector vEnemyPos = GetEnemyLKP();
			m_vecDiveBombDirection = vEnemyPos - GetLocalOrigin();
		}
		else
		{
			// Pick a random forward and down direction.
			Vector forward;
			GetVectors( &forward, NULL, NULL );
			m_vecDiveBombDirection = forward + Vector( random->RandomFloat( -10, 10 ), random->RandomFloat( -10, 10 ), random->RandomFloat( -20, -10 ) );
		}
		VectorNormalize( m_vecDiveBombDirection );

		// Calculate a roll force.
		m_flDiveBombRollForce = random->RandomFloat( 20.0, 420.0 );
		if ( random->RandomInt( 0, 1 ) )
		{
			m_flDiveBombRollForce *= -1;
		}

		DiveBombSoundThink();

		m_nFlyMode = SCANNER_FLY_DIVE;
		TaskComplete();
		break;
	}

	case TASK_SCANNER_PHOTOGRAPH:
	{
		TakePhoto();
		SetWait( 0.1 );
		break;
	}

	case TASK_SCANNER_ATTACK_PRE_FLASH:
	{
		if( IsStriderScout() )
		{
			Vector vecScare = GetEnemy()->EarPosition();
			Vector vecDir = WorldSpaceCenter() - vecScare;
			VectorNormalize( vecDir );
			vecScare += vecDir * 64.0f;

			CSoundEnt::InsertSound( SOUND_DANGER, vecScare, 256, 1.0, this );
		}

		if (m_pEyeFlash)
		{
			AttackPreFlash();
			// Flash red for a while
			SetWait( 1.0f );
		}
		else
		{
			TaskFail("No Flash");
		}
		break;
	}

	case TASK_SCANNER_ATTACK_FLASH:
	{
		AttackFlash();
		// Blinding occurs slightly later
		SetWait( 0.05 );
		break;
	}

	// Override to go to inspect target position whether or not is an entity
	case TASK_GET_PATH_TO_TARGET:
	{
		if (!HaveInspectTarget())
		{
			TaskFail(FAIL_NO_TARGET);
		}
		else if (GetHintNode())
		{
			Vector vNodePos;
			GetHintNode()->GetPosition(this,&vNodePos);

			GetNavigator()->SetGoal( vNodePos );
		}
		else 
		{
			AI_NavGoal_t goal( (const Vector &)InspectTargetPosition() );
			goal.pTarget = GetTarget();
			GetNavigator()->SetGoal( goal );
		}
		break;
	}
	default:
		BaseClass::StartTask(pTask);
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CScanner::OnScheduleChange( void )
{
	m_flSpeed = GetMaxSpeed();

	BaseClass::OnScheduleChange();
}


//-----------------------------------------------------------------------------
// Purpose: Overridden so that scanners play battle sounds while fighting.
// Output : Returns TRUE on success, FALSE on failure.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::ShouldPlayIdleSound( void )
{
	if ( HasSpawnFlags( SF_NPC_GAG ) )
		return false;

	if ( random->RandomInt( 0, 25 ) != 0 )
		return false;

	return true;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_CScanner::ScannerEmitSound( const char *pszSoundName )
{
	CFmtStr snd;
	
	const char szCityScanner[] = "NPC_CScanner";
	const char szShieldScanner[] = "NPC_SScanner";

	if( m_bIsClawScanner )
	{
		snd.sprintf("%s.%s", szShieldScanner, pszSoundName );
	}
	else
	{
		snd.sprintf("%s.%s", szCityScanner, pszSoundName );
	}

	m_bHasSpoken = true;

	EmitSound( snd.Access() );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
float CNPC_CScanner::MinGroundDist( void )
{
	if ( m_nFlyMode == SCANNER_FLY_SPOT && !GetHintNode() )
	{
		return SCANNER_SPOTLIGHT_FLY_HEIGHT;
	}

	return SCANNER_NOSPOTLIGHT_FLY_HEIGHT;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::BlendPhyscannonLaunchSpeed()
{
	// Blend out desired velocity when launched by the physcannon
	if (!VPhysicsGetObject())
		return;

	if ( HasPhysicsAttacker( SCANNER_SMASH_TIME ) && !IsHeldByPhyscannon( ) )
	{
		Vector vecCurrentVelocity;
		VPhysicsGetObject()->GetVelocity( &vecCurrentVelocity, NULL );
		float flLerpFactor = (gpGlobals->curtime - m_flLastPhysicsInfluenceTime) / SCANNER_SMASH_TIME;
		flLerpFactor = clamp( flLerpFactor, 0.0f, 1.0f );
		flLerpFactor = SimpleSplineRemapVal( flLerpFactor, 0.0f, 1.0f, 0.0f, 1.0f );
		flLerpFactor *= flLerpFactor;
		VectorLerp( vecCurrentVelocity, m_vCurrentVelocity, flLerpFactor, m_vCurrentVelocity );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveExecute_Alive(float flInterval)
{
	// Amount of noise to add to flying
	float noiseScale = 3.0f;

	// -------------------------------------------
	//  Avoid obstacles, unless I'm dive bombing
	// -------------------------------------------
	if (m_nFlyMode != SCANNER_FLY_DIVE)
	{
		SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );
	}
	// If I am dive bombing add more noise to my flying
	else
	{
		AttackDivebombCollide(flInterval);
		noiseScale *= 4;
	}

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	if ( pPhysics && pPhysics->IsAsleep() )
	{
		pPhysics->Wake();
	}

	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	AddNoiseToVelocity( noiseScale );

	if ( m_bIsClawScanner )
	{
		m_vCurrentVelocity *= ( 1 + sin( ( gpGlobals->curtime + m_flFlyNoiseBase ) * 2.5f ) * .1 );
	}

	float maxSpeed = GetEnemy() ? ( GetMaxSpeed() * 2.0f ) : GetMaxSpeed();
	if ( m_nFlyMode == SCANNER_FLY_DIVE )
	{
		maxSpeed = -1;
	}

	// Limit fall speed
	LimitSpeed( maxSpeed );

	// Blend out desired velocity when launched by the physcannon
	BlendPhyscannonLaunchSpeed();

	// Update what we're looking at
	UpdateHead( flInterval );

	// Control the tail based on our vertical travel
	float tailPerc = clamp( GetCurrentVelocity().z, -150, 250 );
	tailPerc = SimpleSplineRemapVal( tailPerc, -150, 250, -25, 80 );

	SetPoseParameter( m_nPoseTail, tailPerc );

	// Spin the dynamo based upon our speed
	float flCurrentDynamo = GetPoseParameter( m_nPoseDynamo );
	float speed	= GetCurrentVelocity().Length();
	float flDynamoSpeed = (maxSpeed > 0 ? speed / maxSpeed : 1.0) * 60;
	flCurrentDynamo -= flDynamoSpeed;
	if ( flCurrentDynamo < -180.0 )
	{
		flCurrentDynamo += 360.0;
	}
	SetPoseParameter( m_nPoseDynamo, flCurrentDynamo );

	PlayFlySound();
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_CScanner::OverridePathMove( CBaseEntity *pMoveTarget, float flInterval )
{
	// Save our last patrolling direction
	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetAbsOrigin();
	
	// Continue on our path
	if ( ProgressFlyPath( flInterval, pMoveTarget, (MASK_NPCSOLID|CONTENTS_WATER), false, 64 ) == AINPP_COMPLETE )
	{
		if ( IsCurSchedule( SCHED_SCANNER_PATROL ) )
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::OverrideMove( float flInterval )
{
	// ----------------------------------------------
	//	If dive bombing
	// ----------------------------------------------
	if (m_nFlyMode == SCANNER_FLY_DIVE)
	{
		MoveToDivebomb( flInterval );
	}
	else
	{
		Vector vMoveTargetPos(0,0,0);
		CBaseEntity *pMoveTarget = NULL;
		
		if ( !GetNavigator()->IsGoalActive() || ( GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER ) )
		{
			// Select move target 
			if ( GetTarget() != NULL )
			{
				pMoveTarget = GetTarget();
			}
			else if ( GetEnemy() != NULL )
			{
				pMoveTarget = GetEnemy();
			}
			
			// Select move target position 
			if ( HaveInspectTarget() )
			{
				vMoveTargetPos = InspectTargetPosition(); 
			}
			else if ( GetEnemy() != NULL )
			{
				vMoveTargetPos = GetEnemy()->GetAbsOrigin();
			}
		}
		else
		{
			vMoveTargetPos = GetNavigator()->GetCurWaypointPos();
		}

		ClearCondition( COND_SCANNER_FLY_CLEAR );
		ClearCondition( COND_SCANNER_FLY_BLOCKED );

		// See if we can fly there directly
		if ( pMoveTarget || HaveInspectTarget() )
		{
			trace_t tr;
			AI_TraceHull( GetAbsOrigin(), vMoveTargetPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

			float fTargetDist = (1.0f-tr.fraction)*(GetAbsOrigin() - vMoveTargetPos).Length();
			
			if ( ( tr.m_pEnt == pMoveTarget ) || ( fTargetDist < 50 ) )
			{
				if ( g_debug_cscanner.GetBool() )
				{
					NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 0,255,0, true, 0);
					NDebugOverlay::Cross3D(tr.endpos,Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
				}

				SetCondition( COND_SCANNER_FLY_CLEAR );
			}
			else		
			{
				//HANDY DEBUG TOOL	
				if ( g_debug_cscanner.GetBool() )
				{
					NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 255,0,0, true, 0);
					NDebugOverlay::Cross3D(tr.endpos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
				}

				SetCondition( COND_SCANNER_FLY_BLOCKED );
			}
		}

		// If I have a route, keep it updated and move toward target
		if ( GetNavigator()->IsGoalActive() )
		{
			if ( OverridePathMove( pMoveTarget, flInterval ) )
			{
				BlendPhyscannonLaunchSpeed();
				return true;
			}
		}	
		else if (m_nFlyMode == SCANNER_FLY_SPOT)
		{
			MoveToSpotlight( flInterval );
		}
		// If photographing
		else if ( m_nFlyMode == SCANNER_FLY_PHOTO )
		{
			MoveToPhotograph( flInterval );
		}
		else if ( m_nFlyMode == SCANNER_FLY_FOLLOW )
		{
			MoveToSpotlight( flInterval );
		}
		// ----------------------------------------------
		//	If attacking
		// ----------------------------------------------
		else if (m_nFlyMode == SCANNER_FLY_ATTACK)
		{
			if ( m_hSpotlight )
			{
				SpotlightDestroy();
			}
			
			MoveToAttack( flInterval );
		}
		// -----------------------------------------------------------------
		// If I don't have a route, just decelerate
		// -----------------------------------------------------------------
		else if (!GetNavigator()->IsGoalActive())
		{
			float	myDecay	 = 9.5;
			Decelerate( flInterval, myDecay);
		}
	}
		
	MoveExecute_Alive( flInterval );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Accelerates toward a given position.
// Input  : flInterval - Time interval over which to move.
//			vecMoveTarget - Position to move toward.
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToTarget( float flInterval, const Vector &vecMoveTarget )
{
	// Don't move if stalling
	if ( m_flEngineStallTime > gpGlobals->curtime )
		return;
	
	// Look at our inspection target if we have one
	if ( GetEnemy() != NULL )
	{
		// Otherwise at our enemy
		TurnHeadToTarget( flInterval, GetEnemy()->EyePosition() );
	}
	else if ( HaveInspectTarget() )
	{
		TurnHeadToTarget( flInterval, InspectTargetPosition() );
	}
	else
	{
		// Otherwise face our motion direction
		TurnHeadToTarget( flInterval, vecMoveTarget );
	}

	// -------------------------------------
	// Move towards our target
	// -------------------------------------
	float myAccel;
	float myZAccel = 400.0f;
	float myDecay  = 0.15f;

	Vector vecCurrentDir;

	// Get the relationship between my current velocity and the way I want to be going.
	vecCurrentDir = GetCurrentVelocity();
	VectorNormalize( vecCurrentDir );

	Vector targetDir = vecMoveTarget - GetAbsOrigin();
	float flDist = VectorNormalize(targetDir);

	float flDot;
	flDot = DotProduct( targetDir, vecCurrentDir );

	if( flDot > 0.25 )
	{
		// If my target is in front of me, my flight model is a bit more accurate.
		myAccel = 250;
	}
	else
	{
		// Have a harder time correcting my course if I'm currently flying away from my target.
		myAccel = 128;
	}

	if ( myAccel > flDist / flInterval )
	{
		myAccel = flDist / flInterval;
	}

	if ( myZAccel > flDist / flInterval )
	{
		myZAccel = flDist / flInterval;
	}

	MoveInDirection( flInterval, targetDir, myAccel, myZAccel, myDecay );

	// calc relative banking targets
	Vector forward, right, up;
	GetVectors( &forward, &right, &up );

	m_vCurrentBanking.x	= targetDir.x;
	m_vCurrentBanking.z	= 120.0f * DotProduct( right, targetDir );
	m_vCurrentBanking.y	= 0;

	float speedPerc = SimpleSplineRemapVal( GetCurrentVelocity().Length(), 0.0f, GetMaxSpeed(), 0.0f, 1.0f );

	speedPerc = clamp( speedPerc, 0.0f, 1.0f );

	m_vCurrentBanking *= speedPerc;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToSpotlight( float flInterval )
{
	if ( flInterval <= 0 )
		return;

	Vector vTargetPos;

	if ( HaveInspectTarget() )
	{
		vTargetPos = InspectTargetPosition();
	}
	else if ( GetEnemy() != NULL )
	{
		vTargetPos = GetEnemyLKP();
	}
	else
	{
		return;
	}

	//float flDesiredDist = SCANNER_SPOTLIGHT_NEAR_DIST + ( ( SCANNER_SPOTLIGHT_FAR_DIST - SCANNER_SPOTLIGHT_NEAR_DIST ) / 2 );
	
	float flIdealHeightDiff = SCANNER_SPOTLIGHT_NEAR_DIST;
	if( IsEnemyPlayerInSuit() )
	{
		flIdealHeightDiff *= 0.5;
	}

	Vector idealPos = IdealGoalForMovement( vTargetPos, GetAbsOrigin(), GetGoalDistance(), flIdealHeightDiff );

	MoveToTarget( flInterval, idealPos );

	//TODO: Re-implement?

	/*
	// ------------------------------------------------
	//  Also keep my distance from other squad members
	//  unless I'm inspecting
	// ------------------------------------------------
	if (m_pSquad &&
		gpGlobals->curtime > m_fInspectEndTime)
	{
		CBaseEntity*	pNearest	= m_pSquad->NearestSquadMember(this);
		if (pNearest)
		{
			Vector			vNearestDir = (pNearest->GetLocalOrigin() - GetLocalOrigin());
			if (vNearestDir.Length() < SCANNER_SQUAD_FLY_DIST) 
			{
				vNearestDir		= pNearest->GetLocalOrigin() - GetLocalOrigin();
				VectorNormalize(vNearestDir);
				vFlyDirection  -= 0.5*vNearestDir;
			}
		}
	}

	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
		vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToAttack(float flInterval)
{
	if (GetEnemy() == NULL)
		return;

	if ( flInterval <= 0 )
		return;

	Vector vTargetPos = GetEnemyLKP();

	//float flDesiredDist = SCANNER_ATTACK_NEAR_DIST + ( ( SCANNER_ATTACK_FAR_DIST - SCANNER_ATTACK_NEAR_DIST ) / 2 );
	
	Vector idealPos = IdealGoalForMovement( vTargetPos, GetAbsOrigin(), GetGoalDistance(), SCANNER_ATTACK_NEAR_DIST );

	MoveToTarget( flInterval, idealPos );

	//FIXME: Re-implement?

	/*
	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
		vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}
	*/
}


//-----------------------------------------------------------------------------
// Danger sounds. 
//-----------------------------------------------------------------------------
void CNPC_CScanner::DiveBombSoundThink()
{
	Vector vecPosition, vecVelocity;
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if ( pPhysicsObject == NULL )
		return;

	pPhysicsObject->GetPosition( &vecPosition, NULL );
	pPhysicsObject->GetVelocity( &vecVelocity, NULL );
	
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer )
	{
		Vector vecDelta;
		VectorSubtract( pPlayer->GetAbsOrigin(), vecPosition, vecDelta );
		VectorNormalize( vecDelta );
		if ( DotProduct( vecDelta, vecVelocity ) > 0.5f )
		{
			Vector vecEndPoint;
			VectorMA( vecPosition, 2.0f * TICK_INTERVAL, vecVelocity, vecEndPoint );
			float flDist = CalcDistanceToLineSegment( pPlayer->GetAbsOrigin(), vecPosition, vecEndPoint );
			if ( flDist < 200.0f )
			{
				ScannerEmitSound( "DiveBombFlyby" );
				SetContextThink( &CNPC_CScanner::DiveBombSoundThink, gpGlobals->curtime + 0.5f, s_pDiveBombSoundThinkContext );
				return;
			}
		}
	}

	SetContextThink( &CNPC_CScanner::DiveBombSoundThink, gpGlobals->curtime + 2.0f * TICK_INTERVAL, s_pDiveBombSoundThinkContext );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToDivebomb(float flInterval)
{
	float myAccel = 1600;
	float myDecay = 0.05f; // decay current velocity to 10% in 1 second

	// Fly towards my enemy
	Vector vEnemyPos = GetEnemyLKP();
	Vector vFlyDirection  = vEnemyPos - GetLocalOrigin();
	VectorNormalize( vFlyDirection );

	// Set net velocity 
	MoveInDirection( flInterval, m_vecDiveBombDirection, myAccel, myAccel, myDecay);

	// Spin out of control.
	Vector forward;
	VPhysicsGetObject()->LocalToWorldVector( &forward, Vector( 1.0, 0.0, 0.0 ) );
	AngularImpulse torque = forward * m_flDiveBombRollForce;
	VPhysicsGetObject()->ApplyTorqueCenter( torque );

	// BUGBUG: why Y axis and not Z?
	Vector up;
	VPhysicsGetObject()->LocalToWorldVector( &up, Vector( 0.0, 1.0, 0.0 ) );
	VPhysicsGetObject()->ApplyForceCenter( up * 2000 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CScanner::IsEnemyPlayerInSuit()
{
	if( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		CHL2_Player *pPlayer = NULL;
		pPlayer = (CHL2_Player *)GetEnemy();

		if( pPlayer && pPlayer->IsSuitEquipped() )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CScanner::GetGoalDistance( void )
{
	if ( m_flGoalOverrideDistance != 0.0f )
		return m_flGoalOverrideDistance;

	switch ( m_nFlyMode )
	{
	case SCANNER_FLY_PHOTO:
		return ( SCANNER_PHOTO_NEAR_DIST + ( ( SCANNER_PHOTO_FAR_DIST - SCANNER_PHOTO_NEAR_DIST ) / 2 ) );
		break;

	case SCANNER_FLY_SPOT:
		{
			float goalDist = ( SCANNER_SPOTLIGHT_NEAR_DIST + ( ( SCANNER_SPOTLIGHT_FAR_DIST - SCANNER_SPOTLIGHT_NEAR_DIST ) / 2 ) );
			if( IsEnemyPlayerInSuit() )
			{
				goalDist *= 0.5;
			}
			return goalDist;
		}
		break;
	
	case SCANNER_FLY_ATTACK:
		{
			float goalDist = ( SCANNER_ATTACK_NEAR_DIST + ( ( SCANNER_ATTACK_FAR_DIST - SCANNER_ATTACK_NEAR_DIST ) / 2 ) );
			if( IsEnemyPlayerInSuit() )
			{
				goalDist *= 0.5;
			}
			return goalDist;
		}
		break;

	case SCANNER_FLY_FOLLOW:
		return ( SCANNER_FOLLOW_DIST );
		break;
	}

	return 128.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vOut - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CScanner::GetGoalDirection( Vector *vOut )
{
	CBaseEntity *pTarget = GetTarget();

	if ( pTarget == NULL )
		return false;

	if ( FClassnameIs( pTarget, "info_hint_air" ) || FClassnameIs( pTarget, "info_target" ) )
	{
		AngleVectors( pTarget->GetAbsAngles(), vOut );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &goalPos - 
//			&startPos - 
//			idealRange - 
//			idealHeight - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CNPC_CScanner::IdealGoalForMovement( const Vector &goalPos, const Vector &startPos, float idealRange, float idealHeightDiff )
{
	Vector	vMoveDir;
	
	if ( GetGoalDirection( &vMoveDir ) == false )
	{
		vMoveDir = ( goalPos - startPos );
		vMoveDir.z = 0;
		VectorNormalize( vMoveDir );
	}

	// Move up from the position by the desired amount
	Vector vIdealPos = goalPos + Vector( 0, 0, idealHeightDiff ) + ( vMoveDir * -idealRange );

	// Trace down and make sure we can fit here
	trace_t	tr;
	AI_TraceHull( vIdealPos, vIdealPos - Vector( 0, 0, MinGroundDist() ), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// Move up otherwise
	if ( tr.fraction < 1.0f )
	{
		vIdealPos.z += ( MinGroundDist() * ( 1.0f - tr.fraction ) );
	}

	//FIXME: We also need to make sure that we fit here at all, and if not, chose a new spot

	// Debug tools
	if ( g_debug_cscanner.GetBool() )
	{
		NDebugOverlay::Cross3D( goalPos, -Vector(8,8,8), Vector(8,8,8), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( startPos, -Vector(8,8,8), Vector(8,8,8), 255, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( vIdealPos, -Vector(8,8,8), Vector(8,8,8), 255, 255, 255, true, 0.1f );
		NDebugOverlay::Line( startPos, goalPos, 0, 255, 0, true, 0.1f );

		NDebugOverlay::Cross3D( goalPos + ( vMoveDir * -idealRange ), -Vector(8,8,8), Vector(8,8,8), 255, 255, 255, true, 0.1f );
		NDebugOverlay::Line( goalPos, goalPos + ( vMoveDir * -idealRange ), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Line( goalPos + ( vMoveDir * -idealRange ), vIdealPos, 255, 255, 0, true, 0.1f );
	}

	return vIdealPos;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_CScanner::MoveToPhotograph(float flInterval)
{
	if ( HaveInspectTarget() == false )
		return;

	//float flDesiredDist = SCANNER_PHOTO_NEAR_DIST + ( ( SCANNER_PHOTO_FAR_DIST - SCANNER_PHOTO_NEAR_DIST ) / 2 );
	
	Vector idealPos = IdealGoalForMovement( InspectTargetPosition(), GetAbsOrigin(), GetGoalDistance(), 32.0f );

	MoveToTarget( flInterval, idealPos );

	//FIXME: Re-implement?

	/*
	// ------------------------------------------------
	//  Also keep my distance from other squad members
	//  unless I'm inspecting
	// ------------------------------------------------
	if (m_pSquad &&
		gpGlobals->curtime > m_fInspectEndTime)
	{
		CBaseEntity*	pNearest	= m_pSquad->NearestSquadMember(this);
		if (pNearest)
		{
			Vector			vNearestDir = (pNearest->GetLocalOrigin() - GetLocalOrigin());
			if (vNearestDir.Length() < SCANNER_SQUAD_FLY_DIST) 
			{
				vNearestDir		= pNearest->GetLocalOrigin() - GetLocalOrigin();
				VectorNormalize(vNearestDir);
				vFlyDirection  -= 0.5*vNearestDir;
			}
		}
	}
	*/
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CNPC_CScanner::VelocityToEvade(CBaseCombatCharacter *pEnemy)
{
	if (pEnemy)
	{
		// -----------------------------------------
		//  Keep out of enemy's shooting position
		// -----------------------------------------
		Vector vEnemyFacing = pEnemy->BodyDirection2D( );
		Vector	vEnemyDir   = pEnemy->EyePosition() - GetLocalOrigin();
		VectorNormalize(vEnemyDir);
		float  fDotPr		= DotProduct(vEnemyFacing,vEnemyDir);

		if (fDotPr < -0.9)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (crossProduct.y < 0)
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
		else if (fDotPr < -0.85)
		{
			Vector vDirUp(0,0,1);
			Vector vDir;
			CrossProduct( vEnemyFacing, vDirUp, vDir);

			Vector crossProduct;
			CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
			if (random->RandomInt(0,1))
			{
				vDir = vDir * -1;
			}
			return (vDir);
		}
	}
	return vec3_origin;
}


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_CScanner::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* pSourceEnt)
{
	//	TODO:: - doing this by just an interrupt contition would be a lot better!
	if (interactionType ==	g_interactionScannerSupportEntity)
	{
		// Only accept help request if I'm not already busy
		if (GetEnemy() == NULL && !HaveInspectTarget())
		{
			// Only accept if target is a reasonable distance away
			CBaseEntity* pTarget = (CBaseEntity*)data;
			float fTargetDist = (pTarget->GetLocalOrigin() - GetLocalOrigin()).Length();

			if (fTargetDist < SCANNER_SQUAD_HELP_DIST)
			{
				float fInspectTime = (((CNPC_CScanner*)pSourceEnt)->m_fInspectEndTime - gpGlobals->curtime);
				SetInspectTargetToEnt(pTarget,fInspectTime);

				if (random->RandomInt(0,2)==0)
				{
					SetSchedule(SCHED_SCANNER_PHOTOGRAPH_HOVER);
				}
				else
				{
					SetSchedule(SCHED_SCANNER_SPOTLIGHT_HOVER);
				}
				return true;
			}
		}
	}
	else if (interactionType ==	g_interactionScannerSupportPosition)
	{
		// Only accept help request if I'm not already busy
		if (GetEnemy() == NULL && !HaveInspectTarget())
		{
			// Only accept if target is a reasonable distance away
			Vector vInspectPos;
			vInspectPos.x = ((Vector *)data)->x;
			vInspectPos.y = ((Vector *)data)->y;
			vInspectPos.z = ((Vector *)data)->z;

			float fTargetDist = (vInspectPos - GetLocalOrigin()).Length();

			if (fTargetDist < SCANNER_SQUAD_HELP_DIST)
			{
				float fInspectTime = (((CNPC_CScanner*)pSourceEnt)->m_fInspectEndTime - gpGlobals->curtime);
				SetInspectTargetToPos(vInspectPos,fInspectTime);

				if (random->RandomInt(0,2)==0)
				{
					SetSchedule(SCHED_SCANNER_PHOTOGRAPH_HOVER);
				}
				else
				{
					SetSchedule(SCHED_SCANNER_SPOTLIGHT_HOVER);
				}
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputDisableSpotlight( inputdata_t &inputdata )
{
	m_bNoLight = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CScanner::DrawDebugTextOverlays(void)
{
	int nOffset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT ) 
	{
		Vector vel;
		GetVelocity( &vel, NULL );

		char tempstr[512];
		Q_snprintf( tempstr, sizeof(tempstr), "speed (max): %.2f (%.2f)", vel.Length(), m_flSpeed );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;
	}

	return nOffset;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CNPC_CScanner::GetHeadTurnRate( void ) 
{ 
	if ( GetEnemy() )
		return 800.0f;

	if ( HaveInspectTarget() )
		return 500.0f;

	return 350.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline CBaseEntity *CNPC_CScanner::EntityToWatch( void )
{
	return ( GetTarget() != NULL ) ? GetTarget() : GetEnemy();	// Okay if NULL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::UpdateHead( float flInterval )
{
	float yaw = GetPoseParameter( m_nPoseFaceHoriz );
	float pitch = GetPoseParameter( m_nPoseFaceVert );

	CBaseEntity *pTarget = EntityToWatch();

	Vector	vLookPos;

	if ( !HasCondition( COND_IN_PVS ) || GetAttachment( "eyes", vLookPos ) == false )
	{
		vLookPos = EyePosition();
	}

	if ( pTarget != NULL )
	{
		Vector	lookDir = pTarget->EyePosition() - vLookPos;
		VectorNormalize( lookDir );
		
		if ( DotProduct( lookDir, BodyDirection3D() ) < 0.0f )
		{
			SetPoseParameter( m_nPoseFaceHoriz,	UTIL_Approach( 0, yaw, 10 ) );
			SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( 0, pitch, 10 ) );
			
			return;
		}

		float facingYaw = VecToYaw( BodyDirection3D() );
		float yawDiff = VecToYaw( lookDir );
		yawDiff = UTIL_AngleDiff( yawDiff, facingYaw + yaw );

		float facingPitch = UTIL_VecToPitch( BodyDirection3D() );
		float pitchDiff = UTIL_VecToPitch( lookDir );
		pitchDiff = UTIL_AngleDiff( pitchDiff, facingPitch + pitch );

		SetPoseParameter( m_nPoseFaceHoriz, UTIL_Approach( yaw + yawDiff, yaw, 50 ) );
		SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( pitch + pitchDiff, pitch, 50 ) );
	}
	else
	{
		SetPoseParameter( m_nPoseFaceHoriz,	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( m_nPoseFaceVert, UTIL_Approach( 0, pitch, 10 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &linear - 
//			&angular - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::ClampMotorForces( Vector &linear, AngularImpulse &angular )
{ 
	// limit reaction forces
	if ( m_nFlyMode != SCANNER_FLY_DIVE )
	{
		linear.x = clamp( linear.x, -500, 500 );
		linear.y = clamp( linear.y, -500, 500 );
		linear.z = clamp( linear.z, -500, 500 );
	}

	// If we're dive bombing, we need to drop faster than normal
	if ( m_nFlyMode != SCANNER_FLY_DIVE )
	{
		// Add in weightlessness
		linear.z += 800;
	}

	angular.z = clamp( angular.z, -GetHeadTurnRate(), GetHeadTurnRate() );
	if ( m_nFlyMode == SCANNER_FLY_DIVE )
	{
		// Disable pitch and roll motors while crashing.
		angular.x = 0;
		angular.y = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputSetFollowTarget( inputdata_t &inputdata )
{
	InspectTarget( inputdata, SCANNER_FLY_FOLLOW );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputClearFollowTarget( inputdata_t &inputdata )
{
	SetInspectTargetToEnt( NULL, 0 );
	
	m_nFlyMode = SCANNER_FLY_PATROL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_CScanner::InputSetDistanceOverride( inputdata_t &inputdata )
{
	m_flGoalOverrideDistance = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_cscanner, CNPC_CScanner )

	DECLARE_TASK(TASK_SCANNER_SET_FLY_PHOTO)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_PATROL)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_CHASE)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_SPOT)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_ATTACK)
	DECLARE_TASK(TASK_SCANNER_SET_FLY_DIVE)
	DECLARE_TASK(TASK_SCANNER_PHOTOGRAPH)
	DECLARE_TASK(TASK_SCANNER_ATTACK_PRE_FLASH)
	DECLARE_TASK(TASK_SCANNER_ATTACK_FLASH)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_ON)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_WAIT)
	DECLARE_TASK(TASK_SCANNER_SPOT_INSPECT_OFF)
	DECLARE_TASK(TASK_SCANNER_CLEAR_INSPECT_TARGET)
	DECLARE_TASK(TASK_SCANNER_GET_PATH_TO_INSPECT_TARGET)

	DECLARE_CONDITION(COND_SCANNER_FLY_CLEAR)
	DECLARE_CONDITION(COND_SCANNER_FLY_BLOCKED)
	DECLARE_CONDITION(COND_SCANNER_HAVE_INSPECT_TARGET)
	DECLARE_CONDITION(COND_SCANNER_INSPECT_DONE)
	DECLARE_CONDITION(COND_SCANNER_CAN_PHOTOGRAPH)
	DECLARE_CONDITION(COND_SCANNER_SPOT_ON_TARGET)
	DECLARE_CONDITION(COND_SCANNER_RELEASED_FROM_PHYSCANNON)
	DECLARE_CONDITION(COND_SCANNER_GRABBED_BY_PHYSCANNON)

	DECLARE_ACTIVITY(ACT_SCANNER_SMALL_FLINCH_ALERT)
	DECLARE_ACTIVITY(ACT_SCANNER_SMALL_FLINCH_COMBAT)
	DECLARE_ACTIVITY(ACT_SCANNER_INSPECT)
	DECLARE_ACTIVITY(ACT_SCANNER_WALK_ALERT)
	DECLARE_ACTIVITY(ACT_SCANNER_WALK_COMBAT)
	DECLARE_ACTIVITY(ACT_SCANNER_FLARE)
	DECLARE_ACTIVITY(ACT_SCANNER_RETRACT)
	DECLARE_ACTIVITY(ACT_SCANNER_FLARE_PRONGS)
	DECLARE_ACTIVITY(ACT_SCANNER_RETRACT_PRONGS)
	DECLARE_ACTIVITY(ACT_SCANNER_FLARE_START)

	DECLARE_ANIMEVENT( AE_SCANNER_CLOSED )

	DECLARE_INTERACTION(g_interactionScannerInspect)
	DECLARE_INTERACTION(g_interactionScannerInspectBegin)
	DECLARE_INTERACTION(g_interactionScannerInspectDone)
	DECLARE_INTERACTION(g_interactionScannerInspectHandsUp)
	DECLARE_INTERACTION(g_interactionScannerInspectShowArmband)
	DECLARE_INTERACTION(g_interactionScannerSupportEntity)
	DECLARE_INTERACTION(g_interactionScannerSupportPosition)

	//=========================================================
	// > SCHED_SCANNER_PATROL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PATROL,

		"	Tasks"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		"		TASK_SCANNER_SET_FLY_PATROL			0"
		"		TASK_SET_TOLERANCE_DISTANCE			32"
		"		TASK_SET_ROUTE_SEARCH_TIME			5"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_GET_PATH_TO_RANDOM_NODE		2000"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_PLAYER"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_SCANNER_HAVE_INSPECT_TARGET"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_HOVER
	//
	// Hover above target entity, trying to get spotlight
	// on my target
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_WALK  "
		"		TASK_WAIT							1"
		""
		"	Interrupts"
		"		COND_SCANNER_SPOT_ON_TARGET"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_INSPECT_POS
	//
	// Inspect a position once spotlight is on it
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_INSPECT_POS,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_SCANNER_INSPECT"
		"		TASK_SPEAK_SENTENCE					3"	// Curious sound
		"		TASK_WAIT							5"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT
	//
	// Inspect a citizen once spotlight is on it
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_SPOTLIGHT_INSPECT_CIT,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_SPOT			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_SCANNER_INSPECT"
		"		TASK_SPEAK_SENTENCE					0"	// Stop!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_ON		0"
		"		TASK_WAIT							2"
		"		TASK_SPEAK_SENTENCE					1"	// Hands on head or Show Armband!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_WAIT		0"
		"		TASK_WAIT							5"
		"		TASK_SPEAK_SENTENCE					2"	// Free to go!
		"		TASK_WAIT							1"
		"		TASK_SCANNER_SPOT_INSPECT_OFF		0"
		"		TASK_SCANNER_CLEAR_INSPECT_TARGET	0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_PHOTOGRAPH_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PHOTOGRAPH_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_PHOTO			0"
		"		TASK_WAIT							2"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_SCANNER_CAN_PHOTOGRAPH"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_HOVER,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_PHOTOGRAPH
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_PHOTOGRAPH,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_PHOTO			0"
		"		TASK_SCANNER_PHOTOGRAPH				0"
		""
		"	Interrupts"
		"		COND_SCANNER_INSPECT_DONE"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_FLASH
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_FLASH,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_SCANNER_ATTACK_PRE_FLASH		0 "
		"		TASK_SCANNER_ATTACK_FLASH			0"
		"		TASK_WAIT							0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_ATTACK_DIVEBOMB
	//
	// Only done when scanner is dead
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_ATTACK_DIVEBOMB,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_DIVE			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							10"
		""
		"	Interrupts"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_ENEMY
	//
	//  Different interrupts than normal chase enemy.  
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_ENEMY,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SCANNER_PATROL"
		"		 TASK_SET_TOLERANCE_DISTANCE		120"
		"		 TASK_GET_PATH_TO_ENEMY				0"
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_CHASE_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_CHASE_TARGET,

		"	Tasks"
		"		 TASK_SCANNER_SET_FLY_CHASE			0"
		"		 TASK_SET_TOLERANCE_DISTANCE		64"
		"		 TASK_GET_PATH_TO_TARGET			0"	//FIXME: This is wrong!
		"		 TASK_RUN_PATH						0"
		"		 TASK_WAIT_FOR_MOVEMENT				0"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_MOVE_TO_INSPECT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_MOVE_TO_INSPECT,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_SCANNER_PATROL"
		"		 TASK_SET_TOLERANCE_DISTANCE				128"
		"		 TASK_SCANNER_GET_PATH_TO_INSPECT_TARGET	0"
		"		 TASK_RUN_PATH								0"
		"		 TASK_WAIT_FOR_MOVEMENT						0"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_CLEAR"
		"		COND_NEW_ENEMY"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)
	
	//=========================================================
	// > SCHED_SCANNER_FOLLOW_HOVER
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_FOLLOW_HOVER,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							0.1"
		""
		"	Interrupts"
		"		COND_SCANNER_FLY_BLOCKED"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

	//=========================================================
	// > SCHED_SCANNER_HELD_BY_PHYSCANNON
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SCANNER_HELD_BY_PHYSCANNON,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							5.0"
		""
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SCANNER_RELEASED_FROM_PHYSCANNON"
	)

	
AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
// Claw Scanner
//
// Scanner that always spawns as a claw scanner
//-----------------------------------------------------------------------------
	
class CNPC_ClawScanner : public CNPC_CScanner
{
DECLARE_CLASS( CNPC_ClawScanner, CNPC_CScanner );

public:
	CNPC_ClawScanner();
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CNPC_ClawScanner )
END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_clawscanner, CNPC_ClawScanner);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_ClawScanner::CNPC_ClawScanner()
{
	// override our superclass's setting
	BecomeClawScanner();
}
