//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GAMETRACE_H
#define GAMETRACE_H
#ifdef _WIN32
#pragma once
#endif


#include "cmodel.h"
#include "Color.h"
#include "entity2/entityinstance.h"
#include "mathlib/transform.h"
#include "tier1/generichash.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "ispatialpartition.h"

class IPhysicsBody;
class IPhysicsShape;

typedef IPhysicsBody* HPhysicsBody;
typedef IPhysicsShape* HPhysicsShape;

enum CollisionFunctionMask_t
{
	FCOLLISION_FUNC_ENABLE_SOLID_CONTACT 	= (1<<0),
	FCOLLISION_FUNC_ENABLE_TRACE_QUERY 		= (1<<1),
	FCOLLISION_FUNC_ENABLE_TOUCH_EVENT 		= (1<<2),
	FCOLLISION_FUNC_ENABLE_SELF_COLLISIONS 	= (1<<3),
	FCOLLISION_FUNC_IGNORE_FOR_HITBOX_TEST 	= (1<<4),
	FCOLLISION_FUNC_ENABLE_TOUCH_PERSISTS 	= (1<<5),
};

// these are on by default
#define FCOLLISION_FUNC_DEFAULT (FCOLLISION_FUNC_ENABLE_SOLID_CONTACT | FCOLLISION_FUNC_ENABLE_TRACE_QUERY | FCOLLISION_FUNC_ENABLE_TOUCH_EVENT)

enum RnQueryObjectSet
{
	RNQUERY_OBJECTS_STATIC 				= (1<<0),
	RNQUERY_OBJECTS_DYNAMIC 			= (1<<1),
	RNQUERY_OBJECTS_NON_COLLIDEABLE 	= (1<<2),
	RNQUERY_OBJECTS_KEYFRAMED_ONLY 		= (1<<3) | (1<<8),
	RNQUERY_OBJECTS_DYNAMIC_ONLY		= (1<<4) | (1<<8),
	
	RNQUERY_OBJECTS_ALL_GAME_ENTITIES 	= RNQUERY_OBJECTS_DYNAMIC | RNQUERY_OBJECTS_NON_COLLIDEABLE,
	RNQUERY_OBJECTS_ALL 				= RNQUERY_OBJECTS_STATIC | RNQUERY_OBJECTS_ALL_GAME_ENTITIES,
};

enum HitGroup_t
{
	HITGROUP_INVALID = -1,
	HITGROUP_GENERIC = 0,
	HITGROUP_HEAD,
	HITGROUP_CHEST,
	HITGROUP_STOMACH,
	HITGROUP_LEFTARM,
	HITGROUP_RIGHTARM,
	HITGROUP_LEFTLEG,
	HITGROUP_RIGHTLEG,
	HITGROUP_NECK,
	HITGROUP_UNUSED,
	HITGROUP_GEAR,
	HITGROUP_SPECIAL,
	HITGROUP_COUNT,
};

enum HitboxShapeType_t
{
	HITBOX_SHAPE_HULL = 0,
	HITBOX_SHAPE_SPHERE,
	HITBOX_SHAPE_CAPSULE,
};

class CPhysSurfacePropertiesPhysics
{
public:
	CPhysSurfacePropertiesPhysics()
	{
		m_friction = 0.0f;
		m_elasticity = 0.0f;
		m_density = 0.0f;
		m_thickness = 0.1f;
		m_softContactFrequency = 0.0f;
		m_softContactDampingRatio = 0.0f;
		m_wheelDrag = 0.0f;
	}
	
public:
	float m_friction;
	float m_elasticity;
	float m_density;
	float m_thickness;
	float m_softContactFrequency;
	float m_softContactDampingRatio;
	float m_wheelDrag;
};

class CPhysSurfacePropertiesSoundNames
{
public:
	CPhysSurfacePropertiesSoundNames()
	{
		m_meleeImpact = "";
		m_pushOff = "";
		m_skidStop = "";
	}
	
public:
	CUtlString m_impactSoft;
	CUtlString m_impactHard;
	CUtlString m_scrapeSmooth;
	CUtlString m_scrapeRough;
	CUtlString m_bulletImpact;
	CUtlString m_rolling;
	CUtlString m_break;
	CUtlString m_strain;
	CUtlString m_meleeImpact;
	CUtlString m_pushOff;
	CUtlString m_skidStop;
};

