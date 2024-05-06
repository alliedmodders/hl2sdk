#ifndef UTLSCRATCHMEMORY_H
#define UTLSCRATCHMEMORY_H

#if _WIN32
#pragma once
#endif

struct UtlScratchMemoryPoolMark_t
{
	void *m_pBlock;
	int m_nBytesFree;
};

// Chunked memory allocator of a size (m_nBlockSize), where first sizeof(MemoryBlock_t) bytes are allocated for 
// MemoryBlock_t struct, that holds additional information about next chunk in a linked-list manner.
class CUtlScratchMemoryPool
{
public:
	DLL_CLASS_IMPORT CUtlScratchMemoryPool();

	// Creates a scratch memory pool with set blocksize, already allocated memory chunk could be provided via pExternalMem, where nAllocatedBlockSize
	// is a size of an allocated memory, passing the memory of size less than sizeof(MemoryBlock_t) bytes (required for the header) might lead to OOB access.
	// If bSearchAllBlocks is true, it would search all the blocks to find free memory to use, alternatively it would allocate new block if
	// the current one can't fit the required memory.
	DLL_CLASS_IMPORT CUtlScratchMemoryPool( uint32 nBlockSize, uint32 nAllocatedBlockSize = 0, void *pExternalMem = NULL, bool bSearchAllBlocks = true );
	DLL_CLASS_IMPORT ~CUtlScratchMemoryPool();

	// Initializes scratch memory pool with set blocksize, already allocated memory chunk could be provided via pExternalMem, where nAllocatedBlockSize
	// is a size of an allocated memory, passing the memory of size less than sizeof(MemoryBlock_t) bytes (required for the header) might lead to OOB access.
	// If bSearchAllBlocks is true, it would search all the blocks to find free memory to use, alternatively it would allocate new block if
	// the current one can't fit the required memory.
	DLL_CLASS_IMPORT void Init( uint32 nBlockSize, uint32 nAllocatedBlockSize = 0, void *pExternalMem = NULL, bool bSearchAllBlocks = true );

	// Returns info about currently active chunk of memory
	DLL_CLASS_IMPORT UtlScratchMemoryPoolMark_t GetCurrentAllocPoint() const;

	// Allocates a memory within the current chunk, if it fits, or creates a new one instead (would search other chunks if m_bSearchAllBlocks == true)
	DLL_CLASS_IMPORT void *AllocAligned( int nSizeInBytes, int nAlignment );

	// Counts the overall allocated size of all the chunks within this memory pool
	DLL_CLASS_IMPORT size_t AllocSize() const;
	// Counts the overall amount of chunks allocated within this memory pool
	DLL_CLASS_IMPORT size_t AllocationCount() const;
	// Counts the overall free memory available across all the allocated chunks within this memory pool
	DLL_CLASS_IMPORT size_t TotalMemFree() const;

	// Frees all the allocated chunks, leaving only one active chunk
	DLL_CLASS_IMPORT void FreeAll();
	// Frees all the allocated chunks that lead to the mark, would result in a crash if the mark doesn't point to any of the chunks within this memory pool
	DLL_CLASS_IMPORT void FreeToAllocPoint( UtlScratchMemoryPoolMark_t mark );

protected:
	// Allocates new chunk of memory, if nSizeInBytes > (m_nBlockSize / 2) the allocated chunk would be allocated as is without free memory in it (even if it exceeds the m_nBlockSize).
	DLL_CLASS_IMPORT void *AddNewBlock( int nSizeInBytes );

private:
	struct ALIGN16 MemoryBlock_t
	{
		MemoryBlock_t *m_pNext;
		uint32 m_nBytesFree;
		uint32 m_nBlockSize;
		uint32 m_nAllocSize;
	} ALIGN16_POST;

	MemoryBlock_t *m_pFirstBlock;
	uint32 m_nBlockSize;
	bool m_bSearchAllBlocks;
};

template <size_t SIZE>
class CUtlScratchMemoryPoolFixedGrowable : public CUtlScratchMemoryPool
{
public:
	CUtlScratchMemoryPoolFixedGrowable( bool bSearchAllBlocks = true ) : CUtlScratchMemoryPool( SIZE, SIZE, &m_initialAllocationMemory, bSearchAllBlocks ) {}

private:
	unsigned char m_initialAllocationMemory[SIZE];
};


#endif // UTLSCRATCHMEMORY_H