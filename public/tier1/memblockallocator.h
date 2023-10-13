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

#ifndef MEMBLOCKALLOCATOR_H
#define MEMBLOCKALLOCATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"

typedef unsigned int MemBlockHandle_t;

#define MEMBLOCKHANDLE_INVALID ((MemBlockHandle_t)~0)

class CUtlMemoryBlockAllocator
{
public:
	DLL_CLASS_IMPORT	CUtlMemoryBlockAllocator( int nInitPages, unsigned int nPageSize, RawAllocatorType_t eAllocatorType );
	DLL_CLASS_IMPORT	~CUtlMemoryBlockAllocator( void );

	DLL_CLASS_IMPORT	void				RemoveAll( size_t nSize = 0 );
	DLL_CLASS_IMPORT	void				Purge( void );
	DLL_CLASS_IMPORT	MemBlockHandle_t	Alloc( unsigned int nSize );
	DLL_CLASS_IMPORT	MemBlockHandle_t	AllocAndCopy( const char* pBuf, unsigned int nSize );
	DLL_CLASS_IMPORT	uint64				MemUsage( void );
	DLL_CLASS_IMPORT	void				SetPageSize( unsigned int nPageSize );
	DLL_CLASS_IMPORT	MemBlockHandle_t	FindPageWithSpace( unsigned int nSpace );

	void*				GetBlock( MemBlockHandle_t handle ) const;

private:

	struct MemPage_t
	{
		unsigned int	m_nTotalSize;
		unsigned int	m_nUsedSize;
		byte*			m_pMemory;
	};

	typedef CUtlVector<MemPage_t, CUtlMemory_RawAllocator<MemPage_t>> MemPagesVec_t;

	unsigned int			m_nMaxPagesExp;
	unsigned int			m_nPageIndexMask;
	unsigned int			m_nPageIndexShift;
	unsigned int			m_nBlockOffsetMask;
	MemPagesVec_t			m_MemPages;
	unsigned int			m_nPageSize;
	RawAllocatorType_t		m_eRawAllocatorType;
};

inline void* CUtlMemoryBlockAllocator::GetBlock( MemBlockHandle_t handle ) const
{
	int nPageIndex = handle >> m_nPageIndexShift;
	int nBlockOffset = handle & m_nBlockOffsetMask;

	if ( nPageIndex >= 0 && nPageIndex < m_MemPages.Count() )
		return (void*)&m_MemPages[ nPageIndex ].m_pMemory[ nBlockOffset ];

	return NULL;
}

#endif // MEMBLOCKALLOCATOR_H
