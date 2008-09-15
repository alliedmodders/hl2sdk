//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// $Header: $
// $NoKeywords: $
//
// A growable array class that keeps all elements in order using binary search
//===========================================================================//

#ifndef UTLSORTVECTOR_H
#define UTLSORTVECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"


//-----------------------------------------------------------------------------
// class CUtlSortVector:
// description:
//   This in an sorted order-preserving vector. Items may be inserted or removed
//   at any point in the vector. When an item is inserted, all elements are
//   moved down by one element using memmove. When an item is removed, all 
//   elements are shifted back down. Items are searched for in the vector
//   using a binary search technique. Clients must pass in a Less() function
//   into the constructor of the vector to determine the sort order.
//-----------------------------------------------------------------------------

template <class T, class LessFunc>
class CUtlSortVector : public CUtlVector<T>
{
public:

	// constructor
	CUtlSortVector( int nGrowSize = 0, int initSize = 0 );
	CUtlSortVector( T* pMemory, int numElements );
	
	// inserts (copy constructs) an element in sorted order into the list
	int		Insert( const T& src );
	
	// Finds an element within the list using a binary search
	int		Find( const T& search ) const;
	int		FindLessOrEqual( const T& search ) const;
	
	// Removes a particular element
	void	Remove( const T& search );
	void	Remove( int i );
	
	// Allows methods to set a context to be used with the less function..
	void	SetLessContext( void *pCtx );

	// Note that you can only use this index until sorting is redone!!!
	int		InsertNoSort( const T& src );
	void	RedoSort( bool bForceSort = false );

protected:
	// No copy constructor
	CUtlSortVector( const CUtlSortVector<T, LessFunc> & );

	// never call these; illegal for this class
	int AddToHead();
	int AddToTail();
	int InsertBefore( int elem );
	int InsertAfter( int elem );

	// Adds an element, uses copy constructor
	int AddToHead( const T& src );
	int AddToTail( const T& src );
	int InsertBefore( int elem, const T& src );
	int InsertAfter( int elem, const T& src );

	// Adds multiple elements, uses defaulconst Tructor
	int AddMultipleToHead( int num );
	int AddMultipleToTail( int num, const T *pToCopy=NULL );	   
	int InsertMultipleBefore( int elem, int num, const T *pToCopy=NULL );
	int InsertMultipleAfter( int elem, int num );
	
	// Add the specified array to the tail.
	int AddVectorToTail( CUtlVector<T> const &src );

	void *m_pLessContext;
	bool	m_bNeedsSort;

private:
    void Swap( int L, int R );
	void QuickSort( LessFunc& less, int X, int I );
	int SplitList( LessFunc& less, int nLower, int nUpper );
};


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
CUtlSortVector<T, LessFunc>::CUtlSortVector( int nGrowSize, int initSize ) : 
	m_pLessContext(NULL), CUtlVector<T>( nGrowSize, initSize ), m_bNeedsSort( false )
{
}

template <class T, class LessFunc> 
CUtlSortVector<T, LessFunc>::CUtlSortVector( T* pMemory, int numElements ) :
	m_pLessContext(NULL), CUtlVector<T>( pMemory, numElements ), m_bNeedsSort( false )
{
}

//-----------------------------------------------------------------------------
// Allows methods to set a context to be used with the less function..
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::SetLessContext( void *pCtx )
{
	m_pLessContext = pCtx;
}

//-----------------------------------------------------------------------------
// grows the vector
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
int CUtlSortVector<T, LessFunc>::Insert( const T& src )
{
	AssertFatal( !m_bNeedsSort );

	int pos = FindLessOrEqual( src ) + 1;
	GrowVector();
	ShiftElementsRight(pos);
	CopyConstruct<T>( &Element(pos), src );
	return pos;
}

template <class T, class LessFunc> 
int CUtlSortVector<T, LessFunc>::InsertNoSort( const T& src )
{
	m_bNeedsSort = true;
	int lastElement = CUtlVector<T>::m_Size;
	// Just stick the new element at the end of the vector, but don't do a sort
	GrowVector();
	ShiftElementsRight(lastElement);
	CopyConstruct( &Element(lastElement), src );
	return lastElement;
}

template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::Swap( int L, int R )
{
	T temp = Element( L );
	Element( L ) = Element( R );
	Element( R ) = temp;
}

template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::QuickSort( LessFunc& less, int nLower, int nUpper )
{
    if ( nLower < nUpper )
    {
        int nSplit = SplitList( less, nLower, nUpper );
        QuickSort( less, nLower, nSplit - 1 );
        QuickSort( less, nSplit + 1, nUpper );
    }
}

template <class T, class LessFunc> 
int CUtlSortVector<T, LessFunc>::SplitList( LessFunc& less, int nLower, int nUpper )
{
    int nLeft = nLower + 1;
    int nRight = nUpper;

	const T& val = Element( nLower );

	while ( nLeft <= nRight )
    {
        while ( nLeft <= nRight && !less.Less( val, Element( nLeft ), m_pLessContext ) )
            ++nLeft;
        while ( nLeft <= nRight && !less.Less( Element( nRight ), val, m_pLessContext ) )
            --nRight;

        if ( nLeft < nRight )
        {
			Swap( nLeft, nRight );
            ++nLeft;
            --nRight;
        }
    }

	Swap( nLower, nRight );
    return nRight;
}

template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::RedoSort( bool bForceSort /*= false */ )
{
	if ( !m_bNeedsSort && !bForceSort )
		return;

	m_bNeedsSort = false;
	LessFunc less;
	QuickSort( less, 0, Count() - 1 );
}

//-----------------------------------------------------------------------------
// finds a particular element
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
int CUtlSortVector<T, LessFunc>::Find( const T& src ) const
{
	AssertFatal( !m_bNeedsSort );

	LessFunc less;

	int start = 0, end = Count() - 1;
	while (start <= end)
	{
		int mid = (start + end) >> 1;
		if ( less.Less( Element(mid), src, m_pLessContext ) )
		{
			start = mid + 1;
		}
		else if ( less.Less( src, Element(mid), m_pLessContext ) )
		{
			end = mid - 1;
		}
		else
		{
			return mid;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// finds a particular element
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
int CUtlSortVector<T, LessFunc>::FindLessOrEqual( const T& src ) const
{
	AssertFatal( !m_bNeedsSort );

	LessFunc less;
	int start = 0, end = Count() - 1;
	while (start <= end)
	{
		int mid = (start + end) >> 1;
		if ( less.Less( Element(mid), src, m_pLessContext ) )
		{
			start = mid + 1;
		}
		else if ( less.Less( src, Element(mid), m_pLessContext ) )
		{
			end = mid - 1;
		}
		else
		{
			return mid;
		}
	}
	return end;
}


//-----------------------------------------------------------------------------
// Removes a particular element
//-----------------------------------------------------------------------------
template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::Remove( const T& search )
{
	AssertFatal( !m_bNeedsSort );

	int pos = Find(search);
	if (pos != -1)
	{
		CUtlVector<T>::Remove(pos);
	}
}

template <class T, class LessFunc> 
void CUtlSortVector<T, LessFunc>::Remove( int i )
{
	CUtlVector<T>::Remove( i );
}

#endif // UTLSORTVECTOR_H
