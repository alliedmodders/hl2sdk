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
#include "tier0/memalloc.h"
#include <cstdint>

class ConCommand;
class CCommand;
class CCommandContext;
class ICVarListenerCallbacks;
class IConVarListener;
class ConVar;
struct ConVarCreation_t;
struct ConCommandCreation_t;
struct ConVarSnapshot_t;
union CVValue_t;
class KeyValues;

struct CSplitScreenSlot
{
	CSplitScreenSlot( int index )
	{
		m_Data = index;
	}
	
	int Get() const
	{
		return m_Data;
	}
	
	int m_Data;
};

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
//-----------------------------------------------------------------------------
typedef void(*FnChangeCallbackGlobal_t)(ConVar* cvar, CSplitScreenSlot nSlot, const char *pNewValue, const char *pOldValue);
using FnChangeCallback_t = void(*)(ConVar* cvar, CSplitScreenSlot nSlot, CVValue_t *pNewValue, CVValue_t *pOldValue);
static_assert(sizeof(FnChangeCallback_t) == 0x8, "Wrong size for FnChangeCallback_t");

//-----------------------------------------------------------------------------
// Purpose: Replaces the ConVar*
//-----------------------------------------------------------------------------
class ConVarHandle
{
public:
	ConVarHandle( uint16_t convarIndex = -1, uint16_t unk = -1, uint32_t handle = -1) :
	m_convarIndex(convarIndex),
	m_unknown1(unk),
	m_handleIndex(handle)
	{}

	bool IsValid( ) const;
	void Invalidate( );

	uint16_t GetConVarIndex() const { return m_convarIndex; }
	uint32_t GetIndex() const { return m_handleIndex; }

private:
	uint16_t m_convarIndex;
	uint16_t m_unknown1;
	uint32_t m_handleIndex;
};
static_assert(sizeof(ConVarHandle) == 0x8, "ConVarHandle is of the wrong size!");

static const ConVarHandle INVALID_CONVAR_HANDLE = ConVarHandle();

inline bool ConVarHandle::IsValid() const
{
	return m_convarIndex != 0xFFFF;
}

inline void ConVarHandle::Invalidate()
{
	m_convarIndex = 0xFFFF;
	m_unknown1 = 0xFFFF;
	m_handleIndex = 0x0;
}

//-----------------------------------------------------------------------------
// Purpose: Internal structure of ConVar objects
//-----------------------------------------------------------------------------
enum EConVarType : short
{
	EConVarType_Invalid = -1,
	EConVarType_Bool,
	EConVarType_Int16,
	EConVarType_UInt16,
	EConVarType_Int32,
	EConVarType_UInt32,
	EConVarType_Int64,
	EConVarType_UInt64,
	EConVarType_Float32,
	EConVarType_Float64,
	EConVarType_String,
	EConVarType_Color,
	EConVarType_Vector2,
	EConVarType_Vector3,
	EConVarType_Vector4,
	EConVarType_Qangle,
	EConVarType_MAX
};

abstract_class IConVar
{
friend class ConVar;
protected:
	const char* m_pszName;
	CVValue_t* m_cvvDefaultValue;
	CVValue_t* m_cvvMinValue;
	CVValue_t* m_cvvMaxValue;
	const char* m_pszHelpString;
	EConVarType m_eVarType;

	// This gets copied from the ConVarDesc_t on creation
	short unk1;

	unsigned int timesChanged;
	int64 m_flags;
	unsigned int callback_index;

	// Used when setting default, max, min values from the ConVarDesc_t
	// although that's not the only place of usage
	// flags seems to be:
	// (1 << 0) Skip setting value to split screen slots and also something keyvalues related
	// (1 << 1) Skip setting default value
	// (1 << 2) Skip setting min/max values
	int allocation_flag_of_some_sort;

	CVValue_t* m_value[4];
};

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

	virtual ConVarHandle	FindCommand( const char *name ) = 0;
	virtual ConVarHandle	FindFirstCommand() = 0;
	virtual ConVarHandle	FindNextCommand( ConVarHandle prev ) = 0;
	virtual void			DispatchConCommand( ConVarHandle cmd, const CCommandContext &ctx, const CCommand &args ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( ConVar* var, CSplitScreenSlot nSlot, const char *pOldString, float flOldValue ) = 0;

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
	virtual void		RegisterConVar( const ConVarCreation_t& setup, int64 nAdditionalFlags, ConVarHandle* pCvarRef, IConVar** pCvar ) = 0;
	virtual void		UnregisterConVar( ConVarHandle handle ) = 0;
	virtual IConVar*	GetConVar( ConVarHandle handle ) = 0;

	// Register, unregister commands
	virtual ConVarHandle		RegisterConCommand( const ConCommandCreation_t& setup, int64 nAdditionalFlags = 0 ) = 0;
	virtual void				UnregisterConCommand( ConVarHandle handle ) = 0;
	virtual ConCommand*			GetCommand( ConVarHandle handle ) = 0;

	virtual void QueueThreadSetValue( ConVar* ref, CSplitScreenSlot nSlot, CVValue_t* value ) = 0;
};

//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
