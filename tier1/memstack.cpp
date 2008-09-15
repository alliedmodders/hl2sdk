//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#if defined(_WIN32)
#if !defined(_XBOX)
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#define VA_COMMIT_FLAGS MEM_COMMIT
#else
#include <xtl.h>
#define VA_COMMIT_FLAGS (MEM_COMMIT|MEM_NOZERO)
#endif
#endif

#include "tier0/dbg.h"
#include "memstack.h"
#include "utlmap.h"

#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------

MEMALLOC_DEFINE_EXTERNAL_TRACKING(CMemoryStack);

//-------------------------------------

template <typename T>
inline T MemAlign( T val, unsigned alignment )
{
	return (T)( ( (unsigned)val + alignment - 1 ) & ~( alignment - 1 ) );
}

//-----------------------------------------------------------------------------

CMemoryStack::CMemoryStack()
 : 	m_pBase( NULL ),
	m_pNextAlloc( NULL ),
	m_pAllocLimit( NULL ),
	m_pCommitLimit( NULL ),
	m_alignment( 16 ),
#if defined(_WIN32)
 	m_commitSize( 0 ),
	m_minCommit( 0 ),
#endif
 	m_maxSize( 0 )
{
}
	
//-------------------------------------

CMemoryStack::~CMemoryStack()
{
	if ( m_pBase )
		Term();
}

//-------------------------------------

bool CMemoryStack::Init( unsigned maxSize, unsigned commitSize, unsigned initialCommit, unsigned alignment )
{
	Assert( !m_pBase );

	m_maxSize = maxSize;
	m_alignment = MemAlign( alignment, 4 );

	Assert( m_alignment == alignment );
	Assert( m_maxSize > 0 );

#if defined(_WIN32)
	if ( commitSize != 0 )
	{
		m_commitSize = commitSize;
	}

	unsigned pageSize;

#ifndef _XBOX
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	Assert( !( sysInfo.dwPageSize & (sysInfo.dwPageSize-1)) );
	pageSize = sysInfo.dwPageSize;
#else
	pageSize = 4096;
#endif

	if ( m_commitSize == 0 )
	{
		m_commitSize = pageSize;
	}
	else
	{
		m_commitSize = MemAlign( m_commitSize, pageSize );
	}

	m_maxSize = MemAlign( m_maxSize, m_commitSize );
	
	Assert( m_maxSize % pageSize == 0 && m_commitSize % pageSize == 0 && m_commitSize <= m_maxSize );

	m_pBase = (unsigned char *)VirtualAlloc( NULL, m_maxSize, MEM_RESERVE, PAGE_NOACCESS );
	Assert( m_pBase );
	m_pCommitLimit = m_pNextAlloc = m_pBase;

	if ( initialCommit )
	{
		initialCommit = MemAlign( initialCommit, m_commitSize );
		Assert( initialCommit < m_maxSize );
		if ( !VirtualAlloc( m_pCommitLimit, initialCommit, VA_COMMIT_FLAGS, PAGE_READWRITE ) )
			return false;
		m_minCommit = initialCommit;
		m_pCommitLimit += initialCommit;
		MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
	}

#else
	m_pBase = new unsigned char[m_maxSize];
	m_pNextAlloc = m_pBase;
	m_pCommitLimit = m_pBase + m_maxSize;
#endif

	m_pAllocLimit = m_pBase + m_maxSize;

	return ( m_pBase != NULL );
}

//-------------------------------------

void CMemoryStack::Term()
{
	FreeAll();
	if ( m_pBase )
	{
#if defined(_WIN32)
		VirtualFree( m_pBase, 0, MEM_RELEASE );
#else
		delete m_pBase;
#endif
		m_pBase = NULL;
	}
}

//-------------------------------------

int CMemoryStack::GetSize()
{ 
#ifdef _WIN32
	return m_pCommitLimit - m_pBase; 
#else
	return m_maxSize;
#endif
}


//-------------------------------------

void *CMemoryStack::Alloc( unsigned bytes, const char *pszName )
{
	Assert( m_pBase );
	
	if ( !bytes )
		bytes = 1;

	bytes = MemAlign( bytes, m_alignment );

	void *pResult = m_pNextAlloc;
	m_pNextAlloc += bytes;
	
	if ( m_pNextAlloc > m_pCommitLimit )
	{
#if defined(_WIN32)
		unsigned char *	pNewCommitLimit = MemAlign( m_pNextAlloc, m_commitSize );
		unsigned 		commitSize 		= pNewCommitLimit - m_pCommitLimit;
		
		MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

		Assert( m_pCommitLimit + commitSize < m_pAllocLimit );
		if ( !VirtualAlloc( m_pCommitLimit, commitSize, VA_COMMIT_FLAGS, PAGE_READWRITE ) )
		{
			Assert( 0 );
			return NULL;
		}
		m_pCommitLimit = pNewCommitLimit;

		MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
#else
		Assert( 0 );
		return NULL;
#endif
	}

	memset( pResult, 0, bytes );
	
	return pResult;
}

//-------------------------------------

MemoryStackMark_t CMemoryStack::GetCurrentAllocPoint()
{
	return ( m_pNextAlloc - m_pBase );
}

//-------------------------------------

void CMemoryStack::FreeToAllocPoint( MemoryStackMark_t mark )
{
	void *pAllocPoint = m_pBase + mark;
	Assert( pAllocPoint >= m_pBase && pAllocPoint <= m_pNextAlloc );
	
	if ( pAllocPoint >= m_pBase && pAllocPoint < m_pNextAlloc )
	{
#if defined(_WIN32)
		unsigned char *pDecommitPoint = MemAlign( (unsigned char *)pAllocPoint, m_commitSize );

		if ( pDecommitPoint < m_pBase + m_minCommit )
		{
			pDecommitPoint = m_pBase + m_minCommit;
		}

		unsigned decommitSize = m_pCommitLimit - pDecommitPoint;
		
		if ( decommitSize > 0 )
		{
			MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

			VirtualFree( pDecommitPoint, decommitSize, MEM_DECOMMIT );
			m_pCommitLimit = pDecommitPoint;

			if ( mark > 0 )
			{
				MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
			}
		}
#endif
		m_pNextAlloc = (unsigned char *)pAllocPoint;
	}
}

//-------------------------------------

void CMemoryStack::FreeAll()
{
	if ( m_pBase && m_pCommitLimit - m_pBase > 0 )
	{
#if defined(_WIN32)
		MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

		VirtualFree( m_pBase, m_pCommitLimit - m_pBase, MEM_DECOMMIT );
		m_pCommitLimit = m_pBase;
#endif
		m_pNextAlloc = m_pBase;
	}
}

//-------------------------------------

void CMemoryStack::Access( void **ppRegion, unsigned *pBytes )
{
	*ppRegion = m_pBase;
	*pBytes = ( m_pNextAlloc - m_pBase);
}

//-------------------------------------

void CMemoryStack::PrintContents()
{
	Msg( "Total used memory:      %d", GetUsed() );
	Msg( "Total committed memory: %d", GetSize() );
}

//-----------------------------------------------------------------------------
