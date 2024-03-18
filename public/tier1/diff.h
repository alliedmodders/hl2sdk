//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================//

#ifndef DIFF_H
#define DIFF_H

#ifdef _WIN32
#pragma once
#endif

#include "platform.h"

PLATFORM_INTERFACE int FindDiffs(uint8 const *NewBlock, uint8 const *OldBlock,
			  int NewSize, int OldSize, int &DiffListSize,uint8 *Output,uint32 OutSize);

PLATFORM_INTERFACE int FindDiffsForLargeFiles(uint8 const *NewBlock, uint8 const *OldBlock,
						   int NewSize, int OldSize, int &DiffListSize,uint8 *Output,
						   uint32 OutSize,
						   int hashsize=65536);

PLATFORM_INTERFACE void ApplyDiffs(uint8 const *OldBlock, uint8 const *DiffList,
                int OldSize, int DiffListSize, int &ResultListSize,uint8 *Output,uint32 OutSize);

PLATFORM_INTERFACE int FindDiffsLowMemory(uint8 const *NewBlock, uint8 const *OldBlock,
					   int NewSize, int OldSize, int &DiffListSize,uint8 *Output,uint32 OutSize);

#endif // DIFF_H
