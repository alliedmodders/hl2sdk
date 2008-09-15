//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: abstract system dependent functions
//
// $NoKeywords: $
//=============================================================================//
#include "sys.h"
#include <windows.h>


void Sys_CopyStringToClipboard( const char *pOut )
{
	if ( !pOut || !OpenClipboard( NULL ) )
	{
		return;
	}
	// Remove the current Clipboard contents
	if( !EmptyClipboard() )
	{
		return;
	}
	HGLOBAL clipbuffer;
	char *buffer;
	EmptyClipboard();
	
	clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(pOut)+1);
	buffer = (char*)GlobalLock( clipbuffer );
	strcpy( buffer, pOut );
	GlobalUnlock( clipbuffer );

	SetClipboardData( CF_TEXT,clipbuffer );

	CloseClipboard();
}

