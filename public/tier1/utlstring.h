//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef UTLSTRING_H
#define UTLSTRING_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlmemory.h"
#include "tier1/strtools.h"
#include "limits.h"

#if defined( OSX )
inline wchar_t *wcsdup(const wchar_t *pString)
{
	wchar_t *pMemory;

	if (!pString)
		return NULL;

	size_t len = (wcslen(pString) + 1);
	if ((pMemory = (wchar_t *)malloc(len * sizeof(wchar_t))) != NULL)
	{
		return wcscpy( pMemory, pString );
	}

	return NULL;
}

inline size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return (p ? p - s : n);
}

#endif

class CUtlStringToken
{
	unsigned int m_nHashCode;
};

//-----------------------------------------------------------------------------
// Simple string class. 
// NOTE: This is *not* optimal! Use in tools, but not runtime code
//-----------------------------------------------------------------------------
class CUtlString
{
public:
	typedef enum
	{
		PATTERN_NONE		= 0x00000000,
		PATTERN_DIRECTORY	= 0x00000001
	} TUtlStringPattern;

public:
	// Defining AltArgumentType_t hints that associative container classes should
	// also implement Find/Insert/Remove functions that take const char* params.
	typedef const char* AltArgumentType_t;

	CUtlString();
	CUtlString(const char *pString);
	CUtlString(const char *pString, int length);
	CUtlString(const CUtlString &string);
	DLL_CLASS_IMPORT CUtlString(const CBufferString &string);

	~CUtlString();

	DLL_CLASS_IMPORT CUtlString &operator=(const CBufferString &src);
	CUtlString &operator=(const CUtlString &src);
	CUtlString &operator=(const char *src);

	// Test for equality, both are case sensitive
	DLL_CLASS_IMPORT bool operator==(const CUtlString &src) const;
	DLL_CLASS_IMPORT bool operator==(const CBufferString &src) const;

	bool operator!=(const CUtlString &src) const { return !operator==(src); }
	bool operator!=(const CBufferString &src) const { return !operator==(src); }

	DLL_CLASS_IMPORT char operator[](int i) const;

	DLL_CLASS_IMPORT CUtlString operator+(const char *pOther) const;
	DLL_CLASS_IMPORT CUtlString operator+(const CUtlString &other) const;
	DLL_CLASS_IMPORT CUtlString operator+(int rhs) const;

	DLL_CLASS_IMPORT CUtlString &operator+=(const CUtlString &rhs);
	DLL_CLASS_IMPORT CUtlString &operator+=(const CBufferString &rhs);
	DLL_CLASS_IMPORT CUtlString &operator+=(char c);
	DLL_CLASS_IMPORT CUtlString &operator+=(int rhs);
	DLL_CLASS_IMPORT CUtlString &operator+=(double rhs);
	DLL_CLASS_IMPORT CUtlString &operator+=(const char *rhs);

	const char *Get() const;
	operator const char *() const { return Get(); }
	const char *String() const { return Get(); }

	DLL_CLASS_IMPORT void		Set(const char *pValue);

	// Get this string as an absolute path (calls right through to V_MakeAbsolutePathBuffer).
	DLL_CLASS_IMPORT CUtlString AbsPath(const char *pStartingDir = NULL) const;

	// Calls CBufferString::EnsureOwnedAllocation
	// If the second parameter is true it will set UNK4 from EAllocationOption_t
	// Always uses ALLOW_HEAP_ALLOCATION
	DLL_CLASS_IMPORT void Acquire(CBufferString *pBufferString, bool bUNK4);

	DLL_CLASS_IMPORT void Append(const char *pchAddition);
	DLL_CLASS_IMPORT void Append(const char *pAddition, int nChars);

	DLL_CLASS_IMPORT void Convert(wchar_t *string);

	// Uses CBufferString::ExtractFilePath
	// Returns the root path to the provided directory "a/b/c/" -> "a/b"
	DLL_CLASS_IMPORT CUtlString DirName() const;

	// Appends the string starting with a dot "file".DottedAppend("test") -> "file.test"
	DLL_CLASS_IMPORT CUtlString DottedAppend(const char *string) const;

	// Chops the string to fit the nChop length, appending "..." as a last symbols (inclusive)
	// Works only with nChop amount >= 3, otherwise string would be returned as is
	DLL_CLASS_IMPORT CUtlString Ellipsify(int nChop);

	DLL_CLASS_IMPORT void FixSlashes(char cSeparator = CORRECT_PATH_SEPARATOR);
	DLL_CLASS_IMPORT void FixupPathName();

	DLL_CLASS_IMPORT int Format(const char *pFormat, ...)  FMTFUNCTION(2, 3);
	DLL_CLASS_IMPORT int FormatV(const char *pFormat, va_list marker);

