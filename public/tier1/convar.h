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
#include "utlcommon.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "mathlib/vector4d.h"
#include "bufferstring.h"
#include "tier1/characterset.h"
#include "Color.h"
#include "playerslot.h"

#include <cstdint>
#include <cinttypes>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CCommand;
class ConCommand;
class CCommandContext;
class ConVarRefAbstract;

struct CSplitScreenSlot
{
	CSplitScreenSlot() :
		m_Data(0)
	{}
	
	CSplitScreenSlot( int index )
	{
		m_Data = index;
	}
	
	int Get() const
	{
		return m_Data;
	}

	operator int() const
	{
		return m_Data;
	}
	
	int m_Data;
};

//-----------------------------------------------------------------------------
// Purpose: Internal structure of ConVar objects
//-----------------------------------------------------------------------------
enum EConVarType : int16_t
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

template<typename T>
constexpr EConVarType TranslateConVarType();

template<> constexpr EConVarType TranslateConVarType<bool>( void )		{ return EConVarType_Bool; }
template<> constexpr EConVarType TranslateConVarType<int16>( void )		{ return EConVarType_Int16; }
template<> constexpr EConVarType TranslateConVarType<uint16>( void )	{ return EConVarType_UInt16; }
template<> constexpr EConVarType TranslateConVarType<int32>( void )		{ return EConVarType_Int32; }
template<> constexpr EConVarType TranslateConVarType<uint32>( void )	{ return EConVarType_UInt32; }
template<> constexpr EConVarType TranslateConVarType<int64>( void )		{ return EConVarType_Int64; }
template<> constexpr EConVarType TranslateConVarType<uint64>( void )	{ return EConVarType_UInt64; }
template<> constexpr EConVarType TranslateConVarType<float32>( void )	{ return EConVarType_Float32; }
template<> constexpr EConVarType TranslateConVarType<float64>( void )	{ return EConVarType_Float64; }
template<> constexpr EConVarType TranslateConVarType<CUtlString>( void ){ return EConVarType_String; }
template<> constexpr EConVarType TranslateConVarType<Color>( void )		{ return EConVarType_Color; }
template<> constexpr EConVarType TranslateConVarType<Vector2D>( void )	{ return EConVarType_Vector2; }
template<> constexpr EConVarType TranslateConVarType<Vector>( void )	{ return EConVarType_Vector3; }
template<> constexpr EConVarType TranslateConVarType<Vector4D>( void )	{ return EConVarType_Vector4; }
template<> constexpr EConVarType TranslateConVarType<QAngle>( void )	{ return EConVarType_Qangle; }
template<> constexpr EConVarType TranslateConVarType<void*>( void )		{ return EConVarType_Invalid; }

union CVValue_t
{
	static CVValue_t *InvalidValue()
	{
		static uint8 s_Data[sizeof( CVValue_t )] = {};
		return (CVValue_t *)&s_Data;
	}

	CVValue_t() {}
	CVValue_t( const bool value ) : m_bValue( value ) {}
	CVValue_t( const int16 value ) : m_i16Value( value ) {}
	CVValue_t( const uint16 value ) : m_u16Value( value ) {}
	CVValue_t( const int32 value ) : m_i32Value( value ) {}
	CVValue_t( const uint32 value ) : m_u32Value( value ) {}
	CVValue_t( const int64 value ) : m_i64Value( value ) {}
	CVValue_t( const uint64 value ) : m_u64Value( value ) {}
	CVValue_t( const float32 value ) : m_fl32Value( value ) {}
	CVValue_t( const float64 value ) : m_fl64Value( value ) {}
	CVValue_t( const CUtlString &value ) : m_StringValue( value ) {}
	CVValue_t( const Color &value ) : m_clrValue( value ) {}
	CVValue_t( const Vector2D &value ) : m_vec2Value( value ) {}
	CVValue_t( const Vector &value ) : m_vec3Value( value ) {}
	CVValue_t( const Vector4D &value ) : m_vec4Value( value ) {}
	CVValue_t( const QAngle &value ) : m_angValue( value ) {}
	~CVValue_t() {}

	operator bool() const				{ return m_bValue; }
	operator int16() const				{ return m_i16Value; }
	operator uint16() const				{ return m_u16Value; }
	operator int32() const				{ return m_i32Value; }
	operator uint32() const				{ return m_u32Value; }
	operator int64() const				{ return m_i64Value; }
	operator uint64() const				{ return m_u64Value; }
	operator float32() const			{ return m_fl32Value; }
	operator float64() const			{ return m_fl64Value; }
	operator const char*() const		{ return m_StringValue.Get(); }
	operator const CUtlString&() const	{ return m_StringValue; }
	operator const Color&() const		{ return m_clrValue; }
	operator const Vector2D&() const	{ return m_vec2Value; }
	operator const Vector&() const		{ return m_vec3Value; }
	operator const Vector4D&() const	{ return m_vec4Value; }
	operator const QAngle&() const 		{ return m_angValue; }

	bool		m_bValue;
	int16		m_i16Value;
	uint16		m_u16Value;
	int32		m_i32Value;
	uint32		m_u32Value;
	int64		m_i64Value;
	uint64		m_u64Value;
	float32		m_fl32Value;
	float64		m_fl64Value;
	CUtlString	m_StringValue;
	Color		m_clrValue;
	Vector2D	m_vec2Value;
	Vector		m_vec3Value;
	Vector4D	m_vec4Value;
	QAngle		m_angValue;
};

struct CVarCreationBase_t
{
	CVarCreationBase_t() :
		m_pszName( nullptr ),
		m_pszHelpString( nullptr ),
		m_nFlags( 0 )
	{}

	const char*				m_pszName;
	const char*				m_pszHelpString;
	uint64					m_nFlags;
};

//-----------------------------------------------------------------------------
// ConVar flags
//-----------------------------------------------------------------------------
// The default, no flags at all
#define FCVAR_NONE				0 

// Command to ConVars and ConCommands
// ConVar Systems
#define FCVAR_LINKED_CONCOMMAND (1ull<<0)	// Allows concommand callback chaining. When command is dispatched all chained callbacks would fire.
#define FCVAR_DEVELOPMENTONLY	(1ull<<1)	// Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL			(1ull<<2)	// defined by the game DLL
#define FCVAR_CLIENTDLL			(1ull<<3)  // defined by the client DLL
#define FCVAR_HIDDEN			(1ull<<4)	// Hidden. Doesn't appear in find or auto complete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED			(1ull<<5)  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define FCVAR_SPONLY			(1ull<<6)  // This cvar cannot be changed by clients connected to a multiplayer server.
#define	FCVAR_ARCHIVE			(1ull<<7)	// set to cause it to be saved to vars.rc
#define	FCVAR_NOTIFY			(1ull<<8)	// notifies players when changed
#define	FCVAR_USERINFO			(1ull<<9)	// changes the client's info string

#define FCVAR_REFERENCE			(1ull<<10) // Means cvar is a reference, usually used to get a cvar reference of a cvar registered in other module,
										// and is temporary until the actual cvar was registered
#define FCVAR_UNLOGGED			(1ull<<11)  // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_INITIAL_SETVALUE	(1ull<<12)	// Is set for a first convar SetValue either with its default_value or with a value from a gameinfo.
										// Mostly for callbacks to check for.

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED		(1ull<<13)	// server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_CHEAT				(1ull<<14) // Only useable in singleplayer / debug / multiplayer & sv_cheats
#define FCVAR_PER_USER			(1ull<<15) // causes varnameN where N == 2 through max splitscreen slots for mod to be autogenerated
#define FCVAR_DEMO				(1ull<<16) // record this cvar when starting a demo file
#define FCVAR_DONTRECORD		(1ull<<17) // don't record these command in demofiles
#define FCVAR_PERFORMING_CALLBACKS (1ull<<18)	// Is set when cvar is calling to its callbacks from CallChangeCallback or CallGlobalChangeCallbacks,
											// usually means if you set the value during these callbacks, its value would be queued and set when all callbacks were fired
