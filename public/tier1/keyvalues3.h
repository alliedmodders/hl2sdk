#ifndef KEYVALUES3_H
#define KEYVALUES3_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/bufferstring.h"
#include "tier1/generichash.h"
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlhashtable.h"
#include "tier1/utlleanvector.h"
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"
#include "tier1/utlstringtoken.h"
#include "tier1/utlsymbollarge.h"
#include "mathlib/vector4d.h"
#include "Color.h"
#include "entityhandle.h"

#include "tier0/memdbgon.h"

class KeyValues3;
class CKeyValues3Array;
class CKeyValues3Table;
class CKeyValues3Cluster;
class CKeyValues3Context;
struct KV1ToKV3Translation_t;
struct KV3ToKV1Translation_t;

template < class T > class CKeyValues3ClusterT;
typedef CKeyValues3ClusterT< CKeyValues3Array > CKeyValues3ArrayCluster;
typedef CKeyValues3ClusterT< CKeyValues3Table > CKeyValues3TableCluster;

/* 
	KeyValues3 is a data storage format. See https://developer.valvesoftware.com/wiki/KeyValues3
	Supports various specific data types targeted at the Source2.
	Each specific type corresponds to one of the basic types.

	There are 2 ways to create KeyValues3:

	1. Via CKeyValues3Context:
	- KV's, arrays and tables are stored in fixed memory blocks (clusters) and therefore memory is allocated only when clusters are created.
	- Supports metadata and some other things.

	2. Directly through the constructor.
*/

struct KV3ID_t
{
	const char* m_name;
	uint64		m_data1;
	uint64		m_data2;
};

// encodings
const KV3ID_t g_KV3Encoding_Text 		= { "text", 0x41C58A33E21C7F3Cull, 0xDAA323A6DA77799ull };
const KV3ID_t g_KV3Encoding_Binary 		= { "binary", 0x40C1F7D81B860500ull, 0x14E76782A47582ADull };
const KV3ID_t g_KV3Encoding_BinaryLZ4 	= { "binary_lz4", 0x4F5C63A16847348Aull, 0x19B1D96F805397A1ull };
const KV3ID_t g_KV3Encoding_BinaryZSTD 	= { "binary_zstd", 0x4305FEF06F620A00ull, 0x29DBB14623045FA3ull };
const KV3ID_t g_KV3Encoding_BinaryBC 	= { "binary_bc", 0x4F6C95BC95791A46ull, 0xD2DFB7A1BC050BA7ull };
const KV3ID_t g_KV3Encoding_BinaryAuto 	= { "binary_auto", 0x45836B856EB109E6ull, 0x8C06046E3A7012A3ull };

// formats
const KV3ID_t g_KV3Format_Generic = { "generic", 0x469806E97412167Cull, 0xE73790B53EE6F2AFull };

enum KV1TextEscapeBehavior_t
{
	KV1TEXT_ESC_BEHAVIOR_UNK1 = 0,
	KV1TEXT_ESC_BEHAVIOR_UNK2 = 1,
};

enum KV3SaveTextFlags_t
{
	KV3_SAVE_TEXT_NONE = 0,
	KV3_SAVE_TEXT_TAGGED = (1 << 0), // adds subtype name before value
};

PLATFORM_OVERLOAD void DebugPrintKV3( const KeyValues3* kv );

// When using some LoadKV3/SaveKV3 functions, KV3ID_t structures must be filled in, which specify the format or encoding of the data.

