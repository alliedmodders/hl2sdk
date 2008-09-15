//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PERFSTATS_H
#define PERFSTATS_H
#ifdef _WIN32
#pragma once
#endif

#include "studio.h"
#include "optimize.h"

void SpewPerfStats( studiohdr_t *pStudioHdr, const char *pFilename );

#endif // PERFSTATS_H
