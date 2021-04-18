//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
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


//-----------------------------------------------------------------------------
// Base class, containing simple memory management
//-----------------------------------------------------------------------------
class CUtlBinaryBlock
{
public:
	CUtlBinaryBlock( int growSize = 0, int initSize = 0 );

	// NOTE: nInitialLength indicates how much of the buffer starts full
	CUtlBinaryBlock( void* pMemory, int nSizeInBytes, int nInitialLength );
	CUtlBinaryBlock( const void* pMemory, int nSizeInBytes );
	CUtlBinaryBlock( const CUtlBinaryBlock& src );

	void		Get( void *pValue, int nMaxLen ) const;
	void		Set( const void *pValue, int nLen );
	const void	*Get( ) const;
	void		*Get( );

	unsigned char& operator[]( int i );
	const unsigned char& operator[]( int i ) const;

	int			Length() const;
	void		SetLength( int nLength );	// Undefined memory will result
	bool		IsEmpty() const;

	bool		IsReadOnly() const;

	CUtlBinaryBlock &operator=( const CUtlBinaryBlock &src );

	// Test for equality
	bool operator==( const CUtlBinaryBlock &src ) const;

private:
	CUtlMemory<unsigned char> m_Memory;
	int m_nActualLength;
};


//-----------------------------------------------------------------------------
// class inlines
//-----------------------------------------------------------------------------
inline const void *CUtlBinaryBlock::Get( ) const
{
	return m_Memory.Base();
}

inline void *CUtlBinaryBlock::Get( )
{
	return m_Memory.Base();
}

inline int CUtlBinaryBlock::Length() const
{
	return m_nActualLength;
}

inline unsigned char& CUtlBinaryBlock::operator[]( int i )
{
	return m_Memory[i];
}

inline const unsigned char& CUtlBinaryBlock::operator[]( int i ) const
{
	return m_Memory[i];
}

inline bool CUtlBinaryBlock::IsReadOnly() const
{
	return m_Memory.IsReadOnly();
}

inline bool CUtlBinaryBlock::IsEmpty() const
{
	return Length() == 0;
}


//-----------------------------------------------------------------------------
// Simple string class. 
// NOTE: This is *not* optimal! Use in tools, but not runtime code
//-----------------------------------------------------------------------------
class CUtlString
{
public:
	CUtlString();
	CUtlString( const char *pString );
	CUtlString( const CUtlString& string );

	// Attaches the string to external memory. Useful for avoiding a copy
	CUtlString( void* pMemory, int nSizeInBytes, int nInitialLength );
	CUtlString( const void* pMemory, int nSizeInBytes );

	const char	*Get( ) const;
	void		Set( const char *pValue );

	// Converts to c-strings
	operator const char*() const;

	// for compatibility switching items from UtlSymbol
	const char  *String() const { return Get(); }

	// Returns strlen
	int			Length() const;
	bool		IsEmpty() const;

	// Sets the length (used to serialize into the buffer )
	void		SetLength( int nLen );
	char		*Get();

	// Strips the trailing slash
	void		StripTrailingSlash();
	
	void		ToLower();

	CUtlString &operator=( const CUtlString &src );
	CUtlString &operator=( const char *src );

	// Test for equality
	bool operator==( const CUtlString &src ) const;
	bool operator==( const char *src ) const;
	bool operator!=( const CUtlString &src ) const { return !operator==( src ); }
	bool operator!=( const char *src ) const { return !operator==( src ); }

	CUtlString &operator+=( const CUtlString &rhs );
	CUtlString &operator+=( const char *rhs );
	CUtlString &operator+=( char c );
	CUtlString &operator+=( int rhs );
	CUtlString &operator+=( double rhs );
	
	void		Append( const char *pchAddition )
	{ this->operator+=(pchAddition); }

	int Format( const char *pFormat, ... );

private:
	CUtlBinaryBlock m_Storage;
};

inline void CUtlString::ToLower()
{
	if ( IsEmpty() )
	{
		return;
	}

	V_strlower( (char *)m_Storage.Get() );
}

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline bool CUtlString::IsEmpty() const
{
	return Length() == 0;
}

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
	static void		   Copy( OUT_CAP(iLengthInChars) char *out_pOut, const char *pIn, int iLengthInChars ) { strncpy( out_pOut, pIn, iLengthInChars ); }
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
	static void			  Copy( OUT_CAP(iLengthInChars) wchar_t *out_pOut, const wchar_t  *pIn, int iLengthInChars ) { wcsncpy( out_pOut, pIn, iLengthInChars ); }
	static int			  Compare( const wchar_t *pLhs, const wchar_t *pRhs ) { return wcscmp( pLhs, pRhs ); }
	static int			  CaselessCompare( const wchar_t *pLhs, const wchar_t *pRhs ); // no implementation?
	static int			  Length( const wchar_t *pValue ) { return (int)wcslen( pValue ); }
	static const wchar_t *FindChar( const wchar_t *pStr, const wchar_t cSearch ) { return wcschr( pStr, cSearch ); }
	static const wchar_t *EmptyString() { return L""; }
	static const wchar_t *NullDebugString() { return L"(null)"; }
};

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

#endif // UTLSTRING_H