class CPhysSurfacePropertiesAudio
{
public:
	CPhysSurfacePropertiesAudio()
	{
		m_reflectivity = 0.0f;
		m_hardnessFactor = 0.0f;
		m_roughnessFactor = 0.0f;
		m_roughThreshold = 0.0f;
		m_hardThreshold = 0.0f;
		m_hardVelocityThreshold = 0.0f;
		m_flStaticImpactVolume = 0.0f;
		m_flOcclusionFactor = 0.0f;
	}
	
public:
	float m_reflectivity;
	float m_hardnessFactor;
	float m_roughnessFactor;
	float m_roughThreshold;
	float m_hardThreshold;
	float m_hardVelocityThreshold;
	float m_flStaticImpactVolume;
	float m_flOcclusionFactor;
};

class CPhysSurfaceProperties
{
public:
	CPhysSurfaceProperties()
	{
		m_bHidden = false;
	}
	
public:
	CUtlString m_name;
	uint32 m_nameHash;
	uint32 m_baseNameHash;
	int32 m_nListIndex;
	int32 m_nBaseListIndex;
	bool m_bHidden;
	CUtlString m_description;
	CPhysSurfacePropertiesPhysics m_physics;
	CPhysSurfacePropertiesSoundNames m_audioSounds;
	CPhysSurfacePropertiesAudio m_audioParams;
};

class CHitBox
{
public:
	CHitBox()
	{
		m_vMinBounds.Init();
		m_vMaxBounds.Init();
		m_nBoneNameHash = 0;
		m_nGroupId = HITGROUP_GENERIC;
		m_nShapeType = HITBOX_SHAPE_HULL;
		m_bTranslationOnly = false;
		m_CRC = 0;
		m_cRenderColor.SetColor( 255, 255, 255, 255 );
		m_nHitBoxIndex = 0;
		m_bForcedTransform = false;
		m_forcedTransform.SetToIdentity();
	}
	
public:
	CUtlString m_name;
	CUtlString m_sSurfaceProperty;
	CUtlString m_sBoneName;
	Vector m_vMinBounds;
	Vector m_vMaxBounds;
	float m_flShapeRadius;
	uint32 m_nBoneNameHash;
	int32 m_nGroupId;
	uint8 m_nShapeType;
	bool m_bTranslationOnly;
	uint32 m_CRC;
	Color m_cRenderColor;
	uint16 m_nHitBoxIndex;
	bool m_bForcedTransform;
	CTransform m_forcedTransform;
};

class CDefaultHitbox : public CHitBox
{
public:
	CDefaultHitbox()
	{
		V_strncpy( m_tempName, "invalid_hitbox", sizeof( m_tempName ) );
		V_strncpy( m_tempBoneName, "invalid_bone", sizeof( m_tempBoneName ) );
		V_strncpy( m_tempSurfaceProp, "default", sizeof( m_tempSurfaceProp ) );
		
		m_name = m_tempName;
		m_sSurfaceProperty = m_tempSurfaceProp;
		m_sBoneName = m_tempBoneName;
		
		m_nBoneNameHash = MurmurHash2LowerCase(m_tempBoneName, 0x31415926);

#if 0
		if (g_bUpdateStringTokenDatabase)
		{
			RegisterStringToken(m_nBoneNameHash, m_tempBoneName, 0, true);
		}
#endif

		m_cRenderColor.SetColor( 0, 0, 0, 0 );
	}
	
public:
	char m_tempName[16];
	char m_tempBoneName[16];
	char m_tempSurfaceProp[16];
};

struct RnQueryShapeAttr_t
{
public:
	RnQueryShapeAttr_t()
	{
		m_nInteractsWith = 0;
		m_nInteractsExclude = 0;
		m_nInteractsAs = 0;
		
		m_nEntityIdsToIgnore[0] = -1;
		m_nEntityIdsToIgnore[1] = -1;
		
		m_nOwnerIdsToIgnore[0] = -1;
		m_nOwnerIdsToIgnore[1] = -1;
		
		m_nHierarchyIds[0] = 0;
		m_nHierarchyIds[1] = 0;
		
		m_nObjectSetMask = RNQUERY_OBJECTS_ALL;
		m_nCollisionGroup = COLLISION_GROUP_ALWAYS;
		
		m_bHitSolid = true;
		m_bHitSolidRequiresGenerateContacts = false;
		m_bHitTrigger = false;
		m_bShouldIgnoreDisabledPairs = true;
		m_bIgnoreIfBothInteractWithHitboxes = false;
		m_bForceHitEverything = false;
		m_bUnknown = true;
	}
	
