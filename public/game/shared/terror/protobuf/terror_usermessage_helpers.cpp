
#include "terror_usermessage_helpers.h"

CTerrorUsermessageHelpers g_TerrorUsermessageHelpers;

#define SETUP_MESSAGE( msgname )                          \
	m_NameIndexMap.Insert( #msgname,  UM_Terror_##msgname  ); \
	m_IndexNameMap[UM_Terror_##msgname] = #msgname;           \
	m_Prototypes[UM_Terror_##msgname] = &CUsrMsg_Terror_##msgname::default_instance();

CTerrorUsermessageHelpers::CTerrorUsermessageHelpers()
{
	// Clear all so that any unused are inited.
	memset( m_Prototypes, 0, sizeof(m_Prototypes) );
	memset( m_IndexNameMap, 0, sizeof(m_IndexNameMap) );

	SETUP_MESSAGE( Splatter );
	SETUP_MESSAGE( MeleeSlashSplatter );
	SETUP_MESSAGE( MeleeClubSplatter );
	SETUP_MESSAGE( MudSplatter );
	SETUP_MESSAGE( SplatterClear );
	SETUP_MESSAGE( MessageText );
	SETUP_MESSAGE( TransitionRestore );
	SETUP_MESSAGE( Spawn );
	SETUP_MESSAGE( CreditsLine );
	SETUP_MESSAGE( CreditsMsg );
	SETUP_MESSAGE( JoinLateMsg );
	SETUP_MESSAGE( StatsCrawlMsg );
	SETUP_MESSAGE( StatsSkipState );
	SETUP_MESSAGE( ShowStats );
	SETUP_MESSAGE( BlurFade );
	SETUP_MESSAGE( MusicCmd );
	SETUP_MESSAGE( WitchBloodSplatter );
	SETUP_MESSAGE( AchievementEvent );
	SETUP_MESSAGE( PZDmgMsg );
	SETUP_MESSAGE( AllPlayersConnectedGameStarting );
	SETUP_MESSAGE( VoteRegistered );
	SETUP_MESSAGE( DisconnectToLobby );
	SETUP_MESSAGE( CallVoteFailed );
	SETUP_MESSAGE( SteamWeaponStatData );
	SETUP_MESSAGE( CurrentTimescale );
	SETUP_MESSAGE( DesiredTimescale );
	SETUP_MESSAGE( PZEndGamePanelMsg );
	SETUP_MESSAGE( PZEndGamePanelVoteRegisteredMsg );
	SETUP_MESSAGE( PZEndGameVoteStatsMsg );
	SETUP_MESSAGE( VoteStart );
	SETUP_MESSAGE( VotePass );
	SETUP_MESSAGE( VoteFail );
	SETUP_MESSAGE( SavedConvar );
	SETUP_MESSAGE( KillCam );
	SETUP_MESSAGE( FirearmSlashSplatter );
	SETUP_MESSAGE( FirearmClubSplatter );
}

const google::protobuf::Message *CTerrorUsermessageHelpers::GetPrototype( int index ) const
{
	if( index >= 0 && index < ETerrorUserMessages_ARRAYSIZE )
		return m_Prototypes[index];

	return NULL;
}

const google::protobuf::Message *CTerrorUsermessageHelpers::GetPrototype( const char *name ) const
{
	int index = GetIndex( name );
	if( index > -1 )
		return m_Prototypes[ index ];

	return NULL;
}

int CTerrorUsermessageHelpers::GetIndex( const char *name ) const
{
	unsigned int idx = m_NameIndexMap.Find( name );
	if( idx != m_NameIndexMap.InvalidHandle() )
		return m_NameIndexMap[idx];

	return -1;
}

const char *CTerrorUsermessageHelpers::GetName( int index ) const
{
	if( index >= 0 && index < ETerrorUserMessages_ARRAYSIZE )
		return m_IndexNameMap[index];

	return NULL;
}

