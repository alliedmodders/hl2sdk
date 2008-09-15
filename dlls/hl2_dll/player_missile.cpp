//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player_control.h"
#include "hl2_player.h"
#include "decals.h"
#include "shake.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "smoke_trail.h"
#include "grenade_homer.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PMISSILE_MISSILE_MODEL			"models/weapons/w_missile.mdl"
#define PMISSILE_HULL_MINS				Vector(-22,-22,-22)
#define PMISSILE_HULL_MAXS				Vector(22,22,22)
#define PMISSILE_SPEED					800
#define PMISSILE_TURN_RATE				100
#define PMISSILE_DIE_TIME				2
#define PMISSILE_TRAIL_LIFE				6
#define PMISSILE_LOSE_CONTROL_DIST		30
#define PMISSILE_DANGER_SOUND_DURATION	0.1


extern short	g_sModelIndexFireball;


class CPlayer_Missile : public CPlayer_Control
{
public:
	DECLARE_CLASS( CPlayer_Missile, CPlayer_Control );

	void	Precache(void);
	void	Spawn(void);
	Class_T	Classify( void); 

	void ControlActivate( void );
	void ControlDeactivate( void );

	// Inputs
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );

	void	Launch(void);
	void	FlyThink(void);
	void	ExplodeThink(void);
	void	RemoveThink(void);
	void	FlyTouch( CBaseEntity *pOther );
	void	BlowUp(void);
	void	LoseMissileControl(void);

	void	Event_Killed( const CTakeDamageInfo &info );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );

	CPlayer_Missile();

	float			m_flLaunchDelay;
	float			m_flDamage;
	float			m_flDamageRadius;
	CHandle<SmokeTrail>	m_hSmokeTrail;
	Vector			m_vSpawnPos;
	QAngle			m_vSpawnAng;
	Vector			m_vBounceVel;
	bool			m_bShake;
	float			m_flStatic;
	float			m_flNextDangerTime;

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CPlayer_Missile )

	DEFINE_KEYFIELD( m_flLaunchDelay,	FIELD_FLOAT, "LaunchDelay"),
	DEFINE_KEYFIELD( m_flDamage,			FIELD_FLOAT, "Damage"),
	DEFINE_KEYFIELD( m_flDamageRadius,	FIELD_FLOAT, "DamageRadius"),


	// Fields
	DEFINE_FIELD( m_hSmokeTrail,		FIELD_EHANDLE),
	DEFINE_FIELD( m_vSpawnPos,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vSpawnAng,		FIELD_VECTOR ),
	DEFINE_FIELD( m_vBounceVel,		FIELD_VECTOR ),
	DEFINE_FIELD( m_bShake,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStatic,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextDangerTime,	FIELD_TIME ),
	

	// Function Pointers
	DEFINE_FUNCTION( Launch ),
	DEFINE_FUNCTION( FlyThink ),
	DEFINE_FUNCTION( ExplodeThink ),
	DEFINE_FUNCTION( RemoveThink ),
	DEFINE_FUNCTION( FlyTouch ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( player_missile, CPlayer_Missile );

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CPlayer_Missile::Precache( void )
{
	PrecacheModel("models/manhack_sphere.mdl");
	PrecacheModel( PMISSILE_MISSILE_MODEL );

	PrecacheScriptSound( "Player_Manhack.Fly" );
	PrecacheScriptSound( "Player_Manhack.Fire" );
	PrecacheScriptSound( "Player_Manhack.Damage" );

}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CPlayer_Missile::Spawn( void )
{
	Precache();
	SetMoveType( MOVETYPE_FLY );
	SetFriction( 0.55 ); // deading the bounce a bit
	SetGravity(0.0);
	m_iMaxHealth	= m_iHealth;
	m_iHealth		= 0;

	SetSolid( SOLID_BBOX );
	AddEffects( EF_NODRAW );
	AddFlag( FL_OBJECT ); // So shows up in NPC Look()

	SetModel( "models/manhack_sphere.mdl" );
	UTIL_SetSize(this, PMISSILE_HULL_MINS, PMISSILE_HULL_MAXS);

	m_bActive	= false;
	m_vSpawnPos = GetLocalOrigin();
	m_vSpawnAng = GetLocalAngles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Missile::InputActivate( inputdata_t &inputdata )
{
	// Make sure not already active
	if (m_bActive)
	{
		return;
	}

	BaseClass::InputActivate( inputdata );

	Vector origin = GetLocalOrigin();
	origin.z	+= 50;
	SetLocalOrigin( origin );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	// Using player angles to determine turning.  Initailze to 0,0,0
	CHL2_Player* pPlayer = (CHL2_Player*)UTIL_GetLocalPlayer();

	Assert( pPlayer );

	pPlayer->SetLocalAngles( vec3_angle );	// Note: Set GetLocalAngles(), not pl.v_angle
	pPlayer->SnapEyeAngles( vec3_angle );				// Force reset
	pPlayer->SetFOV( this, 100 );

	pPlayer->SetViewEntity( this );

	m_flStatic = 0;
	SetThink(Launch);
	SetNextThink( gpGlobals->curtime + m_flLaunchDelay );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CPlayer_Missile::Launch(void)
{
	m_lifeState		= LIFE_ALIVE;
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= m_iMaxHealth;

	// Shoot forward
	Vector vVelocity; 
	AngleVectors( GetLocalAngles(), &vVelocity);
	SetAbsVelocity( vVelocity*PMISSILE_SPEED );

	EmitSound( "Player_Manhack.Fly" );
	EmitSound( "Player_Manhack.Fire" );


	SetThink(FlyThink);
	SetTouch(FlyTouch);
	SetNextThink( gpGlobals->curtime );

	// Start smoke trail
	SmokeTrail *pSmokeTrail = SmokeTrail::CreateSmokeTrail();
	if(pSmokeTrail)
	{
		pSmokeTrail->m_SpawnRate = 90;
		pSmokeTrail->m_ParticleLifetime = PMISSILE_TRAIL_LIFE;
		pSmokeTrail->m_StartColor.Init(0.1, 0.1, 0.1);
		pSmokeTrail->m_EndColor.Init(0.5,0.5,0.5);
		pSmokeTrail->m_StartSize = 10;
		pSmokeTrail->m_EndSize = 50;
		pSmokeTrail->m_SpawnRadius = 1;
		pSmokeTrail->m_MinSpeed = 15;
		pSmokeTrail->m_MaxSpeed = 25;
		pSmokeTrail->SetLifetime(120);
		pSmokeTrail->FollowEntity(this);

		m_hSmokeTrail = pSmokeTrail;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the steam jet.
//-----------------------------------------------------------------------------
void CPlayer_Missile::InputDeactivate( inputdata_t &inputdata )
{
	ControlDeactivate();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Missile::ControlDeactivate( void )
{
	BaseClass::ControlDeactivate();

	CHL2_Player*	pPlayer		= (CHL2_Player*)UTIL_GetLocalPlayer();

	Assert( pPlayer );

	pPlayer->SetViewEntity( pPlayer );

	if ( pPlayer->GetActiveWeapon() )
	{
		pPlayer->GetActiveWeapon()->Deploy();
	}

	pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;

	SetAbsVelocity( vec3_origin );
	SetLocalAngles( m_vSpawnAng );
	SetLocalOrigin( m_vSpawnPos );
	AddEffects( EF_NODRAW );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
int CPlayer_Missile::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( (info.GetDamageType() & DMG_MISSILEDEFENSE))
	{
		if (random->RandomInt(0,1)==0)
		{
			EmitSound( "Player_Manhack.Damage" );
		}
		m_bShake = true;
	}
	else
	{
		// UNDONE: Blow up to anything else for now
		BlowUp();
	}
	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------
// Purpose: Blow it up
//-----------------------------------------------------------------------------
void CPlayer_Missile::Event_Killed( const CTakeDamageInfo &info )
{
	BlowUp();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Missile::FlyTouch(CBaseEntity *pOther)
{
	BlowUp();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayer_Missile::BlowUp(void)
{
	// Don't take damage any more
	m_takedamage	= DAMAGE_NO;

	// Mark as dead so won't be attacked
	m_lifeState = LIFE_DEAD;

	// Stop fly and launch sounds
	StopSound( "Player_Manhack.Fly" );
	StopSound( "Player_Manhack.Fire" );

	CPASFilter filter( GetAbsOrigin() );
	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		2.0, 
		15,
		TE_EXPLFLAG_NONE,
		m_flDamageRadius,
		m_flDamage );

	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward,  MASK_SOLID_BRUSHONLY, 
		this, COLLISION_GROUP_NONE, &tr);

	UTIL_DecalTrace( &tr, "Scorch" );

	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 1024, 3.0 );

	RadiusDamage ( CTakeDamageInfo( this, this, m_flDamage, DMG_BLAST ), GetAbsOrigin(), m_flDamageRadius, CLASS_NONE, NULL );

	SetThink(ExplodeThink);
	SetTouch(NULL);
	m_vBounceVel	= -0.2 * GetAbsVelocity();
	AddSolidFlags( FSOLID_NOT_SOLID );

	// Stop emitting smoke
	if(m_hSmokeTrail)
	{
		m_hSmokeTrail->SetEmit(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Manhack is still dangerous when dead
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPlayer_Missile::FlyThink()
{
	SetNextThink( gpGlobals->curtime );

	// -------------------
	//  Get the player
	// -------------------
	CBasePlayer*	pPlayer  = UTIL_GetLocalPlayer();
	if (!pPlayer) return;
	
	// ---------------------------------------------------
	// Calulate amount of xturn from player mouse movement
	// ---------------------------------------------------
	QAngle vecViewAngles = pPlayer->EyeAngles();
	float xLev;
	if (vecViewAngles.x < 180)
	{
		xLev = vecViewAngles.x/180;
	}
	else 
	{
		xLev = (vecViewAngles.x-360)/180;
	}

	// ---------------------------------------------------
	// Calulate amount of xturn from player mouse movement
	// ---------------------------------------------------
	float yLev;

	// Keep yaw in bounds (don't let loop)
	if (vecViewAngles.y > 90)
	{
		if (vecViewAngles.y < 180) 
		{
			pPlayer->SetLocalAngles( QAngle( vecViewAngles.x - 360, 90, 0 ) );

			QAngle newViewAngles = vecViewAngles;
			newViewAngles.y = 90;
			pPlayer->SnapEyeAngles( newViewAngles );			
		}	
		else if (vecViewAngles.y < 270) 
		{
			pPlayer->SetLocalAngles( QAngle( vecViewAngles.x, 270, 0 ) );

			QAngle newViewAngles = vecViewAngles;
			newViewAngles.y = 270;
			pPlayer->SnapEyeAngles( newViewAngles );			
		}
		
	}

	if (vecViewAngles.y < 180)
	{
		yLev = vecViewAngles.y/180;
	}
	else 
	{
		yLev = (vecViewAngles.y-360)/180;
	}
	
	//<<TEMP.>>  this is a temp HUD until we create a real one on the client
	NDebugOverlay::ScreenText( 0.5, 0.5+0.2*xLev, "|", 255, 0, 0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5-(0.1*yLev), 0.5, "=", 255, 0, 0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5-(0.1*yLev), 0.5+0.2*xLev, "*", 255, 0, 0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.45, 0.5, "-----------------------", 0, 255, 0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.425, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.45, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.475, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.5, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.525, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.55, "|", 0, 255,  0, 255, 0.0);
	NDebugOverlay::ScreenText( 0.5, 0.575, "|", 0, 255,  0, 255, 0.0);

	// Add in turn
	Vector vRight,vUp;
	AngleVectors( GetLocalAngles(), 0, &vRight,&vUp);
	QAngle angles = GetLocalAngles();

	angles.x += PMISSILE_TURN_RATE*xLev*gpGlobals->frametime;
	angles.y += PMISSILE_TURN_RATE*yLev*gpGlobals->frametime;

	if (m_bShake)
	{
		angles.x += random->RandomFloat(-3.0,3.0);
		angles.y += random->RandomFloat(-3.0,3.0);
		m_bShake = false;
	}

	SetLocalAngles( angles );

	// Reset velocity
	Vector vVelocity;
	AngleVectors( GetLocalAngles(), &vVelocity);
	SetAbsVelocity( vVelocity*PMISSILE_SPEED );

	// Add some screen noise
	float flClipDist = PlayerControlClipDistance();

	if (flClipDist < PMISSILE_LOSE_CONTROL_DIST)
	{
		LoseMissileControl();
	}

	// Average static over time	
	float flStatic = 255*(1-flClipDist/PCONTROL_CLIP_DIST);
	m_flStatic = 0.9*m_flStatic + 0.1*flStatic;
	color32 white = {255,255,255,m_flStatic};
	UTIL_ScreenFade( pPlayer, white, 0.01, 0.1, FFADE_MODULATE );

	// Insert danger sound so NPCs run away from me
	if (gpGlobals->curtime > m_flNextDangerTime)
	{
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 2000, PMISSILE_DANGER_SOUND_DURATION );
		m_flNextDangerTime = gpGlobals->curtime + PMISSILE_DANGER_SOUND_DURATION;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Explode
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPlayer_Missile::LoseMissileControl(void)
{
	// Create a missile to take the place of this one
	CGrenadeHomer *pGrenade = (CGrenadeHomer*)CreateEntityByName( "grenade_homer" );
	if ( pGrenade )
	{
		pGrenade->Spawn();
		pGrenade->SetLocalOrigin( GetLocalOrigin() );
		pGrenade->SetLocalAngles( GetLocalAngles() );
		pGrenade->SetModel( PMISSILE_MISSILE_MODEL );
		pGrenade->SetDamage(m_flDamage);
		pGrenade->SetDamageRadius(m_flDamageRadius);
		pGrenade->Launch(this,NULL,GetAbsVelocity(),0,0,HOMER_SMOKE_TRAIL_OFF);
		pGrenade->m_hRocketTrail[0] = m_hSmokeTrail;
		m_hSmokeTrail->FollowEntity(pGrenade);
	}

	m_takedamage	= DAMAGE_NO;
	m_lifeState = LIFE_DEAD;
	StopSound( "Player_Manhack.Fly" );
	ControlDeactivate();
}

//-----------------------------------------------------------------------------
// Purpose: Explode
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CPlayer_Missile::ExplodeThink()
{
	SetAbsVelocity( m_vBounceVel );

	color32 black = {0,0,0,255};
	CHL2_Player*	pPlayer		= (CHL2_Player*)UTIL_GetLocalPlayer();

	Assert( pPlayer );

	UTIL_ScreenFade( pPlayer, black, 2.0, 0.1, FFADE_OUT );

	SetThink(RemoveThink);
	SetNextThink( gpGlobals->curtime + PMISSILE_DIE_TIME );
}

//-----------------------------------------------------------------------------
// Purpose: Remove me
//-----------------------------------------------------------------------------
void CPlayer_Missile::RemoveThink()
{
	ControlDeactivate();
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
Class_T	CPlayer_Missile::Classify( void)
{ 
	return CLASS_MISSILE; 
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayer_Missile::CPlayer_Missile()
{
	m_vSpawnPos.Init(0,0,0);
	m_vSpawnAng.Init(0,0,0);
	m_vBounceVel.Init(0,0,0);
}
