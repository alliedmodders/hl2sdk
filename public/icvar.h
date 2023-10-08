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
#include "mathlib/vector4d.h"
#include <cstdint>

class BaseConVar {};
class ConCommand;
class CCommand;
class CCommandContext;
class ICVarListenerCallbacks;
class IConVarListener;
struct ConVarCreation_t;
struct ConCommandCreation_t;
struct ConVarSnapshot_t;
class KeyValues;

template<typename T>
class ConVar;

union CVValue_t
{
	CVValue_t() { memset(this, 0, sizeof(*this)); }
	CVValue_t(CVValue_t const& cp) { memcpy(this, &cp, sizeof(*this)); };
	CVValue_t& operator=(CVValue_t other) { memcpy(this, &other, sizeof(*this)); return *this; }

	template<typename T>
	inline CVValue_t& operator=(T other);

	inline operator bool() const			{ return m_bValue; }
	inline operator int16_t() const			{ return m_i16Value; }
	inline operator uint16_t() const		{ return m_u16Value; }
	inline operator int32_t() const			{ return m_i32Value; }
	inline operator uint32_t() const		{ return m_u32Value; }
	inline operator int64_t() const			{ return m_i64Value; }
	inline operator uint64_t() const		{ return m_u64Value; }
	inline operator float() const			{ return m_flValue; }
	inline operator double() const			{ return m_dbValue; }
	inline operator const char*() const		{ return m_szValue; }
	inline operator const Color&() const	{ return m_clrValue; }
	inline operator const Vector2D&() const	{ return m_vec2Value; }
	inline operator const Vector&() const	{ return m_vec3Value; }
	inline operator const Vector4D&() const	{ return m_vec4Value; }
	inline operator const QAngle&() const 	{ return m_angValue; }

	bool		m_bValue;
	int16_t		m_i16Value;
	uint16_t	m_u16Value;
	int32_t		m_i32Value;
	uint32_t	m_u32Value;
	int64_t		m_i64Value;
	uint64_t	m_u64Value;
	float		m_flValue;
	double		m_dbValue;
	const char*	m_szValue;
	Color		m_clrValue;
	Vector2D	m_vec2Value;
	Vector		m_vec3Value;
	Vector4D	m_vec4Value;
	QAngle		m_angValue;
};
static_assert(sizeof(float) == sizeof(int32_t), "Wrong float type size for ConVar!");
static_assert(sizeof(double) == sizeof(int64_t), "Wrong double type size for ConVar!");

