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

	IN_WEAPON1			= (1ll << 32),
	IN_ABILITY1			= (1ll << 33),
	IN_ABILITY2			= (1ll << 34),
	IN_ABILITY3			= (1ll << 35),
	IN_ABILITY4			= (1ll << 36),
	IN_ITEM1			= (1ll << 37),
	IN_ITEM2			= (1ll << 38),
	IN_ITEM3			= (1ll << 39),
	IN_ITEM4			= (1ll << 40),
	IN_ITEM5			= (1ll << 41),
	IN_ABILITY_HELD		= (1ll << 42),
	IN_INNATE_1			= (1ll << 44),
	IN_INNATE_2			= (1ll << 45),
	IN_INNATE_3			= (1ll << 46),
	IN_MANTLE			= (1ll << 48),
	IN_SPEC_NEXT		= (1ll << 49),
	IN_SPEC_PREV		= (1ll << 50),
	IN_SPEC_MODE		= (1ll << 51),
	IN_SPEC_TOGGLE_TEAM	= (1ll << 52),
	IN_ALT_CAST			= (1ll << 53),
	IN_REPLAY_DEATH		= (1ll << 54),
	IN_TELEPORT			= (1ll << 55),
	IN_ZIPLINE			= (1ll << 56),
	IN_MOVE_UP			= (1ll << 57),
	IN_MOVE_DOWN		= (1ll << 58),
	IN_DUCK_TOGGLE		= (1ll << 59),
	IN_CANCEL_ABILITY	= (1ll << 60),
	IN_COSMETIC_1		= (1ll << 61)
};

#endif // IN_BUTTONS_H