#define FCVAR_RELEASE			(1ull<<19) // Cvars tagged with this are the only cvars avaliable to customers
#define FCVAR_MENUBAR_ITEM		(1ull<<20)
#define FCVAR_COMMANDLINE_ENFORCED	(1ull<<21) // If cvar was set via launch options it would not be reset on calls to ResetConVarsToDefaultValuesByFlag

#define FCVAR_NOT_CONNECTED		(1ull<<22)	// cvar cannot be changed by a client that is connected to a server
#define FCVAR_VCONSOLE_FUZZY_MATCHING (1ull<<23)

#define FCVAR_SERVER_CAN_EXECUTE	(1ull<<24) // the server is allowed to execute this command on clients via ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_CLIENT_CAN_EXECUTE	(1ull<<25) // Assigned to commands to let clients execute them
#define FCVAR_SERVER_CANNOT_QUERY	(1ull<<26) // If this is set, then the server is not allowed to query this cvar's value (via IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_VCONSOLE_SET_FOCUS	(1ull<<27)
#define FCVAR_CLIENTCMD_CAN_EXECUTE	(1ull<<28)	// IVEngineClient::ClientCmd is allowed to execute this command. 
											// Note: IVEngineClient::ClientCmd_Unrestricted can run any client command.

#define FCVAR_EXECUTE_PER_TICK		(1ull<<29)

#define FCVAR_DEFENSIVE				(1ull<<32)


//-----------------------------------------------------------------------------
// Called when a ConCommand needs to execute
//-----------------------------------------------------------------------------
typedef void ( *FnCommandCallback_t )( const CCommandContext &context, const CCommand &command );
typedef void ( *FnCommandCallbackNoContext_t )( const CCommand &command );
typedef void ( *FnCommandCallbackVoid_t )();

//-----------------------------------------------------------------------------
// Returns 0 to COMMAND_COMPLETION_MAXITEMS worth of completion strings
//-----------------------------------------------------------------------------
typedef void (*FnCommandCompletionCallback)( const CCommand &command, CUtlVector< CUtlString > &completions );


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
	virtual int  CommandCompletionCallback( const CCommand &command, CUtlVector< CUtlString > &completions ) = 0;
};

//-----------------------------------------------------------------------------
// Command tokenizer
//-----------------------------------------------------------------------------
enum CommandTarget_t
{
	CT_NO_TARGET = -1,
	CT_FIRST_SPLITSCREEN_CLIENT = 0,
	CT_LAST_SPLITSCREEN_CLIENT = 3,
};

class CCommandContext
{
public:
	CCommandContext( CommandTarget_t nTarget, CPlayerSlot nSlot ) :
		m_nTarget( nTarget ), m_nPlayerSlot( nSlot )
	{}

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

class CCommand
{
public:
	CCommand();
	CCommand( int nArgC, const char **ppArgV );
	bool Tokenize( CUtlString pCommand, characterset_t *pBreakSet = nullptr );
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
	void EnsureBuffers();

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

struct ConCommandCallbackInfo_t
{
	ConCommandCallbackInfo_t() :
		m_fnCommandCallback( nullptr ),
		m_bIsInterface( false ),
		m_bIsVoidCallback( false ),
		m_bIsContextLess( false )
	{}

	ConCommandCallbackInfo_t( FnCommandCallback_t cb ) :
		m_fnCommandCallback( cb ),
		m_bIsInterface( false ),
		m_bIsVoidCallback( false ),
		m_bIsContextLess( false )
	{}

	ConCommandCallbackInfo_t( FnCommandCallbackVoid_t cb ) :
		m_fnVoidCommandCallback( cb ),
		m_bIsInterface( false ),
		m_bIsVoidCallback( cb ? true : false ),
		m_bIsContextLess( false )
	{}

	ConCommandCallbackInfo_t( FnCommandCallbackNoContext_t cb ) :
		m_fnContextlessCommandCallback( cb ),
		m_bIsInterface( false ),
		m_bIsVoidCallback( false ),
		m_bIsContextLess( cb ? true : false )
	{}

	ConCommandCallbackInfo_t( ICommandCallback *cb ) :
		m_pCommandCallback( cb ),
		m_bIsInterface( cb ? true : false ),
		m_bIsVoidCallback( false ),
		m_bIsContextLess( false )
	{}

	bool IsValid() const { return m_fnCommandCallback != nullptr; }

	void Dispatch( const CCommandContext &context, const CCommand &command ) const
	{
		if(!IsValid())
			return;

		if(m_bIsInterface)
			m_pCommandCallback->CommandCallback( context, command );
		else if(m_bIsVoidCallback)
			m_fnVoidCommandCallback();
		else if(m_bIsContextLess)
			m_fnContextlessCommandCallback( command );
		else
			m_fnCommandCallback( context, command );
	}

	union
	{
		FnCommandCallback_t m_fnCommandCallback;
		FnCommandCallbackVoid_t m_fnVoidCommandCallback;
		FnCommandCallbackNoContext_t m_fnContextlessCommandCallback;
		ICommandCallback *m_pCommandCallback;
	};

	bool m_bIsInterface : 1;
	bool m_bIsVoidCallback : 1;
	bool m_bIsContextLess : 1;
};

struct ConCommandCompletionCallbackInfo_t
{
	ConCommandCompletionCallbackInfo_t() :
		m_fnCompletionCallback( nullptr ),
		m_bIsFunction( false ),
		m_bIsInterface( false )
	{}

	ConCommandCompletionCallbackInfo_t( FnCommandCompletionCallback cb ) :
		m_fnCompletionCallback( cb ),
		m_bIsFunction( cb ? true : false ),
		m_bIsInterface( false )
	{}

	ConCommandCompletionCallbackInfo_t( ICommandCompletionCallback *cb ) :
		m_pCommandCompletionCallback( cb ),
		m_bIsFunction( false ),
		m_bIsInterface( cb ? true : false )
	{}

	bool IsValid() const { return m_fnCompletionCallback != nullptr; }

	int Dispatch( const CCommand &command, CUtlVector< CUtlString > &completions ) const
	{
		if(!IsValid())
			return 0;

		if(m_bIsInterface)
			return m_pCommandCompletionCallback->CommandCompletionCallback( command, completions );
		
		m_fnCompletionCallback( command, completions );
		return completions.Count();
	}

	union
	{
		FnCommandCompletionCallback	m_fnCompletionCallback;
		ICommandCompletionCallback *m_pCommandCompletionCallback;
	};

	bool m_bIsFunction;
	bool m_bIsInterface;
};

struct ConCommandCreation_t : CVarCreationBase_t
{
	ConCommandCallbackInfo_t m_CBInfo;
	ConCommandCompletionCallbackInfo_t m_CompletionCBInfo;
};

//-----------------------------------------------------------------------------
// ConCommands internal data storage class
//-----------------------------------------------------------------------------
class ConCommandData
{
public:
	const char *GetName() const { return m_pszName; }
	const char *GetHelpText() const { return m_pszHelpString; }
	bool HasHelpText() const { return m_pszHelpString && m_pszHelpString[0]; }

	bool IsFlagSet( uint64 flag ) const { return (m_nFlags & flag) != 0; }
	void AddFlags( uint64 flags ) { m_nFlags |= flags; }
	void RemoveFlags( uint64 flags ) { m_nFlags &= ~flags; }
	uint64 GetFlags( void ) const { return m_nFlags; }

