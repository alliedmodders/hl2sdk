//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class. This memory class does *not* store all
// of its memory in a contiguous block, and but it does guarantee to not
// reallocate it. Note the strange interface... ints are used instead of void*s
// to make this look as similar as possible to CUtlMemory.
//=============================================================================//

#ifndef UTLFIXEDMEMORY_H
#define UTLFIXEDMEMORY_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include <string.h>
#include <new.h>
#include "tier0/platform.h"

#include "tier0/memdbgon.h"

#pragma warning (disable:4100)
#pragma warning (disable:4514)


//-----------------------------------------------------------------------------
// The CUtlFixedMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T >
class CUtlFixedMemory
{
public:
	// constructor, destructor
	CUtlFixedMemory( int nGrowSize = 0, int nInitSize = 0 );
	~CUtlFixedMemory();

	// element access
	T& operator[]( int i );
	T const& operator[]( int i ) const;
	T& Element( int i );
	T const& Element( int i ) const;

	// Can we use this index?
	bool IsIdxValid( int i ) const;

	// Size
	int AllocationCount() const;

	// Allocate, free a single element
	int Alloc();
	void Remove( int i );
	void RemoveAll( );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// Set the size by which the memory grows
	void SetGrowSize( int size );

	// Used to iterate over all elements
	int FirstElement();
	int NextElement( int i );

private:
	enum
	{
		EXTERNAL_BUFFER_MARKER = -1,
	};

	struct BlockHeader_t
	{
		BlockHeader_t *m_pNext;
		int m_nBlockCount;
	};

private:
	void SetupFreeList( BlockHeader_t *pHeader );
	void Grow( int num = 1 );


private:
	BlockHeader_t *m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
	T *m_pFirstFree;
};


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T >
CUtlFixedMemory<T>::CUtlFixedMemory( int nGrowSize, int nInitAllocationCount ) : m_pMemory(0), m_pFirstFree(0), 
	m_nAllocationCount( nInitAllocationCount ), m_nGrowSize( nGrowSize )
{
	// T must be at least as big as a pointer or the free list fails
	COMPILE_TIME_ASSERT( sizeof(T) >= 4 );

	Assert( (nGrowSize >= 0) && (nGrowSize != EXTERNAL_BUFFER_MARKER) );
	if (m_nAllocationCount)
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (BlockHeader_t*)malloc( sizeof(BlockHeader_t) + m_nAllocationCount * sizeof(T) );
		m_pMemory->m_pNext = NULL;
		m_pMemory->m_nBlockCount = m_nAllocationCount;
		SetupFreeList( m_pMemory );
	}
}

template< class T >
CUtlFixedMemory<T>::~CUtlFixedMemory()
{
	Purge();
}


//-----------------------------------------------------------------------------
// Sets up the free list
//-----------------------------------------------------------------------------
template< class T >
void CUtlFixedMemory<T>::SetupFreeList( BlockHeader_t *pHeader )
{
	T* pPrev = m_pFirstFree;
	m_pFirstFree = (T*)( pHeader + 1 );
	T* pCurr = m_pFirstFree + pHeader->m_nBlockCount;
	while ( --pCurr >= m_pFirstFree )
	{
		// Treat the first four
		*((T**)pCurr) = pPrev;
		pPrev = pCurr;
	}
}


//-----------------------------------------------------------------------------
// Used to iterate over all elements. Not particularly fast.
//-----------------------------------------------------------------------------
template< class T >
int CUtlFixedMemory<T>::FirstElement()
{
	return m_pMemory ? (int)(m_pMemory + 1) : 0;
}


template< class T >
int CUtlFixedMemory<T>::NextElement( int i )
{
	T* pTest = (T*)i;
	BlockHeader_t *pCheck = m_pMemory;
	while (pCheck)
	{
		T* pBase = (T*)(pCheck + 1);
		T* pLast = pBase + pCheck->m_nBlockCount;
		if ( (i >= (int)pBase) && ( i < (int)pLast ) )
		{
			++pTest;
			if ( pTest >= pLast )
			{
				pTest = (T*)(pCheck->m_pNext + 1);
				return (int)pTest;
			}
		}
		pCheck = pCheck->m_pNext;
	}

	return 0;
}


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< class T >
inline T& CUtlFixedMemory<T>::operator[]( int i )
{
	// Indices are actually pointers here
	return *(T*)i;
}

