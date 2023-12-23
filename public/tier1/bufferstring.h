#ifndef BUFFERSTRING_H
#define BUFFERSTRING_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "strtools.h"
#include "utlstring.h"

class CFormatStringElement;
class IFormatOutputStream;

template<size_t MAX_SIZE, bool AllowHeapAllocation>
class CBufferStringGrowable;

/*
	Main idea of CBufferString is to provide the base class for the CBufferStringGrowable wich implements stack allocation
	with the ability to convert to the heap allocation if allowed.

	Example usage of CBufferStringGrowable class:
 
	* Basic buffer allocation:
	```
		CBufferStringGrowable<256> buff;
		buff.Insert(0, "Hello World!");
		printf("Result: %s\n", buff.Get());
	```
	additionaly the heap allocation of the buffer could be disabled, by providing ``AllowHeapAllocation`` template argument,
	by disabling heap allocation, if the buffer capacity is not enough to perform the operation, the app would exit with an Assert;

	* Additional usage:
	CBufferString::IsStackAllocated() - could be used to check if the buffer is stack allocated;
	CBufferString::IsHeapAllocated() - could be used to check if the buffer is heap allocated;
	CBufferString::Get() - would return a pointer to the data, or an empty string if it's not allocated.

	* Additionaly current length of the buffer could be read via CBufferString::GetTotalNumber()
	and currently allocated amount of bytes could be read via CBufferString::GetAllocatedNumber()

	* Most, if not all the functions would ensure the buffer capacity and enlarge it when needed,
	in case of stack allocated buffers, it would switch to heap allocation instead.
*/

