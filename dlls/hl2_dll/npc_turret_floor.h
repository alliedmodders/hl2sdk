#ifndef NPC_TURRET_FLOOR_H
#define NPC_TURRET_FLOOR_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "player_pickup.h"
#include "Sprite.h"

//Turret states
enum turretState_e
{
	TURRET_SEARCHING,
	TURRET_AUTO_SEARCHING,
	TURRET_ACTIVE,
	TURRET_SUPPRESSING,
	TURRET_DEPLOYING,
	TURRET_RETIRING,
};

//Eye states
enum eyeState_t
{
	TURRET_EYE_SEE_TARGET,			//Sees the target, bright and big
	TURRET_EYE_SEEKING_TARGET,		//Looking for a target, blinking (bright)
	TURRET_EYE_DORMANT,				//Not active
	TURRET_EYE_DEAD,				//Completely invisible
	TURRET_EYE_DISABLED,			//Turned off, must be reactivated before it'll deploy again (completely invisible)
};

class CTurretTipController;

//-----------------------------------------------------------------------------
// Purpose: Floor turret
//-----------------------------------------------------------------------------
class CNPC_FloorTurret : public CNPCBaseInteractive<CAI_BaseNPC>, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CNPC_FloorTurret, CNPCBaseInteractive<CAI_BaseNPC> );
public:

	CNPC_FloorTurret( void );
	~CNPC_FloorTurret( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	void	Activate( void );
	bool	CreateVPhysics( void );
	void	UpdateOnRemove( void );
	virtual int	OnTakeDamage( const CTakeDamageInfo &info );
	int		VPhysicsTakeDamage( const CTakeDamageInfo &info );

	// Player pickup
	void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void	OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );
	bool	HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer );
	const QAngle	&PreferredCarryAngles( void ) {return vec3_angle; }
	bool	OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );

	const char *GetTracerType( void ) { return "AR2Tracer"; }

	bool	ShouldSavePhysics() { return true; }

	bool	HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sourceEnt );

	// Think functions
	virtual void	Retire( void );
	virtual void	Deploy( void );
	virtual void	ActiveThink( void );
	virtual void	SearchThink( void );
	virtual void	AutoSearchThink( void );
	virtual void	TippedThink( void );
	virtual void	InactiveThink( void );
	virtual void	SuppressThink( void );
	virtual void	DisabledThink( void );
	void			HackFindEnemy( void );

	virtual float	GetAttackDamageScale( CBaseEntity *pVictim );
	Vector	GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget ) ;

	// Do we have a physics attacker?
	CBasePlayer *HasPhysicsAttacker( float dt );
	bool IsHeldByPhyscannon( )	{ return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD); }

	// Use functions
	void	ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps() 
	{ 
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this, false );
		}
	}

	// Inputs
	void	InputToggle( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	virtual bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool			CanBeAnEnemyOf( CBaseEntity *pEnemy );
	bool			IsBeingCarriedByPlayer( void ) { return m_bCarriedByPlayer; }
	bool			WasJustDroppedByPlayer( void );

	int		BloodColor( void ) { return DONT_BLEED; }
	float	MaxYawSpeed( void );

	Class_T	Classify( void ) 
	{
		if( m_bEnabled ) 
		{
			if( m_bHackedByAlyx )
			{
				return CLASS_PLAYER_ALLY;
			}

			return CLASS_COMBINE;
		}

		return CLASS_NONE;
	}

	Vector EyePosition( void )
	{
		UpdateMuzzleMatrix();
		
		Vector vecOrigin;
		MatrixGetColumn( m_muzzleToWorld, 3, vecOrigin );
		
		Vector vecForward;
		MatrixGetColumn( m_muzzleToWorld, 0, vecForward );
		
		// Note: We back up into the model to avoid an edge case where the eyes clip out of the world and
		//		 cause problems with the PVS calculations -- jdw

		vecOrigin -= vecForward * 8.0f;

		return vecOrigin;
	}

	Vector	EyeOffset( Activity nActivity ) { return Vector( 0, 0, 58 ); }

	// Restore the turret to working operation after falling over
	void	ReturnToLife( void );

	int		DrawDebugTextOverlays( void );

	// INPCInteractive Functions
	virtual bool	CanInteractWith( CAI_BaseNPC *pUser ) { return false; } // Disabled for now (sjb)
	virtual	bool	HasBeenInteractedWith()	{ return m_bHackedByAlyx; }
	virtual void	NotifyInteraction( CAI_BaseNPC *pUser )
	{
		// For now, turn green so we can tell who is hacked.
		SetRenderColor( 0, 255, 0 );
		m_bHackedByAlyx = true; 
	}

	static float fMaxTipControllerVelocity;
	static float fMaxTipControllerAngularVelocity;

protected:

	bool			PreThink( turretState_e state );
	virtual void	Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy, bool bStrict = false );
	void			SetEyeState( eyeState_t state );
	void			Ping( void );	
	void			Toggle( void );
	void			Enable( void );
	void			Disable( void );
	void			SpinUp( void );
	void			SpinDown( void );

	virtual bool	OnSide( void );

	bool	UpdateFacing( void );
	void	DryFire( void );
	void	UpdateMuzzleMatrix();

