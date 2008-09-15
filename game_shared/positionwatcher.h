//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef POSITIONWATCHER_H
#define POSITIONWATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "ehandle.h"

class IPositionWatcher;

struct positionwatcher_t
{
	EHANDLE				hWatcher;
	IPositionWatcher	*pWatcherCallback;
};

class CPositionWatcherList
{
public:
	//CPositionWatcherList(); NOTE: Dataobj doesn't support constructors - it zeros the memory
	~CPositionWatcherList();	// frees the positionwatcher_t's to the pool
	void Init();

	void NotifyWatchers( CBaseEntity *pEntity );
	void AddToList( CBaseEntity *pWatcher );
	void RemoveWatcher( CBaseEntity *pWatcher );

private:
	unsigned short Find( CBaseEntity *pEntity );

	unsigned short m_list;
};

// inherit from this interface to be able to call WatchPositionChanges
abstract_class IPositionWatcher
{
public:
	virtual void NotifyPositionChanged( CBaseEntity *pEntity ) = 0;
};

// NOTE: The table of watchers is NOT saved/loaded!  Recreate these links on restore
void ReportPositionChanged( CBaseEntity *pMovedEntity );
void WatchPositionChanges( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity );
void RemovePositionWatcher( CBaseEntity *pWatcher, CBaseEntity *pMovingEntity );


#endif // POSITIONWATCHER_H
