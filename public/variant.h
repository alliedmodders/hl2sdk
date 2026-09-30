#ifndef CVARIANT_H
#define CVARIANT_H

#if _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "string_t.h"
#include "vector.h"
#include "vector2d.h"
#include "vector4d.h"
#include "vectorws.h"
#include "Color.h"
#include "entity2/entityidentity.h"
#include "entityhandle.h"
#include "tier1/bufferstring.h"
#include "tier1/utlscratchmemory.h"
#include "resourcefile/resourcetype.h"
#include "mathlib.h"

#include "tier0/memdbgon.h"

FORWARD_DECLARE_HANDLE( HSCRIPT );

typedef enum _fieldtypes : uint8
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT32,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INT32,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_INT16,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_xxxAvail1,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_xxxAvail2,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_UNUSED,

	FIELD_VECTOR2D,			// 2 floats
	FIELD_INT64,			// 64bit integer

	FIELD_VECTOR4D,			// 4 floats

	FIELD_RESOURCE,

	FIELD_TYPEUNKNOWN,

	FIELD_CSTRING,
	FIELD_HSCRIPT,
	FIELD_VARIANT,
	FIELD_UINT64,
	FIELD_FLOAT64,
	FIELD_POSITIVEINTEGER_OR_NULL,
	FIELD_HSCRIPT_NEW_INSTANCE,
	FIELD_UINT32,
	FIELD_UTLSTRINGTOKEN,
	FIELD_QANGLE,
	FIELD_NETWORK_ORIGIN_CELL_QUANTIZED_VECTOR,
	FIELD_HMATERIAL,
	FIELD_HMODEL,
	FIELD_NETWORK_QUANTIZED_VECTOR,
	FIELD_NETWORK_QUANTIZED_FLOAT,
	FIELD_DIRECTION_VECTOR_WORLDSPACE,
	FIELD_QANGLE_WORLDSPACE,
	FIELD_QUATERNION_WORLDSPACE,
	FIELD_HSCRIPT_LIGHTBINDING,
	FIELD_V8_VALUE,
	FIELD_V8_OBJECT,
	FIELD_V8_ARRAY,
	FIELD_V8_CALLBACK_INFO,
	FIELD_UTLSTRING,

	FIELD_NETWORK_ORIGIN_CELL_QUANTIZED_POSITION_VECTOR,
	FIELD_HRENDERTEXTURE,

	FIELD_HPARTICLESYSTEMDEFINITION,
	FIELD_UINT8,
	FIELD_UINT16,
	FIELD_CTRANSFORM,
	FIELD_CTRANSFORM_WORLDSPACE,
	FIELD_HPOSTPROCESSING,
	FIELD_MATRIX3X4,
	FIELD_SHIM,
	FIELD_CMOTIONTRANSFORM,
	FIELD_CMOTIONTRANSFORM_WORLDSPACE,
	FIELD_ATTACHMENT_HANDLE,
	FIELD_AMMO_INDEX,
	FIELD_CONDITION_ID,
	DEPRECATED_FIELD_AI_SCHEDULE_BITS,
	FIELD_MODIFIER_HANDLE,
	FIELD_ROTATION_VECTOR,
	FIELD_ROTATION_VECTOR_WORLDSPACE,
	FIELD_HVDATA,
	FIELD_SCALE32,
	FIELD_STRING_AND_TOKEN,
	FIELD_ENGINE_TIME,
	FIELD_ENGINE_TICK,
	FIELD_WORLD_GROUP_ID,
	FIELD_GLOBALSYMBOL,
	FIELD_HNMGRAPHDEFINITION,
	FIELD_NETWORK_QUANTIZED_VECTORWS,
	FIELD_NETWORK_ORIGIN_CELL_QUANTIZED_VECTORWS,
	
	FIELD_TYPECOUNT
} fieldtype_t;

// ========

class CVariantDefaultAllocator
{
public:
	enum { ALWAYS_COPY = 0 };

	static void *Allocate( uint nSize )
	{
		return malloc( nSize );
	}

	static void Free( void *pMemory )
	{
		free( pMemory );
	}
};

class CEntityVariantAllocator
{
public:
	enum { ALWAYS_COPY = 0 };

	static void Free( void *pMemory ) { /* Skipped intentionally */ }

	static void *Allocate( uint nSize )
	{
		return sm_pMemoryPool->AllocAligned( nSize, 8 * (nSize >= 16) + 8 );
	}

	static void Activate( CUtlScratchMemoryPool *pMemoryPool, bool bEnable )
	{
		sm_pMemoryPool = bEnable ? pMemoryPool : nullptr;
	}

private:
	static CUtlScratchMemoryPool *sm_pMemoryPool;
};

enum CVFlags_t
{
	// Indicates that variant has the memory allocated in place of a primitive types and it would be freed when needed
	CV_FREE = 0x01,
};

