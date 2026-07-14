//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable array class that maintains a free list and keeps elements
// in the same location
//=============================================================================//

#ifndef UTLVECTOR_H
#define UTLVECTOR_H

#ifdef _WIN32
#pragma once
#endif


#include <string.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier1/utlvectormemory.h"
#include "tier1/utlblockmemory.h"

#define FOR_EACH_VEC( vecName, iteratorName ) \
	for ( int iteratorName = 0; iteratorName < (vecName).Count(); iteratorName++ )
#define FOR_EACH_VEC_BACK( vecName, iteratorName ) \
	for ( int iteratorName = (vecName).Count()-1; iteratorName >= 0; iteratorName-- )

//-----------------------------------------------------------------------------
// The CUtlVector class:
// A growable array class which doubles in size by default.
// It will always keep all elements consecutive in memory, and may move the
// elements around in memory (via a PvRealloc) when elements are inserted or
// removed. Clients should therefore refer to the elements of the vector
// by index (they should *never* maintain pointers to elements in the vector).
//-----------------------------------------------------------------------------
// UtlVector derives from this so it can be identified at compile time.
struct base_vector_t
{
	enum { IsUtlVector = true };
};

template< class T, class I = int, class A = CUtlVectorMemory_Growable<T, I> >
class CUtlVectorBase : public base_vector_t
{
	typedef A CAllocator;
public:
	typedef T ElemType_t;
	typedef I IndexType_t;

	// constructor, destructor
	CUtlVectorBase( I growSize = 0, I initSize = 0 );
	CUtlVectorBase( T* pMemory, I allocationCount, I numElements = 0 );
	~CUtlVectorBase();
	
	// Copy the array.
	CUtlVectorBase<T, I, A>& operator=( const CUtlVectorBase<T, I, A> &other );

	// element access
	T& operator[]( I i );
	const T& operator[]( I i ) const;
	T& Element( I i );
	const T& Element( I i ) const;
	T& Head();
	const T& Head() const;
	T& Tail();
	const T& Tail() const;

	// Gets the base address (can change when adding elements!)
	T* Base()								{ return m_Memory.Base(); }
	const T* Base() const					{ return m_Memory.Base(); }

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T *pMemory, I allocationCount, I numElements = 0 );
	void SetExternalBuffer( const T *pMemory, I allocationCount, I numElements = 0 );
	void AssumeMemory( T *pMemory, I allocationCount, I numElements = 0 );
	T *Detach();
	void *DetachMemory();

	// Returns the number of elements in the vector
	I Count() const;

	// Is element index valid?
	bool IsValidIndex( I i ) const;
	static I InvalidIndex();

	// Adds an element, uses default constructor
	I AddToHead();
	I AddToTail();
	T* AddToTailGetPtr();
	I InsertBefore( I elem );
	I InsertAfter( I elem );

	// Adds an element, uses copy constructor
	I AddToHead( const T& src );
	I AddToTail( const T& src );
	I InsertBefore( I elem, const T& src );
	I InsertAfter( I elem, const T& src );

	// Adds multiple elements, uses default constructor
	I AddMultipleToHead( I num );
	I AddMultipleToTail( I num );	   
	I AddMultipleToTail( I num, const T *pToCopy );	   
	I InsertMultipleBefore( I elem, I num );
	I InsertMultipleBefore( I elem, I num, const T *pToCopy );
	I InsertMultipleAfter( I elem, I num );

	// Calls RemoveAll() then AddMultipleToTail.
	void SetSize( I size );
	void SetCount( I count );
	void SetCountNonDestructively( I count ); //sets count by adding or removing elements to tail TODO: This should probably be the default behavior for SetCount
	
	// Calls SetSize and copies each element.
	void CopyArray( const T *pArray, I size );

	// Fast swap
	void Swap( CUtlVectorBase<T, I, A> &vec );
	
	// Add the specified array to the tail.
	I AddVectorToTail( CUtlVectorBase<T, I, A> const &src );

	// Finds an element (element needs operator== defined)
	I Find( const T& src ) const;
	void FillWithValue( const T& src );

	bool HasElement( const T& src ) const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	void EnsureCapacity( I num );

	// Makes sure we have at least this many elements
	void EnsureCount( I num );

	// Element removal
	void FastRemove( I elem );	// doesn't preserve order
	void Remove( I elem );		// preserves order, shifts elements
	bool FindAndRemove( const T& src );	// removes first occurrence of src, preserves order, shifts elements
	bool FindAndFastRemove( const T& src );	// removes first occurrence of src, doesn't preserve order
	void RemoveMultiple( I elem, I num );	// preserves order, shifts elements
	void RemoveMultipleFromHead(I num); // removes num elements from tail
	void RemoveMultipleFromTail(I num); // removes num elements from tail
	void RemoveAll();				// doesn't deallocate memory

	// Memory deallocation
	void Purge();

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements();

	// Compacts the vector to the number of elements actually in use 
	void Compact();

	// Set the size by which it grows when it needs to allocate more memory.
	void SetGrowSize( I size )			{ m_Memory.SetGrowSize( size ); }

	I NumAllocated() const;	// Only use this if you really know what you're doing!

	void Sort( int (__cdecl *pfnCompare)(const T *, const T *) );

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, char *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

