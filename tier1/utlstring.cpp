//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "tier1/utlstring.h"
#include "tier1/strtools.h"


//-----------------------------------------------------------------------------
// Base class, containing simple memory management
//-----------------------------------------------------------------------------
CUtlBinaryBlock::CUtlBinaryBlock( int growSize, int initSize ) : m_Memory( growSize, initSize )
{
	m_nActualLength = 0;
}

CUtlBinaryBlock::CUtlBinaryBlock( void* pMemory, int nSizeInBytes, int nInitialLength ) : m_Memory( (unsigned char*)pMemory, nSizeInBytes )
{
	m_nActualLength = nInitialLength;
}

CUtlBinaryBlock::CUtlBinaryBlock( const void* pMemory, int nSizeInBytes ) : m_Memory( (const unsigned char*)pMemory, nSizeInBytes )
{
	m_nActualLength = nSizeInBytes;
}

CUtlBinaryBlock::CUtlBinaryBlock( const CUtlBinaryBlock& src )
{
	Set( src.Get(), src.Length() );
}

void CUtlBinaryBlock::Get( void *pValue, int nLen ) const
{
	Assert( nLen > 0 );
	if ( m_nActualLength < nLen )
	{
		nLen = m_nActualLength;
	}

	if ( nLen > 0 )
	{
		memcpy( pValue, m_Memory.Base(), nLen );
	}
}

void CUtlBinaryBlock::SetLength( int nLength )
{
	Assert( !m_Memory.IsReadOnly() );

	m_nActualLength = nLength;
	if ( nLength > m_Memory.NumAllocated() )
	{
		int nOverFlow = nLength - m_Memory.NumAllocated();
		m_Memory.Grow( nOverFlow );

		// If the reallocation failed, clamp length
		if ( nLength > m_Memory.NumAllocated() )
		{
			m_nActualLength = m_Memory.NumAllocated();
		}
	}

#ifdef _DEBUG
	if ( m_Memory.NumAllocated() > m_nActualLength )
	{
		memset( ( ( char * )m_Memory.Base() ) + m_nActualLength, 0xEB, m_Memory.NumAllocated() - m_nActualLength );
	}
#endif
}

void CUtlBinaryBlock::Set( const void *pValue, int nLen )
{
	Assert( !m_Memory.IsReadOnly() );

	if ( !pValue )
	{
		nLen = 0;
	}

	SetLength( nLen );

	if ( m_nActualLength )
	{
		if ( ( ( const char * )m_Memory.Base() ) >= ( ( const char * )pValue ) + nLen ||
			 ( ( const char * )m_Memory.Base() ) + m_nActualLength <= ( ( const char * )pValue ) )
		{
			memcpy( m_Memory.Base(), pValue, m_nActualLength );
		}
		else
		{
			memmove( m_Memory.Base(), pValue, m_nActualLength );
		}
	}
}


CUtlBinaryBlock &CUtlBinaryBlock::operator=( const CUtlBinaryBlock &src )
{
	Assert( !m_Memory.IsReadOnly() );
	Set( src.Get(), src.Length() );
	return *this;
}


bool CUtlBinaryBlock::operator==( const CUtlBinaryBlock &src ) const
{
	if ( src.Length() != Length() )
		return false;

	return !memcmp( src.Get(), Get(), Length() );
}


//-----------------------------------------------------------------------------
// Simple string class. 
//-----------------------------------------------------------------------------
CUtlString::CUtlString()
{
}

CUtlString::CUtlString( const char *pString )
{
	Set( pString );
}

CUtlString::CUtlString( const CUtlString& string )
{
	Set( string.Get() );
}

// Attaches the string to external memory. Useful for avoiding a copy
CUtlString::CUtlString( void* pMemory, int nSizeInBytes, int nInitialLength )
{
	m_pString = (char *)malloc(nSizeInBytes);
	m_pString = (char *)memcpy( m_pString, pMemory, nSizeInBytes );
}

CUtlString::CUtlString( const void* pMemory, int nSizeInBytes )
{
	m_pString = (char *)malloc(nSizeInBytes);
	m_pString = (char *)memcpy( m_pString, pMemory, nSizeInBytes );
}

void CUtlString::Set( const char *pValue )
{
	int nLen = pValue ? Q_strlen(pValue) + 1 : 1;

	m_pString = (char *)realloc(m_pString, nLen);
	strcpy(m_pString, pValue);
}

// Returns strlen
int CUtlString::Length() const
{
	return m_pString ? Q_strlen(m_pString) + 1 : 0;
}