template <typename CValueAllocator = CVariantDefaultAllocator>
class CVariantBase
{
public:
	CVariantBase() :						m_type( FIELD_VOID ), m_flags( 0 )			{ m_pData = nullptr; }
	CVariantBase( uint8 val ) :				m_type( FIELD_UINT8 ), m_flags( 0 )			{ m_uint8 = val;}
	CVariantBase( int16 val ) :				m_type( FIELD_INT16 ), m_flags( 0 )			{ m_int16 = val;}
	CVariantBase( uint16 val ) :			m_type( FIELD_UINT16 ), m_flags( 0 )		{ m_uint16 = val;}
	CVariantBase( int32 val ) :				m_type( FIELD_INT32 ), m_flags( 0 )			{ m_int32 = val;}
	CVariantBase( uint32 val) :				m_type( FIELD_UINT32 ), m_flags( 0 )		{ m_uint32 = val; }
	CVariantBase( int64 val ) :				m_type( FIELD_INT64 ), m_flags( 0 )			{ m_int64 = val; }
	CVariantBase( uint64 val ) :			m_type( FIELD_UINT64), m_flags( 0 )			{ m_uint64 = val; }
	CVariantBase( float32 val ) :			m_type( FIELD_FLOAT32 ), m_flags( 0 )		{ m_float32 = val; }
	CVariantBase( float64 val ) :			m_type( FIELD_FLOAT64 ), m_flags( 0 )		{ m_float64 = val; }
	CVariantBase( char val ) :				m_type( FIELD_CHARACTER ), m_flags( 0 )		{ m_char = val; }
	CVariantBase( bool val ) :				m_type( FIELD_BOOLEAN ), m_flags( 0 )		{ m_bool = val; }
	CVariantBase( HSCRIPT val ) :			m_type( FIELD_HSCRIPT ), m_flags( 0 )		{ m_hScript = val; }
	CVariantBase( CEntityHandle val ) :		m_type( FIELD_EHANDLE ), m_flags( 0 )		{ m_hEntity = val; }
	CVariantBase( CUtlStringToken val ) :	m_type( FIELD_UTLSTRINGTOKEN ), m_flags( 0 ){ m_utlStringToken = val; }
	CVariantBase( ResourceHandle_t val ) :	m_type( FIELD_RESOURCE ), m_flags( 0 )		{ m_hResource = val; }
	CVariantBase( string_t val ) :			m_type( FIELD_STRING ), m_flags( 0 )		{ m_stringt = val; }
	CVariantBase( color32 val ) :			m_type( FIELD_COLOR32 ), m_flags( 0 )		{ m_color32 = val; }
	CVariantBase( Color val ) :				m_type( FIELD_COLOR32 ), m_flags( 0 )		{ m_color32 = val.ToColor32(); }

	CVariantBase( const Vector &val, bool bCopy = false ) :		m_type( FIELD_VECTOR ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector *val, bool bCopy = false ) :		m_type( FIELD_VECTOR ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const QAngle &val, bool bCopy = false ) :		m_type( FIELD_QANGLE ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const QAngle *val, bool bCopy = false ) :		m_type( FIELD_QANGLE ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const Vector2D &val, bool bCopy = false ) :	m_type( FIELD_VECTOR2D ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector2D *val, bool bCopy = false ) :	m_type( FIELD_VECTOR2D ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const Vector4D &val, bool bCopy = false ) :	m_type( FIELD_VECTOR4D ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector4D *val, bool bCopy = false ) :	m_type( FIELD_VECTOR4D ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const VectorWS &val, bool bCopy = false ) :	m_type( FIELD_POSITION_VECTOR ), m_flags( 0 ){ CopyData(val, bCopy); }
	CVariantBase( const VectorWS *val, bool bCopy = false ) :	m_type( FIELD_POSITION_VECTOR ), m_flags( 0 ){ CopyData(*val, bCopy); }
	CVariantBase( const Quaternion &val, bool bCopy = false ) :	m_type( FIELD_QUATERNION ), m_flags( 0 )	{ CopyData(val, bCopy); }
	CVariantBase( const Quaternion *val, bool bCopy = false ) :	m_type( FIELD_QUATERNION ), m_flags( 0 )	{ CopyData(*val, bCopy); }
	CVariantBase( const char *val, bool bCopy = false ) :		m_type( FIELD_CSTRING ), m_flags( 0 )		{ CopyData(val, bCopy); }

	CVariantBase( const CVariantBase<CValueAllocator> &variant ) : m_flags( 0 ), m_type( FIELD_VOID ) { variant.AssignTo( this ); }
	void operator=( const CVariantBase<CValueAllocator> &variant ) { variant.AssignTo( this ); }

	// Checks if the stored value is of type FIELD_VOID
	bool IsNull() const						{ return (m_type == FIELD_VOID ); }

	operator int32() const					{ Assert( m_type == FIELD_INT32 );			return m_int32; }
	operator uint32() const					{ Assert( m_type == FIELD_UINT32 );			return m_uint32; }
	operator int64() const					{ Assert( m_type == FIELD_INT64);			return m_int64; }
	operator uint64() const					{ Assert( m_type == FIELD_UINT64);			return m_uint64; }
	operator float32() const				{ Assert( m_type == FIELD_FLOAT32 );		return m_float32; }
	operator float64() const				{ Assert( m_type == FIELD_FLOAT64 );		return m_float64; }
	operator const string_t() const			{ Assert( m_type == FIELD_STRING );			return m_stringt; }
	operator const char *() const			{ Assert( m_type == FIELD_CSTRING );		return ( m_pszString ) ? m_pszString : ""; }
	operator const Vector &() const			{ Assert( m_type == FIELD_VECTOR );			static Vector vecNull(0, 0, 0); return (m_pVector) ? *m_pVector : vecNull; }
	operator const VectorWS &() const		{ Assert( m_type == FIELD_POSITION_VECTOR );static VectorWS vecNull(0, 0, 0); return (m_pVectorWS) ? *m_pVectorWS : vecNull; }
	operator const Vector2D &() const		{ Assert( m_type == FIELD_VECTOR2D );		static Vector2D vecNull(0, 0); return (m_pVector2D) ? *m_pVector2D : vecNull; }
	operator const Vector4D &() const		{ Assert( m_type == FIELD_VECTOR4D );		static Vector4D vecNull(0, 0, 0, 0); return (m_pVector4D) ? *m_pVector4D : vecNull; }
	operator const QAngle &() const			{ Assert( m_type == FIELD_QANGLE);			static QAngle angNull(0, 0, 0); return (m_pQAngle) ? *m_pQAngle : angNull; }
	operator const Quaternion &() const		{ Assert( m_type == FIELD_QUATERNION);		static Quaternion quatNull(0, 0, 0, 0); return (m_pQuaternion) ? *m_pQuaternion : quatNull; }
	operator Color() const					{ Assert( m_type == FIELD_COLOR32 );		return Color( m_color32 ); }
	operator color32() const				{ Assert( m_type == FIELD_COLOR32 );		return m_color32; }
	operator char() const					{ Assert( m_type == FIELD_CHARACTER );		return m_char; }
	operator bool() const					{ Assert( m_type == FIELD_BOOLEAN );		return m_bool; }
	operator HSCRIPT() const				{ Assert( m_type == FIELD_HSCRIPT );		return m_hScript; }
	operator CEntityHandle() const			{ Assert( m_type == FIELD_EHANDLE);			return m_hEntity; }
	operator CUtlStringToken() const		{ Assert( m_type == FIELD_UTLSTRINGTOKEN);	return m_utlStringToken; }
	operator ResourceHandle_t() const		{ Assert( m_type == FIELD_RESOURCE);		return m_hResource; }

