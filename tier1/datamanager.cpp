//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "basetypes.h"
#include "datamanager.h"

DECLARE_POINTER_HANDLE( memhandle_t );


CDataManagerBase::CDataManagerBase( unsigned int maxSize )
{
	m_targetMemorySize = maxSize;
	m_memUsed = 0;
	m_lruList = m_memoryLists.CreateList();
	m_lockList = m_memoryLists.CreateList();
	m_freeList = m_memoryLists.CreateList();
	m_listsAreFreed = 0;
}

CDataManagerBase::~CDataManagerBase() 
{
	Assert( m_listsAreFreed );
}

void CDataManagerBase::SetTargetSize( unsigned int targetSize )
{
	m_targetMemorySize = targetSize;
}

// Frees everything!  The LRU AND the LOCKED items.  This is only used to forcibly free the resources,
// not to make space.
void CDataManagerBase::FreeAllLists() 
{
	int node;
	int nextNode;

	node = m_memoryLists.Head(m_lruList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nextNode = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lruList, node );
		FreeByIndex( node );
		node = nextNode;
	}

	node = m_memoryLists.Head(m_lockList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nextNode = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lockList, node );
		m_memoryLists[node].lockCount = 0;
		FreeByIndex( node );
		node = nextNode;
	}
	m_listsAreFreed = true;
}


unsigned int CDataManagerBase::FlushAllUnlocked()
{
	unsigned nBytesInitial = MemUsed_Inline();

	int node = m_memoryLists.Head(m_lruList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		int next = m_memoryLists.Next(node);
		m_memoryLists.Unlink( m_lruList, node );
		FreeByIndex( node );
		node = next;
	}

	return ( nBytesInitial - MemUsed_Inline() );
}

unsigned int CDataManagerBase::FlushToTargetSize()
{
	unsigned nBytesInitial = MemUsed_Inline();
	EnsureCapacity(0);
	return ( nBytesInitial - MemUsed_Inline() );
}

unsigned int CDataManagerBase::FlushAll()
{
	unsigned result = MemUsed_Inline();
	FreeAllLists();
	m_listsAreFreed = false;
	return result;
}

unsigned int CDataManagerBase::Purge( unsigned int nBytesToPurge )
{
	unsigned int nOriginalTargetSize = MemTotal_Inline();
	unsigned int nTempTargetSize = MemUsed_Inline() - nBytesToPurge;
	if ( nTempTargetSize < 0 )
		nTempTargetSize = 0;
	SetTargetSize( nTempTargetSize );
	unsigned result = FlushToTargetSize();
	SetTargetSize( nOriginalTargetSize );
	return result;
}


void CDataManagerBase::DestroyResource( memhandle_t handle )
{
	unsigned short index = FromHandle( handle );
	if ( !m_memoryLists.IsValidIndex(index) )
		return;
	
	Assert( m_memoryLists[index].lockCount == 0  );
	if ( m_memoryLists[index].lockCount )
		BreakLock( handle );
	m_memoryLists.Unlink( m_lruList, index );
	FreeByIndex( index );
}


void *CDataManagerBase::LockResource( memhandle_t handle )
{
	return LockByIndex( FromHandle(handle) );
}

int CDataManagerBase::UnlockResource( memhandle_t handle )
{
	return UnlockByIndex( FromHandle(handle) );
}

void *CDataManagerBase::GetResource_NoLockNoLRUTouch( memhandle_t handle )
{
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		return m_memoryLists[memoryIndex].pStore;
	}
	return NULL;
}


void *CDataManagerBase::GetResource_NoLock( memhandle_t handle )
{
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		TouchByIndex( memoryIndex );
		return m_memoryLists[memoryIndex].pStore;
	}
	return NULL;
}

void CDataManagerBase::TouchResource( memhandle_t handle )
{
	TouchByIndex( FromHandle(handle) );
}

void CDataManagerBase::MarkAsStale( memhandle_t handle )
{
	MarkAsStaleByIndex( FromHandle(handle) );
}

int CDataManagerBase::BreakLock( memhandle_t handle )
{
	unsigned short memoryIndex = FromHandle(handle);
	if ( memoryIndex != m_memoryLists.InvalidIndex() && m_memoryLists[memoryIndex].lockCount )
	{
		int nBroken = m_memoryLists[memoryIndex].lockCount;
		m_memoryLists[memoryIndex].lockCount = 0;
		m_memoryLists.Unlink( m_lockList, memoryIndex );
		m_memoryLists.LinkToTail( m_lruList, memoryIndex );

		return nBroken;
	}
	return 0;
}

int CDataManagerBase::BreakAllLocks()
{
	int nBroken = 0;
	int node;
	int nextNode;

	node = m_memoryLists.Head(m_lockList);
	while ( node != m_memoryLists.InvalidIndex() )
	{
		nBroken++;
		nextNode = m_memoryLists.Next(node);
		m_memoryLists[node].lockCount = 0;
		m_memoryLists.Unlink( m_lockList, node );
		m_memoryLists.LinkToTail( m_lruList, node );
		node = nextNode;
	}

	return nBroken;

}

