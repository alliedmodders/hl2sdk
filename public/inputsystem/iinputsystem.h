//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef IINPUTSYSTEM_H
#define IINPUTSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/platwindow.h"
#include "appframework/IAppSystem.h"
#include "inputsystem/InputEnums.h"
#include "inputsystem/ButtonCode.h"
#include "inputsystem/AnalogCode.h"


///-----------------------------------------------------------------------------
/// A handle to a cursor icon
///-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( InputCursorHandle_t );
#define INPUT_CURSOR_HANDLE_INVALID ( (InputCursorHandle_t)0 )


///-----------------------------------------------------------------------------
/// An enumeration describing well-known cursor icons
///-----------------------------------------------------------------------------
enum InputStandardCursor_t
{
	INPUT_CURSOR_NONE	= 0,
	INPUT_CURSOR_ARROW,
	INPUT_CURSOR_IBEAM,   
	INPUT_CURSOR_HOURGLASS,
	INPUT_CURSOR_CROSSHAIR,
	INPUT_CURSOR_WAITARROW,
	INPUT_CURSOR_UP,
	INPUT_CURSOR_SIZE_NW_SE,
	INPUT_CURSOR_SIZE_NE_SW,
	INPUT_CURSOR_SIZE_W_E,
	INPUT_CURSOR_SIZE_N_S,
	INPUT_CURSOR_SIZE_ALL,
	INPUT_CURSOR_NO,
	INPUT_CURSOR_HAND,

	INPUT_CURSOR_COUNT
};


///-----------------------------------------------------------------------------
/// Main interface for input. This is a low-level interface, creating an
/// OS-independent queue of low-level input events which were sampled since
/// the last call to PollInputState. It also contains facilities for cursor
/// control and creation.
///-----------------------------------------------------------------------------
abstract_class IInputSystem : public IAppSystem
{
public:
	/// Attach, detach input system from a particular window
	/// This window should be the root window for the application
	/// Only 1 window should be attached at any given time.
	virtual void AttachToWindow( void* hWnd ) = 0;
	virtual void DetachFromWindow( ) = 0;

	/// Enables/disables input. PollInputState will not update current 
	/// button/analog states when it is called if the system is disabled.
	virtual void EnableInput( bool bEnable ) = 0;

	/// Enables/disables the windows message pump. PollInputState will not
	/// Peek/Dispatch messages if this is disabled
	virtual void EnableMessagePump( bool bEnable ) = 0;

	/// Polls the current input state
	virtual void PollInputState() = 0;

	/// Gets the time of the last polling in ms
	virtual int GetPollTick() const = 0;

	/// Is a button down? "Buttons" are binary-state input devices (mouse buttons, keyboard keys)
	virtual bool IsButtonDown( ButtonCode_t code ) const = 0;

	/// Returns the tick at which the button was pressed and released
	virtual int GetButtonPressedTick( ButtonCode_t code ) const = 0;
	virtual int GetButtonReleasedTick( ButtonCode_t code ) const = 0;

	/// Gets the value of an analog input device this frame
	/// Includes joysticks, mousewheel, mouse
	virtual int GetAnalogValue( AnalogCode_t code ) const = 0;

	/// Gets the change in a particular analog input device this frame
	/// Includes joysticks, mousewheel, mouse
	virtual int GetAnalogDelta( AnalogCode_t code ) const = 0;

	/// Returns the input events since the last poll
	virtual int GetEventCount() const = 0;
	virtual const InputEvent_t* GetEventData( ) const = 0;

	/// Posts a user-defined event into the event queue; this is expected
	/// to be called in overridden wndprocs connected to the root panel.
	virtual void PostUserEvent( const InputEvent_t &event ) = 0;

	/// Returns the number of joysticks
	virtual int GetJoystickCount() const = 0;

	/// Enable/disable joystick, it has perf costs
	virtual void EnableJoystickInput( int nJoystick, bool bEnable ) = 0;

	/// Enable/disable diagonal joystick POV (simultaneous POV buttons down)
	virtual void EnableJoystickDiagonalPOV( int nJoystick, bool bEnable ) = 0;

	/// Sample the joystick and append events to the input queue
	virtual void SampleDevices( void ) = 0;

	// FIXME: Currently force-feedback is only supported on the Xbox 360
	virtual void SetRumble( float fLeftMotor, float fRightMotor, int userId = INVALID_USER_ID ) = 0;
	virtual void StopRumble( int userId = INVALID_USER_ID ) = 0;

	/// Resets the input state
	virtual void ResetInputState() = 0;

	/// Sets a player as the primary user - all other controllers will be ignored.
	virtual void SetPrimaryUserId( int userId ) = 0;

	/// Convert back + forth between ButtonCode/AnalogCode + strings
	virtual const char *ButtonCodeToString( ButtonCode_t code ) const = 0;
	virtual const char *AnalogCodeToString( AnalogCode_t code ) const = 0;
	virtual ButtonCode_t StringToButtonCode( const char *pString ) const = 0;
	virtual AnalogCode_t StringToAnalogCode( const char *pString ) const = 0;

	/// Sleeps until input happens. Pass a negative number to sleep infinitely
	virtual void SleepUntilInput( int nMaxSleepTimeMS = -1 ) = 0;

	/// Convert back + forth between virtual codes + button codes
	// FIXME: This is a temporary piece of code
	virtual ButtonCode_t VirtualKeyToButtonCode( int nVirtualKey ) const = 0;
	virtual int ButtonCodeToVirtualKey( ButtonCode_t code ) const = 0;
	virtual ButtonCode_t ScanCodeToButtonCode( int lParam ) const = 0;

	/// How many times have we called PollInputState?
	virtual int GetPollCount() const = 0;

	/// Sets the cursor position
	virtual void SetCursorPosition( int x, int y ) = 0;

	/// Tells the input system to generate UI-related events, defined
	/// in inputsystem/inputenums.h (see IE_FirstUIEvent)
	/// We could have multiple clients that care about UI-related events
	/// so we refcount the clients with an Add/Remove strategy. If there
	/// are no interested clients, the UI events are not generated
	virtual void AddUIEventListener() = 0;
	virtual void RemoveUIEventListener() = 0;

	/// Returns the currently attached window
	virtual PlatWindow_t GetAttachedWindow() const = 0;

	/// Creates a cursor using one of the well-known cursor icons
	virtual InputCursorHandle_t GetStandardCursor( InputStandardCursor_t id ) = 0;

	/// Loads a cursor defined in a file
	virtual InputCursorHandle_t LoadCursorFromFile( const char *pFileName, const char *pPathID = NULL ) = 0;

	/// Sets the cursor icon
	virtual void SetCursorIcon( InputCursorHandle_t hCursor ) = 0;

	/// Gets the cursor position
	virtual void GetCursorPosition( int *pX, int *pY ) = 0;

	/// Mouse capture
	virtual void EnableMouseCapture( PlatWindow_t hWnd ) = 0;
	virtual void DisableMouseCapture() = 0;
};

DECLARE_TIER2_INTERFACE( IInputSystem, g_pInputSystem );


#endif // IINPUTSYSTEM_H
