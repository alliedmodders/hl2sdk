//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//

#ifndef UTLVECTORMEMORY_H
#define UTLVECTORMEMORY_H

#ifdef _WIN32
#pragma once
#endif

#include <limits>

#include "tier0/dbg.h"
#include <string.h>
#include "tier0/platform.h"

#include "tier0/memalloc.h"
#include "tier1/rawallocator.h"
#include "mathlib/mathlib.h"
#include "tier0/memdbgon.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#ifdef _MSC_VER
#pragma warning (disable:4100)
#pragma warning (disable:4514)
#endif

//-----------------------------------------------------------------------------


#ifdef UTLMEMORY_TRACK
#define UTLMEMORY_TRACK_ALLOC()		MemAlloc_RegisterAllocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#define UTLMEMORY_TRACK_FREE()		if ( !m_pMemory ) ; else MemAlloc_RegisterDeallocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#else
#define UTLMEMORY_TRACK_ALLOC()		((void)0)
#define UTLMEMORY_TRACK_FREE()		((void)0)
#endif


//-----------------------------------------------------------------------------
// The CUtlVectorMemory_Growable class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, class I = int, int UNK = 0 >
class CUtlVectorMemory_Growable
{
public:
	// constructor, destructor
	CUtlVectorMemory_Growable( I nGrowSize = 0, I nInitSize = 0 );
	CUtlVectorMemory_Growable( T* pMemory, I numElements );
	CUtlVectorMemory_Growable( const T* pMemory, I numElements );
	~CUtlVectorMemory_Growable();

	// Set the size by which the memory grows
	void Init( I nGrowSize = 0, I nInitSize = 0 );

	class Iterator_t
	{
	public:
		Iterator_t( I i ) : index( i ) {}
		I index;

		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ return Iterator_t( IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const	{ return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex() ); }

	// element access
	T& operator[]( I i );
	const T& operator[]( I i ) const;
	T& Element( I i );
	const T& Element( I i ) const;

	// Can we use this index?
	bool IsIdxValid( I i ) const;

	// Specify the invalid ('null') index that we'll only return on failure
	static inline const I INVALID_INDEX = ( I )-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, I numElements );
	void SetExternalBuffer( const T* pMemory, I numElements );
	void AssumeMemory( T *pMemory, I nSize );
	T* Detach();
	void *DetachMemory();

	// Fast swap
	void Swap( CUtlVectorMemory_Growable &mem );

	// Switches the buffer from an external memory buffer to a reallocatable buffer
	// Will copy the current contents of the external buffer to the reallocatable buffer
	void ConvertToGrowableMemory( I nGrowSize );

