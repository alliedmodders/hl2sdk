#ifndef CVARIANT_H
#define CVARIANT_H

#if _WIN32
#pragma once
#endif

#include "string_t.h"
#include "datamap.h"
#include "vector.h"
#include "vector2d.h"
#include "vector4d.h"
#include "Color.h"
#include "entity2/entityidentity.h"
#include "entityhandle.h"
#include "tier1/bufferstring.h"
#include "tier1/utlscratchmemory.h"
#include "resourcefile/resourcetype.h"

#include "tier0/memdbgon.h"

FORWARD_DECLARE_HANDLE( HSCRIPT );

// ========

class CVariantDefaultAllocator
{
public:
	static void Free(void *pMemory)
	{
		free(pMemory);
	}

	static void *Allocate(int nSize)
	{
		return malloc(nSize);
	}
};

class CEntityVariantAllocator
{
public:
	static void Free(void *pMemory) { /* Skipped intentionally */ }

	static void *Allocate(int nSize)
	{
		return sm_pMemoryPool->AllocAligned(nSize, 8 * (nSize >= 16) + 8);
	}

	static void Activate(CUtlScratchMemoryPool *pMemoryPool, bool bEnable)
	{
		sm_pMemoryPool = bEnable ? pMemoryPool : NULL;
	}

private:
	static CUtlScratchMemoryPool *sm_pMemoryPool;
};

inline const char *VariantFieldTypeName(fieldtype_t eType)
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
		default:							return "unknown_variant_type";
	}
}

template <typename T> struct VariantTypeDeducer { };
#define DECLARE_DEDUCE_VARIANT_FIELDTYPE( fieldType, type ) template<> struct VariantTypeDeducer<type> { enum { FIELD_TYPE = fieldType }; };

DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VOID, void);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_CSTRING, const char *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_CSTRING, char *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR, Vector);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR, Vector *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR, const Vector &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR2D, Vector2D);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR2D, Vector2D *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR2D, const Vector2D &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR4D, Vector4D);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR4D, Vector4D *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_VECTOR4D, const Vector4D &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_COLOR32, Color);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_COLOR32, Color *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_COLOR32, const Color &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QANGLE, QAngle);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QANGLE, QAngle *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QANGLE, const QAngle &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QUATERNION, Quaternion);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QUATERNION, Quaternion *);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_QUATERNION, const Quaternion &);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_STRING, string_t);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_BOOLEAN, bool);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_CHARACTER, char);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_INT64, int64);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_UINT64, uint64);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_FLOAT32, float32);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_FLOAT64, float64);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_INT32, int32);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_UINT32, uint32);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_HSCRIPT, HSCRIPT);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_EHANDLE, CEntityHandle);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_RESOURCE, ResourceHandle_t);
DECLARE_DEDUCE_VARIANT_FIELDTYPE(FIELD_UTLSTRINGTOKEN, CUtlStringToken);

#define VariantDeduceType( T ) (fieldtype_t)VariantTypeDeducer<T>::FIELD_TYPE
#undef DECLARE_DEDUCE_VARIANT_FIELDTYPE

template <typename T>
inline const char *VariantFieldTypeName()
{
	return T::using_unknown_variant_type();
}

#define DECLARE_NAMED_VARIANT_FIELDTYPE( fieldType, strName ) template <> inline const char * VariantFieldTypeName<fieldType>() { return strName; }

DECLARE_NAMED_VARIANT_FIELDTYPE(void, "void");
DECLARE_NAMED_VARIANT_FIELDTYPE(const char *, "cstring");
DECLARE_NAMED_VARIANT_FIELDTYPE(char *, "cstring");
DECLARE_NAMED_VARIANT_FIELDTYPE(bool, "boolean");
DECLARE_NAMED_VARIANT_FIELDTYPE(char, "character");
DECLARE_NAMED_VARIANT_FIELDTYPE(int32, "int32");
DECLARE_NAMED_VARIANT_FIELDTYPE(uint32, "uint32");
DECLARE_NAMED_VARIANT_FIELDTYPE(int64, "int64");
DECLARE_NAMED_VARIANT_FIELDTYPE(uint64, "uint64");
DECLARE_NAMED_VARIANT_FIELDTYPE(float32, "float32");
DECLARE_NAMED_VARIANT_FIELDTYPE(float64, "float64");

