//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef IHLTV_H
#define IHLTV_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IServer;
class IHLTVDirector;
class IGameEvent;
struct netadr_t;
class CGameInfo;

//-----------------------------------------------------------------------------
// Interface the HLTV module exposes to the engine
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_HLTVSERVER	"HLTVServer001"

class IHLTVServer : public IBaseInterface
{
public:
	virtual	~IHLTVServer() {}

	virtual	IServer	*GetBaseServer( void ) = 0; // get HLTV base server interface
	virtual	IHLTVDirector *GetDirector( void ) = 0;	// get director interface
	virtual	int		GetHLTVSlot( void ) = 0; // return entity index-1 of HLTV in game
	virtual float	GetOnlineTime( void ) = 0; // seconds since broadcast started
	virtual void	GetLocalStats( int &proxies, int &slots, int &specs ) = 0; 
	virtual void	GetGlobalStats( int &proxies, int &slots, int &specs ) = 0; 

	virtual const netadr_t *GetRelayAddress( void ) = 0; // returns relay address

	virtual bool	IsMasterProxy( void ) = 0; // true, if this is the HLTV master proxy
	virtual bool	IsDemoPlayback( void ) = 0; // true if this is a HLTV demo

	virtual void	BroadcastEvent(IGameEvent *event) = 0; // send a director command to all specs
	
	virtual bool	IsRecording() = 0;
	virtual const char	*GetRecordingDemoFilename ( void ) = 0;
	virtual void	StartAutoRecording( void ) = 0;
	virtual void	StopAutoRecording( const CGameInfo *pInfo ) = 0;	
};

#endif
