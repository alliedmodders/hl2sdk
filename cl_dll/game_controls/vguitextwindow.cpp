//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vguitextwindow.h"
#include <networkstringtabledefs.h>
#include <cdll_client_int.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <FileSystem.h>
#include <KeyValues.h>
#include <convar.h>
#include <vgui_controls/ImageList.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>

#include <cl_dll/iviewport.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
extern INetworkStringTable *g_pStringTableInfoPanel;

#define TEMP_HTML_FILE	"textwindow_temp.html"


CON_COMMAND( showinfo, "Shows a info panel: <type> <title> <message> [<command>]" )
{
	if ( !gViewPortInterface )
		return;
	
	if ( engine->Cmd_Argc() < 4 )
		return;
		
	IViewPortPanel * panel = gViewPortInterface->FindPanelByName( PANEL_INFO );

	 if ( panel )
	 {
		 KeyValues *kv = new KeyValues("data");
		 kv->SetInt( "type", Q_atoi(engine->Cmd_Argv( 1 )) );
		 kv->SetString( "title", engine->Cmd_Argv( 2 ) );
		 kv->SetString( "message", engine->Cmd_Argv( 3 ) );

		 if ( engine->Cmd_Argc() == 5 )
			 kv->SetString( "command", engine->Cmd_Argv( 4 ) );

		 panel->SetData( kv );

		 gViewPortInterface->ShowPanel( panel, true );

		 kv->deleteThis();
	 }
	 else
	 {
		 Msg("Couldn't find info panel.\n" );
	 }
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTextWindow::CTextWindow(IViewPort *pViewPort) : Frame(NULL, PANEL_INFO	)
{
	// initialize dialog
	m_pViewPort = pViewPort;

//	SetTitle("", true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);
	SetProportional(true);

	// hide the system buttons
	SetTitleBarVisible( false );

	m_pTextMessage = new TextEntry(this, "TextMessage");
	m_pHTMLMessage = new HTML(this,"HTMLMessage");;
	m_pTitleLable  = new Label( this, "MessageTitle", "Message Title" );
	m_pOK		   = new Button(this, "ok", "#PropertyDialog_OK");

	m_pOK->SetCommand("okay");
	m_pTextMessage->SetMultiline( true );
	
	LoadControlSettings("Resource/UI/TextWindow.res");
	
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTextWindow::~CTextWindow()
{
	// remove temp file again
	vgui::filesystem()->RemoveFile( TEMP_HTML_FILE, "GAME" );
}

void CTextWindow::Reset( void )
{
	Q_strcpy( m_szTitle, "This could be your Title." );
	Q_strcpy( m_szMessage, "Just for 10 Euros a week!" );
	m_szExitCommand[0] = 0;
	m_nContentType = TYPE_TEXT;
	Update();
}

void CTextWindow::ShowText( const char *text)
{
	m_pTextMessage->SetVisible( true );
	m_pTextMessage->SetText( text );
	m_pTextMessage->GotoTextStart();
}

void CTextWindow::ShowURL( const char *URL)
{
	m_pHTMLMessage->SetVisible( true );
	m_pHTMLMessage->OpenURL( URL );
}

void CTextWindow::ShowIndex( const char *entry)
{
	const char *data = NULL;
	int length = 0;
	int index = g_pStringTableInfoPanel->FindStringIndex( m_szMessage );
		
	if ( index != ::INVALID_STRING_INDEX )
		data = (const char *)g_pStringTableInfoPanel->GetStringUserData( index, &length );

	if ( !data || !data[0] )
		return; // nothing to show

	// is this a web URL ?
	if ( !Q_strncmp( data, "http://", 7 ) )
	{
		ShowURL( data );
		return;
	}

	// try to figure out if this is HTML or not
	if ( data[0] != '<' )
	{
		ShowText( data );
		return;
	}

	// data is a HTML, we have to write to a file and then load the file
	FileHandle_t hFile = vgui::filesystem()->Open( TEMP_HTML_FILE, "wb", "GAME" );

	if ( hFile == FILESYSTEM_INVALID_HANDLE )
		return;

	vgui::filesystem()->Write( data, length, hFile );
	vgui::filesystem()->Close( hFile );

	if ( vgui::filesystem()->Size( TEMP_HTML_FILE ) != (unsigned int)length )
		return; // something went wrong while writing

	ShowFile( TEMP_HTML_FILE );
}

void CTextWindow::ShowFile( const char *filename )
{
	if  ( Q_stristr( filename, ".htm" ) || Q_stristr( filename, ".html" ) )
	{
		// it's a local HTML file
		char localURL[ _MAX_PATH + 7 ];
		Q_strncpy( localURL, "file://", sizeof( localURL ) );
		
		char pPathData[ _MAX_PATH ];
		vgui::filesystem()->GetLocalPath( filename, pPathData, sizeof(pPathData) );
		Q_strncat( localURL, pPathData, sizeof( localURL ), COPY_ALL_CHARACTERS );

		ShowURL( localURL );
	}
	else
	{
		// read from local text from file
		FileHandle_t f = vgui::filesystem()->Open( m_szMessage, "rb", "GAME" );

		if ( !f )
			return;

		char buffer[2048];
			
		int size = min( vgui::filesystem()->Size( f ), sizeof(buffer)-1 ); // just allow 2KB

		vgui::filesystem()->Read( buffer, size, f );
		vgui::filesystem()->Close( f );

		buffer[size]=0; //terminate string

		ShowText( buffer );
	}
}

void CTextWindow::Update( void )
{
	SetTitle( m_szTitle, false );

	m_pTitleLable->SetText( m_szTitle );

	m_pHTMLMessage->SetVisible( false );
	m_pTextMessage->SetVisible( false );

	if ( m_nContentType == TYPE_INDEX )
	{
		ShowIndex( m_szMessage );
	}
	else if ( m_nContentType == TYPE_URL )
	{
		ShowURL( m_szMessage );
	}
	else if ( m_nContentType == TYPE_FILE )
	{
		ShowFile( m_szMessage )		;
	}
	else if ( m_nContentType == TYPE_TEXT )
	{
		ShowText( m_szMessage );
	}
	else
	{
		DevMsg("CTextWindow::Update: unknown content type %i\n", m_nContentType );
	}
}

void CTextWindow::OnCommand( const char *command)
{
    if (!Q_strcmp(command, "okay"))
    {
		if ( m_szExitCommand[0] )
		{
			engine->ClientCmd( m_szExitCommand );
		}
		
		m_pViewPort->ShowPanel( this, false );
	}

	BaseClass::OnCommand(command);
}

void CTextWindow::SetData(KeyValues *data)
{
	SetData( data->GetInt( "type" ), data->GetString( "title"), data->GetString( "msg" ), data->GetString( "cmd" ) );
}

void CTextWindow::SetData( int type, const char *title, const char *message, const char *command )
{
	Q_strncpy(  m_szTitle, title, sizeof( m_szTitle) );
	Q_strncpy(  m_szMessage, message, sizeof( m_szTitle) );
	
	if ( command )
	{
		Q_strncpy( m_szExitCommand, command, sizeof(m_szExitCommand) );
	}
	else
	{
		m_szExitCommand[0]=0;
	}

	m_nContentType = type;

	Update();
}

void CTextWindow::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;

	m_pViewPort->ShowBackGround( bShow );

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}
