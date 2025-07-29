//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
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
#include "tier1/characterset.h"
#include "utllinkedlist.h"
#include "utlhashtable.h"
#include "tier0/memalloc.h"
#include "convar.h"
#include <cstdint>

// Shorthand helper to iterate registered convars
// Example usage:
// FOR_EACH_CONVAR( iter )
// {
//     ConVarRefAbstract aref( iter );
//     Msg( "%s = %d\n", aref.GetName(), aref.GetInt() );
// 
//     /* or use typed version, but make sure to check its validity after,
//        since it would be invalid on type mismatch */
// 
//     CConVarRef<int> cref( iter );
//     if(cref.IsValidRef())
//         Msg( "%s = %d\n", cref.GetName(), cref.Get() );
// }
#define FOR_EACH_CONVAR( iter ) for(ConVarRef iter = g_pCVar->FindFirstConVar(); iter.IsValidRef(); iter = g_pCVar->FindNextConVar( iter ))

// Shorthand helper to iterate registered concommands
#define FOR_EACH_CONCOMMAND( iter ) for(ConCommandRef iter = icvar->FindFirstConCommand(); iter.IsValidRef(); iter = icvar->FindNextConCommand( iter ))


struct ConVarSnapshot_t;
class KeyValues;

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
//-----------------------------------------------------------------------------
typedef void(*FnChangeCallbackGlobal_t)(ConVarRefAbstract* ref, CSplitScreenSlot nSlot, const char *pNewValue, const char *pOldValue);

//-----------------------------------------------------------------------------
// ConVar & ConCommand creation listener callbacks
//-----------------------------------------------------------------------------
class ICVarListenerCallbacks
{
public:
	virtual void OnConVarCreated( ConVarRefAbstract *pNewCvar ) {};
	virtual void OnConCommandCreated( ConCommand *pNewCommand ) {};
};