	int GetAutoCompleteSuggestions( const CCommand &command, CUtlVector< CUtlString > &completions ) const
	{
		return m_CompletionCB.Dispatch( command, completions );
	}

	bool HasCompletionCallback() const { return m_CompletionCB.IsValid(); }
	bool HasCallback() const { return m_CallbackInfoIndex > 0; }

	const char*				m_pszName;
	const char*				m_pszHelpString;
	uint64					m_nFlags;

	ConCommandCompletionCallbackInfo_t m_CompletionCB;

	// Register index of concommand which completion cb comes from
	int m_CompletionCBCmdIndex;
	// Index in a linkedlist of callbackinfos
	uint16 m_CallbackInfoIndex;
};

//-----------------------------------------------------------------------------
// Used to access registered concommands
//-----------------------------------------------------------------------------
class ConCommandRef
{
private:
	static const uint16 kInvalidAccessIndex = 0xFFFF;

public:

	ConCommandRef() : m_CommandAccessIndex( kInvalidAccessIndex ), m_CommandRegisteredIndex( 0 ) {}
	ConCommandRef( uint16 command_idx ) : m_CommandAccessIndex( command_idx ), m_CommandRegisteredIndex( 0 ) {}
	ConCommandRef( uint16 access_idx, int reg_idx ) : m_CommandAccessIndex( access_idx ), m_CommandRegisteredIndex( reg_idx ) {}

	ConCommandRef( const char *name, bool allow_defensive = false );

	ConCommandData *GetRawData();
	const ConCommandData *GetRawData() const { return const_cast<ConCommandRef *>(this)->GetRawData(); }

	const char *GetName() const { return GetRawData()->GetName(); }
	const char *GetHelpText() const { return GetRawData()->GetHelpText(); }
	bool HasHelpText() const { return GetRawData()->HasHelpText(); }

	bool IsFlagSet( uint64 flag ) const { return GetRawData()->IsFlagSet( flag ); }
	void AddFlags( uint64 flags ) { GetRawData()->AddFlags( flags ); }
	void RemoveFlags( uint64 flags ) { GetRawData()->RemoveFlags( flags ); }
	uint64 GetFlags() const { return GetRawData()->GetFlags(); }

	bool HasCompletionCallback() const { return GetRawData()->HasCompletionCallback(); }
	bool HasCallback() const { return GetRawData()->HasCallback(); }

	void Dispatch( const CCommandContext &context, const CCommand &command );

	int GetCompletionCommandIndex() const { return GetRawData()->m_CompletionCBCmdIndex; }
	uint16 GetCallbackIndex() const { return GetRawData()->m_CallbackInfoIndex; }

	int GetAutoCompleteSuggestions( const CCommand &command, CUtlVector< CUtlString > &completions ) const
	{
		return GetRawData()->GetAutoCompleteSuggestions( command, completions );
	}

	void InvalidateRef() { m_CommandAccessIndex = kInvalidAccessIndex; m_CommandRegisteredIndex = 0; }
	bool IsValidRef() const { return m_CommandAccessIndex != kInvalidAccessIndex; }
	uint16 GetAccessIndex() const { return m_CommandAccessIndex; }
	int GetRegisteredIndex() const { return m_CommandRegisteredIndex; }

private:
	// Index into internal linked list of concommands
	uint16 m_CommandAccessIndex;
	// Commands registered positional index
	int m_CommandRegisteredIndex;
};

//-----------------------------------------------------------------------------
// Used to register concommands in the system as well as access its own data
//-----------------------------------------------------------------------------
class ConCommand : public ConCommandRef
{
public:
	typedef ConCommandRef BaseClass;

	ConCommand( const char *pName, ConCommandCallbackInfo_t callback,
		const char *pHelpString, uint64 flags = 0,
		ConCommandCompletionCallbackInfo_t completionFunc = ConCommandCompletionCallbackInfo_t() )
		: BaseClass()
	{
		Create( pName, callback, pHelpString, flags, completionFunc );
	}

	~ConCommand()
	{
		Destroy();
	}

private:
	void Create( const char *pName, const ConCommandCallbackInfo_t &cb, const char *pHelpString, uint64 flags, const ConCommandCompletionCallbackInfo_t &completion_cb );
	void Destroy( );
};

using FnGenericChangeCallback_t = void(*)(ConVarRefAbstract* ref, CSplitScreenSlot nSlot, const CVValue_t* pNewValue, const CVValue_t* pOldValue);

struct ConVarValueInfo_t
{
	ConVarValueInfo_t( EConVarType type = EConVarType_Invalid ) :
		m_Version( 0 ),
		m_bHasDefault( false ),
		m_bHasMin( false ),
		m_bHasMax( false ),
		m_defaultValue {},
		m_minValue {},
		m_maxValue {},
		m_fnCallBack(),
		m_eVarType( type )
	{}

	template<typename T>
	void SetDefaultValue( const T &value )
	{
		m_bHasDefault = true;
		*reinterpret_cast<T *>(m_defaultValue) = value;
	}

	template<typename T>
	void SetMinValue( const T &min )
	{
		m_bHasMin = true;
		*reinterpret_cast<T *>(m_minValue) = min;
	}

	template<typename T>
	void SetMaxValue( const T &max )
	{
		m_bHasMax = true;
		*reinterpret_cast<T *>(m_maxValue) = max;
	}

	int32 m_Version;

	bool m_bHasDefault;
	bool m_bHasMin;
	bool m_bHasMax;

private:
	// Don't use CVValue_t directly to prevent align issues and to avoid initialising memory
	uint8 m_defaultValue[sizeof( CVValue_t )];
	uint8 m_minValue[sizeof( CVValue_t )];
	uint8 m_maxValue[sizeof( CVValue_t )];

public:
	FnGenericChangeCallback_t m_fnCallBack;
	EConVarType m_eVarType;
};

struct ConVarCreation_t : CVarCreationBase_t
{
	ConVarValueInfo_t m_valueInfo;
};

template <typename T> static void CvarTypeTrait_ConstructFn( CVValue_t *obj ) { Construct( (T *)obj ); }
template <typename T> static void CvarTypeTrait_CopyFn( CVValue_t *obj, const CVValue_t &other ) { *(T *)obj = *(T *)&other; }
template <typename T> static void CvarTypeTrait_DestructFn( CVValue_t *obj ) { Destruct( (T *)obj ); }
template <typename T> static bool CvarTypeTrait_StringToValueFn( const char *string, CVValue_t *obj ) { return V_StringToValue<T>( string, *(T *)obj ); }
template <typename T> static void CvarTypeTrait_ValueToStringFn( const CVValue_t *obj, CBufferString &buf );
template <typename T> static bool CvarTypeTrait_EqualFn( const CVValue_t *obj, const CVValue_t *other ) { return *(T *)obj == *(T *)other; }
template <typename T> static void CvarTypeTrait_ClampFn( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min)
		*(T *)obj = Max( *(T *)obj, *(T *)min );

