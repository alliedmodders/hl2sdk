//===== Copyright c 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IGAMETYPES_H
#define IGAMETYPES_H
#ifdef _WIN32
#pragma once
#endif

namespace ELOGameType
{
enum GameType
{
	INVALID = -1,
	CLASSIC_CASUAL,
	CLASSIC_COMPETITIVE,
	GUNGAME_PROGRESSIVE,
	GUNGAME_BOMB,
	COUNT
};
}
typedef ELOGameType::GameType ELOGameType_t;

namespace ELOCalcMode
{
enum CalcMode
{
	INVALID = -1,
	TEAM_VS_TEAM,
	FREE_FOR_ALL,
	COUNT
};
}
typedef ELOCalcMode::CalcMode ELOCalcMode_t;

#define VENGINE_GAMETYPES_VERSION "VENGINE_GAMETYPES_VERSION002"

abstract_class IGameTypes
{
public:
	class WeaponProgression
	{
	public:
		CUtlString m_Name;
		int m_Kills;
	};
	
public:
	virtual ~IGameTypes() {}
	
	virtual bool Initialize( bool force ) = 0;
	virtual bool IsInitialized() const = 0;
	
	virtual bool SetGameTypeAndMode( const char *gameType, const char *gameMode ) = 0;
	virtual void SetAndParseExtendedServerInfo( KeyValues *pExtendedServerInfo ) = 0;
	
	virtual int GetCurrentGameType() const = 0;
	virtual int GetCurrentGameMode() const = 0;
	
	virtual const char *GetCurrentGameTypeNameID() = 0;
	virtual const char *GetCurrentGameModeNameID() = 0;
	
	virtual bool ApplyConvarsForCurrentMode( bool isMultiplayer ) = 0;
	virtual void DisplayConvarsForCurrentMode() = 0;
	
	virtual const CUtlVector< WeaponProgression > *GetWeaponProgressionForCurrentModeCT() = 0;
	virtual const CUtlVector< WeaponProgression > *GetWeaponProgressionForCurrentModeT() = 0;
	
	virtual int GetNoResetVoteThresholdForCurrentModeCT() = 0;
	virtual int GetNoResetVoteThresholdForCurrentModeT() = 0;
	
	virtual const char *GetGameTypeFromInt( int gameType ) = 0;
	virtual const char *GetGameModeFromInt( int gameType, int gameMode ) = 0;
	
	virtual bool GetGameModeAndTypeIntsFromStrings( const char *szGameType, const char *szGameMode, int &iOutGameType, int &iOutGameMode ) = 0;
	virtual bool GetGameModeAndTypeNameIdsFromStrings( const char *szGameType, const char *szGameMode, const char * &szOutGameTypeNameId, const char * &szOutGameModeNameId ) = 0;
	
	virtual const char *GetRandomMapGroup( const char *gameType, const char *gameMode ) = 0;
	
	virtual const char *GetFirstMap( const char *mapGroup ) = 0;
	virtual const char *GetRandomMap( const char *mapGroup ) = 0;
	virtual const char *GetNextMap( const char *mapGroup, const char *mapName ) = 0;
	
	virtual int GetMaxPlayersForTypeAndMode( int iType, int iMode ) = 0;
	
	virtual bool IsValidMapGroupName( const char *mapGroup ) = 0;
	virtual bool IsValidMapInMapGroup( const char *mapGroup, const char *mapName ) = 0;
	virtual bool IsValidMapGroupForTypeAndMode( const char *mapGroup, const char *gameType, const char *gameMode ) = 0;
	
	virtual bool ApplyConvarsForMap( const char *mapName, bool isMultiplayer ) = 0;
	
	virtual bool GetMapInfo( const char *mapName, unsigned int &richPresence ) = 0;
	
	virtual const CUtlStringList *GetTModelsForMap( const char *mapName ) = 0;
	virtual const CUtlStringList *GetCTModelsForMap( const char *mapName ) = 0;
	virtual const CUtlStringList *GetHostageModelsForMap( const char *mapName ) = 0;
	
	virtual const char *GetTViewModelArmsForMap( const char *mapName ) = 0;
	virtual const char *GetCTViewModelArmsForMap( const char *mapName ) = 0;
	
	virtual const CUtlStringList *GetMapGroupMapList( const char *mapGroup ) = 0;
	
	virtual bool SetCustomBotDifficulty( int botDiff ) = 0;
	virtual int GetCustomBotDifficulty() = 0;
	
	virtual bool IsGameModeELORanked( const char *gameType, const char *gameMode, ELOGameType_t *ELOIndex ) = 0;
	virtual bool DoesCurrentGameModeELORankPlayers() = 0;
	virtual ELOGameType_t GetCurrentELOIndex() = 0;
	virtual int GetCurrentELOLockInTime() = 0;
	virtual ELOCalcMode_t GetCurrentELOCalculationMode() = 0;
	virtual float GetCurrentELOExpBase() = 0;
	virtual float GetCurrentELOExpDenom() = 0;
	virtual float GetCurrentELOExpScalar() = 0;
	virtual bool GetCurrentELOBracketDisplay() = 0;
	virtual int GetCurrentServerNumSlots() = 0;
};

#endif // IGAMETYPES_H
