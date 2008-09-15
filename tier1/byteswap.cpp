//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: Low level byte swapping routines.
//
// $NoKeywords: $
//=============================================================================

#include "datamap.h"
#include "byteswap.h"


void WriteField( void* pOutputBuffer, void *pData, typedescription_t *pField )
{
	switch ( pField->fieldType )
	{
	case FIELD_CHARACTER:
		SwapBufferToTargetEndian<char>( (char*)pOutputBuffer, (char*)pData, pField->fieldSize );
		break;

	case FIELD_SHORT:
		SwapBufferToTargetEndian<short>( (short*)pOutputBuffer, (short*)pData, pField->fieldSize );
		break;

	case FIELD_FLOAT:
		SwapBufferToTargetEndian<float>( (float*)pOutputBuffer, (float*)pData, pField->fieldSize );
		break;

	case FIELD_INTEGER:
		SwapBufferToTargetEndian<int>( (int*)pOutputBuffer, (int*)pData, pField->fieldSize );
		break;

	case FIELD_VECTOR:
		SwapBufferToTargetEndian<float>( (float*)pOutputBuffer, (float*)pData, pField->fieldSize * 3 );
		break;

	case FIELD_QUATERNION:
		SwapBufferToTargetEndian<float>( (float*)pOutputBuffer, (float*)pData, pField->fieldSize * 4 );
		break;

	case FIELD_EMBEDDED:
		// Where does the offset happen?
		WriteFields( pOutputBuffer, pData, pField->td->dataDesc, pField->td->dataNumFields );
		break;
		
	default:
		assert(0);
	}
}

void WriteFields( void *pOutputBuffer, void *pBaseData, typedescription_t *pFields, int fieldCount )
{	
	for ( int i = 0; i < fieldCount; ++i )
	{
		typedescription_t *pField = &pFields[i];
		WriteField( (BYTE*)pOutputBuffer + pField->fieldOffset[ TD_OFFSET_NORMAL ],  
			        (BYTE*)pBaseData + pField->fieldOffset[ TD_OFFSET_NORMAL ], 
					pField );
	}
}

//-----------------------------------------------------------------------------
// Determines the target byte ordering we are swapping to.
//-----------------------------------------------------------------------------
static bool g_bTargetLittleEndian = true;

//-----------------------------------------------------------------------------
// Sets the target byte ordering we are swapping to.
//-----------------------------------------------------------------------------
void SetTargetEndian( bool bIsLittleEndian )
{
	g_bTargetLittleEndian = bIsLittleEndian;
}

//-----------------------------------------------------------------------------
// True if the current machine is detected as little endian.
//-----------------------------------------------------------------------------
bool IsLittleEndian()
{
	short nIsLittleEndian = 1;

	// if we are little endian, the first byte will be a 1, if big endian, it will be a zero.
	return (bool)(0 !=  *(char *)&nIsLittleEndian );
}

//-----------------------------------------------------------------------------
// Returns true if the target machine is the same as this one in endianness
// false, if bytes need to be swapped.
//-----------------------------------------------------------------------------
bool IsMachineTargetEndian()
{
	// If we are already in the target endianness, then just return the value:
	if( g_bTargetLittleEndian == IsLittleEndian() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// The lowest level byte swapping workhorse of doom.
//-----------------------------------------------------------------------------
template<class T> T LowLevelByteSwap( T input )
{
	T output = input;	// To solve the "output may not have been initialized" warning.

	for( int i = 0; i < sizeof(T); i++ )
	{
		((unsigned char* )&output)[i] = ((unsigned char*)&input)[sizeof(T)-(i+1)]; 
	}

	return output;
}


//-----------------------------------------------------------------------------
// Returns true if the input is byteswapped relative to the native version of 
// the constant.  
// ( This is useful for detecting byteswapping in magic numbers in structure 
// headers for example. )
//-----------------------------------------------------------------------------
template<class T> bool IsByteSwapped( T input, T nativeConstant )
{
	// If it's the same, it isn't byteswapped:
	if( input == nativeConstant )
		return false;

	if( LowLevelByteSwap<T>(input) == nativeConstant )
		return true;

	// assert( 0 );		// if we get here, input is neither a swapped nor unswapped version of nativeConstant.
	return false;
}

//-----------------------------------------------------------------------------
// Templated swap function for a given type
//-----------------------------------------------------------------------------
template<class T> T SwapToTargetEndian( T input )
{
	if( IsMachineTargetEndian() )
		return input;

	// Otherwise swap it:
	return LowLevelByteSwap<T>( input );
}

//-----------------------------------------------------------------------------
// Swaps an input buffer full of type T into the given output buffer.
//-----------------------------------------------------------------------------
template<class T>  void SwapBufferToTargetEndian( T* outputBuffer, T* inputBuffer, int count )
{
	assert( count >= 0 );
	assert( outputBuffer );

	// Fail gracefully in release:
	if( count <=0 || !outputBuffer )
		return;

	// Optimization for the case when we are swapping in place.
	if( inputBuffer == outputBuffer )
	{
		inputBuffer = NULL;
	}

	// Are we already the correct endienness? ( or are we swapping 1 byte items? )
	if( IsMachineTargetEndian() || ( sizeof(T) == 1 ) )
	{
		// If we were just going to swap in place then return.
		if( !inputBuffer )
			return;
	
		// Otherwise copy the inputBuffer to the outputBuffer:
		memcpy( outputBuffer, inputBuffer, count * sizeof( T ) );
		return;

	}

	// Swap everything in the buffer:
	for( int i = 0; i < count; i++ )
	{
		outputBuffer[i] = LowLevelByteSwap<T>(inputBuffer[i]);
	}
}
