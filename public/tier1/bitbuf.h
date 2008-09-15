//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// NOTE: bf_read is guaranteed to return zeros if it overflows.

#ifndef BITBUF_H
#define BITBUF_H

#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "tier0/dbg.h"


//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------

class Vector;
class QAngle;

class bf_read;


//-----------------------------------------------------------------------------
// You can define a handler function that will be called in case of 
// out-of-range values and overruns here.
//
// NOTE: the handler is only called in debug mode.
//
// Call SetBitBufErrorHandler to install a handler.
//-----------------------------------------------------------------------------

typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE=0,		// Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,				// Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;


typedef void (*BitBufErrorHandler)( BitBufErrorType errorType, const char *pDebugName );


#if defined( _DEBUG )
	extern void InternalBitBufErrorHandler( BitBufErrorType errorType, const char *pDebugName );
	#define CallErrorHandler( errorType, pDebugName ) InternalBitBufErrorHandler( errorType, pDebugName );
#else
	#define CallErrorHandler( errorType, pDebugName )
#endif


// Use this to install the error handler. Call with NULL to uninstall your error handler.
void SetBitBufErrorHandler( BitBufErrorHandler fn );


//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------

inline int BitByte( int bits )
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------

class bf_write
{
public:
					bf_write();
					
					// nMaxBits can be used as the number of bits in the buffer. 
					// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
					bf_write( void *pData, int nBytes, int nMaxBits = -1 );
					bf_write( const char *pDebugName, void *pData, int nBytes, int nMaxBits = -1 );

	// Start writing to the specified buffer.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartWriting( void *pData, int nBytes, int iStartBit = 0, int nMaxBits = -1 );

	// Restart buffer writing.
	void			Reset();

	// Get the base pointer.
	unsigned char*	GetBasePointer() const	{ return m_pData; }

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow( bool bAssert );

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName();
	void			SetDebugName( const char *pDebugName );


// Seek to a specific position.
public:
	
	void			SeekToBit( int bitPos );


// Bit functions.
public:

	void			WriteOneBit(int nValue);
	void			WriteOneBitNoCheck(int nValue);
	void			WriteOneBitAt( int iBit, int nValue );
	
	// Write signed or unsigned. Range is only checked in debug.
	void			WriteUBitLong( unsigned int data, int numbits, bool bCheckRange=true );
	void			WriteSBitLong( int data, int numbits );
	
	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void			WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool			WriteBits(const void *pIn, int nBits);

	// writes an unsigned integer with variable bit length
	void			WriteUBitVar( unsigned int data );

	// Copy the bits straight out of pIn. This seeks pIn forward by nBits.
	// Returns an error if this buffer or the read buffer overflows.
	bool			WriteBitsFromBuffer( bf_read *pIn, int nBits );
	
	void			WriteBitAngle( float fAngle, int numbits );
	void			WriteBitCoord (const float f);
	void			WriteBitFloat(float val);
	void			WriteBitVec3Coord( const Vector& fa );
	void			WriteBitNormal( float f );
	void			WriteBitVec3Normal( const Vector& fa );
	void			WriteBitAngles( const QAngle& fa );


// Byte functions.
public:

	void			WriteChar(int val);
	void			WriteByte(int val);
	void			WriteShort(int val);
	void			WriteWord(int val);
	void			WriteLong(long val);
	void			WriteFloat(float val);
	bool			WriteBytes( const void *pBuf, int nBytes );

	// Returns false if it overflows the buffer.
	bool			WriteString(const char *pStr);


// Status.
public:

	// How many bytes are filled in?
	int				GetNumBytesWritten();
	int				GetNumBitsWritten();
	int				GetMaxNumBits();
	int				GetNumBitsLeft();
	int				GetNumBytesLeft();
	unsigned char*	GetData();

	// Has the buffer overflowed?
	bool			CheckForOverflow(int nBits);
	inline bool		IsOverflowed() const {return m_bOverflow;}

	inline void		SetOverflowFlag();


public:
	// The current buffer.
	unsigned char*	m_pData;
	int				m_nDataBytes;
	int				m_nDataBits;
	
	// Where we are in the buffer.
	int				m_iCurBit;
	
private:

	// Errors?
	bool			m_bOverflow;

	bool			m_bAssertOnOverflow;
	const char		*m_pDebugName;
};


//-----------------------------------------------------------------------------
// Inlined methods
//-----------------------------------------------------------------------------

// How many bytes are filled in?
inline int bf_write::GetNumBytesWritten()	
{
	return BitByte(m_iCurBit);
}

inline int bf_write::GetNumBitsWritten()	
{
	return m_iCurBit;
}

inline int bf_write::GetMaxNumBits()		
{
	return m_nDataBits;
}

inline int bf_write::GetNumBitsLeft()	
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_write::GetNumBytesLeft()	
{
	return GetNumBitsLeft() >> 3;
}

inline unsigned char* bf_write::GetData()			
{
	return m_pData;
}

inline bool bf_write::CheckForOverflow(int nBits)
{
	if ( m_iCurBit + nBits > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
	}
	
	return m_bOverflow;
}

