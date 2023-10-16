//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $NoKeywords: $
//===========================================================================//

#ifndef CONVAR_H
#define CONVAR_H

#if _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "Color.h"
#include "mathlib/vector4d.h"
#include "playerslot.h"

#ifdef _WIN32
#define FORCEINLINE_CVAR FORCEINLINE
#elif POSIX
#define FORCEINLINE_CVAR inline
#else
#error "implement me"
#endif


//-----------------------------------------------------------------------------
// Uncomment me to test for threading issues for material system convars
// NOTE: You want to disable all threading when you do this
// +host_thread_mode 0 +r_threaded_particles 0 +sv_parallel_packentities 0 +sv_disable_querycache 0
//-----------------------------------------------------------------------------
//#define CONVAR_TEST_MATERIAL_THREAD_CONVARS 1


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ConVar;
class CCommand;
class ConCommand;
class ConCommandBase;
class ConVarRefAbstract;
class ConCommandRefAbstract;
struct characterset_t;

class ALIGN8 ConVarHandle
{
public:
    bool IsValid() { return value != kInvalidConVarHandle; }
	uint32 Get() { return value; }
	void Set( uint32 _value ) { value = _value; }

private:
    uint32 value = kInvalidConVarHandle;

private:
	static const uint32 kInvalidConVarHandle = 0xFFFFFFFF;
} ALIGN8_POST;
enum CommandTarget_t
{
	CT_NO_TARGET = -1,
	CT_FIRST_SPLITSCREEN_CLIENT = 0,
	CT_LAST_SPLITSCREEN_CLIENT = 3,
};

class CCommandContext
{
public:
	CCommandContext(CommandTarget_t nTarget, CPlayerSlot nSlot) :
		m_nTarget(nTarget), m_nPlayerSlot(nSlot)
	{
	}

	CommandTarget_t GetTarget() const
	{
		return m_nTarget;
	}

	CPlayerSlot GetPlayerSlot() const
	{
		return m_nPlayerSlot;
	}

private:
	CommandTarget_t m_nTarget;
	CPlayerSlot m_nPlayerSlot;
};

class ALIGN8 ConCommandHandle
{
public:
    bool IsValid() { return value != kInvalidConCommandHandle; }
	uint16 Get() { return value; }
	void Set( uint16 _value ) { value = _value; }
	void Reset() { value = kInvalidConCommandHandle; }

	bool HasCallback() const;
	void Dispatch( const CCommandContext& context, const CCommand& command );

	void Unregister();

private:
    uint16 value = kInvalidConCommandHandle;

private:
	static const uint16 kInvalidConCommandHandle = 0xFFFF;
} ALIGN8_POST;

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
// ConVar flags
//-----------------------------------------------------------------------------
// The default, no flags at all
#define FCVAR_NONE				0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_LINKED_CONCOMMAND (1<<0)
#define FCVAR_DEVELOPMENTONLY	(1<<1)	// Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL			(1<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1<<3)  // defined by the client DLL
#define FCVAR_HIDDEN			(1<<4)	// Hidden. Doesn't appear in find or auto complete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED			(1<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY			(1<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1<<9)	// changes the client's info string

#define FCVAR_MISSING0	 		(1<<10) // Something that hides the cvar from the cvar lookups
#define FCVAR_UNLOGGED			(1<<11)  // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_MISSING1			(1<<12)  // Something that hides the cvar from the cvar lookups

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED		(1<<13)	// server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_CHEAT				(1<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats
#define FCVAR_PER_USER			(1<<15) // causes varnameN where N == 2 through max splitscreen slots for mod to be autogenerated
#define FCVAR_DEMO				(1<<16) // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1<<17) // don't record these command in demofiles
#define FCVAR_MISSING2			(1<<18)
#define FCVAR_RELEASE			(1<<19) // Cvars tagged with this are the only cvars avaliable to customers
#define FCVAR_MENUBAR_ITEM		(1<<20)
#define FCVAR_MISSING3			(1<<21)

#define FCVAR_NOT_CONNECTED		(1<<22)	// cvar cannot be changed by a client that is connected to a server
#define FCVAR_VCONSOLE_FUZZY_MATCHING (1<<23)