	void operator=( int32 i ) 				{ Free(); m_type = FIELD_INT32; m_int32 = i; }
	void operator=( uint32 u )				{ Free(); m_type = FIELD_UINT32; m_uint32 = u; }
	void operator=( int64 i ) 				{ Free(); m_type = FIELD_INT64; m_int64 = i; }
	void operator=( uint64 u )				{ Free(); m_type = FIELD_UINT64; m_uint64 = u; }
	void operator=( float32 f ) 			{ Free(); m_type = FIELD_FLOAT32; m_float32 = f; }
	void operator=( float64 d )				{ Free(); m_type = FIELD_FLOAT64; m_float64 = d; }
	void operator=( const Vector &vec )		{ CopyData( vec ); }
	void operator=( const Vector *vec )		{ CopyData( *vec ); }
	void operator=( const Vector2D &vec )	{ CopyData( vec ); }
	void operator=( const Vector2D *vec )	{ CopyData( *vec ); }
	void operator=( const Vector4D &vec )	{ CopyData( vec ); }
	void operator=( const Vector4D *vec )	{ CopyData( *vec ); }
	void operator=( const VectorWS &vec )	{ CopyData( vec ); }
	void operator=( const VectorWS *vec )	{ CopyData( *vec ); }
	void operator=( const QAngle &ang )		{ CopyData( ang ); }
	void operator=( const QAngle *ang )		{ CopyData( *ang ); }
	void operator=( const Quaternion &quat ){ CopyData( quat ); }
	void operator=( const Quaternion *quat ){ CopyData( *quat ); }
	void operator=( const char *psz )		{ CopyData( psz ); }
	void operator=( string_t psz )			{ CopyData( psz.ToCStr() ); }
	void operator=( Color color )			{ Free(); m_type = FIELD_COLOR32; m_color32 = color.ToColor32(); }
	void operator=( color32 color )			{ Free(); m_type = FIELD_COLOR32; m_color32 = color; }
	void operator=( char c )				{ Free(); m_type = FIELD_CHARACTER; m_char = c; }
	void operator=( bool b ) 				{ Free(); m_type = FIELD_BOOLEAN; m_bool = b; }
	void operator=( HSCRIPT h ) 			{ Free(); m_type = FIELD_HSCRIPT; m_hScript = h; }
	void operator=( CEntityHandle eh) 		{ Free(); m_type = FIELD_EHANDLE; m_hEntity = eh; }
	void operator=( CUtlStringToken tok ) 	{ Free(); m_type = FIELD_UTLSTRINGTOKEN; m_utlStringToken = tok; }
	void operator=( ResourceHandle_t r ) 	{ Free(); m_type = FIELD_RESOURCE; m_hResource = r; }

	~CVariantBase()
	{
		Free();
	}

	// Frees the internal buffer and resets the value to be FIELD_VOID
	void Free();

	template <typename T> T Get() const;

	// Copies the contents of the value into a pDest, also converts the content when possible
	template <typename T> bool AssignTo( T *pDest ) const;
	template <typename T> bool AssignTo( CVariantBase<T> *pDest ) const;
	bool AssignTo( CBufferString &buf ) const;
	bool AssignTo(float *pDest) const;
	bool AssignTo(int *pDest) const;
	bool AssignTo(bool *pDest) const;
	bool AssignTo(string_t *pDest) const;
	bool AssignTo(Vector *pDest) const;
	bool AssignTo(VectorWS *pDest) const;
	bool AssignTo(Vector2D *pDest) const;
	bool AssignTo(Vector4D *pDest) const;
	bool AssignTo(Quaternion *pDest) const;
	bool AssignTo(QAngle *pDest) const;
	bool AssignTo(Color *pDest) const;
	bool AssignTo(ResourceHandle_t *pDest) const;
	bool AssignTo(HSCRIPT *pDest) const;
	bool AssignTo(CUtlStringToken *pDest) const;
	bool AssignTo(CEntityHandle *pDest) const;
	bool AssignTo(CEntityInstance **pDest) const;
	
	// Converts underlying value into a different type
	bool Convert( fieldtype_t newType );

	const char *ToString() const;

	int GetType() const			{ return m_type; }
	uint16 GetFlags() const		{ return m_flags; }

	// Allocates own buffers and copies the internal value when needed, if silent = false, emits a global warning
	void ConvertToCopiedData( bool silent = true );

private:
	void *Allocate( uint nSize );
	template< typename T > T *Allocate();
	void Free( void *pMemory );

	// Copies the src data into an internal value, setting bForceCopy would allocate its own memory to store the contents
	template <typename T>
	void CopyData( const T &src, bool bForceCopy = false );
	void CopyData( const char *src, bool bForceCopy = false );

	// Sets the internal value to the pData content, doesn't allocate memory nor copies the content.
	// Be sure to call ConvertToCopiedData() when required
	void Set( fieldtype_t ftype, void *pData );

	union
	{
		uint8 m_uint8;
		int16 m_int16;
		uint16 m_uint16;
		int32 m_int32;
		uint32 m_uint32;
		int64 m_int64;
		uint64 m_uint64;
		float32 m_float32;
		float64 m_float64;
		const char *m_pszString;
		const Vector *m_pVector;
		const QAngle *m_pQAngle;
		const Vector2D *m_pVector2D;
		const Vector4D *m_pVector4D;
		const VectorWS *m_pVectorWS;
		const Quaternion *m_pQuaternion;
		color32 m_color32;
		void *m_pData;
		char m_char;
		bool m_bool;
		HSCRIPT m_hScript;
		CEntityHandle m_hEntity;
		string_t m_stringt;
		CUtlStringToken m_utlStringToken;
		ResourceHandle_t m_hResource;
	};

