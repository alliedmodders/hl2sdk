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

// 3d memcpy. Copy (up-to) 3 dimensional data with arbitrary source and destination
// strides. Optimizes to just a single memcpy when possible. For 2d data, set numslices to 1.
void CopyMemory3D( void *pDestAdr, void const *pSrcAdr,		
				   int nNumCols, int nNumRows, int nNumSlices, // dimensions of copy
				   int nSrcBytesPerRow, int nSrcBytesPerSlice, // strides for source.
				   int nDestBytesPerRow, int nDestBytesPerSlice // strides for dest
	);


template< class T, class I > class CUtlMemory;
template< class T, class A > class CUtlVector;

// Unicode string conversion policies - what to do if an illegal sequence is encountered
enum EStringConvertErrorPolicy
{
	_STRINGCONVERTFLAG_SKIP =		1,
	_STRINGCONVERTFLAG_FAIL =		2,
	_STRINGCONVERTFLAG_ASSERT =		4,

	STRINGCONVERT_REPLACE =			0,
	STRINGCONVERT_SKIP =			_STRINGCONVERTFLAG_SKIP,
	STRINGCONVERT_FAIL =			_STRINGCONVERTFLAG_FAIL,

	STRINGCONVERT_ASSERT_REPLACE =	_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_REPLACE,
	STRINGCONVERT_ASSERT_SKIP =		_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_SKIP,
	STRINGCONVERT_ASSERT_FAIL =		_STRINGCONVERTFLAG_ASSERT + STRINGCONVERT_FAIL,
};

//-----------------------------------------------------------------------------
// Portable versions of standard string functions
//-----------------------------------------------------------------------------
void	_V_memset	( void *dest, int fill, int count );
void	_V_memcpy	( void *dest, const void *src, int count );
void	_V_memmove	( void *dest, const void *src, int count );
int		_V_memcmp	( const void *m1, const void *m2, int count );
int		_V_strlen	( const char *str );
void	_V_strcpy	( char *dest, const char *src );
char*	_V_strrchr	( const char *s, char c );
int		_V_strcmp	( const char *s1, const char *s2 );
int		_V_wcscmp	( const wchar_t *s1, const wchar_t *s2 );
int		_V_stricmp	( const char *s1, const char *s2 );
char*	_V_strstr	( const char *s1, const char *search );
char*	_V_strupr	( char *start );
char*	_V_strlower	( char *start );
int		_V_wcslen	( const wchar_t *pwch );

#ifdef POSIX
inline char *strupr( char *start )
{
      char *str = start;
      while( str && *str )
      {
              *str = (char)toupper(*str);
              str++;
      }
      return start;
}

inline char *strlwr( char *start )
{
      char *str = start;
      while( str && *str )
      {
              *str = (char)tolower(*str);
              str++;
      }
      return start;
}

#endif // POSIX

// there are some users of these via tier1 templates in used in tier0. but tier0 can't depend on vstdlib which means in tier0 we always need the inlined ones
#if ( !defined( TIER0_DLL_EXPORT ) )

#define V_memset(dest, fill, count)		_V_memset   ((dest), (fill), (count))	
#define V_memcpy(dest, src, count)		_V_memcpy	((dest), (src), (count))	
#define V_memmove(dest, src, count)		_V_memmove	((dest), (src), (count))	
#define V_memcmp(m1, m2, count)			_V_memcmp	((m1), (m2), (count))		
#define V_strlen(str)					_V_strlen	((str))				
#define V_strcpy(dest, src)				_V_strcpy	((dest), (src))			
#define V_strrchr(s, c)					_V_strrchr	((s), (c))				
#define V_strcmp(s1, s2)				_V_strcmp	((s1), (s2))			
#define V_wcscmp(s1, s2)				_V_wcscmp	((s1), (s2))			
#define V_stricmp(s1, s2 )				_V_stricmp	((s1), (s2) )			
#define V_strstr(s1, search )			_V_strstr	((s1), (search) )		
#define V_strupr(start)					_V_strupr	((start))				
#define V_strlower(start)				_V_strlower ((start))		
#define V_wcslen(pwch)					_V_wcslen	((pwch))		

