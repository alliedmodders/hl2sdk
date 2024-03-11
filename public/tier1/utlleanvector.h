//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable array class that maintains a free list and keeps elements
// in the same location
//=============================================================================//

#ifndef UTLLEANVECTOR_H
#define UTLLEANVECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier0/dbg.h"

#include <limits>

#include "tier0/memdbgon.h"

#define FOR_EACH_LEANVEC( vecName, iteratorName ) \
	for ( auto iteratorName = vecName.First(); vecName.IsValidIterator( iteratorName ); iteratorName = vecName.Next( iteratorName ) )

template< class T, class I >
class CUtlLeanVectorBase
{
public:
	// constructor, destructor
	CUtlLeanVectorBase();
	~CUtlLeanVectorBase();
	
	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num, bool force = false );
	
	// Element removal
	void RemoveAll(); // doesn't deallocate memory
	
	// Memory deallocation
	void Purge();

protected:
	I m_nSize;
	I m_nAllocationCount;
	T* m_pMemory;
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, class I >
inline CUtlLeanVectorBase<T, I>::CUtlLeanVectorBase() : m_nSize(0),
	m_nAllocationCount(0), m_pMemory(0)
{
}

template< class T, class I >
inline CUtlLeanVectorBase<T, I>::~CUtlLeanVectorBase()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, class I >
inline T* CUtlLeanVectorBase<T, I>::Base()
{
	return m_nAllocationCount ? m_pMemory : NULL;
}