	if(max)
		*(T *)obj = Min( *(T *)obj, *(T *)max );
}

template<> bool CvarTypeTrait_StringToValueFn<CUtlString>( const char *string, CVValue_t *obj ) { obj->m_StringValue = string; return true; }

template<> void CvarTypeTrait_ValueToStringFn<bool>( const CVValue_t *obj, CBufferString &buf ) { buf.Insert( 0, obj->m_bValue ? "true" : "false" ); }
template<> void CvarTypeTrait_ValueToStringFn<int16>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%d", obj->m_i16Value ); }
template<> void CvarTypeTrait_ValueToStringFn<uint16>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%u", obj->m_u16Value ); }
template<> void CvarTypeTrait_ValueToStringFn<int32>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%d", obj->m_i32Value ); }
template<> void CvarTypeTrait_ValueToStringFn<uint32>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%u", obj->m_u32Value ); }
template<> void CvarTypeTrait_ValueToStringFn<int64>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%lld", obj->m_i64Value ); }
template<> void CvarTypeTrait_ValueToStringFn<uint64>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%llu", obj->m_u64Value ); }
template<> void CvarTypeTrait_ValueToStringFn<float32>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%f", obj->m_fl32Value ); }
template<> void CvarTypeTrait_ValueToStringFn<float64>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%lf", obj->m_fl64Value ); }
template<> void CvarTypeTrait_ValueToStringFn<CUtlString>( const CVValue_t *obj, CBufferString &buf ) { buf.Insert( 0, obj->m_StringValue.Get() ); }
template<> void CvarTypeTrait_ValueToStringFn<Vector2D>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%f %f", obj->m_vec2Value[0], obj->m_vec2Value[1] ); }
template<> void CvarTypeTrait_ValueToStringFn<Vector>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%f %f %f", obj->m_vec3Value[0], obj->m_vec3Value[1], obj->m_vec3Value[2] ); }
template<> void CvarTypeTrait_ValueToStringFn<Vector4D>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%f %f %f %f", obj->m_vec4Value[0], obj->m_vec4Value[1], obj->m_vec4Value[2], obj->m_vec4Value[3] ); }
template<> void CvarTypeTrait_ValueToStringFn<QAngle>( const CVValue_t *obj, CBufferString &buf ) { buf.Format( "%f %f %f", obj->m_angValue[0], obj->m_angValue[1], obj->m_angValue[2] ); }
template<> void CvarTypeTrait_ValueToStringFn<Color>( const CVValue_t *obj, CBufferString &buf )
{
	if( obj->m_clrValue.a() == 255 )
		buf.Format( "%d %d %d", obj->m_clrValue[0], obj->m_clrValue[1], obj->m_clrValue[2] );
	else
		buf.Format( "%d %d %d %d", obj->m_clrValue[0], obj->m_clrValue[1], obj->m_clrValue[2], obj->m_clrValue[3] );
}

template<> void CvarTypeTrait_ClampFn<CUtlString>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max ) { }
template<> void CvarTypeTrait_ClampFn<Color>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min) for(int i = 0; i < 4; i++) obj->m_clrValue[i] = Max( obj->m_clrValue[i], min->m_clrValue[i] );
	if(max) for(int i = 0; i < 4; i++) obj->m_clrValue[i] = Min( obj->m_clrValue[i], max->m_clrValue[i] );
}
template<> void CvarTypeTrait_ClampFn<Vector2D>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min) for(int i = 0; i < 2; i++) obj->m_vec2Value[i] = Max( obj->m_vec2Value[i], min->m_vec2Value[i] );
	if(max) for(int i = 0; i < 2; i++) obj->m_vec2Value[i] = Min( obj->m_vec2Value[i], max->m_vec2Value[i] );
}
template<> void CvarTypeTrait_ClampFn<Vector>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min) for(int i = 0; i < 3; i++) obj->m_vec3Value[i] = Max( obj->m_vec3Value[i], min->m_vec3Value[i] );
	if(max) for(int i = 0; i < 3; i++) obj->m_vec3Value[i] = Min( obj->m_vec3Value[i], max->m_vec3Value[i] );
}
template<> void CvarTypeTrait_ClampFn<Vector4D>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min) for(int i = 0; i < 4; i++) obj->m_vec4Value[i] = Max( obj->m_vec4Value[i], min->m_vec4Value[i] );
	if(max) for(int i = 0; i < 4; i++) obj->m_vec4Value[i] = Min( obj->m_vec4Value[i], max->m_vec4Value[i] );
}
template<> void CvarTypeTrait_ClampFn<QAngle>( CVValue_t *obj, const CVValue_t *min, const CVValue_t *max )
{
	if(min) for(int i = 0; i < 3; i++) obj->m_angValue[i] = Max( obj->m_angValue[i], min->m_angValue[i] );
	if(max) for(int i = 0; i < 3; i++) obj->m_angValue[i] = Min( obj->m_angValue[i], max->m_angValue[i] );
}

class ConVarData;
static ConVarData *GetInvalidConVarData( EConVarType type );

struct CVarTypeTraits
{
	template <typename T>
	CVarTypeTraits *InitAs( const char *name, const char *default_value )
	{
		m_TypeName = name;
		m_ByteSize = sizeof( T );
		m_IsPrimitive = CTypePOD<T>;

		Construct = &CvarTypeTrait_ConstructFn<T>;
		Copy = &CvarTypeTrait_CopyFn<T>;
		Destruct = &CvarTypeTrait_DestructFn<T>;
		StringToValue = &CvarTypeTrait_StringToValueFn<T>;
		ValueToString = &CvarTypeTrait_ValueToStringFn<T>;
		Equal = &CvarTypeTrait_EqualFn<T>;
		Clamp = &CvarTypeTrait_ClampFn<T>;

		m_DefaultValue = default_value;
		m_InvalidCvarData = GetInvalidConVarData( TranslateConVarType<T>() );

		return this;
	}

	const char *m_TypeName;
	int m_ByteSize;
	bool m_IsPrimitive;

	void (*Construct)(CVValue_t *obj);
	void (*Copy)(CVValue_t *obj, const CVValue_t &other);
	void (*Destruct)(CVValue_t *obj);
	bool (*StringToValue)(const char *string, CVValue_t *obj);
	void (*ValueToString)(const CVValue_t *obj, CBufferString &buf);
	bool (*Equal)(const CVValue_t *obj, const CVValue_t *other);
	void (*Clamp)(CVValue_t *obj, const CVValue_t *min, const CVValue_t *max);

	const char *m_DefaultValue;
	ConVarData *m_InvalidCvarData;
};

static CVarTypeTraits *GetCvarTypeTraits( EConVarType type )
{
	Assert( type >= 0 && type < EConVarType_MAX );

	static CVarTypeTraits s_TypeTraits[EConVarType_MAX] = {
		*CVarTypeTraits().InitAs<bool>( "bool", "false" ),
		*CVarTypeTraits().InitAs<int16>( "int16", "0" ),
		*CVarTypeTraits().InitAs<uint16>( "uint16", "0" ),
		*CVarTypeTraits().InitAs<int32>( "int32", "0" ),
		*CVarTypeTraits().InitAs<uint32>( "uint32", "0" ),
		*CVarTypeTraits().InitAs<int64>( "int64", "0" ),
		*CVarTypeTraits().InitAs<uint64>( "uint64", "0" ),
		*CVarTypeTraits().InitAs<float32>( "float32", "0" ),
		*CVarTypeTraits().InitAs<float64>( "float64", "0" ),
		*CVarTypeTraits().InitAs<CUtlString>( "string", "" ),
		*CVarTypeTraits().InitAs<Color>( "color", "0 0 0 255" ),
		*CVarTypeTraits().InitAs<Vector2D>( "vector2", "0 0" ),
		*CVarTypeTraits().InitAs<Vector>( "vector3", "0 0 0" ),
		*CVarTypeTraits().InitAs<Vector4D>( "vector4", "0 0 0 0" ),
		*CVarTypeTraits().InitAs<QAngle>( "qangle", "0 0 0" )
	};

	return &s_TypeTraits[type];
}

class ConVarData
{
public:
	enum
	{
		// GameInfo was used to initialize this cvar
		CVARGI_INITIALIZED = (1 << 0),
		// GameInfo has set default value, and it cannot be overriden
		CVARGI_HAS_DEFAULT_VALUE = (1 << 1),
		// GameInfo has set min value, and it cannot be overriden
		CVARGI_HAS_MIN_VALUE = (1 << 2),
		// GameInfo has set max value, and it cannot be overriden
		CVARGI_HAS_MAX_VALUE = (1 << 3),
		// GameInfo has set cvar version
		CVARGI_HAS_VERSION = (1 << 4)
	};

