//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Definitions of all the entities that control logic flow within a map
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "player.h"
#include "world.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Compares a set of integer inputs to the one main input
//			Outputs true if they are all equivalant, false otherwise
//-----------------------------------------------------------------------------
class CFogController : public CLogicalEntity
{
public:
	DECLARE_CLASS( CFogController, CLogicalEntity );

	CFogController();
	~CFogController();

	// Parse data from a map file
	virtual void Activate( );

	// Input handlers
	void InputSetStartDist(inputdata_t &data);
	void InputSetEndDist(inputdata_t &data);
	void InputTurnOn(inputdata_t &data);
	void InputTurnOff(inputdata_t &data);
	void InputSetColor(inputdata_t &data);
	void InputSetColorSecondary(inputdata_t &data);
	void InputSetFarZ( inputdata_t &data );
	void InputSetAngles( inputdata_t &inputdata );

	void InputSetColorLerpTo(inputdata_t &data);
	void InputSetColorSecondaryLerpTo(inputdata_t &data);
	void InputSetStartDistLerpTo(inputdata_t &data);
	void InputSetEndDistLerpTo(inputdata_t &data);

	void InputStartFogTransition(inputdata_t &data);

	int CFogController::DrawDebugTextOverlays(void);

	void SetLerpValues( void );
	void Spawn( void );

	DECLARE_DATADESC();


public:
	fogparams_t	m_fog;
	bool m_bUseAngles;
	int   m_iChangedVariables;

	static CFogController *s_pFogController;
};


CFogController *CFogController::s_pFogController = NULL;


LINK_ENTITY_TO_CLASS( env_fog_controller, CFogController );


BEGIN_DATADESC( CFogController )

	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetStartDist",	InputSetStartDist ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetEndDist",	InputSetEndDist ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOn",		InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOff",		InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColor",		InputSetColor ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorSecondary",	InputSetColorSecondary ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"SetFarZ",		InputSetFarZ ),
	DEFINE_INPUTFUNC( FIELD_STRING,		"SetAngles",	InputSetAngles ),

	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorLerpTo",		InputSetColorLerpTo ),
	DEFINE_INPUTFUNC( FIELD_COLOR32,	"SetColorSecondaryLerpTo",	InputSetColorSecondaryLerpTo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetStartDistLerpTo",	InputSetStartDistLerpTo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetEndDistLerpTo",	InputSetEndDistLerpTo ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"StartFogTransition", InputStartFogTransition ),

	// Quiet classcheck
	//DEFINE_EMBEDDED( m_fog ),

	DEFINE_KEYFIELD( m_bUseAngles,			FIELD_BOOLEAN,	"use_angles" ),
	DEFINE_KEYFIELD( m_fog.colorPrimary,	FIELD_COLOR32,	"fogcolor" ),
	DEFINE_KEYFIELD( m_fog.colorSecondary,	FIELD_COLOR32,	"fogcolor2" ),
	DEFINE_KEYFIELD( m_fog.dirPrimary,		FIELD_VECTOR,	"fogdir" ),
	DEFINE_KEYFIELD( m_fog.enable,			FIELD_BOOLEAN,	"fogenable" ),
	DEFINE_KEYFIELD( m_fog.blend,			FIELD_BOOLEAN,	"fogblend" ),
	DEFINE_KEYFIELD( m_fog.start,			FIELD_FLOAT,	"fogstart" ),
	DEFINE_KEYFIELD( m_fog.end,				FIELD_FLOAT,	"fogend" ),
	DEFINE_KEYFIELD( m_fog.farz,			FIELD_FLOAT,	"farz" ),
	DEFINE_KEYFIELD( m_fog.duration,		FIELD_FLOAT,	"foglerptime" ),

	DEFINE_THINKFUNC( SetLerpValues ),

	DEFINE_FIELD( m_iChangedVariables, FIELD_INTEGER ),

	DEFINE_FIELD( m_fog.lerptime, FIELD_TIME ),
	DEFINE_FIELD( m_fog.colorPrimaryLerpTo, FIELD_COLOR32 ),
	DEFINE_FIELD( m_fog.colorSecondaryLerpTo, FIELD_COLOR32 ),
	DEFINE_FIELD( m_fog.startLerpTo, FIELD_FLOAT ),
	DEFINE_FIELD( m_fog.endLerpTo, FIELD_FLOAT ),

END_DATADESC()



CFogController::CFogController()
{
	// Make sure that old maps without fog fields don't get wacked out fog values.
	m_fog.enable = false;

	if ( s_pFogController )
	{
		// There should only be one fog controller in the level. Not a fatal error, but
		// the level designer should fix it.
		Warning( "Found multiple fog controllers in the same level.\n" );
	}
	else
	{
		s_pFogController = this;
	}
}


CFogController::~CFogController()
{
	if ( s_pFogController == this )
	{
		s_pFogController = NULL;
	}
	else
	{
		// There should only be one fog controller in the level. Not a fatal error, but
		// the level designer should fix it.
		Warning( "Found multiple fog controllers in the same level.\n" );
	}
}

void CFogController::Spawn( void )
{
	BaseClass::Spawn();

	m_fog.colorPrimaryLerpTo = m_fog.colorPrimary;
	m_fog.colorSecondaryLerpTo = m_fog.colorSecondary;
}

