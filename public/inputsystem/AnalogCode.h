//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ANALOGCODE_H
#define ANALOGCODE_H

#ifdef _WIN32
#pragma once
#endif


#include "inputsystem/InputEnums.h"


//-----------------------------------------------------------------------------
// Macro to get at joystick codes
//-----------------------------------------------------------------------------
#define JOYSTICK_AXIS( _joystick, _axis ) ( JOYSTICK_FIRST_AXIS + ((_joystick) * MAX_JOYSTICK_AXES) + (_axis) )


//-----------------------------------------------------------------------------
// Enumeration for analog input devices. Includes joysticks, mousewheel, mouse
//-----------------------------------------------------------------------------
enum AnalogCode_t
{
	MOUSE_X = 0,
	MOUSE_Y,
	MOUSE_WHEEL,

	JOYSTICK_FIRST_AXIS,
	JOYSTICK_LAST_AXIS = JOYSTICK_AXIS( MAX_JOYSTICKS-1, MAX_JOYSTICK_AXES-1 ),

	ANALOG_CODE_LAST,
};


#endif // ANALOGCODE_H
