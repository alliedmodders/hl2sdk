//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "bone_setup.h"
#include "physics_bone_follower.h"
#include "vcollide_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar vcollide_wireframe( "vcollide_wireframe", "0", FCVAR_CHEAT );
//#define VISUALIZE_FOLLOWERS_BOUNDINGBOX 1

//================================================================================================================
// BONE FOLLOWER MANAGER
//================================================================================================================
CBoneFollowerManager::CBoneFollowerManager( void )
{
	m_hOuter = NULL;
	m_iNumBones = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			iNumBones - 
//			**pFollowerBoneNames - 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::InitBoneFollowers( CBaseEntity *pEntity, int iNumBones, const char **pFollowerBoneNames )
{
	m_hOuter = dynamic_cast<CBaseAnimating*>(pEntity);
	Assert( m_hOuter );
	m_iNumBones = iNumBones;
	m_physBones.EnsureCount( iNumBones );

	// Now init all the bones
	for ( int i = 0; i < iNumBones; i++ )
	{
		CreatePhysicsFollower( m_physBones[i], pFollowerBoneNames[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::AddBoneFollower( CBaseEntity *pEntity, const char *pFollowerBoneName )
{
	m_hOuter = dynamic_cast<CBaseAnimating*>(pEntity);
	Assert( m_hOuter );
	m_iNumBones++;

	int iIndex = m_physBones.AddToTail();
	CreatePhysicsFollower( m_physBones[iIndex], pFollowerBoneName );
}

// walk the hitboxes and find the first one that is attached to the physics bone in question
// return the hitgroup of that box
static int HitGroupFromPhysicsBone( CBaseAnimating *pAnim, int physicsBone )
{
	CStudioHdr *pStudioHdr = pAnim->GetModelPtr( );
	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnim->m_nHitboxSet );
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		if ( pStudioHdr->pBone( set->pHitbox(i)->bone )->physicsbone == physicsBone )
		{
			return set->pHitbox(i)->group;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &follow - 
//			*pBoneName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBoneFollowerManager::CreatePhysicsFollower( physfollower_t &follow, const char *pBoneName )
{
	CStudioHdr *pStudioHdr = m_hOuter->GetModelPtr();
	matrix3x4_t boneToWorld;
	solid_t solid;

	Vector bonePosition;
	QAngle boneAngles;

	int boneIndex = Studio_BoneIndexByName( pStudioHdr, pBoneName );

	if ( boneIndex >= 0 )
	{
		mstudiobone_t *pBone = pStudioHdr->pBone( boneIndex );

		int physicsBone = pBone->physicsbone;
		if ( !PhysModelParseSolidByIndex( solid, m_hOuter, m_hOuter->GetModelIndex(), physicsBone ) )
			return false;

		// fixup in case ragdoll is assigned to a parent of the requested follower bone
		follow.boneIndex = Studio_BoneIndexByName( pStudioHdr, solid.name );
		if ( follow.boneIndex < 0 )
		{
			if ( pStudioHdr->numbones() > 1 )
			{
				Warning("Can't find ragdoll bone %s for model %s\n", solid.name, pStudioHdr->pszName() );
			}
			follow.boneIndex = boneIndex;
		}

		m_hOuter->GetBoneTransform( follow.boneIndex, boneToWorld );
		MatrixAngles( boneToWorld, boneAngles, bonePosition );

		follow.hFollower = CBoneFollower::Create( m_hOuter, STRING(m_hOuter->GetModelName()), solid, bonePosition, boneAngles );
		follow.hFollower->SetTraceData( physicsBone, HitGroupFromPhysicsBone( m_hOuter, physicsBone ) );
		follow.hFollower->SetBlocksLOS( m_hOuter->BlocksLOS() );
		return true;
	}
	else
	{
		Warning( "ERROR: Tried to create bone follower on invalid bone %s\n", pBoneName );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::UpdateBoneFollowers( void )
{
	matrix3x4_t boneToWorld;
	Vector bonePosition;
	QAngle boneAngles;
	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		m_hOuter->GetBoneTransform( m_physBones[i].boneIndex, boneToWorld );
		MatrixAngles( boneToWorld, boneAngles, bonePosition );
		m_physBones[i].hFollower->UpdateFollower( bonePosition, boneAngles, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::DestroyBoneFollowers( void )
{
	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		UTIL_Remove( m_physBones[i].hFollower );
		m_physBones[i].hFollower = NULL;
	}

	m_physBones.Purge();
	m_iNumBones = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
physfollower_t *CBoneFollowerManager::GetBoneFollower( int iFollowerIndex )
{
	Assert( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones );
	if ( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones )
		return &m_physBones[iFollowerIndex];
	return NULL;
}

//================================================================================================================
// BONE FOLLOWER
//================================================================================================================

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBoneFollower )

	DEFINE_FIELD( m_modelIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_solidIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_physicsBone,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hitGroup,	FIELD_INTEGER ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CBoneFollower, DT_BoneFollower )
	SendPropModelIndex(SENDINFO(m_modelIndex)),
	SendPropInt(SENDINFO(m_solidIndex), 6, SPROP_UNSIGNED ),
END_SEND_TABLE()


bool CBoneFollower::Init( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	SetOwnerEntity( pOwner );
	UTIL_SetModel( this, pModelName );

	AddEffects( EF_NODRAW ); // invisible
#if VISUALIZE_FOLLOWERS_BOUNDINGBOX
	m_debugOverlays |= OVERLAY_BBOX_BIT;
#endif

	m_modelIndex = modelinfo->GetModelIndex( pModelName );
	m_solidIndex = solid.index;
	SetAbsOrigin( position );
	SetAbsAngles( orientation );
	SetMoveType( MOVETYPE_PUSH );
	SetSolid( SOLID_VPHYSICS );
	SetCollisionGroup( pOwner->GetCollisionGroup() );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	solid.params.pGameData = (void *)this;
	IPhysicsObject *pPhysics = VPhysicsInitShadow( false, false, &solid );
	if ( !pPhysics )
		return false;

	// we can't use the default model bounds because each entity is only one bone of the model
	// so compute the OBB of the physics model and use that.
	Vector mins, maxs;
	physcollision->CollideGetAABB( mins, maxs, pPhysics->GetCollide(), vec3_origin, vec3_angle );
	SetCollisionBounds( mins, maxs );

	pPhysics->SetCallbackFlags( pPhysics->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH );
	pPhysics->EnableGravity( false );
	// This is not a normal shadow controller that is trying to go to a space occupied by an entity in the game physics
	// This entity is not running PhysicsPusher(), so Vphysics is supposed to move it
	// This line of code informs vphysics of that fact
	pPhysics->GetShadowController()->SetPhysicallyControlled( true );

	return true;
}

int CBoneFollower::UpdateTransmitState()
{
	// We want to visualize our bonefollowers, so send them to the client
	if ( vcollide_wireframe.GetBool() )
		return SetTransmitState( FL_EDICT_ALWAYS );
	return SetTransmitState( FL_EDICT_DONTSEND );
}

void CBoneFollower::DrawDebugGeometryOverlays()
{
	if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_ABSBOX_BIT) )
	{
		studiohdr_t *pStudioHdr = NULL;

		const model_t *model = modelinfo->GetModel( m_modelIndex );
		if ( model )
		{
			pStudioHdr = (studiohdr_t *)modelinfo->GetModelExtraData( model );
		}
		for ( int i = 0; i < pStudioHdr->iHitboxCount(0); i++ )
		{
			mstudiobbox_t *pbox = pStudioHdr->pHitbox( i, 0 );
			mstudiobone_t *pBone = pStudioHdr->pBone( pbox->bone );
			if ( pBone->physicsbone == m_solidIndex )
			{
				NDebugOverlay::BoxAngles( GetAbsOrigin(), pbox->bbmin, pbox->bbmax, GetAbsAngles(), 255, 255, 0, 0 ,0);
			}
		}
	}
	if ( m_debugOverlays & OVERLAY_ABSBOX_BIT )
	{
		BaseClass::DrawDebugGeometryOverlays();
	}
}

void CBoneFollower::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	Vector origin;
	QAngle angles;

	pPhysics->GetPosition( &origin, &angles );

	SetAbsOrigin( origin );
	SetAbsAngles( angles );
}

// a little helper class to temporarily change the physics object
// for an entity - and change it back when it goes out of scope.
class CPhysicsSwapTemp
{
public:
	CPhysicsSwapTemp( CBaseEntity *pEntity, IPhysicsObject *pTmpPhysics )
	{
		Assert(pEntity);
		Assert(pTmpPhysics);
		m_pEntity = pEntity;
		m_pPhysics = m_pEntity->VPhysicsGetObject();
		if ( m_pPhysics )
		{
			m_pEntity->VPhysicsSwapObject( pTmpPhysics );
		}
		else
		{
			m_pEntity->VPhysicsSetObject( pTmpPhysics );
		}
	}
	~CPhysicsSwapTemp()
	{
		m_pEntity->VPhysicsSwapObject( m_pPhysics );
	}

private:
	CBaseEntity *m_pEntity;
	IPhysicsObject *m_pPhysics;
};


void CBoneFollower::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pEvent->pObjects[index] );
		pOwner->VPhysicsCollision( index, pEvent );
	}
}

void CBoneFollower::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pEvent->pObjects[index] );
		pOwner->VPhysicsShadowCollision( index, pEvent );
	}
}

