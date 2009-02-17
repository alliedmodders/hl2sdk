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

#include "vector.h"
#include "cmodel.h"
#include "const.h"
#include "iserverentity.h"
#include "globalvars_base.h"
#include "engine/ICollideable.h"
#include "iservernetworkable.h"
#include "bitvec.h"

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

	CGlobalVars( bool bIsClient );

public:
	
	// Current map
	string_t		mapname;
	int				mapversion;
	string_t		startspot;
	MapLoadType_t	eLoadType;	// How the current map was loaded

	// game specific flags
	bool			deathmatch;
	bool			coop;
	bool			teamplay;
	// current maxentities
	int				maxEntities;
};

inline CGlobalVars::CGlobalVars( bool bIsClient ) : 
	CGlobalVarsBase( bIsClient )
{
}


class CPlayerState;
class IServerNetworkable;
class IServerEntity;


#define FL_EDICT_CHANGED	(1<<0)	// Game DLL sets this when the entity state changes
#define FL_EDICT_FREE		(1<<1)	// this edict if free for reuse
#define FL_EDICT_FULL		(1<<2)	// this is a full server entity

#define FL_EDICT_FULLCHECK	(0<<0)  // call ShouldTransmit() each time, this is a fake flag
#define FL_EDICT_ALWAYS		(1<<3)	// always transmit this entity
#define FL_EDICT_DONTSEND	(1<<4)	// don't transmit this entity
#define FL_EDICT_PVSCHECK	(1<<5)	// always transmit entity, but cull against PVS


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseEdict
{
public:

	// Returns an IServerEntity if FL_FULLEDICT is set or NULL if this 
	// is a lightweight networking entity.
	IServerEntity*			GetIServerEntity();
	const IServerEntity*	GetIServerEntity() const;

	IServerNetworkable*		GetNetworkable();
	IServerUnknown*			GetUnknown();

	// Set when initting an entity. If it's only a networkable, this is false.
	void				SetEdict( IServerUnknown *pUnk, bool bFullEdict );
	
	int					AreaNum() const;
	const char *		GetClassName() const;
	
	bool				IsFree() const;
	void				SetFree();
	void				ClearFree();

	bool				HasStateChanged() const;
	void				ClearStateChanged();
	void				StateChanged();

	void				ClearTransmitState();
		

public:

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	int	m_fStateFlags;	

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	int m_NetworkSerialNumber;	// Game DLL sets this when it gets a serial number for its EHANDLE.

	// NOTE: this is in the edict instead of being accessed by a virtual because the engine needs fast access to it.
	IServerNetworkable	*m_pNetworkable;

protected:
	IServerUnknown		*m_pUnk;		


	friend void InitializeEntityDLLFields( edict_t *pEdict );
};


//-----------------------------------------------------------------------------
// CBaseEdict inlines.
//-----------------------------------------------------------------------------
inline IServerEntity* CBaseEdict::GetIServerEntity()
{
	if ( m_fStateFlags & FL_EDICT_FULL )
		return (IServerEntity*)m_pUnk;
	else
		return 0;
}

inline bool CBaseEdict::IsFree() const
{
	return (m_fStateFlags & FL_EDICT_FREE) != 0;
}



inline bool	CBaseEdict::HasStateChanged() const
{
	return (m_fStateFlags & FL_EDICT_CHANGED) != 0;
}

inline void	CBaseEdict::ClearStateChanged()
{
	m_fStateFlags &= ~FL_EDICT_CHANGED;
}

inline void	CBaseEdict::StateChanged()
{
	m_fStateFlags |= FL_EDICT_CHANGED;
}

inline void CBaseEdict::SetFree()
{
	m_fStateFlags |= FL_EDICT_FREE;
}

inline void CBaseEdict::ClearFree()
{
	m_fStateFlags &= ~FL_EDICT_FREE;
}

inline void	CBaseEdict::ClearTransmitState()
{
	m_fStateFlags &= ~(FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_DONTSEND);
}

inline const IServerEntity* CBaseEdict::GetIServerEntity() const
{
	if ( m_fStateFlags & FL_EDICT_FULL )
		return (IServerEntity*)m_pUnk;
	else
		return 0;
}

inline IServerUnknown* CBaseEdict::GetUnknown()
{
	return m_pUnk;
}

inline IServerNetworkable* CBaseEdict::GetNetworkable()
{
	return m_pNetworkable;
}

inline void CBaseEdict::SetEdict( IServerUnknown *pUnk, bool bFullEdict )
{
	m_pUnk = pUnk;
	if ( (pUnk != NULL) && bFullEdict )
	{
		m_fStateFlags = FL_EDICT_FULL;
	}
	else
	{
		m_fStateFlags = 0;
	}
}

inline int CBaseEdict::AreaNum() const
{
	if ( !m_pUnk )
		return 0;

	return m_pNetworkable->AreaNum();
}

inline const char *	CBaseEdict::GetClassName() const
{
	if ( !m_pUnk )
		return "";
	return m_pNetworkable->GetClassName();
}


//-----------------------------------------------------------------------------
// Purpose: The engine's internal representation of an entity, including some
//  basic collision and position info and a pointer to the class wrapped on top
//  of the structure
//-----------------------------------------------------------------------------
struct edict_t : public CBaseEdict
{
public:
	ICollideable *GetCollideable();

	// The server timestampe at which the edict was freed (so we can try to use other edicts before reallocating this one)
	float		freetime;	
};

inline ICollideable *edict_t::GetCollideable()
{
	IServerEntity *pEnt = GetIServerEntity();
	if ( pEnt )
		return pEnt->GetCollideable();
	else
		return NULL;
}

#endif // EDICT_H
