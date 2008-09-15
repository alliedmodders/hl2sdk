//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "hud.h"
#include "kbutton.h"
#include "input.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-------------------------------------------------- Constants

#define CAM_DIST_DELTA 1.0
#define CAM_ANGLE_DELTA 2.5
#define CAM_ANGLE_SPEED 2.5
#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX  135.0
#define YAW_MIN	 -135.0

//-------------------------------------------------- Global Variables

static ConVar cam_command( "cam_command", "0", FCVAR_CHEAT );	 // tells camera to go to thirdperson
static ConVar cam_snapto( "cam_snapto", "0", FCVAR_ARCHIVE );	 // snap to thirdperson view
static ConVar cam_idealyaw( "cam_idealyaw", "90", FCVAR_ARCHIVE );	 // thirdperson yaw
static ConVar cam_idealpitch( "cam_idealpitch", "0", FCVAR_ARCHIVE );	 // thirperson pitch
static ConVar cam_idealdist( "cam_idealdist", "64", FCVAR_ARCHIVE );	 // thirdperson distance

static ConVar c_maxpitch( "c_maxpitch", "90", FCVAR_ARCHIVE );
static ConVar c_minpitch( "c_minpitch", "0", FCVAR_ARCHIVE );
static ConVar c_maxyaw( "c_maxyaw",   "135", FCVAR_ARCHIVE );
static ConVar c_minyaw( "c_minyaw",   "-135", FCVAR_ARCHIVE );
static ConVar c_maxdistance( "c_maxdistance",   "200", FCVAR_ARCHIVE );
static ConVar c_mindistance( "c_mindistance",   "30", FCVAR_ARCHIVE );
static ConVar c_orthowidth( "c_orthowidth",   "100", FCVAR_ARCHIVE );
static ConVar c_orthoheight( "c_orthoheight",   "100", FCVAR_ARCHIVE );

static kbutton_t cam_pitchup, cam_pitchdown, cam_yawleft, cam_yawright;
static kbutton_t cam_in, cam_out, cam_move;

extern const ConVar *sv_cheats;

// API Wrappers

