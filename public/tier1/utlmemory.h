//===== Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#ifdef _WIN32
#pragma once
#endif

// Source2 replaced CUtlMemory with CUtlVectorMemory. The real classes live in utlvectormemory.h.
// These aliases keep existing consumers building. Prefer CUtlVectorMemory in new code.
#include "tier1/utlvectormemory.h"

template< class T, class I = int >
using CUtlMemory = CUtlVectorMemory< T, I >;

template< class T, size_t SIZE, class I = int >
using CUtlMemoryFixedGrowable = CUtlVectorMemory_FixedGrowable< T, SIZE, I >;

template< typename T, size_t SIZE, int nAlignment = 0 >
using CUtlMemoryFixed = CUtlVectorMemory_Fixed< T, SIZE, nAlignment >;

template< typename T >
using CUtlMemoryConservative = CUtlVectorMemory_Conservative< T >;

template< class T, int nAlignment >
using CUtlMemoryAligned = CUtlVectorMemory_Aligned< T, nAlignment >;

template< class T, class A = CMemAllocAllocator >
using CUtlMemory_RawAllocator = CUtlVectorMemory_RawAllocator< T, A >;

#endif // UTLMEMORY_H