	ConVarData( EConVarType type = EConVarType_Invalid ) :
		m_Values {}
	{
		Invalidate( type, true );
	}

	// Helper method to invalidate convar data to its default, pre-register state
	void Invalidate( EConVarType type = EConVarType_Invalid, bool as_undefined = false )
	{
		if(as_undefined)
			m_pszName = "<undefined>";
		m_defaultValue = CVValue_t::InvalidValue();
		m_minValue = nullptr;
		m_maxValue = nullptr;
		m_pszHelpString = as_undefined ? "This convar is being accessed prior to ConVar_Register being called" : nullptr;
		m_eVarType = type;
		m_Version = 0;
		m_iTimesChanged = 0;
		m_nFlags = FCVAR_REFERENCE;
		m_iCallbackIndex = 0;
		m_GameInfoFlags = 0;
	}

	const char *GetName( void ) const { return m_pszName; }
	const char *GetHelpText( void ) const { return m_pszHelpString; }
	bool HasHelpText() const { return m_pszHelpString && m_pszHelpString[0]; }

	EConVarType	GetType() const { return m_eVarType; }

	short GetVersion() const { return m_Version; }

	int	GetTimesChanged() const { return m_iTimesChanged; }
	void SetTimesChanged( int val ) { m_iTimesChanged = val; }
	void IncrementTimesChanged() { m_iTimesChanged++; }

	bool IsFlagSet( uint64 flag ) const { return (m_nFlags & flag) != 0; }
	void AddFlags( uint64 flags ) { m_nFlags |= flags; }
	void RemoveFlags( uint64 flags ) { m_nFlags &= ~flags; }
	uint64 GetFlags() const { return m_nFlags; }

	int GetGameInfoFlags() const { return m_GameInfoFlags; }
	int GetCallbackIndex() const { return m_iCallbackIndex; }

	int GetMaxSplitScreenSlots() const;

	CVarTypeTraits *TypeTraits() const
	{
		Assert( m_eVarType != EConVarType_Invalid );
		return GetCvarTypeTraits( m_eVarType );
	}

	int GetDataByteSize() const { return TypeTraits()->m_ByteSize; }
	bool IsPrimitiveType() const { return TypeTraits()->m_IsPrimitive; }
	const char *GetDataTypeName() const { return TypeTraits()->m_TypeName; }

	bool HasDefaultValue() const { return m_defaultValue && m_defaultValue != CVValue_t::InvalidValue(); }
	bool HasMinValue() const { return m_minValue != nullptr; }
	bool HasMaxValue() const { return m_maxValue != nullptr; }

	CVValue_t *DefaultValue() const { return m_defaultValue; }
	CVValue_t *MinValue() const { return m_minValue; }
	CVValue_t *MaxValue() const { return m_maxValue; }

	// Since cvars can't be without default value, this just resets it to a global default value if it had one set
	void RemoveDefaultValue() { if(HasDefaultValue()) TypeTraits()->StringToValue( TypeTraits()->m_DefaultValue, m_defaultValue ); }

	// AMNOTE: Min/Max values are allocated by default by a CCvar allocator which doesn't cleanup memory,
	// thus be careful when removing min/max values allocated by other sources and do cleanup yourself
	void RemoveMinValue() { m_minValue = nullptr; }
	void RemoveMaxValue() { m_maxValue = nullptr; }

	// Updates default/min/max values to other value if present
	bool UpdateDefaultValueString( const char *value ) { return HasDefaultValue() && TypeTraits()->StringToValue( value, m_defaultValue ); }
	bool UpdateMinValueString( const char *value ) { return HasMinValue() && TypeTraits()->StringToValue( value, m_minValue ); }
	bool UpdateMaxValueString( const char *value ) { return HasMaxValue() && TypeTraits()->StringToValue( value, m_maxValue ); }

	// Sets default value by copying it, doesn't need to be allocated
	void SetDefaultValue( CVValue_t *value ) { TypeTraits()->Copy( m_defaultValue, value ); }

	// AMNOTE: Expects you to manually allocate its value and for it to be alive while it's used by the cvar
	// Also you should be responsible for clearing memory on cleanup, by default game uses CCvar memory allocator for this
	void SetMinValue( CVValue_t *value ) { m_minValue = value; }
	void SetMaxValue( CVValue_t *value ) { m_maxValue = value; }

	// Could be nullptr if slot is invalid for this cvar
	CVValue_t *Value( CSplitScreenSlot slot ) const;
	// Returns default_value if slot is invalid
	CVValue_t *ValueOrDefault( CSplitScreenSlot slot ) const;

	bool IsSetToDefault( CSplitScreenSlot slot ) const { return TypeTraits()->Equal( ValueOrDefault( slot ), m_defaultValue ); }
	bool IsAllSetToDefault() const;

	void ValueToString( CSplitScreenSlot slot, CBufferString &buf ) const { TypeTraits()->ValueToString( ValueOrDefault( slot ), buf ); }
	void DefaultValueToString( CBufferString &buf ) const { TypeTraits()->ValueToString( m_defaultValue, buf ); }
	void MinValueToString( CBufferString &buf ) const;
	void MaxValueToString( CBufferString &buf ) const;

	void Construct( CSplitScreenSlot slot ) const { if(IsSlotInRange( slot )) TypeTraits()->Construct( Value( slot ) ); }
	void Destruct( CSplitScreenSlot slot ) const { if(IsSlotInRange( slot )) TypeTraits()->Destruct( Value( slot ) ); }
	bool IsEqual( CSplitScreenSlot slot, CVValue_t *other ) const { return TypeTraits()->Equal( ValueOrDefault( slot ), other ); }
	void Clamp( CSplitScreenSlot slot ) const { if(IsSlotInRange( slot )) TypeTraits()->Clamp( Value( slot ), m_minValue, m_maxValue ); }

private:
	bool IsSlotInRange( CSplitScreenSlot slot ) const { return slot.Get() == -1 || (slot.Get() >= 0 && slot.Get() < GetMaxSplitScreenSlots()); }

	const char* m_pszName;

	// Default value is expected to always be present,
	// even if convar wasn't created with default value
	// it would use global per type default value in that case
	CVValue_t* m_defaultValue;

	// Min/Max Could be nullptr if not set
	CVValue_t* m_minValue;
	CVValue_t* m_maxValue;

	const char* m_pszHelpString;
	EConVarType m_eVarType;

	// Might be set by a gameinfo config via "version" key
	short m_Version;

	unsigned int m_iTimesChanged;
	uint64 m_nFlags;

	// Index into a linked list of cvar callbacks
	unsigned int m_iCallbackIndex;

	int m_GameInfoFlags;

	// At convar registration this is trimmed to better match convar type being used
	// or if it was initialized as EConVarType_Invalid it would be of this size
	uint8 m_Values[sizeof( CVValue_t ) * MAX_SPLITSCREEN_CLIENTS];
};