protected:
	matrix3x4_t m_muzzleToWorld;
	int		m_muzzleToWorldTick;
	int		m_iAmmoType;

	bool	m_bAutoStart;
	bool	m_bActive;		//Denotes the turret is deployed and looking for targets
	bool	m_bBlinkState;
	bool	m_bEnabled;		//Denotes whether the turret is able to deploy or not

	float	m_flShotTime;
	float	m_flLastSight;
	float	m_flThrashTime;
	float	m_flPingTime;
	float	m_flNextActivateSoundTime;
	bool	m_bCarriedByPlayer;
	bool	m_bUseCarryAngles;
	float	m_flPlayerDropTime;

	CHandle<CBaseCombatCharacter> m_hLastNPCToKickMe;		// Stores the last NPC who tried to knock me over
	float	m_flKnockOverFailedTime;						// Time at which we should tell the NPC that he failed to knock me over

	QAngle	m_vecGoalAngles;

	int						m_iEyeAttachment;
	int						m_iMuzzleAttachment;
	eyeState_t				m_iEyeState;
	CHandle<CSprite>		m_hEyeGlow;
	CHandle<CTurretTipController>	m_pMotionController;

	Vector	m_vecEnemyLKP;

	// physics influence
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;

	static const char		*m_pShotSounds[];

	COutputEvent m_OnDeploy;
	COutputEvent m_OnRetire;
	COutputEvent m_OnTipped;
	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunDrop;

	bool	m_bHackedByAlyx;
	HSOUNDSCRIPTHANDLE			m_ShotSounds;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

//
// Tip controller
//

class CTurretTipController : public CPointEntity, public IMotionEvent
{
	DECLARE_CLASS( CTurretTipController, CPointEntity );
	DECLARE_DATADESC();

public:

	~CTurretTipController( void );
	void Spawn( void );
	void Activate( void );
	void Enable( bool state = true );
	void Suspend( float time );
	float SuspendedTill( void );

	bool Enabled( void );

	static CTurretTipController	*CreateTipController( CNPC_FloorTurret *pOwner )
	{
		if ( pOwner == NULL )
			return NULL;

		CTurretTipController *pController = (CTurretTipController *) Create( "floorturret_tipcontroller", pOwner->GetAbsOrigin(), pOwner->GetAbsAngles() );

		if ( pController != NULL )
		{
			pController->m_pParentTurret = pOwner;
		}

		return pController;
	}

	// IMotionEvent
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	bool						m_bEnabled;
	float						m_flSuspendTime;
	Vector						m_worldGoalAxis;
	Vector						m_localTestAxis;
	IPhysicsMotionController	*m_pController;
	float						m_angularLimit;
	CNPC_FloorTurret			*m_pParentTurret;
};

#endif //#ifndef NPC_TURRET_FLOOR_H
