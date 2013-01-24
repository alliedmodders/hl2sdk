
#include "cstrike15_usermessage_helpers.h"

CCstrike15UsermessageHelpers g_Cstrike15UsermessageHelpers;

#define SETUP_MESSAGE( msgname )                          \
	m_NameIndexMap.Insert( #msgname,  CS_UM_##msgname  ); \
	m_IndexNameMap[CS_UM_##msgname] = #msgname;           \
	m_Prototypes[CS_UM_##msgname] = &CCSUsrMsg_##msgname::default_instance();

CCstrike15UsermessageHelpers::CCstrike15UsermessageHelpers()
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
	SETUP_MESSAGE( GameTitle );
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
	SETUP_MESSAGE( ProcessSpottedEntityUpdate );
	SETUP_MESSAGE( ReloadEffect );
	SETUP_MESSAGE( AdjustMoney );
	//SETUP_MESSAGE( UpdateTeamMoney );
	SETUP_MESSAGE( StopSpectatorMode );
	SETUP_MESSAGE( KillCam );
	SETUP_MESSAGE( DesiredTimescale );
	SETUP_MESSAGE( CurrentTimescale );
	SETUP_MESSAGE( AchievementEvent );
	SETUP_MESSAGE( MatchEndConditions );
	SETUP_MESSAGE( DisconnectToLobby );
	SETUP_MESSAGE( PlayerStatsUpdate );
	SETUP_MESSAGE( DisplayInventory );
	SETUP_MESSAGE( WarmupHasEnded );
	SETUP_MESSAGE( ClientInfo );
	SETUP_MESSAGE( XRankGet );
	SETUP_MESSAGE( XRankUpd );
	SETUP_MESSAGE( SetPlayerEloDisplayBracket );
	SETUP_MESSAGE( RequestEloBracketInfo );
	SETUP_MESSAGE( SetEloBracketInfo );
	SETUP_MESSAGE( CallVoteFailed );
	SETUP_MESSAGE( VoteStart );
	SETUP_MESSAGE( VotePass );
	SETUP_MESSAGE( VoteFailed );
	SETUP_MESSAGE( VoteSetup );
	SETUP_MESSAGE( ServerRankRevealAll );
	SETUP_MESSAGE( SendLastKillerDamageToClient );
	SETUP_MESSAGE( ServerRankUpdate );
	SETUP_MESSAGE( ItemPickup );
	SETUP_MESSAGE( ShowMenu );
	SETUP_MESSAGE( BarTime );
	SETUP_MESSAGE( AmmoDenied );
	SETUP_MESSAGE( MarkAchievement );
	SETUP_MESSAGE( MatchStatsUpdate );
	SETUP_MESSAGE( ItemDrop );
}

const google::protobuf::Message *CCstrike15UsermessageHelpers::GetPrototype( int index ) const
{
	if( index >= 0 && index < ECstrike15UserMessages_ARRAYSIZE )
		return m_Prototypes[index];

	return NULL;
}

const google::protobuf::Message *CCstrike15UsermessageHelpers::GetPrototype( const char *name ) const
{
	int index = GetIndex( name );
	if( index > -1 )
		return m_Prototypes[ index ];

	return NULL;
}

int CCstrike15UsermessageHelpers::GetIndex( const char *name ) const
{
	int idx = m_NameIndexMap.Find( name );
	if( idx != m_NameIndexMap.InvalidHandle() )
		return m_NameIndexMap[idx];

	return -1;
}

const char *CCstrike15UsermessageHelpers::GetName( int index ) const
{
	if( index >= 0 && index < ECstrike15UserMessages_ARRAYSIZE )
		return m_IndexNameMap[index];

	return NULL;
}