// Sets the length (used to serialize into the buffer )
void CUtlString::SetLength( int nLen )
{
	m_pString = (char *)realloc(m_pString, nLen);
}

const char *CUtlString::Get( ) const
{
	if ( !m_pString )
	{
		return "";
	}

	return m_pString;
}

// Converts to c-strings
CUtlString::operator const char*() const
{
	return Get();
}

char *CUtlString::Get()
{
	if ( !m_pString )
	{
		Set("");
	}

	return m_pString;
}

CUtlString &CUtlString::operator=( const CUtlString &src )
{
	Set( src.Get() );
	return *this;
}

CUtlString &CUtlString::operator=( const char *src )
{
	Set( src );
	return *this;
}

bool CUtlString::operator==( const CUtlString &src ) const
{
	return ( strcmp( Get(), src.Get() ) == 0 );
}

bool CUtlString::operator==( const char *src ) const
{
	return ( strcmp( Get(), src ) == 0 );
}

CUtlString &CUtlString::operator+=( const CUtlString &rhs )
{
	const int lhsLength( Length() );
	const int rhsLength( rhs.Length() );
	const int requestedLength( lhsLength + rhsLength );

	SetLength( requestedLength );
	const int allocatedLength( Length() );
	const int copyLength( allocatedLength - lhsLength < rhsLength ? allocatedLength - lhsLength : rhsLength );
	memcpy( Get() + lhsLength, rhs.Get(), copyLength );
	m_pString[ allocatedLength ] = '\0';

	return *this;
}

CUtlString &CUtlString::operator+=( const char *rhs )
{
	const int lhsLength( Length() );
	const int rhsLength( Q_strlen( rhs ) );
	const int requestedLength( lhsLength + rhsLength );

	SetLength( requestedLength );
	const int allocatedLength( Length() );
	const int copyLength( allocatedLength - lhsLength < rhsLength ? allocatedLength - lhsLength : rhsLength );
	memcpy( Get() + lhsLength, rhs, copyLength );
	m_pString[ allocatedLength ] = '\0';

	return *this;
}

CUtlString &CUtlString::operator+=( char c )
{
	int nLength = Length();
	SetLength( nLength + 1 );
	m_pString[ nLength ] = c;
	m_pString[ nLength+1 ] = '\0';
	return *this;
}

CUtlString &CUtlString::operator+=( int rhs )
{
	Assert( sizeof( rhs ) == 4 );

	char tmpBuf[ 12 ];	// Sufficient for a signed 32 bit integer [ -2147483648 to +2147483647 ]
	Q_snprintf( tmpBuf, sizeof( tmpBuf ), "%d", rhs );
	tmpBuf[ sizeof( tmpBuf ) - 1 ] = '\0';

	return operator+=( tmpBuf );
}

CUtlString &CUtlString::operator+=( double rhs )
{
	char tmpBuf[ 256 ];	// How big can doubles be???  Dunno.
	Q_snprintf( tmpBuf, sizeof( tmpBuf ), "%lg", rhs );
	tmpBuf[ sizeof( tmpBuf ) - 1 ] = '\0';

	return operator+=( tmpBuf );
}

int CUtlString::Format( const char *pFormat, ... )
{
	char tmpBuf[ 4096 ];	//< Nice big 4k buffer, as much memory as my first computer had, a Radio Shack Color Computer

	va_list marker;

	va_start( marker, pFormat );
#ifdef _WIN32
	int len = _vsnprintf( tmpBuf, sizeof( tmpBuf ) - 1, pFormat, marker );
#elif defined _LINUX || defined __APPLE__
	int len = vsnprintf( tmpBuf, sizeof( tmpBuf ) - 1, pFormat, marker );
#else
#error "define vsnprintf type."
#endif
	va_end( marker );

	// Len < 0 represents an overflow
	if( len < 0 )
	{
		len = sizeof( tmpBuf ) - 1;
		tmpBuf[sizeof( tmpBuf ) - 1] = 0;
	}

	Set( tmpBuf );

	return len;
}

//-----------------------------------------------------------------------------
// Strips the trailing slash
//-----------------------------------------------------------------------------
void CUtlString::StripTrailingSlash()
{
	if ( IsEmpty() )
		return;

	int nLastChar = Length() - 1;
	char c = m_pString[ nLastChar ];
	if ( c == '\\' || c == '/' )
	{
		m_pString[ nLastChar ] = 0;
	}
}

void CUtlString::Purge()
{
	if(m_pString) {
		free(m_pString);
		m_pString = NULL;
	}
}