	bool HasInteractsAsLayer( int nLayerIndex ) const		{ return ( m_nInteractsAs & ( 1ull << nLayerIndex ) ) != 0; }
	bool HasInteractsWithLayer( int nLayerIndex ) const		{ return ( m_nInteractsWith & ( 1ull << nLayerIndex ) ) != 0; }
	bool HasInteractsExcludeLayer( int nLayerIndex ) const	{ return ( m_nInteractsExclude & ( 1ull << nLayerIndex ) ) != 0; }
	void EnableInteractsAsLayer( int nLayer )				{ m_nInteractsAs |= ( 1ull << nLayer ); }
	void EnableInteractsWithLayer( int nLayer )				{ m_nInteractsWith |= ( 1ull << nLayer ); }
	void EnableInteractsExcludeLayer( int nLayer )			{ m_nInteractsExclude |= ( 1ull << nLayer ); }
	void DisableInteractsAsLayer( int nLayer )				{ m_nInteractsAs &= ~( 1ull << nLayer ); }
	void DisableInteractsWithLayer( int nLayer )			{ m_nInteractsWith &= ~( 1ull << nLayer ); }
	void DisableInteractsExcludeLayer( int nLayer )			{ m_nInteractsExclude &= ~( 1ull << nLayer ); }
	
public:
	// Which interaction layers do I interact or collide with? (e.g. I collide with LAYER_INDEX_CONTENTS_PASS_BULLETS because I am not a bullet)
	// NOTE: This is analogous to the "solid mask" or "trace mask" in source 1 (bit mask of CONTENTS_* or 1<<LAYER_INDEX_*)
	uint64 m_nInteractsWith;
	
	// Which interaction layers do I _not_ interact or collide with?  If my exclusion layers match m_nInteractsAs on the other object then no interaction happens.
	uint64 m_nInteractsExclude;
	
	// Which interaction layers do I represent? (e.g. I am a LAYER_INDEX_CONTENTS_PLAYER_CLIP volume)
	// NOTE: This is analogous to "contents" in source 1  (bit mask of CONTENTS_* or 1<<LAYER_INDEX_*)
	uint64 m_nInteractsAs;
	
	uint32 m_nEntityIdsToIgnore[2]; // this is the ID of the game entity which should be ignored
	uint32 m_nOwnerIdsToIgnore[2];	// this is the ID of the owner of the game entity which should be ignored
	uint16 m_nHierarchyIds[2];		// this is an ID for the hierarchy of game entities (used to disable collision among objects in a hierarchy)
	uint16 m_nObjectSetMask; 		// set of RnQueryObjectSet bits
	uint8 m_nCollisionGroup; 		// one of the registered collision groups

	bool m_bHitSolid : 1; 							// if true, then query will hit solid
	bool m_bHitSolidRequiresGenerateContacts : 1; 	// if true, the FCOLLISION_FUNC_ENABLE_SOLID_CONTACT flag will be checked to hit solid
	bool m_bHitTrigger : 1; 						// if true, then query will hit tirgger
	bool m_bShouldIgnoreDisabledPairs : 1; 			// if true, then ignores if the query and shape entity IDs are in collision pairs
	bool m_bIgnoreIfBothInteractWithHitboxes : 1;	// if true, then ignores if both query and shape interact with LAYER_INDEX_CONTENTS_HITBOX
	bool m_bForceHitEverything : 1;					// if true, will hit any objects without any conditions
	bool m_bUnknown : 1;							// haven't found where this is used yet
};

struct RnQueryAttr_t : public RnQueryShapeAttr_t
{
};

class CTraceFilter : public RnQueryAttr_t
{
public:
	CTraceFilter( int nCollisionGroup = COLLISION_GROUP_DEFAULT, bool bIterateEntities = true ) 
	{
		m_nCollisionGroup = nCollisionGroup;
		m_bIterateEntities = bIterateEntities;
	}
	
	CTraceFilter( uint64 nInteractsWith, int nCollisionGroup = COLLISION_GROUP_DEFAULT, bool bIterateEntities = true ) 
	{
		m_nInteractsWith = nInteractsWith;
		m_nCollisionGroup = nCollisionGroup;
		m_bIterateEntities = bIterateEntities;
	}

