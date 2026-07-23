#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#ifdef _WIN32
#pragma once
#endif

#include "globalvars_base.h"
#include "string_t.h"


//-----------------------------------------------------------------------------
// Purpose: Defines the ways that a map can be loaded.
//-----------------------------------------------------------------------------
enum MapLoadType_t
{
	MapLoad_NewGame = 0,
	MapLoad_LoadGame,
	MapLoad_Transition,
	MapLoad_Background,
};


//-----------------------------------------------------------------------------
// Purpose: Global variables shared between the engine and the game .dll
//-----------------------------------------------------------------------------
class CGlobalVars : public CGlobalVarsBase
{	
public:

	CGlobalVars();

public:
	// Current map
	string_t		mapname;
	string_t		startspot;
	MapLoadType_t	eLoadType;		// How the current map was loaded
	bool mp_teamplay;

	// current maxentities
	int				maxEntities;

	int				serverCount;
};

inline CGlobalVars::CGlobalVars() : 
	CGlobalVarsBase()
{
	serverCount = 0;
}

#endif // GLOBALVARS_H
