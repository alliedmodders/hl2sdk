#ifndef BUFFERSTRING_H
#define BUFFERSTRING_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

class CFormatStringElement;
class IFormatOutputStream;
enum EStringConvertErrorPolicy;

/*
	Example usage of CBufferString class:
 
	* Stack buffer allocation:
	```
		CBufferString *buff = BUFFER_STRING_STACK(256);
		buff->Insert(0, "Hello World!");
		printf("Result: %s\n", buff->Get());
	```
	to check if the buffer is stack allocated, CBufferString::IsStackAllocated() should be used.
	NOTE: As the buffer is stack allocated, it would be FREED once it's out of the scope it was defined in!

	* Heap buffer allocation:
	```
		// Regular constructor would provide the heap allocated buffer!!
		CBufferString *buff = new CBufferString(256);
		buff->Insert(0, "Hello World!");
		printf("Result: %s\n", buff->Get());
	```
	alternatively CBufferString could be allocated on stack with the buffer on heap:
	```
		// Regular constructor would provide the heap allocated buffer!!
		CBufferString buff(256);
		buff.Insert(0, "Hello World!");
		printf("Result: %s\n", buff.Get());
	```
	to check if the buffer is heap allocated, CBufferString::IsHeapAllocated() should be used.

	* Calling CBufferString::Get() would return a pointer to the data, or an empty string if it's not allocated.

	* Additionaly current length of the buffer could be read via CBufferString::GetTotalNumber()
	and currently allocated amount of bytes could be read via CBufferString::GetAllocatedNumber()

	* Most, if not all the functions would ensure the buffer capacity and enlarge it when needed,
	in case of stack allocated buffers, it would switch to heap allocation instead.
*/
class CBufferString
{
public:

	CBufferString(int nSize): m_nAllocated(0), m_nTotalCount(0)
	{
		// HACK: Using game's built in allocator on windows to avoid heap corruption issues of reallocs made by the game
#if PLATFORM_WINDOWS
		m_Memory.m_pString = (char *)g_pMemAlloc->IndirectAlloc(nSize);
#else
		m_Memory.m_pString = new char[nSize];
#endif
		m_nAllocated |= HEAP_ALLOCATION_MARKER | (nSize & LENGTH_MASK);
		m_nTotalCount |= HEAP_ALLOCATION_MARKER;
	}

	~CBufferString()
	{
		if (IsHeapAllocated() && m_Memory.m_pString)
		{
#if PLATFORM_WINDOWS
			g_pMemAlloc->Free((void*)m_Memory.m_pString);
#else
			delete[] m_Memory.m_pString;
#endif
		}
	}

	// Expects a stack allocated pointer as a first param, use BUFFER_STRING_STACK macro instead
	static inline CBufferString *InitStackAlloc(void *pData, int nSize)
	{
		CBufferString *buff = (CBufferString *)pData;

		memset(buff, 0, nSize);
		buff->m_nAllocated |= STACK_ALLOCATION_MARKER | (nSize & LENGTH_MASK);
		buff->m_nTotalCount = 0;

		return buff;
	}

	inline int GetAllocatedNumber() const
	{
		return m_nAllocated & LENGTH_MASK;
	}

	inline int GetTotalNumber() const
	{
		return m_nTotalCount & LENGTH_MASK;
	}

	inline bool IsStackAllocated() const
	{
		return (m_nAllocated & STACK_ALLOCATION_MARKER) != 0;
	}

	inline bool IsHeapAllocated() const
	{
		return (m_nTotalCount & HEAP_ALLOCATION_MARKER) != 0;
	}

	inline bool IsInputStringUnsafe(const char *pData)
	{
		return ((void *)pData >= this && (void *)pData < &this[1]) ||
			(GetAllocatedNumber() != 0 && pData >= Get() && pData < (Get() + GetAllocatedNumber()));
	}

	inline const char *Get() const
	{
		if (IsStackAllocated())
		{
			return m_Memory.m_szString;
		}
		else if (GetAllocatedNumber() != 0)
		{
			return m_Memory.m_pString;
		}

		return "";
	}