PLATFORM_OVERLOAD bool LoadKV3( CKeyValues3Context* context, CUtlString* error, CUtlBuffer* input, const KV3ID_t& format, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3( KeyValues3* kv, CUtlString* error, CUtlBuffer* input, const KV3ID_t& format, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3( KeyValues3* kv, CUtlString* error, const char* input, const KV3ID_t& format, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3FromFile( CKeyValues3Context* context, CUtlString* error, const char* filename, const char* path, const KV3ID_t& format );
PLATFORM_OVERLOAD bool LoadKV3FromFile( KeyValues3* kv, CUtlString* error, const char* filename, const char* path, const KV3ID_t& format );
PLATFORM_OVERLOAD bool LoadKV3FromJSON( KeyValues3* kv, CUtlString* error, const char* input, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3FromJSONFile( KeyValues3* kv, CUtlString* error, const char* path, const char* filename );
PLATFORM_OVERLOAD bool LoadKV3FromKV1File( KeyValues3* kv, CUtlString* error, const char* path, const char* filename, KV1TextEscapeBehavior_t esc_behavior );
PLATFORM_OVERLOAD bool LoadKV3FromKV1Text( KeyValues3* kv, CUtlString* error, const char* input, KV1TextEscapeBehavior_t esc_behavior, const char* kv_name, bool unk );
PLATFORM_OVERLOAD bool LoadKV3FromKV1Text_Translated( KeyValues3* kv, CUtlString* error, const char* input, KV1TextEscapeBehavior_t esc_behavior, const KV1ToKV3Translation_t* translation, int unk1, const char* kv_name, bool unk2 );
PLATFORM_OVERLOAD bool LoadKV3FromKV3OrKV1( KeyValues3* kv, CUtlString* error, CUtlBuffer* input, const KV3ID_t& format, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3FromOldSchemaText( KeyValues3* kv, CUtlString* error, CUtlBuffer* input, const KV3ID_t& format, const char* kv_name );
PLATFORM_OVERLOAD bool LoadKV3Text_NoHeader( KeyValues3* kv, CUtlString* error, const char* input, const KV3ID_t& format, const char* kv_name );

PLATFORM_OVERLOAD bool SaveKV3( const KV3ID_t& encoding, const KV3ID_t& format, const KeyValues3* kv, CUtlString* error, CUtlBuffer* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3AsJSON( const KeyValues3* kv, CUtlString* error, CUtlBuffer* output );
PLATFORM_OVERLOAD bool SaveKV3AsJSON( const KeyValues3* kv, CUtlString* error, CUtlString* output );
PLATFORM_OVERLOAD bool SaveKV3AsKV1Text( const KeyValues3* kv, CUtlString* error, CUtlBuffer* output, KV1TextEscapeBehavior_t esc_behavior );
PLATFORM_OVERLOAD bool SaveKV3AsKV1Text_Translated( const KeyValues3* kv, CUtlString* error, CUtlBuffer* output, KV1TextEscapeBehavior_t esc_behavior, const KV3ToKV1Translation_t* translation, int unk );
PLATFORM_OVERLOAD bool SaveKV3Text_NoHeader( const KeyValues3* kv, CUtlString* error, CBufferString* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3Text_NoHeader( const KeyValues3* kv, CUtlString* error, CUtlBuffer* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3Text_NoHeader( const KeyValues3* kv, CUtlString* error, CUtlString* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3Text_ToString( const KV3ID_t& format, const KeyValues3* kv, CUtlString* error, CBufferString* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3Text_ToString( const KV3ID_t& format, const KeyValues3* kv, CUtlString* error, CUtlString* output, uint flags = KV3_SAVE_TEXT_NONE );
PLATFORM_OVERLOAD bool SaveKV3ToFile( const KV3ID_t& encoding, const KV3ID_t& format, const KeyValues3* kv, CUtlString* error, const char* filename, const char* path, uint flags = KV3_SAVE_TEXT_NONE );

typedef int32 KV3MemberId_t;
#define KV3_INVALID_MEMBER ((KV3MemberId_t)-1)

enum
{
	KV3_CONTEXT_SIZE = 1584
};

enum
{
	KV3_CLUSTER_MAX_ELEMENTS = 63
};

enum KV3Type_t : uint8
{
	KV3_TYPE_INVALID = 0,
	KV3_TYPE_NULL,
	KV3_TYPE_BOOL,
	KV3_TYPE_INT,
	KV3_TYPE_UINT,
	KV3_TYPE_DOUBLE,
	KV3_TYPE_STRING,
	KV3_TYPE_BINARY_BLOB,
	KV3_TYPE_ARRAY,
	KV3_TYPE_TABLE,

	KV3_TYPE_COUNT,
};

enum KV3TypeOpt_t : uint8
{
	KV3_TYPEOPT_NONE = 0,
	
	KV3_TYPEOPT_STRING_SHORT,
	KV3_TYPEOPT_STRING_EXTERN,
	
	KV3_TYPEOPT_BINARY_BLOB_EXTERN,
	
	KV3_TYPEOPT_ARRAY_FLOAT32,
	KV3_TYPEOPT_ARRAY_FLOAT64,
	KV3_TYPEOPT_ARRAY_INT16,
	KV3_TYPEOPT_ARRAY_INT32,
	KV3_TYPEOPT_ARRAY_UINT8_SHORT,
	KV3_TYPEOPT_ARRAY_INT16_SHORT,
};

enum KV3TypeEx_t : uint8
{
	KV3_TYPEEX_INVALID = 0,
	KV3_TYPEEX_NULL,
	KV3_TYPEEX_BOOL,
	KV3_TYPEEX_INT,
	KV3_TYPEEX_UINT,
	KV3_TYPEEX_DOUBLE,

	KV3_TYPEEX_STRING			= KV3_TYPE_STRING,
	KV3_TYPEEX_STRING_SHORT		= (KV3_TYPEEX_STRING|(KV3_TYPEOPT_STRING_SHORT << 4)),
	KV3_TYPEEX_STRING_EXTERN	= (KV3_TYPEEX_STRING|(KV3_TYPEOPT_STRING_EXTERN << 4)),

	KV3_TYPEEX_BINARY_BLOB			= KV3_TYPE_BINARY_BLOB,
	KV3_TYPEEX_BINARY_BLOB_EXTERN	= (KV3_TYPEEX_BINARY_BLOB|(KV3_TYPEOPT_BINARY_BLOB_EXTERN << 4)),

	KV3_TYPEEX_ARRAY				= KV3_TYPE_ARRAY,
	KV3_TYPEEX_ARRAY_FLOAT32		= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_FLOAT32 << 4)),
	KV3_TYPEEX_ARRAY_FLOAT64		= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_FLOAT64 << 4)),
	KV3_TYPEEX_ARRAY_INT16			= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_INT16 << 4)),
	KV3_TYPEEX_ARRAY_INT32			= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_INT32 << 4)),
	KV3_TYPEEX_ARRAY_UINT8_SHORT	= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_UINT8_SHORT << 4)),
	KV3_TYPEEX_ARRAY_INT16_SHORT	= (KV3_TYPEEX_ARRAY|(KV3_TYPEOPT_ARRAY_INT16_SHORT << 4)),

	KV3_TYPEEX_TABLE = KV3_TYPE_TABLE,
};

enum KV3SubType_t : uint8
{
	KV3_SUBTYPE_INVALID = 0,

	// string types
	KV3_SUBTYPE_RESOURCE,
	KV3_SUBTYPE_RESOURCE_NAME,
	KV3_SUBTYPE_PANORAMA,
	KV3_SUBTYPE_SOUNDEVENT,

	KV3_SUBTYPE_SUBCLASS, // table type
	KV3_SUBTYPE_ENTITY_NAME, // string type
	KV3_SUBTYPE_UNSPECIFIED,
	KV3_SUBTYPE_NULL,
	KV3_SUBTYPE_BINARY_BLOB,
	KV3_SUBTYPE_ARRAY,
	KV3_SUBTYPE_TABLE,
	KV3_SUBTYPE_BOOL8,
	KV3_SUBTYPE_CHAR8,
	KV3_SUBTYPE_UCHAR32,
	KV3_SUBTYPE_INT8,
	KV3_SUBTYPE_UINT8,
	KV3_SUBTYPE_INT16,
	KV3_SUBTYPE_UINT16,
	KV3_SUBTYPE_INT32,
	KV3_SUBTYPE_UINT32,
	KV3_SUBTYPE_INT64,
	KV3_SUBTYPE_UINT64,
	KV3_SUBTYPE_FLOAT32,
	KV3_SUBTYPE_FLOAT64,
	KV3_SUBTYPE_STRING,
	KV3_SUBTYPE_POINTER,
	KV3_SUBTYPE_COLOR32,

	// vector types
	KV3_SUBTYPE_VECTOR,
	KV3_SUBTYPE_VECTOR2D,
	KV3_SUBTYPE_VECTOR4D,
	KV3_SUBTYPE_ROTATION_VECTOR,
	KV3_SUBTYPE_QUATERNION,
	KV3_SUBTYPE_QANGLE,
	KV3_SUBTYPE_MATRIX3X4,
	KV3_SUBTYPE_TRANSFORM,

	KV3_SUBTYPE_STRING_TOKEN,
	KV3_SUBTYPE_EHANDLE,

	KV3_SUBTYPE_COUNT,
};

enum KV3ArrayAllocType_t
{
	KV3_ARRAY_ALLOC_EXTERN = 0,
	KV3_ARRAY_ALLOC_NORMAL = 1,
	KV3_ARRAY_ALLOC_EXTERN_FREE = 2,
};