	// Get a string with the base filename (with CBufferString::ExtractFileBase).
	DLL_CLASS_IMPORT CUtlString GetBaseFilename() const;
	// Empty string for those times when you need to return an empty string and
	// either don't want to pay the construction cost, or are returning a
	// const CUtlString& and cannot just return "".
	DLL_CLASS_IMPORT static const CUtlString &GetEmptyString();
	// Get a string with the file extension
	DLL_CLASS_IMPORT CUtlString GetExtensionAlloc() const;

	DLL_CLASS_IMPORT char *GetForModify();

	DLL_CLASS_IMPORT bool IsEqual_CaseSensitive(const char *src) const;
	DLL_CLASS_IMPORT bool IsEqual_FastCaseInsensitive(const char *src) const;

	// case SENSITIVE, use * for wildcard in pattern string
	// nFlags checks for PATTERN_DIRECTORY
	DLL_CLASS_IMPORT bool MatchesPattern(const CUtlString &Pattern, int nFlags = 0) const;

	// Works like V_ComposeFileName.
	DLL_CLASS_IMPORT static CUtlString PathJoin(const char *pStr1, const char *pStr2);

	// These can be used for utlvector sorts.
	static int __cdecl SortCaseInsensitive(const CUtlString *pString1, const CUtlString *pString2);
	static int __cdecl SortCaseSensitive(const CUtlString *pString1, const CUtlString *pString2);

	DLL_CLASS_IMPORT void Purge();
	void Clear() { Purge(); }

	DLL_CLASS_IMPORT CUtlString Remove(const char *string, bool bCaseSensitive = true) const;
	DLL_CLASS_IMPORT void		RemoveDotSlashes(char replacement);
	DLL_CLASS_IMPORT CUtlString RemoveFromStart(const char *search, bool bCaseSensitive = true) const;

	// Replaces all instances of specified char with a char
	DLL_CLASS_IMPORT CUtlString Replace(char cSearch, char cReplace) const;
	// Replaces all instances of specified char with a string
	DLL_CLASS_IMPORT CUtlString Replace(const char cSearch, const char *pszReplacement) const;
	// Replaces all instances of specified string with another string
	DLL_CLASS_IMPORT CUtlString Replace(const char *pszSearch, const char *pszReplacement, bool bCaseSensitive = true) const;

	// Get a substring starting from the right side.
	DLL_CLASS_IMPORT CUtlString Right(int32 nChars) const;
	// Get a substring starting from the left side.
	DLL_CLASS_IMPORT CUtlString Left(int32 nChars) const;

	// Set directly and don't look for a null terminator in pValue.
	// nChars does not include the nul and this will only copy
	// at most nChars (even if pValue is longer).  If nChars
	// is >strlen(pValue) it will copy past the end, don't do it
	// Does nothing if pValue == String()
	DLL_CLASS_IMPORT void		SetDirect(const char *pValue, int nChars);
	// Sets the length (used to serialize into the buffer )
	// Note: If nLen != 0, then this adds an extra byte for a null-terminator.	
	DLL_CLASS_IMPORT void		SetLength(int nLen);

	// Get a copy of part of the string.
	// If you only specify nStart, it'll go from nStart to the end.
	// You can use negative numbers and it'll wrap around to the start.
	DLL_CLASS_IMPORT CUtlString Slice(int32 nStart = 0, int32 nEnd = INT_MAX) const;

	// Get a string with the extension removed
	DLL_CLASS_IMPORT CUtlString StripExtension() const;
	// Get a string with the filename removed (doesn't strip the trailing slash, looks like it was intentional)
	DLL_CLASS_IMPORT CUtlString StripFilename() const;
	DLL_CLASS_IMPORT CUtlString StripFirstDirectory() const;
	DLL_CLASS_IMPORT void		StripTrailingSlash();

	// Swaps this object with target
	DLL_CLASS_IMPORT void Swap(CUtlString &target);

	// Case Change
	DLL_CLASS_IMPORT void ToLowerFast();
	DLL_CLASS_IMPORT void ToUpperFast();

	// Trim whitespace
	DLL_CLASS_IMPORT void Trim(char cTarget);
	DLL_CLASS_IMPORT void Trim(const char *szTargets = "\t\r\n ");
	DLL_CLASS_IMPORT void TrimLeft(char cTarget);
	DLL_CLASS_IMPORT void TrimLeft(const char *szTargets = "\t\r\n ");
	DLL_CLASS_IMPORT void TrimRight(char cTarget);
	DLL_CLASS_IMPORT void TrimRight(const char *szTargets = "\t\r\n ");

	// param1's least significant 4 bits (param1 & 0xf) are used in a switch in V_UnicodeCaseConvert
	// return value is also dependant on V_UnicodeCaseConvert
	DLL_CLASS_IMPORT int UnicodeCaseConvert(int param1, EStringConvertErrorPolicy eErrorPolicy);