static ConVarData *GetInvalidConVarData( EConVarType type )
{
	Assert( type >= EConVarType_Invalid && type < EConVarType_MAX );

	static ConVarData s_InvalidConVar[EConVarType_MAX + 1] = {
		ConVarData( TranslateConVarType<bool>() ),
		ConVarData( TranslateConVarType<int16>() ),
		ConVarData( TranslateConVarType<uint16>() ),
		ConVarData( TranslateConVarType<int32>() ),
		ConVarData( TranslateConVarType<uint32>() ),
		ConVarData( TranslateConVarType<int64>() ),
		ConVarData( TranslateConVarType<uint64>() ),
		ConVarData( TranslateConVarType<float32>() ),
		ConVarData( TranslateConVarType<float64>() ),
		ConVarData( TranslateConVarType<CUtlString>() ),
		ConVarData( TranslateConVarType<Color>() ),
		ConVarData( TranslateConVarType<Vector2D>() ),
		ConVarData( TranslateConVarType<Vector>() ),
		ConVarData( TranslateConVarType<Vector4D>() ),
		ConVarData( TranslateConVarType<QAngle>() ),
		ConVarData( TranslateConVarType<void *>() ) // EConVarType_MAX
	};

	if(type == EConVarType_Invalid)
	{
		return &s_InvalidConVar[EConVarType_MAX];
	}
	return &s_InvalidConVar[type];
}

//-----------------------------------------------------------------
// Used to read/write/create convars (replaces the FindVar method)
//-----------------------------------------------------------------
class ConVarRef
{
private:
	static const uint16 kInvalidAccessIndex = 0xFFFF;

public:
	ConVarRef() : m_ConVarAccessIndex( kInvalidAccessIndex ), m_ConVarRegisteredIndex( 0 ) {}
	ConVarRef( uint16 convar_idx ) : m_ConVarAccessIndex( convar_idx ), m_ConVarRegisteredIndex( 0 ) {}
	ConVarRef( uint16 convar_idx, int registered_idx ) : m_ConVarAccessIndex( convar_idx ), m_ConVarRegisteredIndex( registered_idx ) {}

	ConVarRef( const char *name, bool allow_defensive = false );

	void InvalidateRef() { m_ConVarAccessIndex = kInvalidAccessIndex; m_ConVarRegisteredIndex = 0; }
	bool IsValidRef() const { return m_ConVarAccessIndex != kInvalidAccessIndex; }
	uint16 GetAccessIndex() const { return m_ConVarAccessIndex; }
	int GetRegisteredIndex() const { return m_ConVarRegisteredIndex; }

protected:
	// Index into internal linked list of concommands
	uint16 m_ConVarAccessIndex;
	// ConVar registered positional index
	int m_ConVarRegisteredIndex;
};

class ConVarRefAbstract : public ConVarRef
{
public:
	typedef ConVarRef BaseClass;

	ConVarRefAbstract( const char *name, bool allow_defensive = false )
		: BaseClass( name, allow_defensive ), m_ConVarData( nullptr )
	{
		Init( *this );
	}

	ConVarRefAbstract( ConVarRef ref, EConVarType type = EConVarType_Invalid )
		: BaseClass( ref ), m_ConVarData( nullptr )
	{
		Init( *this, type );
	}

	ConVarRefAbstract( const ConVarRefAbstract &ref )
		: BaseClass(), m_ConVarData( nullptr )
	{
		CopyRef( ref );
	}

	const char *GetName() const { return m_ConVarData->GetName(); }
	const char *GetHelpText() const { return m_ConVarData->GetHelpText(); }
	bool HasHelpText() const { return m_ConVarData->HasHelpText(); }

	EConVarType	GetType() const { return m_ConVarData->GetType(); }

	bool HasDefault() const { return m_ConVarData->HasDefaultValue(); }
	bool HasMin() const { return m_ConVarData->HasMinValue(); }
	bool HasMax() const { return m_ConVarData->HasMaxValue(); }

	bool IsFlagSet( uint64 flag ) const { return m_ConVarData->IsFlagSet( flag ); }
	void AddFlags( uint64 flags ) { m_ConVarData->AddFlags( flags ); }
	void RemoveFlags( uint64 flags ) { return m_ConVarData->RemoveFlags( flags ); }
	uint64 GetFlags( void ) const { return m_ConVarData->GetFlags(); }

	// Returns true if convar should be treated as hidden
	bool ShouldBeHidden() const { return !m_ConVarData || IsFlagSet( FCVAR_REFERENCE | FCVAR_HIDDEN | FCVAR_DEVELOPMENTONLY ); }

	bool IsSetToDefault( CSplitScreenSlot slot = -1 ) const { return m_ConVarData->IsSetToDefault( slot ); }
	bool IsAllSetToDefault() const { return m_ConVarData->IsAllSetToDefault(); }

	void GetValueAsString( CBufferString &buf, CSplitScreenSlot slot = -1 ) const { m_ConVarData->ValueToString( slot, buf ); }
	void GetDefaultAsString( CBufferString &buf ) const { m_ConVarData->DefaultValueToString( buf ); }
	void GetMinAsString( CBufferString &buf ) const { m_ConVarData->MinValueToString( buf ); }
	void GetMaxAsString( CBufferString &buf ) const { m_ConVarData->MaxValueToString( buf ); }

	bool IsValuePrimitive() const { return m_ConVarData->GetType() != EConVarType_Invalid && m_ConVarData->IsPrimitiveType(); }

	// Attempts to get value as a type T, does type conversion if possible,
	// if no such action is available for CvarType/T combo, global default value for type T would be returned
	template <typename T>
	const T GetAs( CSplitScreenSlot slot = -1 ) const;

	// Attempts to get value as a bool, does type conversion if possible,
	// if no such action is available for CvarType/bool combo, global default value for bool would be returned
	bool GetBool( CSplitScreenSlot slot = -1 ) const { return GetAs<bool>( slot ); }
	// Attempts to get value as a float, does type conversion if possible,
	// if no such action is available for CvarType/float combo, global default value for float would be returned
	float GetFloat( CSplitScreenSlot slot = -1 ) const { return GetAs<float>( slot ); }
	// Attempts to get value as an int, does type conversion if possible,
	// if no such action is available for CvarType/int combo, global default value for int would be returned
	int GetInt( CSplitScreenSlot slot = -1 ) const { return GetAs<int>( slot ); }
	// Parses the value to string, mostly the same to GetValueAsString
	CUtlString GetString( CSplitScreenSlot slot = -1 ) const;

	// Attempts to set value as a type T, does type conversion if possible,
	// if no such action is available for CvarType/T combo, no action would be done
	// 
	// AMNOTE: Using this function (as well as SetBool, SetFloat and SetInt) in a
	// change callbacks (either global or cvar one) will result in a crash since it
	// does constructor casts to CConVarRef<T> which would be invalid if setvalue is queued
	// (which will happen in callbacks), so the solution is to do a static_cast to
	// CConVarRef<T> yourself on the cvar you receive in a callback and call .Set() on it
	template <typename T>
	void SetAs( const T &value, CSplitScreenSlot slot = -1 );

	// Attempts to set value as a bool, does type conversion if possible,
	// if no such action is available for CvarType/bool combo, no action would be done
	void SetBool( bool value, CSplitScreenSlot slot = -1 ) { SetAs<bool>( value, slot ); }
	// Attempts to set value as a float, does type conversion if possible,
	// if no such action is available for CvarType/float combo, no action would be done
	void SetFloat( float value, CSplitScreenSlot slot = -1 ) { SetAs<float>( value, slot ); }
	// Attempts to set value as an int, does type conversion if possible,
	// if no such action is available for CvarType/int combo, no action would be done
	void SetInt( int value, CSplitScreenSlot slot = -1 ) { SetAs<int>( value, slot ); }
	// Parses the string to CvarType type, returns true on success, false otherwise
	bool SetString( CUtlString string, CSplitScreenSlot slot = -1 );

	// Reset to default value
	void Revert( CSplitScreenSlot slot = -1 );
	// Clamps value to min/max bounds if set
	void Clamp( CSplitScreenSlot slot = -1 ) { m_ConVarData->Clamp( slot ); }

	CVarTypeTraits *TypeTraits() const { return m_ConVarData->TypeTraits(); }
	ConVarData* GetConVarData() const { return m_ConVarData; };

