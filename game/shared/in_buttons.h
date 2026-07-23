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

	IN_ATTACK			= (1ll << 0),
	IN_JUMP				= (1ll << 1),
	IN_DUCK				= (1ll << 2),
	IN_FORWARD			= (1ll << 3),
	IN_BACK				= (1ll << 4),
	IN_USE				= (1ll << 5),
	IN_TURNLEFT			= (1ll << 7),
	IN_TURNRIGHT		= (1ll << 8),
	IN_MOVELEFT			= (1ll << 9),
	IN_MOVERIGHT		= (1ll << 10),
	IN_ATTACK2			= (1ll << 11),
	IN_RELOAD			= (1ll << 13),
	IN_SPEED			= (1ll << 16), // Player is holding the speed key
	IN_JOYAUTOSPRINT	= (1ll << 17),

	IN_FIRST_MOD_SPECIFIC_BIT = (1ll << 32),

	IN_USEORRELOAD		= IN_FIRST_MOD_SPECIFIC_BIT,
	IN_SCORE			= (1ll << 33), // Used by client.dll for when scoreboard is held down
	IN_ZOOM				= (1ll << 34), // Zoom key for HUD zoom
	IN_LOOK_AT_WEAPON	= (1ll << 35)
};

#endif // IN_BUTTONS_H