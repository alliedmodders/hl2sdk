//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GLOBALVARS_BASE_H
#define GLOBALVARS_BASE_H

#ifdef _WIN32
#pragma once
#endif

enum GlobalVarsUsageWarning_t
{
	GV_RENDERTIME_CALLED_DURING_SIMULATION,
	GV_CURTIME_CALLED_DURING_RENDERING
};

typedef void (*FnGlobalVarsWarningFunc)(GlobalVarsUsageWarning_t);

//-----------------------------------------------------------------------------
// Purpose: Global variables used by shared code
//-----------------------------------------------------------------------------
class CGlobalVarsBase
{
public:

	CGlobalVarsBase();

public:
	// Absolute time (per frame still - Use Plat_FloatTime() for a high precision real time 
	//  perf clock, but not that it doesn't obey host_timescale/host_framerate)
	float realtime;
	// Absolute frame counter - continues to increase even if game is paused
	int framecount;

	// Non-paused frametime
	float absoluteframetime;
	float absoluteframestarttimestddev;

	int maxClients;

	// zer0k: Command queue related
	int unknown1;
	int unknown2;

	FnGlobalVarsWarningFunc m_pfnWarningFunc;

	// Time spent on last server or client frame (has nothing to do with think intervals)
	float frametime;

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
	//     [client_current_tick * tick_interval + interpolation_amount]
	//
	//   - During prediction, this is based on the client's current tick:
	//     [client_current_tick * tick_interval]
	float curtime;
	float rendertime;

	// zer0k: Command queue + interpolation related 
	float unknown3;
	float unknown4;

	bool m_bInSimulation;
	bool m_bEnableAssertions;

	// Simulation ticks - does not increase when game is paused
	int tickcount;
	// Simulation tick interval
	float interval_per_tick;
};

inline CGlobalVarsBase::CGlobalVarsBase()
{
}

#endif // GLOBALVARS_BASE_H