protected:
	// Can't copy this unless we explicitly do it!
	CUtlVectorBase( CUtlVectorBase const& vec ) { Assert(0); }

	// Grows the vector
	void GrowVector( I num = 1 );

	// Shifts elements....
	void ShiftElementsRight( I elem, I num = 1 );
	void ShiftElementsLeft( I elem, I num = 1 );

	I m_Size;
	CAllocator m_Memory;

};


//-----------------------------------------------------------------------------
// The CUtlVector class:
// The default growable vector. A thin wrapper over CUtlVectorBase.
//-----------------------------------------------------------------------------
template< class T, class I = int, class A = CUtlVectorMemory_Growable< T, I > >
class CUtlVector : public CUtlVectorBase< T, I, A >
{
	typedef CUtlVectorBase< T, I, A > BaseClass;
public:

	using BaseClass::BaseClass;
};


// this is kind of ugly, but until C++ gets templatized typedefs in C++0x, it's our only choice
template < class T, class I = int >
class CUtlBlockVector : public CUtlVectorBase< T, I, CUtlBlockMemory< T, I > >
{
	typedef CUtlVectorBase< T, I, CUtlBlockMemory< T, I > > BaseClass;
public:

	using BaseClass::BaseClass;
};

//-----------------------------------------------------------------------------
// The CUtlVectorMT class:
// A array class with some sort of mutex protection. Not sure which operations are protected from
// which others.
//-----------------------------------------------------------------------------

template< class BASE_UTLVECTOR, class MUTEX_TYPE = CThreadFastMutex >
class CUtlVectorMT : public BASE_UTLVECTOR, public MUTEX_TYPE
{
	typedef BASE_UTLVECTOR BaseClass;
public:
	MUTEX_TYPE Mutex_t;

	using BaseClass::BaseClass;
};


//-----------------------------------------------------------------------------
// The CUtlVectorFixed class:
// A array class with a fixed allocation scheme
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE, class I = int >
class CUtlVectorFixed : public CUtlVectorBase< T, I, CUtlVectorMemory_Fixed<T, MAX_SIZE, I > >
{
	typedef CUtlVectorBase< T, I, CUtlVectorMemory_Fixed<T, MAX_SIZE, I > > BaseClass;
public:

	using BaseClass::BaseClass;
};


//-----------------------------------------------------------------------------
// The CUtlVectorFixedGrowable class:
// A array class with a fixed allocation scheme backed by a dynamic one
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE, class I = int >
class CUtlVectorFixedGrowable : public CUtlVectorBase< T, I, CUtlVectorMemory_FixedGrowable<T, MAX_SIZE, I> >
{
	typedef CUtlVectorBase< T, I, CUtlVectorMemory_FixedGrowable<T, MAX_SIZE, I> > BaseClass;

public:

	// constructor, destructor
	CUtlVectorFixedGrowable( int growSize = 0 ) : BaseClass( growSize, MAX_SIZE ) {}
};

