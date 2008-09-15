//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Physics constraint entities
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "physics.h"
#include "entityoutput.h"
#include "engine/IEngineSound.h"
#include "vphysics/constraints.h"
#include "igamesystem.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_CONSTRAINT_DISABLE_COLLISION			0x0001
#define SF_SLIDE_LIMIT_ENDS						0x0002
#define SF_PULLEY_RIGID							0x0002
#define SF_LENGTH_RIGID							0x0002
#define SF_RAGDOLL_FREEMOVEMENT					0x0002
#define SF_CONSTRAINT_START_INACTIVE			0x0004
#define SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY		0x0008

struct hl_constraint_info_t
{
	hl_constraint_info_t() 
	{ 
		pObjects[0] = pObjects[1] = NULL;
		anchorPosition[0].Init();
		anchorPosition[1].Init();
		swapped = false; 
		massScale[0] = massScale[1] = 1.0f;
	}
	IPhysicsObject	*pObjects[2];
	Vector			anchorPosition[2];
	float			massScale[2];
	bool			swapped;
};

struct constraint_anchor_t
{
	Vector		localOrigin;
	EHANDLE		hEntity;
	string_t	name;
	float		massScale;
};

class CAnchorList : public CAutoGameSystem
{
public:
	CAnchorList( char const *name ) : CAutoGameSystem( name )
	{
	}
	void LevelShutdownPostEntity() 
	{
		m_list.Purge();
	}

	void AddToList( string_t name, CBaseEntity *pEntity, const Vector &localCoordinate, float massScale )
	{
		int index = m_list.AddToTail();
		constraint_anchor_t *pAnchor = &m_list[index];

		pAnchor->hEntity = pEntity;
		pAnchor->name = name;
		pAnchor->localOrigin = localCoordinate;
		pAnchor->massScale = massScale;
	}
	constraint_anchor_t *Find( string_t name )
	{
		for ( int i = m_list.Count()-1; i >=0; i-- )
		{
			if ( FStrEq( STRING(m_list[i].name), STRING(name) ) )
			{
				return &m_list[i];
			}
		}
		return NULL;
	}

private:
	CUtlVector<constraint_anchor_t>	m_list;
};

static CAnchorList g_AnchorList( "CAnchorList" );

class CConstraintAnchor : public CPointEntity
{
	DECLARE_CLASS( CConstraintAnchor, CPointEntity );
public:
	CConstraintAnchor()
	{
		m_massScale = 1.0f;
	}
	void Spawn( void )
	{
		if ( GetParent() )
		{
			g_AnchorList.AddToList( GetEntityName(),  GetParent(), GetLocalOrigin(), m_massScale );
			UTIL_Remove( this );
		}
	}
	DECLARE_DATADESC();

	float m_massScale;
};

BEGIN_DATADESC( CConstraintAnchor )
	DEFINE_KEYFIELD( m_massScale, FIELD_FLOAT, "massScale" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_constraint_anchor, CConstraintAnchor );

class CPhysConstraintSystem : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraintSystem, CLogicalEntity );
public:

	void Spawn();
	IPhysicsConstraintGroup *GetVPhysicsGroup() { return m_pMachine; }

	DECLARE_DATADESC();
private:
	IPhysicsConstraintGroup *m_pMachine;
	int						m_additionalIterations;
};

BEGIN_DATADESC( CPhysConstraintSystem )
	DEFINE_PHYSPTR( m_pMachine ),
	DEFINE_KEYFIELD( m_additionalIterations, FIELD_INTEGER, "additionaliterations" ),
	
END_DATADESC()


void CPhysConstraintSystem::Spawn()
{
	constraint_groupparams_t group;
	group.Defaults();
	group.additionalIterations = m_additionalIterations;
	m_pMachine = physenv->CreateConstraintGroup( group );
}

LINK_ENTITY_TO_CLASS( phys_constraintsystem, CPhysConstraintSystem );

