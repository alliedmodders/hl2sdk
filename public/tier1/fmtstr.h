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
#define FmtStrVSNPrintf( szBuf, nBufSize, ppszFormat, lastArg ) \
	do \
	{ \
		int     result; \
		va_list arg_ptr; \
	\
		va_start(arg_ptr, lastArg); \
		result = Q_vsnprintf((szBuf), (nBufSize)-1, (*(ppszFormat)), arg_ptr); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
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
	CFmtStrN()									{ m_szBuf[0] = 0; }
	
	// Standard C formatting
	CFmtStrN(const char *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat, pszFormat);
	}

	// Use this for pass-through formatting
	CFmtStrN(const char ** ppszFormat, ...)
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat, ppszFormat);
	}

	// Explicit reformat
	const char *sprintf(const char *pszFormat, ...)	FMTFUNCTION( 2, 3 )
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, &pszFormat, pszFormat); 
		return m_szBuf;
	}

	// Use this for pass-through formatting
	void VSprintf(const char **ppszFormat, ...)
	{
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, ppszFormat, ppszFormat);
	}

	// Use for access
	operator const char *() const				{ return m_szBuf; }
	char *Access()								{ return m_szBuf; }
	CFmtStrN<SIZE_BUF> & operator=( const char *pchValue ) { sprintf( pchValue ); return *this; }
	CFmtStrN<SIZE_BUF> & operator+=( const char *pchValue ) { Append( pchValue ); return *this; }
	int Length() const { return V_strlen( m_szBuf ); }

	void Clear()								{ m_szBuf[0] = 0; }

	void AppendFormat( const char *pchFormat, ... ) { int nLength = Length(); char *pchEnd = m_szBuf + nLength; FmtStrVSNPrintf( pchEnd, SIZE_BUF - nLength, &pchFormat, pchFormat ); }
	void AppendFormatV( const char *pchFormat, va_list args );
	void Append( const char *pchValue ) { AppendFormat( pchValue ); }

	void AppendIndent( uint32 unCount, char chIndent = '\t' );
private:
	char m_szBuf[SIZE_BUF];
};


template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendIndent( uint32 unCount, char chIndent )
{
	int nLength = Length();
	if( nLength + unCount >= SIZE_BUF )
		unCount = SIZE_BUF - (1+nLength);
	for ( uint32 x = 0; x < unCount; x++ )
	{
		m_szBuf[ nLength++ ] = chIndent;
	}
	m_szBuf[ nLength ] = '\0';
}

template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendFormatV( const char *pchFormat, va_list args )
{
	int nLength = Length();
	V_vsnprintf( m_szBuf+nLength, SIZE_BUF - nLength, pchFormat, args );
}


//-----------------------------------------------------------------------------
//
// Purpose: Default-sized string formatter
//

#define FMTSTR_STD_LEN 256

typedef CFmtStrN<FMTSTR_STD_LEN> CFmtStr;
typedef CFmtStrN<1024> CFmtStr1024;
typedef CFmtStrN<8192> CFmtStrMax;

//=============================================================================

#endif // FMTSTR_H
