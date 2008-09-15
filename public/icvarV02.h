//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ICVAR_V02_H
#define ICVAR_V02_H
#ifdef _WIN32
#pragma once
#endif

#include "icvar.h"


//-----------------------------------------------------------------------------
// NOTE: Do not change this!! This is the version of the ICVar interface at ship 10/15/04
//-----------------------------------------------------------------------------
#ifndef _XBOX
abstract_class ICvarV02
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
};
#endif

#define VENGINE_CVAR_INTERFACE_VERSION_2 "VEngineCvar002"

//-----------------------------------------------------------------------------
// Connector interface class to deal with backward compat.
//-----------------------------------------------------------------------------
#ifndef _XBOX
class CCvarV02 : public ICvarV02
{
public:
	CCvarV02( ICvar *pCVar ) : m_pCVar( pCVar ) {}

	// Methods of ICvarV02
	virtual void RegisterConCommandBase ( ConCommandBase *variable )
	{
		m_pCVar->RegisterConCommandBase( variable );
	}

	virtual char const* GetCommandLineValue( char const *pVariableName )
	{
		return m_pCVar->GetCommandLineValue( pVariableName );
	}

	virtual ConVar *FindVar ( const char *var_name )
	{
		return m_pCVar->FindVar( var_name );
	}

	virtual const ConVar *FindVar ( const char *var_name ) const
	{
		return m_pCVar->FindVar( var_name );
	}

	virtual ConCommandBase *GetCommands( void )
	{
		return m_pCVar->GetCommands( );
	}


private:
	ICvar *m_pCVar;
};
#endif

#endif // ICVAR_V02_H