template< class T >
inline T const& CUtlFixedMemory<T>::operator[]( int i ) const
{
	return *(T*)i;
}

template< class T >
inline T& CUtlFixedMemory<T>::Element( int i )
{
	// Indices are actually pointers here
	return *(T*)i;
}

template< class T >
inline T const& CUtlFixedMemory<T>::Element( int i ) const
{
	// Indices are actually pointers here
	return *(T*)i;
}


//-----------------------------------------------------------------------------
// Sets the grow size
//-----------------------------------------------------------------------------
template< class T >
void CUtlFixedMemory<T>::SetGrowSize( int nSize )
{
	Assert( nSize >= 0 );
	m_nGrowSize = nSize;
}


//-----------------------------------------------------------------------------
// Number allocated
//-----------------------------------------------------------------------------
template< class T >
inline int CUtlFixedMemory<T>::AllocationCount() const
{
	return m_nAllocationCount;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class T >
inline bool CUtlFixedMemory<T>::IsIdxValid( int i ) const
{
#ifdef _DEBUG
	BlockHeader_t *pCheck = m_pMemory;
	while (pCheck)
	{
		T* pBase = &pCheck->m_Data;
		T* pLast = pBase + pCheck->m_nBlockCount;
		if ( (i >= (int)pBase) && ( i < (int)pLast ) )
			return true;

		pCheck = pCheck->m_pNext;
	}
#endif

	return ( i != 0 );
}
 

//-----------------------------------------------------------------------------
// Allocate, free a single element
//-----------------------------------------------------------------------------
template< class T >
int CUtlFixedMemory<T>::Alloc( )
{
	if (!m_pFirstFree)
	{
		Grow();
	}

	int nRetVal = (int)m_pFirstFree;
	m_pFirstFree = *((T**)m_pFirstFree);
	return nRetVal;
}

template< class T >
void CUtlFixedMemory<T>::Remove( int i )
{
	Assert( IsIdxValid(i) );
	*((T**)i) = m_pFirstFree;
	m_pFirstFree = (T*)i;
}

template< class T >
void CUtlFixedMemory<T>::RemoveAll( )
{
	m_pFirstFree = 0;

	BlockHeader_t *pHeader;
	for ( pHeader = m_pMemory; pHeader; pHeader = pHeader->m_pNext )
	{
		SetupFreeList( pHeader );
	}
}


//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template< class T >
void CUtlFixedMemory<T>::Grow( int num )
{
	int nOldAllocation = m_nAllocationCount;
	int nAllocationRequested = m_nAllocationCount + num;
	while (m_nAllocationCount < nAllocationRequested)
	{
		if (m_nGrowSize)
		{
			m_nAllocationCount += m_nGrowSize;
		}
		else
		{
			if ( m_nAllocationCount != 0 )
			{
				m_nAllocationCount += m_nAllocationCount;
			}
			else
			{
				// Compute an allocation which is at least as big as a cache line...
				m_nAllocationCount = (31 + sizeof(T)) / sizeof(T);
				Assert(m_nAllocationCount != 0);
			}
		}
	}

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	int nNewAllocationCount = m_nAllocationCount - nOldAllocation;

	MEM_ALLOC_CREDIT_CLASS();

	BlockHeader_t *pHeader = (BlockHeader_t*)malloc( sizeof(BlockHeader_t) + nNewAllocationCount * sizeof(T) );
	pHeader->m_pNext = NULL;
	pHeader->m_nBlockCount = nNewAllocationCount;
	SetupFreeList( pHeader );

	pHeader->m_pNext = m_pMemory;
	m_pMemory = pHeader;
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T >
inline void CUtlFixedMemory<T>::EnsureCapacity( int num )
{
	if (m_nAllocationCount < num)
	{
		Grow( num - m_nAllocationCount );
	}
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T >
void CUtlFixedMemory<T>::Purge()
{
	BlockHeader_t *pHeader, *pNext;
	for ( pHeader = m_pMemory; pHeader; pHeader = pNext )
	{
		pNext = pHeader->m_pNext;
		free( pHeader );
	}

	m_pMemory = NULL;
	m_pFirstFree = NULL;
	m_nAllocationCount = 0;
}


#include "tier0/memdbgoff.h"

#endif // UTLFIXEDMEMORY_H