void PhysTeleportConstrainedEntity( CBaseEntity *pTeleportSource, IPhysicsObject *pObject0, IPhysicsObject *pObject1, const Vector &prevPosition, const QAngle &prevAngles, bool physicsRotate )
{
	// teleport the other object
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *> (pObject0->GetGameData());
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *> (pObject1->GetGameData());
	if ( !pEntity0 || !pEntity1 )
		return;

	// figure out which entity needs to be fixed up (the one that isn't pTeleportSource)
	CBaseEntity *pFixup = pEntity1;
	// teleport the other object
	if ( pTeleportSource != pEntity0 )
	{
		if ( pTeleportSource != pEntity1 )
		{
			Msg("Bogus teleport notification!!\n");
			return;
		}
		pFixup = pEntity0;
	}

	// constraint doesn't move this entity
	if ( pFixup->GetMoveType() != MOVETYPE_VPHYSICS )
		return;

	QAngle oldAngles = prevAngles;

	if ( !physicsRotate )
	{
		oldAngles = pTeleportSource->GetAbsAngles();
	}

	matrix3x4_t startCoord, startInv, endCoord, xform;
	AngleMatrix( oldAngles, prevPosition, startCoord );
	MatrixInvert( startCoord, startInv );
	ConcatTransforms( pTeleportSource->EntityToWorldTransform(), startInv, xform );
	QAngle fixupAngles;
	Vector fixupPos;

	ConcatTransforms( xform, pFixup->EntityToWorldTransform(), endCoord );
	MatrixAngles( endCoord, fixupAngles, fixupPos );
	pFixup->Teleport( &fixupPos, &fixupAngles, NULL );
}

abstract_class CPhysConstraint : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraint, CLogicalEntity );
public:

	CPhysConstraint();
	~CPhysConstraint();

	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );
	void Activate( void );

	void ClearStaticFlag( IPhysicsObject *pObj )
	{
		if ( !pObj )
			return;
		PhysClearGameFlags( pObj, FVPHYSICS_CONSTRAINT_STATIC );
	}

	virtual void Deactivate()
	{
		if ( !m_pConstraint )
			return;
		m_pConstraint->Deactivate();
		ClearStaticFlag( m_pConstraint->GetReferenceObject() );
		ClearStaticFlag( m_pConstraint->GetAttachedObject() );
		if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
		{
			// constraint may be getting deactivated because an object got deleted, so check them here.
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			IPhysicsObject *pAtt = m_pConstraint->GetAttachedObject();
			if ( pRef && pAtt )
			{
				PhysEnableEntityCollisions( pRef, pAtt );
			}
		}
	}

	void OnBreak( void )
	{
		Deactivate();
		if ( m_breakSound != NULL_STRING )
		{
			CPASAttenuationFilter filter( this, ATTN_STATIC );

			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;

			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef && (pRef != g_PhysWorldObject) )
			{
				pRef->GetPosition( &refPos, NULL );
				attachPos = refPos;
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach && (pAttach != g_PhysWorldObject) )
			{
				pAttach->GetPosition( &attachPos, NULL );
				if ( !pRef || (pRef == g_PhysWorldObject) )
				{
					refPos = attachPos;
				}
			}
			
			VectorAdd( refPos, attachPos, origin );
			origin *= 0.5f;

			EmitSound_t ep;
			ep.m_nChannel = CHAN_STATIC;
			ep.m_pSoundName = STRING(m_breakSound);
			ep.m_flVolume = VOL_NORM;
			ep.m_SoundLevel = ATTN_TO_SNDLVL( ATTN_STATIC );
			ep.m_pOrigin = &origin;

			EmitSound( filter, entindex(), ep );
		}
		m_OnBreak.FireOutput( this, this );
		UTIL_Remove( this );
	}

	void InputBreak( inputdata_t &inputdata )
	{
		if ( m_pConstraint ) 
			m_pConstraint->Deactivate();
		
		OnBreak();
	}

	void InputOnBreak( inputdata_t &inputdata )
	{
		OnBreak();
	}

	void InputTurnOn( inputdata_t &inputdata )
	{
		if ( m_pConstraint )
		{
			m_pConstraint->Activate();
			m_pConstraint->GetReferenceObject()->Wake();
			m_pConstraint->GetAttachedObject()->Wake();
		}
	}

	void InputTurnOff( inputdata_t &inputdata )
	{
		Deactivate();
	}

	void GetBreakParams( constraint_breakableparams_t &params, const hl_constraint_info_t &info )
	{
		params.Defaults();
		params.forceLimit = lbs2kg(m_forceLimit);
		params.torqueLimit = lbs2kg(m_torqueLimit);
		params.isActive = HasSpawnFlags( SF_CONSTRAINT_START_INACTIVE ) ? false : true;
		params.bodyMassScale[0] = info.massScale[0];
		params.bodyMassScale[1] = info.massScale[1];
	}

	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

