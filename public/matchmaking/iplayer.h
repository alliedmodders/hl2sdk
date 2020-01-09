//========= Copyright © 1996-2009, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//


#ifndef _IPLAYER_H_
#define _IPLAYER_H_

#include "tier1/KeyValues.h"

struct UserProfileData
{
	float	reputation;
	int32	difficulty;
	int32	sensitivity;
	int32	yaxis;
	int32	vibration;
	int32	color1, color2;
	int32	action_autoaim;
	int32	action_autocenter;
	int32	action_movementcontrol;
	int32	region;
	int32	achearned;
	int32	cred;
	int32	zone;
	int32	titlesplayed;
	int32	titleachearned;
	int32	titlecred;
};

//Players are a wrapper or a networked player, as such they may not have all the information current, particularly when first created.
abstract_class IPlayer
{
public:
	enum OnlineState_t
	{
		STATE_OFFLINE,
		STATE_NO_MULTIPLAYER,
		STATE_ONLINE,
	};

public:
	//Info
	virtual XUID GetXUID() = 0;
	virtual int GetPlayerIndex() = 0;

	virtual char const * GetName() = 0;

	virtual OnlineState_t GetOnlineState() = 0;
};

abstract_class IPlayerFriend : public IPlayer
{
public:
	virtual wchar_t const * GetRichPresence() = 0;

	virtual KeyValues *GetGameDetails() = 0;

	virtual bool IsJoinable() = 0;
	virtual void Join() = 0;
};

abstract_class IPlayerLocal : public IPlayer
{
public:
	virtual const UserProfileData& GetPlayerProfileData() = 0;

	virtual const void * GetPlayerTitleData( int iTitleDataIndex ) = 0;
	virtual void UpdatePlayerTitleData( int iTitleDataIndex, const void *pvNewTitleData, int numBytesOffset, int numNewBytes ) = 0;

	virtual void GetLeaderboardData( KeyValues *pLeaderboardInfo ) = 0;
	virtual void UpdateLeaderboardData( KeyValues *pLeaderboardInfo ) = 0;
};

#endif