// AM TODO: handle this for the rest (above and more) now exported by tier0
PLATFORM_INTERFACE int V_stricmp_fast(const char* s1, const char* s2);

PLATFORM_INTERFACE int _V_strnicmp_fast(const char* s1, const char* s2, int n);

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

#else

inline void		V_memset (void *dest, int fill, int count)			{ memset( dest, fill, count ); }
inline void		V_memcpy (void *dest, const void *src, int count)	{ memcpy( dest, src, count ); }
inline void		V_memmove (void *dest, const void *src, int count)	{ memmove( dest, src, count ); }
inline int		V_memcmp (const void *m1, const void *m2, int count){ return memcmp( m1, m2, count ); } 
inline int		V_strlen (const char *str)							{ return (int) strlen ( str ); }
inline void		V_strcpy (char *dest, const char *src)				{ strcpy( dest, src ); }
inline int		V_wcslen(const wchar_t *pwch)						{ return (int)wcslen(pwch); }
inline char*	V_strrchr (const char *s, char c)					{ return (char*)strrchr( s, c ); }
inline int		V_strcmp (const char *s1, const char *s2)			{ return strcmp( s1, s2 ); }
inline int		V_wcscmp (const wchar_t *s1, const wchar_t *s2)		{ return wcscmp( s1, s2 ); }
inline int		V_stricmp( const char *s1, const char *s2 )			{ return stricmp( s1, s2 ); }
inline char*	V_strstr( const char *s1, const char *search )		{ return (char*)strstr( s1, search ); }
inline char*	V_strupr (char *start)								{ return strupr( start ); }
inline char*	V_strlower (char *start)							{ return strlwr( start ); }

#endif


int			V_strncmp (const char *s1, const char *s2, int count);
int			V_strcasecmp (const char *s1, const char *s2);
int			V_strncasecmp (const char *s1, const char *s2, int n);
int			V_strnicmp (const char *s1, const char *s2, int n);
int			V_atoi (const char *str);
int64 		V_atoi64(const char *str);
uint64 		V_atoui64(const char *str);
float		V_atof (const char *str);
char*		V_stristr( char* pStr, const char* pSearch );
const char*	V_stristr( const char* pStr, const char* pSearch );
const char*	V_strnistr( const char* pStr, const char* pSearch, int n );
const char*	V_strnchr( const char* pStr, char c, int n );

// returns string immediately following prefix, (ie str+strlen(prefix)) or NULL if prefix not found
const char *StringAfterPrefix             ( const char *str, const char *prefix );
const char *StringAfterPrefixCaseSensitive( const char *str, const char *prefix );
inline bool	StringHasPrefix             ( const char *str, const char *prefix ) { return StringAfterPrefix             ( str, prefix ) != NULL; }
inline bool	StringHasPrefixCaseSensitive( const char *str, const char *prefix ) { return StringAfterPrefixCaseSensitive( str, prefix ) != NULL; }


// Normalizes a float string in place.  
// (removes leading zeros, trailing zeros after the decimal point, and the decimal point itself where possible)
void			V_normalizeFloatString( char* pFloat );