protected:	
	void GetConstraintObjects( hl_constraint_info_t &info );
	void SetupTeleportationHandling( hl_constraint_info_t &info );
	bool ActivateConstraint( void );
	virtual IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info ) = 0;

	IPhysicsConstraint	*m_pConstraint;

	// These are "template" values used to construct the hinge
	string_t		m_nameAttach1;
	string_t		m_nameAttach2;
	string_t		m_breakSound;
	string_t		m_nameSystem;
	float			m_forceLimit;
	float			m_torqueLimit;
	unsigned int	m_teleportTick;

	COutputEvent	m_OnBreak;
};

BEGIN_DATADESC( CPhysConstraint )

	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_KEYFIELD( m_nameSystem, FIELD_STRING, "constraintsystem" ),
	DEFINE_KEYFIELD( m_nameAttach1, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( m_nameAttach2, FIELD_STRING, "attach2" ),
	DEFINE_KEYFIELD( m_breakSound, FIELD_SOUNDNAME, "breaksound" ),
	DEFINE_KEYFIELD( m_forceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( m_torqueLimit, FIELD_FLOAT, "torquelimit" ),
//	DEFINE_FIELD( m_teleportTick, FIELD_INTEGER ),

	DEFINE_OUTPUT( m_OnBreak, "OnBreak" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ConstraintBroken", InputOnBreak ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()


CPhysConstraint::CPhysConstraint( void )
{
	m_pConstraint = NULL;
	m_nameAttach1 = NULL_STRING;
	m_nameAttach2 = NULL_STRING;
	m_forceLimit = 0;
	m_torqueLimit = 0;
	m_teleportTick = 0xFFFFFFFF;
}

CPhysConstraint::~CPhysConstraint()
{
	Deactivate();
	physenv->DestroyConstraint( m_pConstraint );
}

void CPhysConstraint::Precache( void )
{
	if ( m_breakSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_breakSound) );
	}
}

void CPhysConstraint::Spawn( void )
{
	BaseClass::Spawn();

	Precache();
}

void FindPhysicsAnchor( string_t name, hl_constraint_info_t &info, int index )
{
	constraint_anchor_t *pAnchor = g_AnchorList.Find( name );
	if ( pAnchor )
	{
		CBaseEntity *pEntity = pAnchor->hEntity;
		if ( pEntity )
		{
			info.anchorPosition[index] = pAnchor->localOrigin;
			info.pObjects[index] = pAnchor->hEntity->VPhysicsGetObject();
			info.massScale[index] = pAnchor->massScale;
		}
		else
		{
			pAnchor = NULL;
		}
	}
	if ( !pAnchor )
	{
		info.anchorPosition[index] = vec3_origin;
		info.pObjects[index] = FindPhysicsObjectByName( STRING(name) );
		info.massScale[index] = 1.0f;
	}
}

void CPhysConstraint::SetupTeleportationHandling( hl_constraint_info_t &info )
{
	CBaseEntity *pEntity0 = (CBaseEntity *)info.pObjects[0]->GetGameData();
	if ( pEntity0 )
	{
		g_pNotify->AddEntity( this, pEntity0 );
	}

	CBaseEntity *pEntity1 = (CBaseEntity *)info.pObjects[1]->GetGameData();
	if ( pEntity1 )
	{
		g_pNotify->AddEntity( this, pEntity1 );
	}
}

void CPhysConstraint::GetConstraintObjects( hl_constraint_info_t &info )
{
	FindPhysicsAnchor( m_nameAttach1, info, 0 );
	FindPhysicsAnchor( m_nameAttach2, info, 1 );

	// Missing one object, assume the world instead
	if ( info.pObjects[0] == NULL && info.pObjects[1] )
	{
		if ( Q_strlen(STRING(m_nameAttach1)) )
		{
			Warning("Bogus constraint %s (attaches %s to %s)\n", GetDebugName(), STRING(m_nameAttach1), STRING(m_nameAttach2));
			info.pObjects[0] = info.pObjects[1] = NULL;
			return;
		}
		info.pObjects[0] = g_PhysWorldObject;
		info.massScale[0] = info.massScale[1] = 1.0f; // no mass scale on world constraint
	}
	else if ( info.pObjects[0] && !info.pObjects[1] )
	{
		if ( Q_strlen(STRING(m_nameAttach2)) )
		{
			Warning("Bogus constraint %s (attaches %s to %s)\n", GetDebugName(), STRING(m_nameAttach1), STRING(m_nameAttach2));
			info.pObjects[0] = info.pObjects[1] = NULL;
			return;
		}
		info.pObjects[1] = info.pObjects[0];
		info.pObjects[0] = g_PhysWorldObject;		// Try to make the world object consistently object0 for ease of implementation
		info.massScale[0] = info.massScale[1] = 1.0f; // no mass scale on world constraint
		info.swapped = true;
	}
	else if ( info.pObjects[0] && info.pObjects[1] )
	{
		SetupTeleportationHandling( info );
	}
}

void CPhysConstraint::Activate( void )
{
	BaseClass::Activate();

	if ( !ActivateConstraint() )
	{
		UTIL_Remove(this);
	}
}

IPhysicsConstraintGroup *GetConstraintGroup( string_t systemName )
{
	CBaseEntity *pMachine = gEntList.FindEntityByName( NULL, systemName );

	if ( pMachine )
	{
		CPhysConstraintSystem *pGroup = dynamic_cast<CPhysConstraintSystem *>(pMachine);
		if ( pGroup )
		{
			return pGroup->GetVPhysicsGroup();
		}
	}
	return NULL;
}

bool CPhysConstraint::ActivateConstraint( void )
{
	// A constraint attaches two objects to each other.
	// The constraint is specified in the coordinate frame of the "reference" object
	// and constrains the "attached" object
	hl_constraint_info_t info;
	if ( m_pConstraint )
	{
		// already have a constraint, don't make a new one
		info.pObjects[0] = m_pConstraint->GetReferenceObject();
		info.pObjects[1] = m_pConstraint->GetAttachedObject();
		if ( info.pObjects[0] && info.pObjects[1] )
		{
			SetupTeleportationHandling( info );
		}

		if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
		{
			PhysDisableEntityCollisions( info.pObjects[0], info.pObjects[1] );
		}
		return true;
	}

	GetConstraintObjects( info );
	if ( !info.pObjects[0] && !info.pObjects[1] )
		return false;

	if ( info.pObjects[0]->IsStatic() && info.pObjects[1]->IsStatic() )
	{
		Warning("Constraint (%s) attached to two static objects (%s and %s)!!!\n", STRING(GetEntityName()), STRING(m_nameAttach1), m_nameAttach2 == NULL_STRING ? "world" : STRING(m_nameAttach2) );
		return false;
	}

	if ( info.pObjects[0]->GetShadowController() && info.pObjects[1]->GetShadowController() )
	{
		Warning("Constraint (%s) attached to two shadow objects (%s and %s)!!!\n", STRING(GetEntityName()), STRING(m_nameAttach1), m_nameAttach2 == NULL_STRING ? "world" : STRING(m_nameAttach2) );
		return false;
	}
	IPhysicsConstraintGroup *pGroup = GetConstraintGroup( m_nameSystem );
	m_pConstraint = CreateConstraint( pGroup, info );
	if ( !m_pConstraint )
		return false;

	m_pConstraint->SetGameData( (void *)this );

	if ( pGroup )
	{
		pGroup->Activate();
	}

	if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
	{
		PhysDisableEntityCollisions( info.pObjects[0], info.pObjects[1] );
	}

	return true;
}

void CPhysConstraint::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// don't recurse
	if ( eventType != NOTIFY_EVENT_TELEPORT || (unsigned int)gpGlobals->tickcount == m_teleportTick )
		return;

	m_teleportTick = gpGlobals->tickcount;

	PhysTeleportConstrainedEntity( pNotify, m_pConstraint->GetReferenceObject(), m_pConstraint->GetAttachedObject(), params.pTeleport->prevOrigin, params.pTeleport->prevAngles, params.pTeleport->physicsRotate );
}