enum KV3ToStringFlags_t
{
	KV3_TO_STRING_NONE = 0,
	KV3_TO_STRING_DONT_CLEAR_BUFF = (1 << 0),
	KV3_TO_STRING_DONT_APPEND_STRINGS = (1 << 1),
	KV3_TO_STRING_APPEND_ONLY_NUMERICS = (1 << 2),
	KV3_TO_STRING_RETURN_NON_NUMERICS = (1 << 3),
};

enum KV3MetaDataFlags_t
{
	KV3_METADATA_MULTILINE_STRING = (1 << 0),
	KV3_METADATA_SINGLE_QUOTED_STRING = (1 << 1),
};

namespace KV3Helpers
{

// https://www.chessprogramming.org/BitScan
inline int BitScanFwd( uint64 bb ) 
{
	static const int index64[64] = {
		0,  47,  1, 56, 48, 27,  2, 60,
		57, 49, 41, 37, 28, 16,  3, 61,
		54, 58, 35, 52, 50, 42, 21, 44,
		38, 32, 29, 23, 17, 11,  4, 62,
		46, 55, 26, 59, 40, 36, 15, 53,
		34, 51, 20, 43, 31, 22, 10, 45,
		25, 39, 14, 33, 19, 30,  9, 24,
		13, 18,  8, 12,  7,  6,  5, 63
	};

	const uint64 debruijn64 = 0x03f79d71b4cb0a89ull;
	Assert( bb != 0 );
	return index64[ ( ( bb ^ ( bb - 1 ) ) * debruijn64 ) >> 58 ];
}

// https://www.chessprogramming.org/Population_Count
inline int PopCount( uint64 x )
{
	x =  x - ( ( x >> 1 ) & 0x5555555555555555ull );
    x = ( x & 0x3333333333333333ull ) + ( ( x >> 2 ) & 0x3333333333333333ull );
    x = ( x + ( x >> 4 ) ) & 0x0f0f0f0f0f0f0f0full;
    x = ( x * 0x0101010101010101ull ) >> 56;
    return ( int )x;
}

}

struct KV3MetaData_t
{
	KV3MetaData_t() : m_nLine( 0 ), m_nColumn( 0 ), m_nFlags( 0 ) {}

	void Clear()
	{
		m_nLine = 0;
		m_nColumn = 0;
		m_nFlags = 0;
		m_sName = CUtlSymbolLarge();
		m_Comments.RemoveAll();
	}

	void Purge()
	{
		m_nLine = 0;
		m_nColumn = 0;
		m_nFlags = 0;
		m_sName = CUtlSymbolLarge();
		m_Comments.Purge();
	}

	typedef CUtlMap<int, CBufferStringGrowable<8>, int, CDefLess<int>> CommentsMap_t;
	
	int 			m_nLine;
	int 			m_nColumn;
	uint			m_nFlags;
	CUtlSymbolLarge m_sName;
	CommentsMap_t 	m_Comments;
};

struct KV3BinaryBlob_t
{
	size_t m_nSize;
	union
	{
		const byte*	m_pubData;
		byte		m_ubData[1];
	};
	bool m_bFreeMemory;
};

class CKV3MemberName
{
public:
	inline CKV3MemberName(const char* pszString): m_nHashCode(), m_pszString("")
	{	
		if (!pszString || !pszString[0])
			return;

		m_nHashCode = MakeStringToken( pszString );
		m_pszString = pszString;
	}

	inline CKV3MemberName(): m_nHashCode(), m_pszString("") {}
	inline CKV3MemberName(unsigned int nHashCode, const char* pszString = ""): m_nHashCode(nHashCode), m_pszString(pszString) {}

	inline unsigned int GetHashCode() const { return m_nHashCode.GetHashCode(); }
	inline const char* GetString() const { return m_pszString; }

private:
	CUtlStringToken m_nHashCode;
	const char* m_pszString;
};

class KeyValues3
{
public:
	KeyValues3( KV3TypeEx_t type = KV3_TYPEEX_NULL, KV3SubType_t subtype = KV3_SUBTYPE_UNSPECIFIED );
	~KeyValues3();

	CKeyValues3Context* GetContext() const;
	KV3MetaData_t* GetMetaData( CKeyValues3Context** ppCtx = NULL ) const;

	KV3Type_t GetType() const		{ return ( KV3Type_t )( m_TypeEx & 0xF ); }
	KV3TypeEx_t GetTypeEx() const	{ return ( KV3TypeEx_t )m_TypeEx; }
	KV3SubType_t GetSubType() const	{ return ( KV3SubType_t )m_SubType; }

	const char* GetTypeAsString() const;
	const char* GetSubTypeAsString() const;

	const char* ToString( CBufferString& buff, uint flags = KV3_TO_STRING_NONE ) const;

	void SetToNull() { PrepareForType( KV3_TYPEEX_NULL, KV3_SUBTYPE_NULL ); }

	bool GetBool( bool defaultValue = false ) const			{ return GetValue<bool>( defaultValue ); }
	char8 GetChar( char8 defaultValue = 0 ) const			{ return GetValue<char8>( defaultValue ); }
	uchar32 GetUChar32( uchar32 defaultValue = 0 ) const	{ return GetValue<uint32>( defaultValue ); }
	int8 GetInt8( int8 defaultValue = 0 ) const				{ return GetValue<int8>( defaultValue ); }
	uint8 GetUInt8( uint8 defaultValue = 0 ) const			{ return GetValue<uint8>( defaultValue ); }
	int16 GetShort( int16 defaultValue = 0 ) const			{ return GetValue<int16>( defaultValue ); }
	uint16 GetUShort( uint16 defaultValue = 0 ) const		{ return GetValue<uint16>( defaultValue ); }
	int32 GetInt( int32 defaultValue = 0 ) const			{ return GetValue<int32>( defaultValue ); }
	uint32 GetUInt( uint32 defaultValue = 0 ) const			{ return GetValue<uint32>( defaultValue ); }
	int64 GetInt64( int64 defaultValue = 0 ) const			{ return GetValue<int64>( defaultValue ); }
	uint64 GetUInt64( uint64 defaultValue = 0 ) const		{ return GetValue<uint64>( defaultValue ); }
	float32 GetFloat( float32 defaultValue = 0.0f ) const	{ return GetValue<float32>( defaultValue ); }
	float64 GetDouble( float64 defaultValue = 0.0 ) const	{ return GetValue<float64>( defaultValue ); }

