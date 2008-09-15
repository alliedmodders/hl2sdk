//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Joystick handling function
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#ifndef _XBOX
#include <windows.h>
#endif
#include "cbase.h"
#include "basehandle.h"
#include "utlvector.h"
#include "cdll_client_int.h"
#include "cdll_util.h"
#include "kbutton.h"
#include "usercmd.h"
#include "keydefs.h"
#include "input.h"
#include "iviewrender.h"
#include "convar.h"
#include "hud.h"
#include "vgui/isurface.h"
#include "vgui_controls/controls.h"
#include "vgui/cursor.h"
#include "vstdlib/icommandline.h"
#include "inputsystem/iinputsystem.h"
#include "inputsystem/ButtonCode.h"

#ifdef _XBOX
#include "xbox/xbox_platform.h"
#include "xbox/xbox_win32stubs.h"
#include "xbox/xbox_core.h"
#else
#include "../common/xbox/xboxstubs.h"
#endif

#ifdef HL2_CLIENT_DLL
// FIXME: Autoaim support needs to be moved from HL2_DLL to the client dll, so this include should be c_baseplayer.h
#include "c_basehlplayer.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Control like a joystick
#define JOY_ABSOLUTE_AXIS	0x00000000		
// Control like a mouse, spinner, trackball
#define JOY_RELATIVE_AXIS	0x00000010		

// Axis mapping
static ConVar joy_name( "joy_name", "joystick", FCVAR_ARCHIVE );
static ConVar joy_advanced( "joy_advanced", "0", 0 );
static ConVar joy_advaxisx( "joy_advaxisx", "0", 0 );
static ConVar joy_advaxisy( "joy_advaxisy", "0", 0 );
static ConVar joy_advaxisz( "joy_advaxisz", "0", 0 );
static ConVar joy_advaxisr( "joy_advaxisr", "0", 0 );
static ConVar joy_advaxisu( "joy_advaxisu", "0", 0 );
static ConVar joy_advaxisv( "joy_advaxisv", "0", 0 );