#undef DECLARE_NAMED_VARIANT_FIELDTYPE

enum CVFlags_t
{
	// Indicates that variant has the memory allocated in place of a primitive types and it would be freed when needed
	CV_FREE = 0x01,
};

template <typename A>
class CVariantBase
{
public:
	typedef A Allocator;

	CVariantBase() :						m_type( FIELD_VOID ), m_flags( 0 )			{ m_pData = NULL; }
	CVariantBase( int32 val ) :				m_type( FIELD_INT32 ), m_flags( 0 )			{ m_int32 = val;}
	CVariantBase( uint32 val) :				m_type( FIELD_UINT32 ), m_flags( 0 )		{ m_uint32 = val; }
	CVariantBase( float32 val ) :			m_type( FIELD_FLOAT32 ), m_flags( 0 )		{ m_float32 = val; }
	CVariantBase( float64 val ) :			m_type( FIELD_FLOAT64 ), m_flags( 0 )		{ m_float64 = val; }
	CVariantBase( char val ) :				m_type( FIELD_CHARACTER ), m_flags( 0 )		{ m_char = val; }
	CVariantBase( bool val ) :				m_type( FIELD_BOOLEAN ), m_flags( 0 )		{ m_bool = val; }
	CVariantBase( HSCRIPT val ) :			m_type( FIELD_HSCRIPT ), m_flags( 0 )		{ m_hScript = val; }
	CVariantBase( CEntityHandle val ) :		m_type( FIELD_EHANDLE ), m_flags( 0 )		{ m_hEntity = val; }
	CVariantBase( CUtlStringToken val ) :	m_type( FIELD_UTLSTRINGTOKEN ), m_flags( 0 ){ m_utlStringToken = val; }
	CVariantBase( ResourceHandle_t val ) :	m_type( FIELD_RESOURCE ), m_flags( 0 )		{ m_hResource = val; }
	CVariantBase( string_t val ) :			m_type( FIELD_STRING ), m_flags( 0 )		{ m_stringt = val; }
	CVariantBase( int64 val ) :				m_type( FIELD_INT64 ), m_flags( 0 )			{ m_int64 = val; }
	CVariantBase( uint64 val ) :			m_type( FIELD_UINT64), m_flags( 0 )			{ m_uint64 = val; }

	CVariantBase( const Vector &val, bool bCopy = true) :		m_type( FIELD_VECTOR ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector *val, bool bCopy = true) :		m_type( FIELD_VECTOR ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const QAngle &val, bool bCopy = true) :		m_type( FIELD_QANGLE ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const QAngle *val, bool bCopy = true) :		m_type( FIELD_QANGLE ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const Vector2D &val, bool bCopy = true ) :	m_type( FIELD_VECTOR2D ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector2D *val, bool bCopy = true ) :	m_type( FIELD_VECTOR2D ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const Vector4D &val, bool bCopy = true ) :	m_type( FIELD_VECTOR4D ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Vector4D *val, bool bCopy = true ) :	m_type( FIELD_VECTOR4D ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const Quaternion &val, bool bCopy = true ) :	m_type( FIELD_QUATERNION ), m_flags( 0 )	{ CopyData(val, bCopy); }
	CVariantBase( const Quaternion *val, bool bCopy = true ) :	m_type( FIELD_QUATERNION ), m_flags( 0 )	{ CopyData(*val, bCopy); }
	CVariantBase( const Color &val, bool bCopy = true ) :		m_type( FIELD_COLOR32 ), m_flags( 0 )		{ CopyData(val, bCopy); }
	CVariantBase( const Color *val, bool bCopy = true ) :		m_type( FIELD_COLOR32 ), m_flags( 0 )		{ CopyData(*val, bCopy); }
	CVariantBase( const char *val, bool bCopy = true ) :		m_type( FIELD_CSTRING ), m_flags( 0 )		{ CopyData(val, bCopy); }

	// Checks if the stored value is of type FIELD_VOID
	bool IsNull() const						{ return (m_type == FIELD_VOID ); }

