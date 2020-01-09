//===== Copyright c 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IMATCHFRAMEWORK_H
#define IMATCHFRAMEWORK_H

#ifdef _WIN32
#pragma once
#endif

class IMatchFramework;
class IMatchSession;

#include "appframework/IAppSystem.h"

#include "tier1/interface.h"
#include "KeyValues.h"

#if !defined( _X360 )
#include "xbox/xboxstubs.h"
#endif

#include "inetchannel.h"

#include "imatchasync.h"
#include "imatchtitle.h"
#include "imatchnetworkmsg.h"
#include "imatchextensions.h"
#include "imatchevents.h"
#include "imatchsystem.h"
#include "iplayermanager.h"
#include "iplayer.h"
#include "iservermanager.h"
#include "imatchvoice.h"
#include "isearchmanager.h"
#include "idatacenter.h"

abstract_class IMatchFramework : public IAppSystem
{
public:
	// Run frame of the matchmaking framework
	virtual void RunFrame() = 0;


	// Get matchmaking extensions
	virtual IMatchExtensions * GetMatchExtensions() = 0;

	// Get events container
	virtual IMatchEventsSubscription * GetEventsSubscription() = 0;

	// Get the matchmaking title interface
	virtual IMatchTitle * GetMatchTitle() = 0;

	// Get the match session interface of the current match framework type
	virtual IMatchSession * GetMatchSession() = 0;

	// Get the network msg encode/decode factory
	virtual IMatchNetworkMsgController * GetMatchNetworkMsgController() = 0;

	// Get the match system
	virtual IMatchSystem * GetMatchSystem() = 0;

	// Entry point to create session
	virtual void CreateSession( KeyValues *pSettings ) = 0;

	// Entry point to match into a session
	virtual void MatchSession( KeyValues *pSettings ) = 0;

	// Accept invite
	virtual void AcceptInvite( int iController ) = 0;

	// Close the session
	virtual void CloseSession() = 0;
};

#define IMATCHFRAMEWORK_VERSION_STRING "MATCHFRAMEWORK_001"



abstract_class IMatchSession
{
public:
	// Get an internal pointer to session system-specific data
	virtual KeyValues * GetSessionSystemData() = 0;

	// Get an internal pointer to session settings
	virtual KeyValues * GetSessionSettings() = 0;

	// Update session settings, only changing keys and values need
	// to be passed and they will be updated
	virtual void UpdateSessionSettings( KeyValues *pSettings ) = 0;

	// Issue a session command
	virtual void Command( KeyValues *pCommand ) = 0;
};


#endif // IMATCHFRAMEWORK_H