	void SetBool( bool value )		{ SetValue<bool>( value, KV3_TYPEEX_BOOL, KV3_SUBTYPE_BOOL8 ); }
	void SetChar( char8 value )		{ SetValue<char8>( value, KV3_TYPEEX_INT, KV3_SUBTYPE_CHAR8 ); }
	void SetUChar32( uchar32 value ){ SetValue<uint32>( value, KV3_TYPEEX_UINT, KV3_SUBTYPE_UCHAR32 ); }
	void SetInt8( int8 value )		{ SetValue<int8>( value, KV3_TYPEEX_INT, KV3_SUBTYPE_INT8 ); }
	void SetUInt8( uint8 value )	{ SetValue<uint8>( value, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT8 ); }
	void SetShort( int16 value )	{ SetValue<int16>( value, KV3_TYPEEX_INT, KV3_SUBTYPE_INT16 ); }
	void SetUShort( uint16 value )	{ SetValue<uint16>( value, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT16 ); }
	void SetInt( int32 value )		{ SetValue<int32>( value, KV3_TYPEEX_INT, KV3_SUBTYPE_INT32 ); }
	void SetUInt( uint32 value )	{ SetValue<uint32>( value, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT32 ); }
	void SetInt64( int64 value )	{ SetValue<int64>( value, KV3_TYPEEX_INT, KV3_SUBTYPE_INT64 ); }
	void SetUInt64( uint64 value )	{ SetValue<uint64>( value, KV3_TYPEEX_UINT, KV3_SUBTYPE_UINT64 ); }
	void SetFloat( float32 value )	{ SetValue<float32>( value, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32 ); }
	void SetDouble( float64 value )	{ SetValue<float64>( value, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT64 ); }

	void* GetPointer( void *defaultValue = ( void* )0 ) const { return ( GetSubType() == KV3_SUBTYPE_POINTER ) ? ( void* )m_UInt : defaultValue; }
	void SetPointer( void* ptr ) { SetValue<uint64>( ( uint64 )ptr, KV3_TYPEEX_UINT, KV3_SUBTYPE_POINTER ); }
	
	CUtlStringToken GetStringToken( CUtlStringToken defaultValue = CUtlStringToken() ) const { return ( GetSubType() == KV3_SUBTYPE_STRING_TOKEN ) ? CUtlStringToken( ( uint32 )m_UInt ) : defaultValue; }
	void SetStringToken( CUtlStringToken token ) { SetValue<uint32>( token.GetHashCode(), KV3_TYPEEX_UINT, KV3_SUBTYPE_STRING_TOKEN ); }

	CEntityHandle GetEHandle( CEntityHandle defaultValue = CEntityHandle() ) const { return ( GetSubType() == KV3_SUBTYPE_EHANDLE ) ? CEntityHandle( ( uint32 )m_UInt ) : defaultValue; }
	void SetEHandle( CEntityHandle ehandle ) { SetValue<uint32>( ehandle.ToInt(), KV3_TYPEEX_UINT, KV3_SUBTYPE_EHANDLE ); }

	const char* GetString( const char *defaultValue = "" ) const;
	void SetString( const char* pString, KV3SubType_t subtype = KV3_SUBTYPE_STRING );
	void SetStringExternal( const char* pString, KV3SubType_t subtype = KV3_SUBTYPE_STRING );
	
	const byte* GetBinaryBlob() const;
	int GetBinaryBlobSize() const;
	void SetToBinaryBlob( const byte* blob, int size );
	void SetToBinaryBlobExternal( const byte* blob, int size, bool free_mem );

	Color GetColor( const Color &defaultValue = Color( 0, 0, 0, 255 ) ) const;
	void SetColor( const Color &color );

	Vector GetVector( const Vector &defaultValue = Vector( 0.0f, 0.0f, 0.0f ) ) const						{ return GetVecBasedObj<Vector>( 3, defaultValue ); }
	Vector2D GetVector2D( const Vector2D &defaultValue = Vector2D( 0.0f, 0.0f ) ) const						{ return GetVecBasedObj<Vector2D>( 2, defaultValue ); }
	Vector4D GetVector4D( const Vector4D &defaultValue = Vector4D( 0.0f, 0.0f, 0.0f, 0.0f ) ) const			{ return GetVecBasedObj<Vector4D>( 4, defaultValue ); }
	Quaternion GetQuaternion( const Quaternion &defaultValue = Quaternion( 0.0f, 0.0f, 0.0f, 0.0f ) ) const	{ return GetVecBasedObj<Quaternion>( 4, defaultValue ); }
	QAngle GetQAngle( const QAngle &defaultValue = QAngle( 0.0f, 0.0f, 0.0f ) ) const						{ return GetVecBasedObj<QAngle>( 3, defaultValue ); }
	matrix3x4_t GetMatrix3x4( const matrix3x4_t &defaultValue = matrix3x4_t( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 0.0f ) ) ) const { return GetVecBasedObj<matrix3x4_t>( 3*4, defaultValue ); }

	void SetVector( const Vector &vec )				{ SetVecBasedObj<Vector>( vec, 3, KV3_SUBTYPE_VECTOR ); }
	void SetVector2D( const Vector2D &vec2d )		{ SetVecBasedObj<Vector2D>( vec2d, 2, KV3_SUBTYPE_VECTOR2D ); }
	void SetVector4D( const Vector4D &vec4d )		{ SetVecBasedObj<Vector4D>( vec4d, 4, KV3_SUBTYPE_VECTOR4D ); }
	void SetQuaternion( const Quaternion &quat )	{ SetVecBasedObj<Quaternion>( quat, 4, KV3_SUBTYPE_QUATERNION ); }
	void SetQAngle( const QAngle &ang )				{ SetVecBasedObj<QAngle>( ang, 3, KV3_SUBTYPE_QANGLE ); }
	void SetMatrix3x4( const matrix3x4_t &matrix )	{ SetVecBasedObj<matrix3x4_t>( matrix, 3*4, KV3_SUBTYPE_MATRIX3X4 ); }

	int GetArrayElementCount() const;
	KeyValues3** GetArrayBase();
	KeyValues3* GetArrayElement( int elem );
	KeyValues3* InsertArrayElementBefore( int elem );
	KeyValues3* InsertArrayElementAfter( int elem ) { return InsertArrayElementBefore( elem + 1 ); }
	KeyValues3* AddArrayElementToTail();
	void SetArrayElementCount( int count, KV3TypeEx_t type = KV3_TYPEEX_NULL, KV3SubType_t subtype = KV3_SUBTYPE_UNSPECIFIED );
	void SetToEmptyArray() { SetArrayElementCount( 0 ); }
	void RemoveArrayElements( int elem, int num );
	void RemoveArrayElement( int elem ) { RemoveArrayElements( elem, 1 ); }

	int GetMemberCount() const;
	KeyValues3* GetMember( KV3MemberId_t id );
	const KeyValues3* GetMember( KV3MemberId_t id ) const { return const_cast<KeyValues3*>(this)->GetMember( id ); }
	const char* GetMemberName( KV3MemberId_t id ) const;
	CKV3MemberName GetMemberNameEx( KV3MemberId_t id ) const;
	unsigned int GetMemberHash( KV3MemberId_t id ) const;
	KeyValues3* FindMember( const CKV3MemberName &name, KeyValues3* defaultValue = NULL );
	KeyValues3* FindOrCreateMember( const CKV3MemberName &name, bool *pCreated = NULL );
	bool TableHasBadNames() const;
	void SetTableHasBadNames( bool bHasBadNames );
	void SetToEmptyTable();
	bool RemoveMember( KV3MemberId_t id );
	bool RemoveMember( const KeyValues3* kv );
	bool RemoveMember( const CKV3MemberName &name );

	KeyValues3& operator=( const KeyValues3& src );
	