	// Copies the src data into an internal value, setting bForceCopy would allocate its own memory to store the contents
	void CopyData(const char *src, bool bForceCopy)
	{
		Free();

		m_type = FIELD_CSTRING;

		if(src && bForceCopy)
		{
			int len = strlen(src) + 1;
			m_pszString = (char *)Allocator::Allocate(len);
			memcpy((void *)m_pszString, src, len);

			m_flags |= CV_FREE;
		}
		else
		{
			m_pszString = src;
		}
	}

	// Copies the src data into an internal value, setting bForceCopy would allocate its own memory to store the contents
	template <typename T>
	void CopyData(const T &src, bool bForceCopy)
	{
		Free();

		m_type = VariantDeduceType(T);

		if(bForceCopy)
		{
			m_pData = Allocator::Allocate(sizeof(T));
			*this = src;

			m_flags |= CV_FREE;
		}
		else
		{
			m_pData = *(void **)&src;
		}
	}

	operator int32() const					{ Assert( m_type == FIELD_INT32 );			return m_int32; }
	operator uint32() const					{ Assert( m_type == FIELD_UINT32 );			return m_uint32; }
	operator int64() const					{ Assert( m_type == FIELD_INT64);			return m_int64; }
	operator uint64() const					{ Assert( m_type == FIELD_UINT64);			return m_uint64; }
	operator float32() const				{ Assert( m_type == FIELD_FLOAT32 );		return m_float32; }
	operator float64() const				{ Assert( m_type == FIELD_FLOAT64 );		return m_float64; }
	operator const string_t() const			{ Assert( m_type == FIELD_STRING );			return m_stringt; }
	operator const char *() const			{ Assert( m_type == FIELD_CSTRING );		return ( m_pszString ) ? m_pszString : ""; }
	operator const Vector &() const			{ Assert( m_type == FIELD_VECTOR );			static Vector vecNull(0, 0, 0); return (m_pVector) ? *m_pVector : vecNull; }
	operator const Vector2D &() const		{ Assert( m_type == FIELD_VECTOR2D );		static Vector2D vecNull(0, 0); return (m_pVector2D) ? *m_pVector2D : vecNull; }
	operator const Vector4D &() const		{ Assert( m_type == FIELD_VECTOR4D );		static Vector4D vecNull(0, 0, 0, 0); return (m_pVector4D) ? *m_pVector4D : vecNull; }
	operator const QAngle &() const			{ Assert( m_type == FIELD_QANGLE);			static QAngle angNull(0, 0, 0); return (m_pQAngle) ? *m_pQAngle : angNull; }
	operator const Quaternion &() const		{ Assert( m_type == FIELD_QUATERNION);		static Quaternion quatNull(0, 0, 0, 0); return (m_pQuaternion) ? *m_pQuaternion : quatNull; }
	operator const Color &() const			{ Assert( m_type == FIELD_COLOR32);			static Color colorNull(0, 0, 0); return (m_pColor) ? *m_pColor : colorNull; }
	operator char() const					{ Assert( m_type == FIELD_CHARACTER );		return m_char; }
	operator bool() const					{ Assert( m_type == FIELD_BOOLEAN );		return m_bool; }
	operator HSCRIPT() const				{ Assert( m_type == FIELD_HSCRIPT );		return m_hScript; }
	operator CEntityHandle() const			{ Assert( m_type == FIELD_EHANDLE);			return m_hEntity; }
	operator CUtlStringToken() const		{ Assert( m_type == FIELD_UTLSTRINGTOKEN);	return m_utlStringToken; }
	operator ResourceHandle_t() const		{ Assert( m_type == FIELD_RESOURCE);		return m_hResource; }

