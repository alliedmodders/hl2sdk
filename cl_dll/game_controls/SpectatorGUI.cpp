//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include "spectatorgui.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>

#include <stdio.h> // _snprintf define

#include <cl_dll/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

// void DuckMessage(const char *str); // from vgui_teamfortressviewport.cpp

ConVar spec_scoreboard( "spec_scoreboard", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

CSpectatorGUI *g_pSpectatorGUI = NULL;

static char *s_SpectatorModes[] = { "#Spec_Mode0", "#Spec_Mode1", "#Spec_Mode2", "#Spec_Mode3", "#Spec_Mode4", "#Spec_Mode5", "" };

using namespace vgui;

ConVar cl_spec_mode(
	"cl_spec_mode",
	"1",
	FCVAR_ARCHIVE | FCVAR_USERINFO,
	"spectator mode" );



//-----------------------------------------------------------------------------
// Purpose: left and right buttons pointing buttons
//-----------------------------------------------------------------------------
class CSpecButton : public Button
{
public:
	CSpecButton(Panel *parent, const char *panelName): Button(parent, panelName, "") {}

private:
	void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		Button::ApplySchemeSettings(pScheme);
		SetFont(pScheme->GetFont("Marlett", IsProportional()) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorMenu::CSpectatorMenu( IViewPort *pViewPort ) : Frame( NULL, PANEL_SPECMENU )
{
	m_iDuckKey = -1;
		
	m_pViewPort = pViewPort;

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
	SetTitleBarVisible( false ); // don't draw a title bar
	SetMoveable( false );
	SetSizeable( false );
	SetProportional(true);

	SetScheme("ClientScheme");

	m_pPlayerList = new ComboBox(this, "playercombo", 10 , false);
	m_pViewOptions = new ComboBox(this, "viewcombo", 10 , false );
	m_pConfigSettings = new ComboBox(this, "settingscombo", 10 , false );	

	m_pLeftButton = new CSpecButton( this, "specprev");
	m_pLeftButton->SetText("3");
	m_pRightButton = new CSpecButton( this, "specnext");
	m_pRightButton->SetText("4");

	m_pPlayerList->SetText("");
	m_pViewOptions->SetText("#Spec_Modes");
	m_pConfigSettings->SetText("#Spec_Options");

	m_pPlayerList->SetOpenDirection( Menu::UP );
	m_pViewOptions->SetOpenDirection( Menu::UP );
	m_pConfigSettings->SetOpenDirection( Menu::UP );

	// create view config menu
	CommandMenu * menu = new CommandMenu(m_pViewOptions, "spectatormenu", gViewPortInterface);
	menu->LoadFromFile( "Resource/spectatormenu.res" );
	m_pConfigSettings->SetMenu( menu );	// attach menu to combo box

	// create view mode menu
	menu = new CommandMenu(m_pViewOptions, "spectatormodes", gViewPortInterface);
	menu->LoadFromFile("Resource/spectatormodes.res");
	m_pViewOptions->SetMenu( menu );	// attach menu to combo box

	LoadControlSettings("Resource/UI/BottomSpectator.res");
}

void CSpectatorMenu::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// need to MakeReadyForUse() on the menus so we can set their bg color before they are displayed
	m_pConfigSettings->GetMenu()->MakeReadyForUse();
	m_pViewOptions->GetMenu()->MakeReadyForUse();
	m_pPlayerList->GetMenu()->MakeReadyForUse();

	m_pConfigSettings->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pViewOptions->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
	m_pPlayerList->GetMenu()->SetBgColor( BLACK_BAR_COLOR );
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorMenu::PerformLayout()
{
	int w,h;
	GetHudSize(w, h);

	// fill the screen
	SetSize(w,GetTall());
}


//-----------------------------------------------------------------------------
// Purpose: Handles changes to combo boxes
//-----------------------------------------------------------------------------
void CSpectatorMenu::OnTextChanged(KeyValues *data)
{
	Panel *panel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::ComboBox *box = dynamic_cast<vgui::ComboBox *>( panel );

	if( box == m_pConfigSettings) // don't change the text in the config setting combo
	{
		m_pConfigSettings->SetText("#Spec_Options");
	}
	else if ( box == m_pPlayerList )
	{
		KeyValues *kv = box->GetActiveItemUserData();
		if ( kv && GameResources() )
		{
			const char *player = kv->GetString("player");

			int currentPlayerNum = GetSpectatorTarget();
			const char *currentPlayerName = GameResources()->GetPlayerName( currentPlayerNum );

			if ( !FStrEq( currentPlayerName, player ) )
			{
				char command[128];
				Q_snprintf( command, sizeof(command), "spec_player \"%s\"", player );
				engine->ClientCmd( command );
			}
		}
	}
}

void CSpectatorMenu::OnCommand( const char *command )
{
	if (!stricmp(command, "specnext") )
	{
		engine->ClientCmd("spec_next");
	}
	else if (!stricmp(command, "specprev") )
	{
		engine->ClientCmd("spec_prev");
	}
}


//-----------------------------------------------------------------------------
// Purpose: when duck is pressed it hides the active part of the GUI
//-----------------------------------------------------------------------------
void CSpectatorMenu::OnKeyCodePressed(KeyCode code)
{
	// we can't compare the keycode to a known code, because translation from bound keys
	// to vgui key codes is not 1:1. Get the engine version of the key for the binding
	// and the actual pressed key, and compare those..
	int iLastTrappedKey = engine->GetLastPressedEngineKey();	// the enginekey version of the code param

	if( iLastTrappedKey == m_iDuckKey )
	{
		// hide if DUCK is pressed again
		m_pViewPort->ShowPanel( this, false );
	}
}

void CSpectatorMenu::ShowPanel(bool bShow)
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}

	bool bIsEnabled = true;
	
	 if ( engine->IsHLTV() && HLTVCamera()->IsPVSLocked() )
	 {
		 // when wattching HLTV with a locked PVS, some elements are disabled
		 bIsEnabled = false;
	 }
	
	m_pLeftButton->SetVisible( bIsEnabled );
	m_pRightButton->SetVisible( bIsEnabled );
	m_pPlayerList->SetVisible( bIsEnabled );
	m_pViewOptions->SetVisible( bIsEnabled );
}


int CSpectatorMenu::PlayerAddItem( int itemID, wchar_t *name, KeyValues *data ) 
{
	if ( m_pPlayerList->IsItemIDValid( itemID ) )
	{	
		m_pPlayerList->UpdateItem( itemID, name, data );
		return itemID + 1;
	}
	else
	{
		return m_pPlayerList->AddItem( name, data ) + 1; 
	}
}

void CSpectatorMenu::SetPlayerNameText(const wchar_t *text )
{
	char *ansiText = (char *) _alloca( (wcslen( text ) + 1) * sizeof( char ) );
	if ( ansiText )
	{
		localize()->ConvertUnicodeToANSI( text, ansiText, (wcslen( text ) + 1) * sizeof( char ) );
		m_pPlayerList->SetText( ansiText );
	}
}

//-----------------------------------------------------------------------------
// main spectator panel



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSpectatorGUI::CSpectatorGUI(IViewPort *pViewPort) : Frame( NULL, PANEL_SPECGUI )
{
// 	m_bHelpShown = false;
//	m_bInsetVisible = false;
//	m_iDuckKey = KEY_NONE;
	m_bSpecScoreboard = false;

	m_pViewPort = pViewPort;
	g_pSpectatorGUI = this;

	// initialize dialog
	SetVisible(false);
	SetTitle("SpectatorGUI", true);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTopBar = new Panel( this, "topbar" );
 	m_pBottomBarBlank = new Panel( this, "bottombarblank" );

	// m_pBannerImage = new ImagePanel( m_pTopBar, NULL );
	m_pPlayerLabel = new Label( this, "playerlabel", "" );
	m_pPlayerLabel->SetVisible( false );

	LoadControlSettings("Resource/UI/Spectator.res");
	
	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	m_pBottomBarBlank->SetVisible( true );
	m_pTopBar->SetVisible( true );

	// m_pBannerImage->SetVisible(false);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSpectatorGUI::~CSpectatorGUI()
{
	g_pSpectatorGUI = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CSpectatorGUI::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pTopBar->SetBgColor(BLACK_BAR_COLOR);
	m_pBottomBarBlank->SetBgColor(BLACK_BAR_COLOR);
	// m_pBottomBar->SetBgColor(Color( 0,0,0,0 ));
	SetPaintBorderEnabled(false);

	SetBorder( NULL );

}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CSpectatorGUI::PerformLayout()
{
	int w,h,x,y;
	GetHudSize(w, h);
	
	// fill the screen
	SetBounds(0,0,w,h);

	// stretch the bottom bar across the screen
	m_pBottomBarBlank->GetPos(x,y);
	m_pBottomBarBlank->SetSize( w, h - y );
}

//-----------------------------------------------------------------------------
// Purpose: checks spec_scoreboard cvar to see if the scoreboard should be displayed
//-----------------------------------------------------------------------------
void CSpectatorGUI::OnThink()
{
	BaseClass::OnThink();

	if ( IsVisible() )
	{
		if ( m_bSpecScoreboard != spec_scoreboard.GetBool() )
		{
			if ( !spec_scoreboard.GetBool() || !gViewPortInterface->GetActivePanel() )
			{
				m_bSpecScoreboard = spec_scoreboard.GetBool();
				gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, m_bSpecScoreboard );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the image to display for the banner in the top right corner
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLogoImage(const char *image)
{
	if ( m_pBannerImage )
	{
		m_pBannerImage->SetImage( scheme()->GetImage(image, false) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::SetLabelText(const char *textEntryName, wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CSpectatorGUI::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CSpectatorGUI::ShowPanel(bool bShow)
{
	if ( bShow && !IsVisible() )
	{
		m_bSpecScoreboard = false;
	}
	SetVisible( bShow );
	if ( !bShow && m_bSpecScoreboard )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
	}
}

bool CSpectatorGUI::ShouldShowPlayerLabel( int specmode )
{
	return ( (specmode == OBS_MODE_IN_EYE) ||	(specmode == OBS_MODE_CHASE) );
}
//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearranges elements
//-----------------------------------------------------------------------------
void CSpectatorGUI::Update()
{
	int wide, tall;
	int bx, by, bwide, btall;

	GetHudSize(wide, tall);
	m_pTopBar->GetBounds( bx, by, bwide, btall );

	IGameResources *gr = GameResources();
	int specmode = GetSpectatorMode();
	int playernum = GetSpectatorTarget();

	IViewPortPanel *overview = gViewPortInterface->FindPanelByName( PANEL_OVERVIEW );

	if ( overview && overview->IsVisible() )
	{
		int mx, my, mwide, mtall;

		VPANEL p = overview->GetVPanel();
		vgui::ipanel()->GetPos( p, mx, my );
		vgui::ipanel()->GetSize( p, mwide, mtall );
				
		if ( my < btall )
		{
			// reduce to bar 
			m_pTopBar->SetSize( wide - (mx + mwide), btall );
			m_pTopBar->SetPos( (mx + mwide), 0 );
		}
		else
		{
			// full top bar
			m_pTopBar->SetSize( wide , btall );
			m_pTopBar->SetPos( 0, 0 );
		}
	}
	else
	{
		// full top bar
		m_pTopBar->SetSize( wide , btall ); // change width, keep height
		m_pTopBar->SetPos( 0, 0 );
	}

	m_pPlayerLabel->SetVisible( ShouldShowPlayerLabel(specmode) );

	// update player name filed, text & color

	if ( playernum > 0 && playernum <= gpGlobals->maxClients && gr )
	{
		Color c = gr->GetTeamColor( gr->GetTeam(playernum) ); // Player's team color

		m_pPlayerLabel->SetFgColor( c );
		
		wchar_t playerText[ 80 ], playerName[ 64 ], health[ 10 ];
		wcscpy( playerText, L"Unable to find #Spec_PlayerItem*" );
		memset( playerName, 0x0, sizeof( playerName ) * sizeof( wchar_t ) );

		localize()->ConvertANSIToUnicode( UTIL_SafeName(gr->GetPlayerName( playernum )), playerName, sizeof( playerName ) );
		int iHealth = gr->GetHealth( playernum );
		if ( iHealth > 0  && gr->IsAlive(playernum) )
		{
			_snwprintf( health, sizeof( health ), L"%i", iHealth );
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  health );
		}
		else
		{
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
		}

		m_pPlayerLabel->SetText( playerText );
	}
	else
	{
		m_pPlayerLabel->SetText( L"" );
	}

	// update extra info field
	wchar_t szEtxraInfo[1024];
	wchar_t szTitleLabel[1024];
	char tempstr[128];

	if ( engine->IsHLTV() )
	{
		// set spectator number and HLTV title
		Q_snprintf(tempstr,sizeof(tempstr),"Spectators : %d", HLTVCamera()->GetNumSpectators() );
		localize()->ConvertANSIToUnicode(tempstr,szEtxraInfo,sizeof(szEtxraInfo));
		
		Q_strncpy( tempstr, HLTVCamera()->GetTitleText(), sizeof(tempstr) );
		localize()->ConvertANSIToUnicode(tempstr,szTitleLabel,sizeof(szTitleLabel));
	}
	else
	{
		// otherwise show map name
		Q_FileBase( engine->GetLevelName(), tempstr, sizeof(tempstr) );

		wchar_t wMapName[64];
		localize()->ConvertANSIToUnicode(tempstr,wMapName,sizeof(wMapName));
		localize()->ConstructString( szEtxraInfo,sizeof( szEtxraInfo ), localize()->Find("#Spec_Map" ),1, wMapName );

		localize()->ConvertANSIToUnicode( "" ,szTitleLabel,sizeof(szTitleLabel));
	}

	SetLabelText("extrainfo", szEtxraInfo );
	SetLabelText("titlelabel", szTitleLabel );
}


//-----------------------------------------------------------------------------
// Purpose: Resets the list of players
//-----------------------------------------------------------------------------
void CSpectatorMenu::Update( void )
{
	int itemID = 0;

	IGameResources *gr = GameResources();

	Reset();

	if( m_iDuckKey < 0 )
	{
		m_iDuckKey = gameuifuncs->GetEngineKeyCodeForBind( "duck" );
	}
	
	if ( !gr )
		return;

	int iPlayerIndex;
	for ( iPlayerIndex = 1 ; iPlayerIndex <= gpGlobals->maxClients; iPlayerIndex++ )
	{

		// does this slot in the array have a name?
		if ( !gr->IsConnected( iPlayerIndex ) )
			continue;
			
		if ( gr->IsLocalPlayer( iPlayerIndex ) )
			continue;

		if ( !gr->IsAlive( iPlayerIndex ) )
			continue;

		wchar_t playerText[ 80 ], playerName[ 64 ], *team, teamText[ 64 ];
		char localizeTeamName[64];
		localize()->ConvertANSIToUnicode( UTIL_SafeName( gr->GetPlayerName(iPlayerIndex) ), playerName, sizeof( playerName ) );
		const char * teamname = gr->GetTeamName( gr->GetTeam(iPlayerIndex) );
		if ( teamname )
		{	
			Q_snprintf( localizeTeamName, sizeof( localizeTeamName ), "#%s", teamname );
			team=localize()->Find( localizeTeamName );

			if ( !team ) 
			{
				localize()->ConvertANSIToUnicode( teamname , teamText, sizeof( teamText ) );
				team = teamText;
			}

			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem_Team" ), 2, playerName, team );
		}
		else
		{
			localize()->ConstructString( playerText, sizeof( playerText ), localize()->Find( "#Spec_PlayerItem" ), 1, playerName );
		}

		KeyValues *kv = new KeyValues("UserData", "player", gr->GetPlayerName( iPlayerIndex ) );
		itemID = PlayerAddItem( itemID, playerText, kv ); // -1 means a new slot
		kv->deleteThis();
	}

	// make sure the player combo box is up to date
	int playernum = GetSpectatorTarget();
	const char *selectedPlayerName = gr->GetPlayerName( playernum );
	for ( iPlayerIndex=0; iPlayerIndex<m_pPlayerList->GetItemCount(); ++iPlayerIndex )
	{
		KeyValues *kv = m_pPlayerList->GetItemUserData( iPlayerIndex );
		if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
		{
			m_pPlayerList->ActivateItemByRow( iPlayerIndex );
			m_pPlayerList->SetText( selectedPlayerName );
			break;
		}
	}
}

void CSpectatorMenu::OnThink()
{
	BaseClass::OnThink();

	IGameResources *gr = GameResources();
	if ( !gr )
		return;

	// make sure the player combo box is up to date
	int playernum = GetSpectatorTarget();
	const char *selectedPlayerName = gr->GetPlayerName( playernum );
	const char *currentPlayerName = "";
	KeyValues *kv = m_pPlayerList->GetActiveItemUserData();
	if ( kv )
	{
		currentPlayerName = kv->GetString("player");
	}
	if ( !FStrEq( currentPlayerName, selectedPlayerName ) )
	{
		for ( int i=0; i<m_pPlayerList->GetItemCount(); ++i )
		{
			KeyValues *kv = m_pPlayerList->GetItemUserData( i );
			if ( kv && FStrEq( kv->GetString( "player" ), selectedPlayerName ) )
			{
				m_pPlayerList->ActivateItemByRow( i );
				m_pPlayerList->SetText( selectedPlayerName );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CSpectatorGUI::UpdateTimer()
{
	wchar_t szText[ 63 ];

	int timer = 0;

	_snwprintf ( szText, sizeof( szText ), L"%d:%02d\n", (timer / 60), (timer % 60) );

	szText[63] = 0;


	SetLabelText("timerlabel", szText );
}

static void ForwardSpecCmdToServer()
{
	if ( engine->IsPlayingDemo() )
		return;

	if ( engine->Cmd_Argc() == 1 )
	{
		// just forward the command without parameters
		engine->ServerCmd( engine->Cmd_Argv(0) );
	}
	else if ( engine->Cmd_Argc() == 2 )
	{
		// forward the command with parameter
		char command[128];
		Q_snprintf( command, sizeof(command), "%s \"%s\"", engine->Cmd_Argv(0), engine->Cmd_Argv(1) );
		engine->ServerCmd( command );
	}
}

CON_COMMAND( spec_next, "Spectate next player" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		// handle the command clientside
		if ( !HLTVCamera()->IsPVSLocked() )
		{
			HLTVCamera()->SpecNextPlayer( false );
		}
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_prev, "Spectate previous player" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		// handle the command clientside
		if ( !HLTVCamera()->IsPVSLocked() )
		{
			HLTVCamera()->SpecNextPlayer( true );
		}
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_mode, "Set spectator mode" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->IsHLTV() )
	{
		if ( HLTVCamera()->IsPVSLocked() )
		{
			// in locked mode we can only switch between first and 3rd person
			HLTVCamera()->ToggleChaseAsFirstPerson();
		}
		else
		{
			// we can choose any mode, not loked to PVS
			int mode;

			if ( engine->Cmd_Argc() == 2 )
			{
				// set specifc mode
				mode = Q_atoi( engine->Cmd_Argv(1) );
			}
			else
			{
				// set next mode 
				mode = HLTVCamera()->GetMode()+1;

				if ( mode > OBS_MODE_ROAMING )
					mode = OBS_MODE_IN_EYE;
			}
			
			// handle the command clientside
			HLTVCamera()->SetMode( mode );
		}

			// turn off auto director once user tried to change view settings
		HLTVCamera()->SetAutoDirector( false );
	}
	else
	{
		// we spectate on a game server, forward command
		ForwardSpecCmdToServer();
	}
}

CON_COMMAND( spec_player, "Spectate player by name" )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer || !pPlayer->IsObserver() )
		return;

	if ( engine->Cmd_Argc() != 2 )
		return;

	if ( engine->IsHLTV() )
	{
		// we can only switch primary spectator targets is PVS isnt locked by auto-director
		if ( !HLTVCamera()->IsPVSLocked() )
		{
			HLTVCamera()->SpecNamedPlayer( engine->Cmd_Argv(1) );
		}
	}
	else
	{
		ForwardSpecCmdToServer();
	}
}



