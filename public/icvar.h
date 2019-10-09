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
#include "tier1/iconvar.h"
#include "tier1/utlvector.h"


class ConCommandBase;
class ConCommand;
class ConVar;
class Color;


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
	// Allocate a unique DLL identifier
	virtual CVarDLLIdentifier_t AllocateDLLIdentifier() = 0;

	// Register, unregister commands
	virtual void			RegisterConCommand( ConCommandBase *pCommandBase, bool cmdline=true ) = 0;
	virtual void			UnregisterConCommand( ConCommandBase *pCommandBase ) = 0;
	virtual void			UnregisterConCommands( CVarDLLIdentifier_t id ) = 0;

	// If there is a +<varname> <value> on the command line, this returns the value.
	// Otherwise, it returns NULL.
	inline const char*		GetCommandLineValue( const char *pVariableName );
	virtual bool			HasCommandLineValue( const char *pVariableName ) = 0;

	// Try to find the cvar pointer by name
	virtual ConCommandBase *FindCommandBase( const char *name ) = 0;
	virtual const ConCommandBase *FindCommandBase( const char *name ) const = 0;
	virtual ConVar			*FindVar ( const char *var_name ) = 0;
	virtual const ConVar	*FindVar ( const char *var_name ) const = 0;
	virtual ConCommand		*FindCommand( const char *name ) = 0;
	virtual const ConCommand *FindCommand( const char *name ) const = 0;



	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallback_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallback_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( ConVar *var, const char *pOldString, float flOldValue ) = 0;

	// Install a console printer
	virtual void			InstallConsoleDisplayFunc( IConsoleDisplayFunc* pDisplayFunc ) = 0;
	virtual void			RemoveConsoleDisplayFunc( IConsoleDisplayFunc* pDisplayFunc ) = 0;
	virtual void			ConsoleColorPrintf( const Color& clr, const char *pFormat, ... ) const = 0;
	virtual void			ConsolePrintf( const char *pFormat, ... ) const = 0;
	virtual void			ConsoleDPrintf( const char *pFormat, ... ) const = 0;

	// Reverts cvars which contain a specific flag
	virtual void			RevertFlaggedConVars( int nFlag ) = 0;

	// Method allowing the engine ICvarQuery interface to take over
	// A little hacky, owing to the fact the engine is loaded
	// well after ICVar, so we can't use the standard connect pattern
	virtual void			InstallCVarQuery( ICvarQuery *pQuery ) = 0;

#if defined( _X360 )
	virtual void			PublishToVXConsole( ) = 0;
#endif

	virtual void			SetMaxSplitScreenSlots( int nSlots ) = 0;
	virtual int				GetMaxSplitScreenSlots() const = 0;

	virtual void			AddSplitScreenConVars() = 0;
	virtual void			RemoveSplitScreenConVars( CVarDLLIdentifier_t id ) = 0;

	virtual int				GetConsoleDisplayFuncCount() const = 0;
	virtual void			GetConsoleText( int nDisplayFuncIndex, char *pchText, size_t bufSize ) const = 0;

	// Utilities for convars accessed by the material system thread
	virtual bool			IsMaterialThreadSetAllowed( ) const = 0;
	virtual void			QueueMaterialThreadSetValue( ConVar *pConVar, const char *pValue ) = 0;
	virtual void			QueueMaterialThreadSetValue( ConVar *pConVar, int nValue ) = 0;
	virtual void			QueueMaterialThreadSetValue( ConVar *pConVar, float flValue ) = 0;
	virtual bool			HasQueuedMaterialThreadConVarSets() const = 0;
	virtual int				ProcessQueuedMaterialThreadConVarSets() = 0;

protected:	class ICVarIteratorInternal;
public:
	/// Iteration over all cvars. 
	/// (THIS IS A SLOW OPERATION AND YOU SHOULD AVOID IT.)
	/// usage: 
	/// { ICVar::Iterator iter(g_pCVar); 
	///   for ( iter.SetFirst() ; iter.IsValid() ; iter.Next() )
	///   {  
	///       ConCommandBase *cmd = iter.Get();
	///   } 
	/// }
	/// The Iterator class actually wraps the internal factory methods
	/// so you don't need to worry about new/delete -- scope takes care
	//  of it.
	/// We need an iterator like this because we can't simply return a 
	/// pointer to the internal data type that contains the cvars -- 
	/// it's a custom, protected class with unusual semantics and is
	/// prone to change.
	class Iterator
	{
	public:
		inline Iterator(ICvar *icvar);
		inline ~Iterator(void);
		inline void		SetFirst( void );
		inline void		Next( void );
		inline bool		IsValid( void );
		inline ConCommandBase *Get( void );
	private:
		ICVarIteratorInternal *m_pIter;
	};

protected:
	// internals for  ICVarIterator
	class ICVarIteratorInternal
	{
	public:
		virtual void		SetFirst( void ) = 0;
		virtual void		Next( void ) = 0;
		virtual	bool		IsValid( void ) = 0;
		virtual ConCommandBase *Get( void ) = 0;
	};

	virtual ICVarIteratorInternal	*FactoryInternalIterator( void ) = 0;
	friend class Iterator;
};

inline ICvar::Iterator::Iterator(ICvar *icvar)
{
	m_pIter = icvar->FactoryInternalIterator();
}

inline ICvar::Iterator::~Iterator( void )
{
	g_pMemAlloc->Free(m_pIter);
}

inline void ICvar::Iterator::SetFirst( void )
{
	m_pIter->SetFirst();
}

inline void ICvar::Iterator::Next( void )
{
	m_pIter->Next();
}

inline bool ICvar::Iterator::IsValid( void )
{
	return m_pIter->IsValid();
}

inline ConCommandBase * ICvar::Iterator::Get( void )
{
	return m_pIter->Get();
}

inline const char *ICvar::GetCommandLineValue( const char *pVariableName )
{
	if (pVariableName[0] == '\0')
		return NULL;
	size_t len = strlen(pVariableName);
	char *search = new char[len + 2];
	search[0] = '+';
	memcpy(&search[1], pVariableName, len + 1);
	const char *value = CommandLine()->ParmValue(search);
	delete[] search;
	return value;
}


//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
