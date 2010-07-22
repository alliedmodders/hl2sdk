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

enum progress_textures_t
{
	PROGRESS_TEXTURE_FG,
	PROGRESS_TEXTURE_BG,

	NUM_PROGRESS_TEXTURES,
};

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

	enum ProgressDir_e
	{
		PROGRESS_EAST,
		PROGRESS_WEST,
		PROGRESS_NORTH,
		PROGRESS_SOUTH
	};

	int GetProgressDirection() const { return m_iProgressDirection; }
	void SetProgressDirection( int val ) { m_iProgressDirection = val; }

protected:
	virtual void Paint();
	void PaintSegment( int &x, int &y, int tall, int wide );
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_PARAMS( OnDialogVariablesChanged, "DialogVariables", dialogVariables );
	/* CUSTOM MESSAGE HANDLING
		"SetProgress"
			input:	"progress"	- float value of the progress to set
	*/

protected:
	int m_iProgressDirection;
	float _progress;

private:
	int   _segmentCount;
	int _segmentGap;
	int _segmentWide;
	int m_iBarInset;
	char *m_pszDialogVar;
};

#define NUM_CONTINUOUS_PROGRESS_BAR_TEXTURES 2

//-----------------------------------------------------------------------------
// Purpose: Non-segmented progress bar
//-----------------------------------------------------------------------------
class ContinuousProgressBar : public ProgressBar
{
	DECLARE_CLASS_SIMPLE( ContinuousProgressBar, ProgressBar );

public:
	ContinuousProgressBar(Panel *parent, const char *panelName);

	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void SetImage(const char *imageName, progress_textures_t iPos);

private:
	int m_nTextureId[NUM_PROGRESS_TEXTURES];
	char *m_pszImageName[NUM_PROGRESS_TEXTURES];
	int   m_lenImageName[NUM_PROGRESS_TEXTURES];
};

} // namespace vgui

#endif // PROGRESSBAR_H