private:
	KeyValues3( int cluster_elem, KV3TypeEx_t type, KV3SubType_t subtype );	
	KeyValues3( const KeyValues3& other );

	void Alloc();
	void Free( bool bClearingContext = false );
	void ResolveUnspecified();
	void PrepareForType( KV3TypeEx_t type, KV3SubType_t subtype );
	void OnClearContext();
	void CopyFrom( const KeyValues3* pSrc );

	int GetClusterElement() const { return m_nClusterElement; }
	CKeyValues3Cluster* GetCluster() const;

	template < typename T > T FromString( T defaultValue ) const;
	template < typename T > void SetDirect( T value );

	template < typename T > T GetValue( T defaultValue ) const;
	template < typename T > void SetValue( T value, KV3TypeEx_t type, KV3SubType_t subtype );

	template < typename T > T GetVecBasedObj( int size, const T &defaultValue ) const;
	template < typename T > void SetVecBasedObj( const T &obj, int size, KV3SubType_t subtype );

	template < typename T >
	void NormalizeArray( KV3TypeEx_t type, KV3SubType_t subtype, int size, const T* data, bool bFree );
	void NormalizeArray();

	template < typename T >
	void AllocArray( int size, const T* data, KV3ArrayAllocType_t alloc_type, KV3TypeEx_t type_short, KV3TypeEx_t type_ptr, KV3SubType_t subtype, KV3TypeEx_t type_elem, KV3SubType_t subtype_elem );

	bool ReadArrayInt32( int size, int32* data ) const;
	bool ReadArrayFloat32( int size, float32* data ) const;

private:
	uint64 m_bExternalStorage : 1;
	uint64 m_bFreeArrayMemory : 1;
	uint64 m_TypeEx : 8;
	uint64 m_SubType : 8;
	uint64 m_nFlags : 8;
	uint64 m_nClusterElement : 6;
	uint64 m_nNumArrayElements : 5;
	uint64 m_nReserved : 27;
	
	union
	{
		bool 	m_Bool;
		int64 	m_Int;
		uint64 	m_UInt;
		float64	m_Double;
		
		const char* m_pString;
		char m_szStringShort[8];
		
		KV3BinaryBlob_t* m_pBinaryBlob;

		CKeyValues3Array* 	m_pArray;
		float32*			m_f32Array;
		float64*			m_f64Array;
		int16*				m_i16Array;
		int32*				m_i32Array;
		uint8				m_u8ArrayShort[8];
		int16				m_i16ArrayShort[4];
		
		CKeyValues3Table* m_pTable;

		uint64 m_nData;
		void* m_pData;
		char m_Data[1];
	};

	friend class CKeyValues3Cluster;
	friend class CKeyValues3Context;
};
COMPILE_TIME_ASSERT(sizeof(KeyValues3) == 16);

class CKeyValues3Array
{
public:
	CKeyValues3Array( int cluster_elem = -1 );

	int GetClusterElement() const { return m_nClusterElement; }
	CKeyValues3ArrayCluster* GetCluster() const;
	CKeyValues3Context* GetContext() const;
		
	KeyValues3** Base() { return m_Elements.Base(); }
	KeyValues3* const * Base() const { return m_Elements.Base(); }

	KeyValues3* Element( int i ) { return m_Elements[ i ]; }
	const KeyValues3* Element( int i ) const { return m_Elements[ i ]; }

	int Count() const { return m_Elements.Count(); }
	void SetCount( int count, KV3TypeEx_t type = KV3_TYPEEX_NULL, KV3SubType_t subtype = KV3_SUBTYPE_UNSPECIFIED );
	KeyValues3** InsertBeforeGetPtr( int elem, int num );
	void CopyFrom( const CKeyValues3Array* pSrc );
	void RemoveMultiple( int elem, int num );
	void Purge( bool bClearingContext );

private:
	typedef CUtlLeanVectorFixedGrowable<KeyValues3*, 4, int> ElementsVec_t;
	
	int				m_nClusterElement;
	ElementsVec_t 	m_Elements;  
};

class CKeyValues3Table
{
public:
	CKeyValues3Table( int cluster_elem = -1 );

	int GetClusterElement() const { return m_nClusterElement; }
	CKeyValues3TableCluster* GetCluster() const;
	CKeyValues3Context* GetContext() const;

	int GetMemberCount() const { return m_Hashes.Count(); }
	KeyValues3* GetMember( KV3MemberId_t id ) { return m_Members[ id ]; }
	const KeyValues3* GetMember( KV3MemberId_t id ) const { return m_Members[ id ]; }
	const char* GetMemberName( KV3MemberId_t id ) const { return m_Names[ id ]; }
	unsigned int GetMemberHash( KV3MemberId_t id ) const { return m_Hashes[ id ]; }
	void EnableFastSearch();
	KV3MemberId_t FindMember( const KeyValues3* kv ) const;
	KV3MemberId_t FindMember( const CKV3MemberName &name );
	KV3MemberId_t CreateMember( const CKV3MemberName &name );
	bool HasBadNames() const { return m_bHasBadNames; }
	void SetHasBadNames( bool bHasBadNames ) { m_bHasBadNames = bHasBadNames; }
	void CopyFrom( const CKeyValues3Table* pSrc );
	void RemoveMember( KV3MemberId_t id );
	void RemoveAll( int nAllocSize = 0 );
	void Purge( bool bClearingContext );

private:
	typedef CUtlLeanVectorFixedGrowable<unsigned int, 8, int>	HashesVec_t;
	typedef CUtlLeanVectorFixedGrowable<KeyValues3*, 8, int>	MembersVec_t;
	typedef CUtlLeanVectorFixedGrowable<const char*, 8, int>	NamesVec_t;
	typedef CUtlLeanVectorFixedGrowable<bool, 8, int>			IsExternalNameVec_t;
	