//-----------------------------------------------------------------------------
// The CUtlVectorConservative class:
// A array class with a conservative allocation scheme
//-----------------------------------------------------------------------------
template< class T, class I = int >
class CUtlVectorConservative : public CUtlVectorBase< T, I, CUtlVectorMemory_Conservative<T> >
{
	typedef CUtlVectorBase< T, I, CUtlVectorMemory_Conservative<T> > BaseClass;
public:

	using BaseClass::BaseClass;
};

template< class T, class I = int, class A = CMemAllocAllocator >
class CUtlVectorRawAllocator : public CUtlVectorBase< T, I, CUtlVectorMemory_RawAllocator<T, A> >
{
	typedef CUtlVectorBase< T, I, CUtlVectorMemory_RawAllocator<T, A> > BaseClass;
	typedef A CAllocator;

public:

	using BaseClass::BaseClass;
};

//-----------------------------------------------------------------------------
// The CUtlVectorUltra Conservative class:
// A array class with a very conservative allocation scheme, with customizable allocator
// Especialy useful if you have a lot of vectors that are sparse, or if you're
// carefully packing holders of vectors
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4815 ) // warning C4815: 'staticData' : zero-sized array in stack object will have no elements
#endif

class CUtlVectorUltraConservativeAllocator
{
public:
	static void *Alloc( size_t nSize )
	{
		return malloc( nSize );
	}

	static void *Realloc( void *pMem, size_t nSize )
	{
		return realloc( pMem, nSize );
	}

	static void Free( void *pMem )
	{
		free( pMem );
	}

	static size_t GetSize( void *pMem )
	{
		return mallocsize( pMem );
	}

};

template <typename T, typename A = CUtlVectorUltraConservativeAllocator >
class CUtlVectorUltraConservative : private A
{
public:
	CUtlVectorUltraConservative()
	{
		m_pData = StaticData();
	}

	~CUtlVectorUltraConservative()
	{
		RemoveAll();
	}

	int Count() const
	{
		return m_pData->m_Size;
	}

	static int InvalidIndex()
	{
		return -1;
	}

	inline bool IsValidIndex( int i ) const
	{
		return (i >= 0) && (i < Count());
	}

	T& operator[]( int i )
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	const T& operator[]( int i ) const
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	T& Element( int i )
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	const T& Element( int i ) const
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	void EnsureCapacity( int num )
	{
		int nCurCount = Count();
		if ( num <= nCurCount )
		{
			return;
		}
		if ( m_pData == StaticData() )
		{
			m_pData = (Data_t *)A::Alloc( sizeof(int) + ( num * sizeof(T) ) );
			m_pData->m_Size = 0;
		}
		else
		{
			int nNeeded = sizeof(int) + ( num * sizeof(T) );
			int nHave = A::GetSize( m_pData );
			if ( nNeeded > nHave )
			{
				m_pData = (Data_t *)A::Realloc( m_pData, nNeeded );
			}
		}
	}

	int AddToTail( const T& src )
	{
		int iNew = Count();
		EnsureCapacity( Count() + 1 );
		m_pData->m_Elements[iNew] = src;
		m_pData->m_Size++;
		return iNew;
	}

	void RemoveAll()
	{
		if ( Count() )
		{
			for (int i = m_pData->m_Size; --i >= 0; )
			{
				Destruct(&m_pData->m_Elements[i]);
			}
		}
		if ( m_pData != StaticData() )
		{
			A::Free( m_pData );
			m_pData = StaticData();

		}
	}

	void PurgeAndDeleteElements()
	{
		if ( m_pData != StaticData() )
		{
			for( int i=0; i < m_pData->m_Size; i++ )
			{
				delete Element(i);
			}
			RemoveAll();
		}
	}

	void FastRemove( int elem )
	{
		Assert( IsValidIndex(elem) );

		Destruct( &Element(elem) );
		if (Count() > 0)
		{
			if ( elem != m_pData->m_Size -1 )
				memcpy( (void *)&Element(elem), (void *)&Element(m_pData->m_Size-1), sizeof(T) );
			--m_pData->m_Size;
		}
		if ( !m_pData->m_Size )
		{
			A::Free( m_pData );
			m_pData = StaticData();
		}
	}