#define FCVAR_SERVER_CAN_EXECUTE	(1<<24) // the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_CLIENT_CAN_EXECUTE	(1<<25) // Assigned to commands to let clients execute them
#define FCVAR_SERVER_CANNOT_QUERY	(1<<26) // If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_VCONSOLE_SET_FOCUS	(1<<27)
#define FCVAR_CLIENTCMD_CAN_EXECUTE	(1<<28)	// IVEngineClient::ClientCmd is allowed to execute this command. 
											// Note: IVEngineClient::ClientCmd_Unrestricted can run any client command.

#define FCVAR_EXECUTE_PER_TICK		(1<<29)

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
	EConVarType_Qangle
};

union CVValue_t
{
	bool		m_bValue;
	short		m_i16Value;
	uint16		m_u16Value;
	int			m_i32Value;
	uint		m_u32Value;
	int64		m_i64Value;
	uint64		m_u64Value;
	float		m_flValue;
	double		m_dbValue;
	const char *m_szValue;
	Color		m_clrValue;
	Vector2D	m_vec2Value;
	Vector		m_vec3Value;
	Vector4D	m_vec4Value;
	QAngle		m_angValue;
};

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
//-----------------------------------------------------------------------------
typedef void(*FnChangeCallbackGlobal_t)(ConVarRefAbstract *cvar, CSplitScreenSlot nSlot, const char *pNewValue, const char *pOldValue);
typedef void(*FnChangeCallback_t)(ConVarRefAbstract *cvar, CSplitScreenSlot nSlot, CVValue_t *pNewValue, CVValue_t *pOldValue);

//-----------------------------------------------------------------------------
// ConVar & ConCommand creation listener callbacks
//-----------------------------------------------------------------------------
class ICVarListenerCallbacks
{
public:
	virtual void OnConVarCreated( ConVarRefAbstract *pNewCvar ) = 0;
	virtual void OnConCommandCreated( ConCommandRefAbstract *pNewCommand ) = 0;
};



//-----------------------------------------------------------------------------
// Called when a ConCommand needs to execute
//-----------------------------------------------------------------------------
typedef void ( *FnCommandCallback_t )( const CCommandContext &context, const CCommand &command );
typedef void ( *FnCommandCallbackNoContext_t )( const CCommand &command );
typedef void ( *FnCommandCallbackVoid_t )();

//-----------------------------------------------------------------------------
// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
//-----------------------------------------------------------------------------
typedef int(*FnCommandCompletionCallback)( const char *partial, CUtlVector< CUtlString > &commands );


//-----------------------------------------------------------------------------
// Interface version
//-----------------------------------------------------------------------------
class ICommandCallback
{
public:
	virtual void CommandCallback(const CCommandContext &context, const CCommand &command) = 0;
};

class ICommandCompletionCallback
{
public:
	virtual int  CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands ) = 0;
};

class ConCommandRefAbstract
{
public:
	ConCommandHandle handle;
};

//-----------------------------------------------------------------------------
// Purpose: The base console invoked command/cvar interface
//-----------------------------------------------------------------------------
class ConCommandBase
{
	friend class CCvar;
	friend class ConCommand;

protected:
						ConCommandBase( void );
public:

						~ConCommandBase( void );
	// Check flag
	bool				IsFlagSet( int64 flag ) const;
	// Set flag
	void				AddFlags( int64 flags );
	// Clear flag
	void				RemoveFlags( int64 flags );

	int64				GetFlags() const;

	// Return name of cvar
	const char			*GetName( void ) const;

	// Return help text for cvar
	const char			*GetHelpText( void ) const;

private:	
	// Static data
	const char 					*m_pszName;
	const char 					*m_pszHelpString;
	
	// ConVar flags
	int64						m_nFlags;
};


//-----------------------------------------------------------------------------
// Command tokenizer
//-----------------------------------------------------------------------------
class CCommand
{
public:
	CCommand();
	CCommand( int nArgC, const char **ppArgV );
	bool Tokenize( const char *pCommand, characterset_t *pBreakSet = NULL );
	void Reset();

	int ArgC() const;
	const char **ArgV() const;
	const char *ArgS() const;					// All args that occur after the 0th arg, in string form
	const char *GetCommandString() const;		// The entire command in string form, including the 0th arg
	const char *operator[]( int nIndex ) const;	// Gets at arguments
	const char *Arg( int nIndex ) const;		// Gets at arguments
	
	// Helper functions to parse arguments to commands.
	// 
	// Returns index of argument, or -1 if no such argument.
	int FindArg( const char *pName ) const;

	int FindArgInt( const char *pName, int nDefaultVal ) const;

	static int MaxCommandLength();
	static characterset_t* DefaultBreakSet();

private:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

