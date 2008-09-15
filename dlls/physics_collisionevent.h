//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Pulling CCollisionEvent's definition out of physics.cpp so it can be abstracted upon 
//			
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef PHYSICS_COLLISIONEVENT_H
#define PHYSICS_COLLISIONEVENT_H

#ifdef _WIN32
#pragma once
#endif

#include "physics.h"

struct damageevent_t
{
	CBaseEntity		*pEntity;
	IPhysicsObject	*pInflictorPhysics;
	CTakeDamageInfo	info;
	bool			bRestoreVelocity;
};

struct inflictorstate_t
{
	Vector			savedVelocity;
	AngularImpulse	savedAngularVelocity;
	IPhysicsObject	*pInflictorPhysics;
	float			otherMassMax;
	short			nextIndex;
	short			restored;
};

struct impulseevent_t
{
	IPhysicsObject	*pObject;
	Vector vecCenterForce;
	Vector vecCenterTorque;
};

struct velocityevent_t
{
	IPhysicsObject	*pObject;
	Vector vecVelocity;
};

enum
{
	COLLSTATE_ENABLED = 0,
	COLLSTATE_TRYDISABLE = 1,
	COLLSTATE_TRYNPCSOLVER = 2,
	COLLSTATE_DISABLED = 3
};

struct penetrateevent_t
{
	EHANDLE			hEntity0;
	EHANDLE			hEntity1;
	float			startTime;
	float			timeStamp;
	int				collisionState;
};

class CCollisionEvent : public IPhysicsCollisionEvent, public IPhysicsCollisionSolver, public IPhysicsObjectEvent
{
public:
	CCollisionEvent();
	friction_t *FindFriction( CBaseEntity *pObject );
	void ShutdownFriction( friction_t &friction );
	void FrameUpdate();
	void LevelShutdown( void );

	// IPhysicsCollisionEvent
	void PreCollision( vcollisionevent_t *pEvent );
	void PostCollision( vcollisionevent_t *pEvent );
	void Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData );
	void StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );
	void EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );
	void FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void PostSimulationFrame();
	void ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject );
	void ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject );
	
	bool GetTriggerEvent( triggerevent_t *pEvent, CBaseEntity *pTriggerEntity );
	void BufferTouchEvents( bool enable ) { m_bBufferTouchEvents = enable; }
	void AddDamageEvent( CBaseEntity *pEntity, const CTakeDamageInfo &info, IPhysicsObject *pInflictorPhysics, bool bRestoreVelocity, const Vector &savedVel, const AngularImpulse &savedAngVel );
	void AddImpulseEvent( IPhysicsObject *pPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque );
	void AddSetVelocityEvent( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity );
	void AddRemoveObject(IServerNetworkable *pRemove);

	// IPhysicsCollisionSolver
	int		ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 );
	int		ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt );
	bool	ShouldFreezeObject( IPhysicsObject *pObject ) { return true; }
	int		AdditionalCollisionChecksThisTick( int currentChecksDone ) 
	{
		//CallbackContext check(this);
		if ( currentChecksDone < 1200 )
		{
			DevMsg(1,"VPhysics Collision detection getting expensive, check for too many convex pieces!\n");
			return 1200 - currentChecksDone;
		}
		DevMsg(1,"VPhysics exceeded collision check limit (%d)!!!\nInterpenetration may result!\n", currentChecksDone );
		return 0; 
	}

	// IPhysicsObjectEvent
	// these can be used to optimize out queries on sleeping objects
	// Called when an object is woken after sleeping
	virtual void ObjectWake( IPhysicsObject *pObject ) {}
	// called when an object goes to sleep (no longer simulating)
	virtual void ObjectSleep( IPhysicsObject *pObject ) {}


	// locals
	bool GetInflictorVelocity( IPhysicsObject *pInflictor, Vector &velocity, AngularImpulse &angVelocity );

	void GetListOfPenetratingEntities( CBaseEntity *pSearch, CUtlVector<CBaseEntity *> &list );
	bool IsInCallback() { return m_inCallback > 0 ? true : false; }

private:
	void UpdateFrictionSounds();
	void UpdateTouchEvents();
	void UpdateDamageEvents();
	void UpdateImpulseEvents();
	void UpdateSetVelocityEvents();
	void UpdatePenetrateEvents( void );
	void UpdateFluidEvents();
	void UpdateRemoveObjects();
	void AddTouchEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal );
	penetrateevent_t &FindOrAddPenetrateEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
	float DeltaTimeSinceLastFluid( CBaseEntity *pEntity );

	void RestoreDamageInflictorState( IPhysicsObject *pInflictor );
	void RestoreDamageInflictorState( int inflictorStateIndex, float velocityBlend );
	int AddDamageInflictor( IPhysicsObject *pInflictorPhysics, float otherMass, const Vector &savedVel, const AngularImpulse &savedAngVel, bool addList );
	int	FindDamageInflictor( IPhysicsObject *pInflictorPhysics );

	// make the call into the entity system
	void DispatchStartTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1, const Vector &point, const Vector &normal );
	void DispatchEndTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
	
	class CallbackContext
	{
	public:
		CallbackContext(CCollisionEvent *pOuter)
		{
			m_pOuter = pOuter;
			m_pOuter->m_inCallback++;
		}
		~CallbackContext()
		{
			m_pOuter->m_inCallback--;
		}
	private:
		CCollisionEvent *m_pOuter;
	};
	friend class CallbackContext;
	
	friction_t					m_current[4];
	gamevcollisionevent_t		m_gameEvent;
	CUtlVector<triggerevent_t>	m_triggerEvents;
	triggerevent_t				m_currentTriggerEvent;
	CUtlVector<touchevent_t>	m_touchEvents;
	CUtlVector<damageevent_t>	m_damageEvents;
	CUtlVector<inflictorstate_t>	m_damageInflictors;
	CUtlVector<penetrateevent_t> m_penetrateEvents;
	CUtlVector<fluidevent_t>	m_fluidEvents;
	CUtlVector<impulseevent_t>	m_impulseEvents;
	CUtlVector<velocityevent_t>	m_setVelocityEvents;
	CUtlVector<IServerNetworkable *> m_removeObjects;
	int							m_inCallback;
	bool						m_bBufferTouchEvents;
};

#endif //#ifndef PHYSICS_COLLISIONEVENT_H
