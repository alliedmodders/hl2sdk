//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple class for performing safe and in-expression sprintf-style
//			string formatting
//
// $NoKeywords: $
//=============================================================================//

#ifndef FMTSTR_H
#define FMTSTR_H

#include <stdarg.h>
#include <stdio.h>
#include "tier0/platform.h"
#include "tier1/strtools.h"

#if defined( _WIN32 )
#pragma once
#endif

//=============================================================================

// using macro to be compatable with GCC
#define FmtStrVSNPrintf( szBuf, nBufSize, ppszFormat ) \
	do \
	{ \
		int     result; \
		va_list arg_ptr; \
	\
		va_start(arg_ptr, (*(ppszFormat))); \
		result = Q_vsnprintf((szBuf), (nBufSize)-1, (*(ppszFormat)), arg_ptr); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
		m_nLength = V_strlen( szBuf ); \
	} \
	while (0)

//-----------------------------------------------------------------------------
//
// Purpose: String formatter with specified size
//

template <int SIZE_BUF>
class CFmtStrN
{
public:
	CFmtStrN()									{ m_szBuf[0] = 0; m_nLength = 0; }
	
	// Standard C formatting
	CFmtStrN(const char *pszFormat, ...)		{ FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat); }

	// Use this for pass-through formatting
	CFmtStrN(const char ** ppszFormat, ...)		{ FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat); }

	// Explicit reformat
	const char *sprintf(const char *pszFormat, ...)	{ FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat); return m_szBuf; }

	// Use this for pass-through formatting
	void VSprintf(const char **ppszFormat, ...)	{ FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat); }

	// Use for access
	operator const char *() const				{ return m_szBuf; }
	char *Access()								{ return m_szBuf; }
	char *Get() { return m_szBuf; }

	void Clear()								{ m_szBuf[0] = 0; m_nLength = 0; }

	void Append( const char *pchValue )
	{
		// This function is close to the metal to cut down on the CPU cost
		// of the previous incantation of Append which was implemented as
		// AppendFormat( "%s", pchValue ). This implementation, though not
		// as easy to read, instead does a strcpy from the existing end
		// point of the CFmtStrN. This brings something like a 10-20x speedup
		// in my rudimentary tests. It isn't using V_strncpy because that
		// function doesn't return the number of characters copied, which
		// we need to adjust m_nLength. Doing the V_strncpy with a V_strlen
		// afterwards took twice as long as this implementations in tests,
		// so V_strncpy's implementation was used to write this method.
		char *pDest = m_szBuf + m_nLength;
		const int maxLen = SIZE_BUF - m_nLength;
		char *pLast = pDest + maxLen - 1;
		while ( (pDest < pLast) && (*pchValue != 0) )
		{
			*pDest = *pchValue;
			++pDest; ++pchValue;
		}
		*pDest = 0;
		m_nLength = pDest - m_szBuf;
	}
	
private:
	char m_szBuf[SIZE_BUF];
	int m_nLength;
};

//-----------------------------------------------------------------------------
//
// Purpose: Default-sized string formatter
//

#define FMTSTR_STD_LEN 256

typedef CFmtStrN<FMTSTR_STD_LEN> CFmtStr;
typedef CFmtStrN<1024> CFmtStr1024;
typedef CFmtStrN<8192> CFmtStrMax;

class CNumStr
{
public:
	CNumStr() { m_szBuf[0] = 0; }

	explicit CNumStr( bool b )		{ SetBool( b ); } 

	explicit CNumStr( int8 n8 )		{ SetInt8( n8 ); }
	explicit CNumStr( uint8 un8 )	{ SetUint8( un8 );  }
	explicit CNumStr( int16 n16 )	{ SetInt16( n16 ); }
	explicit CNumStr( uint16 un16 )	{ SetUint16( un16 );  }
	explicit CNumStr( int32 n32 )	{ SetInt32( n32 ); }
	explicit CNumStr( uint32 un32 )	{ SetUint32( un32 ); }
	explicit CNumStr( int64 n64 )	{ SetInt64( n64 ); }
	explicit CNumStr( uint64 un64 )	{ SetUint64( un64 ); }

#if defined(COMPILER_GCC) && defined(PLATFORM_64BITS)
	explicit CNumStr( lint64 n64 )		{ SetInt64( (int64)n64 ); }
	explicit CNumStr( ulint64 un64 )	{ SetUint64( (uint64)un64 ); }
#endif

	explicit CNumStr( double f )	{ SetDouble( f ); }
	explicit CNumStr( float f )		{ SetFloat( f ); }

	inline void SetBool( bool b )			{ Q_memcpy( m_szBuf, b ? "1" : "0", 2 ); } 

#ifdef _WIN32
	inline void SetInt8( int8 n8 )			{ _itoa( (int32)n8, m_szBuf, 10 ); }
	inline void SetUint8( uint8 un8 )		{ _itoa( (int32)un8, m_szBuf, 10 ); }
	inline void SetInt16( int16 n16 )		{ _itoa( (int32)n16, m_szBuf, 10 ); }
	inline void SetUint16( uint16 un16 )	{ _itoa( (int32)un16, m_szBuf, 10 ); }
	inline void SetInt32( int32 n32 )		{ _itoa( n32, m_szBuf, 10 ); }
	inline void SetUint32( uint32 un32 )	{ _i64toa( (int64)un32, m_szBuf, 10 ); }
	inline void SetInt64( int64 n64 )		{ _i64toa( n64, m_szBuf, 10 ); }
	inline void SetUint64( uint64 un64 )	{ _ui64toa( un64, m_szBuf, 10 ); }
#else
	inline void SetInt8( int8 n8 )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n8 ); }
	inline void SetUint8( uint8 un8 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un8 ); }
	inline void SetInt16( int16 n16 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n16 ); }
	inline void SetUint16( uint16 un16 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un16 ); }
	inline void SetInt32( int32 n32 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", n32 ); }
	inline void SetUint32( uint32 un32 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%u", un32 ); }
	inline void SetInt64( int64 n64 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%lld", n64 ); }
	inline void SetUint64( uint64 un64 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%llu", un64 ); }
#endif

	inline void SetDouble( double f )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }
	inline void SetFloat( float f )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }

	inline void SetHexUint64( uint64 un64 )	{ Q_binarytohex( (byte *)&un64, sizeof( un64 ), m_szBuf, sizeof( m_szBuf ) ); }

	operator const char *() const { return m_szBuf; }
	const char* String() const { return m_szBuf; }
	
	void AddQuotes()
	{
		Assert( m_szBuf[0] != '"' );
		const int nLength = Q_strlen( m_szBuf );
		Q_memmove( m_szBuf + 1, m_szBuf, nLength );
		m_szBuf[0] = '"';
		m_szBuf[nLength + 1] = '"';
		m_szBuf[nLength + 2] = 0;
	}

protected:
	char m_szBuf[28]; // long enough to hold 18 digits of precision, a decimal, a - sign, e+### suffix, and quotes

};

//=============================================================================

#endif // FMTSTR_H