//-----------------------------------------------------------------------------
// Purpose: DLL interface to ConVars/ConCommands
//-----------------------------------------------------------------------------
abstract_class ICvar : public IAppSystem
{
public:
	// allow_defensive - Allows finding convars with FCVAR_DEFENSIVE flag
	virtual ConVarRef		FindConVar( const char *name, bool allow_defensive = false ) = 0;
	virtual ConVarRef		FindFirstConVar() = 0;
	virtual ConVarRef		FindNextConVar( ConVarRef prev ) = 0;
	virtual void			CallChangeCallback( ConVarRef cvar, const CSplitScreenSlot nSlot, const CVValue_t* pNewValue, const CVValue_t* pOldValue ) = 0;

	// allow_defensive - Allows finding commands with FCVAR_DEFENSIVE flag
	virtual ConCommandRef	FindConCommand( const char *name, bool allow_defensive = false ) = 0;
	virtual ConCommandRef	FindFirstConCommand() = 0;
	virtual ConCommandRef	FindNextConCommand( ConCommandRef prev ) = 0;
	virtual void			DispatchConCommand( ConCommandRef cmd, const CCommandContext &ctx, const CCommand &args ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( ConVarRefAbstract* ref, CSplitScreenSlot nSlot, const char* newValue, const char* oldValue ) = 0;

	// Reverts cvars to default values which contain a specific flag,
	// cvars with a flag FCVAR_COMMANDLINE_ENFORCED would be skipped
	virtual void			ResetConVarsToDefaultValuesByFlag( uint64 nFlag ) = 0;

	virtual void			SetMaxSplitScreenSlots( int nSlots ) = 0;
	virtual int				GetMaxSplitScreenSlots() const = 0;

	virtual void			RegisterCreationListeners( ICVarListenerCallbacks *callbacks ) = 0;
	virtual void			RemoveCreationListeners( ICVarListenerCallbacks *callbacks ) = 0;

	virtual void			unk001() = 0;

	// Reverts cvars to default values which match pszPrefix string,
	// ignores FCVAR_COMMANDLINE_ENFORCED
	virtual void				ResetConVarsToDefaultValuesByName( const char *pszPrefix ) = 0;

	virtual ConVarSnapshot_t	*TakeConVarSnapshot( void ) = 0;
	virtual void				ResetConVarsToSnapshot( ConVarSnapshot_t *pSnapshot ) = 0;
	virtual void				DestroyConVarSnapshot( ConVarSnapshot_t *pSnapshot ) = 0;

	virtual characterset_t		*GetCharacterSet( void ) = 0;
	virtual void				SetConVarsFromGameInfo( KeyValues *pKV ) = 0;

	// Removes FCVAR_DEVELOPMENTONLY | FCVAR_DEFENSIVE from all cvars and concommands
	// that have FCVAR_DEFENSIVE set
	virtual void				StripDevelopmentFlags() = 0;

	// Register, unregister vars
	virtual void				RegisterConVar( const ConVarCreation_t& setup, uint64 nAdditionalFlags, ConVarRef* pCvarRef, ConVarData** pCvarData ) = 0;
	// Unregisters convar change callback, but leaves the convar in the lists,
	// so all ConVarRefs would still be valid as well as searching for it.
	// Expects ref to have registered index to be set (is set on convar creation)
	virtual void				UnregisterConVarCallbacks( ConVarRef cvar ) = 0;
	// Returns convar data or nullptr if not found
	virtual ConVarData*			GetConVarData( ConVarRef cvar ) = 0;

	// Register, unregister commands
	virtual ConCommandRef		RegisterConCommand( const ConCommandCreation_t& setup, uint64 nAdditionalFlags = 0 ) = 0;
	// Unregisters concommand callbacks, but leaves the command in the lists,
	// so all ConCommandRefs would still be valid as well as searching for it.
	// Expects ref to have registered index to be set (is set on command creation)
	virtual void				UnregisterConCommandCallbacks( ConCommandRef cmd ) = 0;
	// Returns command info or empty <unknown> command struct if not found, never nullptr
	virtual ConCommandData*		GetConCommandData( ConCommandRef cmd ) = 0;

	// Queues up value (creates a copy of it) to be set when convar is ready to be edited
	virtual void				QueueThreadSetValue( ConVarRefAbstract* ref, CSplitScreenSlot nSlot, CVValue_t* value ) = 0;
};

#include "memdbgon.h"

// AMNOTE: CCvar definition is mostly for reference and is reverse engineered
// You shouldn't be using this to directly register cvars/concommands, use its interface instead when possible
class CCvar : public ICvar
{
public:
	static const int kMemoryBufferChunkMaxSize = 8192;
	static const int kStringBufferChunkMaxSize = 2048;

	// Allocates memory in its internal buffer, can't be freed
	// (other than freeing internal buffers completely), so is leaky and thus use cautiously
	// AMNOTE: Mostly used for allocating CVValue_t and ConVarData/ConCommandData objects
	void *AllocateMemory( int size )
	{
		int aligned_size = ALIGN_VALUE( size, 8 );
		
		if(aligned_size + m_CurrentMemoryBufferSize > kMemoryBufferChunkMaxSize)
		{
			m_CurrentMemoryBufferSize = 0;
			*m_MemoryBuffer.AddToTailGetPtr() = (uint8 *)malloc( kMemoryBufferChunkMaxSize );
		}

		int offs = m_CurrentMemoryBufferSize;
		m_CurrentMemoryBufferSize += aligned_size;

		return &m_MemoryBuffer[m_MemoryBuffer.Count() - 1][offs];
	}

	// Allocates memory in its internal string buffer, can't be freed
	// (other than freeing internal buffers completely), so is leaky and thus use cautiously
	// AMNOTE: Mostly used for allocating cvar/concommand names
	const char *AllocateString( const char *string )
	{
		if(!string || !string[0])
			return "";

		int strlen = V_strlen( string ) + 1;

		if(strlen + m_CurrentStringsBufferSize > kStringBufferChunkMaxSize)
		{
			m_CurrentStringsBufferSize = 0;
			*m_StringsBuffer.AddToTailGetPtr() = (char *)malloc( kStringBufferChunkMaxSize );
		}

		int offs = m_CurrentStringsBufferSize;
		m_CurrentStringsBufferSize += strlen;

		char *base = &m_StringsBuffer[m_StringsBuffer.Count() - 1][offs];
		V_memmove( base, string, strlen );

		return base;
	}

	struct CConVarChangeCallbackNode_t
	{
		FnGenericChangeCallback_t m_pCallback;

		// Register index of cvar which change cb comes from
		int m_ConVarIndex;
	};

	struct ConCommandCallbackInfoNode_t
	{
		ConCommandCallbackInfo_t m_CB;

		// Register index of concommand which completion cb comes from
		int m_ConCmdIndex;
		// Index in a linkedlist of callbackinfos
		uint16 m_CallbackInfoIndex;
	};

	struct QueuedConVarSet_t
	{
		ConVarRefAbstract *m_ConVar;
		CSplitScreenSlot m_Slot;
		CVValue_t *m_Value;
	};

	CUtlVector<char *> m_StringsBuffer;
	CUtlVector<uint8 *> m_MemoryBuffer;
	int m_CurrentStringsBufferSize;
	int m_CurrentMemoryBufferSize;

	CUtlLinkedList<ConVarData *> m_ConVarList;
	CUtlHashtable<CUtlStringToken, uint16> m_ConVarHashes;
	CUtlLinkedList<CConVarChangeCallbackNode_t> m_ConVarChangeCBList;
	int m_ConVarCount;

	CUtlVector<ICVarListenerCallbacks *> m_CvarCreationListeners;
	CUtlVector<FnChangeCallbackGlobal_t> m_GlobalChangeCBList;

	CUtlLinkedList<ConCommandData> m_ConCommandList;
	CUtlHashtable<CUtlStringToken, uint16> m_ConCommandHashes;
	CUtlLinkedList<ConCommandCallbackInfoNode_t, unsigned short, true> m_CallbackInfoList;
	int m_ConCommandCount;

	int m_SplitScreenSlots;

	CThreadMutex m_Mutex;
	characterset_t m_CharacterSet;
	KeyValues *m_GameInfoKV;

	uint8 m_CvarDefaultValues[EConVarType_MAX][128];

	CUtlVector<QueuedConVarSet_t> m_SetValueQueue;

	CConCommandMemberAccessor<CCvar> m_FindCmd;
	CConCommandMemberAccessor<CCvar> m_DumpChannelsCmd;
	CConCommandMemberAccessor<CCvar> m_LogLevelCmd;
	CConCommandMemberAccessor<CCvar> m_LogVerbosityCmd;
	CConCommandMemberAccessor<CCvar> m_LogColorCmd;
	CConCommandMemberAccessor<CCvar> m_LogFlagsCmd;
	CConCommandMemberAccessor<CCvar> m_DifferencesCmd;
	CConCommandMemberAccessor<CCvar> m_CvarListCmd;
	CConCommandMemberAccessor<CCvar> m_HelpCmd;
	CConCommandMemberAccessor<CCvar> m_FindFlagsCmd;
};

#include "memdbgoff.h"

//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