	// Gets the filename (everything except the path.. c:\a\b\c\somefile.txt -> somefile.txt).
	DLL_CLASS_IMPORT CUtlString UnqualifiedFilenameAlloc() const;

	// Returns strlen
	int Length() const;
	// IsEmpty() is more efficient than Length() == 0
	bool IsEmpty() const;

private:
	// AllocMemory allocates enough space for length characters plus a terminating zero.
	// Previous characters are preserved, the buffer is null-terminated, but new characters
	// are not touched.
	DLL_CLASS_IMPORT void *AllocMemoryBlock(uint32 length);
	DLL_CLASS_IMPORT void FreeMemoryBlock();

	// If m_pString is not NULL, it points to the start of the string, and the memory allocation.
	char *m_pString;
};

//	// If these are not defined, CUtlConstString as rhs will auto-convert
//	// to const char* and do logical operations on the raw pointers. Ugh.
//	inline friend bool operator<( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) > 0; }
//	inline friend bool operator==( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) == 0; }
//	inline friend bool operator!=( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) != 0; }

inline CUtlString &CUtlString::operator=( const CUtlString &src )
{
	SetDirect( src.Get(), src.Length() );
	return *this;
}

inline CUtlString &CUtlString::operator=( const char *src )
{
	Set( src );
	return *this;
}

inline bool operator==( const char *pString, const CUtlString &utlString )
{
	return utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator!=( const char *pString, const CUtlString &utlString )
{
	return !utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator==( const CUtlString &utlString, const char *pString )
{
	return utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator!=( const CUtlString &utlString, const char *pString )
{
	return !utlString.IsEqual_CaseSensitive( pString );
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CUtlString::CUtlString()
	: m_pString( NULL )
{
}

inline CUtlString::CUtlString( const char *pString )
	: m_pString( NULL )
{
	Set( pString );
}

inline CUtlString::CUtlString( const char *pString, int length )
	: m_pString( NULL )
{
	SetDirect( pString, length );
}

inline CUtlString::CUtlString( const CUtlString &string )
	: m_pString( NULL )
{
	Set( string.Get() );
}

inline CUtlString::~CUtlString()
{
	Purge();
}

inline const char *CUtlString::Get() const
{
	if(!m_pString)
	{
		return "";
	}
	return m_pString;
}

inline int CUtlString::Length() const
{
	if(m_pString)
	{
		return V_strlen( m_pString );
	}
	return 0;
}

inline bool CUtlString::IsEmpty() const
{
	return !m_pString || m_pString[0] == 0;
}

inline int __cdecl CUtlString::SortCaseInsensitive( const CUtlString *pString1, const CUtlString *pString2 )
{
	return V_stricmp( pString1->String(), pString2->String() );
}

inline int __cdecl CUtlString::SortCaseSensitive( const CUtlString *pString1, const CUtlString *pString2 )
{
	return V_strcmp( pString1->String(), pString2->String() );
}



//-----------------------------------------------------------------------------
// Purpose: Implementation of low-level string functionality for character types.
//-----------------------------------------------------------------------------

template < typename T >
class StringFuncs
{
public:
	static T		*Duplicate( const T *pValue );
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void		 Copy( T *out_pOut, const T *pIn, int iLengthInChars );
	static int		 Compare( const T *pLhs, const T *pRhs );
	static int		 CaselessCompare( const T *pLhs, const T *pRhs );
	static int		 Length( const T *pValue );
	static const T  *FindChar( const T *pStr, const T cSearch );
	static const T	*EmptyString();
	static const T	*NullDebugString();
};

template < >
class StringFuncs<char>
{
public:
	static char		  *Duplicate( const char *pValue ) { return strdup( pValue ); }
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void		   Copy( char *out_pOut, const char *pIn, int iLengthInChars ) { strncpy( out_pOut, pIn, iLengthInChars ); }
	static int		   Compare( const char *pLhs, const char *pRhs ) { return strcmp( pLhs, pRhs ); }
	static int		   CaselessCompare( const char *pLhs, const char *pRhs ) { return Q_strcasecmp( pLhs, pRhs ); }
	static int		   Length( const char *pValue ) { return (int)strlen( pValue ); }
	static const char *FindChar( const char *pStr, const char cSearch ) { return strchr( pStr, cSearch ); }
	static const char *EmptyString() { return ""; }
	static const char *NullDebugString() { return "(null)"; }
};

template < >
class StringFuncs<wchar_t>
{
public:
	static wchar_t		 *Duplicate( const wchar_t *pValue ) { return wcsdup( pValue ); }
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void			  Copy( wchar_t *out_pOut, const wchar_t  *pIn, int iLengthInChars ) { wcsncpy( out_pOut, pIn, iLengthInChars ); }
	static int			  Compare( const wchar_t *pLhs, const wchar_t *pRhs ) { return wcscmp( pLhs, pRhs ); }
	static int			  CaselessCompare( const wchar_t *pLhs, const wchar_t *pRhs ); // no implementation?
	static int			  Length( const wchar_t *pValue ) { return (int)wcslen( pValue ); }
	static const wchar_t *FindChar( const wchar_t *pStr, const wchar_t cSearch ) { return wcschr( pStr, cSearch ); }
	static const wchar_t *EmptyString() { return L""; }
	static const wchar_t *NullDebugString() { return L"(null)"; }
};

//-----------------------------------------------------------------------------
// Dirt-basic auto-release string class. Not intended for manipulation,
// can be stored in a container or forwarded as a functor parameter.
// Note the benefit over CUtlString: sizeof(CUtlConstString) == sizeof(char*).
// Also note: null char* pointers are treated identically to empty strings.
//-----------------------------------------------------------------------------

template < typename T = char >
class CUtlConstStringBase
{
public:
	CUtlConstStringBase() : m_pString( NULL ) {}
	explicit CUtlConstStringBase( const T *pString ) : m_pString( NULL ) { Set( pString ); }
	CUtlConstStringBase( const CUtlConstStringBase& src ) : m_pString( NULL ) { Set( src.m_pString ); }
	~CUtlConstStringBase() { Set( NULL ); }

	void Set( const T *pValue );
	void Clear() { Set( NULL ); }

	const T *Get() const { return m_pString ? m_pString : StringFuncs<T>::EmptyString(); }
	operator const T*() const { return m_pString ? m_pString : StringFuncs<T>::EmptyString(); }

	bool IsEmpty() const { return m_pString == NULL; } // Note: empty strings are never stored by Set

	int Compare( const T *rhs ) const;

	// Logical ops
	bool operator<( const T *rhs ) const { return Compare( rhs ) < 0; }
	bool operator==( const T *rhs ) const { return Compare( rhs ) == 0; }
	bool operator!=( const T *rhs ) const { return Compare( rhs ) != 0; }
	bool operator<( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) < 0; }
	bool operator==( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) == 0; }
	bool operator!=( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) != 0; }

	// If these are not defined, CUtlConstString as rhs will auto-convert
	// to const char* and do logical operations on the raw pointers. Ugh.
	inline friend bool operator<( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) > 0; }
	inline friend bool operator==( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) == 0; }
	inline friend bool operator!=( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) != 0; }

	CUtlConstStringBase &operator=( const T *src ) { Set( src ); return *this; }
	CUtlConstStringBase &operator=( const CUtlConstStringBase &src ) { Set( src.m_pString ); return *this; }

	// Defining AltArgumentType_t is a hint to containers that they should
	// implement Find/Insert/Remove functions that take const char* params.
	typedef const T *AltArgumentType_t;

