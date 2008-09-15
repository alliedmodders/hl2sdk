//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RESOURCETAGS_H
#define RESOURCETAGS_H
#ifdef _WIN32
#pragma once
#endif

// resources loaded can be marked with tags
// tagged resources are subject to context purging
#define RESOURCE_TAG_NONE		0x00	// no owning context
#define RESOURCE_TAG_SYSTEM		0x01	// a system resource, generally critical 
#define RESOURCE_TAG_STARTUP	0x02	// part of the startup
#define RESOURCE_TAG_MAINMENU	0x04	// part of the main menu ui
#define RESOURCE_TAG_INGAMEMENU	0x08	// part of the ui at pause
#define RESOURCE_TAG_MAP		0x10	// map specific

extern void	Sys_SetResourceTag( int tag );
extern int	Sys_GetResourceTag( void );

#endif
