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
	inline CVValue_t& operator=(const T& other);

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

template<> inline CVValue_t& CVValue_t::operator=<bool>( const bool& other )				{ m_bValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int16_t>( const int16_t& other )			{ m_i16Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint16_t>( const uint16_t& other )		{ m_u16Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int32_t>( const int32_t& other )			{ m_i32Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint32_t>( const uint32_t& other )		{ m_u32Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<int64_t>( const int64_t& other )			{ m_i64Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<uint64_t>( const uint64_t& other )		{ m_u64Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<float>( const float& other )				{ m_flValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<double>( const double& other )			{ m_dbValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<const char*>( const char*const& other )	{ m_szValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<Color>( const Color& other )				{ m_clrValue = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<Vector2D>( const Vector2D& other )		{ m_vec2Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<Vector>( const Vector& other )			{ m_vec3Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<Vector4D>( const Vector4D& other )		{ m_vec4Value = other; return *this; }
template<> inline CVValue_t& CVValue_t::operator=<QAngle>( const QAngle& other )			{ m_angValue = other; return *this; }

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

	uint16_t GetConCommandIndex() const { return m_concommandIndex; }
	uint32_t GetIndex() const { return m_handleIndex; }

private:
	uint32_t m_concommandIndex;
	uint32_t m_handleIndex;
};

static const ConCommandHandle INVALID_CONCOMMAND_HANDLE = ConCommandHandle();

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

	inline const char*	GetName( void ) const			{ return m_pszName; }
	inline const char*	GetDescription( void ) const	{ return m_pszHelpString; }
	inline EConVarType	GetType( void ) const			{ return m_eVarType; }

	inline bool HasDefaultValue( ) const	{ return m_defaultValue != nullptr; }
	inline bool HasMinValue( ) const		{ return m_minValue != nullptr; }
	inline bool HasMaxValue( ) const		{ return m_maxValue  != nullptr; }

	inline int		GetTimesChanges( void ) const		{ return m_iTimesChanged; }

	inline bool		IsFlagSet( int64_t flag ) const		{ return ( flag & m_nFlags ) ? true : false; }
	inline void		AddFlags( int64_t flags )			{ m_nFlags |= flags; }
	inline void		RemoveFlags( int64_t flags )		{ m_nFlags &= ~flags; }
	inline int64_t	GetFlags( void ) const				{ return m_nFlags; }

	inline int		GetCallbackIndex( void ) const		{ return m_iCallbackIndex; }
protected:
	template<typename T>
	static inline void ValueToString( const T& val, char* dst, size_t length );

	template<typename T>
	static T ValueFromString( const char* val );

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

template<> inline void ConVarBaseData_t::ValueToString<bool>( const bool& val, char* dst, size_t length )
{
	const char* src = ( val ) ? "true" : "false";
	memcpy( dst, src, length );
}
template<> inline void ConVarBaseData_t::ValueToString<uint16_t>( const uint16_t& val, char* dst, size_t length )		{ snprintf( dst, length, "%u", val ); }
template<> inline void ConVarBaseData_t::ValueToString<int16_t>( const int16_t& val, char* dst, size_t length )			{ snprintf( dst, length, "%d", val ); }
template<> inline void ConVarBaseData_t::ValueToString<uint32_t>( const uint32_t& val, char* dst, size_t length )		{ snprintf( dst, length, "%u", val ); }
template<> inline void ConVarBaseData_t::ValueToString<int32_t>( const int32_t& val, char* dst, size_t length )			{ snprintf( dst, length, "%d", val ); }
template<> inline void ConVarBaseData_t::ValueToString<uint64_t>( const uint64_t& val, char* dst, size_t length )		{ snprintf( dst, length, "%lu", val ); }
template<> inline void ConVarBaseData_t::ValueToString<int64_t>( const int64_t& val, char* dst, size_t length )			{ snprintf( dst, length, "%ld", val ); }
template<> inline void ConVarBaseData_t::ValueToString<float>( const float& val, char* dst, size_t length )				{ snprintf( dst, length, "%f", val ); }
template<> inline void ConVarBaseData_t::ValueToString<double>( const double& val, char* dst, size_t length )			{ snprintf( dst, length, "%lf", val ); }
template<> inline void ConVarBaseData_t::ValueToString<const char*>( const char*const& val, char* dst, size_t length )	{ memcpy( dst, val, length ); }
template<> inline void ConVarBaseData_t::ValueToString<Color>( const Color& val, char* dst, size_t length )				{ snprintf( dst, length, "%d %d %d %d", val.r(), val.g(), val.b(), val.a() ); }
template<> inline void ConVarBaseData_t::ValueToString<Vector2D>( const Vector2D& val, char* dst, size_t length )		{ snprintf( dst, length, "%f %f", val.x, val.y ); }
template<> inline void ConVarBaseData_t::ValueToString<Vector>( const Vector& val, char* dst, size_t length )			{ snprintf( dst, length, "%f %f %f", val.x, val.y, val.z ); }
template<> inline void ConVarBaseData_t::ValueToString<Vector4D>( const Vector4D& val, char* dst, size_t length )		{ snprintf( dst, length, "%f %f %f %f", val.x, val.y, val.z, val.w ); }
template<> inline void ConVarBaseData_t::ValueToString<QAngle>( const QAngle& val, char* dst, size_t length )			{ snprintf( dst, length, "%f %f %f", val.x, val.y, val.z ); }

template<> inline bool ConVarBaseData_t::ValueFromString<bool>( const char* val )				{ if (strcmp(val, "true") == 0 || strcmp(val, "1") == 0) { return true; } return false; }
template<> inline uint16_t ConVarBaseData_t::ValueFromString<uint16_t>( const char* val )		{ unsigned int ret; sscanf(val, "%u", &ret); return ret; }
template<> inline int16_t ConVarBaseData_t::ValueFromString<int16_t>( const char* val )			{ int ret; sscanf(val, "%d", &ret); return ret; }
template<> inline uint32_t ConVarBaseData_t::ValueFromString<uint32_t>( const char* val )		{ uint32_t ret; sscanf(val, "%u", &ret); return ret; }
template<> inline int32_t ConVarBaseData_t::ValueFromString<int32_t>( const char* val )			{ int32_t ret; sscanf(val, "%d", &ret); return ret; }
template<> inline uint64_t ConVarBaseData_t::ValueFromString<uint64_t>( const char* val )		{ uint64_t ret; sscanf(val, "%lu", &ret); return ret; }
template<> inline int64_t ConVarBaseData_t::ValueFromString<int64_t>( const char* val )			{ int64_t ret; sscanf(val, "%ld", &ret); return ret; }
template<> inline float ConVarBaseData_t::ValueFromString<float>( const char* val )				{ float ret; sscanf(val, "%f", &ret); return ret; }
template<> inline double ConVarBaseData_t::ValueFromString<double>( const char* val )			{ double ret; sscanf(val, "%lf", &ret); return ret; }
template<> inline const char* ConVarBaseData_t::ValueFromString<const char*>( const char* val )	{ return val; }
template<> inline Color ConVarBaseData_t::ValueFromString<Color>( const char* val )				{ int r, g, b, a; sscanf(val, "%d %d %d %d", &r, &g, &b, &a); return Color(r, g, b, a); }
template<> inline Vector2D ConVarBaseData_t::ValueFromString<Vector2D>( const char* val )		{ float x, y; sscanf(val, "%f %f", &x, &y); return Vector2D(x, y); }
template<> inline Vector ConVarBaseData_t::ValueFromString<Vector>( const char* val )			{ float x, y, z; sscanf(val, "%f %f %f", &x, &y, &z); return Vector(x, y, z); }
template<> inline Vector4D ConVarBaseData_t::ValueFromString<Vector4D>( const char* val )		{ float x, y, z, w; sscanf(val, "%f %f %f %f", &x, &y, &z, &w); return Vector4D(x, y, z, w); }
template<> inline QAngle ConVarBaseData_t::ValueFromString<QAngle>( const char* val )			{ float x, y, z; sscanf(val, "%f %f %f", &x, &y, &z); return QAngle(x, y, z); }

template<typename T>
struct ConVarData_t : ConVarBaseData_t
{
public:
friend class ConVar<T>;
	constexpr ConVarData_t()
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

	inline const T&	GetDefaultValue( ) const	{ return *reinterpret_cast<T*>(m_defaultValue); }
	inline const T&	GetMinValue( ) const		{ return *reinterpret_cast<T*>(m_minValue); }
	inline const T&	GetMaxValue( ) const		{ return *reinterpret_cast<T*>(m_maxValue); }

	inline void SetDefaultValue(const T& value)
	{
		if (!m_defaultValue)
		{
			m_defaultValue = new T();
		}
		*reinterpret_cast<T*>(m_defaultValue) = value;
	}
	inline void SetMinValue(const T& value)
	{
		if (!m_minValue)
		{
			m_minValue = new T();
		}
		*reinterpret_cast<T*>(m_minValue) = value;
	}
	inline void SetMaxValue(const T& value)
	{
		if (!m_maxValue)
		{
			m_maxValue = new T();
		}
		*reinterpret_cast<T*>(m_maxValue) = value;
	}

	inline void RemoveDefaultValue( )
	{
		if (m_defaultValue)
		{
			delete reinterpret_cast<T*>(m_defaultValue);
		}
		m_defaultValue = nullptr;
	}
	inline void RemoveMinValue( )
	{
		if (m_minValue)
		{
			delete reinterpret_cast<T*>(m_minValue);
		}
		m_minValue = nullptr;
	}
	inline void RemoveMaxValue( )
	{
		if (m_maxValue)
		{
			delete reinterpret_cast<T*>(m_maxValue);
		}
		m_maxValue = nullptr;
	}

	inline const T& Clamp(const T& value) const
	{
		if (HasMinValue() && value < GetMinValue())
		{
			return GetMinValue();
		}
		if (HasMaxValue() && value > GetMaxValue())
		{
			return GetMaxValue();
		}
		return value;
	}

	inline const T&	GetValue( const CSplitScreenSlot& index = 0 ) const		{ return m_value[index]; }
	inline void SetValue(const T& value, const CSplitScreenSlot& index = 0) { m_value[index] = value; }

	inline void GetStringValue( char* dst, size_t len, const CSplitScreenSlot& index = 0 ) const { ValueToString( GetValue( index ), dst, len ); }

	inline void GetStringDefaultValue( char* dst, size_t len ) const	{ ValueToString( GetDefaultValue( ), dst, len ); }
	inline void GetStringMinValue( char* dst, size_t len ) const		{ ValueToString( GetMaxValue( ), dst, len ); }
	inline void GetStringMaxValue( char* dst, size_t len ) const		{ ValueToString( GetValue( index ), dst, len ); }

	inline void SetStringValue( const char* src, const CSplitScreenSlot& index = 0 ) const { SetValue( ValueFromString( src ), index ); }

	inline void SetStringDefaultValue( const char* src ) const	{ SetDefaultValue( ValueFromString( src ) ); }
	inline void SetStringMinValue( const char* src ) const		{ SetMinValue( ValueFromString( src ) ); }
	inline void SetStringMaxValue( const char* src ) const		{ SetMaxValue( ValueFromString( src ) ); }

protected:
	static inline void ValueToString( const T& value, char* dst, size_t length ) { ConVarBaseData_t::ValueToString<T>( value, dst, length ); };

	static T ValueFromString( const char* val ) { return ConVarBaseData_t::ValueFromString<T>( val ); };

	T m_value[MAX_SPLITSCREEN_CLIENTS];
};

// Special case for string handling
template<> inline void ConVarData_t<const char*>::SetValue(const char*const& value, const CSplitScreenSlot& index)
{
	char* data = new char[256];
	memcpy(data, value, 256);

	delete[] m_value[index];
	m_value[index] = data;
}

// For some types it makes no sense to clamp
template<> inline const char*const& ConVarData_t<const char*>::Clamp(const char*const& value) const { return value; }
template<> inline const Color& ConVarData_t<Color>::Clamp(const Color& value) const { return value; }
template<> inline const Vector2D& ConVarData_t<Vector2D>::Clamp(const Vector2D& value) const { return value; }
template<> inline const Vector& ConVarData_t<Vector>::Clamp(const Vector& value) const { return value; }
template<> inline const Vector4D& ConVarData_t<Vector4D>::Clamp(const Vector4D& value) const { return value; }
template<> inline const QAngle& ConVarData_t<QAngle>::Clamp(const QAngle& value) const { return value; }

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
	virtual void			CallChangeCallback( ConVarHandle cvarid, const CSplitScreenSlot nSlot, const CVValue_t* pNewValue, const CVValue_t* pOldValue ) = 0;

	virtual ConCommandHandle	FindCommand( const char *name ) = 0;
	virtual ConCommandHandle	FindFirstCommand() = 0;
	virtual ConCommandHandle	FindNextCommand( ConCommandHandle prev ) = 0;
	virtual void				DispatchConCommand( ConCommandHandle cmd, const CCommandContext &ctx, const CCommand &args ) = 0;

	// Install a global change callback (to be called when any convar changes) 
	virtual void			InstallGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			RemoveGlobalChangeCallback( FnChangeCallbackGlobal_t callback ) = 0;
	virtual void			CallGlobalChangeCallbacks( BaseConVar* ref, CSplitScreenSlot nSlot, const char* newValue, char* oldValue ) = 0;
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
