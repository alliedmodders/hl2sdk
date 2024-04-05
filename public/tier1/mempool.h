//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//===========================================================================//

#ifndef MEMPOOL_H
#define MEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/memalloc.h"
#include "tier0/tslist.h"
#include "tier0/platform.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"

//-----------------------------------------------------------------------------
// Purpose: Optimized pool memory allocator
//-----------------------------------------------------------------------------

// Ways the memory pool can grow when it needs to make a new blob.
enum MemoryPoolGrowType_t
{
	UTLMEMORYPOOL_GROW_NONE=0,	// Don't allow new blobs.
	UTLMEMORYPOOL_GROW_FAST=1,	// New blob size is numElements * (i+1)  (ie: the blocks it allocates get larger and larger each time it allocates one).
	UTLMEMORYPOOL_GROW_SLOW=2,	// New blob size is numElements.
};

class CUtlMemoryPoolBase
{
public:
	DLL_CLASS_IMPORT			CUtlMemoryPoolBase( int blockSize, int numElements, int nAlignment = 0, MemoryPoolGrowType_t growMode = UTLMEMORYPOOL_GROW_FAST, const char *pszAllocOwner = NULL, MemAllocAttribute_t allocAttribute = MemAllocAttribute_Unk0 );
	DLL_CLASS_IMPORT			~CUtlMemoryPoolBase();

	// Resets the pool
	DLL_CLASS_IMPORT void		Init( int blockSize, int numElements, int nAlignment, MemoryPoolGrowType_t growMode, const char *pszAllocOwner, MemAllocAttribute_t allocAttribute );

	DLL_CLASS_IMPORT void*		Alloc();	// Allocate the element size you specified in the constructor.
	DLL_CLASS_IMPORT void*		AllocZero();	// Allocate the element size you specified in the constructor, zero the memory before construction
	DLL_CLASS_IMPORT void		Free( void *pMem );
	
	// Frees everything
	void Clear() { ClearDestruct( NULL ); }
	
	// returns number of allocated blocks
	int Count() const		{ return m_BlocksAllocated; }
	int PeakCount() const	{ return m_PeakAlloc; }
	int BlockSize() const	{ return m_BlockSize; }
	int Size() const		{ return m_TotalSize; }

	DLL_CLASS_IMPORT bool		IsAllocationWithinPool( void *pMem ) const;

protected:
	DLL_CLASS_IMPORT void		ClearDestruct( void (*)( void* ) );

private:
	class CBlob
	{
	public:
		CBlob	*m_pNext;
		int		m_NumBytes; // Number of bytes in this blob.
		char	m_Data[1];
		char	m_Padding[3]; // to int align the struct
	};

	DLL_CLASS_IMPORT bool AddNewBlob();
	DLL_CLASS_IMPORT void ResetAllocationCounts();

	int			m_BlockSize;
	int			m_BlocksPerBlob;

	MemoryPoolGrowType_t m_GrowMode;

	CInterlockedInt	m_BlocksAllocated;
	CInterlockedInt	m_PeakAlloc;
	unsigned short	m_nAlignment;
	unsigned short	m_NumBlobs;
	
	CTSListBase		m_FreeBlocks;
	
	MemAllocAttribute_t m_AllocAttribute;
	
	bool 			m_Unk1;
	CThreadMutex	m_Mutex;
	CBlob*			m_pBlobHead;
	int				m_TotalSize;
};


//-----------------------------------------------------------------------------
// Wrapper macro to make an allocator that returns particular typed allocations
// and construction and destruction of objects.
//-----------------------------------------------------------------------------
template< class T >
class CUtlMemoryPool : public CUtlMemoryPoolBase
{
public:
	CUtlMemoryPool( int numElements, MemoryPoolGrowType_t growMode = UTLMEMORYPOOL_GROW_FAST, const char *pszAllocOwner = MEM_ALLOC_CLASSNAME(T), MemAllocAttribute_t allocAttribute = MemAllocAttribute_Unk0 ) 
		: CUtlMemoryPoolBase( sizeof(T), numElements, alignof(T), growMode, pszAllocOwner, allocAttribute ) {}