//-----------------------------------------------------------------------------
// Activate!
//-----------------------------------------------------------------------------
void CFogController::Activate( ) 
{
	BaseClass::Activate();

	if ( m_bUseAngles )
	{
		AngleVectors( GetAbsAngles(), &m_fog.dirPrimary.GetForModify() );
		m_fog.dirPrimary.GetForModify() *= -1.0f; 
	}	    
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog start distance.
//------------------------------------------------------------------------------
void CFogController::InputSetStartDist(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.start = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the fog end distance.
//------------------------------------------------------------------------------
void CFogController::InputSetEndDist(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.end = inputdata.value.Float();
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning on the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOn(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.enable = true;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for turning off the fog.
//------------------------------------------------------------------------------
void CFogController::InputTurnOff(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.enable = false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for setting the primary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColor(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.colorPrimary = inputdata.value.Color32();
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the secondary fog color.
//------------------------------------------------------------------------------
void CFogController::InputSetColorSecondary(inputdata_t &inputdata)
{
	// Get the world entity.
	m_fog.colorSecondary = inputdata.value.Color32();
}

void CFogController::InputSetFarZ(inputdata_t &inputdata)
{
	m_fog.farz = inputdata.value.Int();
}


//------------------------------------------------------------------------------
// Purpose: Sets the angles to use for the secondary fog direction.
//------------------------------------------------------------------------------
void CFogController::InputSetAngles( inputdata_t &inputdata )
{
	const char *pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector( angles.Base(), pAngles );

	Vector vTemp;
	AngleVectors( angles, &vTemp );
	SetAbsAngles( angles );

	AngleVectors( GetAbsAngles(), &m_fog.dirPrimary.GetForModify() );
	m_fog.dirPrimary.GetForModify() *= -1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFogController::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"State: %s",(m_fog.enable)?"On":"Off");
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Start: %3.0f",m_fog.start);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"End  : %3.0f",m_fog.end);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 color = m_fog.colorPrimary;
		Q_snprintf(tempstr,sizeof(tempstr),"1) Red  : %i",color.r);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Green: %i",color.g);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"1) Blue : %i",color.b);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color = m_fog.colorSecondary;
		Q_snprintf(tempstr,sizeof(tempstr),"2) Red  : %i",color.r);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Green: %i",color.g);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"2) Blue : %i",color.b);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

#define FOG_CONTROLLER_COLORPRIMARY_LERP 1
#define FOG_CONTROLLER_COLORSECONDARY_LERP 2
#define FOG_CONTROLLER_START_LERP 4
#define FOG_CONTROLLER_END_LERP 8

void CFogController::InputSetColorLerpTo(inputdata_t &data)
{
	m_iChangedVariables |= FOG_CONTROLLER_COLORPRIMARY_LERP;
	m_fog.colorPrimaryLerpTo = data.value.Color32();
}

void CFogController::InputSetColorSecondaryLerpTo(inputdata_t &data)
{
	m_iChangedVariables |= FOG_CONTROLLER_COLORSECONDARY_LERP;
	m_fog.colorSecondaryLerpTo = data.value.Color32();
}

void CFogController::InputSetStartDistLerpTo(inputdata_t &data)
{
	m_iChangedVariables |= FOG_CONTROLLER_START_LERP;
	m_fog.startLerpTo = data.value.Float();
}

void CFogController::InputSetEndDistLerpTo(inputdata_t &data)
{
	m_iChangedVariables |= FOG_CONTROLLER_END_LERP;
	m_fog.endLerpTo = data.value.Float();
}

void CFogController::InputStartFogTransition(inputdata_t &data)
{
	SetThink( &CFogController::SetLerpValues );

	m_fog.lerptime = gpGlobals->curtime + m_fog.duration + 0.1;
    SetNextThink( gpGlobals->curtime + m_fog.duration );
}

void CFogController::SetLerpValues( void )
{
	if ( m_iChangedVariables & FOG_CONTROLLER_COLORPRIMARY_LERP )
	{
		m_fog.colorPrimary = m_fog.colorPrimaryLerpTo;
	}

	if ( m_iChangedVariables & FOG_CONTROLLER_COLORSECONDARY_LERP )
	{
		m_fog.colorSecondary = m_fog.colorSecondaryLerpTo;
	} 

	if ( m_iChangedVariables & FOG_CONTROLLER_START_LERP )
	{
		m_fog.start = m_fog.startLerpTo;
	}

	if ( m_iChangedVariables & FOG_CONTROLLER_END_LERP )
	{
		m_fog.end = m_fog.endLerpTo;
	}

	m_iChangedVariables = 0;
	m_fog.lerptime = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : fogEnable - 
//			fogColorPrimary - 
//			fogColorSecondary - 
//			fogDirPrimary - 
//			fogStart - 
//			fogEnd - 
//-----------------------------------------------------------------------------
bool GetWorldFogParams( fogparams_t &fog )
{
	if ( CFogController::s_pFogController )
	{
		if ( Q_memcmp( &fog, CFogController::s_pFogController, sizeof(fog) ))
		{
			fog = CFogController::s_pFogController->m_fog;
			return true;
		}
	}
	else
	{
		if ( fog.farz != -1 || fog.enable != false )
		{
			// No fog controller in this level. Use default fog parameters.
			fog.farz = -1;
			fog.enable = false;
			return true;
		}
	}

	return false;
}

