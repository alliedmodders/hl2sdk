//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: builds an intended movement command to send to the server
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "kbutton.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "input.h"
#include "iviewrender.h"
#include "iclientmode.h"
#include "prediction.h"
#include "bitbuf.h"
#include "checksum_md5.h"
#include "hltvcamera.h"
#include <ctype.h> // isalnum()
#include <voice_status.h>

extern ConVar in_joystick;

// For showing/hiding the scoreboard
#include <cl_dll/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME, tie to entity state parsing for player!!!
int g_iAlive = 1;

static int s_ClearInputState = 0;

// Defined in pm_math.c
float anglemod( float a );

// FIXME void V_Init( void );
static int in_impulse = 0;
static int in_cancel = 0;

ConVar cl_anglespeedkey( "cl_anglespeedkey", "0.67", 0 );
ConVar cl_yawspeed( "cl_yawspeed", "210", 0 );
ConVar cl_pitchspeed( "cl_pitchspeed", "225", 0 );
ConVar cl_pitchdown( "cl_pitchdown", "89", FCVAR_CHEAT );
ConVar cl_pitchup( "cl_pitchup", "89", FCVAR_CHEAT );
ConVar cl_sidespeed( "cl_sidespeed", "400", 0 );
ConVar cl_upspeed( "cl_upspeed", "320", FCVAR_ARCHIVE );
ConVar cl_forwardspeed( "cl_forwardspeed", "400", FCVAR_ARCHIVE );
ConVar cl_backspeed( "cl_backspeed", "400", FCVAR_ARCHIVE );
ConVar lookspring( "lookspring", "0", FCVAR_ARCHIVE );
ConVar lookstrafe( "lookstrafe", "0", FCVAR_ARCHIVE );
ConVar in_joystick( "joystick","0", FCVAR_ARCHIVE );

ConVar sv_noclipduringpause( "sv_noclipduringpause", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "If cheats are enabled, then you can noclip with the game paused (for doing screenshots, etc.)." );
static ConVar cl_lagcomp_errorcheck( "cl_lagcomp_errorcheck", "0", 0, "Player index of other player to check for position errors." );

extern ConVar cl_mouselook;
#define UsingMouselook() cl_mouselook.GetBool()

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/

kbutton_t	in_speed;
kbutton_t	in_walk;
kbutton_t	in_jlook;
kbutton_t	in_strafe;
kbutton_t	in_commandermousemove;
kbutton_t	in_forward;
kbutton_t	in_back;
kbutton_t	in_moveleft;
kbutton_t	in_moveright;
// Display the netgraph
kbutton_t	in_graph;  
kbutton_t	in_joyspeed;		// auto-speed key from the joystick (only works for player movement, not vehicles)

static	kbutton_t	in_klook;
static	kbutton_t	in_left;
static	kbutton_t	in_right;
static	kbutton_t	in_lookup;
static	kbutton_t	in_lookdown;
static	kbutton_t	in_use;
static	kbutton_t	in_jump;
static	kbutton_t	in_attack;
static	kbutton_t	in_attack2;
static	kbutton_t	in_up;
static	kbutton_t	in_down;
static	kbutton_t	in_duck;
static	kbutton_t	in_reload;
static	kbutton_t	in_alt1;
static	kbutton_t	in_alt2;
static	kbutton_t	in_score;
static	kbutton_t	in_break;
static	kbutton_t	in_zoom;
static  kbutton_t   in_grenade1;
static  kbutton_t   in_grenade2;

/*
===========
IN_CenterView_f
===========
*/
void IN_CenterView_f (void)
{
	QAngle viewangles;

	if ( UsingMouselook() == false )
	{
		if ( !::input->CAM_InterceptingMouse() )
		{
			engine->GetViewAngles( viewangles );
			viewangles[PITCH] = 0;
			engine->SetViewAngles( viewangles );
		}
	}
}

