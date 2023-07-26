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

// Chunks memory allocator of a size (m_nBlockSize), where first 16 bytes are allocated for 
// MemoryBlock_t struct, that holds additional information about next chunk.
class CUtlScratchMemoryPool
{
public:
	CUtlScratchMemoryPool() : m_pFirstBlock(NULL), m_nBlockSize(0), m_bSearchAllBlocks(false) {}
	CUtlScratchMemoryPool(int nBlockSize, void *pExternalMem, bool bSearchAllBlocks) : 
		m_pFirstBlock(NULL), m_nBlockSize(nBlockSize), m_bSearchAllBlocks(bSearchAllBlocks)
	{
		Init(nBlockSize, pExternalMem, bSearchAllBlocks);
	}

	~CUtlScratchMemoryPool()
	{
		// FreeAll();
	}

	void Init(int nBlockSize, void *pExternalMem, bool bSearchAllBlocks)
	{
		Assert(nBlockSize > sizeof(MemoryBlock_t));

		if(pExternalMem != NULL)
		{
			m_pFirstBlock = (MemoryBlock_t *)pExternalMem;

			m_pFirstBlock->m_pNext = NULL;
			m_pFirstBlock->m_nBytesFree = nBlockSize - 16;
			m_pFirstBlock->m_bSkipDeallocation = true;
		}
	}

	UtlScratchMemoryPoolMark_t GetCurrentAllocPoint() const
	{
		UtlScratchMemoryPoolMark_t result{};
		result.m_pBlock = m_pFirstBlock;

		if(m_pFirstBlock)
			result.m_nBytesFree = m_pFirstBlock->m_nBytesFree;
		else
			result.m_nBytesFree = 0;
		
		return result;
	}

	// GAMMACASE: Not finished functions:
	// void *AddNewBlock(int nSizeInBytes);
	// void *AllocAligned(int nSizeInBytes, int nAlignment);
	// void FreeToAllocPoint(UtlScratchMemoryPoolMark_t mark);
	// void FreeAll();

private:
	struct MemoryBlock_t
	{
		MemoryBlock_t *m_pNext;
		int m_nBytesFree;
		bool m_bSkipDeallocation;
	};

	MemoryBlock_t *m_pFirstBlock;
	int m_nBlockSize;
	bool m_bSearchAllBlocks;
};

template <size_t SIZE>
class CUtlScratchMemoryPoolFixedGrowable : public CUtlScratchMemoryPool
{
public:
	CUtlScratchMemoryPoolFixedGrowable(bool bSearchAllBlocks) : CUtlScratchMemoryPool(SIZE, &m_initialAllocationMemory, bSearchAllBlocks) { }

private:
	unsigned char m_initialAllocationMemory[SIZE];
};


#endif // UTLSCRATCHMEMORY_H