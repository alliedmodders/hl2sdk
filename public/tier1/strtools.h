//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef TIER1_STRTOOLS_H
#define TIER1_STRTOOLS_H

#include "tier0/basetypes.h"

#ifdef _WIN32
#pragma once
#elif POSIX
#include <ctype.h>
#include <wchar.h>
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>

// Forward declaration
class CBufferString;
class Vector;
class Vector2D;
class Vector4D;
class Quaternion;
class Color;
class QAngle;
class CUtlString;

template< class T, class I> class CUtlMemory;
template< class T, class A> class CUtlVector;

abstract_class IParsingErrorListener
{
	virtual void OnParsingError(CBufferString &error_msg) = 0;
};

#define PARSING_FLAG_NONE							(0)
#define PARSING_FLAG_ERROR_ASSERT					(1 << 0) // Triggers debug assertion on parsing errors, the default state
#define PARSING_FLAG_SKIP_ASSERT					(1 << 1) // Internal flag that is set when assertion is triggered, could also be used to prevent debug assertions
#define PARSING_FLAG_EMIT_WARNING					(1 << 2) // Emits global console warning on parsing errors, the default state
#define PARSING_FLAG_SKIP_WARNING					(1 << 3) // Internal flag that is set when global warning is emitted, could also be used to prevent warning messages
#define PARSING_FLAG_SILENT							(1 << 4) // Won't call callback when parsing errors are encountered
#define PARSING_FLAG_ERROR_IF_EMPTY					(1 << 5) // Emits parsing error if the input string was empty or NULL
#define PARSING_FLAG_UNK006							(1 << 6)
#define PARSING_FLAG_USE_BASE_AUTO					(1 << 7) // Use auto detection of a number base when parsing (uses https://en.cppreference.com/w/cpp/string/basic_string/stol under the hood, so same base rules applies)
#define PARSING_FLAG_USE_BASE_16					(1 << 8) // Use base of 16 when parsing

// Unicode string conversion policies - what to do if an illegal sequence is encountered
enum EStringConvertErrorPolicy
{
	_STRINGCONVERTFLAG_SKIP =		1,
	_STRINGCONVERTFLAG_FAIL =		2,
	_STRINGCONVERTFLAG_ASSERT =		4,
	_STRINGCONVERTFLAG_UNK001 =		8,

	STRINGCONVERT_REPLACE =			0,
	STRINGCONVERT_SKIP =			_STRINGCONVERTFLAG_SKIP,
	STRINGCONVERT_FAIL =			_STRINGCONVERTFLAG_FAIL,

	STRINGCONVERT_ASSERT_REPLACE =	_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_REPLACE,
	STRINGCONVERT_ASSERT_SKIP =		_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_SKIP,
	STRINGCONVERT_ASSERT_FAIL =		_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_FAIL,
};

// UNDONE: Find a non-compiler-specific way to do this
#ifdef _WIN32
#ifndef _VA_LIST_DEFINED

#ifdef  _M_ALPHA

struct va_list 
{
    char *a0;       /* pointer to first homed integer argument */
    int offset;     /* byte offset of next parameter */
};

#else  // !_M_ALPHA

typedef char *  va_list;

#endif // !_M_ALPHA

#define _VA_LIST_DEFINED

#endif   // _VA_LIST_DEFINED

#elif POSIX
#include <stdarg.h>
#endif

//-----------------------------------------------------------------------------
// Portable versions of standard string functions
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void			V_tier0_memset( void *dest, int fill, size_t count );
PLATFORM_INTERFACE void			V_tier0_memcpy( void *dest, const void *src, size_t count );
PLATFORM_INTERFACE void			V_tier0_memmove( void *dest, const void *src, size_t count );
PLATFORM_INTERFACE int			V_tier0_memcmp( const void *m1, const void *m2, size_t count );

PLATFORM_INTERFACE int			V_tier0_strlen( const char *str );
PLATFORM_INTERFACE int			V_tier0_strlen16( const uchar16 *str );
PLATFORM_INTERFACE int			V_tier0_strlen32( const uchar32 *str );
PLATFORM_INTERFACE int			V_tier0_wcslen( const wchar_t *str );

PLATFORM_INTERFACE void			V_tier0_strcpy( char *dest, const char *src );
PLATFORM_INTERFACE void			_V_strncpy( char *pDest, const char *pSrc, int maxLen );
PLATFORM_INTERFACE void			V_tier0_strcpy32( uchar32 *dest, const uchar32 *src );
PLATFORM_INTERFACE void			_V_strncpy32_bytes( uchar32 *pDest, const uchar32 *pSrc, int bytes );
PLATFORM_INTERFACE void			V_tier0_wcscpy( wchar_t *dest, const wchar_t *src );
PLATFORM_INTERFACE void			_V_wcsncpy_bytes( OUT_Z_BYTECAP( bytes ) wchar_t *pDest, const wchar_t *pSrc, int bytes );

PLATFORM_INTERFACE char *		V_tier0_strrchr( const char *s, char c );
PLATFORM_INTERFACE char *		V_strnchr( const char *s, char c, int n );
PLATFORM_INTERFACE uchar32 *	V_strchr32( const uchar32 *s, uchar32 c );
PLATFORM_INTERFACE wchar_t *	V_tier0_wcschr( const wchar_t *s, wchar_t c );

PLATFORM_INTERFACE int			V_tier0_strcmp( const char *s1, const char *s2 );
PLATFORM_INTERFACE int			_V_strncmp( const char *s1, const char *s2, int n );
PLATFORM_INTERFACE int			V_strcmp32( const uchar32 *s1, const uchar32 *s2 );
PLATFORM_INTERFACE int			V_tier0_wcscmp( const wchar_t *s1, const wchar_t *s2 );

PLATFORM_INTERFACE int			V_stricmp_fast( const char *s1, const char *s2 );
// A special high-performance case-insensitive compare function that in
// a single call distinguishes between exactly matching strings,
// strings equal in case-insensitive way, and not equal strings:
//   returns 0 if strings match exactly
//   returns >0 if strings match in a case-insensitive way, but do not match exactly
//   returns <0 if strings do not match even in a case-insensitive way
PLATFORM_INTERFACE int			V_stricmp_fast_NegativeForUnequal( const char *s1, const char *s2 );
PLATFORM_INTERFACE int			_V_strnicmp_fast( const char *s1, const char *s2, int n );
PLATFORM_INTERFACE int			V_wcsicmp( const wchar_t *s1, const wchar_t *s2 );
PLATFORM_INTERFACE int			V_wcsnicmp_cch( const wchar_t *s1, const wchar_t *s2, int symbols );

PLATFORM_INTERFACE char *		V_tier0_strstr( const char *s1, const char *search );
PLATFORM_INTERFACE uchar32 *	V_strstr32( const uchar32 *s1, const uchar32 *search );

PLATFORM_INTERFACE char *		V_strupper_fast( char *start );
PLATFORM_INTERFACE uchar32 *	V_towupper32( uchar32 *start );
PLATFORM_INTERFACE wchar_t *	V_towupper( wchar_t *start );

PLATFORM_INTERFACE char *		V_strlower_fast( char *start );
PLATFORM_INTERFACE uchar32 *	V_towlower32( uchar32 *start );
PLATFORM_INTERFACE wchar_t *	V_towlower( wchar_t *start );

PLATFORM_INTERFACE int64		V_atoi( const char *str );
PLATFORM_INTERFACE int64 		V_atoi64( const char *str );
PLATFORM_INTERFACE uint64 		V_atoui64( const char *str );
PLATFORM_INTERFACE double		V_atof( const char *str );
PLATFORM_INTERFACE float		V_atofloat32( const char *str );
PLATFORM_INTERFACE double		V_atofloat64( const char *str );

