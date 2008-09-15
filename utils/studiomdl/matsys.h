//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MATSYS_H
#define MATSYS_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/IMaterialSystem.h"

extern IMaterialSystem *g_pMaterialSystem;
extern CreateInterfaceFn g_MatSysFactory;
extern CreateInterfaceFn g_ShaderAPIFactory;

void InitMaterialSystem( const char *materialBaseDirPath );

#endif // MATSYS_H