	T*		Alloc();
	T*		AllocZero();
	void	Free( T *pMem );
	void	Clear();
};


//-----------------------------------------------------------------------------
// Multi-thread/Thread Safe Memory Class
//-----------------------------------------------------------------------------
template< class T >
using CUtlMemoryPoolMT = CUtlMemoryPool<T>;


//-----------------------------------------------------------------------------
// Specialized pool for aligned data management (e.g., Xbox textures)
//-----------------------------------------------------------------------------
template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE = false, int COMPACT_THRESHOLD = 4 >
class CAlignedMemPool
{
	enum
	{
		BLOCK_SIZE = COMPILETIME_MAX( ALIGN_VALUE( ITEM_SIZE, ALIGNMENT ), 8 ),
	};

public:
	CAlignedMemPool();

	void *Alloc();
	void Free( void *p );

	static int __cdecl CompareChunk( void * const *ppLeft, void * const *ppRight );
	void Compact();

	int NumTotal()			{ AUTO_LOCK( m_mutex ); return m_Chunks.Count() * ( CHUNK_SIZE / BLOCK_SIZE ); }
	int NumAllocated()		{ AUTO_LOCK( m_mutex ); return NumTotal() - m_nFree; }
	int NumFree()			{ AUTO_LOCK( m_mutex ); return m_nFree; }

	int BytesTotal()		{ AUTO_LOCK( m_mutex ); return NumTotal() * BLOCK_SIZE; }
	int BytesAllocated()	{ AUTO_LOCK( m_mutex ); return NumAllocated() * BLOCK_SIZE; }
	int BytesFree()			{ AUTO_LOCK( m_mutex ); return NumFree() * BLOCK_SIZE; }

	int ItemSize()			{ return ITEM_SIZE; }
	int BlockSize()			{ return BLOCK_SIZE; }
	int ChunkSize()			{ return CHUNK_SIZE; }

private:
	struct FreeBlock_t
	{
		FreeBlock_t *pNext;
		byte		reserved[ BLOCK_SIZE - sizeof( FreeBlock_t *) ];
	};

	CUtlVector<void *>	m_Chunks;		// Chunks are tracked outside blocks (unlike CUtlMemoryPool) to simplify alignment issues
	FreeBlock_t *		m_pFirstFree;
	int					m_nFree;
	CAllocator			m_Allocator;
	float				m_TimeLastCompact;

	CThreadFastMutex	m_mutex;
};

//-----------------------------------------------------------------------------
// Pool variant using standard allocation
//-----------------------------------------------------------------------------
template <typename T, int nInitialCount = 0, bool bDefCreateNewIfEmpty = true >
class CObjectPool
{
public:
	CObjectPool()
	{
		int i = nInitialCount;
		while ( i-- > 0 )
		{
			m_AvailableObjects.PushItem( new T );
		}
	}

	~CObjectPool()
	{
		Purge();
	}

	int NumAvailable()
	{
		return m_AvailableObjects.Count();
	}

	void Purge()
	{
		T *p;
		while ( m_AvailableObjects.PopItem( &p ) )
		{
			delete p;
		}
	}

	T *GetObject( bool bCreateNewIfEmpty = bDefCreateNewIfEmpty )
	{
		T *p;
		if ( !m_AvailableObjects.PopItem( &p )  )
		{
			p = ( bCreateNewIfEmpty ) ? new T : NULL;
		}
		return p;
	}

	void PutObject( T *p )
	{
		m_AvailableObjects.PushItem( p );
	}

private:
	CTSList<T *> m_AvailableObjects;
};