inline void bf_write::SetOverflowFlag()
{
	if ( m_bAssertOnOverflow )
	{
		Assert( false );
	}

	m_bOverflow = true;
}

inline void bf_write::WriteOneBitNoCheck(int nValue)
{
	if(nValue)
		m_pData[m_iCurBit >> 3] |= (1 << (m_iCurBit & 7));
	else
		m_pData[m_iCurBit >> 3] &= ~(1 << (m_iCurBit & 7));

	++m_iCurBit;
}

inline void bf_write::WriteOneBit(int nValue)
{
	if( !CheckForOverflow(1) )
		WriteOneBitNoCheck( nValue );
}


inline void	bf_write::WriteOneBitAt( int iBit, int nValue )
{
	if( iBit+1 > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return;
	}

	if( nValue )
		m_pData[iBit >> 3] |= (1 << (iBit & 7));
	else
		m_pData[iBit >> 3] &= ~(1 << (iBit & 7));
}


inline void bf_write::WriteUBitLong( unsigned int curData, int numbits, bool bCheckRange )
{
#ifdef _DEBUG
	// Make sure it doesn't overflow.
	if ( bCheckRange && numbits < 32 )
	{
		if ( curData >= (unsigned long)(1 << numbits) )
		{
			CallErrorHandler( BITBUFERROR_VALUE_OUT_OF_RANGE, GetDebugName() );
		}
	}
	Assert( numbits >= 0 && numbits <= 32 );
#endif

	extern unsigned long g_BitWriteMasks[32][33];

	// Bounds checking..
	if((m_iCurBit+numbits) > m_nDataBits)
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return;
	}

	int nBitsLeft = numbits;
	int iCurBit = m_iCurBit;

	// Mask in a dword.
	unsigned int iDWord = iCurBit >> 5;
	Assert( (iDWord*4 + sizeof(long)) <= (unsigned int)m_nDataBytes );

	unsigned long iCurBitMasked = iCurBit & 31;
	((unsigned long*)m_pData)[iDWord] &= g_BitWriteMasks[iCurBitMasked][nBitsLeft];
	((unsigned long*)m_pData)[iDWord] |= curData << iCurBitMasked;

	// Did it span a dword?
	int nBitsWritten = 32 - iCurBitMasked;
	if(nBitsWritten < nBitsLeft)
	{
		nBitsLeft -= nBitsWritten;
		iCurBit += nBitsWritten;
		curData >>= nBitsWritten;

		unsigned long iCurBitMasked = iCurBit & 31;
		((unsigned long*)m_pData)[iDWord+1] &= g_BitWriteMasks[iCurBitMasked][nBitsLeft];
		((unsigned long*)m_pData)[iDWord+1] |= curData << iCurBitMasked;
	}

	m_iCurBit += numbits;
}


//-----------------------------------------------------------------------------
// This is useful if you just want a buffer to write into on the stack.
//-----------------------------------------------------------------------------

template<int SIZE>
class bf_write_static : public bf_write
{
public:
	inline bf_write_static() : bf_write(m_StaticData, SIZE) {}

	char	m_StaticData[SIZE];
};



//-----------------------------------------------------------------------------
// Used for unserialization
//-----------------------------------------------------------------------------

class bf_read
{
public:
					bf_read();

					// nMaxBits can be used as the number of bits in the buffer. 
					// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
					bf_read( const void *pData, int nBytes, int nBits = -1 );
					bf_read( const char *pDebugName, const void *pData, int nBytes, int nBits = -1 );

	// Start reading from the specified buffer.
	// pData's start address must be dword-aligned.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartReading( const void *pData, int nBytes, int iStartBit = 0, int nBits = -1 );

	// Restart buffer reading.
	void			Reset();

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow( bool bAssert );

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName();
	void			SetDebugName( const char *pName );

	void			ExciseBits( int startbit, int bitstoremove );


// Bit functions.
public:
	
	// Returns 0 or 1.
	int				ReadOneBit();


protected:

	unsigned int	CheckReadUBitLong(int numbits);		// For debugging.
	int				ReadOneBitNoCheck();				// Faster version, doesn't check bounds and is inlined.
	bool			CheckForOverflow(int nBits);


public:

	// Get the base pointer.
	const unsigned char*	GetBasePointer() const	{ return m_pData; }
		
	// Read a list of bits in..
	bool			ReadBits(void *pOut, int nBits);
	
	float			ReadBitAngle( int numbits );

	unsigned int	ReadUBitLong( int numbits );
	unsigned int	PeekUBitLong( int numbits );
	int				ReadSBitLong( int numbits );

	// reads an unsigned integer with variable bit length
	unsigned int	ReadUBitVar();
	
	// You can read signed or unsigned data with this, just cast to 
	// a signed int if necessary.
	unsigned int	ReadBitLong(int numbits, bool bSigned);
	
	float			ReadBitCoord();
	float			ReadBitFloat();
	float			ReadBitNormal();
	void			ReadBitVec3Coord( Vector& fa );
	void			ReadBitVec3Normal( Vector& fa );
	void			ReadBitAngles( QAngle& fa );


// Byte functions (these still read data in bit-by-bit).
public:
	