	CTraceFilter( 	CEntityInstance* pPassEntity, 
					CEntityInstance* pPassEntityOwner, 
					uint16 nHierarchyId,
					uint64 nInteractsWith, 
					int nCollisionGroup = COLLISION_GROUP_DEFAULT, 
					bool bIterateEntities = true ) 
	{
		SetPassEntity1( pPassEntity );
		
		SetPassEntityOwner1( pPassEntityOwner );

		m_nHierarchyIds[0] = nHierarchyId;
		
		m_nInteractsWith = nInteractsWith;
		m_nCollisionGroup = nCollisionGroup;
		m_bIterateEntities = bIterateEntities;
	}
	
	CTraceFilter( 	CEntityInstance* pPassEntity1, 
					CEntityInstance* pPassEntity2, 
					CEntityInstance* pPassEntityOwner1, 
					CEntityInstance* pPassEntityOwner2, 
					uint16 nHierarchyId1,
					uint16 nHierarchyId2,
					uint64 nInteractsWith, 
					int nCollisionGroup = COLLISION_GROUP_DEFAULT, 
					bool bIterateEntities = true ) 
	{
		SetPassEntity1( pPassEntity1 );
		SetPassEntity2( pPassEntity2 );
		
		SetPassEntityOwner1( pPassEntityOwner1 );
		SetPassEntiryOwner2( pPassEntityOwner2 );
		
		m_nHierarchyIds[0] = nHierarchyId1;
		m_nHierarchyIds[1] = nHierarchyId2;
		
		m_nInteractsWith = nInteractsWith;
		m_nCollisionGroup = nCollisionGroup;
		m_bIterateEntities = bIterateEntities;
	}
	
	void SetPassEntity1( CEntityInstance* pEntity ) { m_nEntityIdsToIgnore[0] = pEntity ? pEntity->GetRefEHandle().ToInt() : -1; }
	void SetPassEntity2( CEntityInstance* pEntity ) { m_nEntityIdsToIgnore[1] = pEntity ? pEntity->GetRefEHandle().ToInt() : -1; }
	
	void SetPassEntityOwner1( CEntityInstance* pOwner ) { m_nOwnerIdsToIgnore[0] = pOwner ? pOwner->GetRefEHandle().ToInt() : -1; }
	void SetPassEntiryOwner2( CEntityInstance* pOwner ) { m_nOwnerIdsToIgnore[1] = pOwner ? pOwner->GetRefEHandle().ToInt() : -1; }
	
	virtual ~CTraceFilter() {}
	virtual bool ShouldHitEntity( CEntityInstance* pEntity ) { return true; }
	
public:
	bool m_bIterateEntities; // if true then calls ShouldHitEntity for each hit entity
};

struct RnCollisionAttr_t
{
public:
	RnCollisionAttr_t()
	{
		m_nInteractsAs = 0;
		m_nInteractsWith = 0;
		m_nInteractsExclude = 0;
		m_nEntityId = 0;
		m_nOwnerId = -1;
		m_nHierarchyId = 0;
		m_nCollisionGroup = COLLISION_GROUP_ALWAYS;
		m_nCollisionFunctionMask = FCOLLISION_FUNC_DEFAULT;
	}
	
	bool IsSolidContactEnabled() const						{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_ENABLE_SOLID_CONTACT ) != 0; }
	bool IsTraceAndQueryEnabled() const						{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_ENABLE_TRACE_QUERY ) != 0; }
	bool IsTouchEventEnabled() const						{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_ENABLE_TOUCH_EVENT ) != 0; }
	bool IsSelfCollisionsEnabled() const					{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_ENABLE_SELF_COLLISIONS ) != 0; }
	bool ShouldIgnoreForHitboxTest() const					{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_IGNORE_FOR_HITBOX_TEST ) != 0; }
	bool IsTouchPersistsEnabled() const						{ return ( m_nCollisionFunctionMask & FCOLLISION_FUNC_ENABLE_TOUCH_PERSISTS ) != 0; }

	bool HasInteractsAsLayer( int nLayerIndex ) const		{ return ( m_nInteractsAs & ( 1ull << nLayerIndex ) ) != 0; }
	bool HasInteractsWithLayer( int nLayerIndex ) const		{ return ( m_nInteractsWith & ( 1ull << nLayerIndex ) ) != 0; }
	bool HasInteractsExcludeLayer( int nLayerIndex ) const	{ return ( m_nInteractsExclude & ( 1ull << nLayerIndex ) ) != 0; }
	void EnableInteractsAsLayer( int nLayer )				{ m_nInteractsAs |= ( 1ull << nLayer ); }
	void EnableInteractsWithLayer( int nLayer )				{ m_nInteractsWith |= ( 1ull << nLayer ); }
	void EnableInteractsExcludeLayer( int nLayer )			{ m_nInteractsExclude |= ( 1ull << nLayer ); }
	void DisableInteractsAsLayer( int nLayer )				{ m_nInteractsAs &= ~( 1ull << nLayer ); }
	void DisableInteractsWithLayer( int nLayer )			{ m_nInteractsWith &= ~( 1ull << nLayer ); }
	void DisableInteractsExcludeLayer( int nLayer )			{ m_nInteractsExclude &= ~( 1ull << nLayer ); }
	