void CBoneFollower::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pObject );
		pOwner->VPhysicsFriction( pObject, energy, surfaceProps, surfacePropsHit );
	}
}

bool CBoneFollower::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
	Assert( pCollide && pCollide->solidCount > m_solidIndex );

	UTIL_ClearTrace( trace );

	physcollision->TraceBox( ray, pCollide->solids[m_solidIndex], GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.fraction >= 1 )
		return false;

	// return owner as trace hit
	trace.m_pEnt = GetOwnerEntity();
	trace.hitgroup = m_hitGroup;
	trace.physicsbone = m_physicsBone;
	return true;
}

void CBoneFollower::UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval )
{
	// UNDONE: Shadow update needs timing info?
	VPhysicsGetObject()->UpdateShadow( position, orientation, false, flInterval );

#if VISUALIZE_FOLLOWERS_BOUNDINGBOX
	studiohdr_t *pStudioHdr = NULL;

	const model_t *model = modelinfo->GetModel( m_modelIndex );
	if ( model )
	{
		pStudioHdr = (studiohdr_t *)modelinfo->GetModelExtraData( GetModel() );
	}

	for ( int i = 0; i < pStudioHdr->iHitboxCount(0); i++ )
	{
		mstudiobbox_t *pbox = pStudioHdr->pHitbox( i, 0 );
		mstudiobone_t *pBone = pStudioHdr->pBone( pbox->bone );
		if ( pBone->physicsbone == m_solidIndex )
		{
			NDebugOverlay::BoxAngles( position, pbox->bbmin, pbox->bbmax, orientation, 0, 0, 255, 0 ,0.1);
		}
	}
#endif
}