    int m_nArgv0Size;
    CUtlVectorFixedGrowable<char, COMMAND_MAX_LENGTH> m_ArgSBuffer;
    CUtlVectorFixedGrowable<char, COMMAND_MAX_LENGTH> m_ArgvBuffer;
    CUtlVectorFixedGrowable<char*, COMMAND_MAX_ARGC> m_Args;
};

inline int CCommand::MaxCommandLength()
{
	return COMMAND_MAX_LENGTH - 1;
}

inline int CCommand::ArgC() const
{
	return m_Args.Count();
}

inline const char **CCommand::ArgV() const
{
	return ArgC() ? (const char**)m_Args.Base() : NULL;
}

inline const char *CCommand::ArgS() const
{
	return m_nArgv0Size ? (m_ArgSBuffer.Base() + m_nArgv0Size) : "";
}

inline const char *CCommand::GetCommandString() const
{
	return ArgC() ? m_ArgSBuffer.Base() : "";
}

inline const char *CCommand::Arg( int nIndex ) const
{
	// FIXME: Many command handlers appear to not be particularly careful
	// about checking for valid argc range. For now, we're going to
	// do the extra check and return an empty string if it's out of range
	if ( nIndex < 0 || nIndex >= ArgC() )
		return "";
	return m_Args[nIndex];
}

inline const char *CCommand::operator[]( int nIndex ) const
{
	return Arg( nIndex );
}


//-----------------------------------------------------------------------------
// Purpose: The console invoked command
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandBase
{
friend class CCvar;
friend class ConCommandHandle;

public:
	typedef ConCommandBase BaseClass;

	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallback_t callback,
		const char *pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallbackVoid_t callback,
		const char *pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract* pReferenceOut, const char* pName, FnCommandCallbackNoContext_t callback,
		const char* pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, ICommandCallback *pCallback,
		const char *pHelpString = 0, int64 flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0 );

	~ConCommand( void );

	// Used internally by OneTimeInit to initialize/shutdown
	void Init();
	void Shutdown();

	void Create( const char *pName, const char *pHelpString = 0,
		int64 flags = 0 );

	int AutoCompleteSuggest( const char *partial, CUtlVector< CUtlString > &commands );

	bool CanAutoComplete( void );

	inline ConCommandRefAbstract	*GetRef( void ) const
	{
		return m_pReference;
	}

	inline void SetHandle( ConCommandHandle hndl )
	{
		m_pReference->handle = hndl;
	}

private:
	// Call this function when executing the command
	class CallbackInfo_t
	{
	public:
		union {
			FnCommandCallback_t m_fnCommandCallback;
			FnCommandCallbackVoid_t m_fnVoidCommandCallback;
			FnCommandCallbackNoContext_t m_fnContextlessCommandCallback;
			ICommandCallback* m_pCommandCallback;
		};

		bool m_bUsingCommandCallbackInterface : 1;
		bool m_bHasVoidCommandCallback : 1;
		bool m_bHasContextlessCommandCallback : 1;
	};

	CallbackInfo_t m_Callback;

	// NOTE: To maintain backward compat, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

	union
	{
		FnCommandCompletionCallback	m_fnCompletionCallback;
		ICommandCompletionCallback* m_pCommandCompletionCallback;
	};

	bool m_bHasCompletionCallback : 1;
	bool m_bUsingCommandCompletionInterface : 1;
	
	ConCommandRefAbstract *m_pReference;
};


//-----------------------------------------------------------------------------
// Purpose: A console variable
//-----------------------------------------------------------------------------
class ConVar
{
friend class CCvar;
friend class ConVarRef;
friend class SplitScreenConVarRef;

public:
#ifdef CONVAR_WORK_FINISHED
						ConVar( const char *pName, const char *pDefaultValue, int64 flags = 0);

						ConVar( const char *pName, const char *pDefaultValue, int64 flags, 
									const char *pHelpString );
						ConVar( const char *pName, const char *pDefaultValue, int64 flags, 
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax );
						ConVar( const char *pName, const char *pDefaultValue, int64 flags, 
									const char *pHelpString, FnChangeCallback_t callback );
						ConVar( const char *pName, const char *pDefaultValue, int64 flags, 
									const char *pHelpString, bool bMin, float fMin, bool bMax, float fMax,
									FnChangeCallback_t callback );

						~ConVar( void );

