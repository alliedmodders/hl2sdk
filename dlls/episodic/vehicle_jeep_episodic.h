//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VEHICLE_JEEP_EPISODIC_H
#define VEHICLE_JEEP_EPISODIC_H
#ifdef _WIN32
#pragma once
#endif

#include "vehicle_jeep.h"
#include "ai_basenpc.h"

//=============================================================================
// Episodic jeep

class CPropJeepEpisodic : public CPropJeep
{
	DECLARE_CLASS( CPropJeepEpisodic, CPropJeep );

public:
					CPropJeepEpisodic( void );

	virtual void	Spawn( void );
	virtual void	NPC_FinishedEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	virtual void	NPC_FinishedExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	
	virtual bool	NPC_CanEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );
	virtual bool	NPC_CanExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion );

	DECLARE_DATADESC();

protected:
	virtual float			GetUprightTime( void ) { return 1.0f; }
	virtual float			GetUprightStrength( void );
	virtual bool			ShouldPuntUseLaunchForces( void );
	virtual AngularImpulse	PhysGunLaunchAngularImpulse( void );
	virtual Vector			PhysGunLaunchVelocity( const Vector &forward, float flMass );
	bool					PassengerInTransition( void );

private:
	
	void			InputLockEntrance( inputdata_t &data );
	void			InputUnlockEntrance( inputdata_t &data );
	void			InputLockExit( inputdata_t &data );
	void			InputUnlockExit( inputdata_t &data );

	bool			m_bEntranceLocked;
	bool			m_bExitLocked;

	COutputEvent	m_OnCompanionEnteredVehicle;	// Passenger has completed entering the vehicle
	COutputEvent	m_OnCompanionExitedVehicle;		// Passenger has completed exited the vehicle
	COutputEvent	m_OnHostileEnteredVehicle;	// Passenger has completed entering the vehicle
	COutputEvent	m_OnHostileExitedVehicle;		// Passenger has completed exited the vehicle
};

#endif // VEHICLE_JEEP_EPISODIC_H