inline bool V_isspace(int c)
{
	// The standard white-space characters are the following: space, tab, carriage-return, newline, vertical tab, and form-feed. In the C locale, V_isspace() returns true only for the standard white-space characters. 
	//return c == ' ' || c == 9 /*horizontal tab*/ || c == '\r' || c == '\n' || c == 11 /*vertical tab*/ || c == '\f';
	// codes of whitespace symbols: 9 HT, 10 \n, 11 VT, 12 form feed, 13 \r, 32 space
	
	// easy to understand version, validated:
	// return ((1 << (c-1)) & 0x80001F00) != 0 && ((c-1)&0xE0) == 0;
	
	// 5% faster on Core i7, 35% faster on Xbox360, no branches, validated:
	#ifdef _X360
	return ((1 << (c-1)) & 0x80001F00 & ~(-int((c-1)&0xE0))) != 0;
	#else
	// this is 11% faster on Core i7 than the previous, VC2005 compiler generates a seemingly unbalanced search tree that's faster
	switch(c)
	{
	case ' ':
	case 9:
	case '\r':
	case '\n':
	case 11:
	case '\f':
		return true;
	default:
		return false;
	}
	#endif
}


// These are versions of functions that guarantee NULL termination.
//
// maxLen is the maximum number of bytes in the destination string.
// pDest[maxLen-1] is always NULL terminated if pSrc's length is >= maxLen.
//
// This means the last parameter can usually be a sizeof() of a string.
void V_strncpy( char *pDest, const char *pSrc, int maxLen );
int V_snprintf( char *pDest, int destLen, const char *pFormat, ... ) FMTFUNCTION( 3, 4 );
void V_wcsncpy( wchar_t *pDest, wchar_t const *pSrc, int maxLenInBytes );
int V_snwprintf( wchar_t *pDest, int destLen, const wchar_t *pFormat, ... );

#define COPY_ALL_CHARACTERS -1
char *V_strncat(char *, const char *, size_t destBufferSize, int max_chars_to_copy=COPY_ALL_CHARACTERS );
char *V_strnlwr(char *, size_t);


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

int V_vsnprintf( char *pDest, int maxLenInCharacters, const char *pFormat, va_list params );
template <size_t maxLenInCharacters> int V_vsprintf_safe( char (&pDest)[maxLenInCharacters], const char *pFormat, va_list params ) { return V_vsnprintf( pDest, maxLenInCharacters, pFormat, params ); }
int V_vsnprintf( char *pDest, int maxLen, const char *pFormat, va_list params );

// Prints out a pretified memory counter string value ( e.g., 7,233.27 Mb, 1,298.003 Kb, 127 bytes )
char *V_pretifymem( float value, int digitsafterdecimal = 2, bool usebinaryonek = false );

// Prints out a pretified integer with comma separators (eg, 7,233,270,000)
char *V_pretifynum( int64 value );

// conversion functions wchar_t <-> char, returning the number of characters converted
int V_UTF8ToUnicode( const char *pUTF8, wchar_t *pwchDest, int cubDestSizeInBytes );
int V_UnicodeToUTF8( const wchar_t *pUnicode, char *pUTF8, int cubDestSizeInBytes );
int V_UCS2ToUnicode( const ucs2 *pUCS2, wchar_t *pUnicode, int cubDestSizeInBytes );
int V_UCS2ToUTF8( const ucs2 *pUCS2, char *pUTF8, int cubDestSizeInBytes );

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
void V_hextobinary( char const *in, int numchars, byte *out, int maxoutputbytes );
void V_binarytohex( const byte *in, int inputbytes, char *out, int outsize );

// Tools for working with filenames
// Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
void V_FileBase( const char *in, char *out,int maxlen );
// Remove the final characters of ppath if it's '\' or '/'.
void V_StripTrailingSlash( char *ppath );
// Remove any extension from in and return resulting string in out
void V_StripExtension( const char *in, char *out, int outLen );
// Make path end with extension if it doesn't already have an extension
void V_DefaultExtension( char *path, const char *extension, int pathStringLength );
// Strips any current extension from path and ensures that extension is the new extension.
// NOTE: extension string MUST include the . character
void V_SetExtension( char *path, const char *extension, int pathStringLength );
// Removes any filename from path ( strips back to previous / or \ character )
void V_StripFilename( char *path );
// Remove the final directory from the path
bool V_StripLastDir( char *dirName, int maxlen );
// Returns a pointer to the unqualified file name (no path) of a file name
const char * V_UnqualifiedFileName( const char * in );
// Given a path and a filename, composes "path\filename", inserting the (OS correct) separator if necessary
void V_ComposeFileName( const char *path, const char *filename, char *dest, int destSize );

