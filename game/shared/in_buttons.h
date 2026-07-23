#ifndef IN_BUTTONS_H
#define IN_BUTTONS_H
#ifdef _WIN32
#pragma once
#endif

#include "platform.h"

enum InputBitMask_t : int64
{
	IN_NONE	= 0,
	IN_ALL	= -1,

	IN_ATTACK			= (1 << 0),
	IN_JUMP				= (1 << 1),
	IN_DUCK				= (1 << 2),
	IN_FORWARD			= (1 << 3),
	IN_BACK				= (1 << 4),
	IN_USE				= (1 << 5),
	IN_TURNLEFT			= (1 << 7),
	IN_TURNRIGHT		= (1 << 8),
	IN_MOVELEFT			= (1 << 9),
	IN_MOVERIGHT		= (1 << 10),
	IN_ATTACK2			= (1 << 11),
	IN_RELOAD			= (1 << 13),
	IN_SPEED			= (1 << 16), // Player is holding the speed key
	IN_JOYAUTOSPRINT	= (1 << 17),

	IN_FIRST_MOD_SPECIFIC_BIT = (1 << 32),
};

#endif // IN_BUTTONS_H