	int m_nClusterElement;

	struct kv3tablefastsearch_t {
		kv3tablefastsearch_t() : m_ignore( false ), m_ignores_counter( 0 ) {}

		void Clear()
		{
			m_ignore = false;
			m_ignores_counter = 0;
			m_member_ids.RemoveAll();
		}

		struct EmptyHashFunctor { unsigned int operator()( uint32 n ) const { return n; } };
		typedef CUtlHashtable<unsigned int, KV3MemberId_t, EmptyHashFunctor> Hashtable_t;

		bool		m_ignore;
		int8		m_ignores_counter;
		Hashtable_t	m_member_ids;
	} *m_pFastSearch;

	HashesVec_t 				m_Hashes;
	MembersVec_t 				m_Members;
	NamesVec_t 					m_Names;
	IsExternalNameVec_t			m_IsExternalName; // didn't find this used when deleting a table
	bool						m_bHasBadNames;
};

class CKeyValues3Cluster
{
public:
	CKeyValues3Cluster( CKeyValues3Context* context );
	~CKeyValues3Cluster();

	KeyValues3* Alloc( KV3TypeEx_t type = KV3_TYPEEX_NULL, KV3SubType_t subtype = KV3_SUBTYPE_UNSPECIFIED );
	void Free( int element );
	void Clear();
	void PurgeElements();
	void Purge();

	void EnableMetaData( bool bEnable );
	void FreeMetaData();
	KV3MetaData_t* GetMetaData( int element ) const;

	CKeyValues3Context* GetContext() const { return m_pContext; }
	int NumAllocated() const { return KV3Helpers::PopCount( m_nAllocatedElements ); }
	bool IsFree() const { return ( m_nAllocatedElements != 0x7fffffffffffffffull ); }
	CKeyValues3Cluster* GetNextFree() const { return m_pNextFree; }
	void SetNextFree( CKeyValues3Cluster* free ) { m_pNextFree = free; }

	KeyValues3* Head() { return &m_KeyValues[ 0 ]; }
	const KeyValues3* Head() const { return &m_KeyValues[ 0 ]; }

private:
	CKeyValues3Context*	m_pContext;
	uint64				m_nAllocatedElements;
	KeyValues3			m_KeyValues[KV3_CLUSTER_MAX_ELEMENTS];

	struct kv3metadata_t {
		KV3MetaData_t m_elements[KV3_CLUSTER_MAX_ELEMENTS];
	} *m_pMetaData;

	CKeyValues3Cluster*	m_pNextFree;

	friend CKeyValues3Cluster* KeyValues3::GetCluster() const;
};

template < class T >
class CKeyValues3ClusterT
{
public:
	CKeyValues3ClusterT( CKeyValues3Context* context );

	T* Alloc();
	void Free( int element );
	void Clear() { Purge(); }
	void Purge();

	CKeyValues3Context* GetContext() const { return m_pContext; }
	int NumAllocated() const { return KV3Helpers::PopCount( m_nAllocatedElements ); }
	bool IsFree() const { return ( m_nAllocatedElements != 0x7fffffffffffffffull ); }
	CKeyValues3ClusterT* GetNextFree() const { return m_pNextFree; }
	void SetNextFree( CKeyValues3ClusterT* free ) { m_pNextFree = free; }

private:
	CKeyValues3Context*		m_pContext;
	uint64					m_nAllocatedElements;
	T						m_Elements[KV3_CLUSTER_MAX_ELEMENTS];
	CKeyValues3ClusterT*	m_pNextFree;

	friend CKeyValues3ArrayCluster* CKeyValues3Array::GetCluster() const;
	friend CKeyValues3TableCluster* CKeyValues3Table::GetCluster() const;
};

class CKeyValues3ContextBase
{
public:
	CKeyValues3ContextBase( CKeyValues3Context* context );
	~CKeyValues3ContextBase();

	void Clear();
	void Purge();

protected:
	typedef CUtlLeanVectorFixedGrowable<CKeyValues3Cluster*, 8, int>		KV3ClustersVec_t;
	typedef CUtlLeanVectorFixedGrowable<CKeyValues3ArrayCluster*, 4, int>	ArrayClustersVec_t;
	typedef CUtlLeanVectorFixedGrowable<CKeyValues3TableCluster*, 4, int>	TableClustersVec_t;

	CKeyValues3Context*			m_pContext;
	CUtlBuffer					m_BinaryData;
	
	CKeyValues3Cluster			m_KV3BaseCluster;
	KV3ClustersVec_t			m_KV3Clusters;
	CKeyValues3Cluster*			m_pKV3FreeCluster;
	
	ArrayClustersVec_t			m_ArrayClusters;
	CKeyValues3ArrayCluster*	m_pArrayFreeCluster;
	
	TableClustersVec_t			m_TableClusters;
	CKeyValues3TableCluster*	m_pTableFreeCluster;

	CUtlSymbolTableLarge		m_Symbols;

	bool						m_bMetaDataEnabled : 1;
	bool						m_bFormatConverted : 1;
	bool						m_bRootAvailabe : 1;

	IParsingErrorListener*		m_pParsingErrorListener;
};

class CKeyValues3Context : public CKeyValues3ContextBase
{
	typedef CKeyValues3ContextBase BaseClass;

public:
	CKeyValues3Context( bool bNoRoot = false );
	~CKeyValues3Context();

	KeyValues3* AllocKV( KV3TypeEx_t type = KV3_TYPEEX_NULL, KV3SubType_t subtype = KV3_SUBTYPE_UNSPECIFIED );
	// WARNING: kv must belong to this context!!!
	void FreeKV( KeyValues3* kv );
	
	// gets the pre-allocated kv if we indicated its existence when creating the context
	KeyValues3* Root();
	const KeyValues3* Root() const { return const_cast<CKeyValues3Context*>(this)->Root(); }

	bool IsMetaDataEnabled() const { return m_bMetaDataEnabled; }
	// returns true if the desired format was converted to another after loading via LoadKV3*
	bool IsFormatConverted() const { return m_bFormatConverted; }
	bool IsRootAvailabe() const { return m_bRootAvailabe; }

	// filled in after loading via LoadKV3* in binary encoding
	CUtlBuffer& GetBinaryData() { return m_BinaryData; }

	IParsingErrorListener* GetParsingErrorListener() const { return m_pParsingErrorListener; }
	void SetParsingErrorListener( IParsingErrorListener* listener ) { m_pParsingErrorListener = listener; }