template<> inline CVValue_t& CVValue_t::operator=<bool>( const bool other )					{ m_bValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int16_t>( const int16_t other )			{ m_i16Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint16_t>( const uint16_t other )			{ m_u16Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int32_t>( const int32_t other )			{ m_i32Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint32_t>( const uint32_t other )			{ m_u32Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int64_t>( const int64_t other )			{ m_i64Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint64_t>( const uint64_t other )			{ m_u64Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<float>( const float other )				{ m_flValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<double>( const double other )				{ m_dbValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const char*>( const char* other )			{ m_szValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const Color&>( const Color& other )		{ m_clrValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const Vector2D&>( const Vector2D& other )	{ m_vec2Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const Vector&>( const Vector& other )		{ m_vec3Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const Vector4D&>( const Vector4D& other )	{ m_vec4Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const QAngle&>( const QAngle& other )		{ m_angValue = other; return *this; }

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
typedef void(*FnChangeCallbackGlobal_t)(BaseConVar* ref, CSplitScreenSlot nSlot, const char *pNewValue, const char *pOldValue);

//-----------------------------------------------------------------------------
// Purpose: Replaces the ConVar* and ConCommand*
//-----------------------------------------------------------------------------
class ConVarHandle
{
public:
	ConVarHandle( uint16_t index = -1, uint32_t handle = -1) :
	m_convarIndex(index),
	m_handleIndex(handle)
	{}

	bool IsValid( ) const { return m_convarIndex != 0xFFFF; }
	void Invalidate( )
	{
		m_convarIndex = 0xFFFF;
		m_handleIndex = 0x0;
	}

	uint16_t GetConVarIndex() const { return m_convarIndex; }
	uint32_t GetIndex() const { return m_handleIndex; }

private:
	uint16_t m_convarIndex;
	uint32_t m_handleIndex;
};

static const ConVarHandle INVALID_CONVAR_HANDLE = ConVarHandle();

class ConCommandHandle
{
public:
	ConCommandHandle( uint16_t index = -1, uint32_t handle = -1) :
	m_concommandIndex(index),
	m_handleIndex(handle)
	{}

	bool IsValid( ) const { return m_concommandIndex != 0xFFFF; }
	void Invalidate( )
	{
		m_concommandIndex = 0xFFFF;
		m_handleIndex = 0x0;
	}

	uint16_t GetConVarIndex() const { return m_concommandIndex; }
	uint32_t GetIndex() const { return m_handleIndex; }

private:
	uint32_t m_concommandIndex;
	uint32_t m_handleIndex;
};

static const ConCommandHandle INVALID_CONCOMMAND_HANDLE = ConCommandHandle();

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

template<typename T>
constexpr EConVarType TranslateConVarType();

template<> constexpr EConVarType TranslateConVarType<bool>( void )		{ return EConVarType_Bool; }
template<> constexpr EConVarType TranslateConVarType<int16_t>( void )	{ return EConVarType_Int16; }
template<> constexpr EConVarType TranslateConVarType<uint16_t>( void )	{ return EConVarType_UInt16; }
template<> constexpr EConVarType TranslateConVarType<int32_t>( void )	{ return EConVarType_Int32; }
template<> constexpr EConVarType TranslateConVarType<uint32_t>( void )	{ return EConVarType_UInt32; }
template<> constexpr EConVarType TranslateConVarType<int64_t>( void )	{ return EConVarType_Int64; }
template<> constexpr EConVarType TranslateConVarType<uint64_t>( void )	{ return EConVarType_UInt64; }
template<> constexpr EConVarType TranslateConVarType<float>( void )		{ return EConVarType_Float32; }
template<> constexpr EConVarType TranslateConVarType<double>( void )	{ return EConVarType_Float64; }
template<> constexpr EConVarType TranslateConVarType<const char*>( void ){ return EConVarType_String; }
template<> constexpr EConVarType TranslateConVarType<Color>( void )		{ return EConVarType_Color; }
template<> constexpr EConVarType TranslateConVarType<Vector2D>( void )	{ return EConVarType_Vector2; }
template<> constexpr EConVarType TranslateConVarType<Vector>( void )	{ return EConVarType_Vector3; }
template<> constexpr EConVarType TranslateConVarType<Vector4D>( void )	{ return EConVarType_Vector4; }
template<> constexpr EConVarType TranslateConVarType<QAngle>( void )	{ return EConVarType_Qangle; }
template<> constexpr EConVarType TranslateConVarType<void*>( void )		{ return EConVarType_Invalid; }

template<typename T>
struct ConVarData_t;

struct ConVarBaseData_t
{
	template<typename T>
	inline const ConVarData_t<T>* Cast() const
	{
		if (this->m_eVarType == TranslateConVarType<T>())
		{
			return reinterpret_cast<const ConVarData_t<T>*>(this);
		}
		return nullptr;
	}

	template<typename T>
	inline ConVarData_t<T>* Cast()
	{
		if (this->m_eVarType == TranslateConVarType<T>())
		{
			return reinterpret_cast<ConVarData_t<T>*>(this);
		}
		return nullptr;
	}

	ConVarBaseData_t() :
		m_pszName("<undefined>"),
		m_defaultValue(nullptr),
		m_minValue(nullptr),
		m_maxValue(nullptr),
		m_pszHelpString("This convar is being accessed prior to ConVar_Register being called"),
		m_eVarType(EConVarType_Invalid)
	{
	}

	const char* m_pszName;

	void* m_defaultValue;
	void* m_minValue;
	void* m_maxValue;
	const char* m_pszHelpString;
	EConVarType m_eVarType;

	// This gets copied from the ConVarDesc_t on creation
	short unk1;

	unsigned int m_iTimesChanged;
	int64 m_nFlags;
	unsigned int m_iCallbackIndex;

	// Used when setting default, max, min values from the ConVarDesc_t
	// although that's not the only place of usage
	// flags seems to be:
	// (1 << 0) Skip setting value to split screen slots and also something keyvalues related
	// (1 << 1) Skip setting default value
	// (1 << 2) Skip setting min/max values
	int m_nUnknownAllocFlags;
};

template<typename T>
struct ConVarData_t : ConVarBaseData_t
{
public:
friend class ConVar<T>;
	ConVarData_t()
	{
		m_defaultValue = new T();
		m_minValue = new T();
		m_maxValue = new T();
		m_eVarType = TranslateConVarType<T>();
	}

	~ConVarData_t()
	{
		delete m_defaultValue;
		delete m_minValue;
		delete m_eVarType;
	}

	inline const char*	GetName( ) const			{ return m_pszName; }
	inline const char*	GetDescription( ) const		{ return m_pszHelpString; }
	inline EConVarType	GetType( ) const			{ return m_eVarType; }

	inline const T&	GetDefaultValue( ) const	{ return *reinterpret_cast<T*>(m_defaultValue); }
	inline const T&	GetMinValue( ) const		{ return *reinterpret_cast<T*>(m_minValue); }
	inline const T&	GetMaxValue( ) const		{ return *reinterpret_cast<T*>(m_maxValue); }

	inline void SetDefaultValue(const T& value)	{ *reinterpret_cast<T*>(m_defaultValue) = value; }
	inline void SetMinValue(const T& value)		{ *reinterpret_cast<T*>(m_minValue) = value; }
	inline void SetMaxValue(const T& value)		{ *reinterpret_cast<T*>(m_maxValue) = value; }

	inline const T&	GetValue( int index = 0 ) const	{ return m_value[index]; }
	inline void SetValue(const T& value, int index = 0)		{ m_value[index] = value; }

	inline bool		IsFlagSet( int64_t flag ) const		{ return ( flag & m_nFlags ) ? true : false; }
	inline void		AddFlags( int64_t flags )			{ m_nFlags |= flags; }
	inline void		RemoveFlags( int64_t flags )		{ m_nFlags &= ~flags; }
	inline int64_t	GetFlags( void ) const				{ return m_nFlags; }

	T m_value[MAX_SPLITSCREEN_CLIENTS];
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

	virtual ConCommandHandle	FindCommand( const char *name ) = 0;
	virtual ConCommandHandle	FindFirstCommand() = 0;
	virtual ConCommandHandle	FindNextCommand( ConCommandHandle prev ) = 0;
	virtual void				DispatchConCommand( ConCommandHandle cmd, const CCommandContext &ctx, const CCommand &args ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( BaseConVar* ref, CSplitScreenSlot nSlot, const char *pOldString, float flOldValue ) = 0;
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
	virtual void		RegisterConVar( const ConVarCreation_t& setup, int64 nAdditionalFlags, ConVarHandle* pCvarRef, ConVarBaseData_t** pCvar ) = 0;
	virtual void		UnregisterConVar( ConVarHandle handle ) = 0;
	virtual ConVarBaseData_t*	GetConVar( ConVarHandle handle ) = 0;

	// Register, unregister commands
	virtual ConCommandHandle	RegisterConCommand( const ConCommandCreation_t& setup, int64 nAdditionalFlags = 0 ) = 0;
	virtual void				UnregisterConCommand( ConCommandHandle handle ) = 0;
	virtual ConCommand*			GetCommand( ConCommandHandle handle ) = 0;

	virtual void QueueThreadSetValue( BaseConVar* ref, CSplitScreenSlot nSlot, CVValue_t* value ) = 0;
};

//-----------------------------------------------------------------------------
// These global names are defined by tier1.h, duplicated here so you
// don't have to include tier1.h
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );


#endif // ICVAR_H
