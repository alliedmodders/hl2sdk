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

#include "memdbgon.h"

class CRawAllocator
{
public:
	static void* Alloc( size_t nSize, size_t *nAdjustedSize )
	{
		void *ptr = malloc( nSize );
		*nAdjustedSize = MAX( _msize( ptr ), nSize );
		return ptr;
	}

	static void* Realloc( void *base, size_t nSize, size_t *nAdjustedSize )
	{
		void *ptr = realloc( base, nSize );
		*nAdjustedSize = MAX( _msize( ptr ), nSize );
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
