//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef AI_BEHAVIOR_PASSENGER_COMPANION_H
#define AI_BEHAVIOR_PASSENGER_COMPANION_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior_passenger.h"

struct VehicleAvoidParams_t
{
	Vector vecStartPos;
	Vector vecGoalPos;
	Vector *pNodePositions;
	int nNumNodes;
	int nDirection;
	int nStartNode;
	int nEndNode;
};

struct FailPosition_t
{
	Vector	vecPosition;
	float	flTime;

	DECLARE_SIMPLE_DATADESC();
};

class CAI_PassengerBehaviorCompanion : public CAI_PassengerBehavior
{
	DECLARE_CLASS( CAI_PassengerBehaviorCompanion, CAI_PassengerBehavior );
	DECLARE_DATADESC()

public:

	CAI_PassengerBehaviorCompanion( void );

	enum
	{
		// Schedules
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE = BaseClass::NEXT_SCHEDULE,
		SCHED_PASSENGER_RUN_TO_ENTER_VEHICLE_FAILED,
		SCHED_PASSENGER_ENTER_VEHICLE_PAUSE,
		SCHED_PASSENGER_RANGE_ATTACK1,
		SCHED_PASSENGER_RELOAD,
		SCHED_PASSENGER_EXIT_STUCK_VEHICLE,
		SCHED_PASSENGER_OVERTURNED,
		SCHED_PASSENGER_IMPACT,
		NEXT_SCHEDULE,

		// Tasks
		TASK_GET_PATH_TO_VEHICLE_ENTRY_POINT = BaseClass::NEXT_TASK,
		TASK_GET_PATH_TO_NEAR_VEHICLE,
		TASK_PASSENGER_RELOAD,
		TASK_PASSENGER_EXIT_STUCK_VEHICLE,
		TASK_PASSENGER_OVERTURNED,
		TASK_PASSENGER_IMPACT,
		NEXT_TASK,

		// Conditions
		COND_HARD_IMPACT = BaseClass::NEXT_CONDITION,
		COND_VEHICLE_MOVED_FROM_MARK,
		COND_VEHICLE_STOPPED,
		COND_CAN_LEAVE_STUCK_VEHICLE,
		COND_WARN_OVERTURNED,
		NEXT_CONDITION,
	};

	virtual void	GatherConditions( void );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	AimGun( void );
	virtual void	EnterVehicle( void );
	virtual void	FinishEnterVehicle( void );
	virtual void	FinishExitVehicle( void );
	virtual Activity	NPC_TranslateActivity( Activity activity );
	virtual void	BuildScheduleTestBits( void );
	virtual bool	OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

private:

	virtual void	OnExitVehicleFailed( void );

	bool	BuildVehicleAvoidancePath( CBaseEntity *pVehicle, const Vector &vecMoveDir );
	int		SelectScheduleInsideVehicle( void );
	int		SelectScheduleOutsideVehicle( void );
	bool	FindPathToVehicleEntryPoint( void );

	// ------------------------------------------
	//  Passenger sensing
	// ------------------------------------------

	virtual void	GatherVehicleStateConditions( void );

	float	GetVehicleSpeed( void );
	void	GatherVehicleCollisionConditions( const Vector &localVelocity );

	// ------------------------------------------
	//  Overturned tracking
	// ------------------------------------------
	void	UpdateStuckStatus( void );
	bool	CanExitAtPosition( const Vector &vecTestPos );
	bool	GetStuckExitPos( Vector *vecResult );
	bool	ExitStuckVehicle( void );

	bool			AppendLocalPath( const Vector &vecStart, const Vector &vecEnd, AI_Waypoint_t **pWaypoints );
	AI_Waypoint_t	*BuildNodeRouteAroundVehicle( const VehicleAvoidParams_t &avoidParams );

	bool			PointIsWithinEntryFailureRadius( const Vector &vecPosition );
	void			ResetVehicleEntryFailedState( void );
	void			MarkVehicleEntryFailed( const Vector &vecPosition );
	virtual int		FindEntrySequence( bool bNearest = false );

	float	m_flNextOverturnWarning; // The next time the NPC may complained about being upside-down
	float	m_flOverturnedDuration;	// Amount of time we've been stuck in the vehicle (unable to exit)
	float	m_flUnseenDuration; // Amount of time we've been hidden from the player's view
	int		m_nExitAttempts;	// Number of times we've attempted to exit the vehicle but failed

	CAI_MoveMonitor				m_VehicleMonitor;		// Used to keep track of the vehicle's movement relative to a mark
	CUtlVector<FailPosition_t>	m_FailedEntryPositions;	// Used to keep track of the vehicle's movement relative to a mark

protected:
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};

#endif // AI_BEHAVIOR_PASSENGER_COMPANION_H
