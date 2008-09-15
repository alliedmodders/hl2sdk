//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Controls.h>
#include <vgui_controls/Frame.h>

namespace vgui
{

class Label;
class Button;
class TextEntry;

//-----------------------------------------------------------------------------
// Purpose: Utility dialog, used to let user type in some text
//-----------------------------------------------------------------------------
class InputDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( InputDialog, Frame );

public:
	InputDialog( vgui::Panel *parent, const char *title, char const *prompt, char const *defaultValue = "" );
	~InputDialog();

	void SetMultiline( bool state );

	/* action signals

		"InputCompleted"
			"text"	- the text entered

		"InputCanceled"
	*/
	void DoModal( KeyValues *pContextKeyValues = NULL );
	void AllowNumericInputOnly( bool bOnlyNumeric );

protected:
	virtual void PerformLayout();

	// command buttons
	virtual void OnCommand(const char *command);

private:
	void CleanUpContextKeyValues();
	KeyValues *m_pContextKeyValues;

	vgui::Label			*m_pPrompt;
	vgui::TextEntry		*m_pInput;

	vgui::Button		*m_pCancelButton;
	vgui::Button		*m_pOKButton;
};

} // namespace vgui


#endif // INPUTDIALOG_H
