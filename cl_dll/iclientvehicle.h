//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICLIENTVEHICLE_H
#define ICLIENTVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"

class C_BasePlayer;
class Vector;
class QAngle;
class C_BaseEntity;


//-----------------------------------------------------------------------------
// Purpose: All client vehicles must implement this interface.
//-----------------------------------------------------------------------------
abstract_class IClientVehicle : public IVehicle
{
public:
	// When a player is in a vehicle, here's where the camera will be
	virtual void GetVehicleFOV( float &flFOV ) = 0;

	// Allows the vehicle to restrict view angles, blend, etc.
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) = 0;

	// Hud redraw...
	virtual void DrawHudElements() = 0;

	// Is this predicted?
	virtual bool IsPredicted() const = 0;

	// Get the entity associated with the vehicle.
	virtual C_BaseEntity *GetVehicleEnt() = 0;

	// Allows the vehicle to change the near clip plane
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const = 0;

#ifdef HL2_CLIENT_DLL
	// Ammo in the vehicles
	virtual int GetPrimaryAmmoType() const = 0;
	virtual int GetPrimaryAmmoClip() const = 0;
	virtual bool PrimaryAmmoUsesClips() const = 0;
	virtual int GetPrimaryAmmoCount() const = 0;
#endif
};


//==========================================================================================
// VEHICLE VIEW SMOOTHING CODE
//==========================================================================================

// If we enter the linear part of the remap for curve for any degree of freedom, we can lock
// that DOF (stop remapping). This is useful for making flips feel less spastic as we oscillate
// randomly between different parts of the remapping curve.
struct ViewLockData_t
{
	float	flLockInterval;			// The duration to lock the view when we lock it for this degree of freedom.
									// 0 = never lock this degree of freedom.

	bool	bLocked;				// True if this DOF was locked because of the above condition.

	float	flUnlockTime;			// If this DOF is locked, the time when we will unlock it.

	float	flUnlockBlendInterval;	// If this DOF is locked, how long to spend blending out of the locked view when we unlock.
};


// This is separate from the base vehicle implementation so that any class 
// that derives from IClientVehicle can use it. To use it, contain one of the
// following structs, fill out the first section, and then call VehicleViewSmoothing()
// inside your GetVehicleViewPosition() function.
struct ViewSmoothingData_t
{
	DECLARE_SIMPLE_DATADESC();

	// Fill these out in your vehicle
	CBaseAnimating	*pVehicle;
	bool	bClampEyeAngles;	// Perform eye Z clamping
	float	flPitchCurveZero;	// Pitch values below this are clamped to zero.
	float	flPitchCurveLinear;	// Pitch values above this are mapped directly.
								//		Spline in between.
	float	flRollCurveZero;	// Pitch values below this are clamped to zero.
	float	flRollCurveLinear;	// Roll values above this are mapped directly.
								//		Spline in between.
	float	flFOV;				// FOV when in the vehicle.

	ViewLockData_t pitchLockData;
	ViewLockData_t rollLockData;

	bool	bDampenEyePosition;	// Only set to true for C_PropVehicleDriveable derived vehicles

	// Don't change these, they're used by VehicleViewSmoothing()
	bool	bRunningEnterExit;
	bool	bWasRunningAnim;
	float	flAnimTimeElapsed;
	float	flEnterExitDuration;
	QAngle	vecAnglesSaved;
	Vector	vecOriginSaved;
	QAngle	vecAngleDiffSaved;	// The original angular error between the entry/exit anim and player's view when we started playing the anim.
	QAngle	vecAngleDiffMin;	// Tracks the minimum angular error achieved so we can converge on the anim's angles.
};

void VehicleViewSmoothing( CBasePlayer *pPlayer, Vector *pAbsOrigin, QAngle *pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector *vecEyeExitEndpoint, ViewSmoothingData_t *pData, float *pFOV );

#endif // ICLIENTVEHICLE_H
