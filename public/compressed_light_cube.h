//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef COMPRESSED_LIGHT_CUBE_H
#define COMPRESSED_LIGHT_CUBE_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib.h"


struct CompressedLightCube
{
	ColorRGBExp32 m_Color[6];
};


#endif // COMPRESSED_LIGHT_CUBE_H