class CPhysHinge : public CPhysConstraint
{
	DECLARE_CLASS( CPhysHinge, CPhysConstraint );

public:
	void Spawn( void );
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		if ( m_hinge.worldAxisDirection == vec3_origin )
		{
			DevMsg("ERROR: Hinge with bad data!!!\n" );
			return NULL;
		}
		GetBreakParams( m_hinge.constraint, info );
		m_hinge.constraint.strength = 1.0;
		// BUGBUG: These numbers are very hard to edit
		// Scale by 1000 to make things easier
		// CONSIDER: Unify the units of torque around something other 
		// than HL units (kg * in^2 / s ^2)
		m_hinge.hingeAxis.SetAxisFriction( 0, 0, m_hingeFriction * 1000 );

		int hingeAxis;
		if ( IsWorldHinge( info, &hingeAxis ) )
		{
			info.pObjects[1]->BecomeHinged( hingeAxis );
		}
		else
		{
			RemoveSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY );
		}

		return physenv->CreateHingeConstraint( info.pObjects[0], info.pObjects[1], pGroup, m_hinge );
	}

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line(m_hinge.worldPosition, m_hinge.worldPosition + 48 * m_hinge.worldAxisDirection, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	void InputSetVelocity( inputdata_t &inputdata )
	{
		float speed = inputdata.value.Float();
		float massLoad = 1;
		int numMasses = 0;
		if ( m_pConstraint->GetReferenceObject()->IsMoveable() )
		{
			massLoad = m_pConstraint->GetReferenceObject()->GetInertia().Length();
			numMasses++;
			m_pConstraint->GetReferenceObject()->Wake();
		}
		if ( m_pConstraint->GetAttachedObject()->IsMoveable() )
		{
			massLoad += m_pConstraint->GetAttachedObject()->GetInertia().Length();
			numMasses++;
			m_pConstraint->GetAttachedObject()->Wake();
		}
		if ( numMasses > 0 )
		{
			massLoad /= (float)numMasses;
		}
		float loadscale = m_systemLoadScale != 0 ? m_systemLoadScale : 1;
		m_pConstraint->SetAngularMotor( speed, speed * loadscale * massLoad * loadscale * (1.0/TICK_INTERVAL) );
	}

	virtual void Deactivate()
	{
		if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) )
		{
			// NOTE: RemoveHinged() is always safe
			m_pConstraint->GetAttachedObject()->RemoveHinged();
		}

		BaseClass::Deactivate();
	}


	DECLARE_DATADESC();

