//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Status bar that visually displays discrete progress in the form
//			of a segmented strip
//-----------------------------------------------------------------------------
class ProgressBar : public Panel
{
	DECLARE_CLASS_SIMPLE( ProgressBar, Panel );

public:
	ProgressBar(Panel *parent, const char *panelName);
	~ProgressBar();

	// 'progress' is in the range [0.0f, 1.0f]
	MESSAGE_FUNC_FLOAT( SetProgress, "SetProgress", progress );
	float GetProgress();
	virtual void SetSegmentInfo( int gap, int width );

	// utility function for calculating a time remaining string
	static bool ConstructTimeRemainingString(wchar_t *output, int outputBufferSizeInBytes, float startTime, float currentTime, float currentProgress, float lastProgressUpdateTime, bool addRemainingSuffix);

	void SetBarInset( int pixels );
	int GetBarInset( void );
	
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);
	virtual const char *GetDescription();

	// returns the number of segment blocks drawn
	int GetDrawnSegmentCount();

protected:
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_PARAMS( OnDialogVariablesChanged, "DialogVariables", dialogVariables );
	/* CUSTOM MESSAGE HANDLING
		"SetProgress"
			input:	"progress"	- float value of the progress to set
	*/

private:
	int   _segmentCount;
	float _progress;
	int _segmentGap;
	int _segmentWide;
	int m_iBarInset;
	char *m_pszDialogVar;
};

} // namespace vgui

#endif // PROGRESSBAR_H
