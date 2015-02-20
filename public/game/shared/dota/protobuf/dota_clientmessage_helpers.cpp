
#include "dota_clientmessage_helpers.h"

CDotaClientMessageHelpers g_DotaClientMessageHelpers;

#define SETUP_DOTA_MESSAGE( msgname )                          \
	m_NameIndexMap.Insert( #msgname,  DOTA_CM_##msgname  ); \
	m_IndexNameMap[DOTA_CM_##msgname] = #msgname;           \
	m_Prototypes[DOTA_CM_##msgname] = &CDOTAClientMsg_##msgname::default_instance();

CDotaClientMessageHelpers::CDotaClientMessageHelpers()
{
	// Clear all so that any unused are inited.
	memset( m_Prototypes, 0, sizeof(m_Prototypes) );
	memset( m_IndexNameMap, 0, sizeof(m_IndexNameMap) );

	SETUP_DOTA_MESSAGE( MapLine );
	SETUP_DOTA_MESSAGE( AspectRatio );
	SETUP_DOTA_MESSAGE( MapPing );
	SETUP_DOTA_MESSAGE( UnitsAutoAttack );
	SETUP_DOTA_MESSAGE( AutoPurchaseItems );
	SETUP_DOTA_MESSAGE( TestItems );
	SETUP_DOTA_MESSAGE( SearchString );
	SETUP_DOTA_MESSAGE( Pause );
	SETUP_DOTA_MESSAGE( ShopViewMode );
	SETUP_DOTA_MESSAGE( SetUnitShareFlag );
	SETUP_DOTA_MESSAGE( SwapRequest );
	SETUP_DOTA_MESSAGE( SwapAccept );
	SETUP_DOTA_MESSAGE( WorldLine );
	SETUP_DOTA_MESSAGE( RequestGraphUpdate );
	SETUP_DOTA_MESSAGE( ItemAlert );
	SETUP_DOTA_MESSAGE( ChatWheel );
	SETUP_DOTA_MESSAGE( SendStatPopup );
	SETUP_DOTA_MESSAGE( BeginLastHitChallenge );
	SETUP_DOTA_MESSAGE( UpdateQuickBuy );
	SETUP_DOTA_MESSAGE( UpdateCoachListen );
	SETUP_DOTA_MESSAGE( CoachHUDPing );
	SETUP_DOTA_MESSAGE( RecordVote );
	SETUP_DOTA_MESSAGE( UnitsAutoAttackAfterSpell );
	SETUP_DOTA_MESSAGE( WillPurchaseAlert );
	SETUP_DOTA_MESSAGE( PlayerShowCase );
	SETUP_DOTA_MESSAGE( TeleportRequiresHalt );
	SETUP_DOTA_MESSAGE( CameraZoomAmount );
	SETUP_DOTA_MESSAGE( BroadcasterUsingCamerman );
	SETUP_DOTA_MESSAGE( BroadcastUsingAssistedCameraOperator );
	SETUP_DOTA_MESSAGE( EnemyItemAlert );
	SETUP_DOTA_MESSAGE( FreeInventory );
	SETUP_DOTA_MESSAGE( BuyBackStateAlert );
	SETUP_DOTA_MESSAGE( QuickBuyAlert );
	SETUP_DOTA_MESSAGE( HeroStatueLike );
	SETUP_DOTA_MESSAGE( ModifierAlert );
	SETUP_DOTA_MESSAGE( TeamShowcaseEditor );
	SETUP_DOTA_MESSAGE( HPManaAlert );
	SETUP_DOTA_MESSAGE( GlyphAlert );
	SETUP_DOTA_MESSAGE( TeamShowcaseClientData );
	SETUP_DOTA_MESSAGE( PlayTeamShowcase );
	SETUP_DOTA_MESSAGE( EventCNY2015Cmd );
}

const google::protobuf::Message *CDotaClientMessageHelpers::GetPrototype( int index ) const
{
	if( index >= 0 && index < EDotaClientMessages_ARRAYSIZE )
		return m_Prototypes[index];

	return NULL;
}

const google::protobuf::Message *CDotaClientMessageHelpers::GetPrototype( const char *name ) const
{
	int index = GetIndex( name );
	if( index > -1 )
		return m_Prototypes[ index ];

	return NULL;
}

int CDotaClientMessageHelpers::GetIndex( const char *name ) const
{
	unsigned int idx = m_NameIndexMap.Find( name );
	if( idx != m_NameIndexMap.InvalidHandle() )
		return m_NameIndexMap[idx];

	return -1;
}

const char *CDotaClientMessageHelpers::GetName( int index ) const
{
	if( index >= 0 && index < EDotaClientMessages_ARRAYSIZE )
		return m_IndexNameMap[index];

	return NULL;
}

