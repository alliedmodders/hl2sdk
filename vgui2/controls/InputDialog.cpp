//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <vgui_controls/InputDialog.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextEntry.h>
#include "tier1/KeyValues.h"
#include "vgui/IInput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
InputDialog::InputDialog(vgui::Panel *parent, const char *title, char const *prompt, char const *defaultValue /*=""*/ ) : 
	BaseClass(parent, NULL)
{
	m_pContextKeyValues = NULL;
	SetDeleteSelfOnClose( true );
	SetTitle(title, true);
	SetSize(320, 120);
	SetSizeable( false );

	m_pPrompt = new Label( this, "Prompt", prompt );
	
	m_pInput = new TextEntry( this, "Text" );
	m_pInput->SetText( defaultValue );
	m_pInput->SelectAllText( true );
	m_pInput->RequestFocus();

	m_pCancelButton = new Button(this, "CancelButton", "#VGui_Cancel");
	m_pOKButton = new Button(this, "OKButton", "#VGui_OK");
	m_pCancelButton->SetCommand("Cancel");
	m_pOKButton->SetCommand("OK");
	m_pOKButton->SetAsDefaultButton( true );

	if ( parent )
	{
		AddActionSignalTarget( parent );
	}
}


InputDialog::~InputDialog()
{
	CleanUpContextKeyValues();
}


//-----------------------------------------------------------------------------
// Purpose: Cleans up the keyvalues
//-----------------------------------------------------------------------------
void InputDialog::CleanUpContextKeyValues()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}

	
//-----------------------------------------------------------------------------
// Sets the dialog to be multiline
//-----------------------------------------------------------------------------
void InputDialog::SetMultiline( bool state )
{
	m_pInput->SetMultiline( state );
	m_pInput->SetCatchEnterKey( state );
}

	
//-----------------------------------------------------------------------------
// Allow numeric input only
//-----------------------------------------------------------------------------
void InputDialog::AllowNumericInputOnly( bool bOnlyNumeric )
{
	if ( m_pInput )
	{
		m_pInput->SetAllowNumericInputOnly( bOnlyNumeric );
	}
}

	
//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void InputDialog::DoModal( KeyValues *pContextKeyValues )
{
	CleanUpContextKeyValues();
	m_pContextKeyValues = pContextKeyValues;
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void InputDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );

	// lay out all the controls
	int topy = IsSmallCaption() ? 15 : 30;

	m_pPrompt->SetBounds(24, topy, w - 48, 24);
	m_pInput->SetBounds(24, topy + 30, w - 48, h - 100 );

	m_pOKButton->SetBounds(w - 172, h - 30, 72, 24);
	m_pCancelButton->SetBounds(w - 96, h - 30, 72, 24);
}


//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void InputDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		int nTextLength = m_pInput->GetTextLength() + 1;
		char* txt = (char*)_alloca( nTextLength * sizeof(char) );
		m_pInput->GetText( txt, nTextLength );
		KeyValues *kv = new KeyValues( "InputCompleted", "text", txt );
		if ( m_pContextKeyValues )
		{
			kv->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		PostActionSignal( kv );
		CloseModal();
	}
	else if (!stricmp(command, "Cancel"))
	{
		KeyValues *kv = new KeyValues( "InputCanceled" );
		if ( m_pContextKeyValues )
		{
			kv->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		PostActionSignal( kv );
		CloseModal();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}