	int				ReadChar();
	int				ReadByte();
	int				ReadShort();
	int				ReadWord();
	long			ReadLong();
	float			ReadFloat();
	bool			ReadBytes(void *pOut, int nBytes);

	// Returns false if bufLen isn't large enough to hold the
	// string in the buffer.
	//
	// Always reads to the end of the string (so you can read the
	// next piece of data waiting).
	//
	// If bLine is true, it stops when it reaches a '\n' or a null-terminator.
	//
	// pStr is always null-terminated (unless bufLen is 0).
	//
	// pOutNumChars is set to the number of characters left in pStr when the routine is 
	// complete (this will never exceed bufLen-1).
	//
	bool			ReadString( char *pStr, int bufLen, bool bLine=false, int *pOutNumChars=NULL );

	// Reads a string and allocates memory for it. If the string in the buffer
	// is > 2048 bytes, then pOverflow is set to true (if it's not NULL).
	char*			ReadAndAllocateString( bool *pOverflow = 0 );

// Status.
public:
	int				GetNumBytesLeft();
	int				GetNumBytesRead();
	int				GetNumBitsLeft();
	int				GetNumBitsRead();

	// Has the buffer overflowed?
	inline bool		IsOverflowed() const {return m_bOverflow;}

	bool			Seek(int iBit);					// Seek to a specific bit.
	bool			SeekRelative(int iBitDelta);	// Seek to an offset from the current position.

	// Called when the buffer is overflowed.
	inline void		SetOverflowFlag();


public:

	// The current buffer.
	const unsigned char*	m_pData;
	int						m_nDataBytes;
	int						m_nDataBits;
	
	// Where we are in the buffer.
	int				m_iCurBit;


private:	

	// Errors?
	bool			m_bOverflow;

	// For debugging..
	bool			m_bAssertOnOverflow;

	const char		*m_pDebugName;
};

//-----------------------------------------------------------------------------
// Inlines.
//-----------------------------------------------------------------------------

inline int bf_read::GetNumBytesRead()	
{
	return BitByte(m_iCurBit);
}

inline int bf_read::GetNumBitsLeft()	
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_read::GetNumBytesLeft()	
{
	return GetNumBitsLeft() >> 3;
}

inline int bf_read::GetNumBitsRead()	
{
	return m_iCurBit;
}

inline void bf_read::SetOverflowFlag()
{
	if ( m_bAssertOnOverflow )
	{
		Assert( false );
	}

	m_bOverflow = true;
}

// Seek to an offset from the current position.
inline bool	bf_read::SeekRelative(int iBitDelta)		
{
	return Seek(m_iCurBit+iBitDelta);
}	

inline bool bf_read::CheckForOverflow(int nBits)
{
	if( m_iCurBit + nBits > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
	}

	return m_bOverflow;
}

inline int bf_read::ReadOneBitNoCheck()
{
	int value = m_pData[m_iCurBit >> 3] & (1 << (m_iCurBit & 7));
	++m_iCurBit;
	return !!value;
}

inline int bf_read::ReadOneBit()
{
	return (!CheckForOverflow(1)) ?	ReadOneBitNoCheck() : 0;
}

inline float bf_read::ReadBitFloat()
{
	long val;

	Assert(sizeof(float) == sizeof(long));
	Assert(sizeof(float) == 4);

	if(CheckForOverflow(32))
		return 0.0f;

	int bit = m_iCurBit & 0x7;
	int byte = m_iCurBit >> 3;
	val = m_pData[byte] >> bit;
	val |= ((int)m_pData[byte + 1]) << (8 - bit);
	val |= ((int)m_pData[byte + 2]) << (16 - bit);
	val |= ((int)m_pData[byte + 3]) << (24 - bit);
	if (bit != 0)
		val |= ((int)m_pData[byte + 4]) << (32 - bit);
	m_iCurBit += 32;
	return *((float*)&val);
}


inline unsigned int bf_read::ReadUBitLong( int numbits )
{
	extern unsigned long g_ExtraMasks[32];

	if ( (m_iCurBit+numbits) > m_nDataBits )
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		return 0;
	}

	Assert( numbits > 0 && numbits <= 32 );

	// Read the current dword.
	int idword1 = m_iCurBit >> 5;
	unsigned int dword1 = ((unsigned int*)m_pData)[idword1];
	dword1 >>= (m_iCurBit & 31); // Get the bits we're interested in.

	m_iCurBit += numbits;
	unsigned int ret = dword1;

	// Does it span this dword?
	if ( (m_iCurBit-1) >> 5 == idword1 )
	{
		if(numbits != 32)
			ret &= g_ExtraMasks[numbits];
	}
	else
	{
		int nExtraBits = m_iCurBit & 31;
		unsigned int dword2 = ((unsigned int*)m_pData)[idword1+1] & g_ExtraMasks[nExtraBits];
		
		// No need to mask since we hit the end of the dword.
		// Shift the second dword's part into the high bits.
		ret |= (dword2 << (numbits - nExtraBits));
	}

	return ret;
}


#endif



