//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDICT_H
#define EDICT_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include "cmodel.h"
#include "const.h"
#include "iserverentity.h"
#include "globalvars_base.h"
#include "engine/ICollideable.h"
#include "iservernetworkable.h"
#include "bitvec.h"
#include "tier1/convar.h"

struct edict_t;


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


class CPlayerState;
class IServerNetworkable;
class IServerEntity;


#define FL_EDICT_CHANGED	(1<<0)	// Game DLL sets this when the entity state changes
									// Mutually exclusive with FL_EDICT_PARTIAL_CHANGE.
									
#define FL_EDICT_FREE		(1<<1)	// this edict if free for reuse
#define FL_EDICT_FULL		(1<<2)	// this is a full server entity

#define FL_EDICT_FULLCHECK	(0<<0)  // call ShouldTransmit() each time, this is a fake flag
#define FL_EDICT_ALWAYS		(1<<3)	// always transmit this entity
#define FL_EDICT_DONTSEND	(1<<4)	// don't transmit this entity
#define FL_EDICT_PVSCHECK	(1<<5)	// always transmit entity, but cull against PVS

// Used by local network backdoor.
#define FL_EDICT_PENDING_DORMANT_CHECK	(1<<6)

// This is always set at the same time EFL_DIRTY_PVS_INFORMATION is set, but it 
// gets cleared in a different place.
#define FL_EDICT_DIRTY_PVS_INFORMATION	(1<<7)

// This is used internally to edict_t to remember that it's carrying a 
// "full change list" - all its properties might have changed their value.
#define FL_FULL_EDICT_CHANGED			(1<<8)


// Max # of variable changes we'll track in an entity before we treat it
// like they all changed.
#define MAX_CHANGE_OFFSETS	19
#define MAX_EDICT_CHANGE_INFOS	100


class CEdictChangeInfo
{
public:
	// Edicts remember the offsets of properties that change 
	unsigned short m_ChangeOffsets[MAX_CHANGE_OFFSETS];
	unsigned short m_nChangeOffsets;
};

// Shared between engine and game DLL.
class CSharedEdictChangeInfo
{
public:
	CSharedEdictChangeInfo()
	{
		m_iSerialNumber = 1;
	}
	
	// Matched against edict_t::m_iChangeInfoSerialNumber to determine if its
	// change info is valid.
	unsigned short m_iSerialNumber;
	
#ifdef NETWORK_VARS_ENABLED
	CEdictChangeInfo m_ChangeInfos[MAX_EDICT_CHANGE_INFOS];
	unsigned short m_nChangeInfos;	// How many are in use this frame.
#endif
};
extern CSharedEdictChangeInfo *g_pSharedChangeInfo;

class IChangeInfoAccessor
{
public:
	inline void SetChangeInfo( unsigned short info )
	{
		m_iChangeInfo = info;
	}

	inline void SetChangeInfoSerialNumber( unsigned short sn )
	{
		m_iChangeInfoSerialNumber = sn;
	}

	inline unsigned short	 GetChangeInfo() const
	{
		return m_iChangeInfo;
	}

	inline unsigned short	 GetChangeInfoSerialNumber() const
	{
		return m_iChangeInfoSerialNumber;
	}

private:
	unsigned short m_iChangeInfo;
	unsigned short m_iChangeInfoSerialNumber;
};

#endif // EDICT_H
