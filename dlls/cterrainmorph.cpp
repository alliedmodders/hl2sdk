//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#if !defined(IN_XBOX_CODELINE)
#include "baseentity.h"
#include "terrainmodmgr.h"
#include "terrainmodmgr_shared.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_TERRAINMORPH_INSTANT 0x00000001

#define TERRAINMORPH_THINK_FREQUENCY 0.05


//=========================================================
//=========================================================
class CTerrainMorphParams
{
public:
	Vector	m_vecNormal;

	float	m_flDeltaZ;
	float	m_flDieTime;
	float	m_flRadius;
	float	m_flDeltaRadius;
	float	m_flStrength;
	float	m_flDeltaStrength;

	float	m_flDuration;
	float	m_flStartTime;
	float	m_flStartRadius;
	float	m_flGoalRadius;

	float	m_flFraction;
};


//=========================================================
//=========================================================
class CTerrainMorph : public CPointEntity
{
	DECLARE_CLASS( CTerrainMorph, CPointEntity );
public:
	void Spawn( CTerrainMorphParams &params );
	void Start( void );
	void MorphThink( void );

	// Input handlers
	void InputBeginMorph( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	CTerrainMorphParams m_Params;

	int				m_iIterations;
};


//---------------------------------------------------------
//---------------------------------------------------------
void CTerrainMorph::Spawn( CTerrainMorphParams &params )
{
	m_Params = params;

	SetThink( NULL );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CTerrainMorph::Start( void )
{
// Set up start time, die time, effect duration.
	m_Params.m_flStartTime = gpGlobals->curtime;
	m_Params.m_flDieTime = gpGlobals->curtime + m_Params.m_flDuration;
	m_Params.m_flDuration = m_Params.m_flDuration;

// NORMAL (direction the effect will 'pull')
	GetVectors( &m_Params.m_vecNormal, NULL, NULL );

// RADIUS
	m_Params.m_flRadius = m_Params.m_flStartRadius;

// STRENGTH (This is the distance that the effect will pull the displacement)
	trace_t tr;

	// trace backwards along the normal and find the point under myself that is going to be pulled.
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + m_Params.m_vecNormal * -4096, MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );

	//NDebugOverlay::Line( tr.startpos, tr.endpos, 0,255,0, true, 30 );
	
	// Get that distance.
	float flDist;
	flDist = VectorLength( tr.endpos - tr.startpos );

	// Set the strength relative to the FRACTION specified by the designer.
	m_Params.m_flStrength = flDist * m_Params.m_flFraction;

	SetThink( &CTerrainMorph::MorphThink );
	SetNextThink( gpGlobals->curtime );

	m_iIterations = 0;

	//NDebugOverlay::Line( m_Params.m_vecLocation, m_Params.m_vecLocation + m_Params.m_vecNormal * 200, 255,0,0, true, 3 );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CTerrainMorph::MorphThink( void )
{
	SetNextThink( gpGlobals->curtime + TERRAINMORPH_THINK_FREQUENCY );


	if( m_spawnflags & SF_TERRAINMORPH_INSTANT )
	{
		// Do the full effect in one whack.
		CTerrainModParams	params;
		TerrainModType		type;
		params.m_Flags |= CTerrainModParams::TMOD_SUCKTONORMAL;

		type = TMod_Suck;

		params.m_flRadius = m_Params.m_flRadius;
		params.m_flStrength = m_Params.m_flStrength;
		params.m_vCenter = GetAbsOrigin();
		params.m_vNormal = m_Params.m_vecNormal;

		params.m_flRadius = clamp(params.m_flRadius, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS);

		TerrainMod_Add( type, params );

		UTIL_Remove( this );

		return;
	}

//----
	float flTime;
	
	flTime = ( (gpGlobals->curtime - m_Params.m_flStartTime) / m_Params.m_flDuration );
	flTime = 1 - clamp(flTime, 0, 1 );
//----

	if( flTime >= 0.0 )
	{
		//Msg( "time: %f  radius:%f  strength: %f\n", flTime,params.m_flRadius, params.m_flStrength );
		CTerrainModParams	params;
		TerrainModType		type;
		params.m_Flags |= CTerrainModParams::TMOD_SUCKTONORMAL;

		type = TMod_Suck;

#if 1
		float nextIteration = (1-flTime)*m_Params.m_flStrength;
		
		params.m_flStrength = (int)nextIteration - m_iIterations;
		if ( params.m_flStrength > 0 )
		{
			params.m_vCenter = GetAbsOrigin();
			params.m_vNormal = m_Params.m_vecNormal;

			params.m_flRadius = m_Params.m_flStartRadius * flTime + m_Params.m_flGoalRadius * ( 1 - flTime );			
			params.m_flRadius = clamp(params.m_flRadius, MIN_TMOD_RADIUS, MAX_TMOD_RADIUS);

			//Msg( "Strength: %f - Radius: %f\n", params.m_flStrength, params.m_flRadius );

			TerrainMod_Add( type, params );
			m_iIterations += params.m_flStrength;
		}
#if 0
		NDebugOverlay::Line( m_Params.m_vecLocation, m_Params.m_vecLocation + Vector( 200, 200, 0 ), 0,255,0, true, 3 );
		NDebugOverlay::Line( m_Params.m_vecLocation + Vector( m_Params.m_flRadius, 0, 0 ), m_Params.m_vecLocation + Vector( m_Params.m_flRadius, 0 , 200 ), 0,255,0, true, 3 );
#endif

#endif
	}

	if( gpGlobals->curtime > m_Params.m_flDieTime )
	{
		SetThink( NULL );
		UTIL_Remove( this );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CTerrainMorph::InputBeginMorph( inputdata_t &inputdata )
{
	Start();
}

//---------------------------------------------------------
//---------------------------------------------------------
BEGIN_DATADESC( CTerrainMorph )

	// Function Pointers
	DEFINE_FUNCTION( MorphThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "BeginMorph", InputBeginMorph ),

	// quiet down classcheck
	// DEFINE_FIELD( m_Params, CTerrainMorphParams ),
	DEFINE_KEYFIELD( m_Params.m_flStartRadius, FIELD_FLOAT, "startradius" ),
	DEFINE_KEYFIELD( m_Params.m_flGoalRadius, FIELD_FLOAT, "goalradius" ),
	DEFINE_KEYFIELD( m_Params.m_flDuration, FIELD_FLOAT, "duration" ),
	DEFINE_KEYFIELD( m_Params.m_flFraction, FIELD_FLOAT, "fraction" ),

	DEFINE_FIELD( m_iIterations, FIELD_INTEGER ),

END_DATADESC()
LINK_ENTITY_TO_CLASS(tectonic, CTerrainMorph);
LINK_ENTITY_TO_CLASS(env_terrainmorph, CTerrainMorph);

#endif
