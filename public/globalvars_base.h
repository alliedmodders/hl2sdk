//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GLOBALVARS_BASE_H
#define GLOBALVARS_BASE_H

#ifdef _WIN32
#pragma once
#endif

class CSaveRestoreData;

//-----------------------------------------------------------------------------
// Purpose: Global variables used by shared code
//-----------------------------------------------------------------------------
class CGlobalVarsBase
{
public:

	CGlobalVarsBase( bool bIsClient );
	
	// This can be used to filter debug output or to catch the client or server in the act.
	// ONLY valid during debugging.
#ifdef _DEBUG
	bool IsClient() const;
#endif
	

public:
	
	// Absolute time (per frame still)
	float			realtime;
	// Absolute frmae counter
	int				framecount;
	// Non-paused frametime
	float			absoluteframetime;

	// Current time 
	//
	// On the client, this (along with tickcount) takes a different meaning based on what
	// piece of code you're in:
	// 
	//   - While receiving network packets (like in PreDataUpdate/PostDataUpdate and proxies),
	//     this is set to the SERVER TICKCOUNT for that packet. There is no interval between
	//     the server ticks.
	//     [server_current_Tick * tick_interval]
	//
	//   - While rendering, this is the exact client clock 
	//     [(client_current_tick + interpolation_amount) * tick_interval]
	//
	//   - During prediction, this is based on the client's current tick:
	//     [client_current_tick * tick_interval]
	float			curtime;
	
	// Time spent on last server or client frame (has nothing to do with think intervals)
	float			frametime;
	// current maxplayers
	int				maxClients;

	// Simulation ticks
	int				tickcount;

	// Simulation tick interval
	float			interval_per_tick;

	// interpolation amount ( client-only ) based on fraction of next tick which has elapsed
	float			interpolation_amount;

	// current saverestore data
	CSaveRestoreData *pSaveData;


private:
	// Set to true in client code. This can only be used for debugging code.
	bool			m_bClient;
};


inline CGlobalVarsBase::CGlobalVarsBase( bool bIsClient )
{
	m_bClient = bIsClient;
}


#ifdef _DEBUG
	inline bool CGlobalVarsBase::IsClient() const
	{
		return m_bClient;
	}
#endif


#endif // GLOBALVARS_BASE_H