void CBoneFollower::SetTraceData( int physicsBone, int hitGroup )
{
	m_hitGroup = hitGroup;
	m_physicsBone = physicsBone;
}

CBoneFollower *CBoneFollower::Create( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	CBoneFollower *pFollower = (CBoneFollower *)CreateEntityByName( "phys_bone_follower" );
	if ( pFollower )
	{
		pFollower->Init( pOwner, pModelName, solid, position, orientation );
	}
	return pFollower;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBoneFollower::ObjectCaps() 
{ 
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
		return BaseClass::ObjectCaps() | FCAP_DONT_SAVE | pOwner->ObjectCaps();

	return BaseClass::ObjectCaps() | FCAP_DONT_SAVE;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollower::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->Use( pActivator, pCaller, useType, value );
		return;
	}

	BaseClass::Use( pActivator, pCaller, useType, value );
}

//-----------------------------------------------------------------------------
// Purpose: Pass on Touch calls to the entity we're following
//-----------------------------------------------------------------------------
void CBoneFollower::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		//TODO: fill in the touch trace with the hitbox number associated with this bone
		pOwner->Touch( pOther );
		return;
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Pass on trace attack calls to the entity we're following
//-----------------------------------------------------------------------------
void CBoneFollower::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DispatchTraceAttack( info, vecDir, ptr );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

LINK_ENTITY_TO_CLASS( phys_bone_follower, CBoneFollower );