	const char* AllocString( const char* pString );

	void EnableMetaData( bool bEnable );
	void CopyMetaData( KV3MetaData_t* pDest, const KV3MetaData_t* pSrc );

	void Clear();
	void Purge();

private:
	template< class ELEMENT_TYPE, class CLUSTER_TYPE, class VECTOR_TYPE >
	ELEMENT_TYPE* Alloc( CLUSTER_TYPE *&head, VECTOR_TYPE &vec );

	template< class ELEMENT_TYPE, class CLUSTER_TYPE, class VECTOR_TYPE >
	void Free( ELEMENT_TYPE *element, CLUSTER_TYPE *base, CLUSTER_TYPE *&head, VECTOR_TYPE &vec );

	CKeyValues3Array* AllocArray() { return Alloc<CKeyValues3Array, CKeyValues3ArrayCluster, ArrayClustersVec_t>( m_pArrayFreeCluster, m_ArrayClusters ); }
	CKeyValues3Table* AllocTable() { return Alloc<CKeyValues3Table, CKeyValues3TableCluster, TableClustersVec_t>( m_pTableFreeCluster, m_TableClusters ); }

	void FreeArray( CKeyValues3Array* arr ) { Free<CKeyValues3Array, CKeyValues3ArrayCluster, ArrayClustersVec_t>( arr, NULL, m_pArrayFreeCluster, m_ArrayClusters ); }
	void FreeTable( CKeyValues3Table* table ) { Free<CKeyValues3Table, CKeyValues3TableCluster, TableClustersVec_t>( table, NULL, m_pTableFreeCluster, m_TableClusters ); }

private:
	uint8 pad[ KV3_CONTEXT_SIZE - ( sizeof( BaseClass ) % KV3_CONTEXT_SIZE ) ];

	friend class KeyValues3;
};
COMPILE_TIME_ASSERT(sizeof(CKeyValues3Context) == KV3_CONTEXT_SIZE);

template < typename T > inline T KeyValues3::FromString( T defaultValue ) const { Assert( 0 ); return defaultValue; }
template <> inline bool KeyValues3::FromString( bool defaultValue ) const		{ return V_StringToBool( GetString(), defaultValue ); }
template <> inline char8 KeyValues3::FromString( char8 defaultValue ) const		{ return V_StringToInt8( GetString(), defaultValue ); }
template <> inline int8 KeyValues3::FromString( int8 defaultValue ) const		{ return V_StringToInt8( GetString(), defaultValue ); }
template <> inline uint8 KeyValues3::FromString( uint8 defaultValue ) const		{ return V_StringToUint8( GetString(), defaultValue ); }
template <> inline int16 KeyValues3::FromString( int16 defaultValue ) const		{ return V_StringToInt16( GetString(), defaultValue ); }
template <> inline uint16 KeyValues3::FromString( uint16 defaultValue ) const	{ return V_StringToUint16( GetString(), defaultValue ); }
template <> inline int32 KeyValues3::FromString( int32 defaultValue ) const		{ return V_StringToInt32( GetString(), defaultValue ); }
template <> inline uint32 KeyValues3::FromString( uint32 defaultValue ) const	{ return V_StringToUint32( GetString(), defaultValue ); }
template <> inline int64 KeyValues3::FromString( int64 defaultValue ) const		{ return V_StringToInt64( GetString(), defaultValue ); }
template <> inline uint64 KeyValues3::FromString( uint64 defaultValue ) const	{ return V_StringToUint64( GetString(), defaultValue ); }
template <> inline float32 KeyValues3::FromString( float32 defaultValue ) const	{ return V_StringToFloat32( GetString(), defaultValue ); }
template <> inline float64 KeyValues3::FromString( float64 defaultValue ) const	{ return V_StringToFloat64( GetString(), defaultValue ); }

template < typename T > inline void KeyValues3::SetDirect( T value ) { Assert( 0 ); }
template <> inline void KeyValues3::SetDirect( bool value )		{ m_Bool = value; }
template <> inline void KeyValues3::SetDirect( char8 value )	{ m_Int = ( int64 )value; }  
template <> inline void KeyValues3::SetDirect( int8 value )		{ m_Int = ( int64 )value; }  
template <> inline void KeyValues3::SetDirect( uint8 value )	{ m_UInt = ( uint64 )value; }  
template <> inline void KeyValues3::SetDirect( int16 value )	{ m_Int = ( int64 )value; }  
template <> inline void KeyValues3::SetDirect( uint16 value )	{ m_UInt = ( uint64 )value; }  
template <> inline void KeyValues3::SetDirect( int32 value )	{ m_Int = ( int64 )value; } 
template <> inline void KeyValues3::SetDirect( uint32 value )	{ m_UInt = ( uint64 )value; }  
template <> inline void KeyValues3::SetDirect( int64 value )	{ m_Int = value; }  
template <> inline void KeyValues3::SetDirect( uint64 value )	{ m_UInt = value; }  
template <> inline void KeyValues3::SetDirect( float32 value )	{ m_Double = ( float64 )value; }
template <> inline void KeyValues3::SetDirect( float64 value )	{ m_Double = value; }  

template < typename T >
T KeyValues3::GetVecBasedObj( int size, const T &defaultValue ) const
{
	T obj;
	if ( !ReadArrayFloat32( size, obj.Base() ) )
		obj = defaultValue;
	return obj;
}

template < typename T >
void KeyValues3::SetVecBasedObj( const T &obj, int size, KV3SubType_t subtype )
{
	AllocArray<float32>( size, obj.Base(), KV3_ARRAY_ALLOC_NORMAL, KV3_TYPEEX_INVALID, KV3_TYPEEX_ARRAY_FLOAT32, subtype, KV3_TYPEEX_DOUBLE, KV3_SUBTYPE_FLOAT32 );
}

template < typename T >
T KeyValues3::GetValue( T defaultValue ) const
{
	switch ( GetType() )
	{
		case KV3_TYPE_BOOL:
			return ( T )m_Bool;
		case KV3_TYPE_INT:
			return ( T )m_Int;
		case KV3_TYPE_UINT:
			return ( GetSubType() != KV3_SUBTYPE_POINTER ) ? ( T )m_UInt : defaultValue;
		case KV3_TYPE_DOUBLE:
			return ( T )m_Double;
		case KV3_TYPE_STRING:
			return FromString<T>( defaultValue );
		default:
			return defaultValue;
	}
}