private:
	constraint_hingeparams_t m_hinge;
	float m_hingeFriction;
	float	m_systemLoadScale;
	bool IsWorldHinge( const hl_constraint_info_t &info, int *pAxisOut );
};

BEGIN_DATADESC( CPhysHinge )

// Quiet down classcheck
//	DEFINE_FIELD( m_hinge, FIELD_??? ),

	DEFINE_KEYFIELD( m_hingeFriction, FIELD_FLOAT, "hingefriction" ),
	DEFINE_FIELD( m_hinge.worldPosition, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( m_hinge.worldAxisDirection, FIELD_VECTOR, "hingeaxis" ),
	DEFINE_KEYFIELD( m_systemLoadScale, FIELD_FLOAT, "systemloadscale" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAngularVelocity", InputSetVelocity ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_hinge, CPhysHinge );


void CPhysHinge::Spawn( void )
{
	m_hinge.worldPosition = GetLocalOrigin();
	m_hinge.worldAxisDirection -= GetLocalOrigin();
	VectorNormalize(m_hinge.worldAxisDirection);
	UTIL_SnapDirectionToAxis( m_hinge.worldAxisDirection );

	m_hinge.hingeAxis.SetAxisFriction( 0, 0, 0 );

	if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) )
	{
		masscenteroverride_t params;
		if ( m_nameAttach1 == NULL_STRING )
		{
			params.SnapToAxis( m_nameAttach2, m_hinge.worldPosition, m_hinge.worldAxisDirection );
			PhysSetMassCenterOverride( params );
		}
		else if ( m_nameAttach2 == NULL_STRING )
		{
			params.SnapToAxis( m_nameAttach1, m_hinge.worldPosition, m_hinge.worldAxisDirection );
			PhysSetMassCenterOverride( params );
		}
		else
		{
			RemoveSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY );
		}
	}
}


static int GetUnitAxisIndex( const Vector &axis )
{
	bool valid = false;
	int index = -1;

	for ( int i = 0; i < 3; i++ )
	{
		if ( axis[i] != 0 )
		{
			if ( fabs(axis[i]) == 1 )
			{
				if ( index  < 0 )
				{
					index = i;
					valid = true;
					continue;
				}
			}
			valid = false;
		}
	}
	return valid ? index : -1;
}