	void operator=( int32 i ) 				{ m_type = FIELD_INT32; m_int32 = i; }
	void operator=( uint32 u )				{ m_type = FIELD_UINT32; m_uint32 = u; }
	void operator=( int64 i ) 				{ m_type = FIELD_INT64; m_int64 = i; }
	void operator=( uint64 u )				{ m_type = FIELD_UINT64; m_uint64 = u; }
	void operator=( float32 f ) 			{ m_type = FIELD_FLOAT32; m_float32 = f; }
	void operator=( float64 d )				{ m_type = FIELD_FLOAT64; m_float64 = d; }
	void operator=( const Vector &vec )		{ m_type = FIELD_VECTOR; *(Vector *)m_pVector = vec; }
	void operator=( const Vector *vec )		{ m_type = FIELD_VECTOR; m_pVector = vec; }
	void operator=( const Vector2D &vec )	{ m_type = FIELD_VECTOR2D; *(Vector2D *)m_pVector2D = vec; }
	void operator=( const Vector2D *vec )	{ m_type = FIELD_VECTOR2D; m_pVector2D = vec; }
	void operator=( const Vector4D &vec )	{ m_type = FIELD_VECTOR4D; *(Vector4D *)m_pVector4D = vec; }
	void operator=( const Vector4D *vec )	{ m_type = FIELD_VECTOR4D; m_pVector4D = vec; }
	void operator=( const QAngle &ang )		{ m_type = FIELD_QANGLE; *(QAngle *)m_pQAngle = ang; }
	void operator=( const QAngle *ang )		{ m_type = FIELD_QANGLE; m_pQAngle = ang; }
	void operator=( const Quaternion &quat ){ m_type = FIELD_QUATERNION; *(Quaternion *)m_pQuaternion = quat; }
	void operator=( const Quaternion *quat ){ m_type = FIELD_QUATERNION; m_pQuaternion = quat; }
	void operator=( const Color &color )	{ m_type = FIELD_COLOR32; *(Color *)m_pColor = color; }
	void operator=( const Color *color )	{ m_type = FIELD_COLOR32; m_pColor = color; }
	void operator=( string_t psz )			{ m_type = FIELD_STRING; m_stringt = psz; }
	void operator=( const char *psz )		{ m_type = FIELD_CSTRING; m_pszString = psz; }
	void operator=( char c )				{ m_type = FIELD_CHARACTER; m_char = c; }
	void operator=( bool b ) 				{ m_type = FIELD_BOOLEAN; m_bool = b; }
	void operator=( HSCRIPT h ) 			{ m_type = FIELD_HSCRIPT; m_hScript = h; }
	void operator=( CEntityHandle eh) 		{ m_type = FIELD_EHANDLE; m_hEntity = eh; }
	void operator=( CUtlStringToken tok ) 	{ m_type = FIELD_UTLSTRINGTOKEN; m_utlStringToken = tok; }
	void operator=( ResourceHandle_t r ) 	{ m_type = FIELD_RESOURCE; m_hResource = r; }

	~CVariantBase()
	{
		Free();
	}

	// Frees the internal buffer and resets the value to be FIELD_VOID
	void Free()
	{
		if(m_flags & CV_FREE)
		{
			Allocator::Free(m_pData);
			m_flags &= ~CV_FREE;
		}

		m_pData = NULL;
		m_type = FIELD_VOID;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(CBufferString &buf)
	{
		buf.Purge(0);

		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_FLOAT32:		buf.Format("%g", m_float32); return true;
			case FIELD_FLOAT64:		buf.Format("%g", m_float64); return true;
			case FIELD_INT32:		buf.Format("%d", m_int32); return true;
			case FIELD_UINT32:		buf.Format("%u", m_uint32); return true;
			case FIELD_INT64:		buf.Format("%lld", m_int64); return true;
			case FIELD_UINT64:		buf.Format("%llu", m_uint64); return true;
			case FIELD_BOOLEAN:		buf.Insert(0, m_bool ? "true" : "false"); return true;
			case FIELD_STRING:		buf.Insert(0, m_stringt.ToCStr()); return true;
			case FIELD_CSTRING:		buf.Insert(0, m_pszString ? m_pszString : "(null)"); return true;
			case FIELD_CHARACTER:	buf.Format("%c", m_char); return true;
			case FIELD_VECTOR2D:	buf.Format("%g %g", m_pVector2D->x, m_pVector2D->y); return true;
			case FIELD_COLOR32:		buf.Format("%d %d %d %d", m_pColor->r(), m_pColor->g(), m_pColor->b(), m_pColor->a()); return true;

			case FIELD_VECTOR:
			case FIELD_QANGLE:
			{
				buf.Format("%g %g %g", m_pVector->x, m_pVector->y, m_pVector->z);
				return true;
			}

			case FIELD_QUATERNION:
			case FIELD_VECTOR4D:
			{
				buf.Format("%g %g %g %g", m_pVector4D->x, m_pVector4D->y, m_pVector4D->z, m_pVector4D->w);
				return true;
			}

			default: Warning("No conversion from %s to string at the moment!\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(float *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		*pDest = 0.0; return false;
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
					*pDest = V_atof(m_pszString);
					return true;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					*pDest = V_atof(m_stringt.ToCStr());
					return true;
				}
			}

			default: Warning("No conversion from %s to float right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(int *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		*pDest = 0; return false;
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
					*pDest = V_atoi(m_pszString);
					return true;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					*pDest = V_atoi(m_stringt.ToCStr());
					return true;
				}
			}

