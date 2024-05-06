#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#ifdef _WIN32
#pragma once
#endif

class CCircularBuffer
{
public:
	// Creates circular buffer that works as a FIFO, it would wrap the buffer once it reaches it's size
	// and overwrite past data.
	DLL_CLASS_IMPORT CCircularBuffer();
	// Creates circular buffer that works as a FIFO and allocates a buffer of a size nSize (would use 64 if nSize is less than that),
	// it would wrap the buffer once it reaches it's size and overwrite past data.
	DLL_CLASS_IMPORT CCircularBuffer( uint32 nSize );

	DLL_CLASS_IMPORT ~CCircularBuffer();

	// Resizes internal buffer with nSize size, using 0 as a size would free up the buffer
	DLL_CLASS_IMPORT void Resize( int nSize );

	// Peeks into a buffer and writes to pOut without consuming bytes,
	// number of bytes written is returned
	DLL_CLASS_IMPORT int Peek( void *pOut, int nCount );
	// Reads from the buffer to pOut and consumes bytes,
	// number of bytes written is returned
	DLL_CLASS_IMPORT int Read( void *pOut, int nCount );

	// Consumes nCount bytes from the buffer
	DLL_CLASS_IMPORT int Advance( int nCount );

	// Clears the internal buffer
	DLL_CLASS_IMPORT void Flush();

	// Writes from pData nCount bytes to buffer, leaving it ready for reading
	DLL_CLASS_IMPORT int Write( const void *pData, int nCount );
	// Writes zeroed nCount bytes of data to buffer, leaving it ready for reading
	DLL_CLASS_IMPORT int WriteZeroed( int nCount );

	int GetSize() const { return m_nSize; }
	int GetReadAvailable() const { return m_nCount; }
	int GetWriteAvailable() const { return m_nSize - m_nCount; }

protected:
	DLL_CLASS_IMPORT void AssertValid();

private:
	int m_nCount;

	int m_nRead;
	int m_nWrite;

	int m_nSize;

	void *m_pData;
};

template <typename T, int ELEMENT_COUNT, typename I = int>
class CFixedSizeCircularBuffer
{
public:
	// Creates a circular buffer of a certain type, the buffer works as a LIFO
	CFixedSizeCircularBuffer() : m_Data(), m_nIndex( 0 ), m_nCount( 0 ) {}

	// Called when a new element is written to a buffer
	virtual void ElementAlloc( T &element ) = 0;
	// Called when element is about to be consumed from the buffer
	virtual void ElementRelease( T &element ) = 0;

	I GetReadAvailable() const { return m_nCount; }
	I GetWriteAvailable() const { return ELEMENT_COUNT - m_nCount; }

	// Peeks and reads from the buffer but not consumes from it.
	// Offset could be used to iterate the available for read entries in the buffer.
	// If peek was successful true is returned and data is written to out.
	bool Peek( T *out, I offset = 0 )
	{
		if(m_nCount == 0 || offset < 0 || offset >= m_nCount || !out)
			return false;

		*out = m_Data[(m_nIndex + (m_nCount - 1) - offset) % ELEMENT_COUNT];
		return true;
	}

	// Consumes nCount entries in the buffer, releasing the consumed elements
	// number of consumed items is returned.
	I Advance( I nCount )
	{
		if(m_nCount == 0)
			return 0;

		if(nCount < m_nCount)
		{
			for(I i = 0; i < nCount; i++)
			{
				ElementRelease( m_Data[(m_nIndex + (m_nCount - 1) - i) % ELEMENT_COUNT] );
			}

			m_nCount -= nCount;
		}
		else
		{
			if(m_nCount > 0)
			{
				for(I i = 0; i < m_nCount; i++)
				{
					ElementRelease( m_Data[(m_nIndex + i) % ELEMENT_COUNT] );
				}
			}

			m_nIndex = 0;
			m_nCount = 0;
		}

		return MIN( nCount, m_nCount );
	}

	// Reads from the buffer and consumes if there are entries to read,
	// true is returned if operation was successful and out is filled with the data.
	bool Read( T *out )
	{
		if(!Peek( out ))
			return false;

		Advance( 1 );
		return true;
	}

	// Writes to a buffer and makes the data read ready
	void Write( T &data )
	{
		if(m_nCount == ELEMENT_COUNT)
		{
			ElementRelease( m_Data[m_nIndex] );

			ElementAlloc( m_Data[m_nIndex] );
			m_Data[m_nIndex] = data;

			m_nIndex = (m_nIndex + 1) % ELEMENT_COUNT;
		}
		else
		{
			I idx = (m_nIndex + m_nCount) % ELEMENT_COUNT;

			ElementAlloc( m_Data[idx] );
			m_Data[idx] = data;

			m_nCount++;
		}
	}

private:
	T m_Data[ELEMENT_COUNT];

	I m_nIndex;
	I m_nCount;
};

#endif /* CIRCULARBUFFER_H */