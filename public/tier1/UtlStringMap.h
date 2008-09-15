//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef UTLSTRINGMAP_H
#define UTLSTRINGMAP_H
#ifdef _WIN32
#pragma once
#endif

#include "UtlSymbol.h"

template <class T>
class CUtlStringMap
{
public:
	CUtlStringMap( bool caseInsensitive = true ) : m_Vector( caseInsensitive )
	{
	}

	// Get data by the string itself:
	T& operator[]( const char *pString )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString );
		int index = ( int )( UtlSymId_t )symbol;
		if( m_Vector.Count() <= index )
		{
			m_Vector.EnsureCount( index + 1 );
		}
		return m_Vector[index];
	}

	// Get data by the string's symbol table ID - only used to retrieve a pre-existing symbol, not create a new one!
	T& operator[]( UtlSymId_t n )
	{
		Assert( n >=0 && n <= m_Vector.Count() );
		return m_Vector[index];
	}

	bool Defined( const char *pString )
	{
		return m_SymbolTable.Find( pString ) != UTL_INVAL_SYMBOL;
	}

	int GetNumStrings( void ) const
	{
		return m_SymbolTable.GetNumStrings();
	}

	const char *String( int n )
	{
		return m_SymbolTable.String( n );
	}

private:
	CUtlVector<T> m_Vector;
	CUtlSymbolTable m_SymbolTable;
};

#endif // UTLSTRINGMAP_H
