//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_BASECHAT_H
#define HUD_BASECHAT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include "vgui_basepanel.h"
#include "vgui_controls/frame.h"
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/RichText.h>

class CBaseHudChatInputLine;
class CBaseHudChatEntry;

namespace vgui
{
	class IScheme;
};

#define CHATLINE_NUM_FLASHES 8.0f
#define CHATLINE_FLASH_TIME 5.0f
#define CHATLINE_FADE_TIME 1.0f


//-----------------------------------------------------------------------------
// Purpose: An output/display line of the chat interface
//-----------------------------------------------------------------------------
class CBaseHudChatLine : public vgui::RichText
{
	typedef vgui::RichText BaseClass;

public:
	CBaseHudChatLine( vgui::Panel *parent, const char *panelName );

	void			SetExpireTime( void );

	bool			IsReadyToExpire( void );

	void			Expire( void );

	float			GetStartTime( void );

	int				GetCount( void );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::HFont		GetFont() { return m_hFont; }

	Color			GetTextColor( void ) { return m_clrText; }
	void			SetNameLength( int iLength ) { m_iNameLength = iLength;	}
	void			SetNameColor( Color cColor ){ m_clrNameColor = cColor; 	}
		
	virtual void	PerformFadeout( void );

protected:
	int				m_iNameLength;
	vgui::HFont		m_hFont;

	Color			m_clrText;
	Color			m_clrNameColor;

	float			m_flExpireTime;
	
private:
	float			m_flStartTime;
	int				m_nCount;

	vgui::HFont		m_hFontMarlett;


private:
	CBaseHudChatLine( const CBaseHudChatLine & ); // not defined, not accessible
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseHudChat : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseHudChat, vgui::EditablePanel );
public:
	DECLARE_MULTIPLY_INHERITED();

	enum
	{
		CHAT_INTERFACE_LINES = 6,
		MAX_CHARS_PER_LINE = 128
	};

					CBaseHudChat( const char *pElementName );
					~CBaseHudChat();

	virtual void	CreateChatInputLine( void );
	virtual void	CreateChatLines( void );
	
	virtual void	Init( void );

	void			LevelInit( const char *newmap );
	void			LevelShutdown( void );

	void			MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
	
	virtual void	Printf( const char *fmt, ... );
	virtual void	ChatPrintf( int iPlayerIndex, const char *fmt, ... );
	
	void			StartMessageMode( int iMessageModeType );
	void			StopMessageMode( void );
	void			Send( void );

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	Paint( void );
	virtual void	OnTick( void );
	virtual void	Reset();
#ifdef _XBOX
	virtual bool	ShouldDraw();
#endif
	vgui::Panel		*GetInputPanel( void );

	static int		m_nLineCounter;

	virtual int		GetChatInputOffset( void );

	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);

protected:
	CBaseHudChatLine		*FindUnusedChatLine( void );
	void				ExpireOldest( void );

	CBaseHudChatInputLine	*m_pChatInput;
	CBaseHudChatLine		*m_ChatLines[ CHAT_INTERFACE_LINES ];
	int					m_iFontHeight;

private:	
	void			Clear( void );

	int				ComputeBreakChar( int width, const char *text, int textlen );

	int				m_nMessageMode;

	int				m_nVisibleHeight;
};

class CBaseHudChatEntry : public vgui::TextEntry
{
	typedef vgui::TextEntry BaseClass;
public:
	CBaseHudChatEntry( vgui::Panel *parent, char const *panelName, CBaseHudChat *pChat )
		: BaseClass( parent, panelName )
	{
		SetCatchEnterKey( true );
		SetAllowNonAsciiCharacters( true );
		SetDrawLanguageIDAtLeft( true );
		m_pHudChat = pChat;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings(pScheme);

		SetPaintBorderEnabled( false );
	}

	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		if ( code == vgui::KEY_ENTER || code == vgui::KEY_PAD_ENTER || code == vgui::KEY_ESCAPE )
		{
			if ( code != vgui::KEY_ESCAPE )
			{
				if ( m_pHudChat )
					m_pHudChat->Send();
			}
		
			// End message mode.
			if ( m_pHudChat )
				m_pHudChat->StopMessageMode();
		}
		else if ( code == vgui::KEY_TAB )
		{
			// Ignore tab, otherwise vgui will screw up the focus.
			return;
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

private:
	CBaseHudChat *m_pHudChat;
};

//-----------------------------------------------------------------------------
// Purpose: The prompt and text entry area for chat messages
//-----------------------------------------------------------------------------
class CBaseHudChatInputLine : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
	
public:
	CBaseHudChatInputLine( CBaseHudChat *parent, char const *panelName );

	void			SetPrompt( const wchar_t *prompt );
	void			ClearEntry( void );
	void			SetEntry( const wchar_t *entry );
	void			GetMessageText( wchar_t *buffer, int buffersizebytes );

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);

	vgui::Panel		*GetInputPanel( void );
	virtual vgui::VPANEL GetCurrentKeyFocus() { return m_pInput->GetVPanel(); } 

	virtual void Paint()
	{
		BaseClass::Paint();
	}

protected:
	vgui::Label		*m_pPrompt;
	CBaseHudChatEntry	*m_pInput;
};

#endif // HUD_BASECHAT_H