class CBufferString
{
protected:
	// You shouldn't be initializing this class, use CBufferStringGrowable instead.
	CBufferString() {}

public:
	enum EAllocationOption_t
	{
		UNK1 = -1,
		UNK2 = 0,
		UNK3 = (1 << 1),
		UNK4 = (1 << 8),
		UNK5 = (1 << 9),
		ALLOW_HEAP_ALLOCATION = (1 << 31)
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
	DLL_CLASS_IMPORT const char *AppendConcat(const char *, const char *, ...) FMTFUNCTION(3, 4);
	DLL_CLASS_IMPORT const char *AppendConcatV(const char *, const char *, va_list, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *Concat(const char *, const char *, ...) FMTFUNCTION(3, 4);

	DLL_CLASS_IMPORT int AppendFormat(const char *pFormat, ...) FMTFUNCTION(2, 3);
	DLL_CLASS_IMPORT int AppendFormatV(const char *pFormat, va_list pData);

	DLL_CLASS_IMPORT const char *AppendRepeat(char cChar, int nChars, bool bIgnoreAlignment = false);

	// Given a path and a filename, composes "path\filename", inserting the (OS correct) separator if necessary
	DLL_CLASS_IMPORT const char *ComposeFileName(const char *pPath, const char *pFile, char cSeparator);

	DLL_CLASS_IMPORT const char *ConvertIn(unsigned int const *pData, int nSize, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *ConvertIn(wchar_t const *pData, int nSize, bool bIgnoreAlignment = false);

	// Make path end with extension if it doesn't already have an extension
	DLL_CLASS_IMPORT const char *DefaultExtension(const char *extension);

	// Does string end with 'pSuffix'? (case sensitive/insensitive variants)
	DLL_CLASS_IMPORT bool EndsWith(const char *pSuffix) const;
	DLL_CLASS_IMPORT bool EndsWith_FastCaseInsensitive(const char *pSuffix) const;

	// Ensures the nCapacity condition is met and grows the local buffer if needed.
	// Returns pResultingBuffer pointer to the newly allocated data, as well as resulting capacity that was allocated in bytes.
	DLL_CLASS_IMPORT int EnsureCapacity(int nCapacity, char **pResultingBuffer, bool bIgnoreAlignment = false, bool bForceGrow = true);
	DLL_CLASS_IMPORT int EnsureAddedCapacity(int nCapacity, char **pResultingBuffer, bool bIgnoreAlignment = false, bool bForceGrow = true);

	DLL_CLASS_IMPORT char *EnsureLength(int nCapacity, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);
	DLL_CLASS_IMPORT char *EnsureOwnedAllocation(CBufferString::EAllocationOption_t eAlloc);

	DLL_CLASS_IMPORT const char *EnsureTrailingSlash(char cSeparator, bool bDontAppendIfEmpty = true);

	DLL_CLASS_IMPORT const char *ExtendPath(const char *pPath, char cSeparator);

	DLL_CLASS_IMPORT const char *ExtractFileBase(const char *pPath);

	// Copy out the file extension into dest
	DLL_CLASS_IMPORT const char *ExtractFileExtension(const char *pPath);

	// Copy out the path except for the stuff after the final pathseparator
	DLL_CLASS_IMPORT const char *ExtractFilePath(const char *pPath, bool);


	DLL_CLASS_IMPORT const char *ExtractFirstDir(const char *pPath);

	// Force slashes of either type to be = separator character
	DLL_CLASS_IMPORT const char *FixSlashes(char cSeparator = CORRECT_PATH_SEPARATOR);

	// Fixes up a file name, removing dot slashes, fixing slashes, converting to lowercase, etc.
	DLL_CLASS_IMPORT const char *FixupPathName(char cSeparator);

	DLL_CLASS_IMPORT int Format(const char *pFormat, ...) FMTFUNCTION(2, 3);
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

	// If pPath is a relative path, this function makes it into an absolute path
	// using the current working directory as the base, or pStartingDir if it's non-NULL.
	// Returns NULL if it runs out of room in the string, or if pPath tries to ".." past the root directory.
	DLL_CLASS_IMPORT const char *MakeAbsolutePath(const char *pPath, const char *pStartingDir);

	// Same as MakeAbsolutePath, but also does separator fixup
	DLL_CLASS_IMPORT const char *MakeFixedAbsolutePath(const char *pPath, const char *pStartingDir, char cSeparator = CORRECT_PATH_SEPARATOR);
	
	// Creates a relative path given two full paths
	// The first is the full path of the file to make a relative path for.
	// The second is the full path of the directory to make the first file relative to
	// Returns NULL if they can't be made relative (on separate drives, for example)
	DLL_CLASS_IMPORT const char *MakeRelativePath(const char *pFullPath, const char *pDirectory);

	DLL_CLASS_IMPORT void MoveFrom(CBufferString &pOther);

	DLL_CLASS_IMPORT void Purge(int nLength);

	DLL_CLASS_IMPORT char *Relinquish(CBufferString::EAllocationOption_t eAlloc);

	DLL_CLASS_IMPORT const char *RemoveAt(int nIndex, int nChars);
	DLL_CLASS_IMPORT const char *RemoveAtUTF8(int nByteIndex, int nCharacters);

	DLL_CLASS_IMPORT const char *RemoveDotSlashes(char cSeparator);
	DLL_CLASS_IMPORT int RemoveWhitespace();

	DLL_CLASS_IMPORT const char *RemoveFilePath();
	DLL_CLASS_IMPORT const char *RemoveFirstDir(CBufferString *pRemovedDir);
	DLL_CLASS_IMPORT const char *RemoveToFileBase();

	DLL_CLASS_IMPORT bool RemovePartialUTF8Tail(bool);
	DLL_CLASS_IMPORT const char *RemoveTailUTF8(int nIndex);

	DLL_CLASS_IMPORT int Replace(char cFrom, char cTo);
	DLL_CLASS_IMPORT int Replace(const char *pMatch, const char *pReplace, bool bDontUseStrStr = false);

	DLL_CLASS_IMPORT const char *ReplaceAt(int nIndex, int nOldChars, const char *pData, int nDataLen = -1, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *ReplaceAt(int nIndex, const char *pData, int nDataLen = -1, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT const char *ReverseChars(int nIndex, int nChars);

	// Strips any current extension from path and ensures that extension is the new extension
	DLL_CLASS_IMPORT const char *SetExtension(const char *extension);

	DLL_CLASS_IMPORT char *SetLength(int nLen, bool bIgnoreAlignment = false, int *pNewCapacity = NULL);
	DLL_CLASS_IMPORT void SetPtr(char *pBuf, int nBufferChars, int, bool, bool);

	// Frees the buffer (if it was heap allocated) and writes "~DSTRCT" to the local buffer.
	DLL_CLASS_IMPORT void SetUnusable();

	DLL_CLASS_IMPORT const char *ShortenPath(bool);

	DLL_CLASS_IMPORT bool StartsWith(const char *pMatch) const;
	DLL_CLASS_IMPORT bool StartsWith_FastCaseInsensitive(const char *pMatch) const;

	DLL_CLASS_IMPORT const char *StrAppendFormat(const char *pFormat, ...) FMTFUNCTION(2, 3);
	DLL_CLASS_IMPORT const char *StrFormat(const char *pFormat, ...) FMTFUNCTION(2, 3);

	DLL_CLASS_IMPORT const char *StripExtension();
	DLL_CLASS_IMPORT const char *StripTrailingSlash();

	DLL_CLASS_IMPORT void ToLowerFast(int nStart);
	DLL_CLASS_IMPORT void ToUpperFast(int nStart);

	DLL_CLASS_IMPORT const char *Trim(const char *pTrimChars = "\t\r\n ");
	DLL_CLASS_IMPORT const char *TrimHead(const char *pTrimChars = "\t\r\n ");
	DLL_CLASS_IMPORT const char *TrimTail(const char *pTrimChars = "\t\r\n ");

	DLL_CLASS_IMPORT const char *TruncateAt(int nIndex, bool bIgnoreAlignment = false);
	DLL_CLASS_IMPORT const char *TruncateAt(const char *pStr, bool bIgnoreAlignment = false);

	DLL_CLASS_IMPORT int UnicodeCaseConvert(int, EStringConvertErrorPolicy eErrorPolicy);

	// Casts to CBufferStringGrowable. Very dirty solution until someone figures out the sane one.
	template<size_t MAX_SIZE = 8, bool AllowHeapAllocation = true, typename T = CBufferStringGrowable<MAX_SIZE, AllowHeapAllocation>>
	T *ToGrowable()
	{
		return (T *)this;
	}
};

template<size_t MAX_SIZE, bool AllowHeapAllocation = true>
class CBufferStringGrowable : public CBufferString
{
	friend class CBufferString;

public:
	CBufferStringGrowable() : m_nTotalCount(0), m_nAllocated(STACK_ALLOCATION_MARKER | (MAX_SIZE & LENGTH_MASK))
	{
		memset(m_Memory.m_szString, 0, sizeof(m_Memory.m_szString));
		if (AllowHeapAllocation)
		{
			m_nAllocated |= ALLOW_HEAP_ALLOCATION;
		}
	}

	CBufferStringGrowable(const CBufferStringGrowable& other) : m_nTotalCount(0), m_nAllocated(STACK_ALLOCATION_MARKER | (MAX_SIZE & LENGTH_MASK))
	{
		memset(m_Memory.m_szString, 0, sizeof(m_Memory.m_szString));
		if (AllowHeapAllocation)
		{
			m_nAllocated |= ALLOW_HEAP_ALLOCATION;
		}
		MoveFrom(const_cast<CBufferStringGrowable&>(other));
	}

	~CBufferStringGrowable()
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

	inline CBufferStringGrowable& operator=(const CBufferStringGrowable& src)
	{
		MoveFrom(const_cast<CBufferStringGrowable&>(src));
		return *this;
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

	inline bool IsInputStringUnsafe(const char *pData) const
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

		return StringFuncs<char>::EmptyString();
	}

	inline void Clear()
	{
		if (GetAllocatedNumber() != 0)
		{
			if (IsStackAllocated())
				m_Memory.m_szString[0] = '\0';
			else
				m_Memory.m_pString[0] = '\0';
		}
		m_nTotalCount &= ~LENGTH_MASK;
	}

private:
	int m_nTotalCount;
	int m_nAllocated;

	union
	{
		char *m_pString;
		char m_szString[MAX_SIZE];
	} m_Memory;
};

#endif /* BUFFERSTRING_H */