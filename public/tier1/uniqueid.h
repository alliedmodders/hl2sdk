//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//
// Utilities for globally unique IDs
//=============================================================================//

#ifndef UNIQUEID_H
#define UNIQUEID_H

#ifdef _WIN32
#pragma once
#endif

#include "platform.h"
#include "tier1/utlvector.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CUtlBuffer;


//-----------------------------------------------------------------------------
// Defines a globally unique ID
//-----------------------------------------------------------------------------
struct UniqueId_t
{
	unsigned char m_Value[16];
};


//-----------------------------------------------------------------------------
// Methods related to unique ids
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void CreateUniqueId( UniqueId_t *pDest );
PLATFORM_INTERFACE void InvalidateUniqueId( UniqueId_t *pDest );
PLATFORM_INTERFACE bool IsUniqueIdValid( const UniqueId_t &id );
PLATFORM_INTERFACE bool IsUniqueIdEqual( const UniqueId_t &id1, const UniqueId_t &id2 );
PLATFORM_INTERFACE void UniqueIdToString( const UniqueId_t &id, char *pBuf, int nMaxLen );
PLATFORM_INTERFACE bool UniqueIdFromString( UniqueId_t *pDest, const char *pBuf, int nMaxLen = 0 );
PLATFORM_INTERFACE void CopyUniqueId( const UniqueId_t &src, UniqueId_t *pDest );
PLATFORM_INTERFACE bool Serialize( CUtlBuffer &buf, const UniqueId_t &src );
PLATFORM_INTERFACE bool Unserialize( CUtlBuffer &buf, UniqueId_t &dest );

inline bool operator ==( const UniqueId_t& lhs, const UniqueId_t& rhs )
{
	return !Q_memcmp( (void *)&lhs.m_Value[ 0 ], (void *)&rhs.m_Value[ 0 ], sizeof( lhs.m_Value ) );
}


#endif // UNIQUEID_H