	void Remove( int elem )
	{
		Destruct( &Element(elem) );
		ShiftElementsLeft(elem);
		--m_pData->m_Size;
		if ( !m_pData->m_Size )
		{
			A::Free( m_pData );
			m_pData = StaticData();
		}
	}

	int Find( const T& src ) const
	{
		int nCount = Count();
		for ( int i = 0; i < nCount; ++i )
		{
			if (Element(i) == src)
				return i;
		}
		return -1;
	}

	bool FindAndRemove( const T& src )
	{
		int elem = Find( src );
		if ( elem != -1 )
		{
			Remove( elem );
			return true;
		}
		return false;
	}


	bool FindAndFastRemove( const T& src )
	{
		int elem = Find( src );
		if ( elem != -1 )
		{
			FastRemove( elem );
			return true;
		}
		return false;
	}

	struct Data_t
	{
		int m_Size;
		T m_Elements[];
	};

	Data_t *m_pData;
private:
	void ShiftElementsLeft( int elem, int num = 1 )
	{
		int Size = Count();
		Assert( IsValidIndex(elem) || ( Size == 0 ) || ( num == 0 ));
		int numToMove = Size - elem - num;
		if ((numToMove > 0) && (num > 0))
		{
			Q_memmove( &Element(elem), &Element(elem+num), numToMove * sizeof(T) );

#ifdef _DEBUG
			Q_memset( &Element(Size-num), 0xDD, num * sizeof(T) );
#endif
		}
	}



