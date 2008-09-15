//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A fast stack memory allocator that uses virtual memory if available
//
//=============================================================================//

#ifndef MEMSTACK_H
#define MEMSTACK_H

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

typedef unsigned MemoryStackMark_t;

class CMemoryStack
{
public:
	CMemoryStack();
	~CMemoryStack();

	bool Init( unsigned maxSize = 0, unsigned commitSize = 0, unsigned initialCommit = 0, unsigned alignment = 16 );
	void Term();

	int GetSize();
	int GetMaxSize();
	int	GetUsed();
	
	void *Alloc( unsigned bytes, const char *pszName = NULL );

	MemoryStackMark_t GetCurrentAllocPoint();
	void FreeToAllocPoint( MemoryStackMark_t mark );
	void FreeAll();
	
	void Access( void **ppRegion, unsigned *pBytes );

	void PrintContents();

	void *GetBase();

private:
	unsigned char *m_pNextAlloc;
	unsigned char *m_pCommitLimit;
	unsigned char *m_pAllocLimit;
	
	unsigned char *m_pBase;

	unsigned	   m_maxSize;
	unsigned	   m_alignment;
#ifdef _WIN32
	unsigned	   m_commitSize;
	unsigned	   m_minCommit;
#endif
};

//-------------------------------------

inline int CMemoryStack::GetMaxSize()
{ 
	return m_maxSize;
}

//-------------------------------------

inline int CMemoryStack::GetUsed() 
{ 
	return ( m_pNextAlloc - m_pBase ); 
}

//-------------------------------------

inline void *CMemoryStack::GetBase()
{
	return m_pBase;
}

//-----------------------------------------------------------------------------
// The CUtlMemoryStack class:
// A fixed memory class
//-----------------------------------------------------------------------------
template< typename T, size_t MAX_SIZE, size_t COMMIT_SIZE = 0, size_t INITIAL_COMMIT = 0 >
class CUtlMemoryStack
{
public:
	// constructor, destructor
	CUtlMemoryStack( int nGrowSize = 0, int nInitSize = 0 )	{ m_MemoryStack.Init( MAX_SIZE, COMMIT_SIZE, INITIAL_COMMIT, 4 ); COMPILE_TIME_ASSERT( sizeof(T) % 4 == 0 );	}
	CUtlMemoryStack( T* pMemory, int numElements )			{ Assert( 0 ); 										}

	// Can we use this index?
	bool IsIdxValid( int i ) const							{ return (i >= 0) && (i < m_nAllocated); }

	// Gets the base address
	T* Base()												{ return (T*)m_MemoryStack.GetBase(); }
	const T* Base() const									{ return (T*)m_MemoryStack.GetBase(); }

	// element access
	T& operator[]( int i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( int i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( int i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( int i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements )	{ Assert( 0 ); }

	// Size
	int NumAllocated() const								{ return m_nAllocated; }
	int Count() const										{ return m_nAllocated; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 )								{ Assert( num > 0 ); m_nAllocated += num; m_MemoryStack.Alloc( num * sizeof(T) ); }

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num )							{ Assert( num <= MAX_SIZE ); if ( m_nAllocated < num ) Grow( num - m_nAllocated ); }

	// Memory deallocation
	void Purge()											{ m_MemoryStack.FreeAll(); }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( int size )							{}

private:
	CMemoryStack m_MemoryStack;
	int m_nAllocated;
};

//-----------------------------------------------------------------------------

#endif // MEMSTACK_H
