//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "c_prop_vehicle.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "view.h"
#include "engine/IVDebugOverlay.h"
#include "movevars_shared.h"
#include "iviewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int ScreenTransform( const Vector& point, Vector& screen );


extern ConVar default_fov;


BEGIN_SIMPLE_DATADESC( ViewSmoothingData_t )
	DEFINE_FIELD( vecAnglesSaved, FIELD_VECTOR ),
	DEFINE_FIELD( vecOriginSaved, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( vecAngleDiffSaved, FIELD_VECTOR ),
	DEFINE_FIELD( vecAngleDiffMin, FIELD_VECTOR ),
	DEFINE_FIELD( bRunningEnterExit, FIELD_BOOLEAN ),
	DEFINE_FIELD( bWasRunningAnim, FIELD_BOOLEAN ),
	DEFINE_FIELD( flAnimTimeElapsed, FIELD_FLOAT ),
	DEFINE_FIELD( flEnterExitDuration, FIELD_FLOAT ),

	// These are filled out in the vehicle's constructor:
	//CBaseAnimating	*pVehicle;
	//bool	bClampEyeAngles;
	//float	flPitchCurveZero;
	//float	flPitchCurveLinear;
	//float	flRollCurveZero;
	//float	flRollCurveLinear;
	//ViewLockData_t pitchLockData;
	//ViewLockData_t rollLockData;
	//bool bDampenEyePosition;
END_DATADESC()


IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleDriveable, DT_PropVehicleDriveable, CPropVehicleDriveable)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropInt( RECVINFO( m_nRPM ) ),
	RecvPropFloat( RECVINFO( m_flThrottle ) ),
	RecvPropInt( RECVINFO( m_nBoostTimeLeft ) ),
	RecvPropInt( RECVINFO( m_nHasBoost ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledWeapons ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledVehicle ) ),
	RecvPropInt( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropInt( RECVINFO( m_bExitAnimOn ) ),
	RecvPropInt( RECVINFO( m_bUnableToFire ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropBool( RECVINFO( m_bHasGun ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropVehicleDriveable )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()


#define ROLL_CURVE_ZERO		20		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	90		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

ConVar r_VehicleViewClamp( "r_VehicleViewClamp", "1", FCVAR_CHEAT );


// remaps an angular variable to a 3 band function:
// 0 <= t < start :		f(t) = 0
// start <= t <= end :	f(t) = end * spline(( t-start) / (end-start) )  // s curve between clamped and linear
// end < t :			f(t) = t
float RemapAngleRange( float startInterval, float endInterval, float value, RemapAngleRange_CurvePart_t *peCurvePart )
{
	// Fixup the roll
	value = AngleNormalize( value );
	float absAngle = fabs(value);

	// beneath cutoff?
	if ( absAngle < startInterval )
	{
		if ( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Zero;
		}
		value = 0;
	}
	// in spline range?
	else if ( absAngle <= endInterval )
	{
		float newAngle = SimpleSpline( (absAngle - startInterval) / (endInterval-startInterval) ) * endInterval;

		// grab the sign from the initial value
		if ( value < 0 )
		{
			newAngle *= -1;
		}

		if ( peCurvePart )
		{
			*peCurvePart = RemapAngleRange_CurvePart_Spline;
		}
		value = newAngle;
	}
	// else leave it alone, in linear range
	else if ( peCurvePart )
	{
		*peCurvePart = RemapAngleRange_CurvePart_Linear;
	}

	return value;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::C_PropVehicleDriveable() :
	m_iv_vecGunCrosshair( "C_PropVehicleDriveable::m_iv_vecGunCrosshair" )

{
	m_hPrevPlayer = NULL;

	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.bClampEyeAngles = true;
	m_ViewSmoothingData.bDampenEyePosition = true;

	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;

	m_flFOV = 0.0f;

	AddVar( &m_vecGunCrosshair, &m_iv_vecGunCrosshair, LATCH_SIMULATION_VAR );
}

//-----------------------------------------------------------------------------
// Purpose: De-constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::~C_PropVehicleDriveable()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehicleDriveable::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleDriveable::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_hPlayer && !m_hPrevPlayer )
	{
		OnEnteredVehicle( m_hPlayer );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
	else if ( !m_hPlayer && m_hPrevPlayer )
	{
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		m_ViewSmoothingData.bWasRunningAnim = false;
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

//-----------------------------------------------------------------------------
// Should this object cast render-to-texture shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_PropVehicleDriveable::ShadowCastType()
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	// Always use render-to-texture. We'll always the dirty bits in our think function
	return SHADOWS_RENDER_TO_TEXTURE;
}


//-----------------------------------------------------------------------------
// Mark the shadow as dirty while the vehicle is being driven
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::ClientThink( void )
{
	// The vehicle is always dirty owing to pose parameters while it's being driven.
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetShadowHandle() );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	VehicleViewSmoothing( m_hPlayer, pAbsOrigin, pAbsAngles, m_bEnterAnimOn, m_bExitAnimOn, &m_vecEyeExitEndpoint, &m_ViewSmoothingData, &m_flFOV );
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Simply used to return intensity value based upon current timer passed in
//-----------------------------------------------------------------------------
int GetFlashColorIntensity( int LowIntensity, int HighIntensity, bool Dimming, int Increment, int Timer )
{
	if ( Dimming ) 
		return ( HighIntensity - Timer * Increment );
	else
		return ( LowIntensity + Timer * Increment );
}

#define	TRIANGULATED_CROSSHAIR 1

void C_PropVehicleDriveable::DrawHudElements( )
{
	CHudTexture *pIcon;
	int iIconX, iIconY;

	if (m_bHasGun)
	{
		// draw crosshairs for vehicle gun
		pIcon = gHUD.GetIcon( "gunhair" );

		if ( pIcon != NULL )
		{
			float x, y;
			Vector screen;

			x = ScreenWidth()/2;
			y = ScreenHeight()/2;

		#if TRIANGULATED_CROSSHAIR
			ScreenTransform( m_vecGunCrosshair, screen );
			x += 0.5 * screen[0] * ScreenWidth() + 0.5;
			y -= 0.5 * screen[1] * ScreenHeight() + 0.5;
		#endif

			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
			
			Color	clr = ( m_bUnableToFire ) ? gHUD.m_clrCaution : gHUD.m_clrNormal;
			pIcon->DrawSelf( x, y, clr );
		}

		if ( m_nScannerDisabledWeapons )
		{
			// Draw icons for scanners "weps disabled"  
			pIcon = gHUD.GetIcon( "dmg_bio" );
			if ( pIcon )
			{
				iIconY = 467 - pIcon->Height() / 2;
				iIconX = 385;
				if ( !m_bScannerWepIcon )
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
					m_bScannerWepIcon = true;
					m_iScannerWepFlashTimer = 0;
					m_bScannerWepDim = true;
				}
				else
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerWepDim, 10, m_iScannerWepFlashTimer), 255 ) );
					m_iScannerWepFlashTimer++;
					m_iScannerWepFlashTimer %= 20;
					if(!m_iScannerWepFlashTimer)
						m_bScannerWepDim ^= 1;
				}
			}
		}
	}

	if ( m_nScannerDisabledVehicle )
	{
		// Draw icons for scanners "vehicle disabled"  
		pIcon = gHUD.GetIcon( "dmg_bio" );
		if ( pIcon )
		{
			iIconY = 467 - pIcon->Height() / 2;
			iIconX = 410;
			if ( !m_bScannerVehicleIcon )
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
				m_bScannerVehicleIcon = true;
				m_iScannerVehicleFlashTimer = 0;
				m_bScannerVehicleDim = true;
			}
			else
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerVehicleDim, 10, m_iScannerVehicleFlashTimer), 255 ) );
				m_iScannerVehicleFlashTimer++;
				m_iScannerVehicleFlashTimer %= 20;
				if(!m_iScannerVehicleFlashTimer)
					m_bScannerVehicleDim ^= 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	if ( pYawBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.y, vehicleEyeAngles.y );
		flAngleDiff = clamp( flAngleDiff, pYawBounds[0], pYawBounds[1] );
		vecViewAngles.y = vehicleEyeAngles.y + flAngleDiff;
	}

	// Limit the pitch.
	if ( pPitchBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.x, vehicleEyeAngles.x );
		flAngleDiff = clamp( flAngleDiff, pPitchBounds[0], pPitchBounds[1] );
		vecViewAngles.x = vehicleEyeAngles.x + flAngleDiff;
	}

	// Limit the roll.
	if ( pRollBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.z, vehicleEyeAngles.z );
		flAngleDiff = clamp( flAngleDiff, pRollBounds[0], pRollBounds[1] );
		vecViewAngles.z = vehicleEyeAngles.z + flAngleDiff;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_VehicleViewClamp.GetInt() )
	{
		float pitchBounds[2] = { -85.0f, 25.0f };
		RestrictView( NULL, pitchBounds, NULL, pCmd->viewangles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnEnteredVehicle( C_BaseCombatCharacter *pPassenger )
{
}

//=============================================================================================
// VEHICLE VIEW SMOOTHING. See iclientvehicle.h for details.
//=============================================================================================

//-----------------------------------------------------------------------------
// Purpose: For a given degree of freedom, blends between the raw and clamped
//			view depending on this vehicle's preferences. When vehicles wreck
//			catastrophically, it's often better to lock the view for a little
//			while until things settle down than to keep trying to clamp/flatten
//			the view artificially because we can never really catch up with
//			the chaotic flipping.
//-----------------------------------------------------------------------------
float ApplyViewLocking( float flAngleRaw, float flAngleClamped, ViewLockData_t &lockData, RemapAngleRange_CurvePart_t eCurvePart )
{
	// If we're set up to never lock this degree of freedom, return the clamped value.
	if ( lockData.flLockInterval == 0 )
		return flAngleClamped;

	float flAngleOut = flAngleClamped;

	// Lock the view if we're in the linear part of the curve, and keep it locked
	// until some duration after we return to the flat (zero) part of the curve.
	if ( ( eCurvePart == RemapAngleRange_CurvePart_Linear ) ||
		 ( lockData.bLocked && ( eCurvePart == RemapAngleRange_CurvePart_Spline ) ) )
	{
		//Msg( "LOCKED\n" );
		lockData.bLocked = true;
		lockData.flUnlockTime = gpGlobals->curtime + lockData.flLockInterval;
		flAngleOut = flAngleRaw;
	}
	else
	{
		if ( ( lockData.bLocked ) && ( gpGlobals->curtime > lockData.flUnlockTime ) )
		{
			lockData.bLocked = false;
			if ( lockData.flUnlockBlendInterval > 0 )
			{
				lockData.flUnlockTime = gpGlobals->curtime;
			}
			else
			{
				lockData.flUnlockTime = 0;
			}
		}

		if ( !lockData.bLocked )
		{
			if ( lockData.flUnlockTime != 0 )
			{
				// Blend out from the locked raw view (no remapping) to a remapped view.
				float flBlend = RemapValClamped( gpGlobals->curtime - lockData.flUnlockTime, 0, lockData.flUnlockBlendInterval, 0, 1 );
				//Msg( "BLEND %f\n", flBlend );

				flAngleOut = Lerp( flBlend, flAngleRaw, flAngleClamped );
				if ( flBlend >= 1.0f )
				{
					lockData.flUnlockTime = 0;
				}
			}
			else
			{
				// Not blending out from a locked view to a remapped view.
				//Msg( "CLAMPED\n" );
				flAngleOut = flAngleClamped;
			}
		}
		else
		{
			//Msg( "STILL LOCKED\n" );
			flAngleOut = flAngleRaw;
		}
	}

	return flAngleOut;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pData - 
//			vehicleEyeAngles - 
//-----------------------------------------------------------------------------
void RemapViewAngles( ViewSmoothingData_t *pData, QAngle &vehicleEyeAngles )
{
	QAngle vecEyeAnglesRemapped;

	// Clamp pitch.
	RemapAngleRange_CurvePart_t ePitchCurvePart;
	vecEyeAnglesRemapped.x = RemapAngleRange( pData->flPitchCurveZero, pData->flPitchCurveLinear, vehicleEyeAngles.x, &ePitchCurvePart );

	vehicleEyeAngles.z = vecEyeAnglesRemapped.z = AngleNormalize( vehicleEyeAngles.z );

	// Blend out the roll dampening as our pitch approaches 90 degrees, to avoid gimbal lock problems.
	float flBlendRoll = 1.0;
	if ( fabs( vehicleEyeAngles.x ) > 60 )
	{
		flBlendRoll = RemapValClamped( fabs( vecEyeAnglesRemapped.x ), 60, 80, 1, 0);
	}

	RemapAngleRange_CurvePart_t eRollCurvePart;
	float flRollDamped = RemapAngleRange( pData->flRollCurveZero, pData->flRollCurveLinear, vecEyeAnglesRemapped.z, &eRollCurvePart );
	vecEyeAnglesRemapped.z = Lerp( flBlendRoll, vecEyeAnglesRemapped.z, flRollDamped );

	//Msg("PITCH ");
	vehicleEyeAngles.x = ApplyViewLocking( vehicleEyeAngles.x, vecEyeAnglesRemapped.x, pData->pitchLockData, ePitchCurvePart );

	//Msg("ROLL ");
	vehicleEyeAngles.z = ApplyViewLocking( vehicleEyeAngles.z, vecEyeAnglesRemapped.z, pData->rollLockData, eRollCurvePart );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAbsOrigin - 
//			*pAbsAngles - 
//			bEnterAnimOn - 
//			bExitAnimOn - 
//			*vecEyeExitEndpoint - 
//			*pData - 
//-----------------------------------------------------------------------------
void VehicleViewSmoothing( CBasePlayer *pPlayer, Vector *pAbsOrigin, QAngle *pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector *vecEyeExitEndpoint, ViewSmoothingData_t *pData,
							float *pFOV )
{
	int eyeAttachmentIndex = pData->pVehicle->LookupAttachment( "vehicle_driver_eyes" );
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	pData->pVehicle->GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

	// Dampen the eye positional change as we drive around.
	*pAbsAngles = pPlayer->EyeAngles();
	if ( r_VehicleViewDampen.GetInt() && pData->bDampenEyePosition )
	{
		C_PropVehicleDriveable *pDriveable = assert_cast<C_PropVehicleDriveable*>(pData->pVehicle);
		pDriveable->DampenEyePosition( vehicleEyeOrigin, vehicleEyeAngles );
	}

	// Started running an entry or exit anim?
	bool bRunningAnim = ( bEnterAnimOn || bExitAnimOn );
	if ( bRunningAnim && !pData->bWasRunningAnim )
	{
		pData->bRunningEnterExit = true;
		pData->flAnimTimeElapsed = 0.01;
		pData->flEnterExitDuration = pData->pVehicle->SequenceDuration( pData->pVehicle->GetSequence() );

		pData->vecAnglesSaved = PrevMainViewAngles();
		pData->vecOriginSaved = PrevMainViewOrigin();

		// Save our initial angular error, which we will blend out over the length of the animation.
		pData->vecAngleDiffSaved.x = AngleDiff( vehicleEyeAngles.x, pData->vecAnglesSaved.x );
		pData->vecAngleDiffSaved.y = AngleDiff( vehicleEyeAngles.y, pData->vecAnglesSaved.y );
		pData->vecAngleDiffSaved.z = AngleDiff( vehicleEyeAngles.z, pData->vecAnglesSaved.z );

		pData->vecAngleDiffMin = pData->vecAngleDiffSaved;
	}

	pData->bWasRunningAnim = bRunningAnim;

	float frac = 0;
	float flFracFOV = 0;

	// If we're in an enter/exit animation, blend the player's eye angles to the attachment's
	if ( bRunningAnim || pData->bRunningEnterExit )
	{
		*pAbsAngles = vehicleEyeAngles;

		// Forward integrate to determine the elapsed time in this entry/exit anim.
		frac = pData->flAnimTimeElapsed / pData->flEnterExitDuration;
		frac = clamp( frac, 0.0f, 1.0f );

		flFracFOV = pData->flAnimTimeElapsed / ( pData->flEnterExitDuration * 0.85f );
		flFracFOV = clamp( flFracFOV, 0.0f, 1.0f );

		//Msg("Frac: %f\n", frac );

		if ( frac < 1.0 )
		{
			// Blend to the desired vehicle eye origin
			//Vector vecToView = (vehicleEyeOrigin - PrevMainViewOrigin());
			//vehicleEyeOrigin = PrevMainViewOrigin() + (vecToView * SimpleSpline(frac));
			//debugoverlay->AddBoxOverlay( vehicleEyeOrigin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,255, 64, 10 );
		}
		else 
		{
			pData->bRunningEnterExit = false;

			// Enter animation has finished, align view with the eye attachment point
			// so they can start mouselooking around.
			if ( !bExitAnimOn )
			{
				Vector localEyeOrigin;
				QAngle localEyeAngles;

				pData->pVehicle->GetAttachmentLocal( eyeAttachmentIndex, localEyeOrigin, localEyeAngles );
  				engine->SetViewAngles( localEyeAngles );
			}
		}

		pData->flAnimTimeElapsed += gpGlobals->frametime;
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Damp out some of the vehicle motion (neck/head would do this)
	if ( pData->bClampEyeAngles )
	{
		RemapViewAngles( pData, vehicleEyeAngles );
	}

	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );

	// If we're playing an extry or exit animation...
	if ( bRunningAnim || pData->bRunningEnterExit )
	{
		float flSplineFrac = clamp( SimpleSpline( frac ), 0, 1 );

		// Blend out the error between the player's initial eye angles and the animation's initial
		// eye angles over the duration of the animation. 
		QAngle vecAngleDiffBlend = ( ( 1 - flSplineFrac ) * pData->vecAngleDiffSaved );

		// If our current error is less than the error amount that we're blending 
		// out, use that. This lets the angles converge as quickly as possible.
		QAngle vecAngleDiffCur;
		vecAngleDiffCur.x = AngleDiff( vehicleEyeAngles.x, pData->vecAnglesSaved.x );
		vecAngleDiffCur.y = AngleDiff( vehicleEyeAngles.y, pData->vecAnglesSaved.y );
		vecAngleDiffCur.z = AngleDiff( vehicleEyeAngles.z, pData->vecAnglesSaved.z );

		// In either case, never increase the error, so track the minimum error and clamp to that.
		for (int i = 0; i < 3; i++)
		{
			if ( fabs(vecAngleDiffCur[i] ) < fabs( pData->vecAngleDiffMin[i] ) )
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffCur[i];
			}

			if ( fabs(vecAngleDiffBlend[i] ) < fabs( pData->vecAngleDiffMin[i] ) )
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffBlend[i];
			}
		}

		// Add the error to the animation's eye angles.
		*pAbsAngles -= pData->vecAngleDiffMin;

		// Use this as the basis for the next error calculation.
		pData->vecAnglesSaved = *pAbsAngles;

		//if ( gpGlobals->frametime )
		//{
		//	Msg("Angle : %.2f %.2f %.2f\n", target.x, target.y, target.z );
		//}
		//Msg("Prev: %.2f %.2f %.2f\n", pData->vecAnglesSaved.x, pData->vecAnglesSaved.y, pData->vecAnglesSaved.z );

		Vector vecAbsOrigin = *pAbsOrigin;

		// If we're exiting, our desired position is the server-sent exit position
		if ( bExitAnimOn )
		{
			//debugoverlay->AddBoxOverlay( vecEyeExitEndpoint, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,255,255, 64, 10 );

			// Blend to the exit position
			*pAbsOrigin = Lerp( flSplineFrac, vecAbsOrigin, *vecEyeExitEndpoint );
			if ( ( pData->flFOV != 0.0f ) && pFOV )
			{
				*pFOV = Lerp( flFracFOV, pData->flFOV, default_fov.GetFloat() );
			}
		}
		else
		{
			// Blend from our starting position to the desired origin
			*pAbsOrigin = Lerp( flSplineFrac, pData->vecOriginSaved, vecAbsOrigin );
			if ( ( pData->flFOV != 0.0f ) && pFOV )
			{
				*pFOV = Lerp( flFracFOV, default_fov.GetFloat(), pData->flFOV );
			}
		}
	}
	else if ( pFOV )
	{
		// Not running an entry/exit anim. Just use the vehicle's FOV.
		*pFOV = pData->flFOV;
	}
}
