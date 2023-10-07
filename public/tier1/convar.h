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
#include "tier1/characterset.h"
#include "Color.h"
#include "mathlib/vector4d.h"
#include "playerslot.h"

#include <cstdint>

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
class IConVar;
class CCommand;
class ConCommand;
class ConVar;
class ConCommandRefAbstract;

struct CVarCreationBase_t
{
	CVarCreationBase_t( ) :
	name( nullptr ),
	description( nullptr ),
	flags( 0 )
	{}

	bool				IsFlagSet( int64_t flag ) const;
	void				AddFlags( int64_t flags );
	void				RemoveFlags( int64_t flags );
	int64_t				GetFlags() const;

	const char*				name;
	const char*				description;
	int64_t					flags;
};

inline bool CVarCreationBase_t::IsFlagSet( int64_t flag ) const
{
	return ( flag & this->flags ) ? true : false;
}

inline void CVarCreationBase_t::AddFlags( int64_t flags )
{
	this->flags |= flags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	this->flags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

inline void CVarCreationBase_t::RemoveFlags( int64_t flags )
{
	this->flags &= ~flags;
}

inline int64_t CVarCreationBase_t::GetFlags( void ) const
{
	return this->flags;
}

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
	EConVarType_Qangle,
	EConVarType_MAX
};

union CVValue_t
{
	CVValue_t() { memset(this, 0, sizeof(*this)); }
	CVValue_t(CVValue_t const& cp) { memcpy(this, &cp, sizeof(*this)); };
	CVValue_t& operator=(CVValue_t other) { memcpy(this, &other, sizeof(*this)); return *this; }

	template<typename T>
	FORCEINLINE_CVAR CVValue_t& operator=(T other);

	bool		m_bValue;
	int16_t		m_i16Value;
	uint16_t	m_u16Value;
	int32_t		m_i32Value;
	uint32_t	m_u32Value;
	int64_t		m_i64Value;
	uint64_t	m_u64Value;
	float		m_flValue;
	double		m_dbValue;
	const char* m_szValue;
	Color		m_clrValue;
	Vector2D	m_vec2Value;
	Vector		m_vec3Value;
	Vector4D	m_vec4Value;
	QAngle		m_angValue;
};
static_assert(sizeof(float) == sizeof(int32_t), "Wrong float type size for ConVar!");
static_assert(sizeof(double) == sizeof(int64_t), "Wrong double type size for ConVar!");

