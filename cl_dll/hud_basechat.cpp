//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud_basechat.h"

#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "iclientmode.h"
#include "hud_macros.h"
#include "engine/IEngineSound.h"
#include "text_message.h"
#include <vgui/ILocalize.h>
#include "vguicenterprint.h"
#include "vgui/keycode.h"
#include <KeyValues.h>
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHAT_WIDTH_PERCENTAGE 0.6f

#ifndef _XBOX
ConVar hud_saytext_time( "hud_saytext_time", "12", 0 );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//			*panelName - 
//-----------------------------------------------------------------------------
CBaseHudChatLine::CBaseHudChatLine( vgui::Panel *parent, const char *panelName ) : 
	vgui::RichText( parent, panelName )
{
	m_hFont = m_hFontMarlett = 0;
	m_flExpireTime = 0.0f;
	m_flStartTime = 0.0f;
	m_iNameLength	= 0;

	SetPaintBackgroundEnabled( true );
	
	SetVerticalScrollbar( false );
}
	

void CBaseHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "Default" );

#ifdef HL1_CLIENT_DLL
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );

	SetBorder( NULL );
#else
	SetBgColor( Color( 0, 0, 0, 100 ) );
#endif


	m_hFontMarlett = pScheme->GetFont( "Marlett" );

	m_clrText = pScheme->GetColor( "FgColor", GetFgColor() );
	SetFont( m_hFont );
}


