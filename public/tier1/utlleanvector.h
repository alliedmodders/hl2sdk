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

#include "commonmacros.h"
#include "rawallocator.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"

#include <limits>

#define FOR_EACH_LEANVEC( vecName, iteratorName ) \
	for ( auto iteratorName = vecName.First(); vecName.IsValidIterator( iteratorName ); iteratorName = vecName.Next( iteratorName ) )

template< class T, class I, class A >
class CUtlLeanVectorBase
{
	typedef A CAllocator;

public:
	enum : I
	{
		EXTERNAL_BUFFER_MARKER = (I { 1 } << (std::numeric_limits<I>::digits - 1))
	};

	// constructor, destructor
	CUtlLeanVectorBase( I growSize = 0, I initSize = 0 );
	CUtlLeanVectorBase( T *pMemory, I allocationCount, I numElements = 0 );
	~CUtlLeanVectorBase();
	
	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	void Swap( CUtlLeanVectorBase<T, I, A> &mem );

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num, bool force = false );
	
	bool IsExternallyAllocated() const { return (m_nAllocated & EXTERNAL_BUFFER_MARKER) != 0; }

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T *pMemory, int allocationCount, int numElements = 0 );
	void SetExternalBuffer( const T *pMemory, int allocationCount, int numElements = 0 );
	void AssumeMemory( T *pMemory, int allocationCount, int numElements = 0 );
	T *Detach();
	void *DetachMemory();

	int NumAllocated() const { return (m_nAllocated & (~EXTERNAL_BUFFER_MARKER)); }

	// Element removal
	void RemoveAll(); // doesn't deallocate memory

	bool IsIdxValid( I i ) const { return (i >= 0) && (i < NumAllocated()); }
	
	// Memory deallocation
	void Purge();
protected:

	struct
	{
		I m_nCount;
		I m_nAllocated;
		T* m_pElements;
	};
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, class I, class A >
inline CUtlLeanVectorBase<T, I, A>::CUtlLeanVectorBase( I growSize, I initSize ) :
	m_nCount( 0 ), m_nAllocated( 0 ), m_pElements( nullptr )
{
	EnsureCapacity( initSize );
}
	
template< class T, class I, class A >
inline CUtlLeanVectorBase<T, I, A>::CUtlLeanVectorBase( T *pMemory, I allocationCount, I numElements ) :
	m_nCount( numElements ), m_nAllocated( allocationCount | EXTERNAL_BUFFER_MARKER ), m_pElements( pMemory )
{
}

template< class T, class I, class A >
inline CUtlLeanVectorBase<T, I, A>::~CUtlLeanVectorBase()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, class I, class A >
inline T* CUtlLeanVectorBase<T, I, A>::Base()
{
	return NumAllocated() ? m_pElements : nullptr;
}

template< class T, class I, class A >
inline const T* CUtlLeanVectorBase<T, I, A>::Base() const
{
	return NumAllocated() ? m_pElements : nullptr;
}

//-----------------------------------------------------------------------------
// Fast swap
//-----------------------------------------------------------------------------
template< class T, class I, class A >
void CUtlLeanVectorBase<T, I, A>::Swap( CUtlLeanVectorBase<T, I, A> &vec )
{
	V_swap( m_nCount, vec.m_nCount );
	V_swap( m_nAllocated, vec.m_nAllocated );
	V_swap( m_pElements, vec.m_pElements );
}

//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, class I, class A >
inline void CUtlLeanVectorBase<T, I, A>::SetExternalBuffer( T *pMemory, int allocationCount, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_nCount = numElements;
	m_nAllocated = allocationCount | EXTERNAL_BUFFER_MARKER;
	m_pElements = pMemory;
}

template< class T, class I, class A >
inline void CUtlLeanVectorBase<T, I, A>::SetExternalBuffer( const T *pMemory, int allocationCount, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_nCount = numElements;
	m_nAllocated = allocationCount | EXTERNAL_BUFFER_MARKER;
	m_pElements = const_cast<T *>(pMemory);
}

