// JCalg ultra small, ultra fast compression / decompression lib.
// Documentation available here: http://bitsum.com/jcalg1.htm
// 
// Licence claims to be "This library may be used freely in any and for any application."
//
#ifndef __JCALG1_H__
#define __JCALG1_H__

typedef bool (__stdcall *PFNCALLBACKFUNC)(DWORD, DWORD);
typedef void * (__stdcall *PFNALLOCFUNC)(DWORD);
typedef bool (__stdcall *PFNDEALLOCFUNC)(void *);

struct _JCALG1_Info
{
	DWORD majorVer;
	DWORD minorVer;
	DWORD nFastSize;
	DWORD nSmallSize;
};


extern "C" DWORD _stdcall JCALG1_Compress(
	const void *Source,
	DWORD Length,
	void *Destination,
	DWORD WindowSize,
	PFNALLOCFUNC,
	PFNDEALLOCFUNC,
	PFNCALLBACKFUNC,
	BOOL bDisableChecksum);

extern "C" DWORD _stdcall JCALG1_Decompress_Fast(
	const void *Source,
	void *Destination);

extern "C" DWORD _stdcall JCALG1_Decompress_Small(
	const void *Source,
	void *Destination);

extern "C" DWORD _stdcall JCALG1_GetNeededBufferSize(
	DWORD nSize);

extern "C" DWORD _stdcall JCALG1_GetInfo(
	_JCALG1_Info *JCALG1_Info);

extern "C" DWORD _stdcall JCALG1_GetUncompressedSizeOfCompressedBlock(
	const void *pBlock);

extern "C" DWORD _stdcall JCALG1_GetNeededBufferSize(
	DWORD nSize);



#pragma pack(1)
struct xCompressHeader
{
	enum
	{
		MAGIC = 'xCmp',
		VERSION = 1,

	};

	unsigned nMagic;					// = MAGIC;
	unsigned nVersion;					// = VERSION;
	unsigned nUncompressedFileSize;		// Size of the decompressed file.
	unsigned nReadBlockSize;			// Block size this file was optimized for reading.
	unsigned nDecompressionBufferSize;	// Minimum decompression buffer size needed to safely decompress this file.
	unsigned nWindowSize;
};

// The 'simple' xCompress header for compressing small blobs of memory.  
// Doesn't deal with alignment, block read size optimization or window size.  Just decompresses the whole thing.
struct xCompressSimpleHeader
{
	enum
	{
		MAGIC = 'xSmp',
	};

	unsigned nMagic;					// = MAGIC
	unsigned nUncompressedSize;			// Size of the decompressed memory.
};

// NJS: Jcalg helper functions:
// Decompress a chunk of a buffer compressed via xCompress.
// What you want to do here is read a 2 meg chunk from the file and pass it to this function with an nMaxOutput that is at least 4x nInputLength
// the return value will be the number of bytes decompressed into the output buffer, or 0xFFFFFFFF if the output buffer overflowed.
inline unsigned JCALG1_Decompress_Formatted_Buffer( unsigned nInputLength, void* pInput, unsigned nMaxOutput, void* pOutput )
{
	unsigned outputBufferLength = 0;
	for( char* pInputPointer = (char*)pInput; pInputPointer < (char*)pInput+nInputLength; )
	{
		// Get the size of the next chunk:
		unsigned short bytesThisBlock = *(unsigned short*)pInputPointer;
		
		// A zero sized byte block indicates I've hit the end of the buffer:
		if( !bytesThisBlock )
			break;

		pInputPointer += sizeof(unsigned short);

		// Is this block compressed?
		if( bytesThisBlock & 0x8000 )
		{
			bytesThisBlock &= ~0x8000;

			// No it isn't just copy it:
			memcpy((char*)pOutput + outputBufferLength, pInputPointer, bytesThisBlock );

			outputBufferLength += bytesThisBlock;
			pInputPointer += bytesThisBlock;
		}
		else
		{
			unsigned bytesDecompressed = JCALG1_Decompress_Fast( pInputPointer, (char*)pOutput + outputBufferLength);

			outputBufferLength += bytesDecompressed;
			pInputPointer += bytesThisBlock;
		}

		if( outputBufferLength > nMaxOutput )
		{
			return 0xFFFFFFFF;
		}
	}

	return outputBufferLength;
}

// Decompress a 'simple' jcalg buffer.
inline unsigned JCALG1_Decompress_Simple_Buffer( void* pInput, void* pOutput )
{
	if( !pInput || !pOutput )
		return 0;

	xCompressSimpleHeader* header = (xCompressSimpleHeader*)pInput;

	if( header->nMagic != xCompressSimpleHeader::MAGIC )
	{
		return 0;
	}

	return  JCALG1_Decompress_Fast( (char*)pInput+sizeof(xCompressSimpleHeader), pOutput );
}

#pragma pack()

#endif