	fieldtype_t m_type;

	// CVFlags_t flags
	uint16 m_flags;
};

typedef CVariantBase<> CVariant;
typedef CVariantBase<CEntityVariantAllocator> CEntityVariant;

typedef CVariant variant_t;

template <typename T> struct VariantDeducer_t { enum { FIELD_TYPE = FIELD_TYPEUNKNOWN }; };
#define DECLARE_DEDUCE_VARIANT_FIELDTYPE( fieldType, type ) template<> struct VariantDeducer_t<type> { enum { FIELD_TYPE = fieldType }; };

DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VOID, void );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_CSTRING, const char * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_CSTRING, char * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR, Vector );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR, Vector * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR, const Vector & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_POSITION_VECTOR, VectorWS );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_POSITION_VECTOR, VectorWS * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_POSITION_VECTOR, const VectorWS & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR2D, Vector2D );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR2D, Vector2D * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR2D, const Vector2D & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR4D, Vector4D );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR4D, Vector4D * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_VECTOR4D, const Vector4D & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QANGLE, QAngle );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QANGLE, QAngle * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QANGLE, const QAngle & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QUATERNION, Quaternion );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QUATERNION, Quaternion * );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_QUATERNION, const Quaternion & );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_COLOR32, color32 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_STRING, string_t );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_BOOLEAN, bool );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_CHARACTER, char );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_FLOAT32, float32 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_FLOAT64, float64 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_UINT8, uint8 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_INT16, int16 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_UINT16, uint16 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_INT32, int32 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_UINT32, uint32 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_INT64, int64 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_UINT64, uint64 );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_HSCRIPT, HSCRIPT );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_EHANDLE, CEntityHandle );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_RESOURCE, ResourceHandle_t );
DECLARE_DEDUCE_VARIANT_FIELDTYPE( FIELD_UTLSTRINGTOKEN, CUtlStringToken );

#define VariantDeduceType( T ) ((fieldtype_t)VariantDeducer_t<T>::FIELD_TYPE)
#undef DECLARE_DEDUCE_VARIANT_FIELDTYPE

template <typename T>
inline const char *VariantFieldTypeName()
{
	return T::using_unknown_variant_type();
}

#define DECLARE_NAMED_VARIANT_FIELDTYPE( fieldType, strName ) template <> inline const char * VariantFieldTypeName<fieldType>() { return strName; }

DECLARE_NAMED_VARIANT_FIELDTYPE( void, "void" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const char *, "cstring" );
DECLARE_NAMED_VARIANT_FIELDTYPE( char *, "cstring" );
DECLARE_NAMED_VARIANT_FIELDTYPE( Vector2D, "vector2d" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const Vector2D &, "vector2d" );
DECLARE_NAMED_VARIANT_FIELDTYPE( Vector, "vector" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const Vector &, "vector" );
DECLARE_NAMED_VARIANT_FIELDTYPE( Vector4D, "vector4d" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const Vector4D &, "vector4d" );
DECLARE_NAMED_VARIANT_FIELDTYPE( VectorWS, "vectorws" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const VectorWS &, "vectorws" );
DECLARE_NAMED_VARIANT_FIELDTYPE( QAngle, "qangle" );
DECLARE_NAMED_VARIANT_FIELDTYPE( const QAngle &, "qangle" );
DECLARE_NAMED_VARIANT_FIELDTYPE( color32, "color" );
DECLARE_NAMED_VARIANT_FIELDTYPE( bool, "boolean" );
DECLARE_NAMED_VARIANT_FIELDTYPE( char, "character" );
DECLARE_NAMED_VARIANT_FIELDTYPE( uint8, "uint8" );
DECLARE_NAMED_VARIANT_FIELDTYPE( int16, "int16" );
DECLARE_NAMED_VARIANT_FIELDTYPE( uint16, "uint16" );
DECLARE_NAMED_VARIANT_FIELDTYPE( int32, "int32" );
DECLARE_NAMED_VARIANT_FIELDTYPE( uint32, "uint32" );
DECLARE_NAMED_VARIANT_FIELDTYPE( int64, "int64" );
DECLARE_NAMED_VARIANT_FIELDTYPE( uint64, "uint64" );
DECLARE_NAMED_VARIANT_FIELDTYPE( float32, "float32" );
DECLARE_NAMED_VARIANT_FIELDTYPE( float64, "float64" );
DECLARE_NAMED_VARIANT_FIELDTYPE( HSCRIPT, "hscript" );
DECLARE_NAMED_VARIANT_FIELDTYPE( CVariant, "variant" );
DECLARE_NAMED_VARIANT_FIELDTYPE( CBaseHandle, "ehandle" );
DECLARE_NAMED_VARIANT_FIELDTYPE( Quaternion, "quaternion" );
DECLARE_NAMED_VARIANT_FIELDTYPE( CUtlStringToken, "utlstringtoken" );

#undef DECLARE_NAMED_VARIANT_FIELDTYPE