PLATFORM_INTERFACE double		V_strtod( const char *str, char **endptr = NULL );
PLATFORM_INTERFACE double		V_wcstod( const wchar_t *str, wchar_t **endptr = NULL );
PLATFORM_INTERFACE int64		V_strtoi64( const char *str, char **endptr = NULL );
PLATFORM_INTERFACE int64		V_wcstoi64( const wchar_t *str, wchar_t **endptr = NULL );
PLATFORM_INTERFACE uint64		V_strtoui64( const char *str, char **endptr = NULL );
PLATFORM_INTERFACE uint64		V_wcstoui64( const wchar_t *str, wchar_t **endptr = NULL );

PLATFORM_INTERFACE char *		V_strtok( const char *str, const char *delim );

PLATFORM_OVERLOAD const char *	V_stristr_fast( const char *str, const char *search );
PLATFORM_INTERFACE const char *	_V_strnistr_fast( const char *str, const char *search, int n );
PLATFORM_OVERLOAD const wchar_t *V_wcsistr( const wchar_t *str, const wchar_t *search );

PLATFORM_OVERLOAD int			V_strnlen( const char *str, int n );
PLATFORM_OVERLOAD int			V_strnlen( const uchar32 *str, int n );
PLATFORM_OVERLOAD int			V_strnlen( const wchar_t *str, int n );

PLATFORM_INTERFACE int			_V_strcspn( const char *s1, const char *s2 );

#define COPY_ALL_CHARACTERS -1
PLATFORM_INTERFACE char *		_V_strncat( INOUT_Z_CAP( size ) char *s1, const char *s2, size_t size, int max_chars_to_copy = COPY_ALL_CHARACTERS );
inline void V_strcat( INOUT_Z_CAP( cchDest ) char *dest, const char *src, int cchDest )
{
	_V_strncat( dest, src, (int)cchDest, COPY_ALL_CHARACTERS );
}
template <size_t cchDest> char *V_strcat_safe( INOUT_Z_ARRAY char (&pDest)[cchDest], const char *pSrc, int nMaxCharsToCopy=COPY_ALL_CHARACTERS )
{ 
	return _V_strncat( pDest, pSrc, (int)cchDest, nMaxCharsToCopy );
}

PLATFORM_INTERFACE wchar_t *	V_wcsncat_cch( INOUT_Z_CAP( cchDest ) wchar_t *pDest, const wchar_t *pSrc, int cchDest, int nMaxCharsToCopy=COPY_ALL_CHARACTERS );
inline void V_wcscat( INOUT_Z_CAP(cchDest) wchar_t *dest, const wchar_t *src, int cchDest )
{
	V_wcsncat_cch( dest, src, cchDest, COPY_ALL_CHARACTERS );
}
template <size_t cchDest> wchar_t *V_wcscat_safe( INOUT_Z_ARRAY wchar_t( &pDest )[cchDest], const wchar_t *pSrc, int nMaxCharsToCopy=COPY_ALL_CHARACTERS )
{
	return V_wcsncat_cch( pDest, pSrc, cchDest, nMaxCharsToCopy );
}

PLATFORM_INTERFACE int			V_snprintf( OUT_Z_CAP( destLen ) char *pDest, int destLen, PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 3, 4 );

PLATFORM_INTERFACE int			V_snprintfcat( OUT_Z_CAP( destLen ) char *pDest, int destLen, PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 3, 4 );
PLATFORM_INTERFACE int			V_snwprintf_bytes( OUT_Z_CAP( bytes ) wchar_t *pDest, int bytes, PRINTF_FORMAT_STRING const wchar_t *pFormat, ... );
PLATFORM_INTERFACE int			V_snwprintf_cch( OUT_Z_CAP( symbols ) wchar_t *pDest, int symbols, PRINTF_FORMAT_STRING const wchar_t *pFormat, ... );

PLATFORM_INTERFACE int			V_vsnprintf( OUT_Z_CAP( maxLenInCharacters ) char *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const char *pFormat, va_list params );
PLATFORM_INTERFACE int			V_vsnprintfcat( OUT_Z_CAP( maxLenInCharacters ) char *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const char *pFormat, va_list params );
template <size_t maxLenInCharacters> int V_vsprintf_safe( OUT_Z_ARRAY char (&pDest)[maxLenInCharacters], PRINTF_FORMAT_STRING const char *pFormat, va_list params ) { return V_vsnprintf( pDest, maxLenInCharacters, pFormat, params ); }

template <size_t maxLenInChars> int V_sprintf_safe( OUT_Z_ARRAY char( &pDest )[maxLenInChars], PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 2, 3 );
template <size_t maxLenInChars> int V_sprintf_safe( OUT_Z_ARRAY char( &pDest )[maxLenInChars], PRINTF_FORMAT_STRING const char *pFormat, ... )
{
	va_list params;
	va_start( params, pFormat );
	int result = V_vsnprintf( pDest, maxLenInChars, pFormat, params );
	va_end( params );
	return result;
}

// Append formatted text to an array in a safe manner -- always null-terminated, truncation rather than buffer overrun.
template <size_t maxLenInChars> int V_sprintfcat_safe( INOUT_Z_ARRAY char (&pDest)[maxLenInChars], PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 2, 3 );
template <size_t maxLenInChars> int V_sprintfcat_safe( INOUT_Z_ARRAY char (&pDest)[maxLenInChars], PRINTF_FORMAT_STRING const char *pFormat, ... )
{
	va_list params;
	va_start( params, pFormat );
	size_t usedLength = V_strlen(pDest);
	// This code is here to check against buffer overruns when uninitialized arrays are passed in.
	// It should never be executed. Unfortunately we can't assert in this header file.
	if ( usedLength >= maxLenInChars )
		usedLength = 0;
	int result = V_vsnprintf( pDest + usedLength, maxLenInChars - usedLength, pFormat, params );
	va_end( params );
	return result;
}

PLATFORM_INTERFACE int			V_vsnwprintf_cch( OUT_Z_CAP( maxLenInCharacters ) wchar_t *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const wchar_t *pFormat, va_list params );
template <size_t maxLenInCharacters> int V_vswprintf_safe( OUT_Z_ARRAY wchar_t( &pDest )[maxLenInCharacters], PRINTF_FORMAT_STRING const wchar_t *pFormat, va_list params ) { return V_vsnwprintf_cch( pDest, maxLenInCharacters, pFormat, params ); }

PLATFORM_INTERFACE int			V_vsnprintfRet( OUT_Z_CAP(maxLenInCharacters) char *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const char *pFormat, va_list params, bool *pbTruncated );
template <size_t maxLenInCharacters> int V_vsprintfRet_safe( OUT_Z_ARRAY char (&pDest)[maxLenInCharacters], PRINTF_FORMAT_STRING const char *pFormat, va_list params, bool *pbTruncated ) { return V_vsnprintfRet( pDest, maxLenInCharacters, pFormat, params, pbTruncated ); }

// FMTFUNCTION can only be used on ASCII functions, not wide-char functions.
int V_snwprintf( OUT_Z_CAP(maxLenInCharacters) wchar_t *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const wchar_t *pFormat, ... );
template <size_t maxLenInChars> int V_swprintf_safe( OUT_Z_ARRAY wchar_t (&pDest)[maxLenInChars], PRINTF_FORMAT_STRING const wchar_t *pFormat, ... )
{
	va_list params;
	va_start( params, pFormat );
	int result = V_vsnwprintf_cch( pDest, maxLenInChars, pFormat, params );
	va_end( params );
	return result;
}