public:
	// Which interaction layers do I represent? (e.g. I am a LAYER_INDEX_CONTENTS_PLAYER_CLIP volume)
	// NOTE: This is analogous to "contents" in source 1  (bit mask of CONTENTS_* or 1<<LAYER_INDEX_*)
	uint64 m_nInteractsAs;
	
	// Which interaction layers do I interact or collide with? (e.g. I collide with LAYER_INDEX_CONTENTS_PASS_BULLETS because I am not a bullet)
	// NOTE: This is analogous to the "solid mask" or "trace mask" in source 1 (bit mask of CONTENTS_* or 1<<LAYER_INDEX_*)
	uint64 m_nInteractsWith;
	
	// Which interaction layers do I _not_ interact or collide with?  If my exclusion layers match m_nInteractsAs on the other object then no interaction happens.
	uint64 m_nInteractsExclude;
	
	uint32 m_nEntityId; 			// this is the ID of the game entity
	uint32 m_nOwnerId;  			// this is the ID of the owner of the game entity
	uint16 m_nHierarchyId; 			// this is an ID for the hierarchy of game entities (used to disable collision among objects in a hierarchy)
	uint8 m_nCollisionGroup;		// one of the registered collision groups
	uint8 m_nCollisionFunctionMask;	// set of CollisionFunctionMask_t bits
};

class CGameTrace
{
public:
	CGameTrace()
	{
		Init();
	}

	void Init()
	{
		m_pSurfaceProperties = NULL;
		m_pEnt = NULL;
		m_pHitbox = NULL;
		m_hBody = NULL;
		m_hShape = NULL;
		m_nContents = 0;
		m_BodyTransform.SetToIdentity();
		m_vHitNormal.Init();
		m_vHitPoint.Init();
		m_flHitOffset = 0.0f;
		m_flFraction = 1.0f;
		m_nTriangle = -1;
		m_nHitboxBoneIndex = -1;
		m_eRayType = RAY_TYPE_LINE;
		m_bStartInSolid = false;
		m_bExactHitPoint = false;
	}
	
	// Returns true if there was any kind of impact at all
	bool DidHit() const 
	{ 
		return m_flFraction < 1 || m_bStartInSolid; 
	}
	
public:
	const CPhysSurfaceProperties *m_pSurfaceProperties;
	CEntityInstance *m_pEnt;
	const CHitBox *m_pHitbox;
	
	HPhysicsBody m_hBody;
	HPhysicsShape m_hShape;
	
	uint64 m_nContents;					// contents on other side of surface hit
	
	CTransform m_BodyTransform;
	RnCollisionAttr_t m_ShapeAttributes;
	
	Vector m_vStartPos; 				// start position
	Vector m_vEndPos; 					// final position
	Vector m_vHitNormal; 				// surface normal at impact
	Vector m_vHitPoint;					// exact hit point if m_bExactHitPoint is true, otherwise equal to m_vEndPos
	
	float m_flHitOffset;				// surface normal hit offset
	float m_flFraction;					// time completed, 1.0 = didn't hit anything
	
	int32 m_nTriangle;					// the index of the triangle that was hit
	int16 m_nHitboxBoneIndex; 			// the index of the hitbox bone that was hit
	
	RayType_t m_eRayType;
	
	bool m_bStartInSolid;				// if true, the initial point was in a solid area
	bool m_bExactHitPoint;				// if true, then m_vHitPoint is the exact hit point of the query and the shape
	
private:
	// No copy constructors allowed
	CGameTrace(const CGameTrace& vOther);
};


typedef CGameTrace trace_t;

//=============================================================================

class ITraceListData
{
public:
	virtual ~ITraceListData() {}

	virtual void Reset() = 0;
	virtual bool IsEmpty() = 0;
	// CanTraceRay will return true if the current volume encloses the ray
	// NOTE: The leaflist trace will NOT check this.  Traces are intersected
	// against the culled volume exclusively.
	virtual bool CanTraceRay( const Ray_t &ray ) = 0;
};
#endif // GAMETRACE_H

