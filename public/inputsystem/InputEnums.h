//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef INPUTENUMS_H
#define INPUTENUMS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/basetypes.h"
#include "tier0/platwindow.h"

FORWARD_DECLARE_HANDLE( InputContextHandle_t );

// Standard maximum +/- value of a joystick axis
#define MAX_BUTTONSAMPLE			32768

#if !defined( _X360 )
#define INVALID_USER_ID		-1
#else
#define INVALID_USER_ID		XBX_INVALID_USER_ID
#endif

//-----------------------------------------------------------------------------
// Forward declarations: 
//-----------------------------------------------------------------------------

enum
{
	MAX_JOYSTICKS = MAX_SPLITSCREEN_CLIENTS,
	MOUSE_BUTTON_COUNT = 5,
};

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
 	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum JoystickDeadzoneMode_t
{
	JOYSTICK_DEADZONE_CROSS = 0,
	JOYSTICK_DEADZONE_SQUARE = 1,
};


//-----------------------------------------------------------------------------
// Extra mouse codes
//-----------------------------------------------------------------------------
enum
{
	MS_WM_XBUTTONDOWN	= 0x020B,
	MS_WM_XBUTTONUP		= 0x020C,
	MS_WM_XBUTTONDBLCLK	= 0x020D,
	MS_MK_BUTTON4		= 0x0020,
	MS_MK_BUTTON5		= 0x0040,
};

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------
enum InputEventType_t
{
	IE_ButtonPressed = 0,	// m_nData contains a ButtonCode_t
	IE_ButtonReleased,		// m_nData contains a ButtonCode_t
	IE_ButtonDoubleClicked,	// m_nData contains a ButtonCode_t
	IE_AnalogValueChanged,	// m_nData contains an AnalogCode_t, m_nData2 contains the value

	IE_FirstSystemEvent = 100,
	IE_Quit = IE_FirstSystemEvent,
	IE_ControllerInserted,	// m_nData contains the controller ID
	IE_ControllerUnplugged,	// m_nData contains the controller ID
	IE_Close,
	IE_WindowSizeChanged,	// m_nData contains width, m_nData2 contains height, m_nData3 = 0 if not minimized, 1 if minimized

	IE_FirstUIEvent = 200,
	IE_LocateMouseClick = IE_FirstUIEvent,
	IE_SetCursor,
	IE_KeyTyped,
	IE_KeyCodeTyped,
	IE_InputLanguageChanged,
	IE_IMESetWindow,
	IE_IMEStartComposition,
	IE_IMEComposition,
	IE_IMEEndComposition,
	IE_IMEShowCandidates,
	IE_IMEChangeCandidates,
	IE_IMECloseCandidates,
	IE_IMERecomputeModes,

	IE_FirstVguiEvent = 1000,	// Assign ranges for other systems that post user events here
	IE_FirstAppEvent = 2000,
};

struct InputEvent_t
{
	PlatWindow_t m_hWnd;
	int m_nType;				// Type of the event (see InputEventType_t)
	int m_nTick;				// Tick on which the event occurred
	uint64 m_nData;				// Generic 64-bit data, what it contains depends on the event
	int m_nData2;				// Generic 32-bit data, what it contains depends on the event
	int m_nData3;				// Generic 32-bit data, what it contains depends on the event
};

#endif // INPUTENUMS_H