// Basic "dead zone" and sensitivity
static ConVar joy_forwardthreshold( "joy_forwardthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_sidethreshold( "joy_sidethreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_pitchthreshold( "joy_pitchthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_yawthreshold( "joy_yawthreshold", "0.15", FCVAR_ARCHIVE );
static ConVar joy_forwardsensitivity( "joy_forwardsensitivity", "-1", FCVAR_ARCHIVE );
static ConVar joy_sidesensitivity( "joy_sidesensitivity", "1", FCVAR_ARCHIVE );
static ConVar joy_pitchsensitivity( "joy_pitchsensitivity", "1", FCVAR_ARCHIVE );
static ConVar joy_yawsensitivity( "joy_yawsensitivity", "-1", FCVAR_ARCHIVE );

// Advanced sensitivity and response
static ConVar joy_response_move( "joy_response_move", "1", FCVAR_ARCHIVE, "'Movement' stick response mode: 0=Linear, 1=quadratic, 2=cubic, 3=quadratic extreme" );
static ConVar joy_response_look( "joy_response_look", "0", FCVAR_ARCHIVE, "DISABLED - 'Look' stick response mode: 0=Linear, 1=quadratic, 2=cubic, 3=quadratic extreme, 4=custom" );
static ConVar joy_lowend( "joy_lowend", "1", FCVAR_ARCHIVE );
static ConVar joy_lowmap( "joy_lowmap", "1", FCVAR_ARCHIVE );
static ConVar joy_accelscale( "joy_accelscale", "0.6", FCVAR_ARCHIVE);
static ConVar joy_autoaimdampenrange( "joy_autoaimdampenrange", "0", FCVAR_ARCHIVE, "The stick range where autoaim dampening is applied. 0 = off" );
static ConVar joy_autoaimdampen( "joy_autoaimdampen", "0", FCVAR_ARCHIVE, "How much to scale user stick input when the gun is pointing at a valid target." );

// Misc
static ConVar joy_diagonalpov( "joy_diagonalpov", "0", FCVAR_ARCHIVE, "POV manipulator operates on diagonal axes, too." );
static ConVar joy_display_input("joy_display_input", "0", FCVAR_ARCHIVE);
static ConVar joy_wwhack2( "joy_wingmanwarrior_turnhack", "0", FCVAR_ARCHIVE, "Wingman warrior hack related to turn axes." );
ConVar joy_autosprint("joy_autosprint", "0", 0, "Automatically sprint when moving with an analog joystick" );

extern ConVar lookspring;
extern ConVar cl_forwardspeed;
extern ConVar lookstrafe;
extern ConVar in_joystick;
extern ConVar m_pitch;
extern ConVar l_pitchspeed;
extern ConVar cl_sidespeed;
extern ConVar cl_yawspeed;
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;
extern ConVar cl_pitchspeed;


//-----------------------------------------------
// Response curve function for the move axes
//-----------------------------------------------
static float ResponseCurve( int curve, float x )
{
	switch ( curve )
	{
	case 1:
		// quadratic
		if ( x < 0 )
			return -(x*x);
		return x*x;

	case 2:
		// cubic
		return x*x*x;

	case 3:
		{
		// quadratic extreme
		float extreme = 1.0f;
		if ( fabs( x ) >= 0.95f )
		{
			extreme = 1.5f;
		}
		if ( x < 0 )
			return -extreme * x*x;
		return extreme * x*x;
		}
	}

	// linear
	return x;
}


//-----------------------------------------------
// If we have a valid autoaim target, dampen the 
// player's stick input if it is moving away from
// the target.
//
// This assists the player staying on target.
//-----------------------------------------------
float AutoAimDampening( float x, int axis, float dist )
{
	// FIXME: Autoaim support needs to be moved from HL2_DLL to the client dll, so all games can use it.
#ifdef HL2_CLIENT_DLL
	// Help the user stay on target if the feature is enabled and the user
	// is not making a gross stick movement.
	if( joy_autoaimdampen.GetFloat() > 0.0f && fabs(x) < joy_autoaimdampenrange.GetFloat() )
	{
		// Get the HL2 player
		C_BaseHLPlayer *pLocalPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();

		if( pLocalPlayer )
		{
			// Get the autoaim target.
			CBaseEntity *pTarget = pLocalPlayer->m_HL2Local.m_hAutoAimTarget.Get();

			if( pTarget )
			{
				return joy_autoaimdampen.GetFloat();
			}
		}
	}
#endif
	return 1.0f;// No dampening.
}


//-----------------------------------------------
// This structure holds persistent information used
// to make decisions about how to modulate analog
// stick input.
//-----------------------------------------------
typedef struct 
{
	float	envelopeScale[2];
} envelope_t;

envelope_t	controlEnvelope;


//-----------------------------------------------
// Response curve function specifically for the 
// 'look' analog stick.
//
// when AXIS == YAW, otherAxisValue contains the 
// value for the pitch of the control stick, and
// vice-versa.
//-----------------------------------------------
static float ResponseCurveLook( int curve, float x, int axis, float otherAxis, float dist )
{
	float input = x;

	// Make X positive to make things easier, just remember whether we have to flip it back!
	bool negative = false;
	if( x < 0.0f )
	{
		negative = true;
		x *= -1;
	}

	// Perform the two-stage mapping.
	if( x > joy_lowend.GetFloat() )
	{
		float highmap = 1.0f - joy_lowmap.GetFloat();
		float xNormal = x - joy_lowend.GetFloat();

		float factor = xNormal / ( 1.0f - joy_lowend.GetFloat() );
		x = joy_lowmap.GetFloat() + (highmap * factor);

		// Accelerate.
		if( controlEnvelope.envelopeScale[axis] < 1.0f )
		{
			float delta = x - joy_lowmap.GetFloat();

			x = joy_lowmap.GetFloat() + (delta * controlEnvelope.envelopeScale[axis]);

			controlEnvelope.envelopeScale[axis] += ( gpGlobals->frametime * joy_accelscale.GetFloat() );

			if( controlEnvelope.envelopeScale[axis] > 1.0f )
			{
				controlEnvelope.envelopeScale[axis] = 1.0f;
			}
		}
	}
	else
	{
		// Shut off acceleration
		controlEnvelope.envelopeScale[axis] = 0.0f;

		float factor = x / joy_lowend.GetFloat();
		x = joy_lowmap.GetFloat() * factor;
	}

	x *= AutoAimDampening( input, axis, dist );

	if( axis == PITCH && input != 0.0f && joy_display_input.GetBool() )
	{
		Msg("In:%f Out:%f\n", input, x );
	}

	if( negative )
	{
		x *= -1;
	}

	return x;
}


//-----------------------------------------------------------------------------
// Purpose: Advanced joystick setup
//-----------------------------------------------------------------------------
void CInput::Joystick_Advanced(void)
{
	// called whenever an update is needed
	int	i;
	DWORD dwTemp;

	if ( IsXbox() )
	{
		// Xbox always uses a joystick
		in_joystick.SetValue( 1 );
	}

	// Initialize all the maps
	for ( i = 0; i < MAX_JOYSTICK_AXES; i++ )
	{
		m_rgAxes[i].AxisMap = GAME_AXIS_NONE;
		m_rgAxes[i].ControlMap = JOY_ABSOLUTE_AXIS;
	}

	if ( !joy_advanced.GetBool() )
	{
		// default joystick initialization
		// 2 axes only with joystick control
		m_rgAxes[JOY_AXIS_X].AxisMap = GAME_AXIS_YAW;
		m_rgAxes[JOY_AXIS_Y].AxisMap = GAME_AXIS_FORWARD;
	}
	else
	{
		if ( Q_stricmp( joy_name.GetString(), "joystick") != 0 )
		{
			// notify user of advanced controller
			Msg( "Using joystick '%s' configuration\n", joy_name.GetString() );
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD)joy_advaxisx.GetInt();
		m_rgAxes[JOY_AXIS_X].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_X].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_X", &m_rgAxes[JOY_AXIS_X] );

		dwTemp = (DWORD)joy_advaxisy.GetInt();
		m_rgAxes[JOY_AXIS_Y].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Y].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_Y", &m_rgAxes[JOY_AXIS_Y] );

		dwTemp = (DWORD)joy_advaxisz.GetInt();
		m_rgAxes[JOY_AXIS_Z].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_Z].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_Z", &m_rgAxes[JOY_AXIS_Z] );

		dwTemp = (DWORD)joy_advaxisr.GetInt();
		m_rgAxes[JOY_AXIS_R].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_R].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_R", &m_rgAxes[JOY_AXIS_R] );

		dwTemp = (DWORD)joy_advaxisu.GetInt();
		m_rgAxes[JOY_AXIS_U].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_U].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_U", &m_rgAxes[JOY_AXIS_U] );

		dwTemp = (DWORD)joy_advaxisv.GetInt();
		m_rgAxes[JOY_AXIS_V].AxisMap = dwTemp & 0x0000000f;
		m_rgAxes[JOY_AXIS_V].ControlMap = dwTemp & JOY_RELATIVE_AXIS;

		DescribeJoystickAxis( "JOY_AXIS_V", &m_rgAxes[JOY_AXIS_V] );

		Msg( "Advanced Joystick settings initialized\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CInput::DescribeAxis( int index )
{
	switch ( index )
	{
	case GAME_AXIS_FORWARD:
		return "Forward";
	case GAME_AXIS_PITCH:
		return "Look";
	case GAME_AXIS_SIDE:
		return "Side";
	case GAME_AXIS_YAW:
		return "Turn";
	case GAME_AXIS_NONE:
	default:
		return "Unknown";
	}

	return "Unknown";
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *axis - 
//			*mapping - 
//-----------------------------------------------------------------------------
void CInput::DescribeJoystickAxis( char const *axis, joy_axis_t *mapping )
{
	if ( !mapping->AxisMap )
	{
		Msg( "%s:  unmapped\n", axis );
	}
	else
	{
		Msg( "%s:  mapped to %s (%s)\n",
			axis, 
			DescribeAxis( mapping->AxisMap ),
			mapping->ControlMap != 0 ? "relative" : "absolute" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Allow joystick to issue key events
// Not currently used - controller button events are pumped through the windprocs. KWD
//-----------------------------------------------------------------------------
void CInput::ControllerCommands( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Scales the raw analog value to lie withing the axis range (full range - deadzone )
//-----------------------------------------------------------------------------
float CInput::ScaleAxisValue( const float axisValue, const float axisThreshold )
{
	// Xbox scales the range of all axes in the inputsystem. PC can't do that because each axis mapping
	// has a (potentially) unique threshold value.  If all axes were restricted to a single threshold
	// as they are on the Xbox, this function could move to inputsystem and be slightly more optimal.
	float result = 0.f;
	if ( IsPC() )
	{
		if ( axisValue < -axisThreshold )
		{
			result = ( axisValue + axisThreshold ) / ( MAX_BUTTONSAMPLE - axisThreshold );
		}
		else if ( axisValue > axisThreshold )
		{
			result = ( axisValue - axisThreshold ) / ( MAX_BUTTONSAMPLE - axisThreshold );
		}
	}
	else
	{
		// IsXbox
		result =  axisValue * ( 1.f / MAX_BUTTONSAMPLE );
	}

	return result;
}


//-----------------------------------------------------------------------------
// Purpose: Apply joystick to CUserCmd creation
// Input  : frametime - 
//			*cmd - 
//-----------------------------------------------------------------------------
void CInput::JoyStickMove( float frametime, CUserCmd *cmd )
{
	// complete initialization if first time in ( needed as cvars are not available at initialization time )
	if ( !m_fJoystickAdvancedInit )
	{
		Joystick_Advanced();
		m_fJoystickAdvancedInit = true;
	}

	// verify joystick is available and that the user wants to use it
	if ( !in_joystick.GetInt() || 0 == inputsystem->GetJoystickCount() )
	{
		return; 
	}

	QAngle viewangles;

	// Get starting angles
	engine->GetViewAngles( viewangles );

	struct axis_t
	{
		float	value;
		int		controlType;
	};
	axis_t gameAxes[ MAX_GAME_AXES ];
	memset( &gameAxes, 0, sizeof(gameAxes) );

	// Get each joystick axis value, and normalize the range
	for ( int i = 0; i < MAX_JOYSTICK_AXES; ++i )
	{
		if ( GAME_AXIS_NONE == m_rgAxes[i].AxisMap )
			continue;

		float fAxisValue = inputsystem->GetAnalogValue( (AnalogCode_t)JOYSTICK_AXIS( 0, i ) );

		if (joy_wwhack2.GetInt() != 0 )
		{
			// this is a special formula for the Logitech WingMan Warrior
			// y=ax^b; where a = 300 and b = 1.3
			// also x values are in increments of 800 (so this is factored out)
			// then bounds check result to level out excessively high spin rates
			float fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
			if (fTemp > 14000.0)
				fTemp = 14000.0;
			// restore direction information
			fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
		}

		unsigned int idx = m_rgAxes[i].AxisMap;
		gameAxes[idx].value = fAxisValue;
		gameAxes[idx].controlType = m_rgAxes[i].ControlMap;
	}

	// Re-map the axis values if necessary, based on the joystick configuration
	if ( (joy_advanced.GetInt() == 0) && (in_jlook.state & 1) )
	{
		// user wants forward control to become pitch control
		gameAxes[GAME_AXIS_PITCH] = gameAxes[GAME_AXIS_FORWARD];
		gameAxes[GAME_AXIS_FORWARD].value = 0;

		// if mouse invert is on, invert the joystick pitch value
		// Note: only absolute control support here - joy_advanced = 0
		if ( m_pitch.GetFloat() < 0.0 )
		{
			gameAxes[GAME_AXIS_PITCH].value *= -1;
		}
	}

	if ( (in_strafe.state & 1) || lookstrafe.GetFloat() && (in_jlook.state & 1) )
	{
		// user wants yaw control to become side control
		gameAxes[GAME_AXIS_SIDE] = gameAxes[GAME_AXIS_YAW];
		gameAxes[GAME_AXIS_YAW].value = 0;
	}

	float forward	= ScaleAxisValue( gameAxes[GAME_AXIS_FORWARD].value, MAX_BUTTONSAMPLE * joy_forwardthreshold.GetFloat() );
	float side		= ScaleAxisValue( gameAxes[GAME_AXIS_SIDE].value, MAX_BUTTONSAMPLE * joy_sidethreshold.GetFloat()  );
	float pitch		= ScaleAxisValue( gameAxes[GAME_AXIS_PITCH].value, MAX_BUTTONSAMPLE * joy_pitchthreshold.GetFloat()  );
	float yaw		= ScaleAxisValue( gameAxes[GAME_AXIS_YAW].value, MAX_BUTTONSAMPLE * joy_yawthreshold.GetFloat()  );

	float	joySideMove = 0.f;
	float	joyForwardMove = 0.f;
	float   aspeed = frametime * gHUD.GetFOVSensitivityAdjust();

	// apply forward and side control
	joyForwardMove	+= ResponseCurve( joy_response_move.GetInt(), forward ) * joy_forwardsensitivity.GetFloat() * cl_forwardspeed.GetFloat();
	joySideMove		+= ResponseCurve( joy_response_move.GetInt(), side ) * joy_sidesensitivity.GetFloat() * cl_sidespeed.GetFloat();

	Vector2D move( yaw, pitch );
	float dist = move.Length();

	// apply turn control
	float angle = 0.f;
	if ( JOY_ABSOLUTE_AXIS == gameAxes[GAME_AXIS_YAW].controlType )
	{
		float fAxisValue = ResponseCurveLook( joy_response_look.GetInt(), yaw, YAW, pitch, dist );
		angle = fAxisValue * joy_yawsensitivity.GetFloat() * aspeed * cl_yawspeed.GetFloat();
	}
	else
	{
		angle = yaw * joy_yawsensitivity.GetFloat() * aspeed * 180.0;
	}
	viewangles[YAW] += angle;
	cmd->mousedx = angle;

	// apply look control
	if ( IsXbox() || in_jlook.state & 1 )
	{
		float angle = 0;
		if ( JOY_ABSOLUTE_AXIS == gameAxes[GAME_AXIS_PITCH].controlType )
		{
			float fAxisValue = ResponseCurveLook( joy_response_look.GetInt(), pitch, PITCH, yaw, dist );
			angle = fAxisValue * joy_pitchsensitivity.GetFloat() * aspeed * cl_pitchspeed.GetFloat();
		}
		else
		{
			angle = pitch * joy_pitchsensitivity.GetFloat() * aspeed * 180.0;
		}
		viewangles[PITCH] += angle;
		cmd->mousedy = angle;
		view->StopPitchDrift();
		
		if( pitch == 0.f && lookspring.GetFloat() == 0.f )
		{
			// no pitch movement
			// disable pitch return-to-center unless requested by user
			// *** this code can be removed when the lookspring bug is fixed
			// *** the bug always has the lookspring feature on
			view->StopPitchDrift();
		}
	}

	cmd->forwardmove += joyForwardMove;
	cmd->sidemove += joySideMove;

	if ( IsPC() )
	{
		if ( FloatMakePositive(joyForwardMove) >= joy_autosprint.GetFloat() || FloatMakePositive(joySideMove) >= joy_autosprint.GetFloat() )
		{
			KeyDown( &in_joyspeed, true );
		}
		else
		{
			KeyUp( &in_joyspeed, true );
		}
	}

	// Bound pitch
	viewangles[PITCH] = clamp( viewangles[ PITCH ], -cl_pitchup.GetFloat(), cl_pitchdown.GetFloat() );

	engine->SetViewAngles( viewangles );
}