template < typename T >
void KeyValues3::SetValue( T value, KV3TypeEx_t type, KV3SubType_t subtype )
{
	PrepareForType( type, subtype );
	SetDirect<T>( value );
}

template < typename T >
void KeyValues3::NormalizeArray( KV3TypeEx_t type, KV3SubType_t subtype, int size, const T* data, bool bFree )
{
	m_TypeEx = KV3_TYPEEX_ARRAY;
	m_nData = 0;
	Alloc();

	m_pArray->SetCount( size, type, subtype );

	KeyValues3** arr = m_pArray->Base();
	for ( int i = 0; i < m_pArray->Count(); ++i )
		arr[ i ]->SetDirect( data[ i ] );

	if ( bFree )
		free( (void*)data );
}

template < typename T >
void KeyValues3::AllocArray( int size, const T* data, KV3ArrayAllocType_t alloc_type, KV3TypeEx_t type_short, KV3TypeEx_t type_ptr, KV3SubType_t subtype, KV3TypeEx_t type_elem, KV3SubType_t subtype_elem )
{
	int nMaxSizeShort = sizeof( uint64 ) / sizeof( T );

	if ( type_short != KV3_TYPEEX_INVALID && size <= nMaxSizeShort )
	{
		if ( alloc_type == KV3_ARRAY_ALLOC_EXTERN && type_ptr != KV3_TYPEEX_INVALID )
		{
			PrepareForType( type_ptr, subtype );

			m_bFreeArrayMemory = false;
			m_nNumArrayElements = size;
			m_pData = (void*)data;
		}
		else
		{
			PrepareForType( type_short, subtype );

			m_bFreeArrayMemory = false;
			m_nNumArrayElements = size;
			m_nData = 0;
			memcpy( m_Data, data, size * sizeof( T ) );

			if ( alloc_type == KV3_ARRAY_ALLOC_EXTERN_FREE )
				free( (void*)data );
		}
	}
	else if ( type_ptr != KV3_TYPEEX_INVALID && size <= 31 )
	{
		PrepareForType( type_ptr, subtype );

		m_nNumArrayElements = size;

		if ( alloc_type == KV3_ARRAY_ALLOC_EXTERN )
		{
			m_bFreeArrayMemory = false;
			m_pData = (void*)data;
		}
		else if ( alloc_type == KV3_ARRAY_ALLOC_EXTERN_FREE )
		{
			m_bFreeArrayMemory = true;
			m_pData = (void*)data;
		}
		else
		{
			m_bFreeArrayMemory = true;	
			m_pData = malloc( size * sizeof( T ) );
			memcpy( m_pData, data, size * sizeof( T ) );
		}
	}
	else
	{
		PrepareForType( KV3_TYPEEX_ARRAY, subtype );

		m_pArray->SetCount( size, type_elem, subtype_elem );

		KeyValues3** arr = m_pArray->Base();
		for ( int i = 0; i < m_pArray->Count(); ++i )
			arr[ i ]->SetValue<T>( data[ i ], type_elem, subtype_elem );

		if ( alloc_type == KV3_ARRAY_ALLOC_EXTERN_FREE )
			free( (void*)data );
	}
}

template < class T >
CKeyValues3ClusterT< T >::CKeyValues3ClusterT( CKeyValues3Context* context ) : 
	m_pContext( context ), 
	m_nAllocatedElements( 0 ),
	m_pNextFree( NULL )
{
	memset( &m_Elements, 0, sizeof( m_Elements ) );
}

template < class T >
T* CKeyValues3ClusterT< T >::Alloc()
{
	Assert( IsFree() );
	int element = KV3Helpers::BitScanFwd( ~m_nAllocatedElements );
	m_nAllocatedElements |= ( 1ull << element );
	T* pElement = &m_Elements[ element ];
	Construct( pElement, element );
	return pElement;
}

template < class T >
void CKeyValues3ClusterT< T >::Free( int element )
{
	Assert( element >= 0 && element < KV3_CLUSTER_MAX_ELEMENTS );
	T* pElement = &m_Elements[ element ];
	Destruct( pElement );
	memset( (void *)pElement, 0, sizeof( T ) );
	m_nAllocatedElements &= ~( 1ull << element );
}

template < class T >
void CKeyValues3ClusterT< T >::Purge()
{
	uint64 mask = 1;
	for ( int i = 0; i < KV3_CLUSTER_MAX_ELEMENTS; ++i )
	{
		if ( ( m_nAllocatedElements & mask ) != 0 )
			m_Elements[ i ].Purge( true );
		mask <<= 1;
	}

	m_nAllocatedElements = 0;
}

template< class ELEMENT_TYPE, class CLUSTER_TYPE, class VECTOR_TYPE >
ELEMENT_TYPE* CKeyValues3Context::Alloc( CLUSTER_TYPE *&head, VECTOR_TYPE &vec )
{
	ELEMENT_TYPE* element;

	if ( head )
	{
		element = head->Alloc();

		if ( !head->IsFree() )
		{
			CLUSTER_TYPE* cluster = head->GetNextFree();
			head->SetNextFree( NULL );
			head = cluster;
		}
	}
	else
	{
		CLUSTER_TYPE* cluster = new CLUSTER_TYPE( m_pContext );
		*vec.AddToTailGetPtr() = cluster;
		head = cluster;
		element = cluster->Alloc();
	}

	return element;
}

template< class ELEMENT_TYPE, class CLUSTER_TYPE, class VECTOR_TYPE >
void CKeyValues3Context::Free( ELEMENT_TYPE *element, CLUSTER_TYPE *base, CLUSTER_TYPE *&head, VECTOR_TYPE &vec )
{
	CLUSTER_TYPE* cluster = element->GetCluster();

	Assert( cluster != NULL && cluster->GetContext() == m_pContext );

	cluster->Free( element->GetClusterElement() );

	int num_allocated = cluster->NumAllocated();

	if ( !num_allocated )
	{
		if ( cluster == base )
			return;

		vec.FindAndFastRemove( cluster );

		if ( head != cluster  )
		{
			CLUSTER_TYPE* cur = head;

			while ( cur )
			{
				CLUSTER_TYPE* next = cur->GetNextFree();
				if ( next == cluster )
					break;
				cur = next;
			}

			if ( cur )
				cur->SetNextFree( cluster->GetNextFree() );
		}
		else
		{
			head = cluster->GetNextFree();
		}

		delete cluster;
	}
	else if ( num_allocated == ( KV3_CLUSTER_MAX_ELEMENTS - 1 ) )
	{
		cluster->SetNextFree( head );
		head = cluster;
	}
}

#include "tier0/memdbgoff.h"

#endif // KEYVALUES3_H
