//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Material editor
//=============================================================================

#ifndef VGUIMATSYSAPP_H
#define VGUIMATSYSAPP_H

#ifdef _WIN32
#pragma once
#endif


#include "appframework/AppFramework.h"


//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CVguiMatSysApp : public CSteamAppSystemGroup
{
public:
	CVguiMatSysApp();

	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual void PostShutdown();
	virtual void Destroy();

protected:
	void AppPumpMessages();

	// Sets the video mode
	bool SetVideoMode( );

	// Returns the window
	void* GetAppWindow();

	// Gets the window size
	int GetWindowWidth() const;
	int GetWindowHeight() const;

private:
	// Returns the app name
	virtual const char *GetAppName() = 0;
	virtual const char *GetGameInfoName() { return NULL; }
	virtual bool AppUsesReadPixels() { return false; }

	// Creates the app window
	bool CreateAppWindow( char const *pTitle, bool bWindowed, int w, int h );

	// Sets up the game path
	bool SetupSearchPaths();

	void *m_HWnd;
	int m_nWidth;
	int m_nHeight;
};


#endif // VGUIMATSYSAPP_H
