//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef ICVAR_H
#define ICVAR_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "tier1/convar.h"
#include "tier1/utlvector.h"
#include "tier0/memalloc.h"

class ConCommandBase;
class ConCommand;
class ConVar;
class Color;
class IConVarListener;
class CConVarDetail;
struct ConVarSnapshot_t;
union CVValue_t;
class KeyValues;
class ConVarRefAbstract;


//-----------------------------------------------------------------------------
// Purpose: DLL interface to ConVars/ConCommands
//-----------------------------------------------------------------------------
abstract_class ICvar : public IAppSystem
{
public:
	// bAllowDeveloper - Allows finding convars with FCVAR_DEVELOPMENTONLY flag
	virtual ConVarHandle	FindConVar( const char *name, bool bAllowDeveloper = false ) = 0;
	virtual ConVarHandle	FindFirstConVar() = 0;
	virtual ConVarHandle	FindNextConVar( ConVarHandle prev ) = 0;
	virtual void			SetConVarValue( ConVarHandle cvarid, CSplitScreenSlot nSlot, CVValue_t *pNewValue, CVValue_t *pOldValue ) = 0;

	virtual ConCommandHandle	FindCommand( const char *name ) = 0;
	virtual ConCommandHandle	FindFirstCommand() = 0;
	virtual ConCommandHandle	FindNextCommand( ConCommandHandle prev ) = 0;
	virtual void				DispatchConCommand( ConCommandHandle cmd, const CCommandContext &ctx, const CCommand &args ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( ConVarRefAbstract *var, CSplitScreenSlot nSlot, const char *pOldString, float flOldValue ) = 0;

	// Reverts cvars which contain a specific flag
	virtual void			RevertFlaggedConVars( int nFlag ) = 0;

	virtual void			SetMaxSplitScreenSlots( int nSlots ) = 0;
	virtual int				GetMaxSplitScreenSlots() const = 0;

	virtual void	RegisterCreationListeners( ICVarListenerCallbacks *callbacks ) = 0;
	virtual void	RemoveCreationListeners( ICVarListenerCallbacks *callbacks ) = 0;

	virtual void	unk1() = 0;

	virtual void	ResetConVarsToDefaultValues( const char *pszPrefix ) = 0;

	virtual ConVarSnapshot_t	*TakeConVarSnapshot( void ) = 0;
	virtual void				ResetConVarsToSnapshot( ConVarSnapshot_t *pSnapshot ) = 0;
	virtual void				DestroyConVarSnapshot( ConVarSnapshot_t *pSnaoshot ) = 0;

	virtual characterset_t	GetCharacterSet( void ) = 0;
	virtual void			SetConVarsFromGameInfo( KeyValues *pKV ) = 0;

	virtual void	unk2() = 0;

	// Register, unregister vars
	virtual void	RegisterConVar( ConVar *pConVar, int64 nAdditionalFlags, ConVarHandle &pCvarRef, ConVar &pCvar ) = 0;
	virtual void	UnregisterConVar( ConVarHandle handle ) = 0;
	virtual ConVar*	GetConVar( ConVarHandle handle ) = 0;

	// Register, unregister commands
	virtual ConCommandHandle	RegisterConCommand( ConCommand *pCmd, int64 nAdditionalFlags = 0 ) = 0;
	virtual void				UnregisterConCommand( ConCommandHandle handle ) = 0;
	virtual ConCommand*			GetCommand( ConCommandHandle handle ) = 0;

	virtual void QueueThreadSetValue( ConVarRefAbstract *ref, CSplitScreenSlot nSlot, CVValue_t *value ) = 0;
};

//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