	bool				IsFlagSet( int64 flag ) const;
	const char*			GetHelpText( void ) const;
	const char			*GetName( void ) const;
	// Return name of command (usually == GetName(), except in case of FCVAR_SS_ADDED vars
	const char			*GetBaseName( void ) const;
	int					GetSplitScreenPlayerSlot() const;

	void				AddFlags( int64 flags );
	int64				GetFlags() const;

	// Install a change callback (there shouldn't already be one....)
	void InstallChangeCallback( FnChangeCallback_t callback, bool bInvoke = true );
	void RemoveChangeCallback( FnChangeCallback_t callbackToRemove );

	int GetChangeCallbackCount() const { return m_pParent->m_fnChangeCallbacks.Count(); }
	FnChangeCallback_t GetChangeCallback( int slot ) const { return m_pParent->m_fnChangeCallbacks[ slot ]; }

	// Retrieve value
	FORCEINLINE_CVAR float			GetFloat( void ) const;
	FORCEINLINE_CVAR int			GetInt( void ) const;
	FORCEINLINE_CVAR Color			GetColor( void ) const;
	FORCEINLINE_CVAR bool			GetBool() const {  return !!GetInt(); }
	FORCEINLINE_CVAR char const	   *GetString( void ) const;

	// Compiler driven selection for template use
	template <typename T> T Get( void ) const;
	template <typename T> T Get( T * ) const;

	// Any function that allocates/frees memory needs to be virtual or else you'll have crashes
	//  from alloc/free across dll/exe boundaries.
	
	// These just call into the IConCommandBaseAccessor to check flags and set the var (which ends up calling InternalSetValue).
	void				SetValue( const char *value );
	void				SetValue( float value );
	void				SetValue( int value );
	void				SetValue( Color value );
	
	// Reset to default value
	void						Revert( void );

	// True if it has a min/max setting
	bool						HasMin() const;
	bool						HasMax() const;

	bool						GetMin( float& minVal ) const;
	bool						GetMax( float& maxVal ) const;

	float						GetMinValue() const;
	float						GetMaxValue() const;

	const char					*GetDefault( void ) const;


	FORCEINLINE_CVAR CVValue_t &GetRawValue()
	{
		return m_Value;
	}
	FORCEINLINE_CVAR const CVValue_t &GetRawValue() const
	{
		return m_Value;
	}

private:
	bool						InternalSetColorFromString( const char *value );
	// Called by CCvar when the value of a var is changing.
	void				InternalSetValue(const char *value);
	// For CVARs marked FCVAR_NEVER_AS_STRING
	void				InternalSetFloatValue( float fNewValue );
	void				InternalSetIntValue( int nValue );
	void				InternalSetColorValue( Color value );

	bool				ClampValue( float& value );
	void				ChangeStringValue( const char *tempVal, float flOldValue );

	void				Create( const char *pName, const char *pDefaultValue, int64 flags = 0,
									const char *pHelpString = 0, bool bMin = false, float fMin = 0.0,
									bool bMax = false, float fMax = false, FnChangeCallback_t callback = 0 );

	// Used internally by OneTimeInit to initialize.
	void				Init();



protected:
#if 0
	// Next ConVar in chain
	// Prior to register, it points to the next convar in the DLL.
	// Once registered, though, m_pNext is reset to point to the next
	// convar in the global list
	ConCommandBase* m_pNext;

	// Has the cvar been added to the global list?
	bool						m_bRegistered;

	// Static data
	const char* m_pszName;
	const char* m_pszHelpString;

	// ConVar flags
	int64						m_nFlags;

protected:
	// ConVars add themselves to this list for the executable. 
	// Then ConVar_Register runs through  all the console variables 
	// and registers them into a global list stored in vstdlib.dll
	static ConCommandBase* s_pConCommandBases;

	// ConVars in this executable use this 'global' to access values.
	static IConCommandBaseAccessor* s_pAccessor;
	// This either points to "this" or it points to the original declaration of a ConVar.
	// This allows ConVars to exist in separate modules, and they all use the first one to be declared.
	// m_pParent->m_pParent must equal m_pParent (ie: m_pParent must be the root, or original, ConVar).
	ConVar						*m_pParent;

	// Static data
	const char					*m_pszDefaultValue;
	
	CVValue_t					m_Value;

	// Min/Max values
	bool						m_bHasMin;
	float						m_fMinVal;
	bool						m_bHasMax;
	float						m_fMaxVal;
	