unsigned short CDataManagerBase::CreateHandle()
{
	int memoryIndex = m_memoryLists.Head(m_freeList);
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		m_memoryLists.Unlink( m_freeList, memoryIndex );
		m_memoryLists.LinkToTail( m_lruList, memoryIndex );
	}
	else
	{
		memoryIndex = m_memoryLists.AddToTail( m_lruList );
	}
	return memoryIndex;
}

memhandle_t CDataManagerBase::StoreResourceInHandle( unsigned short memoryIndex, void *pStore, unsigned int realSize )
{
	resource_lru_element_t &mem = m_memoryLists[memoryIndex];
	mem.pStore = pStore;
	m_memUsed += realSize;
	return ToHandle(memoryIndex);
}

void *CDataManagerBase::LockByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToTail( m_lockList, memoryIndex );
		}
		Assert(m_memoryLists[memoryIndex].lockCount != (unsigned short)-1);
		m_memoryLists[memoryIndex].lockCount++;
		return m_memoryLists[memoryIndex].pStore;
	}

	return NULL;
}

void CDataManagerBase::TouchByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToTail( m_lruList, memoryIndex );
		}
	}
}


void CDataManagerBase::MarkAsStaleByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		if ( m_memoryLists[memoryIndex].lockCount == 0 )
		{
			m_memoryLists.Unlink( m_lruList, memoryIndex );
			m_memoryLists.LinkToHead( m_lruList, memoryIndex );
		}
	}
}

memhandle_t CDataManagerBase::ToHandle( unsigned short index )
{
	unsigned int hiword = m_memoryLists.Element(index).serial;
	hiword <<= 16;
	index++;
	return (memhandle_t)( hiword|index );
}

unsigned int CDataManagerBase::TargetSize() 
{ 
	return MemTotal_Inline(); 
}

unsigned int CDataManagerBase::AvailableSize()
{ 
	return MemAvailable_Inline(); 
}


unsigned int CDataManagerBase::UsedSize()
{ 
	return MemUsed_Inline(); 
}

bool CDataManagerBase::FreeLRU()
{
	int lruIndex = m_memoryLists.Head( m_lruList );
	if ( lruIndex == m_memoryLists.InvalidIndex() )
		return false;
	m_memoryLists.Unlink( m_lruList, lruIndex );
	FreeByIndex( lruIndex );
	return true;
}


// free resources until there is enough space to hold "size"
unsigned int CDataManagerBase::EnsureCapacity( unsigned int size )
{
	unsigned nBytesInitial = MemUsed_Inline();
	while ( MemUsed_Inline() > MemTotal_Inline() || MemAvailable_Inline() < size )
	{
		if ( !FreeLRU() )
			break;
	}
	return ( MemUsed_Inline() - nBytesInitial );
}

// unlock this resource, moving out of the locked list if ref count is zero
int CDataManagerBase::UnlockByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		Assert( m_memoryLists[memoryIndex].lockCount > 0 );
		if ( m_memoryLists[memoryIndex].lockCount > 0 )
		{
			m_memoryLists[memoryIndex].lockCount--;
			if ( m_memoryLists[memoryIndex].lockCount == 0 )
			{
				m_memoryLists.Unlink( m_lockList, memoryIndex );
				m_memoryLists.LinkToTail( m_lruList, memoryIndex );
			}
		}
		return m_memoryLists[memoryIndex].lockCount;
	}

	return 0;
}


// free this resource and move the handle to the free list
void CDataManagerBase::FreeByIndex( unsigned short memoryIndex )
{
	if ( memoryIndex != m_memoryLists.InvalidIndex() )
	{
		Assert( m_memoryLists[memoryIndex].lockCount == 0 );

		resource_lru_element_t &mem = m_memoryLists[memoryIndex];
		unsigned size = GetRealSize( mem.pStore );
		if ( size > m_memUsed )
		{
			ExecuteOnce( Warning( "Data manager 'used' memory incorrect\n" ) );
			size = m_memUsed;
		}
		m_memUsed -= size;
		this->DestroyResourceStorage( mem.pStore );
		mem.pStore = NULL;
		mem.serial++;
		m_memoryLists.LinkToTail( m_freeList, memoryIndex );
	}
}

// get a list of everything in the LRU
void CDataManagerBase::GetLRUHandleList( CUtlVector< memhandle_t >& list )
{
	for ( int node = m_memoryLists.Tail(m_lruList);
			node != m_memoryLists.InvalidIndex();
			node = m_memoryLists.Previous(node) )
	{
		list.AddToTail( ToHandle( node ) );
	}
}

// get a list of everything locked
void CDataManagerBase::GetLockHandleList( CUtlVector< memhandle_t >& list )
{
	for ( int node = m_memoryLists.Head(m_lockList);
			node != m_memoryLists.InvalidIndex();
			node = m_memoryLists.Next(node) )
	{
		list.AddToTail( ToHandle( node ) );
	}
}

