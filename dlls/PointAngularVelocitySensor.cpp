//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Used to fire events based on the orientation of a given entity.
//
//			Looks at its target's anglular velocity every frame and fires outputs
//			as the angular velocity passes a given threshold value.
//
//=============================================================================//

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum
{
	AVELOCITY_SENSOR_NO_LAST_RESULT = -2
};

ConVar g_debug_angularsensor( "g_debug_angularsensor", "0", FCVAR_CHEAT );

class CPointAngularVelocitySensor : public CPointEntity
{
	DECLARE_CLASS( CPointAngularVelocitySensor, CPointEntity );

public:

	void Activate(void);
	void Spawn(void);
	void Think(void);

private:

	float SampleAngularVelocity(CBaseEntity *pEntity);
	int CompareToThreshold(CBaseEntity *pEntity, float flThreshold, bool bFireVelocityOutput);
	void FireCompareOutput(int nCompareResult, CBaseEntity *pActivator);
	void DrawDebugLines( void );

	// Input handlers
	void InputTest( inputdata_t &inputdata );

	EHANDLE m_hTargetEntity;				// Entity whose angles are being monitored.
	float m_flThreshold;					// The threshold angular velocity that we are looking for.
	int m_nLastCompareResult;				// Tha comparison result from our last measurement, expressed as -1, 0, or 1
	float m_flFireTime;
	float m_flFireInterval;
	float m_flLastAngVelocity;
	QAngle m_lastOrientation;

	Vector m_vecAxis;
	bool m_bUseHelper;

	// Outputs
	COutputFloat m_AngularVelocity;
	COutputEvent m_OnLessThan;				// Fired when the target's angular velocity becomes less than the threshold velocity.
	COutputEvent m_OnLessThanOrEqualTo;		
	COutputEvent m_OnGreaterThan;			
	COutputEvent m_OnGreaterThanOrEqualTo;
	COutputEvent m_OnEqualTo;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(point_angularvelocitysensor, CPointAngularVelocitySensor);


BEGIN_DATADESC( CPointAngularVelocitySensor )

