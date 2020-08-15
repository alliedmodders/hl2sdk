//===== Copyright c 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IMATCHTITLE_H
#define IMATCHTITLE_H

#ifdef _WIN32
#pragma once
#endif

struct TitleDataFieldsDescription_t
{
	enum DataType_t
	{
		DT_U8		= 8,
		DT_U16		= 16,
		DT_U32		= 32,
		DT_FLOAT	= 33,
		DT_U64		= 64
	};

	enum DataBlock_t
	{
		DB_TD1		= 0,
		DB_TD2		= 1,
		DB_TD3		= 2
	};

	char const *m_szFieldName;
	DataBlock_t m_iTitleDataBlock;
	DataType_t m_eDataType;
	int m_numBytesOffset;
};

struct TitleAchievementsDescription_t
{
	char const *m_szAchievementName;
	int m_idAchievement;
};

struct TitleAvatarAwardsDescription_t
{
	char const *m_szAvatarAwardName;
	int m_idAvatarAward;
};

abstract_class IMatchTitle
{
public:
	// Title ID
	virtual uint64 GetTitleID() = 0;

	// Service ID for XLSP
	virtual uint64 GetTitleServiceID() = 0;

	// Whether we are a single-player title or multi-player title
	virtual bool IsMultiplayer() = 0;

	// Prepare network startup params for the title
	virtual void PrepareNetStartupParams( void *pNetStartupParams ) = 0;

	// Get total number of players supported by the title
	virtual int GetTotalNumPlayersSupported() = 0;

	// Get a guest player name
	virtual char const * GetGuestPlayerName( int iUserIndex ) = 0;

	// Decipher title data fields
	virtual TitleDataFieldsDescription_t const * DescribeTitleDataStorage() = 0;

	// Title achievements
	virtual TitleAchievementsDescription_t const * DescribeTitleAchievements() = 0;

	// Title avatar awards
	virtual TitleAvatarAwardsDescription_t const * DescribeTitleAvatarAwards() = 0;

	// Title leaderboards
	virtual KeyValues * DescribeTitleLeaderboard( char const *szLeaderboardView ) = 0;

	// Sets up all necessary client-side convars and user info before
	// connecting to server
	virtual void PrepareClientForConnect( KeyValues *pSettings ) = 0;

	// Start up a listen server with the given settings
	virtual bool StartServerMap( KeyValues *pSettings ) = 0;
};

//
// Matchmaking title settings extension interface
//

abstract_class IMatchTitleGameSettingsMgr
{
public:
	// Extends server game details
	virtual void ExtendServerDetails( KeyValues *pDetails, KeyValues *pRequest ) = 0;
	
	// Adds the essential part of game details to be broadcast
	virtual void ExtendLobbyDetailsTemplate( KeyValues *pDetails, char const *szReason, KeyValues *pFullSettings ) = 0;

	// Extends game settings update packet for lobby transition,
	// either due to a migration or due to an endgame condition
	virtual void ExtendGameSettingsForLobbyTransition( KeyValues *pSettings, KeyValues *pSettingsUpdate, bool bEndGame ) = 0;


	// Rolls up game details for matches grouping
	//	valid pDetails, null pRollup
	//		returns a rollup representation of pDetails to be used as an indexing key
	//	valid pDetails, valid pRollup (usually called second time)
	//		rolls the details into the rollup, aggregates some values, when
	//		the aggregate values are missing in pRollup, then this is the first
	//		details entry being aggregated and would establish the first rollup
	//		returns pRollup
	//	null pDetails, valid pRollup
	//		tries to determine if the rollup should remain even though no details
	//		matched it, adjusts pRollup to represent no aggregated data
	//		returns null to drop pRollup, returns pRollup to keep rollup
	virtual KeyValues * RollupGameDetails( KeyValues *pDetails, KeyValues *pRollup, KeyValues *pQuery ) = 0;


	// Defines session search keys for matchmaking
	virtual KeyValues * DefineSessionSearchKeys( KeyValues *pSettings ) = 0;

	// Defines dedicated server search key
	virtual KeyValues * DefineDedicatedSearchKeys( KeyValues *pSettings ) = 0;


	// Initializes full game settings from potentially abbreviated game settings
	virtual void InitializeGameSettings( KeyValues *pSettings ) = 0;

	// Extends game settings update packet before it gets merged with
	// session settings and networked to remote clients
	virtual void ExtendGameSettingsUpdateKeys( KeyValues *pSettings, KeyValues *pUpdateDeleteKeys ) = 0;

	// Prepares system for session creation
	virtual KeyValues * PrepareForSessionCreate( KeyValues *pSettings ) = 0;


	// Executes the command on the session settings, this function on host
	// is allowed to modify Members/Game subkeys and has to fill in modified players KeyValues
	// When running on a remote client "ppPlayersUpdated" is NULL and players cannot
	// be modified
	virtual void ExecuteCommand( KeyValues *pCommand, KeyValues *pSessionSystemData, KeyValues *pSettings, KeyValues **ppPlayersUpdated ) = 0;

	// Prepares the host lobby for game or adjust settings of new players who
	// join a game in progress, this function is allowed to modify
	// Members/Game subkeys and has to fill in modified players KeyValues
	virtual void PrepareLobbyForGame( KeyValues *pSettings, KeyValues **ppPlayersUpdated ) = 0;

	// Prepares the host team lobby for game adjusting the game settings
	// this function is allowed to prepare modification package to update
	// Game subkeys.
	// Returns the update/delete package to be applied to session settings
	// and pushed to dependent two sesssion of the two teams.
	virtual KeyValues * PrepareTeamLinkForGame( KeyValues *pSettingsLocal, KeyValues *pSettingsRemote ) = 0;
};

#endif // IMATCHTITLE_H
