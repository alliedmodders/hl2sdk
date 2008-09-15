//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IGAMEUIFUNCS_H
#define IGAMEUIFUNCS_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/keycode.h"

abstract_class IGameUIFuncs
{
public:
	virtual bool		IsKeyDown( char const *keyname, bool& isdown ) = 0;
	virtual const char	*Key_NameForKey( int keynum ) = 0;
	virtual const char	*Key_BindingForKey( int keynum ) = 0;
	virtual vgui::KeyCode GetVGUI2KeyCodeForBind( const char *bind ) = 0;
	virtual void		GetVideoModes( struct vmode_s **liststart, int *count ) = 0;
	virtual void		SetFriendsID( uint friendsID, const char *friendsName ) = 0;
	virtual void		GetDesktopResolution( int &width, int &height ) = 0;
	virtual int			GetEngineKeyCodeForBind( const char *bind ) = 0;
	virtual bool		IsConnectedToVACSecureServer() = 0;
};

#define VENGINE_GAMEUIFUNCS_VERSION "VENGINE_GAMEUIFUNCS_VERSION004"

#endif // IGAMEUIFUNCS_H