	static Data_t *StaticData()
	{
		static Data_t staticData;
		Assert( staticData.m_Size == 0 );
		return &staticData;
	}
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//-----------------------------------------------------------------------------
// The CCopyableUtlVector class:
// A array class that allows copy construction (so you can nest a CUtlVector inside of another one of our containers)
//  WARNING - this class lets you copy construct which can be an expensive operation if you don't carefully control when it happens
// Only use this when nesting a CUtlVectorBase() inside of another one of our container classes (i.e a CUtlMap)
//-----------------------------------------------------------------------------
template< class T, class I = int >
class CCopyableUtlVector : public CUtlVector< T, I >
{
	typedef CUtlVector< T, I > BaseClass;
public:
	CCopyableUtlVector( I growSize = 0, I initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CCopyableUtlVector( T* pMemory, I numElements ) : BaseClass( pMemory, numElements ) {}
	virtual ~CCopyableUtlVector() {}
	CCopyableUtlVector( CCopyableUtlVector const& vec ) { this->CopyArray( vec.Base(), vec.Count() ); }
};

// TODO (Ilya): It seems like all the functions in CUtlVector are simple enough that they should be inlined.

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline CUtlVectorBase<T, I, A>::CUtlVectorBase( I growSize, I initSize ) : 
	m_Size(0), m_Memory(growSize, initSize)
{
}

template< typename T, class I, class A >
inline CUtlVectorBase<T, I, A>::CUtlVectorBase( T* pMemory, I allocationCount, I numElements )	: 
	m_Size(numElements), m_Memory(pMemory, allocationCount)
{
}

template< typename T, class I, class A >
inline CUtlVectorBase<T, I, A>::~CUtlVectorBase()
{
	Purge();
}

template< typename T, class I, class A >
inline CUtlVectorBase<T, I, A>& CUtlVectorBase<T, I, A>::operator=( const CUtlVectorBase<T, I, A> &other )
{
	int nCount = other.Count();
	SetSize( nCount );
	for ( int i = 0; i < nCount; i++ )
	{
		(*this)[ i ] = other[ i ];
	}
	return *this;
}


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline T& CUtlVectorBase<T, I, A>::operator[]( I i )
{
	Assert( i < m_Size );
	return m_Memory[ i ];
}

template< typename T, class I, class A >
inline const T& CUtlVectorBase<T, I, A>::operator[]( I i ) const
{
	Assert( i < m_Size );
	return m_Memory[ i ];
}

template< typename T, class I, class A >
inline T& CUtlVectorBase<T, I, A>::Element( I i )
{
	Assert( i < m_Size );
	return m_Memory[ i ];
}

template< typename T, class I, class A >
inline const T& CUtlVectorBase<T, I, A>::Element( I i ) const
{
	Assert( i < m_Size );
	return m_Memory[ i ];
}

template< typename T, class I, class A >
inline T& CUtlVectorBase<T, I, A>::Head()
{
	Assert( m_Size > 0 );
	return m_Memory[ 0 ];
}

template< typename T, class I, class A >
inline const T& CUtlVectorBase<T, I, A>::Head() const
{
	Assert( m_Size > 0 );
	return m_Memory[ 0 ];
}

template< typename T, class I, class A >
inline T& CUtlVectorBase<T, I, A>::Tail()
{
	Assert( m_Size > 0 );
	return m_Memory[ m_Size - 1 ];
}

template< typename T, class I, class A >
inline const T& CUtlVectorBase<T, I, A>::Tail() const
{
	Assert( m_Size > 0 );
	return m_Memory[ m_Size - 1 ];
}

//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, class I, class A >
inline void CUtlVectorBase<T, I, A>::SetExternalBuffer( T *pMemory, I allocationCount, I numElements )
{
	m_Memory.SetExternalBuffer( pMemory, allocationCount );
	m_Size = numElements;
}

template< class T, class I, class A >
void CUtlVectorBase<T, I, A>::SetExternalBuffer( const T *pMemory, I allocationCount, I numElements )
{
	m_Memory.SetExternalBuffer( pMemory, allocationCount );
	m_Size = numElements;
}

template< class T, class I, class A >
inline void CUtlVectorBase<T, I, A>::AssumeMemory( T *pMemory, I allocationCount, I numElements )
{
	m_Memory.AssumeMemory( pMemory, allocationCount );
	m_Size = numElements;
}

template< typename T, class I, class A >
inline T *CUtlVectorBase<T, I, A>::Detach()
{
	m_Size = 0;
	return m_Memory.Detach();
}

template< typename T, class I, class A >
inline void *CUtlVectorBase<T, I, A>::DetachMemory()
{
	m_Size = 0;
	return m_Memory.DetachMemory();
}

//-----------------------------------------------------------------------------
// Count
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::Count() const
{
	return m_Size;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline bool CUtlVectorBase<T, I, A>::IsValidIndex( I i ) const
{
	return (i >= 0) && (i < m_Size);
}
 

//-----------------------------------------------------------------------------
// Returns in invalid index
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::InvalidIndex()
{
	return -1;
}


//-----------------------------------------------------------------------------
// Grows the vector
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::GrowVector( I num )
{
	if (m_Size + num > m_Memory.NumAllocated())
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_Memory.Grow( m_Size + num - m_Memory.NumAllocated() );
	}

	m_Size += num;
}


//-----------------------------------------------------------------------------
// Sorts the vector
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::Sort( int (__cdecl *pfnCompare)(const T *, const T *) )
{
	typedef int (__cdecl *QSortCompareFunc_t)(const void *, const void *);
	if ( Count() <= 1 )
		return;

	if ( Base() )
	{
		qsort( Base(), Count(), sizeof(T), (QSortCompareFunc_t)(pfnCompare) );
	}
	else
	{
		Assert( 0 );
		// this path is untested
		// if you want to sort vectors that use a non-sequential memory allocator,
		// you'll probably want to patch in a quicksort algorithm here
		// I just threw in this bubble sort to have something just in case...

		for ( I i = m_Size - 1; i >= 0; --i )
		{
			for ( I j = 1; j <= i; ++j )
			{
				if ( pfnCompare( &Element( j - 1 ), &Element( j ) ) < 0 )
				{
					V_swap( Element( j - 1 ), Element( j ) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::EnsureCapacity( I num )
{
	MEM_ALLOC_CREDIT_CLASS();
	m_Memory.EnsureCapacity(num);
}


//-----------------------------------------------------------------------------
// Makes sure we have at least this many elements
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::EnsureCount( I num )
{
	if (Count() < num)
	{
		AddMultipleToTail( num - Count() );
	}
}


//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::ShiftElementsRight( I elem, I num )
{
	Assert( IsValidIndex(elem) || ( m_Size == 0 ) || ( num == 0 ));
	I numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
		memmove( (void*)&Element(elem+num), (void*)&Element(elem), numToMove * sizeof(T) );
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::ShiftElementsLeft( I elem, I num )
{
	Assert( IsValidIndex(elem) || ( m_Size == 0 ) || ( num == 0 ));
	I numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
	{
		memmove( (void*)&Element(elem), (void*)&Element(elem+num), numToMove * sizeof(T) );

#ifdef _DEBUG
		Q_memset( (void*)&Element(m_Size-num), 0xDD, num * sizeof(T) );
#endif
	}
}


//-----------------------------------------------------------------------------
// Adds an element, uses default constructor
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddToHead()
{
	return InsertBefore(0);
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddToTail()
{
	return InsertBefore( m_Size );
}

template< typename T, class I, class A >
inline T* CUtlVectorBase<T, I, A>::AddToTailGetPtr()
{
	return &Element(AddToTail());
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::InsertAfter( I elem )
{
	return InsertBefore( elem + 1 );
}

template< typename T, class I, class A >
I CUtlVectorBase<T, I, A>::InsertBefore( I elem )
{
	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	Construct( &Element(elem) );
	return elem;
}


//-----------------------------------------------------------------------------
// Adds an element, uses copy constructor
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddToHead( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( 0, src );
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddToTail( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( m_Size, src );
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::InsertAfter( I elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( elem + 1, src );
}

template< typename T, class I, class A >
I CUtlVectorBase<T, I, A>::InsertBefore( I elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 

	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	CopyConstruct( &Element(elem), src );
	return elem;
}


//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddMultipleToHead( I num )
{
	return InsertMultipleBefore( 0, num );
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddMultipleToTail( I num )
{
	return InsertMultipleBefore( m_Size, num );
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::AddMultipleToTail( I num, const T *pToCopy )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pToCopy || (pToCopy + num <= Base()) || (pToCopy >= (Base() + Count()) ) ); 

	return InsertMultipleBefore( m_Size, num, pToCopy );
}

template< typename T, class I, class A >
I CUtlVectorBase<T, I, A>::InsertMultipleAfter( I elem, I num )
{
	return InsertMultipleBefore( elem + 1, num );
}


template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::SetCount( I count )
{
	RemoveAll();
	AddMultipleToTail( count );
}

template< typename T, class I, class A >
inline void CUtlVectorBase<T, I, A>::SetSize( I size )
{
	SetCount( size );
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::SetCountNonDestructively( I count )
{
	I delta = count - m_Size;
	if(delta > 0) AddMultipleToTail( delta );
	else if(delta < 0) RemoveMultipleFromTail( -delta );
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::CopyArray( const T *pArray, I size )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pArray || (Base() >= (pArray + size)) || (pArray >= (Base() + Count()) ) ); 

	SetSize( size );
	for( I i=0; i < size; i++ )
	{
		(*this)[i] = pArray[i];
	}
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::Swap( CUtlVectorBase< T, I, A > &vec )
{
	m_Memory.Swap( vec.m_Memory );
	V_swap( m_Size, vec.m_Size );
}

template< typename T, class I, class A >
I CUtlVectorBase<T, I, A>::AddVectorToTail( CUtlVectorBase const &src )
{
	Assert( &src != this );

	I base = Count();
	
	// Make space.
	I nSrcCount = src.Count();
	EnsureCapacity( base + nSrcCount );

	// Copy the elements.	
	m_Size += nSrcCount;
	for ( I i=0; i < nSrcCount; i++ )
	{
		CopyConstruct( &Element(base+i), src[i] );
	}
	return base;
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::InsertMultipleBefore( I elem, I num )
{
	if( num == 0 )
		return elem;

	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector(num);
	ShiftElementsRight( elem, num );

	// Invoke default constructors
	for (I i = 0; i < num; ++i )
	{
		Construct( &Element( elem+i ) );
	}

	return elem;
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::InsertMultipleBefore( I elem, I num, const T *pToInsert )
{
	if( num == 0 )
		return elem;
	
	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector(num);
	ShiftElementsRight( elem, num );

	// Invoke default constructors
	if ( !pToInsert )
	{
		for ( I i = 0; i < num; ++i )
		{
			Construct( &Element( elem+i ) );
		}
	}
	else
	{
		for ( I i=0; i < num; i++ )
		{
			CopyConstruct( &Element( elem+i ), pToInsert[i] );
		}
	}

	return elem;
}


//-----------------------------------------------------------------------------
// Finds an element (element needs operator== defined)
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
I CUtlVectorBase<T, I, A>::Find( const T& src ) const
{
	for ( I i = 0; i < Count(); ++i )
	{
		if (Element(i) == src)
			return i;
	}
	return -1;
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::FillWithValue( const T& src )
{
	for ( I i = 0; i < Count(); i++ )
	{
		Element(i) = src;
	}
}

template< typename T, class I, class A >
bool CUtlVectorBase<T, I, A>::HasElement( const T& src ) const
{
	return ( Find(src) >= 0 );
}


//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::FastRemove( I elem )
{
	Assert( IsValidIndex(elem) );

	Destruct( &Element(elem) );
	if (m_Size > 0)
	{
		if ( elem != m_Size -1 )
			memcpy( (void *)&Element(elem), (void *)&Element(m_Size-1), sizeof(T) );
		--m_Size;
	}
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::Remove( I elem )
{
	Destruct( &Element(elem) );
	ShiftElementsLeft(elem);
	--m_Size;
}

template< typename T, class I, class A >
bool CUtlVectorBase<T, I, A>::FindAndRemove( const T& src )
{
	I elem = Find( src );
	if ( elem != -1 )
	{
		Remove( elem );
		return true;
	}
	return false;
}

template< typename T, class I, class A >
bool CUtlVectorBase<T, I, A>::FindAndFastRemove( const T& src )
{
	I elem = Find( src );
	if ( elem != -1 )
	{
		FastRemove( elem );
		return true;
	}
	return false;
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::RemoveMultiple( I elem, I num )
{
	Assert( elem >= 0 );
	Assert( elem + num <= Count() );

	for (I i = elem + num; --i >= elem; )
		Destruct(&Element(i));

	ShiftElementsLeft(elem, num);
	m_Size -= num;
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::RemoveMultipleFromHead( I num )
{
	Assert( num <= Count() );

	for (I i = num; --i >= 0; )
		Destruct(&Element(i));

	ShiftElementsLeft(0, num);
	m_Size -= num;
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::RemoveMultipleFromTail( I num )
{
	Assert( num <= Count() );

	for (I i = m_Size-num; i < m_Size; i++)
		Destruct(&Element(i));

	m_Size -= num;
}

template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::RemoveAll()
{
	for (I i = m_Size; --i >= 0; )
	{
		Destruct(&Element(i));
	}

	m_Size = 0;
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------

template< typename T, class I, class A >
inline void CUtlVectorBase<T, I, A>::Purge()
{
	RemoveAll();
	m_Memory.Purge();
}


template< typename T, class I, class A >
inline void CUtlVectorBase<T, I, A>::PurgeAndDeleteElements()
{
	for( I i=0; i < m_Size; i++ )
	{
		delete Element(i);
	}
	Purge();
}

template< typename T, class I, class A >
inline void CUtlVectorBase<T, I, A>::Compact()
{
	m_Memory.Purge(m_Size);
}

template< typename T, class I, class A >
inline I CUtlVectorBase<T, I, A>::NumAllocated() const
{
	return m_Memory.NumAllocated();
}


//-----------------------------------------------------------------------------
// Data and memory validation
//-----------------------------------------------------------------------------
#ifdef DBGFLAG_VALIDATE
template< typename T, class I, class A >
void CUtlVectorBase<T, I, A>::Validate( CValidator &validator, char *pchName )
{
	validator.Push( typeid(*this).name(), this, pchName );

	m_Memory.Validate( validator, "m_Memory" );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE

// A vector class for storing pointers, so that the elements pointed to by the pointers are deleted
// on exit.
template<class T, class I = int> class CUtlVectorAutoPurge : public CUtlVector< T, I >
{
public:
	~CUtlVectorAutoPurge( void )
	{
		this->PurgeAndDeleteElements();
	}

};

#endif // CCVECTOR_H