// Copy out the path except for the stuff after the final pathseparator
bool V_ExtractFilePath( const char *path, char *dest, int destSize );
// Copy out the file extension into dest
void V_ExtractFileExtension( const char *path, char *dest, int destSize );

const char *V_GetFileExtension( const char * path );

// This removes "./" and "../" from the pathname. pFilename should be a full pathname.
// Returns false if it tries to ".." past the root directory in the drive (in which case 
// it is an invalid path).
bool V_RemoveDotSlashes( char *pFilename, char separator = CORRECT_PATH_SEPARATOR );

// If pPath is a relative path, this function makes it into an absolute path
// using the current working directory as the base, or pStartingDir if it's non-NULL.
// Returns false if it runs out of room in the string, or if pPath tries to ".." past the root directory.
void V_MakeAbsolutePath( char *pOut, int outLen, const char *pPath, const char *pStartingDir = NULL );

// Creates a relative path given two full paths
// The first is the full path of the file to make a relative path for.
// The second is the full path of the directory to make the first file relative to
// Returns false if they can't be made relative (on separate drives, for example)
bool V_MakeRelativePath( const char *pFullPath, const char *pDirectory, char *pRelativePath, int nBufLen );

// Fixes up a file name, removing dot slashes, fixing slashes, converting to lowercase, etc.
void V_FixupPathName( char *pOut, size_t nOutLen, const char *pPath );

// Adds a path separator to the end of the string if there isn't one already. Returns false if it would run out of space.
void V_AppendSlash( char *pStr, int strSize );

// Returns true if the path is an absolute path.
bool V_IsAbsolutePath( const char *pPath );

// Scans pIn and replaces all occurences of pMatch with pReplaceWith.
// Writes the result to pOut.
// Returns true if it completed successfully.
// If it would overflow pOut, it fills as much as it can and returns false.
bool V_StrSubst( const char *pIn, const char *pMatch, const char *pReplaceWith,
	char *pOut, int outLen, bool bCaseSensitive=false );


// Split the specified string on the specified separator.
// Returns a list of strings separated by pSeparator.
// You are responsible for freeing the contents of outStrings (call outStrings.PurgeAndDeleteElements).
void V_SplitString( const char *pString, const char *pSeparator, CUtlVector<char*, CUtlMemory<char*, int> > &outStrings );

// Just like V_SplitString, but it can use multiple possible separators.
void V_SplitString2( const char *pString, const char **pSeparators, int nSeparators, CUtlVector<char*, CUtlMemory<char*, int> > &outStrings );

// Returns false if the buffer is not large enough to hold the working directory name.
bool V_GetCurrentDirectory( char *pOut, int maxLen );

// Set the working directory thus.
bool V_SetCurrentDirectory( const char *pDirName );


// This function takes a slice out of pStr and stores it in pOut.
// It follows the Python slice convention:
// Negative numbers wrap around the string (-1 references the last character).
// Large numbers are clamped to the end of the string.
void V_StrSlice( const char *pStr, int firstChar, int lastCharNonInclusive, char *pOut, int outSize );

// Chop off the left nChars of a string.
void V_StrLeft( const char *pStr, int nChars, char *pOut, int outSize );

// Chop off the right nChars of a string.
void V_StrRight( const char *pStr, int nChars, char *pOut, int outSize );

// change "special" characters to have their c-style backslash sequence. like \n, \r, \t, ", etc.
// returns a pointer to a newly allocated string, which you must delete[] when finished with.
char *V_AddBackSlashesToSpecialChars( char const *pSrc );

// Force slashes of either type to be = separator character
void V_FixSlashes( char *pname, char separator = CORRECT_PATH_SEPARATOR );