	// Call this function when ConVar changes
	CUtlVector< FnChangeCallback_t > m_fnChangeCallbacks;
#endif
#endif // CONVAR_WORK_FINISHED
	const char* m_pszName;
	CVValue_t* m_cvvDefaultValue;
	CVValue_t* m_cvvMinValue;
	CVValue_t* m_cvvMaxValue;
	const char* m_pszHelpString;
	EConVarType m_eVarType;

	// This gets copied from the ConVarDesc_t on creation
	short unk1;

	unsigned int timesChanged;
	int64 flags;
	unsigned int callback_index;

	// Used when setting default, max, min values from the ConVarDesc_t
	// although that's not the only place of usage
	// flags seems to be:
	// (1 << 0) Skip setting value to split screen slots and also something keyvalues related
	// (1 << 1) Skip setting default value
	// (1 << 2) Skip setting min/max values
	int allocation_flag_of_some_sort;

	CVValue_t** values;
};


#ifdef CONVAR_WORK_FINISHED
//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
// Output : float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVar::GetFloat( void ) const
{
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
// Output : int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVar::GetInt( void ) const 
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color
// Output : Color
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR Color ConVar::GetColor( void ) const 
{
	unsigned char *pColorElement = ((unsigned char *)&m_pParent->m_Value.m_nValue);
	return Color( pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3] );
}


//-----------------------------------------------------------------------------

