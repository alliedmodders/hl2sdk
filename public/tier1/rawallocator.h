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

#ifndef RAWALLOCATOR_H
#define RAWALLOCATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "platform.h"
#include "basetypes.h"

#include <limits>

#include "memdbgon.h"

class CMemAllocAllocator
{
public:
	template<typename T, typename I = int>
	static T* Alloc( I nCount, I &nAdjustedCount )
	{
		size_t byte_size = nCount * sizeof( T );
		T *ptr = (T *)malloc( byte_size );
		nAdjustedCount = MIN( (I)(MAX( _msize( ptr ), byte_size ) / sizeof( T )), (std::numeric_limits<I>::max)() );
		return ptr;
	}

	template<typename T, typename I = int>
	static T* Realloc( T *base, I nCount, I &nAdjustedCount )
	{
		size_t byte_size = nCount * sizeof( T );
		T *ptr = (T *)realloc( base, byte_size );
		nAdjustedCount = MIN( (I)(MAX( _msize( ptr ), byte_size ) / sizeof( T )), (std::numeric_limits<I>::max)() );
		return ptr;
	}

	static void Free( void* pMem )
	{
		if(pMem)
			free( pMem );
	}
};

#include "memdbgoff.h"

#endif // RAWALLOCATOR_H