	// Size
	I NumAllocated() const;
	I Count() const;

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( I num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( I num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements
	void Purge( I numElements );

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// is the memory read only?
	bool IsReadOnly() const;

	// Set the size by which the memory grows
	void SetGrowSize( I size );

protected:
	void ValidateGrowSize()
	{
#ifdef _X360
		if ( m_nGrowSize && (m_nGrowSize & EXTERNAL_BUFFER_MARKER) == 0 )
		{
			// Max grow size at 128 bytes on XBOX
			const int MAX_GROW = 128;
			if ( m_nGrowSize * sizeof(T) > MAX_GROW )
			{
				m_nGrowSize = max( 1, MAX_GROW / sizeof(T) );
			}
		}
#endif
	}

	enum
	{
		EXTERNAL_CONST_BUFFER_MARKER = (1 << 30),
		EXTERNAL_BUFFER_MARKER = (1 << 31),
	};

	T* m_pMemory;
	I m_nAllocationCount;
	I m_nGrowSize;
};

//-----------------------------------------------------------------------------
// The CUtlVectorMemory_FixedGrowable class:
// A growable memory class backed by a fixed allocation.
//-----------------------------------------------------------------------------
template< class T, size_t SIZE, class I = int >
class CUtlVectorMemory_FixedGrowable : public CUtlVectorMemory_Growable< T, I >
{
	typedef CUtlVectorMemory_Growable< T, I > BaseClass;

public:
	CUtlVectorMemory_FixedGrowable( I nGrowSize = 0, I nInitSize = SIZE ) : BaseClass( m_pFixedMemory, SIZE ) 
	{
		Assert( nInitSize == 0 || nInitSize == SIZE );
	}

private:
	T m_pFixedMemory[ SIZE ];
};

//-----------------------------------------------------------------------------
// The CUtlVectorMemory_Fixed class:
// A fixed memory class
//-----------------------------------------------------------------------------
template< typename T, size_t SIZE, class I = int, int nAlignment = 0 >
class CUtlVectorMemory_Fixed
{
public:
	// constructor, destructor
	CUtlVectorMemory_Fixed( I nGrowSize = 0, I nInitSize = 0 )	{ Assert( nInitSize == 0 || nInitSize == (I)SIZE ); }
	CUtlVectorMemory_Fixed( T* pMemory, I numElements )			{ Assert( 0 ); 										}

	// Can we use this index?
	bool IsIdxValid( I i ) const							{ return (i >= 0) && (i < (I)SIZE); }

	// Specify the invalid ('null') index that we'll only return on failure
	static inline const I INVALID_INDEX = -1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	// Gets the base address
	T* Base()												{ if ( nAlignment == 0 ) return (T*)(&m_Memory[0]); else return (T*)AlignValue( &m_Memory[0], nAlignment ); }
	const T* Base() const									{ if ( nAlignment == 0 ) return (T*)(&m_Memory[0]); else return (T*)AlignValue( &m_Memory[0], nAlignment ); }

	// element access
	T& operator[]( I i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( I i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( I i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( I i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, I numElements )		{ Assert( 0 ); }

	// Size
	I NumAllocated() const									{ return (I)SIZE; }
	I Count() const											{ return (I)SIZE; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( I num = 1 )									{ Assert( 0 ); }

	// Makes sure we've got at least this much memory
	void EnsureCapacity( I num )							{ Assert( num <= (I)SIZE ); }

	// Memory deallocation
	void Purge()											{}

	// Purge all but the given number of elements (NOT IMPLEMENTED IN CUtlVectorMemory_Fixed)
	void Purge( I numElements )								{ Assert( 0 ); }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( I size )								{}

	class Iterator_t
	{
	public:
		Iterator_t( I i ) : index( i ) {}
		I index;
		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ return Iterator_t( IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex() ); }

private:
	char m_Memory[ SIZE*sizeof(T) + nAlignment ];
};

#if defined(POSIX)
#define REMEMBER_ALLOC_SIZE_FOR_VALGRIND 1
#endif

//-----------------------------------------------------------------------------
// The CUtlVectorMemory_Conservative class:
// A dynamic memory class that tries to minimize overhead (itself small, no custom grow factor)
//-----------------------------------------------------------------------------
template< typename T, class I = int >
class CUtlVectorMemory_Conservative
{
public:
	// constructor, destructor
	CUtlVectorMemory_Conservative( I nGrowSize = 0, I nInitSize = 0 ) : m_pMemory( NULL )
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		m_nCurAllocSize = 0;
#endif

	}
	CUtlVectorMemory_Conservative( T* pMemory, I numElements )		{ Assert( 0 ); }
	~CUtlVectorMemory_Conservative()								{ if ( m_pMemory ) free( m_pMemory ); }

	// Can we use this index?
	bool IsIdxValid( I i ) const							{ return ( IsDebug() ) ? ( i >= 0 && i < NumAllocated() ) : ( i >= 0 ); }
	static I InvalidIndex()									{ return (I)-1; }

	// Gets the base address
	T* Base()												{ return m_pMemory; }
	const T* Base() const									{ return m_pMemory; }

	// element access
	T& operator[]( I i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( I i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( I i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( I i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, I numElements )		{ Assert( 0 ); }

	// Size
	FORCEINLINE void RememberAllocSize( size_t sz )
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		m_nCurAllocSize = sz;
#endif
	}

	size_t AllocSize( void ) const
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		return m_nCurAllocSize;
#else
		return ( m_pMemory ) ? g_pMemAlloc->GetSize( m_pMemory ) : 0;
#endif
	}

	I NumAllocated() const
	{
		return AllocSize() / sizeof( T );
	}
	I Count() const
	{
		return NumAllocated();
	}

	FORCEINLINE void ReAlloc( size_t sz )
	{
		m_pMemory = (T*)realloc( m_pMemory, sz );
		RememberAllocSize( sz );
	}
	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( I num = 1 )
	{
		I nCurN = NumAllocated();
		ReAlloc( ( (size_t)nCurN + num ) * sizeof( T ) );
	}

	// Makes sure we've got at least this much memory
	void EnsureCapacity( I num )
	{
		size_t nSize = sizeof( T ) * (size_t)MAX( num, Count() );
		ReAlloc( nSize );
	}

	// Memory deallocation
	void Purge()
	{
		free( m_pMemory ); 
		RememberAllocSize( 0 );
		m_pMemory = NULL; 
	}

	// Purge all but the given number of elements
	void Purge( I numElements )								{ ReAlloc( (size_t)numElements * sizeof(T) ); }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( I size )								{}

	class Iterator_t
	{
	public:
		Iterator_t( I i, I _limit ) : index( i ), limit( _limit ) {}
		I index;
		I limit;
		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ I limit = NumAllocated(); return Iterator_t( limit ? 0 : InvalidIndex(), limit ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( ( it.index + 1 < it.limit ) ? it.index + 1 : InvalidIndex(), it.limit ); }
	I GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const	{ return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ) && ( it.index < it.limit ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex(), 0 ); }

private:
	T *m_pMemory;
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
	size_t m_nCurAllocSize;
#endif

};


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template< class T, class I, int UNK >
CUtlVectorMemory_Growable<T,I,UNK>::CUtlVectorMemory_Growable( I nGrowSize, I nInitAllocationCount ) : m_pMemory(0), 
	m_nAllocationCount( nInitAllocationCount ), m_nGrowSize( nGrowSize & ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER) )
{
	ValidateGrowSize();
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
	}
}

template< class T, class I, int UNK >
CUtlVectorMemory_Growable<T,I,UNK>::CUtlVectorMemory_Growable( T* pMemory, I numElements ) : m_pMemory(pMemory),
	m_nAllocationCount( numElements )
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I, int UNK >
CUtlVectorMemory_Growable<T,I,UNK>::CUtlVectorMemory_Growable( const T* pMemory, I numElements ) : m_pMemory( (T*)pMemory ),
	m_nAllocationCount( numElements )
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I, int UNK >
CUtlVectorMemory_Growable<T,I,UNK>::~CUtlVectorMemory_Growable()
{
	Purge();
}

template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::Init( I nGrowSize /*= 0*/, I nInitSize /*= 0*/ )
{
	Purge();

	m_nGrowSize = nGrowSize & ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);
	m_nAllocationCount = nInitSize;
	ValidateGrowSize();
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
	}
}

//-----------------------------------------------------------------------------
// Fast swap
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::Swap( CUtlVectorMemory_Growable &mem )
{
	V_swap( m_nGrowSize, mem.m_nGrowSize );
	V_swap( m_pMemory, mem.m_pMemory );
	V_swap( m_nAllocationCount, mem.m_nAllocationCount );
}


//-----------------------------------------------------------------------------
// Switches the buffer from an external memory buffer to a reallocatable buffer
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::ConvertToGrowableMemory( I nGrowSize )
{
	if ( !IsExternallyAllocated() )
		return;

	m_nGrowSize = nGrowSize & ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();

		size_t nNumBytes = m_nAllocationCount * sizeof(T);
		T *pMemory = (T*)malloc( nNumBytes );
		memcpy( (void*)pMemory, (void *)m_pMemory, nNumBytes ); 
		m_pMemory = pMemory;
	}
	else
	{
		m_pMemory = NULL;
	}
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::SetExternalBuffer( T* pMemory, I numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_pMemory = pMemory;
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::SetExternalBuffer( const T* pMemory, I numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_pMemory = const_cast<T*>( pMemory );
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::AssumeMemory( T* pMemory, I numElements )
{
	// Blow away any existing allocated memory
	Purge();

	// Simply take the pointer but don't mark us as external
	m_pMemory = pMemory;
	m_nAllocationCount = numElements;

	m_nGrowSize &= ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);
}

template< class T, class I, int UNK >
void *CUtlVectorMemory_Growable<T,I,UNK>::DetachMemory()
{
	if ( IsExternallyAllocated() )
		return NULL;

	void *pMemory = m_pMemory;
	m_pMemory = 0;
	m_nAllocationCount = 0;
	return pMemory;
}

template< class T, class I, int UNK >
inline T* CUtlVectorMemory_Growable<T,I,UNK>::Detach()
{
	return (T*)DetachMemory();
}


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
inline T& CUtlVectorMemory_Growable<T,I,UNK>::operator[]( I i )
{
	Assert( !IsReadOnly() );
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I, int UNK >
inline const T& CUtlVectorMemory_Growable<T,I,UNK>::operator[]( I i ) const
{
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I, int UNK >
inline T& CUtlVectorMemory_Growable<T,I,UNK>::Element( I i )
{
	Assert( !IsReadOnly() );
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}

template< class T, class I, int UNK >
inline const T& CUtlVectorMemory_Growable<T,I,UNK>::Element( I i ) const
{
	Assert( IsIdxValid(i) );
	return m_pMemory[i];
}


//-----------------------------------------------------------------------------
// is the memory externally allocated?
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
bool CUtlVectorMemory_Growable<T,I,UNK>::IsExternallyAllocated() const
{
	return (m_nGrowSize & (EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER)) != 0;
}


//-----------------------------------------------------------------------------
// is the memory read only?
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
bool CUtlVectorMemory_Growable<T,I,UNK>::IsReadOnly() const
{
	return (m_nGrowSize & EXTERNAL_CONST_BUFFER_MARKER) != 0;
}


template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::SetGrowSize( I nSize )
{
	m_nGrowSize |= nSize & ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);
	ValidateGrowSize();
}


//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
inline T* CUtlVectorMemory_Growable<T,I,UNK>::Base()
{
	Assert( !IsReadOnly() );
	return m_pMemory;
}

template< class T, class I, int UNK >
inline const T *CUtlVectorMemory_Growable<T,I,UNK>::Base() const
{
	return m_pMemory;
}


//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
inline I CUtlVectorMemory_Growable<T,I,UNK>::NumAllocated() const
{
	return m_nAllocationCount;
}

template< class T, class I, int UNK >
inline I CUtlVectorMemory_Growable<T,I,UNK>::Count() const
{
	return m_nAllocationCount;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
inline bool CUtlVectorMemory_Growable<T,I,UNK>::IsIdxValid( I i ) const
{
	// GCC warns if I is an unsigned type and we do a ">= 0" against it (since the comparison is always 0).
	// We get the warning even if we cast inside the expression. It only goes away if we assign to another variable.
	long x = i;
	return ( x >= 0 ) && ( x < m_nAllocationCount );
}

PLATFORM_INTERFACE int		UtlVectorMemory_CalcNewAllocationCount( int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem );
PLATFORM_INTERFACE void*	UtlVectorMemory_Alloc( void* pMem, bool bRealloc, int nNewSize, int nOldSize );
PLATFORM_INTERFACE void		UtlVectorMemory_FailedAllocation( int nTotalElements, int nNewElements );

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::Grow( I num )
{
	Assert( num > 0 );

	if ( IsReadOnly() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	if ( ( ( size_t )m_nAllocationCount + num ) > (std::numeric_limits<I>::max)() )
		UtlVectorMemory_FailedAllocation( m_nAllocationCount, m_nAllocationCount + num );

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	I nAllocationRequested = m_nAllocationCount + num;

	UTLMEMORY_TRACK_FREE();

	I nNewAllocationCount = (I)UtlVectorMemory_CalcNewAllocationCount( m_nAllocationCount, m_nGrowSize & ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER), nAllocationRequested, sizeof(T) );

	// if m_nAllocationRequested wraps index type I, recalculate
	if ( nNewAllocationCount < nAllocationRequested )
	{
		if ( nNewAllocationCount == 0 && ( nNewAllocationCount - 1 ) >= nAllocationRequested )
		{
			--nNewAllocationCount; // deal w/ the common case of m_nAllocationCount == MAX_USHORT + 1
		}
		else
		{
			if ( nAllocationRequested != nAllocationRequested )
			{
				// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
				Assert( 0 );
				return;
			}
			while ( nNewAllocationCount < nAllocationRequested )
			{
				nNewAllocationCount = ( nNewAllocationCount + nAllocationRequested ) / 2;
			}
		}
	}

	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = (T*)UtlVectorMemory_Alloc( m_pMemory, !IsExternallyAllocated(), (int)nNewAllocationCount * sizeof(T), (int)m_nAllocationCount * sizeof(T) );
	Assert( m_pMemory );

	if ( IsExternallyAllocated() )
		m_nGrowSize &= ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);

	m_nAllocationCount = nNewAllocationCount;

	UTLMEMORY_TRACK_ALLOC();
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
inline void CUtlVectorMemory_Growable<T,I,UNK>::EnsureCapacity( I num )
{
	if (m_nAllocationCount >= num)
		return;

	if ( IsReadOnly() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	if(( size_t )num > (std::numeric_limits<I>::max)())
		UtlVectorMemory_FailedAllocation( m_nAllocationCount, num );

	UTLMEMORY_TRACK_FREE();

	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = (T*)UtlVectorMemory_Alloc( m_pMemory, !IsExternallyAllocated(), (int)num * sizeof(T), (int)m_nAllocationCount * sizeof(T) );

	if ( IsExternallyAllocated() )
		m_nGrowSize &= ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);

	m_nAllocationCount = num;

	UTLMEMORY_TRACK_ALLOC();
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::Purge()
{
	if ( !IsExternallyAllocated() )
	{
		if (m_pMemory)
		{
			UTLMEMORY_TRACK_FREE();
			free( (void*)m_pMemory );
			m_pMemory = 0;
		}
		m_nAllocationCount = 0;
	}
}

template< class T, class I, int UNK >
void CUtlVectorMemory_Growable<T,I,UNK>::Purge( I numElements )
{
	Assert( numElements >= 0 );

	if( numElements > m_nAllocationCount )
	{
		// Ensure this isn't a grow request in disguise.
		Assert( numElements <= m_nAllocationCount );
		return;
	}

	// If we have zero elements, simply do a purge:
	if( numElements == 0 )
	{
		Purge();
		return;
	}

	if ( IsReadOnly() )
	{
		// Can't shrink a buffer whose memory was externally allocated, fail silently like purge 
		return;
	}

	// If the number of elements is the same as the allocation count, we are done.
	if( numElements == m_nAllocationCount )
	{
		return;
	}


	if( !m_pMemory )
	{
		// Allocation count is non zero, but memory is null.
		Assert( m_pMemory );
		return;
	}

	UTLMEMORY_TRACK_FREE();

	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = (T*)UtlVectorMemory_Alloc( m_pMemory, !IsExternallyAllocated(), (int)numElements * sizeof(T), (int)m_nAllocationCount * sizeof(T) );

	if ( IsExternallyAllocated() )
		m_nGrowSize &= ~(EXTERNAL_CONST_BUFFER_MARKER | EXTERNAL_BUFFER_MARKER);

	m_nAllocationCount = numElements;

	UTLMEMORY_TRACK_ALLOC();
}

//-----------------------------------------------------------------------------
// The CUtlVectorMemory_Growable class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I = int >
class CUtlVectorMemory_Aligned	: public CUtlVectorMemory_Growable<T, I>
{
	typedef CUtlVectorMemory_Growable<T, I> BaseClass;

public:
	// constructor, destructor
	CUtlVectorMemory_Aligned( I nGrowSize = 0, I nInitSize = 0 );
	CUtlVectorMemory_Aligned( T* pMemory, I numElements );
	CUtlVectorMemory_Aligned( const T* pMemory, I numElements );
	~CUtlVectorMemory_Aligned();

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, I numElements );
	void SetExternalBuffer( const T* pMemory, I numElements );

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( I num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( I num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements (NOT IMPLEMENTED IN CUtlVectorMemory_Aligned)
	void Purge( I numElements )	{ Assert( 0 ); }

private:
	void *Align( const void *pAddr );
};


//-----------------------------------------------------------------------------
// Aligns a pointer
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
void *CUtlVectorMemory_Aligned<T, nAlignment, I>::Align( const void *pAddr )
{
	size_t nAlignmentMask = nAlignment - 1;
	return (void*)( ((intp)pAddr + nAlignmentMask) & (~nAlignmentMask) );
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
CUtlVectorMemory_Aligned<T, nAlignment, I>::CUtlVectorMemory_Aligned( I nGrowSize, I nInitAllocationCount )
{
	BaseClass::m_pMemory = 0; 
	BaseClass::m_nAllocationCount = nInitAllocationCount;
	BaseClass::m_nGrowSize = nGrowSize;
	this->ValidateGrowSize();

	// Alignment must be a power of two
	COMPILE_TIME_ASSERT( (nAlignment & (nAlignment-1)) == 0 );
	Assert( (nGrowSize >= 0) && (nGrowSize & BaseClass::EXTERNAL_BUFFER_MARKER) == 0 );
	if ( BaseClass::m_nAllocationCount )
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		BaseClass::m_pMemory = (T*)_aligned_malloc( (size_t)nInitAllocationCount * sizeof(T), nAlignment );
	}
}

template< class T, int nAlignment, class I >
CUtlVectorMemory_Aligned<T, nAlignment, I>::CUtlVectorMemory_Aligned( T* pMemory, I numElements )
{
	// Special marker indicating externally supplied memory
	BaseClass::m_nGrowSize = CUtlVectorMemory_Growable<T>::EXTERNAL_BUFFER_MARKER;

	BaseClass::m_pMemory = (T*)Align( pMemory );
	BaseClass::m_nAllocationCount = ( (intp)(pMemory + numElements) - (intp)BaseClass::m_pMemory ) / sizeof(T);
}

template< class T, int nAlignment, class I >
CUtlVectorMemory_Aligned<T, nAlignment, I>::CUtlVectorMemory_Aligned( const T* pMemory, I numElements )
{
	// Special marker indicating externally supplied memory
	BaseClass::m_nGrowSize = CUtlVectorMemory_Growable<T>::EXTERNAL_CONST_BUFFER_MARKER;

	BaseClass::m_pMemory = (T*)Align( pMemory );
	BaseClass::m_nAllocationCount = ( (intp)(pMemory + numElements) - (intp)BaseClass::m_pMemory ) / sizeof(T);
}

template< class T, int nAlignment, class I >
CUtlVectorMemory_Aligned<T, nAlignment, I>::~CUtlVectorMemory_Aligned()
{
	Purge();
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
void CUtlVectorMemory_Aligned<T, nAlignment, I>::SetExternalBuffer( T* pMemory, I numElements )
{
	// Blow away any existing allocated memory
	Purge();

	BaseClass::m_pMemory = (T*)Align( pMemory );
	BaseClass::m_nAllocationCount = ( (intp)(pMemory + numElements) - (intp)BaseClass::m_pMemory ) / sizeof(T);

	// Indicate that we don't own the memory
	BaseClass::m_nGrowSize = BaseClass::EXTERNAL_BUFFER_MARKER;
}

template< class T, int nAlignment, class I >
void CUtlVectorMemory_Aligned<T, nAlignment, I>::SetExternalBuffer( const T* pMemory, I numElements )
{
	// Blow away any existing allocated memory
	Purge();

	BaseClass::m_pMemory = (T*)Align( pMemory );
	BaseClass::m_nAllocationCount = ( (intp)(pMemory + numElements) - (intp)BaseClass::m_pMemory ) / sizeof(T);

	// Indicate that we don't own the memory
	BaseClass::m_nGrowSize = BaseClass::EXTERNAL_CONST_BUFFER_MARKER;
}


//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
void CUtlVectorMemory_Aligned<T, nAlignment, I>::Grow( I num )
{
	Assert( num > 0 );

	if ( this->IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	if(((size_t)BaseClass::m_nAllocationCount + num) > (std::numeric_limits<I>::max)())
		UtlVectorMemory_FailedAllocation( BaseClass::m_nAllocationCount, BaseClass::m_nAllocationCount + num );

	UTLMEMORY_TRACK_FREE();

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	I nAllocationRequested = BaseClass::m_nAllocationCount + num;

	BaseClass::m_nAllocationCount = UtlVectorMemory_CalcNewAllocationCount( BaseClass::m_nAllocationCount, BaseClass::m_nGrowSize, nAllocationRequested, sizeof(T) );

	UTLMEMORY_TRACK_ALLOC();

	if (BaseClass::m_pMemory )
	{
		MEM_ALLOC_CREDIT_CLASS();
		BaseClass::m_pMemory = (T*)MemAlloc_ReallocAligned( BaseClass::m_pMemory, (size_t)BaseClass::m_nAllocationCount * sizeof(T), nAlignment );
		Assert( BaseClass::m_pMemory );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		BaseClass::m_pMemory = (T*)MemAlloc_AllocAligned( (size_t)BaseClass::m_nAllocationCount * sizeof(T), nAlignment );
		Assert( BaseClass::m_pMemory );
	}
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
inline void CUtlVectorMemory_Aligned<T, nAlignment, I>::EnsureCapacity( I num )
{
	if ( BaseClass::m_nAllocationCount >= num )
		return;

	if ( this->IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	if((size_t)num > (std::numeric_limits<I>::max)())
		UtlVectorMemory_FailedAllocation( BaseClass::m_nAllocationCount, num );

	UTLMEMORY_TRACK_FREE();

	BaseClass::m_nAllocationCount = num;

	UTLMEMORY_TRACK_ALLOC();

	if (BaseClass::m_pMemory )
	{
		MEM_ALLOC_CREDIT_CLASS();
		BaseClass::m_pMemory = (T*)MemAlloc_ReallocAligned( BaseClass::m_pMemory, (size_t)BaseClass::m_nAllocationCount * sizeof(T), nAlignment );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		BaseClass::m_pMemory = (T*)MemAlloc_AllocAligned( (size_t)BaseClass::m_nAllocationCount * sizeof(T), nAlignment );
	}
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, int nAlignment, class I >
void CUtlVectorMemory_Aligned<T, nAlignment, I>::Purge()
{
	if ( !this->IsExternallyAllocated() )
	{
		if (BaseClass::m_pMemory )
		{
			UTLMEMORY_TRACK_FREE();
			MemAlloc_FreeAligned( BaseClass::m_pMemory );
			BaseClass::m_pMemory = nullptr;
		}
		BaseClass::m_nAllocationCount = 0;
	}
}

#pragma pack(push, 1)
template< class T, class A >
class CUtlVectorMemory_RawAllocator
{
	typedef A CAllocator;

public:
	// constructor, destructor
	CUtlVectorMemory_RawAllocator( int nGrowSize = 0, int nInitSize = 0 );
	CUtlVectorMemory_RawAllocator( T *pMemory, int numElements ) { Assert( 0 ); }
	~CUtlVectorMemory_RawAllocator();

	// Can we use this index?
	bool IsIdxValid( int i ) const						{ return (i >= 0) && (i < NumAllocated()); }
	static int InvalidIndex()							{ return -1; }

	// Gets the base address (can change when adding elements!)
	T* Base()											{ return m_nAllocationCount > 0 ? m_pMemory : nullptr; }
	const T* Base() const								{ return m_nAllocationCount > 0 ? m_pMemory : nullptr; }

	// element access
	T& operator[]( int i )								{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( int i ) const					{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( int i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( int i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements ) { Assert( 0 ); }
	void AssumeMemory( T *pMemory, int nSize );
	T* Detach();
	void *DetachMemory();

	// Fast swap
	void Swap( CUtlVectorMemory_RawAllocator< T, A > &mem );

	// Size
	int NumAllocated() const							{ return m_nAllocationCount; }
	int Count() const									{ return m_nAllocationCount; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements
	void Purge( int numElements );

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	class Iterator_t
	{
	public:
		Iterator_t( int i ) : index( i ) {}
		int index;
		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ return Iterator_t( IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	int GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( int i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex() ); }

private:
	int m_nAllocationCount;
	T* m_pMemory;
};
#pragma pack(pop)


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template< class T, class A >
CUtlVectorMemory_RawAllocator<T, A>::CUtlVectorMemory_RawAllocator( int nGrowSize, int nInitSize )
	: m_nAllocationCount( 0 ), m_pMemory( nullptr )
{
	EnsureCapacity( nInitSize );
}

template< class T, class A >
CUtlVectorMemory_RawAllocator<T, A>::~CUtlVectorMemory_RawAllocator()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Fast swap
//-----------------------------------------------------------------------------
template< class T, class A >
void CUtlVectorMemory_RawAllocator<T, A>::Swap( CUtlVectorMemory_RawAllocator<T, A> &mem )
{
	V_swap( m_pMemory, mem.m_pMemory );
	V_swap( m_nAllocationCount, mem.m_nAllocationCount );
}

template< class T, class A >
void CUtlVectorMemory_RawAllocator<T, A>::AssumeMemory( T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	// Simply take the pointer but don't mark us as external
	m_pMemory = pMemory;
	m_nAllocationCount = numElements;
}

template< class T, class A >
void *CUtlVectorMemory_RawAllocator<T, A>::DetachMemory()
{
	void *pMemory = m_pMemory;
	m_pMemory = 0;
	m_nAllocationCount = 0;
	return pMemory;
}

template< class T, class A >
inline T* CUtlVectorMemory_RawAllocator<T, A>::Detach()
{
	return (T*)DetachMemory();
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template< class T, class A >
void CUtlVectorMemory_RawAllocator<T, A>::Grow( int num )
{
	Assert( num > 0 );
	EnsureCapacity( m_nAllocationCount + num );
}

//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, class A >
inline void CUtlVectorMemory_RawAllocator<T, A>::EnsureCapacity( int num )
{
	if(m_nAllocationCount >= num)
		return;

	int new_alloc_size = CalcNewDoublingCount( m_nAllocationCount, num, 2, INT_MAX );

	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = CAllocator::Realloc( m_pMemory, new_alloc_size, m_nAllocationCount );

	UTLMEMORY_TRACK_ALLOC();
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class A >
void CUtlVectorMemory_RawAllocator<T, A>::Purge()
{
	if (m_nAllocationCount > 0)
	{
		UTLMEMORY_TRACK_FREE();
		CAllocator::Free( m_pMemory );
		m_pMemory = 0;
	}
	
	m_nAllocationCount = 0;
}

template< class T, class A >
void CUtlVectorMemory_RawAllocator<T, A>::Purge( int numElements )
{
	Assert( numElements >= 0 );

	if( numElements > m_nAllocationCount )
	{
		// Ensure this isn't a grow request in disguise.
		Assert( numElements <= m_nAllocationCount );
		return;
	}

	// If we have zero elements, simply do a purge:
	if( numElements == 0 )
	{
		Purge();
		return;
	}

	// If the number of elements is the same as the allocation count, we are done.
	if( numElements == m_nAllocationCount )
	{
		return;
	}


	if( !m_pMemory )
	{
		// Allocation count is non zero, but memory is null.
		Assert( m_pMemory );
		return;
	}

	UTLMEMORY_TRACK_FREE();

	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = CAllocator::Realloc( m_pMemory, numElements, m_nAllocationCount );

	UTLMEMORY_TRACK_ALLOC();
}

#include "tier0/memdbgoff.h"

#endif // UTLVECTORMEMORY_H