// This function fixes cases of filenames like materials\\blah.vmt or somepath\otherpath\\ and removes the extra double slash.
void V_FixDoubleSlashes( char *pStr );

// Convert multibyte to wchar + back
// Specify -1 for nInSize for null-terminated string
void V_strtowcs( const char *pString, int nInSize, wchar_t *pWString, int nOutSize );
void V_wcstostr( const wchar_t *pWString, int nInSize, char *pString, int nOutSize );

// buffer-safe strcat
inline void V_strcat( char *dest, const char *src, int cchDest )
{
	V_strncat( dest, src, cchDest, COPY_ALL_CHARACTERS );
}

// Convert from a string to an array of integers.
void V_StringToIntArray( int *pVector, int count, const char *pString );

// Convert from a string to a 4 byte color value.
void V_StringToColor32( color32 *color, const char *pString );

// Convert \r\n (Windows linefeeds) to \n (Unix linefeeds).
void V_TranslateLineFeedsToUnix( char *pStr );

//-----------------------------------------------------------------------------
// generic unique name helper functions
//-----------------------------------------------------------------------------

// returns -1 if no match, nDefault if pName==prefix, and N if pName==prefix+N
inline int V_IndexAfterPrefix( const char *pName, const char *prefix, int nDefault = 0 )
{
	if ( !pName || !prefix )
		return -1;

	const char *pIndexStr = StringAfterPrefix( pName, prefix );
	if ( !pIndexStr )
		return -1;

	if ( !*pIndexStr )
		return nDefault;

	return atoi( pIndexStr );
}

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


extern bool V_StringToBin( const char*pString, void *pBin, uint nBinSize );
extern bool V_BinToString( char*pString, void *pBin, uint nBinSize );

template<typename T>
struct BinString_t
{
	BinString_t(){}
	BinString_t( const char *pStr )
	{
		V_strncpy( m_string, pStr, sizeof(m_string) );
		ToBin();
	}
	BinString_t( const T & that )
	{
		m_bin = that;
		ToString();
	}
	bool ToBin()
	{
		return V_StringToBin( m_string, &m_bin, sizeof( m_bin ) );
	}
	void ToString()
	{
		V_BinToString( m_string, &m_bin, sizeof( m_bin ) );
	}
	T m_bin;
	char m_string[sizeof(T)*2+2]; // 0-terminated string representing the binary data in hex
};

template <typename T>
inline BinString_t<T> MakeBinString( const T& that )
{
	return BinString_t<T>( that );
}



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
#define Q_strupr				V_strupr
#define Q_strlower				V_strlower
#define Q_wcslen				V_wcslen
#define	Q_strncmp				V_strncmp 
#define	Q_strcasecmp			V_strcasecmp
#define	Q_strncasecmp			V_strncasecmp
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
#define Q_snwprintf				V_snwprintf
#define Q_wcsncpy				V_wcsncpy
#define Q_strncat				V_strncat
#define Q_strnlwr				V_strnlwr
#define Q_vsnprintf				V_vsnprintf
#define Q_pretifymem			V_pretifymem
#define Q_pretifynum			V_pretifynum
#define Q_UTF8ToUnicode			V_UTF8ToUnicode
#define Q_UnicodeToUTF8			V_UnicodeToUTF8
#define Q_hextobinary			V_hextobinary
#define Q_binarytohex			V_binarytohex
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
#define Q_SplitString2			V_SplitString2
#define Q_StrSlice				V_StrSlice
#define Q_StrLeft				V_StrLeft
#define Q_StrRight				V_StrRight
#define Q_FixSlashes			V_FixSlashes
#define Q_strtowcs				V_strtowcs
#define Q_wcstostr				V_wcstostr
#define Q_strcat				V_strcat
#define Q_MakeRelativePath		V_MakeRelativePath
#define Q_FixupPathName			V_FixupPathName

#endif // !defined( VSTDLIB_DLL_EXPORT )


#endif	// TIER1_STRTOOLS_H