bool CPhysHinge::IsWorldHinge( const hl_constraint_info_t &info, int *pAxisOut )
{
	if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) && info.pObjects[0] == g_PhysWorldObject )
	{
		Vector localHinge;
		info.pObjects[1]->WorldToLocalVector( &localHinge, m_hinge.worldAxisDirection );
		UTIL_SnapDirectionToAxis( localHinge );
		int hingeAxis = GetUnitAxisIndex( localHinge );
		if ( hingeAxis >= 0 )
		{
			*pAxisOut = hingeAxis;
			return true;
		}
	}
	return false;
}

class CPhysBallSocket : public CPhysConstraint
{
public:
	DECLARE_CLASS( CPhysBallSocket, CPhysConstraint );

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		constraint_ballsocketparams_t ballsocket;
	
		ballsocket.Defaults();
		
		for ( int i = 0; i < 2; i++ )
		{
			info.pObjects[i]->WorldToLocal( &ballsocket.constraintPosition[i], GetAbsOrigin() );
		}
		GetBreakParams( ballsocket.constraint, info );
		ballsocket.constraint.torqueLimit = 0;

		return physenv->CreateBallsocketConstraint( info.pObjects[0], info.pObjects[1], pGroup, ballsocket );
	}
};

LINK_ENTITY_TO_CLASS( phys_ballsocket, CPhysBallSocket );

class CPhysSlideConstraint : public CPhysConstraint
{
public:
	DECLARE_CLASS( CPhysSlideConstraint, CPhysConstraint );

	DECLARE_DATADESC();
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );
	void InputSetVelocity( inputdata_t &inputdata )
	{
		float speed = inputdata.value.Float();
		float massLoad = 1;
		int numMasses = 0;
		if ( m_pConstraint->GetReferenceObject()->IsMoveable() )
		{
			massLoad = m_pConstraint->GetReferenceObject()->GetMass();
			numMasses++;
			m_pConstraint->GetReferenceObject()->Wake();
		}
		if ( m_pConstraint->GetAttachedObject()->IsMoveable() )
		{
			massLoad += m_pConstraint->GetAttachedObject()->GetMass();
			numMasses++;
			m_pConstraint->GetAttachedObject()->Wake();
		}
		if ( numMasses > 0 )
		{
			massLoad /= (float)numMasses;
		}
		float loadscale = m_systemLoadScale != 0 ? m_systemLoadScale : 1;
		m_pConstraint->SetLinearMotor( speed, speed * loadscale * massLoad * (1.0/TICK_INTERVAL) );
	}


	Vector	m_axisEnd;
	float	m_slideFriction;
	float	m_systemLoadScale;
};

LINK_ENTITY_TO_CLASS( phys_slideconstraint, CPhysSlideConstraint );

BEGIN_DATADESC( CPhysSlideConstraint )

	DEFINE_KEYFIELD( m_axisEnd, FIELD_POSITION_VECTOR, "slideaxis" ),
	DEFINE_KEYFIELD( m_slideFriction, FIELD_FLOAT, "slidefriction" ),
	DEFINE_KEYFIELD( m_systemLoadScale, FIELD_FLOAT, "systemloadscale" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVelocity", InputSetVelocity ),

END_DATADESC()



IPhysicsConstraint *CPhysSlideConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_slidingparams_t sliding;
	sliding.Defaults();
	GetBreakParams( sliding.constraint, info );
	sliding.constraint.strength = 1.0;

	Vector axisDirection = m_axisEnd - GetAbsOrigin();
	VectorNormalize( axisDirection );
	UTIL_SnapDirectionToAxis( axisDirection );

	sliding.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1], axisDirection );
	sliding.friction = m_slideFriction;
	if ( m_spawnflags & SF_SLIDE_LIMIT_ENDS )
	{
		Vector position;
		info.pObjects[1]->GetPosition( &position, NULL );

		sliding.limitMin = DotProduct( axisDirection, GetAbsOrigin() );
		sliding.limitMax = DotProduct( axisDirection, m_axisEnd );
		if ( sliding.limitMax < sliding.limitMin )
		{
			swap( sliding.limitMin, sliding.limitMax );
		}

		// expand limits to make initial position of the attached object valid
		float limit = DotProduct( position, axisDirection );
		if ( limit < sliding.limitMin )
		{
			sliding.limitMin = limit;
		}
		else if ( limit > sliding.limitMax )
		{
			sliding.limitMax = limit;
		}
		// offset so that the current position is 0
		sliding.limitMin -= limit;
		sliding.limitMax -= limit;
	}

	return physenv->CreateSlidingConstraint( info.pObjects[0], info.pObjects[1], pGroup, sliding );
}