void CBaseHudChatLine::PerformFadeout( void )
{
	// Flash + Extra bright when new
	float curtime = gpGlobals->curtime;

	int lr = m_clrText[0];
	int lg = m_clrText[1];
	int lb = m_clrText[2];
	
	if ( curtime >= m_flStartTime && curtime < m_flStartTime + CHATLINE_FLASH_TIME )
	{
		float frac1 = ( curtime - m_flStartTime ) / CHATLINE_FLASH_TIME;
		float frac = frac1;

		frac *= CHATLINE_NUM_FLASHES;
		frac *= 2 * M_PI;

		frac = cos( frac );

		frac = clamp( frac, 0.0f, 1.0f );

		frac *= (1.0f-frac1);

		int r = lr, g = lg, b = lb;

		r = r + ( 255 - r ) * frac;
		g = g + ( 255 - g ) * frac;
		b = b + ( 255 - b ) * frac;
	
		// Draw a right facing triangle in red, faded out over time
		int alpha = 63 + 192 * (1.0f - frac1 );
		alpha = clamp( alpha, 0, 255 );

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( r, g, b, 255 ) );
		InsertString( wbuf );
	}
	else if ( curtime <= m_flExpireTime && curtime > m_flExpireTime - CHATLINE_FADE_TIME )
	{
		float frac = ( m_flExpireTime - curtime ) / CHATLINE_FADE_TIME;

		int alpha = frac * 255;
		alpha = clamp( alpha, 0, 255 );

		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( lr * frac, lg * frac, lb * frac, alpha ) );
		InsertString( wbuf );
	}
	else
	{
		wchar_t wbuf[4096];
		GetText(0, wbuf, sizeof(wbuf));

		SetText( "" );

		InsertColorChange( Color( lr, lg, lb, 255 ) );
		InsertString( wbuf );
	}

	OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CBaseHudChatLine::SetExpireTime( void )
{
	m_flStartTime = gpGlobals->curtime;
	m_flExpireTime = m_flStartTime + hud_saytext_time.GetFloat();
	m_nCount = CBaseHudChat::m_nLineCounter++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseHudChatLine::GetCount( void )
{
	return m_nCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHudChatLine::IsReadyToExpire( void )
{
	// Engine disconnected, expire right away
	if ( !engine->IsInGame() && !engine->IsConnected() )
		return true;

	if ( gpGlobals->curtime >= m_flExpireTime )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseHudChatLine::GetStartTime( void )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChatLine::Expire( void )
{
	SetVisible( false );

	// Spit out label text now
//	char text[ 256 ];
//	GetText( text, 256 );

//	Msg( "%s\n", text );
}
#endif _XBOX

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
#ifndef _XBOX
CBaseHudChatInputLine::CBaseHudChatInputLine( CBaseHudChat *parent, char const *panelName ) : 
	vgui::Panel( parent, panelName )
{
	SetMouseInputEnabled( false );

	m_pPrompt = new vgui::Label( this, "ChatInputPrompt", L"Enter text:" );

	m_pInput = new CBaseHudChatEntry( this, "ChatInput", parent );	
	m_pInput->SetMaximumCharCount( 127 );
}

void CBaseHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	// FIXME:  Outline
	vgui::HFont hFont = pScheme->GetFont( "Trebuchet18" );

	m_pPrompt->SetFont( hFont );
	m_pInput->SetFont( hFont );

	SetPaintBackgroundEnabled( false );
	m_pPrompt->SetPaintBackgroundEnabled( false );
	m_pPrompt->SetContentAlignment( vgui::Label::a_west );
	m_pPrompt->SetTextInset( 2, 0 );

#ifdef HL1_CLIENT_DLL
	m_pInput->SetBgColor( Color( 255, 255, 255, 0 ) );
#else
	m_pInput->SetFgColor( GetFgColor() );
	m_pInput->SetBgColor( GetBgColor() );
#endif

}

void CBaseHudChatInputLine::SetPrompt( const wchar_t *prompt )
{
	Assert( m_pPrompt );
	m_pPrompt->SetText( prompt );
	InvalidateLayout();
}

void CBaseHudChatInputLine::ClearEntry( void )
{
	Assert( m_pInput );
	SetEntry( L"" );
}

void CBaseHudChatInputLine::SetEntry( const wchar_t *entry )
{
	Assert( m_pInput );
	Assert( entry );

	m_pInput->SetText( entry );
}

void CBaseHudChatInputLine::GetMessageText( wchar_t *buffer, int buffersizebytes )
{
	m_pInput->GetText( buffer, buffersizebytes);
}

void CBaseHudChatInputLine::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	int w,h;
	m_pPrompt->GetContentSize( w, h); 
	m_pPrompt->SetBounds( 0, 0, w, tall );

	m_pInput->SetBounds( w + 2, 0, wide - w - 2 , tall );
}

vgui::Panel *CBaseHudChatInputLine::GetInputPanel( void )
{
	return m_pInput;
}
#endif //_XBOX

int CBaseHudChat::m_nLineCounter = 1;
//-----------------------------------------------------------------------------
// Purpose: Text chat input/output hud element
//-----------------------------------------------------------------------------
CBaseHudChat::CBaseHudChat( const char *pElementName )
: CHudElement( pElementName ), BaseClass( NULL, "HudChat" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);

	m_nMessageMode = 0;

	CBaseHudChatLine *line = m_ChatLines[ 0 ];

	if ( line )
	{
		vgui::HFont font = line->GetFont();
		m_iFontHeight = vgui::surface()->GetFontTall( font ) + 2;

		// Put input area at bottom
		int w, h;
		GetSize( w, h );
		m_pChatInput->SetBounds( 1, h - m_iFontHeight - 1, w-2, m_iFontHeight );
	}

	if ( IsPC() )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel() );
	}

	// (We don't actually want input until they bring up the chat line).
	MakePopup();
	SetZPos( -30 );

	SetHiddenBits( HIDEHUD_CHAT );
}

CBaseHudChat::~CBaseHudChat()
{
	if ( IsXbox() )
		return;

	gameeventmanager->RemoveListener( this );
}

void CBaseHudChat::CreateChatInputLine( void )
{
#ifndef _XBOX
	m_pChatInput = new CBaseHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
#endif
}

void CBaseHudChat::CreateChatLines( void )
{
#ifndef _XBOX
	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		char sz[ 32 ];
		Q_snprintf( sz, sizeof( sz ), "ChatLine%02i", i );
		m_ChatLines[ i ] = new CBaseHudChatLine( this, sz );
		m_ChatLines[ i ]->SetVisible( false );		
	}
#endif
}

void CBaseHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	m_nVisibleHeight = 0;

	// Put input area at bottom
	if ( m_pChatInput )
	{
		int w, h;
		GetSize( w, h );
		m_pChatInput->SetBounds( 1, h - m_iFontHeight - 1, w-2, m_iFontHeight );
	}

#ifdef HL1_CLIENT_DLL
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );
#else
	SetBgColor( Color( 0, 0, 0, 100 ) );
#endif

}