			default: Warning("No conversion from %s to int right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(bool *pDest)
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
					*pDest = V_StringToBool(m_pszString, false, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					*pDest = V_StringToBool(m_stringt.ToCStr(), false, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to bool right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(string_t *pDest)
	{
		if(m_type == FIELD_STRING)
		{
			*pDest = *this;
			return true;
		}
		else if(m_type == FIELD_CSTRING)
		{
			if(m_pszString)
			{
				*pDest = MAKE_STRING(m_pszString);
				return true;
			}
		}
		else
		{
			Warning("No conversion from %s to string_t right now\n", VariantFieldTypeName(m_type));
		}
		
		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(Vector *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_VECTOR:		*pDest = *this; return true;
			case FIELD_CSTRING:	
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToVector(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToVector(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to Vector right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(Vector2D *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_VECTOR2D:	*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToVector2D(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToVector2D(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to Vector2D right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(Vector4D *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_VECTOR4D:	*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToVector4D(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToVector4D(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to Vector4D right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(Quaternion *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_QUATERNION:	*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToQuaternion(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToQuaternion(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to Quaternion right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(QAngle *pDest)
	{
		switch(m_type)
		{
			case FIELD_VOID:		return false;
			case FIELD_QANGLE:		*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToQAngle(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToQAngle(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to QAngle right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(Color *pDest)
	{
		switch(m_type)
		{
			case FIELD_COLOR32:		*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0] != '\0')
				{
					bool successful = false;
					V_StringToColor(m_pszString, *pDest, &successful);
					return successful;
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					bool successful = false;
					V_StringToColor(m_stringt.ToCStr(), *pDest, &successful);
					return successful;
				}
			}

			default: Warning("No conversion from %s to Color right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(ResourceHandle_t *pDest)
	{
		if(m_type == FIELD_RESOURCE)
		{
			*pDest = *this;
			return true;
		}
		else
		{
			Warning("No conversion from %s to ResourceHandle_t right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(HSCRIPT *pDest)
	{
		if(m_type == FIELD_HSCRIPT)
		{
			*pDest = *this;
			return true;
		}
		else
		{
			Warning("No conversion from %s to HSCRIPT right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(CUtlStringToken *pDest)
	{
		if(m_type == FIELD_UTLSTRINGTOKEN)
		{
			*pDest = *this;
			return true;
		}
		else
		{
			Warning("No conversion from %s to CUtlStringToken right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(CEntityHandle *pDest)
	{
		switch(m_type)
		{
			case FIELD_EHANDLE:		*pDest = *this; return true;
			case FIELD_CSTRING:
			{
				if(m_pszString && m_pszString[0])
				{
					// TODO: Perform actual entity handle lookup via CEntitySystem::FindFirstEntityHandleByName()
					Assert(false);
				}
			}
			case FIELD_STRING:
			{
				if(m_stringt.ToCStr()[0])
				{
					// TODO: Perform actual entity handle lookup via CEntitySystem::FindFirstEntityHandleByName()
					Assert(false);
				}
			}

			default: Warning("No conversion from %s to EHANDLE2 right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	bool AssignTo(CEntityInstance *pDest)
	{
		if(m_type == FIELD_EHANDLE)
		{
			// TODO: Perform actual CEntityInstance lookup via CEntityHandle
			Assert(false);

			return true;
		}
		else
		{
			Warning("No conversion from %s to CEntityInstance right now\n", VariantFieldTypeName(m_type));
		}

		return false;
	}

	// Copies the contents of the value into a pDest, also converts the content when possible
	template <typename T>
	bool AssignTo(CVariantBase<T> *pDest)
	{
		switch(m_type)
		{
			case FIELD_VECTOR:		pDest->CopyData(*m_pVector, true); return true;
			case FIELD_VECTOR2D:	pDest->CopyData(*m_pVector2D, true); return true;
			case FIELD_VECTOR4D:	pDest->CopyData(*m_pVector4D, true); return true;
			case FIELD_QUATERNION:	pDest->CopyData(*m_pQuaternion, true); return true;
			case FIELD_QANGLE:		pDest->CopyData(*m_pQAngle, true); return true;
			case FIELD_COLOR32:		pDest->CopyData(*m_pColor, true); return true;
			case FIELD_CSTRING:		pDest->CopyData(m_pszString, true); return true;
			default:
			{
				pDest->Free();
				pDest->m_type = m_type;
				pDest->m_pData = m_pData;
				return true;
			}
		}
	}

	template <typename T>
	bool AssignTo(T *pDest)
	{
		fieldtype_t destType = VariantDeduceType(T);
		if(destType == FIELD_VOID)
		{
			return false;
		}

		if(destType == m_type)
		{
			*pDest = *this;
			return true;
		}

		if(destType != FIELD_VECTOR && destType != FIELD_QUATERNION && destType != FIELD_VECTOR2D &&
		   destType != FIELD_VECTOR4D && destType != FIELD_CSTRING && destType != FIELD_QANGLE)
		{
			switch(m_type)
			{
				case FIELD_VOID:		*pDest = 0; return false;
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
			Warning("No free conversion of %s variant to %s right now\n", VariantFieldTypeName(m_type), VariantFieldTypeName<T>());
			*pDest = 0;
		}

		return false;
	}

	// Allocates own buffers and copies the internal value when needed, if silent = false, emits a global warning
	void ConvertToCopiedData(bool silent = true)
	{
		switch(m_type)
		{
			case FIELD_VECTOR:		CopyData(*m_pVector, true); break;
			case FIELD_VECTOR2D:	CopyData(*m_pVector2D, true); break;
			case FIELD_VECTOR4D:	CopyData(*m_pVector4D, true); break;
			case FIELD_QUATERNION:	CopyData(*m_pQuaternion, true); break;
			case FIELD_QANGLE:		CopyData(*m_pQAngle, true); break;
			case FIELD_COLOR32:		CopyData(*m_pColor, true); break;
			case FIELD_CSTRING:		CopyData(m_pszString, true); break;
			default:
			{
				if(!silent)
				{
					Warning("Attempted to ConvertToCopiedData for unsupported type (%d)\n", m_type);
				}
			}
		}
	}

	template <typename T>
	T Get()
	{
		T value = {};
		AssignTo(&value);
		return value;
	}

	// Sets the internal value to the pData content, doesn't allocate memory nor copies the content.
	// Be sure to call ConvertToCopiedData() when required
	void Set(fieldtype_t ftype, void *pData)
	{
		switch(ftype)
		{
			case FIELD_VOID:			Free(); m_type = FIELD_VOID; m_pData = NULL; return;
			case FIELD_FLOAT32:			CopyData(*(float32 *)pData, false); return;
			case FIELD_FLOAT64:			CopyData(*(float64 *)pData, false); return;
			case FIELD_INT32:			CopyData(*(int32 *)pData, false); return;
			case FIELD_UINT32:			CopyData(*(uint32 *)pData, false); return;
			case FIELD_INT64:			CopyData(*(int64 *)pData, false); return;
			case FIELD_UINT64:			CopyData(*(uint64 *)pData, false); return;
			case FIELD_BOOLEAN:			CopyData(*(bool *)pData, false); return;
			case FIELD_CHARACTER:		CopyData(*(char *)pData, false); return;
			case FIELD_STRING:			CopyData(*(string_t *)pData, false); return;
			case FIELD_CSTRING:			CopyData(*(const char **)pData, false); return;
			case FIELD_VECTOR:			CopyData((Vector *)pData, false); return;
			case FIELD_VECTOR2D:		CopyData((Vector2D *)pData, false); return;
			case FIELD_VECTOR4D:		CopyData((Vector4D *)pData, false); return;
			case FIELD_COLOR32:			CopyData((Color *)pData, false); return;
			case FIELD_QANGLE:			CopyData((QAngle *)pData, false); return;
			case FIELD_QUATERNION:		CopyData((Quaternion *)pData, false); return;
			case FIELD_HSCRIPT:			CopyData(*(HSCRIPT *)pData, false); return;
			case FIELD_EHANDLE:			CopyData(*(CEntityHandle *)pData, false); return;
			case FIELD_RESOURCE:		CopyData(*(ResourceHandle_t *)pData, false); return;
			case FIELD_UTLSTRINGTOKEN:	CopyData(*(CUtlStringToken *)pData, false); return;
		}
	}

	// Converts underlying value into a different type
	bool Convert(fieldtype_t newType)
	{
		if(newType == m_type)
		{
			return true;
		}

		bool successful = false;
		void *pData = NULL;
		switch(newType)
		{
			case FIELD_VOID:			successful = true; Free(); m_type = FIELD_VOID; m_pData = NULL; break;
			case FIELD_FLOAT32:			if((successful = AssignTo((float32 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_FLOAT64:			if((successful = AssignTo((float64 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_INT32:			if((successful = AssignTo((int32 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_UINT32:			if((successful = AssignTo((uint32 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_INT64:			if((successful = AssignTo((int64 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_UINT64:			if((successful = AssignTo((uint64 *)&pData))) { Set(newType, &pData); } break;
			case FIELD_BOOLEAN:			if((successful = AssignTo((bool *)&pData))) { Set(newType, &pData); } break;
			case FIELD_CHARACTER:		if((successful = AssignTo((char *)&pData))) { Set(newType, &pData); } break;
			case FIELD_STRING:			if((successful = AssignTo((string_t *)&pData))) { Set(newType, &pData); } break;
			case FIELD_CSTRING:			successful = true; CopyData(ToString(), true); break;
			case FIELD_HSCRIPT:			if((successful = AssignTo((HSCRIPT *)&pData))) { Set(newType, &pData); } break;
			case FIELD_EHANDLE:			if((successful = AssignTo((CEntityHandle *)&pData))) { Set(newType, &pData); } break;
			case FIELD_RESOURCE:		if((successful = AssignTo((ResourceHandle_t *)&pData))) { Set(newType, &pData); } break;
			case FIELD_UTLSTRINGTOKEN:	if((successful = AssignTo((CUtlStringToken *)&pData))) { Set(newType, &pData); } break;
			case FIELD_VECTOR:			{ Vector vec; if((successful = AssignTo(&vec))) { CopyData(vec, true); } break; }
			case FIELD_VECTOR2D:		{ Vector2D vec; if((successful = AssignTo(&vec))) { CopyData(vec, true); } break; }
			case FIELD_VECTOR4D:		{ Vector4D vec; if((successful = AssignTo(&vec))) { CopyData(vec, true); } break; }
			case FIELD_COLOR32:			{ Color clr; if((successful = AssignTo(&clr))) { CopyData(clr, true); } break; }
			case FIELD_QANGLE:			{ QAngle ang; if((successful = AssignTo(&ang))) { CopyData(ang, true); } break; }
			case FIELD_QUATERNION:		{ Quaternion quat; if((successful = AssignTo(&quat))) { CopyData(quat, true); } break; }
		}

		if(successful)
		{
			m_type = newType;
		}

		return successful;
	}

	const char *ToString()
	{
		static CBufferStringN<200> szBuf;
		AssignTo(szBuf);
		return szBuf.Get();
	}

public:
	union
	{
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
		const Quaternion *m_pQuaternion;
		const Color *m_pColor;
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

typedef CVariantBase<CVariantDefaultAllocator> CVariant;
typedef CVariantBase<CEntityVariantAllocator> CEntityVariant;

typedef CVariant variant_t;

#include "tier0/memdbgoff.h"

#endif // CVARIANT_H