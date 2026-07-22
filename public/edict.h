//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDICT_H
#define EDICT_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"
#include "utlhashtable.h"
#include "utlvector.h"

struct edict_t;

#define FL_EDICT_CHANGED	(1<<0)	// Game DLL sets this when the entity state changes
// Mutually exclusive with FL_EDICT_PARTIAL_CHANGE.

// This is used internally to edict_t to remember that it's carrying a 
// "full change list" - all its properties might have changed their value.
#define FL_FULL_EDICT_CHANGED		(1<<1)

#define FL_EDICT_FULLCHECK	(0<<0)  // call ShouldTransmit() each time, this is a fake flag
#define FL_EDICT_ALWAYS		(1<<2)	// always transmit this entity
#define FL_EDICT_DONTSEND	(1<<3)	// don't transmit this entity
#define FL_EDICT_PVSCHECK	(1<<4)	// always transmit entity, but cull against PVS


// Max # of variable changes we'll track in an entity before we treat it
// like they all changed.
#define MAX_CHANGE_OFFSETS	19
#define MAX_EDICT_CHANGE_INFOS	100

struct OffsetIgnore_t
{
	typedef uint16 OffsetIgnoreValueType_t;

	CUtlHashtable<uint16, empty_t> m_OffsetHashtable;
	OffsetIgnoreValueType_t m_unMaxOffset;
};

struct ChangeAccessorFieldPathIndex_t
{
	ChangeAccessorFieldPathIndex_t() { m_Value = -1; }
	ChangeAccessorFieldPathIndex_t( int32 value ) { m_Value = value; }

	int32 m_Value;
};

struct ChangeAccessorFieldPathIndexInfo_t
{
	struct IgnoreCache_t
	{
		uint16 m_Offsets[16];
		ChangeAccessorFieldPathIndex_t m_FieldPaths[16];
		int m_nCount;
		int m_nFirstElement;
	};

	typedef CUtlLeanVectorFixedGrowable<uint32> PackedFieldPathVec_t;

	PackedFieldPathVec_t m_ChangeAccessorFieldPathIdList;
	const OffsetIgnore_t *m_pBaseOffsetToIgnore;
	CUtlVectorFixedGrowable<const OffsetIgnore_t *, 4> m_OffsetsToIgnoreForPaths;
	IgnoreCache_t m_ShouldIgnore;
	IgnoreCache_t m_ShouldNotIgnore;
};

struct VarChangeInfo_t
{
	ChangeAccessorFieldPathIndex_t m_nRootPathIndex;
	int16 m_nArrayIndex;
	uint32 m_nFieldOffset : 31;
	uint32 m_bResolved : 1;
};


class CEdictChangeInfo
{
public:
	ChangeAccessorFieldPathIndexInfo_t *m_pChangeAccessorFieldPathInfo;
	// Edicts remember the offsets of properties that change 
	VarChangeInfo_t m_ChangeOffsets[MAX_CHANGE_OFFSETS];
	uint32 m_nChangeOffsets;
};

// Shared between engine and game DLL.
class CSharedEdictChangeInfo
{
public:
	CSharedEdictChangeInfo()
	{
		m_iSerialNumber = 1;
	}

	// Matched against edict_t::m_iChangeInfoSerialNumber to determine if its
	// change info is valid.
	unsigned short m_iSerialNumber;

	CEdictChangeInfo m_ChangeInfos[MAX_EDICT_CHANGE_INFOS];
	unsigned short m_nChangeInfos;	// How many are in use this frame.
};
extern CSharedEdictChangeInfo *g_pSharedChangeInfo;

#endif // EDICT_H