	// Checks if stored ConVarData points to invalid convar data
	bool IsConVarDataValid() const { return m_ConVarData && GetType() != EConVarType_Invalid && m_ConVarData != TypeTraits()->m_InvalidCvarData; }

	// Checks if ConVarData is available for usage (means cvar was registered),
	// mostly useful for CConVarRef cvars which register themselves partially
	bool IsConVarDataAvailable() const { return !m_ConVarData->IsFlagSet( FCVAR_REFERENCE ) && IsConVarDataValid(); }

protected:
	ConVarRefAbstract() : BaseClass(), m_ConVarData( nullptr ) {}

	void CopyRef( const ConVarRefAbstract &ref )
	{
		m_ConVarAccessIndex = ref.m_ConVarAccessIndex;
		m_ConVarRegisteredIndex = ref.m_ConVarRegisteredIndex;
		m_ConVarData = ref.m_ConVarData;
	}

	void CallChangeCallbacks( CSplitScreenSlot slot, CVValue_t *new_value, CVValue_t *prev_value, const char *new_str, const char *prev_str );

	void SetOrQueueValueInternal( CSplitScreenSlot slot, CVValue_t *value );
	void QueueSetValueInternal( CSplitScreenSlot slot, CVValue_t *value );
	void SetValueInternal( CSplitScreenSlot slot, CVValue_t *value );

	// Does type conversion from CvarType to type T, only valid for primitive types
	template <typename T>
	T ConvertFromPrimitiveTo( CSplitScreenSlot slot ) const;
	// Does type conversion from type T to CvarType, only valid for primitive types
	template <typename T>
	void ConvertToPrimitiveFrom( CSplitScreenSlot slot, const T &value ) const;

	// Initialises this cvar, if ref is invalid, ConVarData would be initialised to invalid convar data of a set type
	void Init( ConVarRef ref, EConVarType type = EConVarType_Invalid );

	void InvalidateConVarData( EConVarType type = EConVarType_Invalid );

	ConVarData* m_ConVarData;
};

uint64 SanitiseConVarFlags( uint64 flags );
void SetupConVar( ConVarRefAbstract *cvar, ConVarData **cvar_data, ConVarCreation_t &info );
void UnRegisterConVar( ConVarRef *cvar );

template<typename T>
class CConVarRef : public ConVarRefAbstract
{
public:
	typedef ConVarRefAbstract BaseClass;

	// Creates a convar ref that will pre-register convar in a system if it's not yet registered
	// otherwise it just acts as a normal reference to a convar
	// Mostly useful for cross module convar access, where you can't guarantee the order of creation
	// and can't get direct access to convar CConVar<T>
	CConVarRef( const char *name ) : BaseClass()
	{
		ConVarValueInfo_t value_info( TranslateConVarType<T>() );
		Register( name, FCVAR_REFERENCE, nullptr, value_info );
	}

	// Constructs typed cvar ref if the type matches, otherwise this would be initialised to invalid convar data!
	CConVarRef( const ConVarRefAbstract &ref ) : BaseClass()
	{
		// If the ref type doesn't match ours, bad cast was attempted,
		// fall back to invalid cvar data
		if(ref.GetType() == TranslateConVarType<T>())
			CopyRef( ref );
		else
			Init( ConVarRef(), TranslateConVarType<T>() );
	}

	// Constructs typed cvar ref if the type matches, otherwise this would be initialised to invalid convar data!
	CConVarRef( const ConVarRef &ref ) : CConVarRef( ConVarRefAbstract( ref ) ) { }

	const T &Get( CSplitScreenSlot slot = -1 ) const { return *reinterpret_cast<T *>(m_ConVarData->ValueOrDefault( slot )); }
	void Set( const T &value, CSplitScreenSlot slot = -1 );

	const T &GetDefault() const { return *reinterpret_cast<T *>(m_ConVarData->DefaultValue()); }
	const T &GetMin() const { return *reinterpret_cast<T *>(m_ConVarData->MinValue()); }
	const T &GetMax() const { return *reinterpret_cast<T *>(m_ConVarData->MaxValue()); }

protected:
	CConVarRef() : BaseClass() {}

	void Register( const char *name, uint64 flags, const char *help_string, const ConVarValueInfo_t &value_info )
	{
		Assert( name );

		Init( ConVarRef(), TranslateConVarType<T>() );

		ConVarCreation_t info;
		info.m_pszName = name;
		info.m_pszHelpString = help_string;
		info.m_nFlags = SanitiseConVarFlags( flags );
		info.m_valueInfo = value_info;

		SetupConVar( this, &m_ConVarData, info );
	}
};

template<typename T>
class CConVar : public CConVarRef<T>
{
public:
	typedef CConVarRef<T> BaseClass;

	using FnChangeCallback_t = void(*)(CConVar<T> *ref, CSplitScreenSlot nSlot, const T *pNewValue, const T *pOldValue);

	CConVar( const char *name, uint64 flags, const char *help_string, const T &default_value, FnChangeCallback_t cb = nullptr )
		: BaseClass()
	{
		Assert( name );

		BaseClass::Init( ConVarRef(), TranslateConVarType<T>() );

		ConVarValueInfo_t value_info( TranslateConVarType<T>() );
		value_info.SetDefaultValue( default_value );
		value_info.m_fnCallBack = reinterpret_cast<FnGenericChangeCallback_t>(cb);

		BaseClass::Register( name, flags, help_string, value_info );
	}

	CConVar( const char *name, uint64 flags, const char *help_string, const T &default_value, bool min, const T &minValue, bool max, const T &maxValue, FnChangeCallback_t cb = nullptr )
		: BaseClass()
	{
		Assert( name );

		BaseClass::Init( ConVarRef(), TranslateConVarType<T>() );

		ConVarValueInfo_t value_info( TranslateConVarType<T>() );
		value_info.SetDefaultValue( default_value );

		if(min)
			value_info.SetMinValue( minValue );

		if(max)
			value_info.SetMaxValue( maxValue );

		value_info.m_fnCallBack = reinterpret_cast<FnGenericChangeCallback_t>(cb);

		BaseClass::Register( name, flags, help_string, value_info );
	}

	~CConVar()
	{
		UnRegisterConVar( this );
		BaseClass::InvalidateConVarData();
	}
};

template<typename T>
inline const T ConVarRefAbstract::GetAs( CSplitScreenSlot slot ) const
{
	CVValue_t *value = m_ConVarData->ValueOrDefault( slot );

	if(GetType() == TranslateConVarType<T>())
		return *value;
	else if(GetType() == EConVarType_String)
	{
		CVValue_t obj;
		GetCvarTypeTraits( TranslateConVarType<T>() )->Construct( &obj );

		if(GetCvarTypeTraits( TranslateConVarType<T>() )->StringToValue( value->m_StringValue.Get(), &obj ))
		{
			T ret = obj;
			GetCvarTypeTraits( TranslateConVarType<T>() )->Destruct( &obj );
			return ret;
		}

		GetCvarTypeTraits( TranslateConVarType<T>() )->Destruct( &obj );
	}
	else if(IsValuePrimitive())
		return ConvertFromPrimitiveTo<T>( slot );

	return *GetInvalidConVarData( TranslateConVarType<T>() )->DefaultValue();
}

template<>
inline const CUtlString ConVarRefAbstract::GetAs( CSplitScreenSlot slot ) const
{
	CBufferString buf;
	GetValueAsString( buf, slot );
	return buf.Get();
}

inline CUtlString ConVarRefAbstract::GetString( CSplitScreenSlot slot ) const
{
	return GetAs<CUtlString>( slot );
}