template< class T, class I, class A >
inline void CUtlLeanVectorBase<T, I, A>::AssumeMemory( T *pMemory, int allocationCount, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_nCount = numElements;
	m_nAllocated = allocationCount;

	// Simply take the pointer but don't mark us as external
	m_pElements = pMemory;
}

template< class T, class I, class A >
inline T *CUtlLeanVectorBase<T, I, A>::Detach()
{
	return (T *)DetachMemory();
}

template< class T, class I, class A >
inline void *CUtlLeanVectorBase<T, I, A>::DetachMemory()
{
	if(IsExternallyAllocated())
		return nullptr;

	void *pMemory = m_pElements;
	m_nCount = 0;
	m_nAllocated = 0;

	m_pElements = 0;
	return pMemory;
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, class I, class A >
void CUtlLeanVectorBase<T, I, A>::EnsureCapacity( int num, bool force )
{
	if(num <= NumAllocated())
		return;
	
	I nMinAllocated = (31 + sizeof( T )) / sizeof( T );
	I nMaxAllocated = (std::numeric_limits<I>::max)();

	if ( num > nMaxAllocated )
	{
		Msg( "%s allocation count overflow( %llu > %llu )\n", __FUNCTION__, ( uint64 )num, ( uint64 )nMaxAllocated );
		Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
		DebuggerBreak();
	}
	
	I nNewAllocated = num;
	if ( !force )
		nNewAllocated = CalcNewDoublingCount( NumAllocated(), num, nMinAllocated, nMaxAllocated );
	
	T *pNew = nullptr;
	if(IsExternallyAllocated())
	{
		pNew = CAllocator::template Alloc<T>( nNewAllocated, nNewAllocated );
		V_memmove( pNew, Base(), m_nCount * sizeof( T ) );
	}
	else
	{
		pNew = CAllocator::Realloc( m_pElements, nNewAllocated, nNewAllocated );
	}

	m_pElements = pNew;
	m_nAllocated = nNewAllocated;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< class T, class I, class A >
void CUtlLeanVectorBase<T, I, A>::RemoveAll()
{
	T* pElement = Base();
	const T* pEnd = &pElement[ m_nCount ];
	while ( pElement != pEnd )
		Destruct( pElement++ );

	m_nCount = 0;
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class I, class A >
inline void CUtlLeanVectorBase<T, I, A>::Purge()
{
	RemoveAll();
	
	if(!IsExternallyAllocated())
	{
		if(NumAllocated() > 0)
		{
			CAllocator::Free( m_pElements );
			m_pElements = nullptr;
		}

		m_nAllocated = 0;
	}
}

template< class T, size_t N, class I, class A >
class CUtlLeanVectorFixedGrowableBase
{
	typedef A CAllocator;

public:
	enum : I
	{
		EXTERNAL_BUFFER_MARKER = (I { 1 } << (std::numeric_limits<I>::digits - 1))
	};

	// constructor, destructor
	CUtlLeanVectorFixedGrowableBase( I growSize = 0, I initSize = 0 );
	CUtlLeanVectorFixedGrowableBase( T *pMemory, I allocationCount, I numElements = 0 );
	~CUtlLeanVectorFixedGrowableBase();
	
	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( int num, bool force = false );
	
	bool IsExternallyAllocated() const { return (m_nAllocated & EXTERNAL_BUFFER_MARKER) != 0; }

	int NumAllocated() const { return (m_nAllocated & (~EXTERNAL_BUFFER_MARKER)); }

	// Element removal
	void RemoveAll(); // doesn't deallocate memory
	
	bool IsIdxValid( I i ) const { return (i >= 0) && (i < NumAllocated()); }

	// Memory deallocation
	void Purge();

protected:

	union
	{
		struct
		{
			I m_nCount;
			I m_nAllocated;
		};
		
		struct
		{
			I m_nFixedCount;
			I m_nFixedAllocated;
			T m_FixedAlloc[ N ];
		};
		
		struct
		{
			I m_nAllocCount;
			I m_nAllocAllocated;
			T* m_pElements;
		};
	};
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, size_t N, class I, class A >
inline CUtlLeanVectorFixedGrowableBase<T, N, I, A>::CUtlLeanVectorFixedGrowableBase( I growSize, I initSize ) :
	m_nCount( 0 ), m_nAllocated( N )
{
	EnsureCapacity( initSize );
}

template< class T, size_t N, class I, class A >
inline CUtlLeanVectorFixedGrowableBase<T, N, I, A>::CUtlLeanVectorFixedGrowableBase( T *pMemory, I allocationCount, I numElements ) :
	m_nAllocCount( numElements ), m_nAllocAllocated( allocationCount | EXTERNAL_BUFFER_MARKER ), m_pElements( pMemory )
{
}

template< class T, size_t N, class I, class A >
inline CUtlLeanVectorFixedGrowableBase<T, N, I, A>::~CUtlLeanVectorFixedGrowableBase()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, size_t N, class I, class A >
inline T* CUtlLeanVectorFixedGrowableBase<T, N, I, A>::Base()
{
	if ( NumAllocated() )
	{
		if ( IsExternallyAllocated() || ( size_t )NumAllocated() > N )
			return m_pElements;
		else
			return &m_FixedAlloc[ 0 ];
	}
	
	return nullptr;
}

template< class T, size_t N, class I, class A >
inline const T* CUtlLeanVectorFixedGrowableBase<T, N, I, A>::Base() const
{
	if ( NumAllocated() )
	{
		if ( IsExternallyAllocated() || ( size_t )NumAllocated() > N )
			return m_pElements;
		else
			return &m_FixedAlloc[ 0 ];
	}
	
	return nullptr;
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, size_t N, class I, class A >
void CUtlLeanVectorFixedGrowableBase<T, N, I, A>::EnsureCapacity( int num, bool force )
{
	if ( num <= NumAllocated() )
		return;
	
	I nMinAllocated = (31 + sizeof( T )) / sizeof( T );
	I nMaxAllocated = (std::numeric_limits<I>::max)();
	I nNewAllocated = num;

	if ( ( size_t )num > N )
	{
		if ( num > nMaxAllocated )
		{
			Msg( "%s allocation count overflow( %llu > %llu )\n", __FUNCTION__, ( uint64 )num, ( uint64 )nMaxAllocated );
			Plat_FatalErrorFunc( "%s allocation count overflow", __FUNCTION__ );
			DebuggerBreak();
		}
	}

	if(!force)
		nNewAllocated = CalcNewDoublingCount( NumAllocated(), num, nMinAllocated, nMaxAllocated );
	
	T *pNew = nullptr;
	if(!IsExternallyAllocated() && (size_t)NumAllocated() > N)
	{
		pNew = CAllocator::Realloc( m_pElements, nNewAllocated, nNewAllocated );
	}
	else
	{
		pNew = CAllocator::template Alloc<T>( nNewAllocated, nNewAllocated );
		V_memmove( pNew, Base(), m_nCount * sizeof( T ) );
	}
	
	m_pElements = pNew;
	m_nAllocated = nNewAllocated;
}

//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< class T, size_t N, class I, class A >
void CUtlLeanVectorFixedGrowableBase<T, N, I, A>::RemoveAll()
{
	T* pElement = Base();
	const T* pEnd = &pElement[ m_nCount ];
	while ( pElement != pEnd )
		Destruct( pElement++ );

	m_nCount = 0;
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, size_t N, class I, class A >
inline void CUtlLeanVectorFixedGrowableBase<T, N, I, A>::Purge()
{
	RemoveAll();
	
	if(!IsExternallyAllocated())
	{
		if((size_t)NumAllocated() > N)
			CAllocator::Free( m_pElements );

		m_nAllocated = N;
	}
}

template< class B, class T, class I >
class CUtlLeanVectorImpl : public B
{
	typedef B BaseClass;
public:

	// constructor, destructor
	CUtlLeanVectorImpl( I growSize = 0, I initSize = 0 ) : BaseClass( growSize, initSize ) {};
	CUtlLeanVectorImpl( T *pMemory, I allocationCount, I numElements = 0 ) : BaseClass( pMemory, allocationCount, numElements ) {};
	~CUtlLeanVectorImpl() {};

	// Copy the array.
	CUtlLeanVectorImpl<B, T, I>& operator=( const CUtlLeanVectorImpl<B, T, I> &other );

	class Iterator_t
	{
	public:
		Iterator_t( I i ) : index( i ) {}
		I index;

		bool operator==( const Iterator_t it ) const { return index == it.index; }
		bool operator!=( const Iterator_t it ) const { return index != it.index; }
	};
	Iterator_t First() const { return Iterator_t( this->IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const { return Iterator_t( this->IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const { return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const { return this->IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const { return Iterator_t( InvalidIndex() ); }

	T &operator[]( const Iterator_t &it ) { return Element( it.index ); }
	const T &operator[]( const Iterator_t &it ) const { return Element( it.index ); }

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

	// Specify the invalid ('null') index that we'll only return on failure
	static const I INVALID_INDEX = (I)-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	// Adds an element, uses default constructor
	T* AddToTailGetPtr();

	// Adds an element, uses copy constructor
	int AddToTail();
	int AddToTail( const T& src );

	// Adds multiple elements, uses default constructor
	int AddMultipleToTail( int nSize );	

	// Adds multiple elements, uses default constructor
	T* InsertBeforeGetPtr( int nBeforeIndex, int nSize = 1 );

	void SetSize( int size );
	void SetCount( int count );

	void EnsureCount( int num );

	// Finds an element (element needs operator== defined)
	int Find( const T& src ) const;

	// Element removal
	void FastRemove( int elem ); // doesn't preserve order
	void Remove( int elem ); // preserves order, shifts elements
	bool FindAndRemove( const T& src );	// removes first occurrence of src, preserves order, shifts elements
	bool FindAndFastRemove( const T& src );	// removes first occurrence of src, doesn't preserve order
	void RemoveMultiple( int elem, int num ); // preserves order, shifts elements
	void RemoveMultipleFromHead( int num ); // preserves order, shifts elements
	void RemoveMultipleFromTail( int num ); // preserves order, shifts elements

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
	Assert( i < this->m_nCount );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::operator[]( int i ) const
{
	Assert( i < this->m_nCount );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Element( int i )
{
	Assert( i < this->m_nCount );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Element( int i ) const
{
	Assert( i < this->m_nCount );
	return this->Base()[ i ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Head()
{
	Assert( this->m_nCount > 0 );
	return this->Base()[ 0 ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Head() const
{
	Assert( this->m_nCount > 0 );
	return this->Base()[ 0 ];
}

template< class B, class T, class I >
inline T& CUtlLeanVectorImpl<B, T, I>::Tail()
{
	Assert( this->m_nCount > 0 );
	return this->Base()[ this->m_nCount - 1 ];
}

template< class B, class T, class I >
inline const T& CUtlLeanVectorImpl<B, T, I>::Tail() const
{
	Assert( this->m_nCount > 0 );
	return this->Base()[ this->m_nCount - 1 ];
}

//-----------------------------------------------------------------------------
// Count
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline int CUtlLeanVectorImpl<B, T, I>::Count() const
{
	return this->m_nCount;
}

//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class B, class T, class I >
inline bool CUtlLeanVectorImpl<B, T, I>::IsValidIndex( int i ) const
{
	return (i >= 0) && (i < this->m_nCount);
}

//-----------------------------------------------------------------------------
// Adds an element, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
T* CUtlLeanVectorImpl<B, T, I>::AddToTailGetPtr()
{
	this->EnsureCapacity( this->m_nCount + 1 );
	T* pBase = this->Base();
	Construct( &pBase[ this->m_nCount ] );
	return &pBase[ this->m_nCount++ ];
}

//-----------------------------------------------------------------------------
// Adds an element, uses copy constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::AddToTail()
{
	this->EnsureCapacity( this->m_nCount + 1 );
	T* pBase = this->Base();
	Construct( &pBase[ this->m_nCount ] );
	return this->m_nCount++;
}

template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::AddToTail( const T& src )
{
	this->EnsureCapacity( this->m_nCount + 1 );
	T* pBase = this->Base();
	CopyConstruct( &pBase[ this->m_nCount ], src );
	return this->m_nCount++;
}

//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
int CUtlLeanVectorImpl<B, T, I>::AddMultipleToTail( int nSize )
{
	int nOldSize = this->m_nCount;

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

		this->m_nCount = nNewSize;
	}

	return nOldSize;
}

//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< class B, class T, class I >
T* CUtlLeanVectorImpl<B, T, I>::InsertBeforeGetPtr( int nBeforeIndex, int nSize )
{
	int nOldSize = this->m_nCount;

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

	this->m_nCount = nNewSize;

	return &pBase[ nBeforeIndex ];
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::SetCount( int count )
{
	this->EnsureCapacity( count );

	T* pBase = this->Base();

	if ( this->m_nCount < count )
		ConstructElements( &pBase[ this->m_nCount ], &pBase[ count ] );
	else if ( this->m_nCount > count )
		DestructElements( &pBase[ count ], &pBase[ this->m_nCount ] );

	this->m_nCount = count;
}

template< class B, class T, class I >
inline void CUtlLeanVectorImpl<B, T, I>::SetSize( int size )
{
	SetCount( size );
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::EnsureCount( int num )
{
	if(Count() < num)
	{
		AddMultipleToTail( num - Count() );
	}
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
	if ( this->m_nCount > 0 )
	{
		if ( elem != this->m_nCount - 1 )
			V_memmove( &pBase[ elem ], &pBase[ this->m_nCount - 1 ], sizeof( T ) );
		--this->m_nCount;
	}
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::Remove( int elem )
{
	T* pBase = this->Base();
	Destruct( &pBase[ elem ] );
	ShiftElements( &pBase[ elem ], &pBase[ elem + 1 ], &pBase[ this->m_nCount ] );
	--this->m_nCount;
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
	ShiftElements( &pBase[ elem ], &pBase[ elem + num ], &pBase[ this->m_nCount ] );
	this->m_nCount -= num;
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::RemoveMultipleFromHead( int num )
{
	Assert( num <= Count() );
	RemoveMultiple( 0, num );
}

template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::RemoveMultipleFromTail( int num )
{
	Assert( num <= Count() );
	RemoveMultiple( Count() - num, num );
}

//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template< class B, class T, class I >
void CUtlLeanVectorImpl<B, T, I>::ShiftElements( T* pDest, const T* pSrc, const T* pSrcEnd )
{
	ptrdiff_t numToMove = pSrcEnd - pSrc;
	if ( numToMove > 0 )
		V_memmove( pDest, pSrc, numToMove * sizeof( T ) );
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

template < class T, class I = short, class A = CMemAllocAllocator >
using CUtlLeanVector = CUtlLeanVectorImpl< CUtlLeanVectorBase< T, I, A >, T, I >;

template < class T, size_t N = 3, class I = int, class A = CMemAllocAllocator >
using CUtlLeanVectorFixedGrowable = CUtlLeanVectorImpl< CUtlLeanVectorFixedGrowableBase< T, N, I, A >, T, I >;

#endif // UTLLEANVECTOR_H
