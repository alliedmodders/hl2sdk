//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
// $NoKeywords: $
//=============================================================================//

#include <string.h>

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class CColorCorrection : public CBaseEntity
{
	DECLARE_CLASS( CColorCorrection, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CColorCorrection();

	void Spawn( void );
	int  UpdateTransmitState();
	void Activate( void );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	// Inputs
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

private:
	
	bool	m_bStartDisabled;
	CNetworkVar( bool, m_bEnabled );
	
	CNetworkVar( float, m_Weight );
	CNetworkVar( float, m_MinFalloff );
	CNetworkVar( float, m_MaxFalloff );
	CNetworkVar( float, m_MaxWeight );
	CNetworkString( m_netlookupFilename, MAX_PATH );

	string_t	m_lookupFilename;
};

LINK_ENTITY_TO_CLASS(color_correction, CColorCorrection);

BEGIN_DATADESC( CColorCorrection )

	DEFINE_KEYFIELD( m_Weight,			  FIELD_FLOAT,	 "weight" ),
	DEFINE_KEYFIELD( m_MinFalloff,		  FIELD_FLOAT,   "minfalloff" ),
	DEFINE_KEYFIELD( m_MaxFalloff,		  FIELD_FLOAT,   "maxfalloff" ),
	DEFINE_KEYFIELD( m_MaxWeight,		  FIELD_FLOAT,	 "maxweight" ),
	DEFINE_KEYFIELD( m_lookupFilename,	  FIELD_STRING,  "filename" ),

	DEFINE_KEYFIELD( m_bEnabled,		  FIELD_BOOLEAN, "enabled" ),
	DEFINE_KEYFIELD( m_bStartDisabled,    FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST_NOBASE(CColorCorrection, DT_ColorCorrection)
	SendPropVector( SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat(  SENDINFO(m_MinFalloff) ),
	SendPropFloat(  SENDINFO(m_MaxFalloff) ),
	SendPropFloat(  SENDINFO(m_MaxWeight) ),
	SendPropString( SENDINFO(m_netlookupFilename) ),
	SendPropBool( SENDINFO(m_bEnabled) ),
END_SEND_TABLE()


CColorCorrection::CColorCorrection() : BaseClass()
{
	m_bEnabled = true;
	m_MinFalloff = 0.0f;
	m_MaxFalloff = 1000.0f;
	m_MaxWeight = 1.0f;
	m_netlookupFilename.GetForModify()[0] = 0;
	m_lookupFilename = NULL_STRING;
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CColorCorrection::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CColorCorrection::Spawn( void )
{
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT | EFL_DIRTY_ABSTRANSFORM );
	Precache();
	SetSolid( SOLID_NONE );

	if( m_bStartDisabled )
	{
		m_bEnabled = false;
	}
	else
	{
		m_bEnabled = true;
	}

	BaseClass::Spawn();
}

void CColorCorrection::Activate( void )
{
	BaseClass::Activate();

	Q_strncpy( m_netlookupFilename.GetForModify(), STRING( m_lookupFilename ), MAX_PATH );
}


//------------------------------------------------------------------------------
// Purpose : Input handlers
//------------------------------------------------------------------------------
void CColorCorrection::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

void CColorCorrection::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}