template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<bool>( bool other )					{ m_bValue = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<int16_t>( int16_t other )			{ m_i16Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<uint16_t>( uint16_t other )			{ m_u16Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<int32_t>( int32_t other )			{ m_i32Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<uint32_t>( uint32_t other )			{ m_u32Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<int64_t>( int64_t other )			{ m_i64Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<uint64_t>( uint64_t other )			{ m_u64Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<float>( float other )				{ m_flValue = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<double>( double other )				{ m_dbValue = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const char*>( const char* other )	{ m_szValue = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const Color&>( const Color& other )		{ m_clrValue = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const Vector2D&>( const Vector2D& other )	{ m_vec2Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const Vector&>( const Vector& other )		{ m_vec3Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const Vector4D&>( const Vector4D& other )	{ m_vec4Value = other; return *this; }
template<> FORCEINLINE_CVAR CVValue_t& CVValue_t::operator=<const QAngle&>( const QAngle& other )	{ m_angValue = other; return *this; }

//-----------------------------------------------------------------------------
// Called when a ConVar changes value
//-----------------------------------------------------------------------------
typedef void(*FnChangeCallbackGlobal_t)(ConVar* cvar, CSplitScreenSlot nSlot, const char *pNewValue, const char *pOldValue);
using FnChangeCallback_t = void(*)(ConVar* cvar, CSplitScreenSlot nSlot, CVValue_t *pNewValue, CVValue_t *pOldValue);
static_assert(sizeof(FnChangeCallback_t) == 0x8, "Wrong size for FnChangeCallback_t");

//-----------------------------------------------------------------------------
// ConVar & ConCommand creation listener callbacks
//-----------------------------------------------------------------------------
class ICVarListenerCallbacks
{
public:
	virtual void OnConVarCreated( ConVar* pNewCvar ) = 0;
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
	ConVarHandle handle;
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
struct ConCommandCreation_t : CVarCreationBase_t
{
	ConCommandCreation_t() :
	has_complitioncallback(false),
	is_interface(false),
	refHandle(nullptr)
	{}

	// Call this function when executing the command
	struct CallbackInfo_t
	{
		CallbackInfo_t() :
		fnCommandCallback(nullptr),
		is_interface(false),
		is_voidcallback(false),
		is_contextless(false)
		{}

		union {
			FnCommandCallback_t fnCommandCallback;
			FnCommandCallbackVoid_t fnVoidCommandCallback;
			FnCommandCallbackNoContext_t fnContextlessCommandCallback;
			ICommandCallback* pCommandCallback;
		};

		bool is_interface;
		bool is_voidcallback;
		bool is_contextless;
	};

	CallbackInfo_t callback;

	// NOTE: To maintain backward compat, we have to be very careful:
	// All public virtual methods must appear in the same order always
	// since engine code will be calling into this code, which *does not match*
	// in the mod code; it's using slightly different, but compatible versions
	// of this class. Also: Be very careful about adding new fields to this class.
	// Those fields will not exist in the version of this class that is instanced
	// in mod code.

	union
	{
		FnCommandCompletionCallback	fnCompletionCallback;
		ICommandCompletionCallback* pCommandCompletionCallback;
	};

	bool has_complitioncallback;
	bool is_interface;

	ConVarHandle* refHandle;
};
static_assert(sizeof(ConCommandCreation_t) == 0x40, "ConCommandCreation_t is of the wrong size!");

class ConCommandBase {}; // For metamod compatibility only!!!!
class ConCommand : public ConCommandBase
{
public:
	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallback_t callback,
		const char *pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, FnCommandCallbackVoid_t callback,
		const char *pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract* pReferenceOut, const char* pName, FnCommandCallbackNoContext_t callback,
		const char* pHelpString = 0, int64 flags = 0, FnCommandCompletionCallback completionFunc = 0 );
	ConCommand( ConCommandRefAbstract *pReferenceOut, const char *pName, ICommandCallback *pCallback,
		const char *pHelpString = 0, int64 flags = 0, ICommandCompletionCallback *pCommandCompletionCallback = 0 );

	~ConCommand()
	{
		this->Destroy();
	}

private:
	void Create( const char *pName, const char *pHelpString, int64_t flags, ConCommandCreation_t& setup );
	void Destroy( );

	ConVarHandle m_Handle;
};

#pragma pack(push,1)
struct ConVarSetup_t
{
	ConVarSetup_t() :
	unknown0(0),
	has_default(false),
	has_min(false),
	has_max(false),
	default_value(),
	min_value(),
	max_value(),
	callback(nullptr),
	type(EConVarType_Invalid),
	unk1(0),
	unk2(0)
	{}

	int32 unknown0; // 0x0

	bool has_default; // 0x4
	bool has_min; // 0x5
	bool has_max; // 0x6

	CVValue_t default_value; // 0x7
	CVValue_t min_value; // 0x17
	CVValue_t max_value; // 0x27

	char pad; // 0x37

	FnChangeCallback_t callback; // 0x38
	EConVarType type; // 0x40

	int32_t unk1; // 0x42
	int16_t unk2; // 0x46
};
#pragma pack(pop)

static_assert(sizeof(ConVarSetup_t) == 0x48, "ConVarSetup_t is of the wrong size!");
static_assert(sizeof(ConVarSetup_t) % 8 == 0x0, "ConVarSetup_t isn't 8 bytes aligned!");

struct ConVarCreation_t : CVarCreationBase_t {
	ConVarCreation_t() :
	refHandle(nullptr),
	refConVar(nullptr)
	{}
	ConVarSetup_t setup; // 0x18

	ConVarHandle* refHandle; // 0x60
	IConVar** refConVar; // 0x68
};

static_assert(sizeof(ConVarCreation_t) == 0x70, "ConVarCreation_t wrong size!");
static_assert(sizeof(ConVarCreation_t) % 8 == 0x0, "ConVarCreation_t isn't 8 bytes aligned!");
static_assert(sizeof(CVValue_t) == 0x10, "CVValue_t wrong size!");

//-----------------------------------------------------------------
// Used to read/write/create? convars (replaces the FindVar method)
//-----------------------------------------------------------------
class ConVar
{
public:
	// sub_6A66B0
	template<typename T>
	ConVar(const char* name, int32_t flags, const char* description, T value)
	{
		this->Init(INVALID_CONVAR_HANDLE, TranslateType<T>());

		ConVarSetup_t setup;
		setup.has_default = true;
		setup.default_value = value;
		setup.type = TranslateType<T>();

		this->Register(name, flags &~ FCVAR_DEVELOPMENTONLY, description, setup);
	}

	~ConVar();

private:
	template<typename T>
	static constexpr EConVarType TranslateType();

	// sub_10B7BC0
	void Init(ConVarHandle defaultHandle, EConVarType type);

	// sub_10B7C70
	void Register(const char* name, int32_t flags, const char* description, const ConVarSetup_t& obj);
public:
	// High-speed method to read convar data
	ConVarHandle m_Handle;
	IConVar* m_ConVar;
};

template<> constexpr EConVarType ConVar::TranslateType<bool>( void )		{ return EConVarType_Bool; }
template<> constexpr EConVarType ConVar::TranslateType<int16_t>( void )		{ return EConVarType_Int16; }
template<> constexpr EConVarType ConVar::TranslateType<uint16_t>( void )	{ return EConVarType_UInt16; }
template<> constexpr EConVarType ConVar::TranslateType<int32_t>( void )		{ return EConVarType_Int32; }
template<> constexpr EConVarType ConVar::TranslateType<uint32_t>( void )	{ return EConVarType_UInt32; }
template<> constexpr EConVarType ConVar::TranslateType<int64_t>( void )		{ return EConVarType_Int64; }
template<> constexpr EConVarType ConVar::TranslateType<uint64_t>( void )	{ return EConVarType_UInt64; }
template<> constexpr EConVarType ConVar::TranslateType<float>( void )		{ return EConVarType_Float32; }
template<> constexpr EConVarType ConVar::TranslateType<double>( void )		{ return EConVarType_Float64; }
template<> constexpr EConVarType ConVar::TranslateType<const char*>( void )	{ return EConVarType_String; }
template<> constexpr EConVarType ConVar::TranslateType<Color>( void )		{ return EConVarType_Color; }
template<> constexpr EConVarType ConVar::TranslateType<Vector2D>( void )	{ return EConVarType_Vector2; }
template<> constexpr EConVarType ConVar::TranslateType<Vector>( void )		{ return EConVarType_Vector3; }
template<> constexpr EConVarType ConVar::TranslateType<Vector4D>( void )	{ return EConVarType_Vector4; }
template<> constexpr EConVarType ConVar::TranslateType<QAngle>( void )		{ return EConVarType_Qangle; }

// sub_10B7760
IConVar* ConVar_Invalid(EConVarType type);

class IConVar
{
friend class ConVarRegList;
friend class ConVar;
public:
	IConVar(EConVarType type);
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
void ConVar_PrintDescription( const CVarCreationBase_t* pVar );


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
		this->Destroy();
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