protected:
	const T *m_pString;
};

template < typename T >
void CUtlConstStringBase<T>::Set( const T *pValue )
{
	if ( pValue != m_pString )
	{
		free( ( void* ) m_pString );
		m_pString = pValue && pValue[0] ? StringFuncs<T>::Duplicate( pValue ) : NULL;
	}
}

template < typename T >
int CUtlConstStringBase<T>::Compare( const T *rhs ) const
{
	// Empty or null RHS?
	if ( !rhs || !rhs[0] )
		return m_pString ? 1 : 0;

	// Empty *this, non-empty RHS?
	if ( !m_pString )
		return -1;

	// Neither empty
	return StringFuncs<T>::Compare( m_pString, rhs );
}

typedef	CUtlConstStringBase<char>		CUtlConstString;
typedef	CUtlConstStringBase<wchar_t>	CUtlConstWideString;

//-----------------------------------------------------------------------------
// Helper functor objects.
//-----------------------------------------------------------------------------

template < typename T > struct UTLConstStringCaselessStringLessFunctor { bool operator()( const CUtlConstStringBase<T>& a, const char *b ) const { return StringFuncs<T>::CaselessCompare( a.Get(), b ) < 0; } };
template < typename T > struct UTLConstStringCaselessStringEqualFunctor { bool operator()( const CUtlConstStringBase<T>& a, const char *b ) const { return StringFuncs<T>::CaselessCompare( a.Get(), b ) == 0; } };

// Helper function for CUtlMaps with a CUtlString key
inline bool UtlStringLessFunc( const CUtlString &lhs, const CUtlString &rhs ) { return V_strcmp( lhs.Get(), rhs.Get() ) < 0; } 
inline bool UtlStringCaseInsensitiveLessFunc( const CUtlString &lhs, const CUtlString &rhs ) { return V_stricmp( lhs.Get(), rhs.Get() ) < 0; } 

#endif // UTLSTRING_H