inline const char *VariantFieldTypeName( fieldtype_t eType )
{
	switch(eType)
	{
		case FIELD_VOID:					return "void";
		case FIELD_FLOAT32:					return "float32";
		case FIELD_STRING:					return "string_t";
		case FIELD_VECTOR:					return "vector";
		case FIELD_QUATERNION:				return "quaternion";
		case FIELD_INT32:					return "int32";
		case FIELD_BOOLEAN:					return "boolean";
		case FIELD_CHARACTER:				return "character";
		case FIELD_COLOR32:					return "color";
		case FIELD_EHANDLE:					return "ehandle";
		case FIELD_POSITION_VECTOR:			return "vectorws";
		case FIELD_VECTOR2D:				return "vector2d";
		case FIELD_VECTOR4D:				return "vector4d";
		case FIELD_INT64:					return "int64";
		case FIELD_RESOURCE:				return "resourcehandle";
		case FIELD_CSTRING:					return "cstring";
		case FIELD_HSCRIPT:					return "hscript";
		case FIELD_VARIANT:					return "variant";
		case FIELD_UINT64:					return "uint64";
		case FIELD_FLOAT64:					return "float64";
		case FIELD_UINT32:					return "unsigned";
		case FIELD_UTLSTRINGTOKEN:			return "utlstringtoken";
		case FIELD_QANGLE:					return "qangle";
		case FIELD_HSCRIPT_LIGHTBINDING:	return "hscript_lightbinding";
		case FIELD_V8_VALUE:				return "js_value";
		case FIELD_V8_OBJECT:				return "js_object";
		case FIELD_V8_ARRAY:				return "js_array";
		case FIELD_V8_CALLBACK_INFO:		return "js_raw_args";
		case FIELD_UINT8:					return "uint8";
		case FIELD_GLOBALSYMBOL:			return "globalsymbol";
		default:							return "unknown_variant_type";
	}
}

#define VARIANT_NULL CVariant()

//-----------------------------------------------------------------------------
// Default allocator
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline void *CVariantBase<CValueAllocator>::Allocate( uint nSize )
{
	return CValueAllocator::Allocate( nSize );
}