/*
==============================
CAM_ToThirdPerson

==============================
*/
void CAM_ToThirdPerson(void)
{
	input->CAM_ToThirdPerson();
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
void CAM_ToFirstPerson(void) 
{ 
	input->CAM_ToFirstPerson();
}

/*
==============================
CAM_ToOrthographic

==============================
*/
void CAM_ToOrthographic(void) 
{ 
	input->CAM_ToOrthographic();
}

/*
==============================
CAM_StartMouseMove

==============================
*/
void CAM_StartMouseMove( void )
{
	input->CAM_StartMouseMove();
}

/*
==============================
CAM_EndMouseMove

==============================
*/
void CAM_EndMouseMove( void )
{
	input->CAM_EndMouseMove();
}

/*
==============================
CAM_StartDistance

==============================
*/
void CAM_StartDistance( void )
{
	input->CAM_StartDistance();
}

/*
==============================
CAM_EndDistance

==============================
*/
void CAM_EndDistance( void )
{
	input->CAM_EndDistance();
}

/*
==============================
CAM_ToggleSnapto

==============================
*/
void CAM_ToggleSnapto( void )
{ 
	cam_snapto.SetValue( !cam_snapto.GetInt() );
}


/*
==============================
MoveToward

==============================
*/
float MoveToward( float cur, float goal, float maxspeed )
{
	if( cur != goal )
	{
		if( abs( cur - goal ) > 180.0 )
		{
			if( cur < goal )
				cur += 360.0;
			else
				cur -= 360.0;
		}

		if( cur < goal )
		{
			if( cur < goal - 1.0 )
				cur += ( goal - cur ) / 4.0;
			else
				cur = goal;
		}
		else
		{
			if( cur > goal + 1.0 )
				cur -= ( cur - goal ) / 4.0;
			else
				cur = goal;
		}
	}


	// bring cur back into range
	if( cur < 0 )
		cur += 360.0;
	else if( cur >= 360 )
		cur -= 360;

	return cur;
}

/*
==============================
CAM_Think

==============================
*/
void CInput::CAM_Think( void )
{
	Vector origin;
	Vector ext, pnt, camForward, camRight, camUp;
	float dist;
	Vector camAngles;
	float flSensitivity;
	QAngle viewangles;
	
	switch( cam_command.GetInt() )
	{
	case CAM_COMMAND_TOTHIRDPERSON:
		CAM_ToThirdPerson();
		break;
		
	case CAM_COMMAND_TOFIRSTPERSON:
		CAM_ToFirstPerson();
		break;
		
	case CAM_COMMAND_NONE:
	default:
		break;
	}
	
	if( !m_fCameraInThirdPerson )
		return;

	if ( !sv_cheats )
	{
		sv_cheats = cvar->FindVar( "sv_cheats" );
	}

	// If cheats have been disabled, pull us back out of third-person view.
	if ( sv_cheats && !sv_cheats->GetBool() )
	{
		CAM_ToFirstPerson();
		return;
	}
	
	camAngles[ PITCH ] = cam_idealpitch.GetFloat();
	camAngles[ YAW ] = cam_idealyaw.GetFloat();
	dist = cam_idealdist.GetFloat();
	//
	//movement of the camera with the mouse
	//
	if (m_fCameraMovingWithMouse)
	{
		int cpx, cpy;
#ifndef _XBOX		
		//get windows cursor position
		GetMousePos (cpx, cpy);
#else
		//xboxfixme
		cpx = cpy = 0;
#endif
		
		m_nCameraX = cpx;
		m_nCameraY = cpy;
		
		//check for X delta values and adjust accordingly
		//eventually adjust YAW based on amount of movement
		//don't do any movement of the cam using YAW/PITCH if we are zooming in/out the camera	
		if (!m_fCameraDistanceMove)
		{
			int x, y;
			GetWindowCenter( x,  y );
			
			//keep the camera within certain limits around the player (ie avoid certain bad viewing angles)  
			if (m_nCameraX>x)
			{
				//if ((camAngles[YAW]>=225.0)||(camAngles[YAW]<135.0))
				if (camAngles[YAW]<c_maxyaw.GetFloat())
				{
					camAngles[ YAW ] += (CAM_ANGLE_MOVE)*((m_nCameraX-x)/2);
				}
				if (camAngles[YAW]>c_maxyaw.GetFloat())
				{
					
					camAngles[YAW]=c_maxyaw.GetFloat();
				}
			}
			else if (m_nCameraX<x)
			{
				//if ((camAngles[YAW]<=135.0)||(camAngles[YAW]>225.0))
				if (camAngles[YAW]>c_minyaw.GetFloat())
				{
					camAngles[ YAW ] -= (CAM_ANGLE_MOVE)* ((x-m_nCameraX)/2);
					
				}
				if (camAngles[YAW]<c_minyaw.GetFloat())
				{
					camAngles[YAW]=c_minyaw.GetFloat();
					
				}
			}
			
			//check for y delta values and adjust accordingly
			//eventually adjust PITCH based on amount of movement
			//also make sure camera is within bounds
			if (m_nCameraY > y)
			{
				if(camAngles[PITCH]<c_maxpitch.GetFloat())
				{
					camAngles[PITCH] +=(CAM_ANGLE_MOVE)* ((m_nCameraY-y)/2);
				}
				if (camAngles[PITCH]>c_maxpitch.GetFloat())
				{
					camAngles[PITCH]=c_maxpitch.GetFloat();
				}
			}
			else if (m_nCameraY<y)
			{
				if (camAngles[PITCH]>c_minpitch.GetFloat())
				{
					camAngles[PITCH] -= (CAM_ANGLE_MOVE)*((y-m_nCameraY)/2);
				}
				if (camAngles[PITCH]<c_minpitch.GetFloat())
				{
					camAngles[PITCH]=c_minpitch.GetFloat();
				}
			}
			
			//set old mouse coordinates to current mouse coordinates
			//since we are done with the mouse
			
			if ( ( flSensitivity = gHUD.GetSensitivity() ) != 0 )
			{
				m_nCameraOldX=m_nCameraX*flSensitivity;
				m_nCameraOldY=m_nCameraY*flSensitivity;
			}
			else
			{
				m_nCameraOldX=m_nCameraX;
				m_nCameraOldY=m_nCameraY;
			}
#ifndef _XBOX
			ResetMouse();
#endif
		}
	}
	
	//Nathan code here
	if( input->KeyState( &cam_pitchup ) )
		camAngles[ PITCH ] += CAM_ANGLE_DELTA;
	else if( input->KeyState( &cam_pitchdown ) )
		camAngles[ PITCH ] -= CAM_ANGLE_DELTA;
	
	if( input->KeyState( &cam_yawleft ) )
		camAngles[ YAW ] -= CAM_ANGLE_DELTA;
	else if( input->KeyState( &cam_yawright ) )
		camAngles[ YAW ] += CAM_ANGLE_DELTA;
	
	if( input->KeyState( &cam_in ) )
	{
		dist -= CAM_DIST_DELTA;
		if( dist < CAM_MIN_DIST )
		{
			// If we go back into first person, reset the angle
			camAngles[ PITCH ] = 0;
			camAngles[ YAW ] = 0;
			dist = CAM_MIN_DIST;
		}
		
	}
	else if( input->KeyState( &cam_out ) )
		dist += CAM_DIST_DELTA;
	
	if (m_fCameraDistanceMove)
	{
		int x, y;
		GetWindowCenter( x, y );

		if (m_nCameraY>y)
		{
			if(dist<c_maxdistance.GetFloat())
			{
				dist +=CAM_DIST_DELTA * ((m_nCameraY-y)/2);
			}
			if (dist>c_maxdistance.GetFloat())
			{
				dist=c_maxdistance.GetFloat();
			}
		}
		else if (m_nCameraY<y)
		{
			if (dist>c_mindistance.GetFloat())
			{
				dist -= (CAM_DIST_DELTA)*((y-m_nCameraY)/2);
			}
			if (dist<c_mindistance.GetFloat())
			{
				dist=c_mindistance.GetFloat();
			}
		}
		//set old mouse coordinates to current mouse coordinates
		//since we are done with the mouse
		m_nCameraOldX=m_nCameraX*gHUD.GetSensitivity();
		m_nCameraOldY=m_nCameraY*gHUD.GetSensitivity();
#ifndef _XBOX
		ResetMouse();
#endif
	}
	
	// update ideal
	cam_idealpitch.SetValue( camAngles[ PITCH ] );
	cam_idealyaw.SetValue( camAngles[ YAW ] );
	cam_idealdist.SetValue( dist );
	
	// Move towards ideal
	VectorCopy( m_vecCameraOffset, camAngles );
	
	engine->GetViewAngles( viewangles );
	
	if( cam_snapto.GetInt() )
	{
		camAngles[ YAW ] = cam_idealyaw.GetFloat() + viewangles[ YAW ];
		camAngles[ PITCH ] = cam_idealpitch.GetFloat() + viewangles[ PITCH ];
		camAngles[ 2 ] = cam_idealdist.GetFloat();
	}
	else
	{
		if( camAngles[ YAW ] - viewangles[ YAW ] != cam_idealyaw.GetFloat() )
			camAngles[ YAW ] = MoveToward( camAngles[ YAW ], cam_idealyaw.GetFloat() + viewangles[ YAW ], CAM_ANGLE_SPEED );
		
		if( camAngles[ PITCH ] - viewangles[ PITCH ] != cam_idealpitch.GetFloat() )
			camAngles[ PITCH ] = MoveToward( camAngles[ PITCH ], cam_idealpitch.GetFloat() + viewangles[ PITCH ], CAM_ANGLE_SPEED );
		
		if( abs( camAngles[ 2 ] - cam_idealdist.GetFloat() ) < 2.0 )
			camAngles[ 2 ] = cam_idealdist.GetFloat();
		else
			camAngles[ 2 ] += ( cam_idealdist.GetFloat() - camAngles[ 2 ] ) / 4.0;
	}
	
	m_vecCameraOffset[ 0 ] = camAngles[ 0 ];
	m_vecCameraOffset[ 1 ] = camAngles[ 1 ];
	m_vecCameraOffset[ 2 ] = dist;
}

void CAM_PitchUpDown(void) { KeyDown( &cam_pitchup ); }
void CAM_PitchUpUp(void) { KeyUp( &cam_pitchup ); }
void CAM_PitchDownDown(void) { KeyDown( &cam_pitchdown ); }
void CAM_PitchDownUp(void) { KeyUp( &cam_pitchdown ); }
void CAM_YawLeftDown(void) { KeyDown( &cam_yawleft ); }
void CAM_YawLeftUp(void) { KeyUp( &cam_yawleft ); }
void CAM_YawRightDown(void) { KeyDown( &cam_yawright ); }
void CAM_YawRightUp(void) { KeyUp( &cam_yawright ); }
void CAM_InDown(void) { KeyDown( &cam_in ); }
void CAM_InUp(void) { KeyUp( &cam_in ); }
void CAM_OutDown(void) { KeyDown( &cam_out ); }
void CAM_OutUp(void) { KeyUp( &cam_out ); }

/*
==============================
CAM_ToThirdPerson

==============================
*/
void CInput::CAM_ToThirdPerson(void)
{ 
	QAngle viewangles;

#if !defined( CSTRIKE_DLL )
#if !defined( _DEBUG )

// Do allow third person in TF for now
#if defined ( TF_CLIENT_DLL )
		// This is empty intentionally!
#else
	if ( gpGlobals->maxClients > 1 )
	{
		// no thirdperson in multiplayer.
		return;
	}
#endif

#endif
#endif

	engine->GetViewAngles( viewangles );

	if( !m_fCameraInThirdPerson )
	{
		m_fCameraInThirdPerson = true; 
		
		m_vecCameraOffset[ YAW ] = viewangles[ YAW ]; 
		m_vecCameraOffset[ PITCH ] = viewangles[ PITCH ]; 
		m_vecCameraOffset[ 2 ] = CAM_MIN_DIST; 
	}

	cam_command.SetValue( 0 );
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_ToFirstPerson(void)
{
	m_fCameraInThirdPerson = false;
	cam_command.SetValue( 0 );
}

/*
==============================
CAM_ToFirstPerson

==============================
*/
bool CInput::CAM_IsOrthographic(void) const
{
	return m_CameraIsOrthographic;
}


/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_OrthographicSize(float& w, float& h) const
{
	w = c_orthowidth.GetFloat(); h = c_orthoheight.GetFloat();
}


/*
==============================
CAM_ToFirstPerson

==============================
*/
void CInput::CAM_ToOrthographic(void)
{
	m_fCameraInThirdPerson = false;
	m_CameraIsOrthographic = true;
	cam_command.SetValue( 0 );
}

/*
==============================
CAM_StartMouseMove

==============================
*/
void CInput::CAM_StartMouseMove(void)
{
	float flSensitivity;
		
	//only move the cam with mouse if we are in third person.
	if ( m_fCameraInThirdPerson )
	{
		//set appropriate flags and initialize the old mouse position
		//variables for mouse camera movement
		if (!m_fCameraMovingWithMouse)
		{
			int cpx, cpy;

			m_fCameraMovingWithMouse=true;
			m_fCameraInterceptingMouse=true;
#ifndef _XBOX			
			GetMousePos(cpx, cpy);
#else
			// xboxfixme
			cpx = cpy = 0;
#endif
			m_nCameraX = cpx;
			m_nCameraY = cpy;

			if ( ( flSensitivity = gHUD.GetSensitivity() ) != 0 )
			{
				m_nCameraOldX=m_nCameraX*flSensitivity;
				m_nCameraOldY=m_nCameraY*flSensitivity;
			}
			else
			{
				m_nCameraOldX=m_nCameraX;
				m_nCameraOldY=m_nCameraY;
			}
		}
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		m_fCameraMovingWithMouse=false;
		m_fCameraInterceptingMouse=false;
	}
}

/*
==============================
CAM_EndMouseMove

the key has been released for camera movement
tell the engine that mouse camera movement is off
==============================
*/
void CInput::CAM_EndMouseMove(void)
{
   m_fCameraMovingWithMouse=false;
   m_fCameraInterceptingMouse=false;
}

/*
==============================
CAM_StartDistance

routines to start the process of moving the cam in or out 
using the mouse
==============================
*/
void CInput::CAM_StartDistance(void)
{
	//only move the cam with mouse if we are in third person.
	if ( m_fCameraInThirdPerson )
	{
	  //set appropriate flags and initialize the old mouse position
	  //variables for mouse camera movement
	  if (!m_fCameraDistanceMove)
	  {
		  int cpx, cpy;

		  m_fCameraDistanceMove=true;
		  m_fCameraMovingWithMouse=true;
		  m_fCameraInterceptingMouse=true;
#ifndef _XBOX
		  GetMousePos(cpx, cpy);
#else
		  // xboxfixme
		  cpx = cpy = 0;
#endif

		  m_nCameraX = cpx;
		  m_nCameraY = cpy;

		  m_nCameraOldX=m_nCameraX*gHUD.GetSensitivity();
		  m_nCameraOldY=m_nCameraY*gHUD.GetSensitivity();
	  }
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		m_fCameraDistanceMove=false;
		m_fCameraMovingWithMouse=false;
		m_fCameraInterceptingMouse=false;
	}
}

/*
==============================
CAM_EndDistance

the key has been released for camera movement
tell the engine that mouse camera movement is off
==============================
*/
void CInput::CAM_EndDistance(void)
{
   m_fCameraDistanceMove=false;
   m_fCameraMovingWithMouse=false;
   m_fCameraInterceptingMouse=false;
}

/*
==============================
CAM_IsThirdPerson

==============================
*/
int CInput::CAM_IsThirdPerson( void )
{
	return m_fCameraInThirdPerson;
}

/*
==============================
CAM_GetCameraOffset

==============================
*/
void CInput::CAM_GetCameraOffset( Vector& ofs )
{
	VectorCopy( m_vecCameraOffset, ofs );
}

/*
==============================
CAM_InterceptingMouse

==============================
*/
int CInput::CAM_InterceptingMouse( void )
{
	return m_fCameraInterceptingMouse;
}

static ConCommand startpitchup( "+campitchup", CAM_PitchUpDown );
static ConCommand endpitcup( "-campitchup", CAM_PitchUpUp );
static ConCommand startpitchdown( "+campitchdown", CAM_PitchDownDown );
static ConCommand endpitchdown( "-campitchdown", CAM_PitchDownUp );
static ConCommand startcamyawleft( "+camyawleft", CAM_YawLeftDown );
static ConCommand endcamyawleft( "-camyawleft", CAM_YawLeftUp );
static ConCommand startcamyawright( "+camyawright", CAM_YawRightDown );
static ConCommand endcamyawright( "-camyawright", CAM_YawRightUp );
static ConCommand startcamin( "+camin", CAM_InDown );
static ConCommand endcamin( "-camin", CAM_InUp );
static ConCommand startcamout( "+camout", CAM_OutDown );
static ConCommand camout( "-camout", CAM_OutUp );
static ConCommand thirdperson( "thirdperson", ::CAM_ToThirdPerson, "Switch to thirdperson camera.", FCVAR_CHEAT );
static ConCommand firstperson( "firstperson", ::CAM_ToFirstPerson, "Switch to firstperson camera." );
static ConCommand camortho( "camortho", ::CAM_ToOrthographic, "Switch to orthographic camera.", FCVAR_CHEAT );
static ConCommand startcammousemove( "+cammousemove",::CAM_StartMouseMove);
static ConCommand endcammousemove( "-cammousemove",::CAM_EndMouseMove);
static ConCommand startcamdistance( "+camdistance", ::CAM_StartDistance );
static ConCommand endcamdistance( "-camdistance", ::CAM_EndDistance );
static ConCommand snapto( "snapto", CAM_ToggleSnapto );
/*
==============================
Init_Camera

==============================
*/
void CInput::Init_Camera( void )
{
	m_CameraIsOrthographic = false;
}