	enum EAllocationOption_t
	{
		UNK1 = -1,
		UNK2 = 0,
		UNK3 = (1 << 1),
		UNK4 = (1 << 8),
		UNK5 = (1 << 9)
	};

	enum EAllocationFlags_t
	{
		LENGTH_MASK = (1 << 30) - 1,
		FLAGS_MASK = ~LENGTH_MASK,

		STACK_ALLOCATION_MARKER = (1 << 30),
		HEAP_ALLOCATION_MARKER = (1 << 31)
	};

public:
	DLL_CLASS_IMPORT const char *AppendConcat(int, const char * const *, const int *, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *AppendConcat(const char *, const char *, ...);
	DLL_CLASS_IMPORT const char *AppendConcatV(const char *, const char *, char *, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *Concat(const char *, const char *, ...);

	DLL_CLASS_IMPORT int AppendFormat(const char *pFormat, ...);
	DLL_CLASS_IMPORT int AppendFormatV(const char *pFormat, char *pData);

	DLL_CLASS_IMPORT const char *AppendRepeat(char cChar, int nChars, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT const char *ComposeFileName(const char *pPath, const char *pFile, char cSeparator);

	DLL_CLASS_IMPORT const char *ConvertIn(unsigned int const *pData, int nSize, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *ConvertIn(wchar_t const *pData, int nSize, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT const char *DefaultExtension(const char *pExt);

	DLL_CLASS_IMPORT bool EndsWith(const char *pMatch) const;
	DLL_CLASS_IMPORT bool EndsWith_FastCaseInsensitive(const char *pMatch) const;

	// Ensures the nCapacity condition is met and grows the local buffer if needed.
	// Returns pResultingBuffer pointer to the newly allocated data, as well as resulting capacity that was allocated in bytes.
	DLL_CLASS_IMPORT int EnsureCapacity(int nCapacity, char **pResultingBuffer, bool bIgnoreAlignment = false, bool bForceGrow = true);
	DLL_CLASS_IMPORT int EnsureAddedCapacity(int nCapacity, char **pResultingBuffer, bool bIgnoreAlignment = false, bool bForceGrow = true);

	DLL_CLASS_IMPORT char *EnsureLength(int nCapacity, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);
	DLL_CLASS_IMPORT char *EnsureOwnedAllocation(CBufferString::EAllocationOption_t eAlloc);

	DLL_CLASS_IMPORT const char *EnsureTrailingSlash(char cSeparator, bool bDontAppendIfEmpty = true);

	DLL_CLASS_IMPORT const char *ExtendPath(const char *pPath, char cSeparator);

	DLL_CLASS_IMPORT const char *ExtractFileBase(const char *pPath);
	DLL_CLASS_IMPORT const char *ExtractFileExtension(const char *pPath);
	DLL_CLASS_IMPORT const char *ExtractFilePath(const char *pPath, bool);
	DLL_CLASS_IMPORT const char *ExtractFirstDir(const char *pPath);

	DLL_CLASS_IMPORT const char *FixSlashes(char cSeparator);
	DLL_CLASS_IMPORT const char *FixupPathName(char cSeparator);

	DLL_CLASS_IMPORT int Format(const char *pFormat, ...);
	DLL_CLASS_IMPORT void FormatTo(IFormatOutputStream* pOutputStream, CFormatStringElement pElement) const;

protected:
	// Returns aligned size based on capacity requested
	DLL_CLASS_IMPORT static int GetAllocChars(int nSize, int nCapacity);

public:
	// Inserts the nCount bytes of data from pBuf buffer at nIndex position.
	// If nCount is -1, it would count the bytes of the input buffer manualy.
	// Returns the resulting char buffer (Same as to what CBufferString->Get() returns).
	DLL_CLASS_IMPORT const char *Insert(int nIndex, const char *pBuf, int nCount = -1, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT char *GetInsertPtr(int nIndex, int nChars, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);
	DLL_CLASS_IMPORT char *GetReplacePtr(int nIndex, int nOldChars, int nNewChars, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);

	DLL_CLASS_IMPORT int GrowByChunks(int, int);

	// Wrapper around V_MakeAbsolutePath()
	DLL_CLASS_IMPORT const char *MakeAbsolutePath(const char *pPath, const char *pStartingDir);
	// Wrapper around V_MakeAbsolutePath() but also does separator fixup
	DLL_CLASS_IMPORT const char *MakeFixedAbsolutePath(const char *pPath, const char *pStartingDir, char cSeparator);
	// Wrapper around V_MakeRelativePath()
	DLL_CLASS_IMPORT const char *MakeRelativePath(const char *pFullPath, const char *pDirectory);

	DLL_CLASS_IMPORT void MoveFrom(CBufferString& pOther);

	DLL_CLASS_IMPORT void Purge(int nLength);

	DLL_CLASS_IMPORT char *Relinquish(CBufferString::EAllocationOption_t eAlloc);

	DLL_CLASS_IMPORT const char *RemoveAt(int nIndex, int nChars);
	DLL_CLASS_IMPORT const char *RemoveAtUTF8(int nByteIndex, int nCharacters);

	DLL_CLASS_IMPORT const char *RemoveDotSlashes(char cSeparator);
	DLL_CLASS_IMPORT int RemoveWhitespace();

	DLL_CLASS_IMPORT const char *RemoveFilePath();
	DLL_CLASS_IMPORT const char *RemoveFirstDir(CBufferString* pRemovedDir);
	DLL_CLASS_IMPORT const char *RemoveToFileBase();

	DLL_CLASS_IMPORT bool RemovePartialUTF8Tail(bool);
	DLL_CLASS_IMPORT const char *RemoveTailUTF8(int nIndex);

	DLL_CLASS_IMPORT int Replace(char cFrom, char cTo);
	DLL_CLASS_IMPORT int Replace(const char *pMatch, const char *pReplace, bool bDontUseStrStr = false);

	DLL_CLASS_IMPORT const char *ReplaceAt(int nIndex, int nOldChars, const char *pData, int nDataLen = -1, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *ReplaceAt(int nIndex, const char *pData, int nDataLen = -1, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT const char *ReverseChars(int nIndex, int nChars);

	// Appends the pExt to the local buffer, also appends '.' in between even if it wasn't provided in pExt.
	DLL_CLASS_IMPORT const char *SetExtension(const char *pExt);

	DLL_CLASS_IMPORT char *SetLength(int nLen, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);
	DLL_CLASS_IMPORT void SetPtr(char *pBuf, int nBufferChars, int, bool, bool);

	// Frees the buffer (if it was heap allocated) and writes "~DSTRCT" to the local buffer.
	DLL_CLASS_IMPORT void SetUnusable();

	DLL_CLASS_IMPORT const char *ShortenPath(bool);

	DLL_CLASS_IMPORT bool StartsWith(const char *pMatch) const;
	DLL_CLASS_IMPORT bool StartsWith_FastCaseInsensitive(const char *pMatch) const;

	DLL_CLASS_IMPORT const char *StrAppendFormat(const char *pFormat, ...);
	DLL_CLASS_IMPORT const char *StrFormat(const char *pFormat, ...);
							    
	DLL_CLASS_IMPORT const char *StripExtension();
	DLL_CLASS_IMPORT const char *StripTrailingSlash();

	DLL_CLASS_IMPORT void ToLowerFast(int nStart);
	DLL_CLASS_IMPORT void ToUpperFast(int nStart);

	DLL_CLASS_IMPORT const char *Trim(const char *pTrimChars);
	DLL_CLASS_IMPORT const char *TrimHead(const char *pTrimChars);
	DLL_CLASS_IMPORT const char *TrimTail(const char *pTrimChars);

	DLL_CLASS_IMPORT const char *TruncateAt(int nIndex, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *TruncateAt(const char *pStr, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT int UnicodeCaseConvert(int, EStringConvertErrorPolicy eErrorPolicy);

private:
	int m_nTotalCount;
	int m_nAllocated;

	union CBufferStringMemory
	{
		const char *m_pString;
		const char m_szString[];

		CBufferStringMemory() {}
	} m_Memory;
};

#define BUFFER_STRING_STACK(_size) CBufferString::InitStackAlloc(stackalloc(sizeof(CBufferString) + _size), ALIGN_VALUE(sizeof(CBufferString) + _size, 16 ) - sizeof(CBufferString))

#endif /* BUFFERSTRING_H */