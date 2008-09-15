//========= Copyright © 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: Low level byte swapping routines.
//
// $NoKeywords: $
//=============================================================================

#ifndef BYTESWAP_H
#define BYTESWAP_H

#if defined(_WIN32)
#pragma once
#endif

void WriteField( void *pData, typedescription_t *pField );
void WriteFields( void *pOutputBuffer, void *pBaseData, typedescription_t *pFields, int fieldCount );

//-----------------------------------------------------------------------------
// Sets the target byte ordering we are swapping to.
//-----------------------------------------------------------------------------
void SetTargetEndian( bool bIsLittleEndian );

//-----------------------------------------------------------------------------
// True if the current machine is detected as little endian.
//-----------------------------------------------------------------------------
bool IsLittleEndian();

//-----------------------------------------------------------------------------
// Returns true if the target machine is the same as this one in endianness
// false, if bytes need to be swapped.
//-----------------------------------------------------------------------------
bool IsMachineTargetEndian();

//-----------------------------------------------------------------------------
// The lowest level byte swapping workhorse of doom.
//-----------------------------------------------------------------------------
template<class T> T LowLevelByteSwap( T input );

//-----------------------------------------------------------------------------
// Returns true if the input is byteswapped relative to the native version of 
// the constant.  
// ( This is useful for detecting byteswapping in magic numbers in structure 
// headers for example. )
//-----------------------------------------------------------------------------
template<class T> bool IsByteSwapped( T input, T nativeConstant );

//-----------------------------------------------------------------------------
// Templated swap function for a given type
//-----------------------------------------------------------------------------
template<class T> T SwapToTargetEndian( T input );

//-----------------------------------------------------------------------------
// Swaps an input buffer full of type T into the given output buffer.
//-----------------------------------------------------------------------------
template<class T>  void SwapBufferToTargetEndian( T* outputBufferInput, T* inputBufferInput = NULL, int count = 1 );

#endif /* !BYTESWAP_H */