//-----------------------------------------------------------------------------
// Fixed budget pool with overflow to malloc
//-----------------------------------------------------------------------------
template <size_t PROVIDED_ITEM_SIZE, int ITEM_COUNT>
class CFixedBudgetMemoryPool
{
public:
	CFixedBudgetMemoryPool()
	{
		m_pBase = m_pLimit = 0;
		COMPILE_TIME_ASSERT( ITEM_SIZE % 4 == 0 );
	}

	bool Owns( void *p )
	{
		return ( p >= m_pBase && p < m_pLimit );
	}

	void *Alloc()
	{
		MEM_ALLOC_CREDIT_CLASS();
#ifndef USE_MEM_DEBUG
		if ( !m_pBase )
		{
			LOCAL_THREAD_LOCK();
			if ( !m_pBase )
			{
				byte *pMemory = m_pBase = (byte *)malloc( ITEM_COUNT * ITEM_SIZE );
				m_pLimit = m_pBase + ( ITEM_COUNT * ITEM_SIZE );

				for ( int i = 0; i < ITEM_COUNT; i++ )
				{
					m_freeList.Push( (TSLNodeBase_t *)pMemory );
					pMemory += ITEM_SIZE;
				}
			}
		}

		void *p = m_freeList.Pop();
		if ( p )
			return p;
#endif
		return malloc( ITEM_SIZE );
	}

	void Free( void *p )
	{
#ifndef USE_MEM_DEBUG
		if ( Owns( p ) )
			m_freeList.Push( (TSLNodeBase_t *)p );
		else
#endif
			free( p );
	}

	enum
	{
		ITEM_SIZE = ALIGN_VALUE( PROVIDED_ITEM_SIZE, 4 )
	};

	CTSListBase m_freeList;
	byte *m_pBase;
	byte *m_pLimit;
};

#define BIND_TO_FIXED_BUDGET_POOL( poolName )									\
	inline void* operator new( size_t size ) { return poolName.Alloc(); }   \
	inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { return poolName.Alloc(); }   \
	inline void  operator delete( void* p ) { poolName.Free(p); }		\
	inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { poolName.Free(p); }

//-----------------------------------------------------------------------------


template< class T >
inline T* CUtlMemoryPool<T>::Alloc()
{
	T *pRet;

	{
	MEM_ALLOC_CREDIT_CLASS();
	pRet = (T*)CUtlMemoryPoolBase::Alloc();
	}

	if ( pRet )
	{
		Construct( pRet );
	}
	return pRet;
}

template< class T >
inline T* CUtlMemoryPool<T>::AllocZero()
{
	T *pRet;

	{
	MEM_ALLOC_CREDIT_CLASS();
	pRet = (T*)CUtlMemoryPoolBase::AllocZero();
	}

	if ( pRet )
	{
		Construct( pRet );
	}
	return pRet;
}

template< class T >
inline void CUtlMemoryPool<T>::Free(T *pMem)
{
	if ( pMem )
	{
		Destruct( pMem );
	}

	CUtlMemoryPoolBase::Free( pMem );
}

template< class T >
inline void CUtlMemoryPool<T>::Clear()
{
	CUtlMemoryPoolBase::ClearDestruct( (void (*)( void* ))&Destruct<T> );
}


//-----------------------------------------------------------------------------
// Macros that make it simple to make a class use a fixed-size allocator
// Put DECLARE_FIXEDSIZE_ALLOCATOR in the private section of a class,
// Put DEFINE_FIXEDSIZE_ALLOCATOR in the CPP file
//-----------------------------------------------------------------------------
#define DECLARE_FIXEDSIZE_ALLOCATOR( _class )									\
	public:																		\
		inline void* operator new( size_t size ) { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPool"); return s_Allocator.Alloc(); }   \
		inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPool"); return s_Allocator.Alloc(); }   \
		inline void  operator delete( void* p ) { s_Allocator.Free((_class*)p); }		\
		inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { s_Allocator.Free((_class*)p); }   \
	private:																		\
		static   CUtlMemoryPool<_class>   s_Allocator
    
