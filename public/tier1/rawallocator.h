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

enum RawAllocatorType_t : uint8
{
	RawAllocator_Standard = 0,
	RawAllocator_Platform = 1,
};

class CRawAllocator
{
public:
	DLL_CLASS_IMPORT static void* Alloc( RawAllocatorType_t eAllocatorType, size_t nSize, size_t* nAdjustedSize );
	DLL_CLASS_IMPORT static void Free( RawAllocatorType_t eAllocatorType, void* pMem, size_t nSize );
};

#endif // RAWALLOCATOR_H
