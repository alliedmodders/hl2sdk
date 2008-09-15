//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOUSEOVERPANELBUTTON_H
#define MOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/Button.h>
#include <filesystem.h>

extern vgui::Panel *g_lastPanel;

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//    
// the new panel has the same dimensions as the passed templatePanel and is of
// the same class.
//
// must at least inherit from vgui::EditablePanel to support LoadControlSettings
//-----------------------------------------------------------------------------
template <class T>
class MouseOverButton : public vgui::Button
{
private:
	DECLARE_CLASS_SIMPLE( MouseOverButton, vgui::Button );
	
public:
	MouseOverButton(vgui::Panel *parent, const char *panelName, T *templatePanel ) :
					Button( parent, panelName, "MouseOverButton")
	{
		m_pPanel = new T( parent, NULL );
		m_pPanel ->SetVisible( false );

		// copy size&pos from template panel
		int x,y,wide,tall;
		templatePanel->GetBounds( x, y, wide, tall );
		m_pPanel->SetBounds( x, y, wide, tall );
		int px, py;
		templatePanel->GetPinOffset( px, py );
		int rx, ry;
		templatePanel->GetResizeOffset( rx, ry );
		// Apply pin settings from template, too
		m_pPanel->SetAutoResize( templatePanel->GetPinCorner(), templatePanel->GetAutoResize(), px, py, rx, ry );

	}

	virtual void ShowPage()
	{
		if( m_pPanel )
		{
			m_pPanel->SetVisible( true );
			m_pPanel->MoveToFront();
			g_lastPanel = m_pPanel;
		}
	}
	
	virtual void HidePage()
	{
		if ( m_pPanel )
		{
			m_pPanel->SetVisible( false );
		}
	}

	const char *GetClassPage( const char *className )
	{
		static char classPanel[ _MAX_PATH ];
		Q_snprintf( classPanel, sizeof( classPanel ), "classes/%s.res", className);

		if ( vgui::filesystem()->FileExists( classPanel ) )
		{
		}
		else if (vgui::filesystem()->FileExists( "classes/default.res" ) )
		{
			Q_snprintf ( classPanel, sizeof( classPanel ), "classes/default.res" );
		}
		else
		{
			return NULL;
		}

		return classPanel;
	}

#ifdef REFRESH_CLASSMENU_TOOL

	void RefreshClassPage( void )
	{
		m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}

#endif

	virtual void ApplySettings( KeyValues *resourceData ) 
	{
		BaseClass::ApplySettings( resourceData );

		// name, position etc of button is set, now load matching
		// resource file for associated info panel:
		m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}		

private:

	virtual void OnCursorEntered() 
	{
		BaseClass::OnCursorEntered();

		if ( m_pPanel && IsEnabled() )
		{
			if ( g_lastPanel )
			{
				g_lastPanel->SetVisible( false );
			}
			ShowPage();
		}
	}

	T *m_pPanel;
};

#define MouseOverPanelButton MouseOverButton<vgui::EditablePanel>

#endif // MOUSEOVERPANELBUTTON_H