template< class CValueAllocator >
template< typename T >
inline T *CVariantBase<CValueAllocator>::Allocate()
{
	// ASSERT_MEMALLOC_WILL_ALIGN( T );
	return (T *)CValueAllocator::Allocate( sizeof( T ) );
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::Free( void *pMemory )
{
	return CValueAllocator::Free( pMemory );
}


//-----------------------------------------------------------------------------
// Data freeing
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::Free()
{
	if(m_flags & CV_FREE)
	{
		Free( m_pData );
		m_flags &= ~CV_FREE;
	}

	m_pData = nullptr;
	m_type = FIELD_VOID;
}


//-----------------------------------------------------------------------------
// Copy helper
//-----------------------------------------------------------------------------
template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::CopyData( const char *src, bool bForceCopy )
{
	Free();

	m_type = FIELD_CSTRING;

	if(src && (CValueAllocator::ALWAYS_COPY || bForceCopy))
	{
		int len = strlen( src ) + 1;
		m_pszString = (char *)Allocate( len );
		memcpy( (void *)m_pszString, src, len );

		m_flags |= CV_FREE;
	}
	else
	{
		m_pszString = src;
	}
}

// Copies the src data into an internal value, setting bForceCopy would allocate its own memory to store the contents
template< class CValueAllocator >
template <typename T>
inline void CVariantBase<CValueAllocator>::CopyData( const T &src, bool bForceCopy )
{
	COMPILE_TIME_ASSERT( VariantDeduceType( T ) != FIELD_TYPEUNKNOWN );

	Free();

	m_type = VariantDeduceType( T );

	if(CValueAllocator::ALWAYS_COPY || bForceCopy)
	{
		m_pData = Allocate<T>();
		*(T *)m_pData = src;

		m_flags |= CV_FREE;
	}
	else
	{
		m_pData = const_cast<T *>(&src);
	}
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::ConvertToCopiedData( bool silent )
{
	if((m_flags & CV_FREE) == 0)
	{
		switch( m_type )
		{
			case FIELD_VECTOR:		CopyData( *m_pVector, true ); break;
			case FIELD_VECTOR2D:	CopyData( *m_pVector2D, true ); break;
			case FIELD_VECTOR4D:	CopyData( *m_pVector4D, true ); break;
			case FIELD_POSITION_VECTOR:CopyData( *m_pVectorWS, true ); break;
			case FIELD_QUATERNION:	CopyData( *m_pQuaternion, true ); break;
			case FIELD_QANGLE:		CopyData( *m_pQAngle, true ); break;
			case FIELD_CSTRING:		CopyData( m_pszString, true ); break;
			default:
			{
				if(!silent)
					Warning( "Attempted to ConvertToCopiedData for unsupported type (%d)\n", m_type );
				break;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Type converting get operations
//-----------------------------------------------------------------------------
template< class CValueAllocator >
template< typename T >
inline T CVariantBase<CValueAllocator>::Get() const
{
	T value{};
	AssignTo( &value );
	return value;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CBufferString &buf ) const
{
	buf.Purge( 0 );

	switch(m_type)
	{
		case FIELD_VOID:		return false;
		case FIELD_FLOAT32:		buf.Format( "%g", m_float32 ); return true;
		case FIELD_FLOAT64:		buf.Format( "%g", m_float64 ); return true;
		case FIELD_INT32:		buf.Format( "%d", m_int32 ); return true;

		case FIELD_EHANDLE:		buf.Format( "%u", m_hEntity.ToInt() ); return true;
		case FIELD_UTLSTRINGTOKEN:buf.Format( "%u", m_utlStringToken.GetHashCode() ); return true;
		case FIELD_UINT32:		buf.Format( "%u", m_uint32 ); return true;
		case FIELD_UINT8:		buf.Format( "%u", m_uint8 ); return true;

		case FIELD_INT64:		buf.Format( "%lld", m_int64 ); return true;
		case FIELD_UINT64:		buf.Format( "%llu", m_uint64 ); return true;
		case FIELD_BOOLEAN:		buf.Insert( 0, m_bool ? "true" : "false" ); return true;
		case FIELD_STRING:		buf.Insert( 0, m_stringt.ToCStr() ); return true;
		case FIELD_CSTRING:		buf.Insert( 0, m_pszString ? m_pszString : "(null)" ); return true;
		case FIELD_CHARACTER:	buf.Format( "%c", m_char ); return true;
		case FIELD_VECTOR2D:	buf.Format( "%g %g", m_pVector2D->x, m_pVector2D->y ); return true;
		case FIELD_COLOR32:		buf.Format( "%d %d %d %d", m_color32.r, m_color32.g, m_color32.b, m_color32.a ); return true;

		case FIELD_POSITION_VECTOR:
		case FIELD_VECTOR:
		case FIELD_QANGLE:
		{
			buf.Format( "%g %g %g", m_pVector->x, m_pVector->y, m_pVector->z );
			return true;
		}

		case FIELD_QUATERNION:
		case FIELD_VECTOR4D:
		{
			buf.Format( "%g %g %g %g", m_pVector4D->x, m_pVector4D->y, m_pVector4D->z, m_pVector4D->w );
			return true;
		}

		default: Warning( "No conversion from %s to string at the moment!\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( float *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = 0.0; return false;
		case FIELD_UINT8:		*pDest = m_uint8; return true;
		case FIELD_INT32:		*pDest = m_int32; return true;
		case FIELD_INT64:		*pDest = m_int64; return true;
		case FIELD_UINT32:		*pDest = m_uint32; return true;
		case FIELD_UINT64:		*pDest = m_uint64; return true;
		case FIELD_FLOAT32:		*pDest = m_float32; return true;
		case FIELD_FLOAT64:		*pDest = m_float64; return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0])
			{
				*pDest = V_atof( m_pszString );
				return true;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				*pDest = V_atof( m_stringt.ToCStr() );
				return true;
			}
		}

		default: Warning( "No conversion from %s to float right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( int *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = 0; return false;
		case FIELD_UINT8:		*pDest = m_uint8; return true;
		case FIELD_INT32:		*pDest = m_int32; return true;
		case FIELD_INT64:		*pDest = m_int64; return true;
		case FIELD_UINT32:		*pDest = m_uint32; return true;
		case FIELD_UINT64:		*pDest = m_uint64; return true;
		case FIELD_FLOAT32:		*pDest = m_float32; return true;
		case FIELD_FLOAT64:		*pDest = m_float64; return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		case FIELD_CSTRING:
		{
			*pDest = 0;
			if(m_pszString)
				*pDest = V_atoi( m_pszString );

			return m_pszString != nullptr;
		}
		case FIELD_STRING:
		{
			*pDest = V_atoi( m_stringt.ToCStr() );
			return !m_stringt;
		}

		default: Warning( "No conversion from %s to int now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( bool *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = 0; return false;
		case FIELD_INT32:		*pDest = m_int32 != 0; return true;
		case FIELD_INT64:		*pDest = m_int64 != 0; return true;
		case FIELD_UINT32:		*pDest = m_uint32 != 0; return true;
		case FIELD_UINT64:		*pDest = m_uint64 != 0; return true;
		case FIELD_FLOAT32:		*pDest = m_float32 != 0.0; return true;
		case FIELD_FLOAT64:		*pDest = m_float64 != 0.0; return true;
		case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0])
			{
				bool successful = false;
				*pDest = V_StringToBool( m_pszString, false, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				*pDest = V_StringToBool( m_stringt.ToCStr(), false, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to bool right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( string_t *pDest ) const
{
	if(m_type == FIELD_STRING)
	{
		*pDest = m_stringt;
		return true;
	}
	else if(m_type == FIELD_CSTRING)
	{
		if(m_pszString)
		{
			*pDest = MAKE_STRING( m_pszString );
			return true;
		}
	}
	else
	{
		Warning( "No conversion from %s to string_t right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Vector *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:			*pDest = vec3_origin; return false;
		case FIELD_VECTOR:			*pDest = *m_pVector; return true;
		case FIELD_POSITION_VECTOR:	*pDest = *(Vector *)m_pVectorWS; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToVector( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToVector( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to Vector right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( VectorWS *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:			*pDest = VectorWS( vec3_origin.x, vec3_origin.y, vec3_origin.z ); return false;
		case FIELD_POSITION_VECTOR:	*pDest = *m_pVectorWS; return true;
		case FIELD_VECTOR:			*pDest = *(VectorWS *)m_pVector; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToVectorWS( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToVectorWS( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to Vector right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Vector2D *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = vec2_origin; return false;
		case FIELD_VECTOR2D:	*pDest = *m_pVector2D; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToVector2D( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToVector2D( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to Vector2D right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Vector4D *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = vec4_origin; return false;
		case FIELD_VECTOR4D:	*pDest = *m_pVector4D; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToVector4D( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToVector4D( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to Vector4D right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Quaternion *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = quat_identity; return false;
		case FIELD_QUATERNION:	*pDest = *m_pQuaternion; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToQuaternion( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToQuaternion( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}
		case FIELD_QANGLE:
		{
			AngleQuaternion( *m_pQAngle, *pDest );
			return true;
		}

		default: Warning( "No conversion from %s to Quaternion right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( QAngle *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VOID:		*pDest = vec3_angle; return false;
		case FIELD_QANGLE:		*pDest = *m_pQAngle; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToQAngle( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToQAngle( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}
		case FIELD_QUATERNION:
		{
			QuaternionAngles( *m_pQuaternion, *pDest );
			return true;
		}

		default: Warning( "No conversion from %s to QAngle right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( Color *pDest ) const
{
	switch(m_type)
	{
		case FIELD_COLOR32:		*pDest = Color( m_color32 ); return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0] != '\0')
			{
				bool successful = false;
				V_StringToColor( m_pszString, *pDest, &successful );
				return successful;
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				bool successful = false;
				V_StringToColor( m_stringt.ToCStr(), *pDest, &successful );
				return successful;
			}
		}

		default: Warning( "No conversion from %s to Color right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( ResourceHandle_t *pDest ) const
{
	if(m_type == FIELD_RESOURCE)
	{
		*pDest = m_hResource;
		return true;
	}
	else
	{
		Warning( "No conversion from %s to ResourceHandle_t right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( HSCRIPT *pDest ) const
{
	if(m_type == FIELD_HSCRIPT)
	{
		*pDest = m_hScript;
		return true;
	}
	else
	{
		Warning( "No conversion from %s to HSCRIPT right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CUtlStringToken *pDest ) const
{
	switch(m_type)
	{
		case FIELD_UTLSTRINGTOKEN: *pDest = m_utlStringToken; return true;
		case FIELD_CSTRING: *pDest = CUtlStringToken( m_pszString ); return true;
		case FIELD_STRING: *pDest = CUtlStringToken( m_stringt.ToCStr() ); return true;
		default:
		{
			Warning( "No free conversion of %s variant to CUtlStringToken right now\n", VariantFieldTypeName( m_type ) );
		}
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CEntityHandle *pDest ) const
{
	switch(m_type)
	{
		case FIELD_EHANDLE:		*pDest = m_hEntity; return true;
		case FIELD_CSTRING:
		{
			if(m_pszString && m_pszString[0])
			{
				// TODO: Perform actual entity handle lookup via CEntitySystem::FindFirstEntityHandleByName()
				Assert( false );
			}
		}
		case FIELD_STRING:
		{
			if(m_stringt.ToCStr()[0])
			{
				// TODO: Perform actual entity handle lookup via CEntitySystem::FindFirstEntityHandleByName()
				Assert( false );
			}
		}

		default: Warning( "No conversion from %s to EHANDLE right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::AssignTo( CEntityInstance **pDest ) const
{
	if(m_type == FIELD_EHANDLE)
	{
		*pDest = m_hEntity.Get();
		return true;
	}
	else
	{
		Warning( "No conversion from %s to CEntityInstance * right now\n", VariantFieldTypeName( m_type ) );
	}

	return false;
}

// Copies the contents of the value into a pDest, also converts the content when possible
template< class CValueAllocator >
template< typename T >
inline bool CVariantBase<CValueAllocator>::AssignTo( CVariantBase<T> *pDest ) const
{
	switch(m_type)
	{
		case FIELD_VECTOR:		pDest->CopyData( *m_pVector, true ); return true;
		case FIELD_VECTOR2D:	pDest->CopyData( *m_pVector2D, true ); return true;
		case FIELD_VECTOR4D:	pDest->CopyData( *m_pVector4D, true ); return true;
		case FIELD_POSITION_VECTOR:	pDest->CopyData( *m_pVectorWS, true ); return true;
		case FIELD_QUATERNION:	pDest->CopyData( *m_pQuaternion, true ); return true;
		case FIELD_QANGLE:		pDest->CopyData( *m_pQAngle, true ); return true;
		case FIELD_CSTRING:		pDest->CopyData( m_pszString, true ); return true;
		default:
		{
			pDest->m_type = m_type;
			pDest->m_pData = m_pData;
			return true;
		}
	}
}

template< class CValueAllocator >
template< typename T >
inline bool CVariantBase<CValueAllocator>::AssignTo( T *pDest ) const
{
	fieldtype_t destType = VariantDeduceType( T );

	if(destType == FIELD_TYPEUNKNOWN)
	{
		Warning( "Unable to convert variant to unknown type\n" );
	}

	if(destType == m_type)
	{
		*pDest = *this;
		return true;
	}

	if(m_type != FIELD_VECTOR2D && m_type != FIELD_VECTOR && m_type != FIELD_VECTOR4D && m_type != FIELD_POSITION_VECTOR && m_type != FIELD_QANGLE && m_type != FIELD_QUATERNION && m_type != FIELD_CSTRING &&
		destType != FIELD_VECTOR2D && destType != FIELD_VECTOR && destType != FIELD_VECTOR4D && destType != FIELD_POSITION_VECTOR && destType != FIELD_QANGLE && destType != FIELD_QUATERNION && destType != FIELD_CSTRING)
	{
		switch(m_type)
		{
			case FIELD_INT32:		*pDest = m_int32; return true;
			case FIELD_INT64:		*pDest = m_int64; return true;
			case FIELD_UINT32:		*pDest = m_uint32; return true;
			case FIELD_UINT64:		*pDest = m_uint64; return true;
			case FIELD_FLOAT32:		*pDest = m_float32; return true;
			case FIELD_FLOAT64:		*pDest = m_float64; return true;
			case FIELD_CHARACTER:	*pDest = m_char; return true;
			case FIELD_BOOLEAN:		*pDest = m_bool; return true;
		}
	}
	else
	{
		Warning( "No free conversion of %s variant to %s right now\n", VariantFieldTypeName( m_type ), VariantFieldTypeName<T>() );
		*pDest = {};
	}

	return false;
}

template< class CValueAllocator >
inline void CVariantBase<CValueAllocator>::Set( fieldtype_t ftype, void *pData )
{
	switch(ftype)
	{
		case FIELD_VOID:			Free(); m_type = FIELD_VOID; m_pData = nullptr; return;
		case FIELD_FLOAT32:			CopyData( *(float32 *)pData, false ); return;
		case FIELD_FLOAT64:			CopyData( *(float64 *)pData, false ); return;
		case FIELD_UINT8:			CopyData( *(uint8 *)pData, false ); return;
		case FIELD_INT16:			CopyData( *(int16 *)pData, false ); return;
		case FIELD_UINT16:			CopyData( *(uint16 *)pData, false ); return;
		case FIELD_INT32:			CopyData( *(int32 *)pData, false ); return;
		case FIELD_UINT32:			CopyData( *(uint32 *)pData, false ); return;
		case FIELD_INT64:			CopyData( *(int64 *)pData, false ); return;
		case FIELD_UINT64:			CopyData( *(uint64 *)pData, false ); return;
		case FIELD_BOOLEAN:			CopyData( *(bool *)pData, false ); return;
		case FIELD_CHARACTER:		CopyData( *(char *)pData, false ); return;
		case FIELD_STRING:			CopyData( *(string_t *)pData, false ); return;
		case FIELD_CSTRING:			CopyData( *(const char **)pData, false ); return;
		case FIELD_VECTOR:			CopyData( (Vector *)pData, false ); return;
		case FIELD_VECTOR2D:		CopyData( (Vector2D *)pData, false ); return;
		case FIELD_VECTOR4D:		CopyData( (Vector4D *)pData, false ); return;
		case FIELD_POSITION_VECTOR:	CopyData( (VectorWS *)pData, false ); return;
		case FIELD_QANGLE:			CopyData( (QAngle *)pData, false ); return;
		case FIELD_QUATERNION:		CopyData( (Quaternion *)pData, false ); return;
		case FIELD_COLOR32:			CopyData( *(color32 *)pData, false ); return;
		case FIELD_HSCRIPT:			CopyData( *(HSCRIPT *)pData, false ); return;
		case FIELD_EHANDLE:			CopyData( *(CEntityHandle *)pData, false ); return;
		case FIELD_RESOURCE:		CopyData( *(ResourceHandle_t *)pData, false ); return;
		case FIELD_UTLSTRINGTOKEN:	CopyData( *(CUtlStringToken *)pData, false ); return;
	}
}

template< class CValueAllocator >
inline bool CVariantBase<CValueAllocator>::Convert( fieldtype_t newType )
{
	if(newType == m_type)
	{
		return true;
	}

	bool successful = false;
	void *pData = nullptr;
	switch(newType)
	{
		case FIELD_VOID:			successful = true; Free(); m_type = FIELD_VOID; m_pData = nullptr; break;
		case FIELD_FLOAT32:			if((successful = AssignTo( (float32 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_FLOAT64:			if((successful = AssignTo( (float64 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_UINT8:			if((successful = AssignTo( (uint8 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_INT16:			if((successful = AssignTo( (int16 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_UINT16:			if((successful = AssignTo( (uint16 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_INT32:			if((successful = AssignTo( (int32 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_UINT32:			if((successful = AssignTo( (uint32 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_INT64:			if((successful = AssignTo( (int64 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_UINT64:			if((successful = AssignTo( (uint64 *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_BOOLEAN:			if((successful = AssignTo( (bool *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_CHARACTER:		if((successful = AssignTo( (char *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_STRING:			if((successful = AssignTo( (string_t *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_CSTRING:			successful = true; CopyData( ToString(), true ); break;
		case FIELD_HSCRIPT:			if((successful = AssignTo( (HSCRIPT *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_EHANDLE:			if((successful = AssignTo( (CEntityHandle *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_RESOURCE:		if((successful = AssignTo( (ResourceHandle_t *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_UTLSTRINGTOKEN:	if((successful = AssignTo( (CUtlStringToken *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_COLOR32:			if((successful = AssignTo( (Color *)&pData ))) { Set( newType, &pData ); } break;
		case FIELD_VECTOR:			{ Vector vec; if((successful = AssignTo( &vec ))) { CopyData( vec, true ); } break; }
		case FIELD_VECTOR2D:		{ Vector2D vec; if((successful = AssignTo( &vec ))) { CopyData( vec, true ); } break; }
		case FIELD_VECTOR4D:		{ Vector4D vec; if((successful = AssignTo( &vec ))) { CopyData( vec, true ); } break; }
		case FIELD_QANGLE:			{ QAngle ang; if((successful = AssignTo( &ang ))) { CopyData( ang, true ); } break; }
		case FIELD_QUATERNION:		{ Quaternion quat; if((successful = AssignTo( &quat ))) { CopyData( quat, true ); } break; }
	}

	if(successful)
	{
		m_type = newType;
	}

	return successful;
}

template< class CValueAllocator >
inline const char *CVariantBase<CValueAllocator>::ToString() const
{
	switch(m_type)
	{
		case FIELD_CSTRING:	return m_pszString;
		case FIELD_STRING:	return m_stringt.ToCStr();
		default:
		{
			static CBufferStringN<512> szBuf;
			AssignTo( szBuf );
			return szBuf.Get();
		}
	}
}

// overloading == as an external function
template< class CValueAllocator1, class CValueAllocator2 >
inline bool operator ==( const CVariantBase<CValueAllocator1> &v1, const CVariantBase<CValueAllocator2> &v2 )
{
	if(v1.m_type != v2.m_type)
		return false;

	switch(v1.m_type)
	{
		case FIELD_UINT8: { return v1.m_uint8 == v2.m_uint8; }
		case FIELD_INT16: { return v1.m_int16 == v2.m_int16; }
		case FIELD_UINT16: { return v1.m_uint16 == v2.m_uint16; }
		case FIELD_INT32: { return v1.m_int32 == v2.m_int32; }
		case FIELD_UINT32: { return v1.m_uint32 == v2.m_uint32; }
		case FIELD_INT64: { return v1.m_int64 == v2.m_int64; }
		case FIELD_UINT64: { return v1.m_uint64 == v2.m_uint64; }
		case FIELD_FLOAT32: { return v1.m_float32 == v2.m_float32; }
		case FIELD_FLOAT64: { return v1.m_float64 == v2.m_float64; }
		case FIELD_CHARACTER: { return v1.m_char == v2.m_char; }
		case FIELD_BOOLEAN: { return v1.m_bool == v2.m_bool; }
		case FIELD_HSCRIPT: { return v1.m_hScript == v2.m_hScript; }
		case FIELD_EHANDLE: { return v1.m_hEntity == v2.m_hEntity; }
		case FIELD_COLOR32: { return v1.m_color32 == v2.m_color32; }
		case FIELD_UTLSTRINGTOKEN: { return v1.m_utlStringToken == v2.m_utlStringToken; }
	}

	return false;
}

// overloading != as an external function
template< class CValueAllocator1, class CValueAllocator2 >
inline bool operator !=( const CVariantBase<CValueAllocator1> &v1, const CVariantBase<CValueAllocator2> &v2 )
{
	return !(v1 == v2);
}

#include "tier0/memdbgoff.h"

#endif // CVARIANT_H