/*
===========
IN_Joystick_Advanced_f
===========
*/
void IN_Joystick_Advanced_f (void)
{
	::input->Joystick_Advanced();
}

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
int KB_ConvertString( char *in, char **ppout )
{
	char sz[ 4096 ];
	char binding[ 64 ];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if ( !ppout )
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;
	while ( *p )
	{
		if ( *p == '+' )
		{
			pEnd = binding;
			while ( *p && ( isalnum( *p ) || ( pEnd == binding ) ) && ( ( pEnd - binding ) < 63 ) )
			{
				*pEnd++ = *p++;
			}

			*pEnd =  '\0';

			pBinding = NULL;
			if ( strlen( binding + 1 ) > 0 )
			{
				// See if there is a binding for binding?
				pBinding = engine->Key_LookupBinding( binding + 1 );
			}

			if ( pBinding )
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ( *pEnd )
			{
				*pOut++ = *pEnd++;
			}

			if ( pBinding )
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	int maxlen = strlen( sz ) + 1;
	pOut = ( char * )malloc( maxlen );
	Q_strncpy( pOut, sz, maxlen );
	*ppout = pOut;

	return 1;
}

/*
==============================
FindKey

Allows the engine to request a kbutton handler by name, if the key exists.
==============================
*/
kbutton_t *CInput::FindKey( const char *name )
{
	CKeyboardKey *p;
	p = m_pKeys;
	while ( p )
	{
		if ( !Q_stricmp( name, p->name ) )
		{
			return p->pkey;
		}

		p = p->next;
	}
	return NULL;
}

/*
============
AddKeyButton

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void CInput::AddKeyButton( const char *name, kbutton_t *pkb )
{
	CKeyboardKey *p;	
	kbutton_t *kb;

	kb = FindKey( name );
	
	if ( kb )
		return;

	p = new CKeyboardKey;

	Q_strncpy( p->name, name, sizeof( p->name ) );
	p->pkey = pkb;

	p->next = m_pKeys;
	m_pKeys = p;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::CInput( void )
{
	m_pCommands = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CInput::~CInput( void )
{
}

/*
============
Init_Keyboard

Add kbutton_t definitions that the engine can query if needed
============
*/
void CInput::Init_Keyboard( void )
{
	m_pKeys = NULL;

	AddKeyButton( "in_graph", &in_graph );
	AddKeyButton( "in_jlook", &in_jlook );
}

/*
============
Shutdown_Keyboard

Clear kblist
============
*/
void CInput::Shutdown_Keyboard( void )
{
	CKeyboardKey *p, *n;
	p = m_pKeys;
	while ( p )
	{
		n = p->next;
		delete p;
		p = n;
	}
	m_pKeys = NULL;
}

/*
============
KeyDown
============
*/
void KeyDown( kbutton_t *b, bool bIgnoreKey )
{
	int		k = -1;
	const char	*c = NULL;

	if ( !bIgnoreKey )
	{
		c = engine->Cmd_Argv(1);
		if (c[0])
			k = atoi(c);
	}

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		if ( c[0] )
		{
			DevMsg( 1,"Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c);
		}
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp( kbutton_t *b, bool bIgnoreKey )
{
	int		k;
	const char	*c;
	
	if ( bIgnoreKey )
	{
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	c = engine->Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
	{
		//Msg ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_CommanderMouseMoveDown() {KeyDown(&in_commandermousemove);}
void IN_CommanderMouseMoveUp() {KeyUp(&in_commandermousemove);}
void IN_BreakDown( void ) { KeyDown( &in_break );};
void IN_BreakUp( void )
{ 
	KeyUp( &in_break ); 
#if defined( _DEBUG )
	_asm
	{
		int 3;
	}
#endif
};
void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_JLookDown (void) {KeyDown(&in_jlook);}
void IN_JLookUp (void) {KeyUp(&in_jlook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}
void IN_WalkDown(void) {KeyDown(&in_walk);}
void IN_WalkUp(void) {KeyUp(&in_walk);}
void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}
void IN_Attack2Down(void) { KeyDown(&in_attack2);}
void IN_Attack2Up(void) {KeyUp(&in_attack2);}
void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}
void IN_JumpDown (void) {KeyDown(&in_jump);}
void IN_JumpUp (void) {KeyUp(&in_jump);}
void IN_DuckDown(void) {KeyDown(&in_duck);}
void IN_DuckUp(void) {KeyUp(&in_duck);}
void IN_ReloadDown(void) {KeyDown(&in_reload);}
void IN_ReloadUp(void) {KeyUp(&in_reload);}
void IN_Alt1Down(void) {KeyDown(&in_alt1);}
void IN_Alt1Up(void) {KeyUp(&in_alt1);}
void IN_Alt2Down(void) {KeyDown(&in_alt2);}
void IN_Alt2Up(void) {KeyUp(&in_alt2);}
void IN_GraphDown(void) {KeyDown(&in_graph);}
void IN_GraphUp(void) {KeyUp(&in_graph);}
void IN_ZoomDown(void) {KeyDown(&in_zoom);}
void IN_ZoomUp(void) {KeyUp(&in_zoom);}
void IN_Grenade1Up(void) { KeyUp( &in_grenade1 ); }
void IN_Grenade1Down(void) { KeyDown( &in_grenade1 ); }
void IN_Grenade2Up(void) { KeyUp( &in_grenade2 ); }
void IN_Grenade2Down(void) { KeyDown( &in_grenade2 ); }


void IN_AttackDown(void)
{
	KeyDown( &in_attack );
}

void IN_AttackUp(void)
{
	KeyUp( &in_attack );
	in_cancel = 0;
}

// Special handling
void IN_Cancel(void)
{
	in_cancel = 1;
}

void IN_Impulse (void)
{
	in_impulse = atoi( engine->Cmd_Argv(1) );
}

void IN_ScoreDown(void)
{
	KeyDown(&in_score);
	if ( gViewPortInterface )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
	}
}

void IN_ScoreUp(void)
{
	KeyUp(&in_score);
	if ( gViewPortInterface )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
		GetClientVoiceMgr()->StopSquelchMode();
	}
}


/*
============
KeyEvent

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int CInput::KeyEvent( int down, int keynum, const char *pszCurrentBinding )
{
	if ( g_pClientMode )
		return g_pClientMode->KeyInput(down, keynum, pszCurrentBinding);

	return 1;
}



/*
===============
KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CInput::KeyState ( kbutton_t *key )
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup	= key->state & 4;
	down		= key->state & 1;
	
	if ( impulsedown && !impulseup )
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if ( impulseup && !impulsedown )
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if ( !impulsedown && !impulseup )
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if ( impulsedown && impulseup )
	{
		if ( down )
		{
			// released and re-pressed this frame
			val = 0.75;	
		}
		else
		{
			// pressed and released this frame
			val = 0.25;	
		}
	}

	// clear impulses
	key->state &= 1;		
	return val;
}

/*
==============================
DetermineKeySpeed

==============================
*/
float CInput::DetermineKeySpeed( float frametime )
{
	float speed;

	speed = frametime;

	if ( in_speed.state & 1 )
	{
		speed *= cl_anglespeedkey.GetFloat();
	}

	return speed;
}

/*
==============================
AdjustYaw

==============================
*/
void CInput::AdjustYaw( float speed, QAngle& viewangles )
{
	if ( !(in_strafe.state & 1) )
	{
		viewangles[YAW] -= speed*cl_yawspeed.GetFloat() * KeyState (&in_right);
		viewangles[YAW] += speed*cl_yawspeed.GetFloat() * KeyState (&in_left);
	}
}

/*
==============================
AdjustPitch

==============================
*/
void CInput::AdjustPitch( float speed, QAngle& viewangles )
{
	// only allow keyboard looking if mouse look is disabled
	if ( UsingMouselook() == false )
	{
		float	up, down;

		if ( in_klook.state & 1 )
		{
			view->StopPitchDrift ();
			viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * KeyState (&in_forward);
			viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * KeyState (&in_back);
		}

		up		= KeyState ( &in_lookup );
		down	= KeyState ( &in_lookdown );
		
		viewangles[PITCH] -= speed*cl_pitchspeed.GetFloat() * up;
		viewangles[PITCH] += speed*cl_pitchspeed.GetFloat() * down;

		if ( up || down )
		{
			view->StopPitchDrift ();
		}
	}	
}

/*
==============================
ClampAngles

==============================
*/
void CInput::ClampAngles( QAngle& viewangles )
{
	if ( viewangles[PITCH] > cl_pitchdown.GetFloat() )
	{
		viewangles[PITCH] = cl_pitchdown.GetFloat();
	}
	if ( viewangles[PITCH] < -cl_pitchup.GetFloat() )
	{
		viewangles[PITCH] = -cl_pitchup.GetFloat();
	}

	if ( viewangles[ROLL] > 50 )
	{
		viewangles[ROLL] = 50;
	}
	if ( viewangles[ROLL] < -50 )
	{
		viewangles[ROLL] = -50;
	}
}

/*
================
AdjustAngles

Moves the local angle positions
================
*/
void CInput::AdjustAngles ( float frametime )
{
	float	speed;
	QAngle viewangles;
	
	// Determine control scaling factor ( multiplies time )
	speed = DetermineKeySpeed( frametime );

	// Retrieve latest view direction from engine
	engine->GetViewAngles( viewangles );

	// Adjust YAW
	AdjustYaw( speed, viewangles );

	// Adjust PITCH if keyboard looking
	AdjustPitch( speed, viewangles );
	
	// Make sure values are legitimate
	ClampAngles( viewangles );

	// Store new view angles into engine view direction
	engine->SetViewAngles( viewangles );
}

/*
==============================
ComputeSideMove

==============================
*/
void CInput::ComputeSideMove( CUserCmd *cmd )
{
	// If strafing, check left and right keys and act like moveleft and moveright keys
	if ( in_strafe.state & 1 )
	{
		cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_right);
		cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_left);
	}

	// Otherwise, check strafe keys
	cmd->sidemove += cl_sidespeed.GetFloat() * KeyState (&in_moveright);
	cmd->sidemove -= cl_sidespeed.GetFloat() * KeyState (&in_moveleft);
}

/*
==============================
ComputeUpwardMove

==============================
*/
void CInput::ComputeUpwardMove( CUserCmd *cmd )
{
	cmd->upmove += cl_upspeed.GetFloat() * KeyState (&in_up);
	cmd->upmove -= cl_upspeed.GetFloat() * KeyState (&in_down);
}

/*
==============================
ComputeForwardMove

==============================
*/
void CInput::ComputeForwardMove( CUserCmd *cmd )
{
	if ( !(in_klook.state & 1 ) )
	{	
		cmd->forwardmove += cl_forwardspeed.GetFloat() * KeyState (&in_forward);
		cmd->forwardmove -= cl_backspeed.GetFloat() * KeyState (&in_back);
	}	
}

/*
==============================
ScaleMovements

==============================
*/
void CInput::ScaleMovements( CUserCmd *cmd )
{
	// float spd;

	// clip to maxspeed
	// FIXME FIXME:  This doesn't work
	return;

	/*
	spd = engine->GetClientMaxspeed();
	if ( spd == 0.0 )
		return;

	// Scale the speed so that the total velocity is not > spd
	float fmov = sqrt( (cmd->forwardmove*cmd->forwardmove) + (cmd->sidemove*cmd->sidemove) + (cmd->upmove*cmd->upmove) );

	if ( fmov > spd && fmov > 0.0 )
	{
		float fratio = spd / fmov;

		if ( !IsNoClipping() ) 
		{
			cmd->forwardmove	*= fratio;
			cmd->sidemove		*= fratio;
			cmd->upmove			*= fratio;
		}
	}
	*/
}


/*
===========
ControllerMove
===========
*/
void CInput::ControllerMove( float frametime, CUserCmd *cmd )
{
	if ( IsPC() )
	{
		if ( !m_fCameraInterceptingMouse && m_fMouseActive )
		{
			MouseMove( cmd);
		}
	}

	JoyStickMove( frametime, cmd);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *weapon - 
//-----------------------------------------------------------------------------
void CInput::MakeWeaponSelection( C_BaseCombatWeapon *weapon )
{
	m_hSelectedWeapon = weapon;
}

/*
================
CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/

void CInput::ExtraMouseSample( float frametime, bool active )
{
	CUserCmd dummy;
	CUserCmd *cmd = &dummy;

	cmd->Reset();


	QAngle viewangles;

	if ( active )
	{
		// Determine view angles
		AdjustAngles ( frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( frametime, cmd );
	}

	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Set button and flag bits, don't blow away state
	cmd->buttons = GetButtonBits( 0 );

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Let the move manager override anything it wants to.
	g_pClientMode->CreateMove( frametime, cmd );

	// Get current view angles after the client mode tweaks with it
	engine->SetViewAngles( cmd->viewangles );

	prediction->SetLocalViewAngles( cmd->viewangles );
}

void CInput::CreateMove ( int sequence_number, float input_sample_frametime, bool active )
{	
	CUserCmd *cmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP];

	cmd->Reset();

	cmd->command_number = sequence_number;
	cmd->tick_count = gpGlobals->tickcount;

	QAngle viewangles;

	if ( active || sv_noclipduringpause.GetInt() )
	{
		// Determine view angles
		AdjustAngles ( input_sample_frametime );

		// Determine sideways movement
		ComputeSideMove( cmd );

		// Determine vertical movement
		ComputeUpwardMove( cmd );

		// Determine forward movement
		ComputeForwardMove( cmd );

		// Scale based on holding speed key or having too fast of a velocity based on client maximum
		//  speed.
		ScaleMovements( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( input_sample_frametime, cmd );
	}
	else
	{
		// need to run and reset mouse input so that there is no view pop when unpausing
		if ( !m_fCameraInterceptingMouse && m_fMouseActive )
		{
			float mx, my;
			GetAccumulatedMouseDeltasAndResetAccumulators( &mx, &my );
			ResetMouse();
		}
	}
	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Latch and clear impulse
	cmd->impulse = in_impulse;
	in_impulse = 0;

	// Latch and clear weapon selection
	if ( m_hSelectedWeapon != NULL )
	{
		C_BaseCombatWeapon *weapon = m_hSelectedWeapon;

		cmd->weaponselect = weapon->entindex();
		cmd->weaponsubtype = weapon->GetSubType();

		// Always clear weapon selection
		m_hSelectedWeapon = NULL;
	}

	// Set button and flag bits
	cmd->buttons = GetButtonBits( 1 );

	// Using joystick?
	if ( in_joystick.GetInt() )
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	// Use new view angles if alive, otherwise user last angles we stored off.
	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, m_angPreviousViewAngles );
	}
	else
	{
		VectorCopy( m_angPreviousViewAngles, cmd->viewangles );
	}

	// Let the move manager override anything it wants to.
	g_pClientMode->CreateMove( input_sample_frametime, cmd );

	// Get current view angles after the client mode tweaks with it
	engine->SetViewAngles( cmd->viewangles );

	m_flLastForwardMove = cmd->forwardmove;

	cmd->random_seed = MD5_PseudoRandom( sequence_number ) & 0x7fffffff;

	HLTVCamera()->CreateMove( cmd );

#if defined( HL2_CLIENT_DLL )
	// copy backchannel data
	int i;
	for (i = 0; i < m_EntityGroundContact.Count(); i++)
	{
		cmd->entitygroundcontact.AddToTail( m_EntityGroundContact[i] );
	}
	m_EntityGroundContact.RemoveAll();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::EncodeUserCmdToBuffer( bf_write& buf, int sequence_number )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = GetUserCmd( sequence_number);

	WriteUsercmd( &buf, cmd, &nullcmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CInput::DecodeUserCmdFromBuffer( bf_read& buf, int sequence_number )
{
	CUserCmd nullcmd;
	CUserCmd *cmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP];

	ReadUsercmd( &buf, cmd, &nullcmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			from - 
//			to - 
//-----------------------------------------------------------------------------
bool CInput::WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand )
{
	Assert( m_pCommands );

	CUserCmd nullcmd;

	CUserCmd *f, *t;

	int startbit = buf->GetNumBitsWritten();

	if ( from == -1 )
	{
		f = &nullcmd;
	}
	else
	{
		f = GetUserCmd( from );

		if ( !f )
		{
			// DevMsg( "WARNING! User command delta too old (from %i, to %i)\n", from, to );
			f = &nullcmd;
		}
	}

	t = GetUserCmd( to );

	if ( !t )
	{
		// DevMsg( "WARNING! User command too old (from %i, to %i)\n", from, to );
		t = &nullcmd;
	}

	// Write it into the buffer
	WriteUsercmd( buf, t, f );

	if ( buf->IsOverflowed() )
	{
		int endbit = buf->GetNumBitsWritten();

		Msg( "WARNING! User command buffer overflow(%i %i), last cmd was %i bits long\n",
			from, to,  endbit - startbit );

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : slot - 
// Output : CUserCmd
//-----------------------------------------------------------------------------
CUserCmd *CInput::GetUserCmd( int sequence_number )
{
	Assert( m_pCommands );

	CUserCmd *usercmd = &m_pCommands[ sequence_number % MULTIPLAYER_BACKUP ];

	if ( usercmd->command_number != sequence_number )
	{
		return NULL;	// usercmd was overwritten by newer command
	}
	
	return usercmd;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bits - 
//			in_button - 
//			in_ignore - 
//			*button - 
//			reset - 
// Output : static void
//-----------------------------------------------------------------------------
static void CalcButtonBits( int& bits, int in_button, int in_ignore, kbutton_t *button, bool reset )
{
	// Down or still down?
	if ( button->state & 3 )
	{
		bits |= in_button;
	}

	int clearmask = ~2;
	if ( in_ignore & in_button )
	{
		// This gets taken care of below in the GetButtonBits code
		//bits &= ~in_button;
		// Remove "still down" as well as "just down"
		clearmask = ~3;
	}

	if ( reset )
	{
		button->state &= clearmask;
	}
}

/*
============
GetButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CInput::GetButtonBits( int bResetState )
{
	int bits = 0;

	CalcButtonBits( bits, IN_SPEED, s_ClearInputState, &in_speed, bResetState );
	CalcButtonBits( bits, IN_WALK, s_ClearInputState, &in_walk, bResetState );
	CalcButtonBits( bits, IN_ATTACK, s_ClearInputState, &in_attack, bResetState );
	CalcButtonBits( bits, IN_DUCK, s_ClearInputState, &in_duck, bResetState );
	CalcButtonBits( bits, IN_JUMP, s_ClearInputState, &in_jump, bResetState );
	CalcButtonBits( bits, IN_FORWARD, s_ClearInputState, &in_forward, bResetState );
	CalcButtonBits( bits, IN_BACK, s_ClearInputState, &in_back, bResetState );
	CalcButtonBits( bits, IN_USE, s_ClearInputState, &in_use, bResetState );
	CalcButtonBits( bits, IN_LEFT, s_ClearInputState, &in_left, bResetState );
	CalcButtonBits( bits, IN_RIGHT, s_ClearInputState, &in_right, bResetState );
	CalcButtonBits( bits, IN_MOVELEFT, s_ClearInputState, &in_moveleft, bResetState );
	CalcButtonBits( bits, IN_MOVERIGHT, s_ClearInputState, &in_moveright, bResetState );
	CalcButtonBits( bits, IN_ATTACK2, s_ClearInputState, &in_attack2, bResetState );
	CalcButtonBits( bits, IN_RELOAD, s_ClearInputState, &in_reload, bResetState );
	CalcButtonBits( bits, IN_ALT1, s_ClearInputState, &in_alt1, bResetState );
	CalcButtonBits( bits, IN_ALT2, s_ClearInputState, &in_alt2, bResetState );
	CalcButtonBits( bits, IN_SCORE, s_ClearInputState, &in_score, bResetState );
	CalcButtonBits( bits, IN_ZOOM, s_ClearInputState, &in_zoom, bResetState );
	CalcButtonBits( bits, IN_GRENADE1, s_ClearInputState, &in_grenade1, bResetState );
	CalcButtonBits( bits, IN_GRENADE2, s_ClearInputState, &in_grenade2, bResetState );

	// Cancel is a special flag
	if (in_cancel)
	{
		bits |= IN_CANCEL;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON1 )
	{
		bits |= IN_WEAPON1;
	}

	if ( gHUD.m_iKeyBits & IN_WEAPON2 )
	{
		bits |= IN_WEAPON2;
	}

	// Clear out any residual
	bits &= ~s_ClearInputState;

	if ( bResetState )
	{
		s_ClearInputState = 0;
	}

	return bits;
}


//-----------------------------------------------------------------------------
// Causes an input to have to be re-pressed to become active
//-----------------------------------------------------------------------------
void CInput::ClearInputButton( int bits )
{
	s_ClearInputState |= bits;
}


/*
==============================
GetLookSpring

==============================
*/
float CInput::GetLookSpring( void )
{
	return lookspring.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CInput::GetLastForwardMove( void )
{
	return m_flLastForwardMove;
}


#if defined( HL2_CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: back channel contact info for ground contact
// Output :
//-----------------------------------------------------------------------------

void CInput::AddIKGroundContactInfo( int entindex, float minheight, float maxheight )
{
	CEntityGroundContact data;
	data.entindex = entindex;
	data.minheight = minheight;
	data.maxheight = maxheight;

	if (m_EntityGroundContact.Count() >= MAX_EDICTS)
	{
		// some overflow here, probably bogus anyway
		Assert(0);
		m_EntityGroundContact.RemoveAll();
		return;
	}

	m_EntityGroundContact.AddToTail( data );
}
#endif


static ConCommand startcommandermousemove("+commandermousemove", IN_CommanderMouseMoveDown);
static ConCommand endcommandermousemove("-commandermousemove", IN_CommanderMouseMoveUp);
static ConCommand startmoveup("+moveup",IN_UpDown);
static ConCommand endmoveup("-moveup",IN_UpUp);
static ConCommand startmovedown("+movedown",IN_DownDown);
static ConCommand endmovedown("-movedown",IN_DownUp);
static ConCommand startleft("+left",IN_LeftDown);
static ConCommand endleft("-left",IN_LeftUp);
static ConCommand startright("+right",IN_RightDown);
static ConCommand endright("-right",IN_RightUp);
static ConCommand startforward("+forward",IN_ForwardDown);
static ConCommand endforward("-forward",IN_ForwardUp);
static ConCommand startback("+back",IN_BackDown);
static ConCommand endback("-back",IN_BackUp);
static ConCommand startlookup("+lookup", IN_LookupDown);
static ConCommand endlookup("-lookup", IN_LookupUp);
static ConCommand startlookdown("+lookdown", IN_LookdownDown);
static ConCommand lookdown("-lookdown", IN_LookdownUp);
static ConCommand startstrafe("+strafe", IN_StrafeDown);
static ConCommand endstrafe("-strafe", IN_StrafeUp);
static ConCommand startmoveleft("+moveleft", IN_MoveleftDown);
static ConCommand endmoveleft("-moveleft", IN_MoveleftUp);
static ConCommand startmoveright("+moveright", IN_MoverightDown);
static ConCommand endmoveright("-moveright", IN_MoverightUp);
static ConCommand startspeed("+speed", IN_SpeedDown);
static ConCommand endspeed("-speed", IN_SpeedUp);
static ConCommand startwalk("+walk", IN_WalkDown);
static ConCommand endwalk("-walk", IN_WalkUp);
static ConCommand startattack("+attack", IN_AttackDown);
static ConCommand endattack("-attack", IN_AttackUp);
static ConCommand startattack2("+attack2", IN_Attack2Down);
static ConCommand endattack2("-attack2", IN_Attack2Up);
static ConCommand startuse("+use", IN_UseDown);
static ConCommand enduse("-use", IN_UseUp);
static ConCommand startjump("+jump", IN_JumpDown);
static ConCommand endjump("-jump", IN_JumpUp);
static ConCommand impulse("impulse", IN_Impulse);
static ConCommand startklook("+klook", IN_KLookDown);
static ConCommand endklook("-klook", IN_KLookUp);
static ConCommand startjlook("+jlook", IN_JLookDown);
static ConCommand endjlook("-jlook", IN_JLookUp);
static ConCommand startduck("+duck", IN_DuckDown);
static ConCommand endduck("-duck", IN_DuckUp);
static ConCommand startreload("+reload", IN_ReloadDown);
static ConCommand endreload("-reload", IN_ReloadUp);
static ConCommand startalt1("+alt1", IN_Alt1Down);
static ConCommand endalt1("-alt1", IN_Alt1Up);
static ConCommand startalt2("+alt2", IN_Alt2Down);
static ConCommand endalt2("-alt2", IN_Alt2Up);
static ConCommand startscore("+score", IN_ScoreDown);
static ConCommand endscore("-score", IN_ScoreUp);
static ConCommand startshowscores("+showscores", IN_ScoreDown);
static ConCommand endshowscores("-showscores", IN_ScoreUp);
static ConCommand startgraph("+graph", IN_GraphDown);
static ConCommand endgraph("-graph", IN_GraphUp);
static ConCommand startbreak("+break",IN_BreakDown);
static ConCommand endbreak("-break",IN_BreakUp);
static ConCommand force_centerview("force_centerview", IN_CenterView_f);
static ConCommand joyadvancedupdate("joyadvancedupdate", IN_Joystick_Advanced_f);
static ConCommand startzoom("+zoom", IN_ZoomDown);
static ConCommand endzoom("-zoom", IN_ZoomUp);
static ConCommand endgrenade1( "-grenade1", IN_Grenade1Up );
static ConCommand startgrenade1( "+grenade1", IN_Grenade1Down );
static ConCommand endgrenade2( "-grenade2", IN_Grenade2Up );
static ConCommand startgrenade2( "+grenade2", IN_Grenade2Down );

/*
============
Init_All
============
*/
void CInput::Init_All (void)
{
	Assert( !m_pCommands );
	m_pCommands = new CUserCmd[ MULTIPLAYER_BACKUP ];

	m_fMouseInitialized	= false;
	m_fRestoreSPI		= false;
	m_fMouseActive		= false;
	Q_memset( m_rgOrigMouseParms, 0, sizeof( m_rgOrigMouseParms ) );
	Q_memset( m_rgNewMouseParms, 0, sizeof( m_rgNewMouseParms ) );
	Q_memset( m_rgCheckMouseParam, 0, sizeof( m_rgCheckMouseParam ) );

	m_rgNewMouseParms[ MOUSE_ACCEL_THRESHHOLD1 ] = 0; // no 2x
	m_rgNewMouseParms[ MOUSE_ACCEL_THRESHHOLD2 ] = 0; // no 4x
	m_rgNewMouseParms[ MOUSE_SPEED_FACTOR ] = 1; // slowest (10 default, 20 max)

	m_fMouseParmsValid	= false;
	m_fJoystickAdvancedInit = false;
	m_flLastForwardMove = 0.0;

	// Initialize inputs
	if ( IsPC() )
	{
		Init_Mouse ();
		Init_Keyboard();
	}
		
	// Initialize third person camera controls.
	Init_Camera();
}

/*
============
Shutdown_All
============
*/
void CInput::Shutdown_All(void)
{
	DeactivateMouse();
	Shutdown_Keyboard();

	delete[] m_pCommands;
	m_pCommands = NULL;
}

void CInput::LevelInit( void )
{
#if defined( HL2_CLIENT_DLL )
	// Remove any IK information
	m_EntityGroundContact.RemoveAll();
#endif
}

