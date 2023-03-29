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
#include "tier1/utlvector.h"
#include "tier0/memalloc.h"
#include "convar.h"

class ConVarSnapshot_t;
class KeyValues;

//-----------------------------------------------------------------------------
// ConVars/ComCommands are marked as having a particular DLL identifier
//-----------------------------------------------------------------------------
typedef int CVarDLLIdentifier_t;


//-----------------------------------------------------------------------------
// Used to display console messages
//-----------------------------------------------------------------------------
abstract_class IConsoleDisplayFunc
{
public:
	virtual void ColorPrint( const Color& clr, const char *pMessage ) = 0;
	virtual void Print( const char *pMessage ) = 0;
	virtual void DPrint( const char *pMessage ) = 0;

	virtual void GetConsoleText( char *pchText, size_t bufSize ) const = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Applications can implement this to modify behavior in ICvar
//-----------------------------------------------------------------------------
#define CVAR_QUERY_INTERFACE_VERSION "VCvarQuery001"
abstract_class ICvarQuery : public IAppSystem
{
public:
	// Can these two convars be aliased?
	virtual bool AreConVarsLinkable( const ConVar *child, const ConVar *parent ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: DLL interface to ConVars/ConCommands
//-----------------------------------------------------------------------------
abstract_class ICvar : public IAppSystem
{
public:
	// allow_developer - Allows finding convars with FCVAR_DEVELOPMENTONLY flag
	virtual ConVarID FindConVar(const char *szName, bool bAllowDeveloper = false) = 0;
	virtual ConVarID FindFirstConVar() = 0;
	virtual ConVarID FindNextConVar(ConVarID previous) = 0;

	virtual void SetConVarValue(ConVarID cvarid, CSplitScreenSlot nSlot, CVValue_t *pNewValue, CVValue_t *pOldValue) = 0;

	virtual ConCommandID FindCommand(const char *szName) = 0;
	virtual ConCommandID FindFirstCommand() = 0;
	virtual ConCommandID FindNextCommand(ConCommandID previous) = 0;

	virtual void DispatchConCommand(ConCommandID commandid, CCommandContext& ctx, CCommand& tok) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void InstallGlobalChangeCallback(FnChangeCallbackGlobal_t callback) = 0;
	virtual void RemoveGlobalChangeCallback(FnChangeCallbackGlobal_t callback) = 0;
	virtual void CallGlobalChangeCallbacks(ConVarRefAbstract *cvar, CSplitScreenSlot nSlot, const char *pNewValue, const char* pOldValue) = 0;

	// Reverts cvars which contain a specific flag
	virtual void RevertFlaggedConVars(int nFlag) = 0;

	virtual void SetMaxSplitScreenSlots(int nSlots) = 0;
	virtual int	GetMaxSplitScreenSlots() const = 0;

	virtual void RegisterCreationListeners(CCreationListenerCallbacks *callbacks) = 0;
	virtual void RemoveCreationListeners(CCreationListenerCallbacks *callbacks) = 0;

	virtual void unk1() = 0;

	virtual void ResetConVarsToDefaultValues(const char *pszSearchString) = 0;

	virtual ConVarSnapshot_t TakeConVarSnapshot() = 0;
	virtual void ResetConVarsFromSnapshot(ConVarSnapshot_t *snapshot) = 0;
	virtual void DestroyConVarSnapshot(ConVarSnapshot_t *snapshot) = 0;

	virtual characterset_t GetCharacterSet() = 0;

	virtual void SetConVarsFromGameInfo(KeyValues *) = 0;

	virtual void RegisterConVar(ConVarDesc_t*, void*, ConVarID&, ConVar&) = 0;
	virtual void UnregisterConVar(ConVarID cvarid) = 0;
	virtual ConVar* GetConVar(ConVarID cvarid) = 0;

	virtual void RegisterConCommand(void*, void*, ConCommandID&, ConCommand&) = 0;
	virtual void UnregisterConCommand(ConCommandID commandid) = 0;
	virtual ConCommand* GetConCommand(ConCommandID commandid) = 0;

	virtual void QueueThreadSetValue(ConVarRefAbstract *ref, CSplitScreenSlot nSlot, CVValue_t *value) = 0;
};

//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
