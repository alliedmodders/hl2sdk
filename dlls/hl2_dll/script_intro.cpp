//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "eventqueue.h"
#include "script_intro.h"
#include "point_camera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global point to the active intro script
CHandle<CScriptIntro> g_hIntroScript;

LINK_ENTITY_TO_CLASS(script_intro, CScriptIntro);

BEGIN_DATADESC(CScriptIntro)
	// Keys
	DEFINE_FIELD( m_vecCameraView, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecCameraViewAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecPlayerView, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecPlayerViewAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_iBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNextBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextBlendTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flBlendStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iNextFOV, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextFOVBlendTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flFOVBlendStartTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_iFOV, FIELD_INTEGER ),
	DEFINE_ARRAY( m_flFadeColor, FIELD_FLOAT, 3 ),
	DEFINE_FIELD( m_flFadeAlpha, FIELD_FLOAT ),
	DEFINE_FIELD( m_flFadeDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_hCameraEntity, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_bAlternateFOV, FIELD_BOOLEAN, "alternatefovchange" ),


	// Inputs
	DEFINE_INPUTFUNC(FIELD_STRING, "SetCameraViewEntity", InputSetCameraViewEntity ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetBlendMode", InputSetBlendMode ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetNextFOV", InputSetNextFOV ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetFOVBlendTime", InputSetFOVBlendTime ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetFOV", InputSetFOV ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetNextBlendMode", InputSetNextBlendMode ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "AdvanceToNextBlendMode", InputAdvanceToNextBlendMode ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetNextBlendTime", InputSetNextBlendTime ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC(FIELD_STRING, "FadeTo", InputFadeTo ),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetFadeColor", InputSetFadeColor ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CScriptIntro, DT_ScriptIntro )
	SendPropVector(SENDINFO(m_vecCameraView), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vecCameraViewAngles), -1, SPROP_COORD),
	SendPropInt( SENDINFO( m_iBlendMode ), 5 ),
	SendPropInt( SENDINFO( m_iNextBlendMode ), 5 ),
	SendPropFloat( SENDINFO( m_flNextBlendTime ), 10 ),
	SendPropFloat( SENDINFO( m_flBlendStartTime ), 10 ),
	SendPropBool( SENDINFO( m_bActive ) ),

	// Fov & fov blends
	SendPropInt( SENDINFO( m_iFOV ), 9 ),
	SendPropInt( SENDINFO( m_iNextFOV ), 9 ),
	SendPropFloat( SENDINFO( m_flNextFOVBlendTime ), 10 ),
	SendPropFloat( SENDINFO( m_flFOVBlendStartTime ), 10 ),

	SendPropBool( SENDINFO( m_bAlternateFOV ) ),

	// Fades
	SendPropFloat( SENDINFO( m_flFadeAlpha ), 10 ),
	SendPropArray(
		SendPropFloat( SENDINFO_ARRAY(m_flFadeColor), 32, SPROP_NOSCALE),
		m_flFadeColor),
	SendPropFloat( SENDINFO( m_flFadeDuration ), 10, SPROP_ROUNDDOWN, 0.0f, 255.0 ),
	SendPropEHandle(SENDINFO( m_hCameraEntity ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::Spawn( void )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetSize( -Vector(5,5,5), Vector(5,5,5) );
	m_bActive = false;
	m_iNextFOV = 0;
	m_iFOV = 0;
}

void CScriptIntro::Precache()
{
	PrecacheMaterial( "scripted/intro_screenspaceeffect" );
	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model.
//------------------------------------------------------------------------------
int CScriptIntro::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetCameraViewEntity( inputdata_t &inputdata )
{
	// Find the specified entity
	string_t iszEntityName = inputdata.value.StringID();
	if ( iszEntityName == NULL_STRING )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, iszEntityName, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		Warning("script_intro %s couldn't find SetCameraViewEntity named %s\n", STRING(GetEntityName()), STRING(iszEntityName) );
		return;
	}

	m_hCameraEntity = pEntity;
	m_vecCameraView = pEntity->GetAbsOrigin();
	m_vecCameraViewAngles = pEntity->GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Fill out the origin that should be included in the player's PVS
//-----------------------------------------------------------------------------
bool CScriptIntro::GetIncludedPVSOrigin( Vector *pOrigin, CBaseEntity **ppCamera )
{
	if ( m_bActive && m_hCameraEntity.Get() )
	{
		*ppCamera = m_hCameraEntity.Get();
		*pOrigin = m_hCameraEntity.Get()->GetAbsOrigin();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetBlendMode( inputdata_t &inputdata )
{
	m_iBlendMode = inputdata.value.Int();

	//Msg("%.2f INPUT: Blend mode set to %d\n", gpGlobals->curtime, m_iBlendMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextBlendMode( inputdata_t &inputdata )
{
	m_iNextBlendMode = inputdata.value.Int();

	//Msg("%.2f INPUT: Next Blend mode set to %d\n", gpGlobals->curtime, m_iNextBlendMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextFOV( inputdata_t &inputdata )
{
	m_iNextFOV = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFOVBlendTime( inputdata_t &inputdata )
{
	m_flNextFOVBlendTime = gpGlobals->curtime + inputdata.value.Float();
	m_flFOVBlendStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFOV( inputdata_t &inputdata )
{
	m_iFOV = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextBlendTime( inputdata_t &inputdata )
{
	m_flNextBlendTime = gpGlobals->curtime + inputdata.value.Float();
	m_flBlendStartTime = gpGlobals->curtime;

	//Msg("%.2f BLEND STARTED: %d to %d, end at %.2f\n", gpGlobals->curtime, m_iBlendMode, m_iNextBlendMode, m_flNextBlendTime.Get() );

	// Set our blend once the time is up
	variant_t value;
	value.SetInt( m_iNextBlendMode );
	g_EventQueue.AddEvent( this, "AdvanceToNextBlendMode", value, (m_flNextBlendTime - gpGlobals->curtime), NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputAdvanceToNextBlendMode( inputdata_t &inputdata )
{
	m_iBlendMode = inputdata.value.Int();

	//Msg("%.2f ADVANCED TO NEXT MODE: Blend mode set to %d\n", gpGlobals->curtime, m_iBlendMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputActivate( inputdata_t &inputdata )
{
	m_bActive = true;
	g_hIntroScript = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputDeactivate( inputdata_t &inputdata )
{
	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputFadeTo( inputdata_t &inputdata )
{
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get the fade alpha
	char *pszParam = strtok(parseString," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received FadeTo input without an alpha. Syntax: <fade alpha> <fade duration>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flAlpha = atof( pszParam );

	// Get the fade duration
	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received FadeTo input without a duration. Syntax: <fade alpha> <fade duration>\n", GetClassname(), GetDebugName() );
		return;
	}

	// Set the two variables
	m_flFadeAlpha = flAlpha;
	m_flFadeDuration = atof( pszParam );

	//Msg("%.2f INPUT FADE: Fade to %.2f. End at %.2f\n", gpGlobals->curtime, m_flFadeAlpha.Get(), gpGlobals->curtime + m_flFadeDuration.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFadeColor( inputdata_t &inputdata )
{
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get the fade colors
	char *pszParam = strtok(parseString," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flR = atof( pszParam );

	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flG = atof( pszParam );

	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flB = atof( pszParam );

	// Use the colors
	m_flFadeColor.Set( 0, flR );
	m_flFadeColor.Set( 1, flG );
	m_flFadeColor.Set( 2, flB );
}