// this is locale-unaware and therefore faster version of standard isdigit()
// It also avoids sign-extension errors.
inline bool V_isdigit( char c )
{
	return c >= '0' && c <= '9';
}

PLATFORM_INTERFACE bool V_isdigit_str( const char *str );
PLATFORM_INTERFACE bool V_iswdigit( int c );

inline bool V_isempty( const char* pszString ) { return !pszString || !pszString[ 0 ]; }

// The islower/isdigit/etc. functions all expect a parameter that is either
// 0-0xFF or EOF. It is easy to violate this constraint simply by passing
// 'char' to these functions instead of unsigned char.
// The V_ functions handle the char/unsigned char mismatch by taking a
// char parameter and casting it to unsigned char so that chars with the
// sign bit set will be zero extended instead of sign extended.
// Not that EOF cannot be passed to these functions.
//
// These functions could also be used for optimizations if locale
// considerations make some of the CRT functions slow.
inline bool V_isalpha(char c) { return isalpha( (unsigned char)c ) != 0; }
inline bool V_isalnum(char c) { return isalnum( (unsigned char)c ) != 0; }
inline bool V_isprint(char c) { return isprint( (unsigned char)c ) != 0; }
inline bool V_isxdigit(char c) { return isxdigit( (unsigned char)c ) != 0; }
inline bool V_ispunct(char c) { return ispunct( (unsigned char)c ) != 0; }
inline bool V_isgraph(char c) { return isgraph( (unsigned char)c ) != 0; }
inline bool V_isupper(char c) { return isupper( (unsigned char)c ) != 0; }
inline bool V_islower(char c) { return islower( (unsigned char)c ) != 0; }
inline bool V_iscntrl(char c) { return iscntrl( (unsigned char)c ) != 0; }
inline bool V_isspace(char c) { return isspace( (unsigned char)c ) != 0; }
PLATFORM_INTERFACE bool	V_iswspace( wchar_t c );

// Short form remaps
#define V_memset(dest, fill, count)		V_tier0_memset		((dest), (fill), (count))
#define V_memcpy(dest, src, count)		V_tier0_memcpy		((dest), (src), (count))
#define V_memmove(dest, src, count)		V_tier0_memmove		((dest), (src), (count))
#define V_memcmp(m1, m2, count)			V_tier0_memcmp		((m1), (m2), (count))

#define V_strlen(str)					V_tier0_strlen		((str))
#define V_strlen16(str)					V_tier0_strlen16	((str))
#define V_strlen32(str)					V_tier0_strlen32	((str))
#define V_wcslen(str)					V_tier0_wcslen		((str))

#define V_strcpy(dest, src)				V_tier0_strcpy		((dest), (src))
#define V_strncpy(dest, src, count)		_V_strncpy			((dest), (src), (count))
#define V_strcpy32(dest, src)			V_tier0_strcpy32	((dest), (src))
#define V_strncpy32(dest, src, bytes)	_V_strncpy32_bytes	((dest), (src), (bytes))
#define V_wcscpy(dest, src)				V_tier0_wcscpy		((dest), (src))
#define V_wcsncpy(dest, src, bytes)		_V_wcsncpy_bytes	((dest), (src), (bytes))

#define V_strrchr(s, c)					V_tier0_strrchr		((s), (c))
#define V_wcschr(s, c)					V_tier0_wcschr		((s), (c))

#define V_strcmp(s1, s2)				V_tier0_strcmp		((s1), (s2))
#define V_strncmp(s1, s2, count)		_V_strncmp			((s1), (s2), (count))
#define V_wcscmp(s1, s2)				V_tier0_wcscmp		((s1), (s2))

#define V_stricmp(s1, s2)				V_stricmp_fast		((s1), (s2) )
#define V_stricmp_n(s1, s2)				V_stricmp_fast_NegativeForUnequal((s1), (s2) )
#define V_strnicmp(s1, s2, count)		_V_strnicmp_fast	((s1), (s2), (count))
#define V_wcsnicmp(s1, s2, symbols)		V_wcsnicmp_cch		((s1), (s2), (symbols))
#define V_strcasecmp(s1, s2)			V_stricmp			((s1), (s2))
#define V_strncasecmp(s1, s2, count)	V_strnicmp			((s1), (s2), (count))

#define V_strstr(s1, search)			V_tier0_strstr		((s1), (search))

#define V_strupper(start)				V_strupper_fast		((start))
#define V_strlower(start)				V_strlower_fast		((start))

#define V_stristr(s1, search)			V_stristr_fast		((s1), (search))
#define V_strnistr(s1, search, count)	_V_strnistr_fast	((s1), (search), (count))

#define V_strcspn(s1, s2)				_V_strcspn			((s1), (s2))

#define V_strncat(s1, s2, count)		_V_strncat			((s1), (s2), (count))

// Compares two strings with the support of wildcarding only for the first arg (includes '*' for multiple and '?' for single char usages)
PLATFORM_INTERFACE int V_CompareNameWithWildcards(const char *wildcarded_string, const char *compare_to, bool case_sensitive = false);

