//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVTEX_H
#define IVTEX_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"


// This shipped with HL2.
class IVTex
{
public:
	virtual int VTex( int argc, char **argv ) = 0;
};

#define IVTEX_VERSION_STRING "VTEX001"


class IVTex3
{
public:
	virtual int VTex( CreateInterfaceFn filesystemFactory, const char *pGameDir, int argc, char **argv ) = 0;
};
#define IVTEX3_VERSION_STRING "VTEX3_001"


#endif // IVTEX_H