template< class T, class I >
inline const T* CUtlLeanVectorBase<T, I>::Base() const
{
	return m_nAllocationCount ? m_pMemory : NULL;
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlLeanVectorBase<T, I>::EnsureCapacity( int num, bool force )
{
	I nMinAllocationCount = ( 31 + sizeof( T ) ) / sizeof( T );
	I nMaxAllocationCount = (std::numeric_limits<I>::max)();
	I nNewAllocationCount = m_nAllocationCount;
	
	if ( force )
	{
		if ( num == m_nAllocationCount )
			return;
	}
	else
	{
		if ( num <= m_nAllocationCount )
			return;
	}
	
	if ( num > nMaxAllocationCount )
	{
		Msg( "%s allocation count overflow( %llu > %llu )\n", __FUNCTION__, ( uint64 )num, ( uint64 )nMaxAllocationCount );
		Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
		DebuggerBreak();
	}
	
	if ( force )
	{
		nNewAllocationCount = num;
	}
	else
	{
		while ( nNewAllocationCount < num )
		{
			if ( nNewAllocationCount < nMaxAllocationCount/2 )
				nNewAllocationCount = MAX( nNewAllocationCount*2, nMinAllocationCount );
			else
				nNewAllocationCount = nMaxAllocationCount;
		}
	}
	
	m_pMemory = (T*)realloc( m_pMemory, nNewAllocationCount * sizeof(T) );
	m_nAllocationCount = nNewAllocationCount;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlLeanVectorBase<T, I>::RemoveAll()
{
	T* pElement = Base();
	const T* pEnd = &pElement[ m_nSize ];
	while ( pElement != pEnd )
		Destruct( pElement++ );

	m_nSize = 0;
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class I >
inline void CUtlLeanVectorBase<T, I>::Purge()
{
	RemoveAll();
	
	if ( m_nAllocationCount > 0 )
	{
		free( (void*)m_pMemory );
		m_pMemory = 0;
	}
	
	m_nAllocationCount = 0;
}

template< class T, size_t N, class I >
class CUtlLeanVectorFixedGrowableBase
{
public:
	// constructor, destructor
	CUtlLeanVectorFixedGrowableBase();
	~CUtlLeanVectorFixedGrowableBase();
	
	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num, bool force = false );
	
	// Element removal
	void RemoveAll(); // doesn't deallocate memory
	
	// Memory deallocation
	void Purge();

protected:
	I m_nSize;
	I m_nAllocationCount;
	
	union
	{
		T* m_pMemory;
		T m_Memory[ N ];
	};
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, size_t N, class I >
inline CUtlLeanVectorFixedGrowableBase<T, N, I>::CUtlLeanVectorFixedGrowableBase() : m_nSize(0),
	m_nAllocationCount(N)
{
}

template< class T, size_t N, class I >
inline CUtlLeanVectorFixedGrowableBase<T, N, I>::~CUtlLeanVectorFixedGrowableBase()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, size_t N, class I >
inline T* CUtlLeanVectorFixedGrowableBase<T, N, I>::Base()
{
	if ( m_nAllocationCount )
	{
		if ( ( size_t )m_nAllocationCount > N )
			return m_pMemory;
		else
			return &m_Memory[ 0 ];
	}
	
	return NULL;
}

template< class T, size_t N, class I >
inline const T* CUtlLeanVectorFixedGrowableBase<T, N, I>::Base() const
{
	if ( m_nAllocationCount )
	{
		if ( ( size_t )m_nAllocationCount > N )
			return m_pMemory;
		else
			return &m_Memory[ 0 ];
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, size_t N, class I >
void CUtlLeanVectorFixedGrowableBase<T, N, I>::EnsureCapacity( int num, bool force )
{
	I nMinAllocationCount = ( 31 + sizeof( T ) ) / sizeof( T );
	I nMaxAllocationCount = (std::numeric_limits<I>::max)();
	I nNewAllocationCount = m_nAllocationCount;
	
	if ( num <= m_nAllocationCount )
		return;
	
	if ( ( size_t )num > N )
	{
		if ( num > nMaxAllocationCount )
		{
			Msg( "%s allocation count overflow( %llu > %llu )\n", __FUNCTION__, ( uint64 )num, ( uint64 )nMaxAllocationCount );
			Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
			DebuggerBreak();
		}
		
		if ( force )
		{
			nNewAllocationCount = num;
		}
		else
		{
			while ( nNewAllocationCount < num )
			{
				if ( nNewAllocationCount < nMaxAllocationCount/2 )
					nNewAllocationCount = MAX( nNewAllocationCount*2, nMinAllocationCount );
				else
					nNewAllocationCount = nMaxAllocationCount;
			}
		}
	}
	else
	{
		nNewAllocationCount = num;
	}
	
	if ( ( size_t )m_nAllocationCount > N )
	{
		m_pMemory = (T*)realloc( m_pMemory, nNewAllocationCount * sizeof(T) );
	}
	else if ( ( size_t )nNewAllocationCount > N )
	{
		T* pNew = (T*)malloc( nNewAllocationCount * sizeof(T) );
		memcpy( pNew, Base(), m_nSize * sizeof(T) );
		m_pMemory = pNew;
	}
	
	m_nAllocationCount = nNewAllocationCount;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< class T, size_t N, class I >
void CUtlLeanVectorFixedGrowableBase<T, N, I>::RemoveAll()
{
	T* pElement = Base();
	const T* pEnd = &pElement[ m_nSize ];
	while ( pElement != pEnd )
		Destruct( pElement++ );

	m_nSize = 0;
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, size_t N, class I >
inline void CUtlLeanVectorFixedGrowableBase<T, N, I>::Purge()
{
	RemoveAll();
	
	if ( ( size_t )m_nAllocationCount > N )
		free( (void*)m_pMemory );
	
	m_nAllocationCount = N;
}

template< class B, class T, class I >
class CUtlLeanVectorImpl : public B
{
public:
	// constructor, destructor
	CUtlLeanVectorImpl() {};
	~CUtlLeanVectorImpl() {};

	// Copy the array.
	CUtlLeanVectorImpl<B, T, I>& operator=( const CUtlLeanVectorImpl<B, T, I> &other );

	class Iterator_t
	{
	public:
		Iterator_t( const T* _elem, const T* _end ) : elem( _elem ), end( _end ) {}
		const T* elem;
		const T* end;
		bool operator==( const Iterator_t it ) const	{ return elem == it.elem && end == it.end; }
		bool operator!=( const Iterator_t it ) const	{ return elem != it.elem || end != it.end; }
	};
	Iterator_t First() const							{ const T* base = this->Base(); return Iterator_t( base, &base[ this->m_nSize ] ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( it.elem + 1, it.end ); }
	bool IsValidIterator( const Iterator_t &it ) const	{ return it.elem != it.end; }
	T& operator[]( const Iterator_t &it )				{ return *const_cast<T*>(it.elem); }
	const T& operator[]( const Iterator_t &it ) const	{ return *it.elem; }

	// element access
	T& operator[]( int i );
	const T& operator[]( int i ) const;
	T& Element( int i );
	const T& Element( int i ) const;
	T& Head();
	const T& Head() const;
	T& Tail();
	const T& Tail() const;
	
	// Returns the number of elements in the vector
	int Count() const;

	// Is element index valid?
	bool IsValidIndex( int i ) const;
	static int InvalidIndex();

	// Adds an element, uses default constructor
	T* AddToTailGetPtr();

	// Adds an element, uses copy constructor
	int AddToTail( const T& src );

	// Adds multiple elements, uses default constructor
	int AddMultipleToTail( int nSize );	

	// Adds multiple elements, uses default constructor
	T* InsertBeforeGetPtr( int nBeforeIndex, int nSize = 1 );

	void SetSize( int size );
	void SetCount( int count );

	// Finds an element (element needs operator== defined)
	int Find( const T& src ) const;

	// Element removal
	void FastRemove( int elem ); // doesn't preserve order
	void Remove( int elem ); // preserves order, shifts elements
	bool FindAndRemove( const T& src );	// removes first occurrence of src, preserves order, shifts elements
	bool FindAndFastRemove( const T& src );	// removes first occurrence of src, doesn't preserve order
	void RemoveMultiple( int elem, int num ); // preserves order, shifts elements

protected:
	// Can't copy this unless we explicitly do it!
	CUtlLeanVectorImpl( CUtlLeanVectorImpl const& vec ) { Assert(0); }

	// Shifts elements....
	void ShiftElements( T* pDest, const T* pSrc, const T* pSrcEnd );

	// construct, destruct elements
	void ConstructElements( T* pElement, const T* pEnd );
	void DestructElements( T* pElement, const T* pEnd );
};

template< class B, class T, class I >
inline CUtlLeanVectorImpl<B, T, I>& CUtlLeanVectorImpl<B, T, I>::operator=( const CUtlLeanVectorImpl<B, T, I> &other )
{
	int nCount = other.Count();
	SetSize( nCount );

	T* pDest = this->Base();
	const T* pSrc = other.Base();
	const T* pEnd = &pSrc[ nCount ];

	while ( pSrc != pEnd )
		*(pDest++) = *(pSrc++);

	return *this;
}

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::operator[]( int i )
{
	Assert( i < this->m_nSize );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::operator[]( int i ) const
{
	Assert( i < this->m_nSize );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Element( int i )
{
	Assert( i < this->m_nSize );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Element( int i ) const
{
	Assert( i < this->m_nSize );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Head()
{
	Assert( this->m_nSize > 0 );
	return this->Base()[ 0 ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Head() const
{
	Assert( this->m_nSize > 0 );
	return this->Base()[ 0 ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Tail()
{
	Assert( this->m_nSize > 0 );
	return this->Base()[ this->m_nSize - 1 ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Tail() const
{
	Assert( this->m_nSize > 0 );
	return this->Base()[ this->m_nSize - 1 ];
}

//-----------------------------------------------------------------------------
// Count
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline int CUtlLeanVectorImpl<B, T, I>::Count() const
{
	return this->m_nSize;
}

//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline bool CUtlLeanVectorImpl<B, T, I>::IsValidIndex( int i ) const
{
	return (i >= 0) && (i < this->m_nSize);
}

//-----------------------------------------------------------------------------
// Returns in invalid index
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline int CUtlLeanVectorImpl<B, T, I>::InvalidIndex()
{
	return -1;
}

//-----------------------------------------------------------------------------
// Adds an element, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
T* CUtlLeanVectorImpl<B, T, I>::AddToTailGetPtr()
{
	this->EnsureCapacity( this->m_nSize + 1 );
	T* pBase = this->Base();
	Construct( &pBase[ this->m_nSize ] );
	return &pBase[ this->m_nSize++ ];
}

//-----------------------------------------------------------------------------
// Adds an element, uses copy constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::AddToTail( const T& src )
{
	this->EnsureCapacity( this->m_nSize + 1 );
	T* pBase = this->Base();
	CopyConstruct( &pBase[ this->m_nSize ], src );
	return this->m_nSize++;
}

//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::AddMultipleToTail( int nSize )
{
	int nOldSize = this->m_nSize;

	if ( nSize > 0 )
	{
		int nMaxSize = (std::numeric_limits<I>::max)();

		if ( ( nMaxSize - nOldSize ) < nSize )
		{
			Msg( "%s allocation count overflow( add %llu + current %llu > max %llu )\n", __FUNCTION__, ( uint64 )nSize, ( uint64 )nOldSize, ( uint64 )nMaxSize );
			Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
			DebuggerBreak();
		}

		int nNewSize = nOldSize + nSize;

		this->EnsureCapacity( nNewSize );

		T* pBase = this->Base();

		ConstructElements( &pBase[ nOldSize ], &pBase[ nNewSize ] );

		this->m_nSize = nNewSize;
	}

	return nOldSize;
}

//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
T* CUtlLeanVectorImpl<B, T, I>::InsertBeforeGetPtr( int nBeforeIndex, int nSize )
{
	int nOldSize = this->m_nSize;

	if ( nBeforeIndex < 0 || nBeforeIndex > nOldSize )
	{
		Plat_FatalErrorFunc( "%s: invalid nBeforeIndex %d\n", __FUNCTION__, nBeforeIndex );
		DebuggerBreak();
	}

	if ( nSize <= 0 )
	{
		Plat_FatalErrorFunc( "%s: invalid nSize %d\n", __FUNCTION__, nSize );
		DebuggerBreak();
	}

	int nMaxSize = (std::numeric_limits<I>::max)();

	if ( ( nMaxSize - nOldSize ) < nSize )
	{
		Msg( "%s allocation count overflow( add %llu + current %llu > max %llu )\n", __FUNCTION__, ( uint64 )nSize, ( uint64 )nOldSize, ( uint64 )nMaxSize );
		Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
		DebuggerBreak();
	}

	int nNewSize = nOldSize + nSize;

	this->EnsureCapacity( nNewSize );

	T* pBase = this->Base();

	ShiftElements( &pBase[ nBeforeIndex + nSize ], &pBase[ nBeforeIndex ], &pBase[ nOldSize ] );
	ConstructElements( &pBase[ nBeforeIndex ], &pBase[ nBeforeIndex + nSize ] );

	this->m_nSize = nNewSize;

	return &pBase[ nBeforeIndex ];
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::SetCount( int count )
{
	this->EnsureCapacity( count );

	T* pBase = this->Base();

	if ( this->m_nSize < count )
		ConstructElements( &pBase[ this->m_nSize ], &pBase[ count ] );
	else if ( this->m_nSize > count )
		DestructElements( &pBase[ count ], &pBase[ this->m_nSize ] );

	this->m_nSize = count;
}

template< class B, class T, class I >
inline void CUtlLeanVectorImpl<B, T, I>::SetSize( int size )
{
	SetCount( size );
}

//-----------------------------------------------------------------------------
// Finds an element (element needs operator== defined)
//-----------------------------------------------------------------------------
template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::Find( const T& src ) const
{
	const T* pBase = this->Base();
	for ( int i = 0; i < Count(); ++i )
	{
		if ( pBase[i] == src )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::FastRemove( int elem )
{
	Assert( IsValidIndex(elem) );

	T* pBase = this->Base();
	Destruct( &pBase[ elem ] );
	if ( this->m_nSize > 0 )
	{
		if ( elem != this->m_nSize - 1 )
			memcpy( &pBase[ elem ], &pBase[ this->m_nSize - 1 ], sizeof( T ) );
		--this->m_nSize;
	}
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::Remove( int elem )
{
	T* pBase = this->Base();
	Destruct( &pBase[ elem ] );
	ShiftElements( &pBase[ elem ], &pBase[ elem + 1 ], &pBase[ this->m_nSize ] );
	--this->m_nSize;
}

template< class B, class T, class I >
bool CUtlLeanVectorImpl<B, T, I>::FindAndRemove( const T& src )
{
	int elem = Find( src );
	if ( elem != -1 )
	{
		Remove( elem );
		return true;
	}
	return false;
}

template< class B, class T, class I >
bool CUtlLeanVectorImpl<B, T, I>::FindAndFastRemove( const T& src )
{
	int elem = Find( src );
	if ( elem != -1 )
	{
		FastRemove( elem );
		return true;
	}
	return false;
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::RemoveMultiple( int elem, int num )
{
	Assert( elem >= 0 );
	Assert( elem + num <= Count() );

	T* pBase = this->Base();
	DestructElements( &pBase[ elem ], &pBase[ elem + num ] );
	ShiftElements( &pBase[ elem ], &pBase[ elem + num ], &pBase[ this->m_nSize ] );
	this->m_nSize -= num;
}

//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::ShiftElements( T* pDest, const T* pSrc, const T* pSrcEnd )
{
	ptrdiff_t numToMove = pSrcEnd - pSrc;
	if ( numToMove > 0 )
		memmove( pDest, pSrc, numToMove * sizeof( T ) );
}

//-----------------------------------------------------------------------------
// construct, destruct elements
//-----------------------------------------------------------------------------
template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::ConstructElements( T* pElement, const T* pEnd )
{
	while ( pElement < pEnd )
		Construct( pElement++ );
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::DestructElements( T* pElement, const T* pEnd )
{
	while ( pElement < pEnd )
		Destruct( pElement++ );
}

template < class T, class I = short >
using CUtlLeanVector = CUtlLeanVectorImpl< CUtlLeanVectorBase< T, I >, T, I >;

template < class T, size_t N = 3, class I = short >
using CUtlLeanVectorFixedGrowable = CUtlLeanVectorImpl< CUtlLeanVectorFixedGrowableBase< T, N, I >, T, I >;

#include "tier0/memdbgoff.h"

#endif // UTLLEANVECTOR_H