// Parses string equivalent of ("true", "false", "yes", "no", "1", "0") to the boolean value
// where default_value is what would be returned if parsing has failed
PLATFORM_INTERFACE bool V_StringToBool(const char *buf, bool default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a float array up to the amount of arr_size or up to the string limit, the amount of parsed values is returned
PLATFORM_INTERFACE int V_StringToFloatArray(const char *buf, float *out_arr, int arr_size, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into an int array up to the amount of arr_size or up to the string limit, the amount of parsed values is returned
PLATFORM_INTERFACE int V_StringToIntArray(const char *buf, int *out_arr, int arr_size, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a Vector structure
PLATFORM_INTERFACE void V_StringToVector(const char *buf, Vector &out_vec, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a Vector2D structure
PLATFORM_INTERFACE void V_StringToVector2D(const char *buf, Vector2D &out_vec, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a Vector4D structure
PLATFORM_INTERFACE void V_StringToVector4D(const char *buf, Vector4D &out_vec, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a Color structure
PLATFORM_INTERFACE void V_StringToColor(const char *buf, Color &out_clr, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a QAngle structure
PLATFORM_INTERFACE void V_StringToQAngle(const char *buf, QAngle &out_ang, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string into a Quaternion structure
PLATFORM_INTERFACE void V_StringToQuaternion(const char *buf, Quaternion &out_quat, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a uint64 value, where if the value exceeds min/max limits (inclusive), the parsing fails and default_value is returned
PLATFORM_INTERFACE uint64 V_StringToUint64Limit(const char *buf, uint64 min, uint64 max, uint64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a int64 value, where if the value exceeds min/max limits (inclusive), the parsing fails and default_value is returned
PLATFORM_INTERFACE int64 V_StringToInt64Limit(const char *buf, int64 min, int64 max, int64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a uint64 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE uint64 V_StringToUint64(const char *buf, uint64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a int64 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE int64 V_StringToInt64(const char *buf, int64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a uint32 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE uint32 V_StringToUint32(const char *buf, uint32 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a int32 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE int32 V_StringToInt32(const char *buf, int32 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a uint16 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE uint16 V_StringToUint16(const char *buf, uint16 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a int16 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE int16 V_StringToInt16(const char *buf, int16 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a uint8 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE uint8 V_StringToUint8(const char *buf, uint8 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a int8 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE int8 V_StringToInt8(const char *buf, int8 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a float64 value, where if the value exceeds min/max limits (inclusive), the parsing fails and default_value is returned
PLATFORM_INTERFACE float64 V_StringToFloat64Limit(const char *buf, float64 min, float64 max, float64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a float64 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE float64 V_StringToFloat64(const char *buf, float64 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a float32 value, if the parsing fails, default_value is returned
PLATFORM_INTERFACE float32 V_StringToFloat32(const char *buf, float32 default_value, bool *successful = NULL, char **remainder = NULL, uint flags = PARSING_FLAG_NONE, IParsingErrorListener *err_listener = NULL);

// Parses string as a float64 value, if the parsing fails, default_value is returned, doesn't perform error checking/reporting
PLATFORM_INTERFACE float64 V_StringToFloat64Raw(const char *buf, float64 default_value, bool *successful = NULL, char **remainder = NULL);

// Parses string as a float32 value, if the parsing fails, default_value is returned, doesn't perform error checking/reporting
PLATFORM_INTERFACE float32 V_StringToFloat32Raw(const char *buf, float32 default_value, bool *successful = NULL, char **remainder = NULL);

// Templatised and shortened version of the generic V_StringTo* functions
// these are silent, so you won't get error console warnings if parsing fails by default
template <typename T>
inline bool V_StringToValue( const char *string, T &value, uint flags = PARSING_FLAG_SKIP_ASSERT | PARSING_FLAG_SKIP_WARNING );

template <> inline bool V_StringToValue<bool>( const char *string, bool &value, uint flags )
{ bool success = false; value = V_StringToBool( string, false, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<int8>( const char *string, int8 &value, uint flags )
{ bool success = false; value = V_StringToInt8( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<uint8>( const char *string, uint8 &value, uint flags )
{ bool success = false; value = V_StringToUint8( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<int16>( const char *string, int16 &value, uint flags )
{ bool success = false; value = V_StringToInt16( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<uint16>( const char *string, uint16 &value, uint flags )
{ bool success = false; value = V_StringToUint16( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<int32>( const char *string, int32 &value, uint flags )
{ bool success = false; value = V_StringToInt32( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<uint32>( const char *string, uint32 &value, uint flags )
{ bool success = false; value = V_StringToUint32( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<int64>( const char *string, int64 &value, uint flags )
{ bool success = false; value = V_StringToInt64( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<uint64>( const char *string, uint64 &value, uint flags )
{ bool success = false; value = V_StringToUint64( string, 0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<float32>( const char *string, float32 &value, uint flags )
{ bool success = false; value = V_StringToFloat32( string, 0.0f, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<float64>( const char *string, float64 &value, uint flags )
{ bool success = false; value = V_StringToFloat64( string, 0.0, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<Vector>( const char *string, Vector &value, uint flags )
{ bool success = false; V_StringToVector( string, value, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<Vector2D>( const char *string, Vector2D &value, uint flags )
{ bool success = false; V_StringToVector2D( string, value, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<Vector4D>( const char *string, Vector4D &value, uint flags )
{ bool success = false; V_StringToVector4D( string, value, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<Color>( const char *string, Color &value, uint flags )
{ bool success = false; V_StringToColor( string, value, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<QAngle>( const char *string, QAngle &value, uint flags )
{ bool success = false; V_StringToQAngle( string, value, &success, nullptr, flags ); return success; }
template <> inline bool V_StringToValue<Quaternion>( const char *string, Quaternion &value, uint flags )
{ bool success = false; V_StringToQuaternion( string, value, &success, nullptr, flags ); return success; }

// returns string immediately following prefix, (ie str+strlen(prefix)) or NULL if prefix not found
PLATFORM_INTERFACE const char *_V_StringAfterPrefix( const char *str, const char *prefix );
PLATFORM_INTERFACE const char *_V_StringAfterPrefixCaseSensitive( const char *str, const char *prefix );

#define V_StringAfterPrefix(str, prefix)					_V_StringAfterPrefix((str), (prefix))
#define V_StringAfterPrefixCaseSensitive(str, prefix)		_V_StringAfterPrefixCaseSensitive((str), (prefix))

inline bool	V_StringHasPrefix             ( const char *str, const char *prefix ) { return V_StringAfterPrefix( str, prefix ) != NULL; }
inline bool	V_StringHasPrefixCaseSensitive( const char *str, const char *prefix ) { return V_StringAfterPrefixCaseSensitive( str, prefix ) != NULL; }

// Normalizes a float string in place.  
// (removes leading zeros, trailing zeros after the decimal point, and the decimal point itself where possible)
PLATFORM_INTERFACE void	V_normalizeFloatString( char* pFloat );
PLATFORM_INTERFACE void	V_normalizeFloatWString( wchar_t* pFloat );

// Prints out a pretified memory counter string value ( e.g., 7,233.27 Mb, 1,298.003 Kb, 127 bytes )
PLATFORM_INTERFACE char *V_PrettifyMem( float value, int digitsafterdecimal = 2, bool usebinaryonek = false );

// Prints out a pretified integer with comma separators (eg, 7,233,270,000)
PLATFORM_INTERFACE char *V_PrettifyNum( int64 value );

// Returns the UTF8 encoded length in this byte
PLATFORM_INTERFACE int V_UTF8LenFromFirst( char c );

// Conversion functions, returning the number of bytes consumed
// Decode a single UTF-8 character to a uchar32, returns number of UTF-8 bytes parsed
PLATFORM_INTERFACE int V_UTF8ToUChar32( const char *str, uchar32 &result, bool &failed );
PLATFORM_INTERFACE int V_UTF32ToUChar32( const uchar32 *str, uchar32 &result, bool &failed );

// Decode a single UTF-16 character to a uchar32, returns number of UTF-16 characters (NOT BYTES) consumed
PLATFORM_INTERFACE int V_UTF16ToUChar32( const uchar16 *str, uchar32 &result, bool &failed );

PLATFORM_INTERFACE int V_UChar32ToUTF16( const uchar32 *str, uchar16 *result );
PLATFORM_INTERFACE int V_UChar32ToUTF8( const uchar32 *str, char *result );

// Conversion between Unicode string types (UTF-8, UTF-16, UTF-32). Deals with bytes, not element counts,
// to minimize harm from the programmer mistakes which continue to plague our wide-character string code.
// Returns the number of bytes written to the output, or if output is NULL, the number of bytes required.
PLATFORM_INTERFACE int V_UTF8ToUTF16( const char *str, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF8ToUTF32( const char *str, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF16ToUTF8( const uchar16 *str, OUT_Z_BYTECAP( dest_size ) char *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF16ToUTF16( const uchar16 *str, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF16ToUTF32( const uchar16 *str, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF32ToUTF8( const uchar32 *str, OUT_Z_BYTECAP( dest_size ) char *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF32ToUTF16( const uchar32 *str, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );

// This is disgusting and exist only easily to facilitate having 16-bit and 32-bit wchar_t's on different platforms
PLATFORM_INTERFACE int V_UTF32ToUTF32( const uchar32 *str, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );

// Conversion between count-limited UTF-n character arrays, including any potential NULL characters.
// Output has a terminating NULL for safety; strip the last character if you want an unterminated string.
// Returns the number of bytes written to the output, or if output is NULL, the number of bytes required.
PLATFORM_INTERFACE int V_UTF8CharsToUTF16( const char *str, int size, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF8CharsToUTF32( const char *str, int size, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF16CharsToUTF8( const uchar16 *str, int size, OUT_Z_BYTECAP( dest_size ) char *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF16CharsToUTF32( const uchar16 *str, int size, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF32CharsToUTF8( const uchar32 *str, int size, OUT_Z_BYTECAP( dest_size ) char *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );
PLATFORM_INTERFACE int V_UTF32CharsToUTF16( const uchar32 *str, int size, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, EStringConvertErrorPolicy policy = STRINGCONVERT_ASSERT_REPLACE );

// NOTE: WString means either UTF32 or UTF16 depending on the platform and compiler settings.
#if defined( _MSC_VER ) || defined( _WIN32 )
#define V_UTF8ToWString V_UTF8ToUTF16
#define V_UTF8CharsToWString V_UTF8CharsToUTF16
#define V_UTF32ToWString V_UTF32ToUTF16
#define V_WStringToUTF8 V_UTF16ToUTF8
#define V_WStringCharsToUTF8 V_UTF16CharsToUTF8
#define V_WStringToUTF32 V_UTF16ToUTF32
#else
#define V_UTF8ToWString V_UTF8ToUTF32
#define V_UTF8CharsToWString V_UTF8CharsToUTF32
#define V_UTF32ToWString V_UTF32ToUTF32
#define V_WStringToUTF8 V_UTF32ToUTF8
#define V_WStringCharsToUTF8 V_UTF32CharsToUTF8
#define V_WStringToUTF32 V_UTF32ToUTF32
#endif

PLATFORM_OVERLOAD int V_UnicodeCaseCollate( const char *s1, const char *s2, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

// Validate that a Unicode string is well-formed and contains only valid code points
PLATFORM_OVERLOAD bool V_UnicodeValidate( const char *pUTF8 );
PLATFORM_OVERLOAD bool V_UnicodeValidate( const uchar16 *pUTF16 );
PLATFORM_OVERLOAD bool V_UnicodeValidate( const uchar32 *pUTF32 );

// Returns length of string in Unicode code points (printed glyphs or non-printing characters)
PLATFORM_OVERLOAD int V_UnicodeLength( const char *pUTF8 );
PLATFORM_OVERLOAD int V_UnicodeLength( const uchar16 *pUTF16 );
PLATFORM_OVERLOAD int V_UnicodeLength( const uchar32 *pUTF32 );

PLATFORM_OVERLOAD int V_UnicodeBufferLength( char const *pUTF8, int size, bool &success );
PLATFORM_OVERLOAD int V_UnicodeBufferLength( const uchar16 *pUTF32, int size, bool &success );
PLATFORM_OVERLOAD int V_UnicodeBufferLength( const uchar32 *pUTF16, int size, bool &success );

// Repair invalid Unicode strings by dropping truncated characters and fixing improperly-double-encoded UTF-16 sequences.
// Unlike conversion functions which replace with '?' by default, a repair operation assumes that you know that something
// is wrong with the string (eg, mid-sequence truncation) and you just want to do the best possible job of cleaning it up.
// You can pass a REPLACE or FAIL policy if you would prefer to replace characters with '?' or clear the entire string.
// Returns nonzero on success, or 0 if the policy is FAIL and an invalid sequence was found.
PLATFORM_OVERLOAD int V_UnicodeRepair( char *pUTF8, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeRepair( uchar16 *pUTF16, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeRepair( uchar32 *pUTF32, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

// Advance pointer forward by N Unicode code points (printed glyphs or non-printing characters), stopping at terminating null if encountered.
PLATFORM_OVERLOAD char *V_UnicodeAdvance( char *pUTF8, int nCharacters );
PLATFORM_OVERLOAD uchar16 *V_UnicodeAdvance( uchar16 *pUTF16, int nCharactersnCharacters );
PLATFORM_OVERLOAD uchar32 *V_UnicodeAdvance( uchar32 *pUTF32, int nChars );
inline const char *V_UnicodeAdvance( const char *pUTF8, int nCharacters ) { return V_UnicodeAdvance( (char *)pUTF8, nCharacters ); }
inline const uchar16 *V_UnicodeAdvance( const uchar16 *pUTF16, int nCharacters ) { return V_UnicodeAdvance( (uchar16 *)pUTF16, nCharacters ); }
inline const uchar32 *V_UnicodeAdvance( const uchar32 *pUTF32, int nCharacters ) { return V_UnicodeAdvance( (uchar32 *)pUTF32, nCharacters ); }

// Truncate to maximum of N Unicode code points (printed glyphs or non-printing characters)
inline void V_UnicodeTruncate( char *pUTF8, int nCharacters ) { *V_UnicodeAdvance( pUTF8, nCharacters ) = 0; }
inline void V_UnicodeTruncate( uchar16 *pUTF16, int nCharacters ) { *V_UnicodeAdvance( pUTF16, nCharacters ) = 0; }
inline void V_UnicodeTruncate( uchar32 *pUTF32, int nCharacters ) { *V_UnicodeAdvance( pUTF32, nCharacters ) = 0; }

PLATFORM_OVERLOAD int V_UnicodeCaseCompare( const char *s1, const char *s2, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeCaseCompare( const uchar16 *s1, const uchar16 *s2, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeCaseCompare( const uchar32 *s1, const uchar32 *s2, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

PLATFORM_OVERLOAD int V_UnicodeCaseConvert( const char *pUTF8, OUT_Z_BYTECAP( dest_size ) char *dest, int dest_size, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeCaseConvert( const uchar16 *pUTF16, OUT_Z_BYTECAP( dest_size ) uchar16 *dest, int dest_size, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD int V_UnicodeCaseConvert( const uchar32 *pUTF32, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

PLATFORM_OVERLOAD bool V_UnicodeCaseStringInString( const char *pUTF8, const char *substr, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD bool V_UnicodeCaseStringInString( const uchar16 *pUTF16, const uchar16 *substr, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD bool V_UnicodeCaseStringInString( const uchar32 *pUTF32, const uchar32 *substr, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

PLATFORM_OVERLOAD uchar32 *V_UnicodeConvertAndCaseStr( const char *pUTF8, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, uchar32 **new_buf, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD uchar32 *V_UnicodeConvertAndCaseStr( const uchar16 *pUTF16, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, uchar32 **new_buf, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );
PLATFORM_OVERLOAD uchar32 *V_UnicodeConvertAndCaseStr( const uchar32 *pUTF32, OUT_Z_BYTECAP( dest_size ) uchar32 *dest, int dest_size, uchar32 **new_buf, int flags, EStringConvertErrorPolicy ePolicy = STRINGCONVERT_SKIP );

// Functions for converting hexidecimal character strings back into binary data etc.
//
// e.g., 
// int output;
// V_hextobinary( "ffffffff", 8, &output, sizeof( output ) );
// would make output == 0xfffffff or -1
// Similarly,
// char buffer[ 9 ];
// V_binarytohex( &output, sizeof( output ), buffer, sizeof( buffer ) );
// would put "ffffffff" into buffer (note null terminator!!!)
PLATFORM_INTERFACE void _V_hextobinary( char const *in, int numchars, byte *out, int maxoutputbytes );
PLATFORM_INTERFACE void _V_binarytohex( const byte *in, int inputbytes, char *out, int outsize );

#define V_HexToBinary(in, numchars, out, maxoutputbytes)	_V_hextobinary((in), (numchars), (out), (maxoutputbytes))
#define V_BinaryToHex(in, inputbytes, out, outsize)			_V_binarytohex((in), (inputbytes), (out), (outsize))

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define CORRECT_PATH_SEPARATOR_S "\\"
#define INCORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR_S "/"
#elif POSIX
#define CORRECT_PATH_SEPARATOR '/'
#define CORRECT_PATH_SEPARATOR_S "/"
#define INCORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR_S "\\"
#endif

// Tools for working with filenames
// Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
PLATFORM_INTERFACE void _V_FileBase( const char *in, char *out, int maxlen );
#define V_FileBase _V_FileBase

// Remove the final characters of ppath if it's '\' or '/'.
PLATFORM_INTERFACE void V_StripTrailingSlash( char *ppath );

// Remove the final characters of ppline if they are whitespace (uses V_isspace)
PLATFORM_INTERFACE void V_StripTrailingWhitespace( char *ppline );

// Remove the initial characters of ppline if they are whitespace (uses V_isspace)
PLATFORM_INTERFACE void V_StripLeadingWhitespace( char *ppline );

// Remove the initial/final characters of ppline if they are " quotes
PLATFORM_INTERFACE void V_StripSurroundingQuotes( char *ppline );

// Remove any extension from in and return resulting string in out
PLATFORM_INTERFACE void _V_StripExtension( const char *in, char *out, int outLen );
#define V_StripExtension _V_StripExtension

// Make path end with extension if it doesn't already have an extension
PLATFORM_INTERFACE void _V_DefaultExtension( char *path, const char *extension, int pathStringLength );
#define V_DefaultExtension _V_DefaultExtension

// Strips any current extension from path and ensures that extension is the new extension.
// NOTE: extension string MUST include the . character
PLATFORM_INTERFACE void _V_SetExtension( char *path, const char *extension, int pathStringLength );
#define V_SetExtension _V_SetExtension

// Removes any filename from path ( strips back to previous / or \ character )
PLATFORM_INTERFACE void V_StripFilename( char *path );

// Remove the final directory from the path
PLATFORM_INTERFACE bool _V_StripLastDir( char *dirName, int maxlen );
#define V_StripLastDir _V_StripLastDir

// Returns a pointer to the unqualified file name (no path) of a file name
PLATFORM_INTERFACE const char *V_UnqualifiedFileName( const char *in );

// Given a path and a filename, composes "path\filename", inserting the (OS correct) separator if necessary
PLATFORM_INTERFACE void _V_ComposeFileName( const char *path, const char *filename, char *dest, int destSize );
#define V_ComposeFileName _V_ComposeFileName

// Copy out the path except for the stuff after the final pathseparator
PLATFORM_INTERFACE bool _V_ExtractFilePath( const char *path, char *dest, int destSize );
#define V_ExtractFilePath _V_ExtractFilePath

// Copy out the file extension into dest
PLATFORM_INTERFACE void _V_ExtractFileExtension( const char *path, char *dest, int destSize );
#define V_ExtractFileExtension _V_ExtractFileExtension

PLATFORM_INTERFACE const char *V_GetFileExtension( const char *path );
// Returns empty string instead of null on failure
PLATFORM_INTERFACE const char *V_GetFileExtensionSafe( const char *path );

// This removes "./" and "../" from the pathname. pFilename should be a full pathname.
// Returns false if it tries to ".." past the root directory in the drive (in which case 
// it is an invalid path).
PLATFORM_INTERFACE bool V_RemoveDotSlashes( char *pFilename, char separator = CORRECT_PATH_SEPARATOR );

// If pPath is a relative path, this function makes it into an absolute path
// using the current working directory as the base, or pStartingDir if it's non-NULL.
// Returns false if it runs out of room in the string, or if pPath tries to ".." past the root directory.
PLATFORM_INTERFACE void _V_MakeAbsolutePath( OUT_Z_CAP( outLen ) char *pOut, int outLen, const char *pPath, const char *pStartingDir = NULL );
PLATFORM_INTERFACE bool V_MakeAbsolutePathBuffer( CBufferString &buf, const char *pPath, const char *pStartingDir = NULL );
#define V_MakeAbsolutePath _V_MakeAbsolutePath

PLATFORM_INTERFACE void V_RemoveFormatSpecifications( const char *pszFrom, char *pszTo, size_t sizeDest );

// Creates a relative path given two full paths
// The first is the full path of the file to make a relative path for.
// The second is the full path of the directory to make the first file relative to
// Returns false if they can't be made relative (on separate drives, for example)
PLATFORM_INTERFACE bool _V_MakeRelativePath( const char *pFullPath, const char *pDirectory, OUT_Z_CAP( nBufLen ) char *pRelativePath, int nBufLen, bool );
PLATFORM_INTERFACE bool V_MakeRelativePathBuffer( const char *pFullPath, const char *pDirectory, CBufferString &buf, bool );
#define V_MakeRelativePath _V_MakeRelativePath

// Fixes up a file name, removing dot slashes, fixing slashes, converting to lowercase, etc.
PLATFORM_INTERFACE void _V_FixupPathName( OUT_Z_CAP( nOutLen ) char *pOut, size_t nOutLen, const char *pPath, bool convert_to_lower = true );
#define V_FixupPathName _V_FixupPathName

// Adds a path separator to the end of the string if there isn't one already. Returns false if it would run out of space.
PLATFORM_INTERFACE void _V_AppendSlash( INOUT_Z_CAP( strSize ) char *pStr, int strSize, char separator = CORRECT_PATH_SEPARATOR );
#define V_AppendSlash _V_AppendSlash

// Returns true if the path is an absolute path.
PLATFORM_INTERFACE bool V_IsAbsolutePath( const char *pPath );

// Scans pIn and replaces all occurences of pMatch with pReplaceWith.
// Writes the result to pOut.
// Returns true if it completed successfully.
// If it would overflow pOut, it fills as much as it can and returns false.
PLATFORM_INTERFACE bool _V_StrSubst( const char *pIn, const char *pMatch, const char *pReplaceWith,
									 OUT_Z_CAP( outLen ) char *pOut, int outLen, bool bCaseSensitive=false );
#define V_StrSubst _V_StrSubst

// AM TODO: If possible, use CSplitString instead rn. 
// These are exported by tier0, but will require changes to CUtlVector (additional template arg)
// 
// Split the specified string on the specified separator.
// Returns a list of strings separated by pSeparator.
// You are responsible for freeing the contents of outStrings (call outStrings.PurgeAndDeleteElements).
PLATFORM_OVERLOAD void V_SplitString( const char *pString, const char *pSeparator, CUtlVector<CUtlString, CUtlMemory<CUtlString, int>> &outStrings, bool include_empty = false );

// Just like V_SplitString, but it can use multiple possible separators.
PLATFORM_OVERLOAD void V_SplitStringInPlace( char *pString, const char *pSeparator, CUtlVector<const char *, CUtlMemory<const char *, int>> &outStrings );

// This function takes a slice out of pStr and stores it in pOut.
// It follows the Python slice convention:
// Negative numbers wrap around the string (-1 references the last character).
// Large numbers are clamped to the end of the string.
PLATFORM_INTERFACE void _V_StrSlice( const char *pStr, int firstChar, int lastCharNonInclusive, char *pOut, int outSize );
#define V_StrSlice _V_StrSlice

// Chop off the left nChars of a string.
PLATFORM_INTERFACE void _V_StrLeft( const char *pStr, int nChars, char *pOut, int outSize );
#define V_StrLeft _V_StrLeft

// Chop off the right nChars of a string.
PLATFORM_INTERFACE void _V_StrRight( const char *pStr, int nChars, char *pOut, int outSize );
#define V_StrRight _V_StrRight

// change "special" characters to have their c-style backslash sequence. like \n, \r, \t, ", etc.
// returns a pointer to a newly allocated string, which you must delete[] when finished with.
PLATFORM_INTERFACE char *V_AddBackSlashesToSpecialChars( char const *pSrc );

// Force slashes of either type to be = separator character
PLATFORM_INTERFACE void V_FixSlashes( char *pname, char separator = CORRECT_PATH_SEPARATOR );

// This function fixes cases of filenames like materials\\blah.vmt or somepath\otherpath\\ and removes the extra double slash.
PLATFORM_INTERFACE void V_FixDoubleSlashes( char *pStr );

// Convert \r\n (Windows linefeeds) to \n (Unix linefeeds).
PLATFORM_INTERFACE void V_TranslateLineFeedsToUnix( char *pStr );

// Encode a string for display as HTML -- this only encodes ' " & < >, which are the important ones to encode for 
// security and ensuring HTML display doesn't break.  Other special chars like the ? sign and so forth will not
// be encoded
//
// Returns false if there was not enough room in pDest to encode the entire source string, otherwise true
PLATFORM_INTERFACE bool V_BasicHtmlEntityEncode( OUT_Z_CAP( nDestSize ) char *pDest, const int nDestSize, char const *pIn, const int nInSize, bool bPreserveWhitespace = false );


// Decode a string with htmlentities HTML -- this should handle all special chars, not just the ones V_BasicHtmlEntityEncode uses.
//
// Returns false if there was not enough room in pDest to decode the entire source string, otherwise true
PLATFORM_INTERFACE bool V_HtmlEntityDecodeToUTF8( OUT_Z_CAP( nDestSize ) char *pDest, const int nDestSize, char const *pIn, const int nInSize );

// strips HTML from a string.  Should call Q_HTMLEntityDecodeToUTF8 afterward.
PLATFORM_INTERFACE void V_StripAndPreserveHTML( CBufferString *pbuffer, const char *pchHTML, const char **rgszPreserveTags, uint cPreserveTags, uint cMaxResultSize );
PLATFORM_INTERFACE void V_StripAndPreserveHTMLCore( CBufferString *pbuffer, const char *pchHTML, const char **rgszPreserveTags, uint cPreserveTags, const char **rgszNoCloseTags, uint cNoCloseTags, uint cMaxResultSize );

// Extracts the domain from a URL
PLATFORM_INTERFACE bool V_ExtractDomainFromURL( const char *pchURL, OUT_Z_CAP( cchDomain ) char *pchDomain, int cchDomain );

// returns true if the url passed in is on the specified domain
PLATFORM_INTERFACE bool V_URLContainsDomain( const char *pchURL, const char *pchDomain );

//-----------------------------------------------------------------------------
// returns true if the character is allowed in a URL, false otherwise
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool V_IsValidURLCharacter( const char *pch, int *pAdvanceBytes );

//-----------------------------------------------------------------------------
// returns true if the character is allowed in a DNS doman name, false otherwise
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool V_IsValidDomainNameCharacter( const char *pch, int *pAdvanceBytes );

 // Converts BBCode tags to HTML tags
PLATFORM_INTERFACE bool V_BBCodeToHTML( OUT_Z_CAP( nDestSize ) char *pDest, const int nDestSize, char const *pIn, const int nInSize );


// helper to identify "mean" spaces, which we don't like in visible identifiers
// such as player Name
PLATFORM_INTERFACE bool V_IsMeanSpaceW( wchar_t wch );

// helper to identify characters which are deprecated in Unicode,
// and we simply don't accept
PLATFORM_INTERFACE bool V_IsDeprecatedW( wchar_t wch );

//-----------------------------------------------------------------------------
// generic unique name helper functions
//-----------------------------------------------------------------------------

// returns -1 if no match, nDefault if pName==prefix, and N if pName==prefix+N
PLATFORM_INTERFACE int V_IndexAfterPrefix( const char *pName, const char *prefix, int nDefault = 0 );

// returns startindex if none found, 2 if "prefix" found, and n+1 if "prefixn" found
template < class NameArray >
int V_GenerateUniqueNameIndex( const char *prefix, const NameArray &nameArray, int startindex = 0 )
{
	if ( !prefix )
		return 0;

	int freeindex = startindex;

	int nNames = nameArray.Count();
	for ( int i = 0; i < nNames; ++i )
	{
		int index = V_IndexAfterPrefix( nameArray[ i ], prefix, 1 ); // returns -1 if no match, 0 for exact match, N for 
		if ( index >= freeindex )
		{
			// TODO - check that there isn't more junk after the index in pElementName
			freeindex = index + 1;
		}
	}

	return freeindex;
}

template < class NameArray >
bool V_GenerateUniqueName( char *name, int memsize, const char *prefix, const NameArray &nameArray )
{
	if ( name == NULL || memsize == 0 )
		return false;

	if ( prefix == NULL )
	{
		name[ 0 ] = '\0';
		return false;
	}

	int prefixLength = V_strlen( prefix );
	if ( prefixLength + 1 > memsize )
	{
		name[ 0 ] = '\0';
		return false;
	}

	int i = V_GenerateUniqueNameIndex( prefix, nameArray );
	if ( i <= 0 )
	{
		V_strncpy( name, prefix, memsize );
		return true;
	}

	int newlen = prefixLength + ( int )log10( ( float )i ) + 1;
	if ( newlen + 1 > memsize )
	{
		V_strncpy( name, prefix, memsize );
		return false;
	}

	V_snprintf( name, memsize, "%s%d", prefix, i );
	return true;
}

//
// This utility class is for performing UTF-8 <-> UTF-16 conversion.
// It is intended for use with function/method parameters.
//
// For example, you can call
//     FunctionTakingUTF16( CStrAutoEncode( utf8_string ).ToUChar16() )
// or
//     FunctionTakingUTF8( CStrAutoEncode( utf16_string ).ToString() )
//
// The converted string is allocated off the heap, and destroyed when
// the object goes out of scope.
//
// if the string cannot be converted, NULL is returned.
//
// This class doesn't have any conversion operators; the intention is
// to encourage the developer to get used to having to think about which
// encoding is desired.
//
class CStrAutoEncode
{
public:
	explicit CStrAutoEncode( const char *buf )
	{
		InitEmpty();

		m_pch = buf;
		m_bHasUTF8 = true;
	}

	explicit CStrAutoEncode( const uchar16 *buf )
	{
		InitEmpty();

		m_char16 = buf;
		m_bHasUTF16 = true;
	}

	explicit CStrAutoEncode( const uchar32 *buf )
	{
		InitEmpty();

		m_char32 = buf;
		m_bHasUTF32 = true;
	}

	CStrAutoEncode( const CStrAutoEncode &other )
	{
		Copy( other );
	}

	// returns the UTF-8 string, converting on the fly.
	const char *ToUTF8() { return ToString(); }
	const char* ToString()
	{
		PopulateUTF8();
		return m_pch;
	}

	// returns the UTF-16 string, converting on the fly.
	const uchar16 *ToUTF16() { return ToUChar16(); }
	const uchar16 *ToUChar16()
	{
		PopulateUTF16();
		return m_char16;
	}

	// returns the UTF-32 string, converting on the fly.
	const uchar32 *ToUTF32() { return ToUChar32(); }
	const uchar32 *ToUChar32()
	{
		PopulateUTF32();
		return m_char32;
	}

	~CStrAutoEncode()
	{
		Clear();
	}

	PLATFORM_CLASS void Clear();
	PLATFORM_CLASS CStrAutoEncode &Copy( const CStrAutoEncode &other );

	// Creates a copy and stores it
	PLATFORM_CLASS void SetCopy( const char *buf, int nCount = -1 );
	PLATFORM_CLASS void SetCopy( const uchar16 *buf, int nCount = -1 );
	PLATFORM_CLASS void SetCopy( const uchar32 *buf, int nCount = -1 );

	// Creates a copy and coverts to UTF8 then stores it
	PLATFORM_CLASS void SetUTF8Copy( const uchar32 *buf );
	PLATFORM_CLASS void SetUTF8Copy( const uchar16 *buf );

	// Creates a copy and coverts to UTF16 then stores it
	PLATFORM_CLASS void SetUTF16Copy( const char *buf );
	PLATFORM_CLASS void SetUTF16Copy( const uchar32 *buf );

	// Creates a copy and coverts to UTF32 then stores it
	PLATFORM_CLASS void SetUTF32Copy( const char *buf );
	PLATFORM_CLASS void SetUTF32Copy( const uchar16 *buf );

private:
	PLATFORM_CLASS void InitEmpty();

	PLATFORM_CLASS void PopulateUTF8() const;
	PLATFORM_CLASS void PopulateUTF16() const;
	PLATFORM_CLASS void PopulateUTF32() const;

	// one of these pointers is an owned pointer; whichever
	// one is the encoding OTHER than the one we were initialized
	// with is the pointer we've allocated and must free.
	const char *m_pch;
	const uchar16 *m_char16;
	const uchar32 *m_char32;

	bool m_bHasUTF8 : 1;
	bool m_bOwnUTF8 : 1;

	bool m_bHasUTF16 : 1;
	bool m_bOwnUTF16 : 1;

	bool m_bHasUTF32 : 1;
	bool m_bOwnUTF32 : 1;

	bool m_bOriginal : 1;
};

// trim right whitespace
PLATFORM_INTERFACE char *TrimRight( char *pString );

PLATFORM_INTERFACE const char *SkipBlanks( const char *pString );

// Encodes a string (or binary data) in URL encoding format, see rfc1738 section 2.2.
// Dest buffer should be 3 times the size of source buffer to guarantee it has room to encode.
PLATFORM_INTERFACE void V_URLEncodeRaw( OUT_Z_CAP(nDestLen) char *pchDest, int nDestLen, const char *pchSource, int nSourceLen );

// Decodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.
// Dest buffer should be at least as large as source buffer to gurantee room for decode.
// Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//
// Returns the amount of space actually used in the output buffer.  
PLATFORM_INTERFACE size_t V_URLDecodeRaw( OUT_CAP(nDecodeDestLen) char *pchDecodeDest, int nDecodeDestLen, const char *pchEncodedSource, int nEncodedSourceLen );

// Encodes a string (or binary data) in URL encoding format, this isn't the strict rfc1738 format, but instead uses + for spaces.  
// This is for historical reasons and HTML spec foolishness that lead to + becoming a de facto standard for spaces when encoding form data.
// Dest buffer should be 3 times the size of source buffer to guarantee it has room to encode.
PLATFORM_INTERFACE void V_URLEncode( OUT_Z_CAP(nDestLen) char *pchDest, int nDestLen, const char *pchSource, int nSourceLen );

// Decodes a string (or binary data) in URL encoding format, this isn't the strict rfc1738 format, but instead uses + for spaces.  
// This is for historical reasons and HTML spec foolishness that lead to + becoming a de facto standard for spaces when encoding form data.
// Dest buffer should be at least as large as source buffer to gurantee room for decode.
// Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//
// Returns the amount of space actually used in the output buffer.  
PLATFORM_INTERFACE size_t V_URLDecode( OUT_CAP(nDecodeDestLen) char *pchDecodeDest, int nDecodeDestLen, const char *pchEncodedSource, int nEncodedSourceLen );

// 3d memcpy. Copy (up-to) 3 dimensional data with arbitrary source and destination
// strides. Optimizes to just a single memcpy when possible. For 2d data, set numslices to 1.
PLATFORM_INTERFACE void V_CopyMemory3D(
	void *pDestAdr, void const *pSrcAdr,
	int nNumCols, int nNumRows, int nNumSlices, // dimensions of copy
	int nSrcBytesPerRow, int nSrcBytesPerSlice, // strides for source.
	int nDestBytesPerRow, int nDestBytesPerSlice // strides for dest
);

// NOTE: This is for backward compatability!
// We need to DLL-export the Q methods in vstdlib but not link to them in other projects
#if !defined( VSTDLIB_BACKWARD_COMPAT )

#define Q_memset				V_memset
#define Q_memcpy				V_memcpy
#define Q_memmove				V_memmove
#define Q_memcmp				V_memcmp
#define Q_strlen				V_strlen
#define Q_strcpy				V_strcpy
#define Q_strrchr				V_strrchr
#define Q_strcmp				V_strcmp
#define Q_wcscmp				V_wcscmp
#define Q_stricmp				V_stricmp
#define Q_strstr				V_strstr
#define Q_strupr				V_strupper
#define Q_strlower				V_strlower
#define Q_wcslen				V_wcslen
#define	Q_strncmp				V_strncmp 
#define	Q_strcasecmp			V_stricmp
#define	Q_strncasecmp			V_strnicmp
#define	Q_strnicmp				V_strnicmp
#define	Q_atoi					V_atoi
#define	Q_atoi64				V_atoi64
#define Q_atoui64				V_atoui64
#define	Q_atof					V_atof
#define	Q_stristr				V_stristr
#define	Q_strnistr				V_strnistr
#define	Q_strnchr				V_strnchr
#define Q_normalizeFloatString	V_normalizeFloatString
#define Q_strncpy				V_strncpy
#define Q_wcsncpy				V_wcsncpy
#define Q_snprintf				V_snprintf
#define Q_snwprintf				V_snwprintf_bytes
#define Q_wcsncpy				V_wcsncpy
#define Q_strncat				V_strncat
#define Q_vsnprintf				V_vsnprintf
#define Q_pretifymem			V_PrettifyMem
#define Q_pretifynum			V_PrettifyNum
#define Q_hextobinary			V_HexToBinary
#define Q_binarytohex			V_BinaryToHex
#define Q_FileBase				V_FileBase
#define Q_StripTrailingSlash	V_StripTrailingSlash
#define Q_StripExtension		V_StripExtension
#define	Q_DefaultExtension		V_DefaultExtension
#define Q_SetExtension			V_SetExtension
#define Q_StripFilename			V_StripFilename
#define Q_StripLastDir			V_StripLastDir
#define Q_UnqualifiedFileName	V_UnqualifiedFileName
#define Q_ComposeFileName		V_ComposeFileName
#define Q_ExtractFilePath		V_ExtractFilePath
#define Q_ExtractFileExtension	V_ExtractFileExtension
#define Q_GetFileExtension		V_GetFileExtension
#define Q_RemoveDotSlashes		V_RemoveDotSlashes
#define Q_MakeAbsolutePath		V_MakeAbsolutePath
#define Q_AppendSlash			V_AppendSlash
#define Q_IsAbsolutePath		V_IsAbsolutePath
#define Q_StrSubst				V_StrSubst
#define Q_SplitString			V_SplitString
#define Q_StrSlice				V_StrSlice
#define Q_StrLeft				V_StrLeft
#define Q_StrRight				V_StrRight
#define Q_FixSlashes			V_FixSlashes
#define Q_strcat				V_strcat
#define Q_MakeRelativePath		V_MakeRelativePath
#define Q_FixupPathName			V_FixupPathName

#endif // !defined( VSTDLIB_DLL_EXPORT )

#endif	// TIER1_STRTOOLS_H