//-----------------------------------------------------------------------------
// Purpose: Fixed breakable constraint
//-----------------------------------------------------------------------------
class CPhysFixed : public CPhysConstraint
{
	DECLARE_CLASS( CPhysFixed, CPhysConstraint );
public:
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );
};

LINK_ENTITY_TO_CLASS( phys_constraint, CPhysFixed );

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysFixed::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1] );
	GetBreakParams( fixed.constraint, info );

	// constraining to the world means object 1 is fixed
	if ( info.pObjects[0] == g_PhysWorldObject )
	{
		PhysSetGameFlags( info.pObjects[1], FVPHYSICS_CONSTRAINT_STATIC );
	}

	return physenv->CreateFixedConstraint( info.pObjects[0], info.pObjects[1], pGroup, fixed );
}


//-----------------------------------------------------------------------------
// Purpose: Breakable pulley w/ropes constraint
//-----------------------------------------------------------------------------
class CPhysPulley : public CPhysConstraint
{
	DECLARE_CLASS( CPhysPulley, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			NDebugOverlay::Line( refPos, origin, 0, 255, 0, false, 0 );
			NDebugOverlay::Line( origin, m_position2, 0, 255, 0, false, 0 );
			NDebugOverlay::Line( m_position2, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_position2;
	Vector		m_offset[2];
	float		m_addLength;
	float		m_gearRatio;
};

BEGIN_DATADESC( CPhysPulley )

	DEFINE_KEYFIELD( m_position2, FIELD_POSITION_VECTOR, "position2" ),
	DEFINE_AUTO_ARRAY( m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( m_gearRatio, FIELD_FLOAT, "gearratio" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_pulleyconstraint, CPhysPulley );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysPulley::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_pulleyparams_t pulley;
	pulley.Defaults();
	pulley.pulleyPosition[0] = GetAbsOrigin();
	pulley.pulleyPosition[1] = m_position2;

	matrix3x4_t matrix;
	Vector world[2];

	info.pObjects[0]->GetPositionMatrix( &matrix );
	VectorTransform( info.anchorPosition[0], matrix, world[0] );
	info.pObjects[1]->GetPositionMatrix( &matrix );
	VectorTransform( info.anchorPosition[1], matrix, world[1] );

	for ( int i = 0; i < 2; i++ )
	{
		pulley.objectPosition[i] = info.anchorPosition[i];
		m_offset[i] = info.anchorPosition[i];
	}
	
	pulley.totalLength = m_addLength + 
		(world[0] - pulley.pulleyPosition[0]).Length() + 
		((world[1] - pulley.pulleyPosition[1]).Length() * m_gearRatio);

	if ( m_gearRatio != 0 )
	{
		pulley.gearRatio = m_gearRatio;
	}
	GetBreakParams( pulley.constraint, info );
	if ( m_spawnflags & SF_PULLEY_RIGID )
	{
		pulley.isRigid = true;
	}

	return physenv->CreatePulleyConstraint( info.pObjects[0], info.pObjects[1], pGroup, pulley );
}

//-----------------------------------------------------------------------------
// Purpose: Breakable rope/length constraint
//-----------------------------------------------------------------------------
class CPhysLength : public CPhysConstraint
{
	DECLARE_CLASS( CPhysLength, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_offset[2];
	Vector		m_vecAttach;
	float		m_addLength;
	float		m_minLength;
};

BEGIN_DATADESC( CPhysLength )

	DEFINE_AUTO_ARRAY( m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( m_minLength, FIELD_FLOAT, "minlength" ),
	DEFINE_KEYFIELD( m_vecAttach, FIELD_POSITION_VECTOR, "attachpoint" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_lengthconstraint, CPhysLength );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysLength::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_lengthparams_t length;
	length.Defaults();
	Vector position[2];
	position[0] = GetAbsOrigin();
	position[1] = m_vecAttach;
	int index = info.swapped ? 1 : 0;
	length.InitWorldspace( info.pObjects[0], info.pObjects[1], position[index], position[!index] );
	length.totalLength += m_addLength;
	length.minLength = m_minLength;
	if ( HasSpawnFlags(SF_LENGTH_RIGID) )
	{
		length.minLength = length.totalLength;
	}

	for ( int i = 0; i < 2; i++ )
	{
		m_offset[i] = length.objectPosition[i];
	}
	GetBreakParams( length.constraint, info );

	return physenv->CreateLengthConstraint( info.pObjects[0], info.pObjects[1], pGroup, length );
}

//-----------------------------------------------------------------------------
// Purpose: Limited ballsocket constraint with toggle-able translation constraints
//-----------------------------------------------------------------------------
class CRagdollConstraint : public CPhysConstraint
{
	DECLARE_CLASS( CRagdollConstraint, CPhysConstraint );
public:
	DECLARE_DATADESC();
#if 0
	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
#endif

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	float		m_xmin;	// constraint limits in degrees
	float		m_xmax;
	float		m_ymin;
	float		m_ymax;
	float		m_zmin;
	float		m_zmax;

	float		m_xfriction;
	float		m_yfriction;
	float		m_zfriction;
};

BEGIN_DATADESC( CRagdollConstraint )

	DEFINE_KEYFIELD( m_xmin, FIELD_FLOAT, "xmin" ),
	DEFINE_KEYFIELD( m_xmax, FIELD_FLOAT, "xmax" ),
	DEFINE_KEYFIELD( m_ymin, FIELD_FLOAT, "ymin" ),
	DEFINE_KEYFIELD( m_ymax, FIELD_FLOAT, "ymax" ),
	DEFINE_KEYFIELD( m_zmin, FIELD_FLOAT, "zmin" ),
	DEFINE_KEYFIELD( m_zmax, FIELD_FLOAT, "zmax" ),
	DEFINE_KEYFIELD( m_xfriction, FIELD_FLOAT, "xfriction" ),
	DEFINE_KEYFIELD( m_yfriction, FIELD_FLOAT, "yfriction" ),
	DEFINE_KEYFIELD( m_zfriction, FIELD_FLOAT, "zfriction" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_ragdollconstraint, CRagdollConstraint );

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CRagdollConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_ragdollparams_t ragdoll;
	ragdoll.Defaults();

	matrix3x4_t entityToWorld, worldToEntity;
	info.pObjects[0]->GetPositionMatrix( &entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToReference );

	info.pObjects[1]->GetPositionMatrix( &entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToAttached );

	ragdoll.onlyAngularLimits = HasSpawnFlags( SF_RAGDOLL_FREEMOVEMENT ) ? true : false;

	// FIXME: Why are these friction numbers in different units from what the hinge uses?
	ragdoll.axes[0].SetAxisFriction( m_xmin, m_xmax, m_xfriction );
	ragdoll.axes[1].SetAxisFriction( m_ymin, m_ymax, m_yfriction );
	ragdoll.axes[2].SetAxisFriction( m_zmin, m_zmax, m_zfriction );

	if ( HasSpawnFlags( SF_CONSTRAINT_START_INACTIVE ) )
	{
		ragdoll.isActive = false;
	}
	return physenv->CreateRagdollConstraint( info.pObjects[0], info.pObjects[1], pGroup, ragdoll );
}



class CPhysConstraintEvents : public IPhysicsConstraintEvent
{
	void ConstraintBroken( IPhysicsConstraint *pConstraint )
	{
		CBaseEntity *pEntity = (CBaseEntity *)pConstraint->GetGameData();
		if ( pEntity )
		{
			IPhysicsConstraintEvent *pConstraintEvent = dynamic_cast<IPhysicsConstraintEvent*>( pEntity );
			//Msg("Constraint broken %s\n", pEntity->GetDebugName() );
			if ( pConstraintEvent )
			{
				pConstraintEvent->ConstraintBroken( pConstraint );
			}
			else
			{
				variant_t emptyVariant;
				pEntity->AcceptInput( "ConstraintBroken", NULL, NULL, emptyVariant, 0 );
			}
		}
	}
};

static CPhysConstraintEvents constraintevents;
// registered in physics.cpp
IPhysicsConstraintEvent *g_pConstraintEvents = &constraintevents;
