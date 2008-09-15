//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ICVAR_H
#define ICVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/convar.h"
#include "tier1/interface.h"
#include "appframework/IAppSystem.h"


//-----------------------------------------------------------------------------
// Purpose: DLL interface to ConVars/ConCommands
//-----------------------------------------------------------------------------
abstract_class ICvar : public IAppSystem
{
public:
	// Try to register cvar
	virtual void			RegisterConCommandBase ( ConCommandBase *variable ) = 0;

	// If there is a +<varname> <value> on the command line, this returns the value.
	// Otherwise, it returns NULL.
	virtual char const*		GetCommandLineValue( char const *pVariableName ) = 0;

	// Try to find the cvar pointer by name
	virtual ConVar			*FindVar ( const char *var_name ) = 0;
	virtual const ConVar	*FindVar ( const char *var_name ) const = 0;

	// Get first ConCommandBase to allow iteration
	virtual ConCommandBase	*GetCommands( void ) = 0;

	// Removes all cvars with flag bit set
	virtual void			UnlinkVariables( int flag ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallback callback ) = 0;
	virtual void			CallGlobalChangeCallback( ConVar *var, char const *pOldString ) = 0;
};

#define VENGINE_CVAR_INTERFACE_VERSION "VEngineCvar003"

extern ICvar *cvar;

DLL_EXPORT void SetCVarIF( ICvar *pCVarIF );
DLL_EXPORT ICvar *GetCVarIF( void );


#endif // ICVAR_H
