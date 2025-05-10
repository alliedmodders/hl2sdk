//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#ifndef UTLSTRINGMAP_H
#define UTLSTRINGMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "utlsymbol.h"

#define FOR_EACH_STRING_MAP( mapName, iter ) \
	for ( auto iter = (mapName).Head(); iter < (mapName).GetNumStrings() && iter != (mapName).InvalidIndex(); iter = (mapName).Next( iter ) )

template <class T>
class CUtlStringMap
{
public:
	CUtlStringMap( bool caseInsensitive = true, int initsize = 32 ) : 
		  m_Vector( initsize ),
		  m_SymbolTable( 0, initsize, caseInsensitive )
	{
	}

	// Get data by the string itself:
	T& operator[]( const char *pString )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString );
		int index = ( int )symbol;
		if( m_Vector.Count() <= index )
		{
			m_Vector.EnsureCount( index + 1 );
		}
		return m_Vector[index];
	}

	// Get data by the string's symbol table ID - only used to retrieve a pre-existing symbol, not create a new one!
	T& operator[]( UtlSymId_t n )
	{
		Assert( n <= m_Vector.Count() );
		return m_Vector[n];
	}

	const T& operator[]( UtlSymId_t n ) const
	{
		Assert( n <= m_Vector.Count() );
		return m_Vector[n];
	}

	unsigned int Count() const
	{
		Assert( m_Vector.Count() == m_SymbolTable.GetNumStrings() );

		return m_Vector.Count();
	}

	bool Defined( const char *pString ) const
	{
		return m_SymbolTable.Find( pString ).IsValid();
	}

	CUtlSymbol Find( const char *pString ) const
	{
		return m_SymbolTable.Find( pString );
	}

	CUtlSymbol AddString( const char *pString, bool* created = NULL )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString, created );
		int index = ( int )symbol;
		if( m_Vector.Count() <= index )
		{
			m_Vector.EnsureCount( index + 1 );
		}
		return symbol;
	}

	/// Add a string to the map and also insert an item at 
	/// its location in the same operation. Returns the 
	/// newly created index (or the one that was just 
	/// overwritten, if pString already existed.)
	CUtlSymbol Insert( const char *pString, const T &item )
	{
		CUtlSymbol symbol = m_SymbolTable.AddString( pString ); // implicit coercion
		if ( m_Vector.Count() > symbol ) 
		{
			// this string is already in the dictionary.

		}
		else if ( m_Vector.Count() == symbol )
		{
			// this is the expected case when we've added one more to the tail.
			m_Vector.AddToTail( item );
		}
		else // ( m_Vector.Count() < symbol )
		{
			// this is a strange shouldn't-happen case.
			AssertMsg( false, "CUtlStringMap insert unexpected entries." );
			m_Vector.EnsureCount( symbol + 1 );
			m_Vector[symbol] = item;
		}
		return symbol;
	}

	bool FindAndRemove( const char *pString )
	{
		CUtlSymbol symbol = m_SymbolTable.Find( pString );

		if ( !symbol.IsValid() )
		{
			return false;
		}

		Destruct( &m_Vector[ symbol ] );
		m_Vector[ symbol ] = {};
		m_SymbolTable.Remove( symbol );

		return true;
	}

	static CUtlSymbol InvalidIndex()
	{
		return {};
	}

	// iterators (for uniformity with other map types)
	inline CUtlSymbol Head() const
	{
		return m_SymbolTable.GetNumStrings() > 0 ? CUtlSymbol( 0 )  : InvalidIndex();
	}

	inline CUtlSymbol Next( const CUtlSymbol &i ) const
	{
		CUtlSymbol n = i+1;
		return n < m_SymbolTable.GetNumStrings() ? n : InvalidIndex();
	}


	int GetNumStrings( void ) const
	{
		return m_SymbolTable.GetNumStrings();
	}

	const char *String( int n )	const
	{
		return m_SymbolTable.String( n );
	}

	// Clear all of the data from the map
	void Clear()
	{
		m_Vector.RemoveAll();
		m_SymbolTable.RemoveAll();
	}

	void Purge()
	{
		m_Vector.Purge();
		m_SymbolTable.RemoveAll();
	}

	void PurgeAndDeleteElements()
	{
		m_Vector.PurgeAndDeleteElements();
		m_SymbolTable.RemoveAll();
	}



private:
	CUtlVector<T> m_Vector;
	CUtlSymbolTable m_SymbolTable;
};


template< class T >
class CUtlStringMapAutoPurge : public CUtlStringMap < T >
{
public:
	~CUtlStringMapAutoPurge( void )
	{
		this->PurgeAndDeleteElements();
	}

};

#endif // UTLSTRINGMAP_H
