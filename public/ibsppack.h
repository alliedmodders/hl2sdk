//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IBSPPACK_H
#define IBSPPACK_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class IFileSystem;

abstract_class IBSPPack
{
public:
	virtual void LoadBSPFile( IFileSystem *pFileSystem, char *filename ) = 0;
	virtual void WriteBSPFile( char *filename ) = 0;
	virtual void ClearPackFile( void ) = 0;
	virtual void AddFileToPack( const char *relativename, const char *fullpath ) = 0;
	virtual void AddBufferToPack( const char *relativename, void *data, int length, bool bTextMode ) = 0;
	virtual void SetHDRMode( bool bHDR ) = 0;
};

#define IBSPPACK_VERSION_STRING "IBSPPACK003"

#endif // IBSPPACK_H