void CBaseHudChat::Reset( void )
{
#ifndef HL1_CLIENT_DLL
	m_nVisibleHeight = 0;
	Clear();
#endif
}

#ifdef _XBOX
bool CBaseHudChat::ShouldDraw()
{
	// never think, never draw
	return false;
}
#endif

void CBaseHudChat::Paint( void )
{
#ifndef _XBOX
	if ( m_nVisibleHeight == 0 )
		return;

	int w, h;
	GetSize( w, h );

	vgui::surface()->DrawSetColor( GetBgColor() );
	vgui::surface()->DrawFilledRect( 0, h - m_nVisibleHeight, w, h );

	vgui::surface()->DrawSetColor( GetFgColor() );
	vgui::surface()->DrawOutlinedRect( 0, h - m_nVisibleHeight, w, h );
#endif
}

void CBaseHudChat::Init( void )
{
	if ( IsXbox() )
		return;

	CreateChatInputLine();
	CreateChatLines();

	gameeventmanager->AddListener( this, "hltv_chat", false );
}

#ifndef _XBOX
static int __cdecl SortLines( void const *line1, void const *line2 )
{
	CBaseHudChatLine *l1 = *( CBaseHudChatLine ** )line1;
	CBaseHudChatLine *l2 = *( CBaseHudChatLine ** )line2;

	// Invisible at bottom
	if ( l1->IsVisible() && !l2->IsVisible() )
		return -1;
	else if ( !l1->IsVisible() && l2->IsVisible() )
		return 1;

	// Oldest start time at top
	if ( l1->GetStartTime() < l2->GetStartTime() )
		return -1;
	else if ( l1->GetStartTime() > l2->GetStartTime() )
		return 1;

	// Otherwise, compare counter
	if ( l1->GetCount() < l2->GetCount() )
		return -1;
	else if ( l1->GetCount() > l2->GetCount() )
		return 1;

	return 0;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Allow inheriting classes to change this spacing behavior
//-----------------------------------------------------------------------------
int CBaseHudChat::GetChatInputOffset( void )
{
	return m_iFontHeight;
}

//-----------------------------------------------------------------------------
// Purpose: Do respositioning here to avoid latency due to repositioning of vgui
//  voice manager icon panel
//-----------------------------------------------------------------------------
void CBaseHudChat::OnTick( void )
{
#ifndef _XBOX
	int i;
	for ( i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CBaseHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		if ( !line->IsReadyToExpire() )
			continue;

		line->Expire();
	}

	int w, h;

	GetSize( w, h );

	CBaseHudChatLine *line = m_ChatLines[ 0 ];

	if ( line )
	{
		vgui::HFont font = line->GetFont();

		if ( font )
		{
			m_iFontHeight = vgui::surface()->GetFontTall( font ) + 2;

			// Put input area at bottom
			int w, h;
			GetSize( w, h );
			m_pChatInput->SetBounds( 1, h - m_iFontHeight - 1, w-2, m_iFontHeight );
		}
	}

	// Sort chat lines 
	qsort( m_ChatLines, CHAT_INTERFACE_LINES, sizeof( CBaseHudChatLine * ), SortLines );

	// Step backward from bottom
	int currentY = h - m_iFontHeight - 1;
	int startY = currentY;
	int ystep = m_iFontHeight;

	currentY -= GetChatInputOffset();

	// Walk backward
	for ( i = CHAT_INTERFACE_LINES - 1; i >= 0 ; i-- )
	{
		CBaseHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
		{
			line->SetSize( w, m_iFontHeight );
			continue;
		}

		line->PerformFadeout();
		line->SetSize( w, m_iFontHeight * line->GetNumLines() );
		line->SetPos( 0, ( currentY+m_iFontHeight) - m_iFontHeight * line->GetNumLines() );
		
		currentY -= ystep * line->GetNumLines();
	}

	if ( currentY != startY )
	{
		m_nVisibleHeight = startY - currentY + 2;
	}
	else
	{
		m_nVisibleHeight = 0;
	}

	vgui::surface()->MovePopupToBack( GetVPanel() );
#endif
}

// Release build is crashing on long strings...sigh
#pragma optimize( "", off )

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : width - 
//			*text - 
//			textlen - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseHudChat::ComputeBreakChar( int width, const char *text, int textlen )
{
#ifndef _XBOX
	CBaseHudChatLine *line = m_ChatLines[ 0 ];
	vgui::HFont font = line->GetFont();

	int currentlen = 0;
	int lastbreak = textlen;
	for (int i = 0; i < textlen ; i++)
	{
		char ch = text[i];

		if ( ch <= 32 )
		{
			lastbreak = i;
		}

		wchar_t wch[2];

		vgui::localize()->ConvertANSIToUnicode( &ch, wch, sizeof( wch ) );

		int a,b,c;

		vgui::surface()->GetCharABCwide(font, wch[0], a, b, c);
		currentlen += a + b + c;

		if ( currentlen >= width )
		{
			// If we haven't found a whitespace char to break on before getting
			//  to the end, but it's still too long, break on the character just before
			//  this one
			if ( lastbreak == textlen )
			{
				lastbreak = max( 0, i - 1 );
			}
			break;
		}
	}

	if ( currentlen >= width )
	{
		return lastbreak;
	}
	return textlen;
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CBaseHudChat::Printf( const char *fmt, ... )
{
#ifndef _XBOX
	// No chat text in single player
	if ( gpGlobals->maxClients == 1 )
	{
		return;
	}

	va_list marker;
	char msg[4096];

	const char *pTranslatedFmt = hudtextmessage->LookupString(fmt);

	va_start(marker, fmt);
	Q_vsnprintf( msg, sizeof( msg ), pTranslatedFmt, marker );
	va_end(marker);

	// Strip any trailing '\n'
	if ( strlen( msg ) > 0 && msg[ strlen( msg )-1 ] == '\n' )
	{
		msg[ strlen( msg ) - 1 ] = 0;
	}

	// Strip leading \n characters ( or notify/color signifiers )
	char *pmsg = msg;
	while ( *pmsg && ( *pmsg == '\n' || *pmsg == 1 || *pmsg == 2 ) )
	{
		pmsg++;
	}
	
	if ( !*pmsg )
		return;

	// Search for return characters
	char *pos = strstr( pmsg, "\n" );
	if ( pos )
	{
		char msgcopy[ 8192 ];
		Q_strncpy( msgcopy, pmsg, sizeof( msgcopy ) );

		int offset = pos - pmsg;
		
		// Terminate first part
		msgcopy[ offset ] = 0;
		
		// Print first part
#if defined( CSTRIKE_DLL ) || defined( DOD_DLL ) // reltodo
		Printf( "%s", msgcopy );

		// Print remainder
		Printf( "%s", &msgcopy[ offset ] + 1 );
#else
		Printf( msgcopy );

		// Print remainder
		Printf( &msgcopy[ offset ] + 1 );
#endif
		return;
	}

	CBaseHudChatLine *firstline = m_ChatLines[ 0 ];

	int len = strlen( pmsg );

	// Check for string too long and split into multiple lines 
	// 
	int breakpos = ComputeBreakChar( firstline->GetWide() - 10, pmsg, len );
	if ( breakpos > 0 && breakpos < len )
	{
		char msgcopy[ 8192 ];
		Q_strncpy( msgcopy, pmsg, sizeof( msgcopy ) );

		int offset = breakpos;
		
		char savechar;

		savechar = msgcopy[ offset ];

		// Terminate first part
		msgcopy[ offset ] = 0;
		
		// Print first part
#if defined( CSTRIKE_DLL ) || defined( DOD_DLL ) // reltodo
		Printf( "%s", msgcopy );
#else
		Printf( msgcopy );
#endif

		// Was breakpos a printable char?
		if ( savechar > 32 )
		{
			msgcopy[ offset ] = savechar;

			// Print remainder
#if defined( CSTRIKE_DLL ) || defined( DOD_DLL ) // reltodo
			Printf( "%s", &msgcopy[ offset ] );
#else
			Printf( &msgcopy[ offset ] );
#endif
		}
		else
		{
#if defined( CSTRIKE_DLL ) || defined( DOD_DLL ) // reltodo
			Printf( "%s", &msgcopy[ offset ] + 1 );
#else
			Printf( &msgcopy[ offset ] + 1 );
#endif
		}
		return;
	}

	CBaseHudChatLine *line = FindUnusedChatLine();
	if ( !line )
	{
		ExpireOldest();
		line = FindUnusedChatLine();
	}

	if ( !line )
	{
		return;
	}

	line->SetText( "" );
	line->InsertColorChange( line->GetTextColor() );
	line->SetExpireTime();
	line->InsertString( pmsg );
	line->SetVisible( true );
	line->SetNameLength( 0 );
	
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );
#endif
}
	
#pragma optimize( "", on )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::StartMessageMode( int iMessageModeType )
{
#ifndef _XBOX
	m_nMessageMode = iMessageModeType;

	m_pChatInput->ClearEntry();

	if ( m_nMessageMode == MM_SAY )
	{
		m_pChatInput->SetPrompt( L"Say :" );
	}
	else
	{
		m_pChatInput->SetPrompt( L"Say (TEAM) :" );
	}
	
	vgui::SETUP_PANEL( this );
	SetKeyBoardInputEnabled( true );
	m_pChatInput->SetVisible( true );
	vgui::surface()->CalculateMouseVisible();
	m_pChatInput->RequestFocus();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::StopMessageMode( void )
{
#ifndef _XBOX
	SetKeyBoardInputEnabled( false );
	m_pChatInput->SetVisible( false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseHudChatLine
//-----------------------------------------------------------------------------
CBaseHudChatLine *CBaseHudChat::FindUnusedChatLine( void )
{
#ifndef _XBOX
	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CBaseHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( line->IsVisible() )
			continue;

		return line;
	}
	return NULL;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::ExpireOldest( void )
{
#ifndef _XBOX
	float oldestTime = 100000000.0f;
	CBaseHudChatLine *oldest = NULL;

	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CBaseHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		if ( !oldest )
		{
			oldest = line;
			oldestTime = line->GetStartTime();
			continue;
		}

		if ( line->GetStartTime() < oldestTime )
		{
			oldest = line;
			oldestTime = line->GetStartTime();
		}
	}

	if ( !oldest )
	{
		oldest = m_ChatLines[ 0  ];
	}

	oldest->Expire(); 
#endif
}

void CBaseHudChat::Send( void )
{
#ifndef _XBOX
	wchar_t szTextbuf[128];

	m_pChatInput->GetMessageText( szTextbuf, sizeof( szTextbuf ) );
	
	char ansi[128];
	vgui::localize()->ConvertUnicodeToANSI( szTextbuf, ansi, sizeof( ansi ) );

	int len = Q_strlen(ansi);

	/*
This is a very long string that I am going to attempt to paste into the cs hud chat entry and we will see if it gets cropped or not.
	*/

	// remove the \n
	if ( len > 0 &&
		ansi[ len - 1 ] == '\n' )
	{
		ansi[ len - 1 ] = '\0';
	}

	if( len > 0 )
	{
		char szbuf[144];	// more than 128
		Q_snprintf( szbuf, sizeof(szbuf), "%s \"%s\"", m_nMessageMode == MM_SAY ? "say" : "say_team", ansi );

		engine->ClientCmd(szbuf);
	}
	
	m_pChatInput->ClearEntry();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *CBaseHudChat::GetInputPanel( void )
{
#ifndef _XBOX
	return m_pChatInput->GetInputPanel();
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::Clear( void )
{
#ifndef _XBOX
	// Kill input prompt
	StopMessageMode();

	// Expire all messages
	for ( int i = 0; i < CHAT_INTERFACE_LINES; i++ )
	{
		CBaseHudChatLine *line = m_ChatLines[ i ];
		if ( !line )
			continue;

		if ( !line->IsVisible() )
			continue;

		line->Expire();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *newmap - 
//-----------------------------------------------------------------------------
void CBaseHudChat::LevelInit( const char *newmap )
{
	Clear();
}

void CBaseHudChat::LevelShutdown( void )
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *fmt - 
//			... - 
//-----------------------------------------------------------------------------
void CBaseHudChat::ChatPrintf( int iPlayerIndex, const char *fmt, ... )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHudChat::FireGameEvent( IGameEvent *event )
{
#ifndef _XBOX
	const char *eventname = event->GetName();

	if ( Q_strcmp( "hltv_chat", eventname ) == 0 )
	{
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

		if ( !player )
			return;
		
		ChatPrintf( player->entindex(), "(SourceTV) %s", event->GetString( "text" ) );
	}
#endif
}