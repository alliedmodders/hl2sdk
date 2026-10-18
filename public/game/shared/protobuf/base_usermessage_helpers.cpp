
#include "base_usermessage_helpers.h"

CUsermessageHelpers g_BaseUsermessageHelpers;

// Some enum entries have no message schema in this SDK. Keep their names and
// indices available while leaving their prototypes NULL.
#define SETUP_MESSAGE_NAME( msgname ) \
	m_NameIndexMap.Insert( #msgname,  UM_##msgname  ); \
	m_IndexNameMap[UM_##msgname] = #msgname;

#define SETUP_MESSAGE( msgname ) \
	SETUP_MESSAGE_NAME( msgname ); \
	m_Prototypes[UM_##msgname] = &CUsrMsg_##msgname::default_instance();

CUsermessageHelpers::CUsermessageHelpers()
{
	// Clear all so that any unused are inited.
	memset( m_Prototypes, 0, sizeof(m_Prototypes) );
	memset( m_IndexNameMap, 0, sizeof(m_IndexNameMap) );

	SETUP_MESSAGE( VGUIMenu );
	SETUP_MESSAGE( Geiger );
	SETUP_MESSAGE( Train );
	SETUP_MESSAGE( HudText );
	SETUP_MESSAGE( SayText );
	SETUP_MESSAGE( SayText2 );
	SETUP_MESSAGE( TextMsg );
	SETUP_MESSAGE( HudMsg );
	SETUP_MESSAGE( ResetHud );
	SETUP_MESSAGE( Shake );
	SETUP_MESSAGE( Fade );
	SETUP_MESSAGE( Rumble );
	SETUP_MESSAGE( CloseCaption );
	SETUP_MESSAGE( CloseCaptionDirect );
	SETUP_MESSAGE( SendAudio );
	SETUP_MESSAGE( RawAudio );
	SETUP_MESSAGE( VoiceMask );
	SETUP_MESSAGE( RequestState );
	SETUP_MESSAGE( Damage );
	SETUP_MESSAGE( RadioText );
	SETUP_MESSAGE( HintText );
	SETUP_MESSAGE( KeyHintText );
	SETUP_MESSAGE_NAME( UpdateTeamMoney );
	SETUP_MESSAGE( StopSpectatorMode );
	SETUP_MESSAGE( DesiredTimescale );
	SETUP_MESSAGE( CurrentTimescale );
	SETUP_MESSAGE( AchievementEvent );
	SETUP_MESSAGE( PlayerStatsUpdate );
	SETUP_MESSAGE( CallVoteFailed );
	SETUP_MESSAGE( VoteStart );
	SETUP_MESSAGE( VotePass );
	SETUP_MESSAGE( VoteFailed );
	SETUP_MESSAGE( VoteSetup );
	SETUP_MESSAGE( SendLastKillerDamageToClient );
	SETUP_MESSAGE( ItemPickup );
	SETUP_MESSAGE( ShowMenu );
	SETUP_MESSAGE( BarTime );
	SETUP_MESSAGE( AmmoDenied );
	SETUP_MESSAGE( MarkAchievement );
	SETUP_MESSAGE( GlowPropTurnOff );
	SETUP_MESSAGE_NAME( SendPlayerItemDrops );
	SETUP_MESSAGE_NAME( SendPlayerItemFound );
	SETUP_MESSAGE_NAME( XpUpdate );
	SETUP_MESSAGE_NAME( ScoreLeaderboardData );
	SETUP_MESSAGE_NAME( PlayerDecalDigitalSignature );
	SETUP_MESSAGE( WeaponSound );
	SETUP_MESSAGE( UpdateScreenHealthBar );
	SETUP_MESSAGE( EntityOutlineHighlight );
	SETUP_MESSAGE_NAME( SSUI );
	SETUP_MESSAGE_NAME( SurvivalStats );
	SETUP_MESSAGE_NAME( DisconnectToLobby2 );
	SETUP_MESSAGE( Tilt );
	SETUP_MESSAGE( Battery );
	SETUP_MESSAGE( CreditsMsg );
	SETUP_MESSAGE( LogoTimeMsg );
	SETUP_MESSAGE( UpdateJalopyRadar );
	SETUP_MESSAGE( MessageText );
	SETUP_MESSAGE( StatsSkipState );
	SETUP_MESSAGE( PanoramaEvent );
}

const google::protobuf::Message *CUsermessageHelpers::GetPrototype( int index ) const
{
	if( index >= 0 && index < EUserMessages_ARRAYSIZE )
		return m_Prototypes[index];

	return NULL;
}

const google::protobuf::Message *CUsermessageHelpers::GetPrototype( const char *name ) const
{
	int index = GetIndex( name );
	if( index > -1 )
		return m_Prototypes[ index ];

	return NULL;
}

int CUsermessageHelpers::GetIndex( const char *name ) const
{
	unsigned int idx = m_NameIndexMap.Find( name );
	if( idx != m_NameIndexMap.InvalidHandle() )
		return m_NameIndexMap[idx];

	return -1;
}

const char *CUsermessageHelpers::GetName( int index ) const
{
	if( index >= 0 && index < EUserMessages_ARRAYSIZE )
		return m_IndexNameMap[index];

	return NULL;
}