template <> FORCEINLINE_CVAR float			ConVar::Get<float>( void ) const		{ return GetFloat(); }
template <> FORCEINLINE_CVAR int			ConVar::Get<int>( void ) const			{ return GetInt(); }
template <> FORCEINLINE_CVAR bool			ConVar::Get<bool>( void ) const			{ return GetBool(); }
template <> FORCEINLINE_CVAR const char *	ConVar::Get<const char *>( void ) const	{ return GetString(); }
template <> FORCEINLINE_CVAR float			ConVar::Get<float>( float *p ) const				{ return ( *p = GetFloat() ); }
template <> FORCEINLINE_CVAR int			ConVar::Get<int>( int *p ) const					{ return ( *p = GetInt() ); }
template <> FORCEINLINE_CVAR bool			ConVar::Get<bool>( bool *p ) const					{ return ( *p = GetBool() ); }
template <> FORCEINLINE_CVAR const char *	ConVar::Get<const char *>( char const **p ) const	{ return ( *p = GetString() ); }

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
// Output : const char *
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *ConVar::GetString( void ) const 
{
	if ( m_nFlags & FCVAR_NEVER_AS_STRING )
		return "FCVAR_NEVER_AS_STRING";
	
	char const *str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

class CSplitScreenAddedConVar : public ConVar
{
	typedef ConVar BaseClass;
public:
	CSplitScreenAddedConVar( int nSplitScreenSlot, const char *pName, const ConVar *pBaseVar ) :
		BaseClass
		( 
			pName, 
			pBaseVar->GetDefault(), 
			// Keep basevar flags, except remove _SS and add _SS_ADDED instead
			( pBaseVar->GetFlags() & ~FCVAR_SS ) | FCVAR_SS_ADDED, 
			pBaseVar->GetHelpText(), 
			pBaseVar->HasMin(),
			pBaseVar->GetMinValue(),
			pBaseVar->HasMax(),
			pBaseVar->GetMaxValue()
		),
		m_pBaseVar( pBaseVar ),
		m_nSplitScreenSlot( nSplitScreenSlot )
	{
		for ( int i = 0; i < pBaseVar->GetChangeCallbackCount(); ++i )
		{
			InstallChangeCallback( pBaseVar->GetChangeCallback( i ), false );
		}
		Assert( nSplitScreenSlot >= 1 );
		Assert( nSplitScreenSlot < MAX_SPLITSCREEN_CLIENTS );
		Assert( m_pBaseVar );
		Assert( IsFlagSet( FCVAR_SS_ADDED ) );
		Assert( !IsFlagSet( FCVAR_SS ) );
	}

	const ConVar *GetBaseVar() const;
	virtual const char *GetBaseName() const;
	void SetSplitScreenPlayerSlot( int nSlot );
	virtual int GetSplitScreenPlayerSlot() const;

protected:

	const ConVar	*m_pBaseVar;
	int		m_nSplitScreenSlot;
};

FORCEINLINE_CVAR const ConVar *CSplitScreenAddedConVar::GetBaseVar() const 
{ 
	Assert( m_pBaseVar );
	return m_pBaseVar; 
}

FORCEINLINE_CVAR const char *CSplitScreenAddedConVar::GetBaseName() const 
{ 
	Assert( m_pBaseVar );
	return m_pBaseVar->GetName(); 
}

FORCEINLINE_CVAR void CSplitScreenAddedConVar::SetSplitScreenPlayerSlot( int nSlot ) 
{ 
	m_nSplitScreenSlot = nSlot; 
}

FORCEINLINE_CVAR int CSplitScreenAddedConVar::GetSplitScreenPlayerSlot() const 
{ 
	return m_nSplitScreenSlot; 
}

#endif // CONVAR_WORK_FINISHED

//-----------------------------------------------------------------------------
// Used to read/write convars that already exist (replaces the FindVar method)
//-----------------------------------------------------------------------------
class ConVarRefAbstract
{
public:
#ifdef CONVAR_WORK_FINISHED
	ConVarRefAbstract( const char *pName );
	ConVarRefAbstract( const char *pName, bool bIgnoreMissing );
	ConVarRefAbstract( IConVar *pConVar );

	void Init( const char *pName, bool bIgnoreMissing );
	bool IsValid() const;
	bool IsFlagSet( int64 nFlags ) const;
	IConVar *GetLinkedConVar();

	// Get/Set value
	float GetFloat( void ) const;
	int GetInt( void ) const;
	Color GetColor( void ) const;
	bool GetBool() const { return !!GetInt(); }
	const char *GetString( void ) const;

	void SetValue( const char *pValue );
	void SetValue( float flValue );
	void SetValue( int nValue );
	void SetValue( Color value );
	void SetValue( bool bValue );

	const char *GetName() const;

	const char *GetDefault() const;

	const char *GetBaseName() const;

	int	GetSplitScreenPlayerSlot() const;

private:
#endif // CONVAR_WORK_FINISHED
	// High-speed method to read convar data
	ConVarHandle m_Handle;
	ConVar *m_pConVarState;
};


#ifdef CONVAR_WORK_FINISHED
//-----------------------------------------------------------------------------
// Did we find an existing convar of that name?
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR bool ConVarRefAbstract::IsFlagSet( int64 nFlags ) const
{
	return ( m_pConVar->IsFlagSet( nFlags ) != 0 );
}

FORCEINLINE_CVAR IConVar *ConVarRefAbstract::GetLinkedConVar()
{
	return m_pConVar;
}

FORCEINLINE_CVAR const char *ConVarRefAbstract::GetName() const
{
	return m_pConVar->GetName();
}

FORCEINLINE_CVAR const char *ConVarRefAbstract::GetBaseName() const
{
	return m_pConVar->GetBaseName();
}

FORCEINLINE_CVAR int ConVarRefAbstract::GetSplitScreenPlayerSlot() const
{
	return m_pConVar->GetSplitScreenPlayerSlot();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float ConVarRefAbstract::GetFloat( void ) const
{
	return m_pConVarState->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int ConVarRefAbstract::GetInt( void ) const
{
	return m_pConVarState->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR Color ConVarRefAbstract::GetColor( void ) const
{
	return m_pConVarState->GetColor();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *ConVarRefAbstract::GetString( void ) const
{
	Assert( !IsFlagSet( FCVAR_NEVER_AS_STRING ) );
	return m_pConVarState->m_Value.m_pszString;
}


FORCEINLINE_CVAR void ConVarRefAbstract::SetValue( const char *pValue )
{
	m_pConVar->SetValue( pValue );
}

FORCEINLINE_CVAR void ConVarRefAbstract::SetValue( float flValue )
{
	m_pConVar->SetValue( flValue );
}

FORCEINLINE_CVAR void ConVarRefAbstract::SetValue( int nValue )
{
	m_pConVar->SetValue( nValue );
}

FORCEINLINE_CVAR void ConVarRefAbstract::SetValue( Color value )
{
	m_pConVar->SetValue( value );
}

FORCEINLINE_CVAR void ConVarRefAbstract::SetValue( bool bValue )
{
	m_pConVar->SetValue( bValue ? 1 : 0 );
}

FORCEINLINE_CVAR const char *ConVarRefAbstract::GetDefault() const
{
	return m_pConVarState->m_pszDefaultValue;
}

#if 0
//-----------------------------------------------------------------------------
// Helper for referencing splitscreen convars (i.e., "name" and "name2")
//-----------------------------------------------------------------------------
class SplitScreenConVarRef
{
public:
	SplitScreenConVarRef( const char *pName );
	SplitScreenConVarRef( const char *pName, bool bIgnoreMissing );
	SplitScreenConVarRef( IConVar *pConVar );

	void Init( const char *pName, bool bIgnoreMissing );
	bool IsValid() const;
	bool IsFlagSet( int64 nFlags ) const;

	// Get/Set value
	float GetFloat( int nSlot ) const;
	int GetInt( int nSlot ) const;
	Color GetColor( int nSlot ) const;
	bool GetBool( int nSlot ) const { return !!GetInt( nSlot ); }
	const char *GetString( int nSlot  ) const;

	void SetValue( int nSlot, const char *pValue );
	void SetValue( int nSlot, float flValue );
	void SetValue( int nSlot, int nValue );
	void SetValue( int nSlot, Color value );
	void SetValue( int nSlot, bool bValue );

	const char *GetName( int nSlot ) const;

	const char *GetDefault() const;

	const char *GetBaseName() const;

private:
	struct cv_t
	{
		IConVar *m_pConVar;
		ConVar *m_pConVarState;
	};

	cv_t	m_Info[ MAX_SPLITSCREEN_CLIENTS ];
};

//-----------------------------------------------------------------------------
// Did we find an existing convar of that name?
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR bool SplitScreenConVarRef::IsFlagSet( int64 nFlags ) const
{
	return ( m_Info[ 0 ].m_pConVar->IsFlagSet( nFlags ) != 0 );
}

FORCEINLINE_CVAR const char *SplitScreenConVarRef::GetName( int nSlot ) const
{
	return m_Info[ nSlot ].m_pConVar->GetName();
}

FORCEINLINE_CVAR const char *SplitScreenConVarRef::GetBaseName() const
{
	return m_Info[ 0 ].m_pConVar->GetBaseName();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR float SplitScreenConVarRef::GetFloat( int nSlot ) const
{
	return m_Info[ nSlot ].m_pConVarState->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR int SplitScreenConVarRef::GetInt( int nSlot ) const 
{
	return m_Info[ nSlot ].m_pConVarState->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an int
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR Color SplitScreenConVarRef::GetColor( int nSlot ) const 
{
	return m_Info[ nSlot ].m_pConVarState->GetColor();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string, return "" for bogus string pointer, etc.
//-----------------------------------------------------------------------------
FORCEINLINE_CVAR const char *SplitScreenConVarRef::GetString( int nSlot ) const 
{
	Assert( !IsFlagSet( FCVAR_NEVER_AS_STRING ) );
	return m_Info[ nSlot ].m_pConVarState->m_Value.m_pszString;
}


FORCEINLINE_CVAR void SplitScreenConVarRef::SetValue( int nSlot, const char *pValue )
{
	m_Info[ nSlot ].m_pConVar->SetValue( pValue );
}

FORCEINLINE_CVAR void SplitScreenConVarRef::SetValue( int nSlot, float flValue )
{
	m_Info[ nSlot ].m_pConVar->SetValue( flValue );
}

FORCEINLINE_CVAR void SplitScreenConVarRef::SetValue( int nSlot, int nValue )
{
	m_Info[ nSlot ].m_pConVar->SetValue( nValue );
}

FORCEINLINE_CVAR void SplitScreenConVarRef::SetValue( int nSlot, Color value )
{
	m_Info[ nSlot ].m_pConVar->SetValue( value );
}

FORCEINLINE_CVAR void SplitScreenConVarRef::SetValue( int nSlot, bool bValue )
{
	m_Info[ nSlot ].m_pConVar->SetValue( bValue ? 1 : 0 );
}

FORCEINLINE_CVAR const char *SplitScreenConVarRef::GetDefault() const
{
	return m_Info[ 0 ].m_pConVarState->m_pszDefaultValue;
}
#endif
#endif // CONVAR_WORK_FINISHED

//-----------------------------------------------------------------------------
// Called by the framework to register ConVars and ConCommands with the ICVar
//-----------------------------------------------------------------------------
void ConVar_Register( int64 nCVarFlag = 0 );
void ConVar_Unregister( );


//-----------------------------------------------------------------------------
// Utility methods 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConCommandBase *pVar );


//-----------------------------------------------------------------------------
// Purpose: Utility class to quickly allow ConCommands to call member methods
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning (disable : 4355 )
#endif

template< class T >
class CConCommandMemberAccessor : public ConCommand, public ICommandCallback, public ICommandCompletionCallback
{
	typedef ConCommand BaseClass;
	typedef void ( T::*FnMemberCommandCallback_t )( const CCommandContext &context, const CCommand &command );
	typedef int  ( T::*FnMemberCommandCompletionCallback_t )( const char *pPartial, CUtlVector< CUtlString > &commands );

public:
	CConCommandMemberAccessor( T* pOwner, const char *pName, FnMemberCommandCallback_t callback, const char *pHelpString = 0,
		int64 flags = 0, FnMemberCommandCompletionCallback_t completionFunc = 0 ) :
		BaseClass( &m_ConCommandRef, pName, this, pHelpString, flags, ( completionFunc != 0 ) ? this : NULL )
	{
		m_pOwner = pOwner;
		m_Func = callback;
		m_CompletionFunc = completionFunc;
	}

	~CConCommandMemberAccessor()
	{
		Shutdown();
	}

	void SetOwner( T* pOwner )
	{
		m_pOwner = pOwner;
	}

	virtual void CommandCallback( const CCommandContext &context, const CCommand &command )
	{
		Assert( m_pOwner && m_Func );
		(m_pOwner->*m_Func)( context, command );
	}

	virtual int  CommandCompletionCallback( const char *pPartial, CUtlVector< CUtlString > &commands )
	{
		Assert( m_pOwner && m_CompletionFunc );
		return (m_pOwner->*m_CompletionFunc)( pPartial, commands );
	}

private:
	T* m_pOwner;
	FnMemberCommandCallback_t m_Func;
	FnMemberCommandCompletionCallback_t m_CompletionFunc;
	ConCommandRefAbstract m_ConCommandRef;
};

#ifdef _MSC_VER
#pragma warning ( default : 4355 )
#endif

//-----------------------------------------------------------------------------
// Purpose: Utility macros to quicky generate a simple console command
//-----------------------------------------------------------------------------
#define CON_COMMAND( name, description ) \
   static ConCommandRefAbstract name##_ref; \
   static void name##_callback( const CCommand &args ); \
   static ConCommand name##_command( &name##_ref, #name, name##_callback, description ); \
   static void name##_callback( const CCommand &args )
#ifdef CLIENT_DLL
	#define CON_COMMAND_SHARED( name, description ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( &name##_ref, #name "_client", name##_callback, description ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_SHARED( name, description ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( &name##_ref, #name, name##_callback, description ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_F( name, description, flags ) \
	static ConCommandRefAbstract name##_ref; \
	static void name##_callback( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( &name##_ref, #name, name##_callback, description, flags ); \
	static void name##_callback( const CCommandContext &context, const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( &name##_ref, #name "_client", name##_callback, description, flags ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( &name##_ref, #name, name##_callback, description, flags ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	static ConCommandRefAbstract name##_ref; \
	static void name##_callback( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( &name##_ref, #name, name##_callback, description, flags, completion ); \
	static void name##_callback( const CCommandContext &context, const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( name##_command, #name "_client", name##_callback, description, flags, completion ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static ConCommandRefAbstract name##_ref; \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( name##_command, #name, name##_callback, description, flags, completion ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_EXTERN( name, _funcname, description ) \
	static ConCommandRefAbstract name##_ref; \
	void _funcname( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( &name##_ref, #name, _funcname, description ); \
	void _funcname( const CCommandContext &context, const CCommand &args )

#define CON_COMMAND_EXTERN_F( name, _funcname, description, flags ) \
	static ConCommandRefAbstract name##_ref; \
	void _funcname( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( &name##_ref, #name, _funcname, description, flags ); \
	void _funcname( const CCommandContext &context, const CCommand &args )

#define CON_COMMAND_MEMBER_F( _thisclass, name, _funcname, description, flags ) \
	void _funcname( const CCommandContext &context, const CCommand &args );						\
	friend class CCommandMemberInitializer_##_funcname;			\
	class CCommandMemberInitializer_##_funcname					\
	{															\
	public:														\
		CCommandMemberInitializer_##_funcname() : m_ConCommandAccessor( NULL, name, &_thisclass::_funcname, description, flags )	\
		{														\
			m_ConCommandAccessor.SetOwner( GET_OUTER( _thisclass, m_##_funcname##_register ) );	\
		}														\
	private:													\
		CConCommandMemberAccessor< _thisclass > m_ConCommandAccessor;	\
	};															\
																\
	CCommandMemberInitializer_##_funcname m_##_funcname##_register;		\


#endif // CONVAR_H