template<typename T>
inline T ConVarRefAbstract::ConvertFromPrimitiveTo( CSplitScreenSlot slot ) const
{
	if constexpr(CTypePOD<T>)
	{
		CVValue_t *value = m_ConVarData->ValueOrDefault( slot );

		switch(GetType())
		{
			case EConVarType_Bool:		return value->m_bValue;
			case EConVarType_Int16:		return value->m_i16Value;
			case EConVarType_UInt16:	return value->m_u16Value;
			case EConVarType_Int32:		return value->m_i32Value;
			case EConVarType_UInt32:	return value->m_u32Value;
			case EConVarType_Int64:		return value->m_i64Value;
			case EConVarType_UInt64:	return value->m_u64Value;
			case EConVarType_Float32:	return value->m_fl32Value;
			case EConVarType_Float64:	return value->m_fl64Value;
		}
	}

	return *GetInvalidConVarData( TranslateConVarType<T>() )->DefaultValue();
}

template<typename T>
inline void CConVarRef<T>::Set( const T &value, CSplitScreenSlot slot )
{
	CVValue_t *cvvalue = m_ConVarData->Value( slot );

	if(cvvalue)
	{
		CVValue_t newval;
		TypeTraits()->Construct( &newval );
		TypeTraits()->Copy( &newval, value );

		SetOrQueueValueInternal( slot, &newval );

		TypeTraits()->Destruct( &newval );
	}
}

template<typename T>
inline void ConVarRefAbstract::SetAs( const T &value, CSplitScreenSlot slot )
{
	if(GetType() == TranslateConVarType<T>())
	{
		CConVarRef<T>( *this ).Set( value, slot );
	}
	else if(GetType() == EConVarType_String)
	{
		CBufferString buf;
		CVValue_t cvvalue( value );
		GetCvarTypeTraits( TranslateConVarType<T>() )->ValueToString( &cvvalue, buf );

		CConVarRef<CUtlString>( *this ).Set( buf.Get(), slot );
	}
	else if(IsValuePrimitive())
	{
		ConvertToPrimitiveFrom<T>( slot, value );
	}
}

template<> inline void ConVarRefAbstract::SetAs( const CUtlString &value, CSplitScreenSlot slot )
{
	SetString( value, slot );
}

template<typename T>
inline void ConVarRefAbstract::ConvertToPrimitiveFrom( CSplitScreenSlot slot, const T &value ) const
{
	if constexpr(CTypePOD<T>)
	{
		switch(GetType())
		{
			case EConVarType_Bool:		return CConVarRef<bool>( *this ).Set( value, slot );
			case EConVarType_Int16:		return CConVarRef<int16>( *this ).Set( value, slot );
			case EConVarType_UInt16:	return CConVarRef<uint16>( *this ).Set( value, slot );
			case EConVarType_Int32:		return CConVarRef<int32>( *this ).Set( value, slot );
			case EConVarType_UInt32:	return CConVarRef<uint32>( *this ).Set( value, slot );
			case EConVarType_Int64:		return CConVarRef<int64>( *this ).Set( value, slot );
			case EConVarType_UInt64:	return CConVarRef<uint64>( *this ).Set( value, slot );
			case EConVarType_Float32:	return CConVarRef<float32>( *this ).Set( value, slot );
			case EConVarType_Float64:	return CConVarRef<float64>( *this ).Set( value, slot );
		}
	}
}

//-----------------------------------------------------------------------------
// Called by the framework to register ConVars and ConCommands with the ICVar
//-----------------------------------------------------------------------------
typedef void (*FnConVarRegisterCallback)(ConVarRefAbstract *ref);
typedef void (*FnConCommandRegisterCallback)(ConCommandRef *ref);

void ConVar_Register( uint64 nCVarFlag = 0, FnConVarRegisterCallback cvar_reg_cb = nullptr, FnConCommandRegisterCallback cmd_reg_cb = nullptr );
void ConVar_Unregister( );


//-----------------------------------------------------------------------------
// Utility methods 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription( const ConVarRefAbstract *ref );


//-----------------------------------------------------------------------------
// Purpose: Utility class to quickly allow ConCommands to call member methods
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning (disable : 4355 )
#endif

template< class T >
class CConCommandMemberAccessor : public ICommandCallback, public ICommandCompletionCallback, public ConCommand
{
	typedef ConCommand BaseClass;
	typedef void ( T::*FnMemberCommandCallback_t )( const CCommandContext &context, const CCommand &command );
	typedef int  ( T::*FnMemberCommandCompletionCallback_t )( const CCommand &command, CUtlVector< CUtlString > &completions );

public:
	CConCommandMemberAccessor( T* pOwner, const char *pName, FnMemberCommandCallback_t callback, const char *pHelpString,
		uint64 flags = 0, FnMemberCommandCompletionCallback_t completionFunc = 0 ) :
		BaseClass( pName, this, pHelpString, flags, ( completionFunc != 0 ) ? this : NULL )
	{
		m_pOwner = pOwner;
		m_Func = callback;
		m_CompletionFunc = completionFunc;
	}

	~CConCommandMemberAccessor()
	{
		this->Destroy();
	}

	void SetOwner( T* pOwner )
	{
		m_pOwner = pOwner;
	}

	virtual void CommandCallback( const CCommandContext &context, const CCommand &command ) override
	{
		Assert( m_pOwner && m_Func );
		(m_pOwner->*m_Func)( context, command );
	}

	virtual int  CommandCompletionCallback( const CCommand &command, CUtlVector< CUtlString > &completions ) override
	{
		Assert( m_pOwner && m_CompletionFunc );
		return (m_pOwner->*m_CompletionFunc)( command, completions );
	}

private:
	T* m_pOwner;
	FnMemberCommandCallback_t m_Func;
	FnMemberCommandCompletionCallback_t m_CompletionFunc;
};

#ifdef _MSC_VER
#pragma warning ( default : 4355 )
#endif

//-----------------------------------------------------------------------------
// Purpose: Utility macros to quicky generate a simple console command
//-----------------------------------------------------------------------------
#define CON_COMMAND( name, description ) \
   static void name##_callback( const CCommand &args ); \
   static ConCommand name##_command( #name, name##_callback, description ); \
   static void name##_callback( const CCommand &args )
#ifdef CLIENT_DLL
	#define CON_COMMAND_SHARED( name, description ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( #name "_client", name##_callback, description ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_SHARED( name, description ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( #name, name##_callback, description ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_F( name, description, flags ) \
	static void name##_callback( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( #name, name##_callback, description, flags ); \
	static void name##_callback( const CCommandContext &context, const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( #name "_client", name##_callback, description, flags ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_F_SHARED( name, description, flags ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( #name, name##_callback, description, flags ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_F_COMPLETION( name, description, flags, completion ) \
	static void name##_callback( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( #name, name##_callback, description, flags, completion ); \
	static void name##_callback( const CCommandContext &context, const CCommand &args )

#ifdef CLIENT_DLL
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command_client( name##_command, #name "_client", name##_callback, description, flags, completion ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#else
	#define CON_COMMAND_F_COMPLETION_SHARED( name, description, flags, completion ) \
		static void name##_callback( const CCommandContext &context, const CCommand &args ); \
		static ConCommand name##_command( name##_command, #name, name##_callback, description, flags, completion ); \
		static void name##_callback( const CCommandContext &context, const CCommand &args )
#endif


#define CON_COMMAND_EXTERN( name, _funcname, description ) \
	void _funcname( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( #name, _funcname, description ); \
	void _funcname( const CCommandContext &context, const CCommand &args )

#define CON_COMMAND_EXTERN_F( name, _funcname, description, flags ) \
	void _funcname( const CCommandContext &context, const CCommand &args ); \
	static ConCommand name##_command( #name, _funcname, description, flags ); \
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