#define DEFINE_FIXEDSIZE_ALLOCATOR( _class, _initsize, _grow )					\
	CUtlMemoryPool<_class>   _class::s_Allocator(_initsize, _grow, #_class " CUtlMemoryPool", MemAllocAttribute_Unk2)

#define DECLARE_FIXEDSIZE_ALLOCATOR_MT( _class )									\
	public:																		\
	   inline void* operator new( size_t size ) { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPoolMT"); return s_Allocator.Alloc(); }   \
	   inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPoolMT"); return s_Allocator.Alloc(); }   \
	   inline void  operator delete( void* p ) { s_Allocator.Free((_class*)p); }		\
	   inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { s_Allocator.Free((_class*)p); }   \
	private:																		\
		static   CUtlMemoryPoolMT<_class>   s_Allocator

#define DEFINE_FIXEDSIZE_ALLOCATOR_MT( _class, _initsize, _grow )					\
	CUtlMemoryPoolMT<_class>   _class::s_Allocator(_initsize, _grow, #_class " CUtlMemoryPoolMT", MemAllocAttribute_Unk2)

//-----------------------------------------------------------------------------
// Macros that make it simple to make a class use a fixed-size allocator
// This version allows us to use a memory pool which is externally defined...
// Put DECLARE_FIXEDSIZE_ALLOCATOR_EXTERNAL in the private section of a class,
// Put DEFINE_FIXEDSIZE_ALLOCATOR_EXTERNAL in the CPP file
//-----------------------------------------------------------------------------

#define DECLARE_FIXEDSIZE_ALLOCATOR_EXTERNAL( _class )							\
   public:																		\
      inline void* operator new( size_t size )  { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPool"); return s_pAllocator->Alloc(); }   \
      inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )  { MEM_ALLOC_CREDIT_(#_class " CUtlMemoryPool"); return s_pAllocator->Alloc(); }   \
      inline void  operator delete( void* p )   { s_pAllocator->Free((_class*)p); }		\
   private:																		\
      static   CUtlMemoryPool<_class>*   s_pAllocator

#define DEFINE_FIXEDSIZE_ALLOCATOR_EXTERNAL( _class, _allocator )				\
   CUtlMemoryPool<_class>*   _class::s_pAllocator = _allocator


template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE, int COMPACT_THRESHOLD >
inline CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, GROWMODE, COMPACT_THRESHOLD>::CAlignedMemPool()
  : m_pFirstFree( 0 ),
	m_nFree( 0 ),
	m_TimeLastCompact( 0 )
{
	COMPILE_TIME_ASSERT( sizeof( FreeBlock_t ) >= BLOCK_SIZE );
	COMPILE_TIME_ASSERT( ALIGN_VALUE( sizeof( FreeBlock_t ), ALIGNMENT ) == sizeof( FreeBlock_t ) );
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE, int COMPACT_THRESHOLD >
inline void *CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, GROWMODE, COMPACT_THRESHOLD>::Alloc()
{
	AUTO_LOCK( m_mutex );

	if ( !m_pFirstFree )
	{
		if ( !GROWMODE && m_Chunks.Count() )
		{
			return NULL;
		}

		FreeBlock_t *pNew = (FreeBlock_t *)m_Allocator.Alloc( CHUNK_SIZE );
		Assert( (unsigned)pNew % ALIGNMENT == 0 );
		m_Chunks.AddToTail( pNew );
		m_nFree = CHUNK_SIZE / BLOCK_SIZE;
		m_pFirstFree = pNew;
		for ( int i = 0; i < m_nFree - 1; i++ )
		{
			pNew->pNext = pNew + 1;
			pNew++;
		}
		pNew->pNext = NULL;
	}

	void *p = m_pFirstFree;
	m_pFirstFree = m_pFirstFree->pNext;
	m_nFree--;

	return p;
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE, int COMPACT_THRESHOLD >
inline void CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, GROWMODE, COMPACT_THRESHOLD>::Free( void *p )
{
	AUTO_LOCK( m_mutex ); 

	// Insertion sort to encourage allocation clusters in chunks
	FreeBlock_t *pFree = ((FreeBlock_t *)p);
	FreeBlock_t *pCur = m_pFirstFree;
	FreeBlock_t *pPrev = NULL;

	while ( pCur && pFree > pCur )
	{
		pPrev = pCur;
		pCur = pCur->pNext;
	}

	pFree->pNext = pCur;

	if ( pPrev )
	{
		pPrev->pNext = pFree;
	}
	else
	{
		m_pFirstFree = pFree;
	}
	m_nFree++;

	if ( m_nFree >= ( CHUNK_SIZE / BLOCK_SIZE ) * COMPACT_THRESHOLD )
	{
		float time = Plat_FloatTime();
		float compactTime = ( m_nFree >= ( CHUNK_SIZE / BLOCK_SIZE ) * COMPACT_THRESHOLD * 4 ) ? 15.0 : 30.0;
		if ( m_TimeLastCompact > time || m_TimeLastCompact + compactTime < Plat_FloatTime() )
		{
			Compact();
			m_TimeLastCompact = time;
		}
	}
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE, int COMPACT_THRESHOLD >
inline int __cdecl CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, GROWMODE, COMPACT_THRESHOLD>::CompareChunk( void * const *ppLeft, void * const *ppRight )
{
	return ((uintp)*ppLeft) - ((uintp)*ppRight);
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, bool GROWMODE, int COMPACT_THRESHOLD >
inline void CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, GROWMODE, COMPACT_THRESHOLD>::Compact()
{
	FreeBlock_t *pCur = m_pFirstFree;
	FreeBlock_t *pPrev = NULL;

	m_Chunks.Sort( CompareChunk );

#ifdef VALIDATE_ALIGNED_MEM_POOL
	{
		FreeBlock_t *p = m_pFirstFree;
		while ( p )
		{
			if ( p->pNext && p > p->pNext )
			{
				__asm { int 3 }
			}
			p = p->pNext;
		}

		for ( int i = 0; i < m_Chunks.Count(); i++ )
		{
			if ( i + 1 < m_Chunks.Count() )
			{
				if ( m_Chunks[i] > m_Chunks[i + 1] )
				{
					__asm { int 3 }
				}
			}
		}
	}
#endif

	int i;

	for ( i = 0; i < m_Chunks.Count(); i++ )
	{
		int nBlocksPerChunk = CHUNK_SIZE / BLOCK_SIZE;
		FreeBlock_t *pChunkLimit = ((FreeBlock_t *)m_Chunks[i]) + nBlocksPerChunk;
		int nFromChunk = 0;
		if ( pCur == m_Chunks[i] )
		{
			FreeBlock_t *pFirst = pCur;
			while ( pCur && pCur >= m_Chunks[i] && pCur < pChunkLimit )
			{
				pCur = pCur->pNext;
				nFromChunk++;
			}
			pCur = pFirst;

		}

		while ( pCur && pCur >= m_Chunks[i] && pCur < pChunkLimit )
		{
			if ( nFromChunk != nBlocksPerChunk )
			{
				if ( pPrev )
				{
					pPrev->pNext = pCur;
				}
				else
				{
					m_pFirstFree = pCur;
				}
				pPrev = pCur;
			}
			else if ( pPrev )
			{
				pPrev->pNext = NULL;
			}
			else
			{
				m_pFirstFree = NULL;
			}

			pCur = pCur->pNext;
		}

		if ( nFromChunk == nBlocksPerChunk )
		{
			m_Allocator.Free( m_Chunks[i] );
			m_nFree -= nBlocksPerChunk;
			m_Chunks[i] = 0;
		}
	}

	for ( i = m_Chunks.Count() - 1; i >= 0 ; i-- )
	{
		if ( !m_Chunks[i] )
		{
			m_Chunks.FastRemove( i );
		}
	}
}

#endif // MEMPOOL_H