	// Fields
	DEFINE_FIELD( m_hTargetEntity, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(m_flThreshold, FIELD_FLOAT, "threshold"),
	DEFINE_FIELD(m_nLastCompareResult, FIELD_INTEGER),
	DEFINE_FIELD( m_flFireTime, FIELD_TIME ),
	DEFINE_FIELD( m_flFireInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLastAngVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( m_lastOrientation, FIELD_VECTOR ),
	
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Test", InputTest),

	// Outputs
	DEFINE_OUTPUT(m_OnLessThan, "OnLessThan"),
	DEFINE_OUTPUT(m_OnLessThanOrEqualTo, "OnLessThanOrEqualTo"),
	DEFINE_OUTPUT(m_OnGreaterThan, "OnGreaterThan"),
	DEFINE_OUTPUT(m_OnGreaterThanOrEqualTo, "OnGreaterThanOrEqualTo"),
	DEFINE_OUTPUT(m_OnEqualTo, "OnEqualTo"),
	DEFINE_OUTPUT(m_AngularVelocity, "AngularVelocity"),

	DEFINE_KEYFIELD( m_vecAxis, FIELD_VECTOR, "axis" ),
	DEFINE_KEYFIELD( m_bUseHelper, FIELD_BOOLEAN, "usehelper" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Called when spawning after parsing keyvalues.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Spawn(void)
{
	m_flThreshold = fabs(m_flThreshold);
	m_nLastCompareResult = AVELOCITY_SENSOR_NO_LAST_RESULT;
	m_flFireInterval = 0.2;
	m_lastOrientation = vec3_angle;
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities in the map have spawned.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Activate(void)
{
	BaseClass::Activate();

	m_hTargetEntity = gEntList.FindEntityByName( NULL, m_target );

	if (m_hTargetEntity)
	{
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws magic lines...
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::DrawDebugLines( void )
{
	if ( m_hTargetEntity )
	{
		Vector vForward, vRight, vUp;
		AngleVectors( m_hTargetEntity->GetAbsAngles(), &vForward, &vRight, &vUp );

		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vForward * 64, 255, 0, 0, false, 0 );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vRight * 64, 0, 255, 0, false, 0 );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vUp * 64, 0, 0, 255, false, 0 );
	}

	if ( m_bUseHelper == true )
	{
		QAngle Angles;
		Vector vAxisForward, vAxisRight, vAxisUp;

		Vector vLine = m_vecAxis - GetAbsOrigin();

		VectorNormalize( vLine );

		VectorAngles( vLine, Angles );
		AngleVectors( Angles, &vAxisForward, &vAxisRight, &vAxisUp );

		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vAxisForward * 64, 255, 0, 0, false, 0 );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vAxisRight * 64, 0, 255, 0, false, 0 );
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + vAxisUp * 64, 0, 0, 255, false, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the magnitude of the entity's angular velocity.
//-----------------------------------------------------------------------------
float CPointAngularVelocitySensor::SampleAngularVelocity(CBaseEntity *pEntity)
{
	if (pEntity->GetMoveType() == MOVETYPE_VPHYSICS)
	{
		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if (pPhys != NULL)
		{
			Vector vecVelocity;
			AngularImpulse vecAngVelocity;
			pPhys->GetVelocity(&vecVelocity, &vecAngVelocity);

			QAngle angles;
			pPhys->GetPosition( NULL, &angles );

			float dt = gpGlobals->curtime - GetLastThink();
			if ( dt == 0 )
				dt = 0.1;

			// HACKHACK: We don't expect a real 'delta' orientation here, just enough of an error estimate to tell if this thing
			// is trying to move, but failing.
			QAngle delta = angles - m_lastOrientation;

			if ( ( delta.Length() / dt )  < ( vecAngVelocity.Length() * 0.01 ) )
			{
				return 0.0f;
			}
			m_lastOrientation = angles;

			if ( m_bUseHelper == false )
			{
				return vecAngVelocity.Length();
			}
			else
			{
				Vector vLine = m_vecAxis - GetAbsOrigin();
				VectorNormalize( vLine );

				Vector vecWorldAngVelocity;
				pPhys->LocalToWorldVector( &vecWorldAngVelocity, vecAngVelocity );
				float flDot = DotProduct( vecWorldAngVelocity, vLine );

				return flDot;
			}
		}
	}
	else
	{
		QAngle vecAngVel = pEntity->GetLocalAngularVelocity();
		float flMax = max(fabs(vecAngVel[PITCH]), fabs(vecAngVel[YAW]));

		return max(flMax, fabs(vecAngVel[ROLL]));
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Compares the given entity's angular velocity to the threshold velocity.
// Input  : pEntity - Entity whose angular velocity is being measured.
//			flThreshold - 
// Output : Returns -1 if less than, 0 if equal to, or 1 if greater than the threshold.
//-----------------------------------------------------------------------------
int CPointAngularVelocitySensor::CompareToThreshold(CBaseEntity *pEntity, float flThreshold, bool bFireVelocityOutput)
{
	if (pEntity == NULL)
	{
		return 0;
	}

	float flAngVelocity = SampleAngularVelocity(pEntity);

	if ( g_debug_angularsensor.GetBool() )
	{
		DrawDebugLines();
	}

	if (bFireVelocityOutput && (flAngVelocity != m_flLastAngVelocity))
	{
		m_AngularVelocity.Set(flAngVelocity, pEntity, this);
		m_flLastAngVelocity = flAngVelocity;
	}

	if (flAngVelocity > flThreshold)
	{
		return 1;
	}

	if (flAngVelocity == flThreshold)
	{
		return 0;
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::Think(void)
{
	if (m_hTargetEntity != NULL)
	{
		//
		// Check to see if the measure entity's forward vector has been within
		// given tolerance of the target entity for the given period of time.
		//
		int nCompare = CompareToThreshold(m_hTargetEntity, m_flThreshold, true);
		if ((nCompare != m_nLastCompareResult) && (m_nLastCompareResult != AVELOCITY_SENSOR_NO_LAST_RESULT))
		{
			if (!m_flFireTime)
			{
				m_flFireTime = gpGlobals->curtime;
			}

			if (gpGlobals->curtime >= (m_flFireTime + m_flFireInterval))
			{
				//
				// The compare result has changed. We need to fire the output.
				//
				FireCompareOutput(nCompare, this);

				// Save the result for next time.
				m_flFireTime = 0;
				m_nLastCompareResult = nCompare;
			}
		}
		else if (m_nLastCompareResult == AVELOCITY_SENSOR_NO_LAST_RESULT) 
		{
			m_nLastCompareResult = nCompare;
		}

		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for forcing an instantaneous test of the condition.
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::InputTest( inputdata_t &inputdata )
{
	int nCompareResult = CompareToThreshold(m_hTargetEntity, m_flThreshold, false);
	FireCompareOutput(nCompareResult, inputdata.pActivator);
}

	
//-----------------------------------------------------------------------------
// Purpose: Fires the appropriate output based on the given comparison result.
// Input  : nCompareResult - 
//			pActivator - 
//-----------------------------------------------------------------------------
void CPointAngularVelocitySensor::FireCompareOutput( int nCompareResult, CBaseEntity *pActivator )
{
	if (nCompareResult == -1)
	{
		m_OnLessThan.FireOutput(pActivator, this);
		m_OnLessThanOrEqualTo.FireOutput(pActivator, this);
	}
	else if (nCompareResult == 1)
	{
		m_OnGreaterThan.FireOutput(pActivator, this);
		m_OnGreaterThanOrEqualTo.FireOutput(pActivator, this);
	}
	else
	{
		m_OnEqualTo.FireOutput(pActivator, this);
		m_OnLessThanOrEqualTo.FireOutput(pActivator, this);
		m_OnGreaterThanOrEqualTo.FireOutput(pActivator, this);
	}
}
